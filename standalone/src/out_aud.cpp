#define RESAMPLER_IMPLEMENTATION
#include <SDL2/SDL.h>
#include "resampler.h"
#include "out_aud.h"
#include "utils.h"

struct fifo_buffer
{
    uint8_t *buffer;
    size_t size;
    volatile int readpos;
    volatile int writepos;
};

struct audio_ctx
{
    bool floating_point;
    fifo_buffer *_fifo;
    SDL_AudioDeviceID dev;
    unsigned client_rate;
    double system_rate;
    void *resample;
    float *input_float;
    float *output_float;
} audio_ctx_s;

typedef struct fifo_buffer fifo_buffer_t;
static inline void fifo_clear(fifo_buffer_t *buffer)
{
    buffer->readpos = 0;
    buffer->writepos = 0;
}

static inline void fifo_free(fifo_buffer_t *buffer)
{
    if (!buffer)
        return;

    free(buffer->buffer);
    free(buffer);
}
#define fifo_size(x) ((x)->size - 1)
#define fifo_read_avail(x) (((x)->writepos - (x)->readpos) & fifo_size(x))
#define fifo_write_avail(x) ((x)->size - 1 - fifo_read_avail(x))

fifo_buffer_t *fifo_new(size_t size)
{
    uint8_t *buffer = NULL;
    fifo_buffer_t *buf = (fifo_buffer_t *)calloc(1, sizeof(*buf));
    if (!buf)
        return NULL;
    buf->size = pow2up(size);
    buffer = (uint8_t *)malloc(size);
    if (!buffer)
    {
        free(buf);
        return NULL;
    }
    memset(buffer, 0, size);
    buf->buffer = buffer;
    buf->size = size;
    return buf;
}

int fifo_write(fifo_buffer_t *buffer, void *in_buf, size_t size, bool read)
{
    int total;
    int i;
    char *buf = (char *)in_buf;
    total = (read) ? fifo_read_avail(buffer) : fifo_write_avail(buffer);
    if (size > total)
        size = total;
    else
        total = size;
    i = (read) ? buffer->readpos : buffer->writepos;
    if (i + size > buffer->size)
    {
        if (read)
            memcpy(buf, buffer->buffer + i, buffer->size - i);
        else
            memcpy(buffer->buffer + i, buf, buffer->size - i);
        buf += buffer->size - i;
        size -= buffer->size - i;
        i = 0;
    }
    if (read)
    {
        memcpy(buf, buffer->buffer + i, size);
        buffer->readpos = i + size;
    }
    else
    {
        memcpy(buffer->buffer + i, buf, size);
        buffer->writepos = i + size;
    }
    return total;
}

int fifo_writespin(fifo_buffer_t *f, void *buf, unsigned len)
{
    while (fifo_write_avail(f) < len)
        ;
    return fifo_write(f, buf, len, false);
}

int fifo_readspin(fifo_buffer_t *f, void *buf, unsigned len)
{
    while (fifo_read_avail(f) < len)
        ;
    return fifo_write(f, buf, len, true);
}

inline void s16tof(float *dst, const int16_t *src, unsigned int count)
{
    for (int i=0; i < count; i++)
        dst[i] = (float)src[i] * 0.000030517578125f;
}

void func_callback(void *userdata, Uint8 *stream, int len)
{
    audio_ctx *context = (audio_ctx *)userdata;
    int amount = fifo_read_avail(context->_fifo);
    amount = (len > amount) ? amount : len;
    fifo_write(context->_fifo, (uint8_t *)stream, amount, true);
    memset(stream + amount, 0, len - amount);
}

void audio_mix(void *samples, size_t size)
{

    struct resampler_data src_data = {0};
    size_t written = 0;
    uint32_t in_len = size * 2;
    int half_size = (int)(audio_ctx_s._fifo->size / 2);
    int delta_mid = (int)fifo_write_avail(audio_ctx_s._fifo) - half_size;
    float drc_ratio = (float)(audio_ctx_s.client_rate / audio_ctx_s.system_rate) *
                      (1.0 + 0.005 * ((double)delta_mid / half_size));
    if(!audio_ctx_s.floating_point)
    {
        s16tof(audio_ctx_s.input_float, (int16_t*)samples,in_len);
        src_data.data_in = audio_ctx_s.input_float;
    }
    else
    src_data.data_in = (float*)samples;
    src_data.input_frames = size;
    src_data.ratio = drc_ratio;
    src_data.data_out = audio_ctx_s.output_float;
    resampler_sinc_process(audio_ctx_s.resample, &src_data);
    size_t out_bytes = src_data.output_frames * 2 * sizeof(float);

    while (written < out_bytes)
    {
        size_t avail = fifo_write_avail(audio_ctx_s._fifo);
        if (avail)
        {
            size_t write_amt = out_bytes - written > avail ? avail : out_bytes - written;
            fifo_write(audio_ctx_s._fifo,
                       (char *)audio_ctx_s.output_float + written, write_amt, false);
            written += write_amt;
        }
    }
}

void audio_changeratefps(float refreshra, float input_srate, float fps)
{
    unsigned swap = 1;
    if (refreshra > fps)
        swap = refreshra / (unsigned)fps;
    float refreshtarget = refreshra / swap;
    float timing_skew = fabs(1.0f - fps / refreshtarget);
    if (timing_skew <= 0.05)
        audio_ctx_s.system_rate = input_srate * (refreshtarget / fps);
    else
        audio_ctx_s.system_rate = input_srate;
}

bool audio_init(float refreshra, float input_srate, float fps, bool fp)
{
    audio_ctx_s.floating_point=fp;
    SDL_AudioSpec shit = {0};
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
    size_t sampsize = (out.size * 2);
    audio_ctx_s.input_float = (float *)memalign_alloc(64, sampsize * 4);
    audio_ctx_s.output_float = (float *)memalign_alloc(64, sampsize * 4);
    memset(audio_ctx_s.input_float, 0, sampsize * 4);
    memset(audio_ctx_s.output_float, 0, sampsize * 4);
    audio_ctx_s._fifo = fifo_new(sampsize); // number of bytes
    SDL_PauseAudioDevice(audio_ctx_s.dev, 0);
    return true;
}
void audio_destroy()
{
    SDL_PauseAudioDevice(audio_ctx_s.dev, 1);
    SDL_CloseAudioDevice(audio_ctx_s.dev);
    fifo_free(audio_ctx_s._fifo);
    memalign_free(audio_ctx_s.input_float);
    memalign_free(audio_ctx_s.output_float);
    resampler_sinc_free(audio_ctx_s.resample);
}