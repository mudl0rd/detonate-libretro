#include "audiodecode.h"
#include "stb_vorbis.h"

#define NUM_FRAMES 1024

class auddecode_vorbis : public auddecode
{
private:
    bool isplaying2;
    stb_vorbis *stream;
    bool repeat;

public:
    ~auddecode_vorbis()
    {
        stream = NULL;
    }

    auddecode_vorbis()
    {
        isplaying2 = false;
        stream = NULL;
    }

    bool open(const char *filename, float *samplerate, bool loop)
    {
        stream = stb_vorbis_open_filename(filename, NULL, NULL);
        if (!stream)
        {
            return false;
        }
        *samplerate = stream->sample_rate;
        repeat = loop;
        isplaying2 = true;
        return true;
    }

    virtual void seek(unsigned ms)
    {
        uint64_t index = ms;
        index *= stream->sample_rate;
        index /= 1000;
        stb_vorbis_seek(stream, index);
    }

    void stop()
    {
        if (isplaying2)
        {
            isplaying2 = false;
            if (stream)
                stb_vorbis_close(stream);
        }
    }

    bool is_playing()
    {
        return isplaying2;
    }

    unsigned song_duration()
    {
        return uint32_t(stb_vorbis_stream_length_in_seconds(stream) * 1000ull);
    }

    const char *song_title()
    {
        return NULL;
    }

    const char *file_types()
    {
        return "ogg";
    }

    void mix(float *&buffer_samps, unsigned &count)
    {
        float temp_buffer[NUM_FRAMES * 4 * sizeof(float)] = {0};
        unsigned temp_samples = 0;
        if (isplaying2)
        {
        again:
            temp_samples = stb_vorbis_get_samples_float_interleaved(stream, 2, temp_buffer, NUM_FRAMES * 2);
            if (temp_samples == 0)
            {
                if (repeat)
                {
                    stb_vorbis_seek_frame(stream, 0);
                    goto again;
                }
                isplaying2 = false;
            }
        }
        buffer_samps = temp_buffer;
        count = temp_samples;
    }
};

auddecode *create_vorbis()
{
    return new auddecode_vorbis;
}