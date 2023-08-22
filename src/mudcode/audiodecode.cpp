#include "audiodecode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libretro.h>
#define RESAMPLER_IMPLEMENTATION
#include "resampler.h"

auddecode *replayer = NULL;
float srate = 0.0;
float *input_float = NULL;
float *output_float = NULL;
void *resample = NULL;

const char *get_filename_ext(const char *filename)
{
   const char *dot = strrchr(filename, '.');
   if (!dot || dot == filename)
      return "";
   return dot + 1;
}

auddecode *make_decoder(const char* filename)
{
   auddecode *replay = NULL;
   int i=0;
   do
   {
      if(!auddecode_factory[i].init)return NULL;
      replay = auddecode_factory[i].init();
      const char *ext = get_filename_ext(filename);
      if(strcmp(ext,replay->file_types())==0){
      if(!replay->open(filename,&srate,false)){
     again:
      delete replay;
      replay=NULL;
      i++;
      continue;
      }
      else
      return replay;
      }
      else goto again;
   }while(1);
}

inline void s16tof(float *dst, const int16_t *src, unsigned int count)
{
    unsigned int i = 0;
    float fgain = 1.0 / UINT32_C(0x80000000);
    __m128 factor = _mm_set1_ps(fgain);
    for (i = 0; i + 8 <= count; i += 8, src += 8, dst += 8)
    {
        __m128i input = _mm_loadu_si128((const __m128i *)src);
        __m128i regs_l = _mm_unpacklo_epi16(_mm_setzero_si128(), input);
        __m128i regs_r = _mm_unpackhi_epi16(_mm_setzero_si128(), input);
        __m128 output_l = _mm_mul_ps(_mm_cvtepi32_ps(regs_l), factor);
        __m128 output_r = _mm_mul_ps(_mm_cvtepi32_ps(regs_r), factor);
        _mm_storeu_ps(dst + 0, output_l);
        _mm_storeu_ps(dst + 4, output_r);
    }
    fgain = 1.0 / 0x8000;
    count = count - i;
    i = 0;
    for (; i < count; i++)
        dst[i] = (float)src[i] * fgain;
}

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


#define BUFSIZE 4096
bool isplaying=false;

char *buffer = NULL;

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
        free(input_float);
        free(output_float);
        resampler_sinc_free(resample);      
    }

}
bool music_play(const char* filename)
{
   music_stop();
   replayer = make_decoder(filename);
    size_t sampsize = ((2048*sizeof(float)) * 8);
    input_float = (float *)malloc(sampsize);
    output_float = (float *)malloc(sampsize);
    memset(input_float, 0, sampsize);
    memset(output_float, 0, sampsize);
    resample = resampler_sinc_init();
    return (replayer != NULL)?true:false;
}
void music_run()
{
   struct resampler_data src_data = {0};

   float samples[BUFSIZE * 2] = {0};
   int16_t samples_int[2 * BUFSIZE] = {0};
   extern retro_audio_sample_batch_t audio_batch_cb;
   if(replayer &&replayer->isplaying())
   {
   float *samples =NULL;
   unsigned count=0;
   replayer->mix(samples,count);


   if(srate != 44100)
   {
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

   }
}