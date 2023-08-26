#ifndef ad_h_
#define ad_h_

#include <stdint.h>
#include <stdbool.h>

bool music_isplaying();
void music_stop();
bool music_play(const char* filename);
void music_run();

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

auddecode *create_mp3();
auddecode *create_flac();
auddecode *create_vorbis();
auddecode *create_wav();

typedef   auddecode* (*create_filetype)();

static const char *filetypes = ".mp3|.flac|.ogg|.wav";


static struct auddecode_factory_ {
   create_filetype  init; 
 }  auddecode_factory []={
    create_mp3,
    create_flac,
    create_vorbis,
    create_wav,
    NULL
 };

 #endif
