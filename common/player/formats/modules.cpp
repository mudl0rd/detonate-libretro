#include "audiodecode.h"
#include "m4p/m4p.h"
#include "mudutils/utils.h"

#define NUM_FRAMES 1024

class auddecode_modules : public auddecode
{
private:
int current_format;
    bool isplaying2;
    int sample_r;
    int bits;
public:
	~auddecode_modules() {

    }

    auddecode_modules()
    {
    }

	bool open(const char* filename ,float * samplerate, bool loop)
    {

        int sample_r = 44100;
        int sz=0;
        std::vector<uint8_t>dat= load_data(filename,(unsigned int *)&sz);
        if(!m4p_LoadFromData(dat.data(), dat.size(), sample_r, 1024))
        return false;
        *samplerate = sample_r;
        isplaying2=true;
        m4p_PlaySong();
        return true;
    }


	virtual void seek(unsigned ms)
    {
       
    }

    void stop()
    {
        if(isplaying2)
        {
        isplaying2=false;
        m4p_Close();
	    m4p_FreeSong();
        }
    }

    bool is_playing()
    {
        return isplaying2;
    }

    unsigned song_duration(){
        return 0;
    }

    const char* song_title()
    {
        return NULL;

    }

    const char* file_types(){
         return "xm";
    }


	void mix( float *& buffer_samps, unsigned & count)
    {
        float temp_buffer[NUM_FRAMES * 4 * sizeof(float)] = { 0 };
        int16_t temp_bufferint[NUM_FRAMES * 4 * sizeof(int32_t)] = { 0 };
        unsigned temp_samples=0;
         int status=0;
         if(isplaying2){
     again:
         status=m4p_GenerateSamples(temp_bufferint,NUM_FRAMES);
         conv2float(temp_buffer,(uint8_t*)temp_bufferint, NUM_FRAMES*2,sampfmt::P16);
         temp_samples=NUM_FRAMES;
        }
      if (status=0)
      {
         isplaying2=false;
      }
      buffer_samps = temp_buffer;
      count = temp_samples;
    }
};

auddecode *create_modules(){
    return new auddecode_modules;
}