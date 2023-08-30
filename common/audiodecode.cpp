#include "audiodecode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef LIBRETRO
#define RESAMPLER_IMPLEMENTATION
#include "libretro.h"
#include "resampler.h"
float *input_float = NULL;
float *output_float = NULL;
void *resample = NULL;
#else
#include "out_aud.h"
#endif

auddecode *replayer = NULL;
float srate = 0.0;

const char *get_filename_ext(const char *filename)
{
   const char *dot = strrchr(filename, '.');
   if (!dot || dot == filename)
      return "";
   return dot + 1;
}


std::string auddecode_formats()
{
   std::ostringstream oss;
   char const* sep = "";
   for (int i = 0; auddecode_factory[i].init; ++i) {
    std::unique_ptr<auddecode_factory_> replay(auddecode_factory[i].init());
    formats << sep << replay->file_types();
    sep = "|";
   }
   return oss.str();
}

auddecode *make_decoder(const char* filename)
{
   auddecode *replay = NULL;
   for(int i=0;auddecode_factory[i].init;++i)
   {
   replay = auddecode_factory[i].init();
   const char *ext = get_filename_ext(filename);
   if(strcmp(ext,replay->file_types())==0){
   if(!replay->open(filename,&srate,false)){
   delete replay;
   replay=NULL;
   }
   else
   return replay;
   }
   }
}

#ifdef LIBRETRO
void convert_float_to_s16(int16_t *out,
                          const float *in, size_t samples)
{
   size_t i = 0;
   for (; i < samples; i++)
   {
      int32_t val = (int32_t)(in[i] * 0x8000);
      out[i] = (val > 0x7FFF) ? 0x7FFF : (val < -0x8000 ? -0x8000 : (int16_t)val);
   }
}
#endif

bool isplaying=false;
bool music_isplaying(){
    return isplaying;
}

void music_stop()
{
 if(replayer)
    {
        replayer->stop();
        delete replayer;
        replayer = NULL;      
   #ifndef LIBRETRO  
        audio_destroy();
   #else
        free(input_float);
        free(output_float);
        resampler_sinc_free(resample);      
   #endif
    }
}

bool music_play(const char* filename)
{
   music_stop();
   replayer = make_decoder(filename);
   #ifndef LIBRETRO  
   audio_init(0.0,srate,0.0,true);
   #else
   size_t sampsize = ((2048*sizeof(float)) * 8);
   input_float = (float *)malloc(sampsize);
    output_float = (float *)malloc(sampsize);
    memset(input_float, 0, sampsize);
    memset(output_float, 0, sampsize);
   resample = resampler_sinc_init();
   #endif 
    return (replayer != NULL)?true:false;
}
#define BUFSIZE 4096
void music_run()
{
   #ifdef LIBRETRO 
   float samples[BUFSIZE * 2] = {0};
   int16_t samples_int[2 * BUFSIZE] = {0};
   #endif
   if(replayer &&replayer->is_playing())
   {
   float *samples2 =NULL;
   unsigned count=0;
   replayer->mix(samples2,count);
   #ifndef LIBRETRO 
   audio_mix(samples2,count);
   #else
   extern retro_audio_sample_batch_t audio_batch_cb;
   if(srate != 44100)
   {
    struct resampler_data src_data = {0};
    src_data.input_frames = count;
    src_data.ratio = 44100.f/srate;
    src_data.data_in = samples;
    src_data.data_out = output_float;
    resampler_sinc_process(resample, &src_data);
    convert_float_to_s16(samples_int, output_float,src_data.output_frames*2);
    audio_batch_cb(samples_int,src_data.output_frames);
   }
   else
   {
   convert_float_to_s16(samples_int, samples,count*2);
   audio_batch_cb(samples_int,count);
   }
   #endif

   }
}