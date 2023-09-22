#include "audiodecode.h"
#include "opus/opusfile/opusfile.h"

#define NUM_FRAMES 1024

class auddecode_opus : public auddecode
{
private:
    bool isplaying2;
    OggOpusFile *stream;
    bool repeat;

public:
    ~auddecode_opus()
    {
        stream = NULL;
    }

    auddecode_opus()
    {
        isplaying2 = false;
        stream = NULL;
    }

    bool open(const char *filename, float *samplerate, bool loop)
    {
        int err;
        stream = op_open_file(filename, &err);
        if (!stream)
        {
            return false;
        }
        *samplerate = 48000;
        repeat = loop;
        isplaying2 = true;
        return true;
    }

    virtual void seek(unsigned ms)
    {
        uint64_t index = ms;
        index *= 48000;
        index /= 1000;
        op_pcm_seek(stream, index);
    }

    void stop()
    {
        if (isplaying2)
        {
            isplaying2 = false;
            if (stream)
                op_free(stream);
        }
    }

    bool is_playing()
    {
        return isplaying2;
    }

    unsigned song_duration()
    {
        int64_t total_samps = op_pcm_total(const_cast<OggOpusFile *>(stream), -1);
        return static_cast<uint32_t>((1000ull * total_samps) / 48000);
    }

    const char *song_title()
    {
        return NULL;
    }

    const char *file_types()
    {
        return "opus";
    }

    void mix(float *&buffer_samps, unsigned &count)
    {
        float temp_buffer[NUM_FRAMES * 4 * sizeof(float)] = {0};
        unsigned temp_samples = 0;
        if (isplaying2)
        {
        again:
            temp_samples = op_read_float(stream, temp_buffer, NUM_FRAMES * 2, NULL);
            if (temp_samples == 0)
            {
                if (repeat)
                {
                    op_pcm_seek(stream, 0);
                    goto again;
                }
                isplaying2 = false;
            }
        }
        buffer_samps = temp_buffer;
        count = temp_samples;
    }
};

auddecode *create_opus()
{
    return new auddecode_opus;
}