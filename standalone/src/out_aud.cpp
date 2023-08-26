#define RESAMPLER_IMPLEMENTATION
#include <SDL2/SDL.h>
#include "resampler.h"
#include "out_aud.h"
#include "mudutils/utils.h"

struct fifo_buffer
{
    uint8_t *buffer;
    size_t size;
    size_t first;
    size_t end;
};

struct audio_ctx
{
    fifo_buffer *_fifo;
    SDL_AudioDeviceID dev;
    unsigned client_rate;
    double system_rate;
    void *resample;
    float *input_float;
    float *output_float;
    float timing_skew;
    bool floating_point;
} audio_ctx_s;

typedef struct fifo_buffer fifo_buffer_t;

static inline void fifo_clear(fifo_buffer_t *buffer)
{
    buffer->first = 0;
    buffer->end = 0;
}

static inline void fifo_free(fifo_buffer_t *buffer)
{
    if (!buffer)
        return;

    free(buffer->buffer);
    free(buffer);
}

static inline size_t fifo_read_avail(fifo_buffer_t *buffer)
{
    return (buffer->end + ((buffer->end < buffer->first) ? buffer->size : 0)) -
           buffer->first;
}

static inline size_t fifo_write_avail(fifo_buffer_t *buffer)
{
    return (buffer->size - 1) -
           ((buffer->end + ((buffer->end < buffer->first) ? buffer->size : 0)) -
            buffer->first);
}

fifo_buffer_t *fifo_new(size_t size)
{
    uint8_t *buffer = NULL;
    fifo_buffer_t *buf = (fifo_buffer_t *)calloc(1, sizeof(*buf));
    if (!buf)
        return NULL;
    buffer = (uint8_t *)calloc(1, size + 1);
    if (!buffer)
    {
        free(buf);
        return NULL;
    }
    buf->buffer = buffer;
    buf->size = size + 1;
    return buf;
}

void fifo_write(fifo_buffer_t *buffer, const void *in_buf, size_t size)
{
    size_t first_write = size;
    size_t rest_write = 0;
    if (buffer->end + size > buffer->size)
    {
        first_write = buffer->size - buffer->end;
        rest_write = size - first_write;
    }
    memcpy(buffer->buffer + buffer->end, in_buf, first_write);
    memcpy(buffer->buffer, (const uint8_t *)in_buf + first_write, rest_write);
    buffer->end = (buffer->end + size) % buffer->size;
}

