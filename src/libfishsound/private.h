/*
   Copyright (c) 2002, 2003, Xiph.org Foundation

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __FISH_SOUND_PRIVATE_H__
#define __FISH_SOUND_PRIVATE_H__

#include <stdlib.h>

#include <fishsound/constants.h>

#undef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))


typedef struct _FishSound FishSound;
typedef struct _FishSoundInfo FishSoundInfo;
typedef struct _FishSoundCodec FishSoundCodec;
typedef struct _FishSoundFormat FishSoundFormat;

typedef int         (*FSCodecIdentify) (unsigned char * buf, long bytes);
typedef FishSound * (*FSCodecInit) (FishSound * fsound);
typedef FishSound * (*FSCodecDelete) (FishSound * fsound);
typedef int         (*FSCodecReset) (FishSound * fsound);
typedef int         (*FSCodecCommand) (FishSound * fsound, int command,
				       void * data, int datasize);
typedef long        (*FSCodecDecode) (FishSound * fsound, unsigned char * buf,
				      long bytes);
typedef long        (*FSCodecEncodeI) (FishSound * fsound, float ** pcm,
				       long frames);
typedef long        (*FSCodecEncodeN) (FishSound * fsound, float ** pcm,
				       long frames);
typedef long        (*FSCodecFlush) (FishSound * fsound);

struct _FishSoundCodec {
  FishSoundFormat * format;
  FSCodecIdentify identify;
  FSCodecInit init;
  FSCodecDelete del;
  FSCodecReset reset;
  FSCodecCommand command;
  FSCodecDecode decode;
  FSCodecEncodeI encode_i;
  FSCodecEncodeN encode_n;
  FSCodecFlush flush;
};

struct _FishSoundInfo {
  int samplerate;
  int channels;
  int format;
};

struct _FishSound {
  /** FISH_SOUND_DECODE or FISH_SOUND_ENCODE */
  FishSoundMode mode;

  /** General info related to sound */
  FishSoundInfo info;

  int interleave;

  /** The codec class structure */
  FishSoundCodec * codec;

  /** codec specific data */
  void * codec_data;

  /* encode or decode callback */
  void * callback;

  /** user data for encode/decode callback */
  void * user_data; 
};

struct _FishSoundFormat {
  int format;
  const char * name;
  const char * extension;
};

typedef int (*FishSoundDecoded) (FishSound * fsound, float ** pcm,
				 long frames, void * user_data);

typedef int (*FishSoundEncoded) (FishSound * fsound, unsigned char * buf,
				 long bytes, void * user_data);

extern FishSoundCodec fish_sound_vorbis;
extern FishSoundCodec fish_sound_speex;

/* inline functions */

static inline void
_fs_deinterleave (float ** src, float * dest[],
		  long frames, int channels, float mult_factor)
{
  int i, j;

  for (i = 0; i < frames; i++) {
    for (j = 0; j < channels; j++) {
      dest[j][i] = src[i][j] * mult_factor;
    }
  }
}

static inline void
_fs_interleave (float * src[], float ** dest,
		long frames, int channels, float mult_factor)
{
  int i, j;
  float * s, * d = (float *)dest;

  for (i = 0; i < frames; i++) {
    for (j = 0; j < channels; j++) {
      s = src[j];
      d[i*channels + j] = s[i] * mult_factor;
    }
  }
}

#endif /* __FISH_SOUND_PRIVATE_H__ */
