#include "audiodecode.h"
#include "musepack/mpcdec.h"
#include "musepack/reader.h"
#include "musepack/internal.h"

#define NUM_FRAMES 1024

class auddecode_mpc : public auddecode
{

private:
    bool isplaying2;
    mpc_reader m_reader;
    mpc_demux *demux;
    FILE *filez;
    bool repeat;
    int sample_r;

public:
    ~auddecode_mpc()
    {
        demux = NULL;
        filez = NULL;
    }

    auddecode_mpc()
    {
        isplaying2 = false;
        demux = NULL;
        filez = NULL;
    }

    bool open(const char *filename, float *samplerate, bool loop)
    {
        filez = fopen(filename, "rb");
        mpc_reader_init_stdio_stream(&m_reader, filez);
        demux = mpc_demux_init(&m_reader);
        if (!demux)
        {
            mpc_reader_exit_stdio(&m_reader);
            return false;
        }
        mpc_streaminfo info = {};
        mpc_demux_get_info(demux, &info);
        *samplerate = info.sample_freq;
        sample_r = info.sample_freq;
        repeat = loop;
        isplaying2 = true;
        return true;
    }

    virtual void seek(unsigned ms)
    {
        double index = ms / 1000.f;
        if (isplaying2)
            mpc_demux_seek_second(demux, index);
    }

    void stop()
    {
        if (isplaying2)
            isplaying2 = false;
        if (demux)
        {
            mpc_demux_exit(demux);
            mpc_reader_exit_stdio(&m_reader);
            fclose(filez);
        }
    }

    bool is_playing()
    {
        return isplaying2;
    }

    unsigned song_duration()
    {
        mpc_streaminfo info = {};
        mpc_demux_get_info(demux, &info);
        int64_t total_samps = mpc_streaminfo_get_length_samples(&info);
        return static_cast<uint32_t>((1000ull * total_samps) / sample_r);
    }

    const char *song_title()
    {
        return NULL;
    }

   std::vector <std::string> file_types()
    {
        std::vector<std::string>  a3 = { "mpc","mpp" };
        return a3;
    }

    void mix(float *&buffer_samps, unsigned &count)
    {
        float temp_buffer[NUM_FRAMES * 4 * sizeof(float)] = {0};
        unsigned temp_samples = 0;
        if (isplaying2)
        {
        again:
            mpc_status err;
            mpc_frame_info frame = {};
            frame.buffer = temp_buffer;
            err = mpc_demux_decode(demux, &frame);
            temp_samples = frame.samples;
            if (temp_samples == 0 && err != MPC_STATUS_OK)
            {
                if (repeat)
                {
                    mpc_demux_seek_second(demux, 0);
                    goto again;
                }
                isplaying2 = false;
            }
        }
        buffer_samps = temp_buffer;
        count = temp_samples;
    }
};

auddecode *create_mpc()
{
    return new auddecode_mpc;
}