void fifo_read(fifo_buffer_t *buffer, void *in_buf, size_t size)
{
    size_t first_read = size;
    size_t rest_read = 0;
    if (buffer->first + size > buffer->size)
    {
        first_read = buffer->size - buffer->first;
        rest_read = size - first_read;
    }
    memcpy(in_buf, (const uint8_t *)buffer->buffer + buffer->first, first_read);
    memcpy((uint8_t *)in_buf + first_read, buffer->buffer, rest_read);
    buffer->first = (buffer->first + size) % buffer->size;
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

void func_callback(void *userdata, Uint8 *stream, int len)
{
    memset(stream, 0, len);
    audio_ctx *context = (audio_ctx *)userdata;
    int amount = fifo_read_avail(context->_fifo);
    if(amount){
    amount = (len > amount) ? amount : len;
    fifo_read(context->_fifo, (uint8_t *)stream, amount);
    }
}

void audio_mix(void *samples, size_t size_frames)
{

    struct resampler_data src_data = {0};
    size_t written = 0;
    uint32_t in_len = size_frames * 2;
    int half_size = (int)(audio_ctx_s._fifo->size / 2);
    int delta_mid = (int)fifo_write_avail(audio_ctx_s._fifo) - half_size;
    double drc_ratio = (double)(audio_ctx_s.client_rate / audio_ctx_s.system_rate) *
                      (1.0 + audio_ctx_s.timing_skew * ((double)delta_mid / half_size));
    if(!audio_ctx_s.floating_point)
    s16tof(audio_ctx_s.input_float, (int16_t*)samples, in_len);

    void *out_samples = NULL;
    size_t out_bytes = 0;
    if(drc_ratio != 1.0){
    src_data.input_frames = size_frames;
    src_data.ratio = drc_ratio;
    src_data.data_in = 
    audio_ctx_s.floating_point?(float*)samples:audio_ctx_s.input_float;
    src_data.data_out = audio_ctx_s.output_float;
    resampler_sinc_process(audio_ctx_s.resample, &src_data);
    out_bytes = src_data.output_frames * 2 * sizeof(float);
    out_samples = audio_ctx_s.output_float;
    }
    else
    {
        out_samples=samples;
        out_bytes = in_len*
        (audio_ctx_s.floating_point?sizeof(float):sizeof(short));
    }
    while (written < out_bytes)
    {
        //SDL_LockAudioDevice(audio_ctx_s.dev);
        size_t avail = fifo_write_avail(audio_ctx_s._fifo);
        if (avail)
        {

            size_t write_amt = out_bytes - written > avail ? avail : out_bytes - written;
            fifo_write(audio_ctx_s._fifo,
                       (const char *)out_samples + written, write_amt);
            written += write_amt;
        }
      //  SDL_UnlockAudioDevice(audio_ctx_s.dev);
    }
}

void audio_changeratefps(float refreshra, float input_srate, float fps)
{
    if (fps)
    {
        unsigned swap = 1;
        if (refreshra > fps)
            swap = refreshra / (unsigned)fps;
        float refreshtarget = refreshra / swap;
        float timing_skew = fabs(1.0f - fps / refreshtarget);
        if (timing_skew <= 0.005)
        {
            audio_ctx_s.timing_skew=timing_skew;
            audio_ctx_s.system_rate = input_srate * (refreshtarget / fps);
        }
    }
}

bool audio_init(float refreshra, float input_srate, float fps,bool fp)
{
    SDL_AudioSpec shit = {0};
    audio_ctx_s.system_rate = input_srate;
    audio_ctx_s.floating_point = fp;
    audio_ctx_s.timing_skew=0.0;
    audio_changeratefps(refreshra, input_srate, fps);
    SDL_AudioSpec shit2 = {0};
    SDL_GetDefaultAudioInfo(NULL, &shit2, 0);

    shit.freq = shit2.freq;
    shit.format = AUDIO_F32;
    shit.samples = 2048;
    shit.callback = func_callback;
    shit.userdata = (audio_ctx *)&audio_ctx_s;
    shit.channels = 2;
    audio_ctx_s.client_rate = shit.freq;
    audio_ctx_s.resample = resampler_sinc_init();
    SDL_AudioSpec out;
    audio_ctx_s.dev = SDL_OpenAudioDevice(NULL, 0, &shit, &out, 0);
    // allocate some in tank. Accounts for resampler too
    size_t sampsize = (out.size * 8);
    if(!fp)
    {
        audio_ctx_s.input_float = (float *)memalign_alloc(64, sampsize);
        memset(audio_ctx_s.input_float, 0, sampsize);
    }

    audio_ctx_s.output_float = (float *)memalign_alloc(64, sampsize);
    memset(audio_ctx_s.output_float, 0, sampsize);
    audio_ctx_s._fifo = fifo_new(sampsize); // number of bytes
//    auto tmp = std::make_unique<uint8_t[]>(sampsize);
  //  fifo_write(audio_ctx_s._fifo, tmp.get(), sampsize);
    SDL_PauseAudioDevice(audio_ctx_s.dev, 0);
    return true;
}
void audio_destroy()
{
    SDL_PauseAudioDevice(audio_ctx_s.dev, 1);
    SDL_CloseAudioDevice(audio_ctx_s.dev);
    fifo_free(audio_ctx_s._fifo);
    if(!audio_ctx_s.floating_point)
    memalign_free(audio_ctx_s.input_float);
    memalign_free(audio_ctx_s.output_float);
    resampler_sinc_free(audio_ctx_s.resample);
}
