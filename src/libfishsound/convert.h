/*
   Copyright (C) 2003 Commonwealth Scientific and Industrial Research
   Organisation (CSIRO) Australia

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of CSIRO Australia nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __FISH_SOUND_CONVERT_H__
#define __FISH_SOUND_CONVERT_H__

/* inline functions */

static inline void
_fs_deinterleave_s_s (short ** src, short * dest[],
		      long frames, int channels)
{
  int i, j;
  short * d, * s = (short *)src;

  for (i = 0; i < frames; i++) {
    for (j = 0; j < channels; j++) {
      d = dest[j];
      d[i] = s[i*channels + j];
    }
  }
}

static inline void
_fs_deinterleave_s_i (short ** src, int * dest[], long frames, int channels)
{
  int i, j;
  short * s = (short *)src;
  int * d;

  for (i = 0; i < frames; i++) {
    for (j = 0; j < channels; j++) {
      d = dest[j];
      d[i] = (int) s[i*channels + j];
    }
  }
}

static inline void
_fs_deinterleave_f_s (float ** src, short * dest[],
		      long frames, int channels, float mult)
{
  int i, j;
  float * s = (float *)src;
  short * d;

  for (i = 0; i < frames; i++) {
    for (j = 0; j < channels; j++) {
      d = dest[j];
      d[i] = (short) (s[i*channels + j] * mult);
    }
  }
}

static inline void
_fs_deinterleave_f_i (float ** src, int * dest[],
		      long frames, int channels, float mult)
{
  int i, j;
  float * s = (float *)src;
  int * d;

  for (i = 0; i < frames; i++) {
    for (j = 0; j < channels; j++) {
      d = dest[j];
      d[i] = (int) (s[i*channels + j] * mult);
    }
  }
}

static inline void
_fs_deinterleave_f_f (float ** src, float * dest[],
		      long frames, int channels, float mult)
{
  int i, j;
  float * s = (float *)src, * d;

  for (i = 0; i < frames; i++) {
    for (j = 0; j < channels; j++) {
      d = dest[j];
      d[i] = s[i*channels + j] * mult;
    }
  }
}

static inline void
_fs_deinterleave_f_d (float ** src, double * dest[],
		      long frames, int channels, float mult)
{
  int i, j;
  float * s = (float *)src;
  double * d;

  for (i = 0; i < frames; i++) {
    for (j = 0; j < channels; j++) {
      d = dest[j];
      d[i] = (double) s[i*channels + j] * mult;
    }
  }
}

static inline void
_fs_interleave_f_f (float * src[], float ** dest,
		    long frames, int channels, float mult)
{
  int i, j;
  float * s, * d = (float *)dest;

  for (i = 0; i < frames; i++) {
    for (j = 0; j < channels; j++) {
      s = src[j];
      d[i*channels + j] = s[i] * mult;
    }
  }
}

static inline void
_fs_convert_s_i (short ** src, int ** dest, long samples)
{
  int i;
  short * s = (short *)src;
  int * d = (int *)dest;

  for (i = 0; i < samples; i++) {
    d[i] = (int) s[i];
  }
}

static inline void
_fs_convert_f_s (float ** src, short ** dest, long samples)
{
  int i;
  float * s = (float *)src;
  short * d = (short *)dest;

  for (i = 0; i < samples; i++) {
    d[i] = (short) s[i];
  }
}

static inline void
_fs_convert_f_i (float ** src, int ** dest, long samples)
{
  int i;
  float * s = (float *)src;
  int * d = (int *)dest;

  for (i = 0; i < samples; i++) {
    d[i] = (int) s[i];
  }
}

static inline void
_fs_convert_f_f (float ** src, float ** dest, long samples, float mult)
{
  int i;
  float * s = (float *)src;
  float * d = (float *)dest;

  for (i = 0; i < samples; i++) {
    d[i] = ((float) s[i]) * mult;
  }
}

static inline void
_fs_convert_f_d (float ** src, double ** dest, long samples, float mult)
{
  int i;
  float * s = (float *)src;
  double * d = (double *)dest;

  for (i = 0; i < samples; i++) {
    d[i] = ((double) s[i]) * mult;
  }
}

#endif /* __FISH_SOUND_CONVERT_H__ */
