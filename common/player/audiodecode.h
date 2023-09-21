#ifndef ad_h_
#define ad_h_

#include <stdint.h>
#include <stdbool.h>
#include <string>

bool music_isplaying();
void music_stop();
bool music_play(const char* filename);
void music_run();
uint32_t music_getduration();
uint32_t music_getposition();
uint32_t music_setposition(uint64_t pos);


enum sampfmt
{
    U8,
    S8,
    P16,
    P24,
    P32,
    P64,
    FLT,
    DBL,
    END
};
void conv2float(float * dst, const uint8_t * src, const size_t N, sampfmt f);


inline int32_t Pack(uint8_t a, uint8_t b, uint8_t c)
{
    // uint32_t tmp = ((c & 0x80) ? (0xFF << 24) : 0x00 << 24) | (c << 16) | (b << 8) | (a << 0); // alternate method
    int32_t x = (c << 16) | (b << 8) | (a << 0);
    auto sign_extended = (x) | (!!((x) & 0x800000) * 0xff000000);
    return sign_extended;
}

#define R_INT16_MAX 32767.f
#define R_INT24_MAX 8388608.f
#define R_INT32_MAX 2147483648.f
static const float R_BYTE_2_FLT = 1.0f / 127.0f;
#define int8_to_float32(s)  ((float) (s) * R_BYTE_2_FLT)
#define uint8_to_float32(s)(((float) (s) - 128) * R_BYTE_2_FLT)
#define int16_to_float32(s) ((float) (s) / R_INT16_MAX)
#define int24_to_float32(s) ((float) (s) / R_INT24_MAX)
#define int32_to_float32(s) ((float) (s) / R_INT32_MAX)

class auddecode
{
public:
	virtual ~auddecode() {}

	virtual bool open(const char *filename ,float *samplerate, bool loop) = 0;

	virtual void seek(unsigned ms) = 0;

   virtual void stop() = 0;

    virtual bool is_playing() = 0;

    virtual unsigned song_duration() = 0;

    virtual const char* song_title() = 0;

    virtual const char* file_types() = 0;

	virtual void mix( float *& buffer_samps, unsigned & count) = 0;
};
std::string auddecode_formats();
auddecode *create_mp3();
auddecode *create_flac();
auddecode *create_vorbis();
auddecode *create_wav();
auddecode *create_opus();
auddecode *create_mpc();
auddecode *create_wv();
auddecode *create_modules();
typedef   auddecode* (*create_filetype)();
static struct auddecode_factory_ {
   create_filetype  init; 
 }  auddecode_factory []={
    create_mp3,
    create_flac,
    create_vorbis,
    create_wav,
    create_opus,
    create_mpc,
    create_wv,
    create_modules,
    NULL
 };

 #endif
