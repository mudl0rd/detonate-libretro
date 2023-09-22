#include "../audiodecode.h"
#define DR_FLAC_IMPLEMENTATION
#include <dr/dr_flac.h>
#include <stdint.h>
#include <stdbool.h>

#define NUM_FRAMES 1024

class auddecode_flac : public auddecode
{
private:
    bool isplaying2;
    drflac *stream;
    bool repeat;

public:
    ~auddecode_flac()
    {
    }

    auddecode_flac()
    {
        stream = NULL;
        isplaying2 = false;
    }

    bool open(const char *filename, float *samplerate, bool loop)
    {
        stream = drflac_open_file(filename, NULL);
        if (!stream)
        {
            return false;
        }
        *samplerate = stream->sampleRate;
        repeat = loop;
        isplaying2 = true;
        return true;
    }

    virtual void seek(unsigned ms)
    {
        drflac_uint64 index = ms;
        index *= stream->sampleRate;
        index /= 1000;
        drflac_seek_to_pcm_frame(stream, index);
    }

    void stop()
    {
        if (isplaying2)
        isplaying2 = false;
        if (stream)
        {
            drflac_free(stream, NULL);
            stream = NULL;
        }
    }

    bool is_playing()
    {
        return isplaying2;
    }

    unsigned song_duration()
    {
        return static_cast<uint32_t>((1000ull * stream->totalPCMFrameCount) / stream->sampleRate);
    }

    const char *song_title()
    {
        return NULL;
    }

    const char *file_types()
    {
        return "flac";
    }

    void mix(float *&buffer_samps, unsigned &count)
    {
        float temp_buffer[NUM_FRAMES * 4 * sizeof(float)] = {0};
        unsigned temp_samples = 0;
        if (isplaying2)
        {
        again:
            temp_samples = (unsigned)drflac_read_pcm_frames_f32(stream, NUM_FRAMES, temp_buffer);
            if (temp_samples == 0)
            {
                if (repeat)
                {
                    drflac_seek_to_pcm_frame(stream, 0);
                    goto again;
                }
                isplaying2 = false;
            }
        }
        buffer_samps = temp_buffer;
        count = temp_samples;
    }
};

auddecode *create_flac()
{
    return new auddecode_flac;
}