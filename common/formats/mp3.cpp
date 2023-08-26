#include "../audiodecode.h"
#define DR_MP3_IMPLEMENTATION
#include <dr/dr_mp3.h>
#include <stdint.h>
#include <stdbool.h>

#define NUM_FRAMES 1024

class auddecode_mp3 : public auddecode
{
private:
    bool isplaying2;
    drmp3       stream;
    bool        repeat;
public:
	~auddecode_mp3() {
    }

    auddecode_mp3()
    {
        isplaying2 = false;
    }

	bool open(const char* filename ,float * samplerate, bool loop)
    {

        if (!drmp3_init_file(&stream, filename, NULL)) {
        return false;
        }
        *samplerate = stream.sampleRate;
         repeat = loop;
         isplaying2=true;
        return true;
    }


	virtual void seek(unsigned ms)
    {
        drmp3_uint64 index = ms;
        index *= stream.sampleRate;
        index /= 1000;
        drmp3_seek_to_pcm_frame(&stream, index);
    }

    void stop()
    {
        if(isplaying2)
        {
        isplaying2=false;
        if(&stream)
        drmp3_uninit(&stream);
       
        }
    }

    bool is_playing()
    {
        return isplaying2;
    }

    unsigned song_duration(){
        drmp3_uint64 index=drmp3_get_pcm_frame_count(&stream)*stream.channels;
        index /= stream.sampleRate;
        return index;
    }

    const char* song_title()
    {
        return NULL;

    }

    const char* file_types(){
         return "mp3";
    }

	void mix( float *& buffer_samps, unsigned & count)
    {
        float temp_buffer[NUM_FRAMES * 4 * sizeof(float)] = { 0 };
        unsigned temp_samples=0;
         if(isplaying2){
     again:
      temp_samples = (unsigned)drmp3_read_pcm_frames_f32(&stream,NUM_FRAMES, temp_buffer);
      if (temp_samples == 0)
      {
         if (repeat)
         {
            drmp3_seek_to_pcm_frame(&stream,0);
            goto again;
         }
         isplaying2=false;
      }
    
    }
      buffer_samps = temp_buffer;
      count = temp_samples;
    }
};

auddecode *create_mp3(){
    return new auddecode_mp3;
}