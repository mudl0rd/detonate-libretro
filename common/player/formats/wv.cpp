#include "audiodecode.h"
#include "wavpack/wavpack.h"

#define NUM_FRAMES 1024

class auddecode_wv : public auddecode
{
private:
    bool isplaying2;
    WavpackContext *wpc;
    bool repeat;
    int sample_r;
    int bits;

    inline void convintfloatwv(float *dst, const int32_t *src, const size_t N, int bits)
    {
        if (bits == 16)
            for (size_t i = 0; i < N; ++i)
                dst[i] = int16_to_float32(src[i]);
        else if (bits == 24)
        {
            const uint8_t *ptr = reinterpret_cast<const uint8_t *>(src);
            size_t c = 0;
            for (size_t i = 0; i < N; ++i)
            {
                int32_t sample = Pack(ptr[c], ptr[c + 1], ptr[c + 2]);
                dst[i] = int24_to_float32(sample);
                c += 4;
            }
        }
        else if (bits == 32)
            for (size_t i = 0; i < N; ++i)
                dst[i] = int32_to_float32(src[i]);
    }

public:
    ~auddecode_wv()
    {
        wpc = NULL;
    }

    auddecode_wv()
    {
        isplaying2 = false;
        wpc = NULL;
    }

    bool open(const char *filename, float *samplerate, bool loop)
    {
        char error[128] = {0};
        int open_flags = OPEN_DSD_AS_PCM | OPEN_NORMALIZE | OPEN_WVC | OPEN_2CH_MAX;
        wpc = WavpackOpenFileInput(filename, error, open_flags, 0);
        if (!wpc)
        {
            return false;
        }
        bits = WavpackGetBitsPerSample(wpc);
        int nch = WavpackGetReducedChannels(wpc);
        if (nch > 2)
        {
            WavpackCloseFile(wpc);
            return false;
        }
        sample_r = WavpackGetSampleRate(wpc);
        *samplerate = sample_r;
        repeat = loop;
        isplaying2 = true;
        return true;
    }

    virtual void seek(unsigned ms)
    {
        uint64_t index = ms;
        if (isplaying2)
        {
            index *= sample_r;
            index /= 1000;
            WavpackSeekSample64(wpc, index);
        }
    }

    void stop()
    {
        if (isplaying2)
        {
            isplaying2 = false;
            if (wpc)
                WavpackCloseFile(wpc);
        }
    }

    bool is_playing()
    {
        return isplaying2;
    }

    unsigned song_duration()
    {

        int64_t total_samps = WavpackGetNumSamples64(wpc);
        return static_cast<uint32_t>((1000ull * total_samps) / sample_r);
    }

    const char *song_title()
    {
        return NULL;
    }

    const char *file_types()
    {
        return "wv";
    }

    void mix(float *&buffer_samps, unsigned &count)
    {
        float temp_buffer[NUM_FRAMES * 4 * sizeof(float)] = {0};
        int32_t temp_bufferint[NUM_FRAMES * 4 * sizeof(int32_t)] = {0};
        unsigned temp_samples = 0;
        if (isplaying2)
        {
        again:
            int mode = WavpackGetMode(wpc);
            if (MODE_FLOAT & mode)
                temp_samples = WavpackUnpackSamples(wpc, reinterpret_cast<int32_t *>(&temp_buffer[0]), NUM_FRAMES);
            else
            {
                temp_samples = WavpackUnpackSamples(wpc, temp_bufferint, NUM_FRAMES);
                convintfloatwv(temp_buffer, temp_bufferint, NUM_FRAMES * 2, bits);
            }
            if (temp_samples == 0)
            {
                if (repeat)
                {
                    WavpackSeekSample64(wpc, 0);
                    goto again;
                }
                isplaying2 = false;
            }
            buffer_samps = temp_buffer;
            count = temp_samples;
        }
    }
};

auddecode *create_wv()
{
    return new auddecode_wv;
}