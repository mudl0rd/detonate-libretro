#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>
#include "audiodecode.h"
#include <stdint.h>
#include <stdbool.h>


class auddecode_wav : public auddecode
{
private:
    bool isplaying2;
    drwav stream;
    bool repeat;
    bool isplaying;
public:
    ~auddecode_wav()
    {
    }

    auddecode_wav()
    {
        isplaying2 = false;
    }

    bool open(const char *filename, float *samplerate, bool loop)
    {
        if (!drwav_init_file(&stream, filename, NULL))
        {
            return false;
        }
        *samplerate = stream.sampleRate;
        repeat = loop;
        isplaying2 = true;
        return true;
    }

    virtual void seek(unsigned ms)
    {
        drwav_uint64 index = ms;
        index *= stream.sampleRate;
        index /= 1000;
        drwav_seek_to_pcm_frame(&stream, index);
    }

    void stop()
    {
        if (isplaying2)
            isplaying2 = false;
        if (&stream)
            drwav_uninit(&stream);
    }

    bool is_playing()
    {
        return isplaying2;
    }

    unsigned song_duration()
    {
        drwav_uint64 index;
        drwav_get_length_in_pcm_frames(&stream, &index);
        index *= 1000ull;
        index /= stream.sampleRate;
        return index;
    }

    const char *song_title()
    {
        return NULL;
    }

    std::vector <std::string> file_types()
    {
        std::vector<std::string>  a3 = { "wav","w64","aiff" };
        return a3;
    }

    void mix(float *&buffer_samps, unsigned &count)
    {
        float temp_buffer[count * 4 * sizeof(float)] = {0};
        unsigned temp_samples = 0;
        if (isplaying2)
        {
        again:
            temp_samples = (unsigned)drwav_read_pcm_frames_f32(&stream,count, temp_buffer);
            if (temp_samples == 0)
            {
                if (repeat)
                {
                    drwav_seek_to_pcm_frame(&stream, 0);
                    goto again;
                }
                isplaying2 = false;
            }
        }
        buffer_samps = temp_buffer;
        count = temp_samples;
    }
};

auddecode *create_wav()
{
    return new auddecode_wav;
}