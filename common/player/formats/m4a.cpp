#include "audiodecode.h"
#define MINIMP4_IMPLEMENTATION
#include "minimp4.h"
#include <fdk-aac/aacdecoder_lib.h>
#include "mudutils/utils.h"

#define NUM_FRAMES 2048

typedef struct
{
    uint8_t *buffer;
    ssize_t size;
} INPUT_BUFFER;

static int read_callback(int64_t offset, void *buffer, size_t size, void *token)
{
    INPUT_BUFFER *buf = (INPUT_BUFFER *)token;
    size_t to_copy = MINIMP4_MIN(size, buf->size - offset - size);
    memcpy(buffer, buf->buffer + offset, to_copy);
    return to_copy != size;
}

class auddecode_m4a : public auddecode
{
private:
    bool isplaying2;
    MP4D_demux_t demux;
    HANDLE_AACDECODER dec;
    bool repeat;
    uint64_t sample_cnt;
    float snglength;
    unsigned long srate;
    unsigned char nch;
   uint8_t *m4a_ptr;
    std::vector<uint8_t> m4a_data;

public:
    ~auddecode_m4a()
    {
        demux = {
            0,
        };
    }

    auddecode_m4a()
    {
        isplaying2 = false;
        demux = {
            0,
        };
    }

    bool open(const char *filename, float *samplerate, bool loop)
    {
        m4a_data = load_data(filename);
        INPUT_BUFFER buf = {m4a_data.data(), m4a_data.size()};
        if (!MP4D_open(&demux, read_callback, &buf, m4a_data.size()))
            return false;
        sample_cnt = 0;
        dec = aacDecoder_Open(TT_MP4_RAW, 1);
        UCHAR *dsi = (UCHAR *)demux.track[0].dsi;
        UINT dsi_size = demux.track[0].dsi_bytes;
        if (AAC_DEC_OK != aacDecoder_ConfigRaw(dec, &dsi, &dsi_size))return false;
        srate=demux.track[0].SampleDescription.audio.samplerate_hz;
        nch=demux.track[0].SampleDescription.audio.channelcount;
        m4a_ptr=m4a_data.data();
        double timescale_rcp = 1.0 / double(demux.track[0].timescale);
        uint64_t duration =  (uint64_t)demux.track[0].duration_hi << 32 | demux.track[0].duration_lo;
        snglength =  int(double(duration*timescale_rcp))*1000ull;
        *samplerate = srate;
        repeat = loop;
        isplaying2 = true;
        return true;
    }

    virtual void seek(unsigned ms)
    {
        uint64_t index = ms;
        index *= srate;
        index /= 1024;
        sample_cnt = index;
    }

    void stop()
    {
        if (isplaying2)
        {
            isplaying2 = false;
            aacDecoder_Close(dec);
            MP4D_close(&demux);
        }
    }

    bool is_playing()
    {
        return isplaying2;
    }

    unsigned song_duration()
    {

        return unsigned(snglength);
    }

    const char *song_title()
    {
        return NULL;
    }

    const char *file_types()
    {
        return "m4a";
    }

    void mix(float *&buffer_samps, unsigned &count)
    {
        float temp_buffer[NUM_FRAMES * 4 * sizeof(float)] = {0};
        int16_t temp_bufferint[NUM_FRAMES * 4 * sizeof(int32_t)] = {0};
        unsigned temp_samples = 0;

        if (isplaying2)
        {
            if (demux.track[0].handler_type == MP4D_HANDLER_TYPE_SOUN)
            {
                again:
                if (sample_cnt < demux.track[0].sample_count)
                {
                    CStreamInfo* info;
                    unsigned frame_bytes, timestamp, duration;
                    MP4D_file_offset_t ofs = MP4D_frame_offset(&demux, 0, sample_cnt++, &frame_bytes, &timestamp, &duration);
                    UCHAR *frame = (UCHAR *)(m4a_ptr + ofs);
                    UINT frame_size = frame_bytes;
                    UINT valid = frame_size;
                    AAC_DECODER_ERROR aac_err=aacDecoder_Fill(dec, &frame, &frame_size, &valid);
                    if(aac_err != AAC_DEC_OK)
                    goto bail;
                    aac_err = aacDecoder_DecodeFrame(dec,temp_bufferint, sizeof(temp_bufferint), 0);
                    if(aac_err != AAC_DEC_OK)
                    goto bail;
                    info = aacDecoder_GetStreamInfo(dec);
                    if(info->frameSize)
                    {
                       conv2float(temp_buffer, (uint8_t *)temp_bufferint, NUM_FRAMES * 2, sampfmt::P16);
                       temp_samples=info->frameSize;
                    }    
                 }
                else{
                if (repeat){
                    sample_cnt=0;
                    goto again;
                }
                bail:
                isplaying2 = false;
                 }
            }
        }
        buffer_samps = temp_buffer;
        count = temp_samples;
    }
};

auddecode *create_m4a()
{
    return new auddecode_m4a;
}