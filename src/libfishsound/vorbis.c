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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>

#include "private.h"

/*#define DEBUG*/

#if HAVE_VORBIS

#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>

typedef struct _FishSoundVorbisInfo {
  int packetno;
  struct vorbis_info vi;
  struct vorbis_comment vc;
  vorbis_dsp_state vd; /** central working state for the PCM->packet encoder */
  vorbis_block vb;     /** local working space for PCM->packet encode */
  float * ipcm;
  long max_pcm;
} FishSoundVorbisInfo;

#if 1

static void _v_readstring(oggpack_buffer *o,char *buf,int bytes){
  while(bytes--){
    *buf++=(char)oggpack_read(o,8);
  }
}

int my_vorbis_synthesis_headerin(vorbis_info *vi,vorbis_comment *vc,ogg_packet *op){
  oggpack_buffer opb;

  if(op){
    oggpack_readinit(&opb,op->packet,op->bytes);

    /* Which of the three types of header is this? */
    /* Also verify header-ness, vorbis */
    {
      char buffer[6];
      int packtype=oggpack_read(&opb,8);
      memset(buffer,0,6);
      _v_readstring(&opb,buffer,6);
      if(memcmp(buffer,"vorbis",6)){
        /* not a vorbis header */
        printf ("not a vorbis header\n");
        return(OV_ENOTVORBIS);
      }
      switch(packtype){
      case 0x01: /* least significant *bit* is read first */
        if(!op->b_o_s){
          /* Not the initial packet */
	  printf ("Not the initial packet\n");
          return(OV_EBADHEADER);
        }
        if(vi->rate!=0){
          /* previously initialized info header */
	  printf ("previously initialized info header\n");
          return(OV_EBADHEADER);
        }

        /*return(_vorbis_unpack_info(vi,&opb));*/
	printf ("OK, want to unpack info ...\n");
	return 0;

      case 0x03: /* least significant *bit* is read first */
        if(vi->rate==0){
          /* um... we didn't get the initial header */
	  printf ("um... we didn't get the initial header\n");
          return(OV_EBADHEADER);
        }

        /*return(_vorbis_unpack_comment(vc,&opb));*/
	printf ("OK, want to unpack coment ...\n");

      case 0x05: /* least significant *bit* is read first */
        if(vi->rate==0 || vc->vendor==NULL){
          /* um... we didn;t get the initial header or comments yet */
	  printf ("um... we didn't get the initial header or comments yet\n");
          return(OV_EBADHEADER);
        }

        /*return(_vorbis_unpack_books(vi,&opb));*/
	printf ("OK, want to unpack books ...\n");

      default:
        /* Not a valid vorbis header type */
	printf ("Not a valid vorbis header type\n");
        return(OV_EBADHEADER);
        break;
      }
    }
  }
  return(OV_EBADHEADER);
}


#endif

static int
fs_vorbis_identify (unsigned char * buf, long bytes)
{
  struct vorbis_info vi;
  struct vorbis_comment vc;
  ogg_packet op;
  int ret;

  if (!strncmp ((char *)&buf[1], "vorbis", 6)) {
    /* if only a short buffer was passed, do a weak identify */
    if (bytes == 8) return FISH_SOUND_VORBIS;

    /* otherwise, assume the buffer is an entire initial header and
     * feed it to vorbis_synthesis_headerin() */

    vorbis_info_init (&vi);
    vorbis_comment_init (&vc);

    op.packet = buf;
    op.bytes = bytes;
    op.b_o_s = 1;
    op.e_o_s = 0;
    op.granulepos = 0;
    op.packetno = 0;

    if ((ret = vorbis_synthesis_headerin (&vi, &vc, &op)) == 0) {
      if (vi.rate != 0) return FISH_SOUND_VORBIS;
#ifdef DEBUG
    } else {
      printf ("vorbis_synthesis_headerin returned %d\n", ret);
#endif
    }
  }

  return FISH_SOUND_UNKNOWN;
}

static int
fs_vorbis_command (FishSound * fsound, int command, void * data,
		   int datasize)
{
  return 0;
}

#ifdef FS_DECODE
static long
fs_vorbis_decode (FishSound * fsound, unsigned char * buf, long bytes)
{
  FishSoundVorbisInfo * fsv = (FishSoundVorbisInfo *)fsound->codec_data;
  ogg_packet op;
  float ** pcm, **retpcm;
  long samples;
  int ret;

  /* Make a fake ogg_packet structure to pass the data to libvorbis */
  op.packet = buf;
  op.bytes = bytes;
  op.b_o_s = (fsv->packetno == 0) ? 1 : 0;
  op.e_o_s = 0;
  op.granulepos = 7;
  op.packetno = fsv->packetno;

  if (fsv->packetno < 3) {

    if ((ret = vorbis_synthesis_headerin (&fsv->vi, &fsv->vc, &op)) == 0) {
      if (fsv->vi.rate != 0) {
#ifdef DEBUG
	printf ("Got vorbis info: version %d\tchannels %d\trate %ld\n",
		fsv->vi.version, fsv->vi.channels, fsv->vi.rate);
#endif
	fsound->info.samplerate = fsv->vi.rate;
	fsound->info.channels = fsv->vi.channels;
      }
    }

    /* Decode comments from packet 1. Vorbis has 7 bytes of marker at the
     * start of vorbiscomment packet. */
    if (fsv->packetno == 1 && bytes > 7 && buf[0] == 0x03 &&
	!strncmp ((char *)&buf[1], "vorbis", 6)) {
      fish_sound_comments_decode (fsound, buf+7, bytes-7);
    } else if (fsv->packetno == 2) {
      vorbis_synthesis_init (&fsv->vd, &fsv->vi);
      vorbis_block_init (&fsv->vd, &fsv->vb);
    }
  } else {
    if (vorbis_synthesis (&fsv->vb, &op) == 0)
      vorbis_synthesis_blockin (&fsv->vd, &fsv->vb);

    while ((samples = vorbis_synthesis_pcmout (&fsv->vd, &pcm)) > 0) {
      vorbis_synthesis_read (&fsv->vd, samples);

      if (fsound->interleave) {
	if (samples > fsv->max_pcm) {
	  fsv->ipcm = realloc (fsv->ipcm, sizeof(float) * samples *
			       fsound->info.channels);
	  fsv->max_pcm = samples;
	}
	_fs_interleave (pcm, (float **)fsv->ipcm, samples,
			fsound->info.channels, 1.0);
	retpcm = (float **)fsv->ipcm;
      } else {
	retpcm = pcm;
      }

      fsound->frameno += samples;
      
      if (fsound->callback) {
	((FishSoundDecoded)fsound->callback) (fsound, retpcm, samples,
					      fsound->user_data);
      }
    }
  }

  fsv->packetno++;

  return 0;
}
#else /* !FS_DECODE */

#define fs_vorbis_decode NULL

#endif


#if (defined(FS_ENCODE) && defined(HAVE_VORBISENC))

static FishSound *
fs_vorbis_enc_headers (FishSound * fsound)
{
  FishSoundVorbisInfo * fsv = (FishSoundVorbisInfo *)fsound->codec_data;
  const FishSoundComment * comment;
  ogg_packet header;
  ogg_packet header_comm;
  ogg_packet header_code;

  /* Vorbis streams begin with three headers; the initial header (with
     most of the codec setup parameters) which is mandated by the Ogg
     bitstream spec.  The second header holds any comment fields.  The
     third header holds the bitstream codebook.  We merely need to
     make the headers, then pass them to libvorbis one at a time;
     libvorbis handles the additional Ogg bitstream constraints */

  /* Update the comments */
  for (comment = fish_sound_comment_first (fsound); comment;
       comment = fish_sound_comment_next (fsound, comment)) {
#ifdef DEBUG
    fprintf (stderr, "fs_vorbis_enc_headers: %s = %s\n",
	     comment->name, comment->value);
#endif
    vorbis_comment_add_tag (&fsv->vc, comment->name, comment->value);
  }
  
  /* Generate the headers */
  vorbis_analysis_headerout(&fsv->vd, &fsv->vc,
			    &header, &header_comm, &header_code);
  
  /* Pass the generated headers to the user */
  if (fsound->callback) {
    FishSoundEncoded encoded = (FishSoundEncoded)fsound->callback;
    
    encoded (fsound, header.packet, header.bytes, fsound->user_data);
    encoded (fsound, header_comm.packet, header_comm.bytes,
	     fsound->user_data);
    encoded (fsound, header_code.packet, header_code.bytes,
	     fsound->user_data);
    fsv->packetno = 3;
  }

  return fsound;
}

static long
fs_vorbis_encode_write (FishSound * fsound, long len)
{
  FishSoundVorbisInfo * fsv = (FishSoundVorbisInfo *)fsound->codec_data;
  ogg_packet op;

  vorbis_analysis_wrote (&fsv->vd, len);
  
  while (vorbis_analysis_blockout (&fsv->vd, &fsv->vb) == 1) {
    vorbis_analysis (&fsv->vb, NULL);
    vorbis_bitrate_addblock (&fsv->vb);
    
    while (vorbis_bitrate_flushpacket (&fsv->vd, &op)) {
      if (fsound->callback) {
	FishSoundEncoded encoded = (FishSoundEncoded)fsound->callback;

	fsound->frameno = op.granulepos;
	encoded (fsound, op.packet, op.bytes, fsound->user_data);
	fsv->packetno++;
      }
    }
  }

  return len;
}

static long
fs_vorbis_encode_i (FishSound * fsound, float ** pcm, long frames)
{
  FishSoundVorbisInfo * fsv = (FishSoundVorbisInfo *)fsound->codec_data;
  float ** vpcm;
  long len, remaining = frames;
  float * d = (float *)pcm;

  if (fsv->packetno == 0) {
    fs_vorbis_enc_headers (fsound);
  }

  while (remaining > 0) {
    len = MIN (1024, remaining);

    /* expose the buffer to submit data */
    vpcm = vorbis_analysis_buffer (&fsv->vd, 1024);

    _fs_deinterleave ((float **)d, vpcm, len, fsound->info.channels, 1.0);

    d += (len * fsound->info.channels);

    fs_vorbis_encode_write (fsound, len);

    remaining -= len;
  }

  return 0;
}

static long
fs_vorbis_encode_n (FishSound * fsound, float * pcm[], long frames)
{
  FishSoundVorbisInfo * fsv = (FishSoundVorbisInfo *)fsound->codec_data;
  float ** vpcm;
  long len, remaining = frames;
  int i;
  float ** ppcm = alloca (sizeof (float *) * fsound->info.channels);

  if (fsv->packetno == 0) {
    fs_vorbis_enc_headers (fsound);
  }

  for (i = 0; i < fsound->info.channels; i++) {
    ppcm[i] = pcm[i];
  }

  while (remaining > 0) {
    len = MIN (1024, remaining);

#ifdef DEBUG
    printf ("fs_vorbis_encode: processing %ld frames\n", len);
#endif

    /* expose the buffer to submit data */
    vpcm = vorbis_analysis_buffer (&fsv->vd, 1024);

    for (i = 0; i < fsound->info.channels; i++) {
      memcpy (vpcm[i], ppcm[i], sizeof (float) * len);
    }

    fs_vorbis_encode_write (fsound, len);

    remaining -= len;
  }

  return 0;
}

#else /* !FS_ENCODE || !HAVE_VORBISENC */

#define fs_vorbis_encode_i NULL
#define fs_vorbis_encode_n NULL

#endif

static int
fs_vorbis_reset (FishSound * fsound)
{
  FishSoundVorbisInfo * fsv = (FishSoundVorbisInfo *)fsound->codec_data;

  vorbis_block_init (&fsv->vd, &fsv->vb);

  return 0;
}

static FishSound *
fs_vorbis_enc_init (FishSound * fsound)
{
  FishSoundVorbisInfo * fsv = (FishSoundVorbisInfo *)fsound->codec_data;

#ifdef DEBUG
  printf ("Vorbis enc init: %d channels, %d Hz\n", fsound->info.channels,
	  fsound->info.samplerate);
#endif


  vorbis_encode_init_vbr (&fsv->vi, fsound->info.channels,
			  fsound->info.samplerate, (float)0.3 /* quality */);

  vorbis_encode_setup_init (&fsv->vi);

  /* set up the analysis state and auxiliary encoding storage */
  vorbis_analysis_init (&fsv->vd, &fsv->vi);
  vorbis_block_init (&fsv->vd, &fsv->vb);

  return fsound;
}

static FishSound *
fs_vorbis_init (FishSound * fsound)
{
  FishSoundVorbisInfo * fsv;

  fsv = malloc (sizeof (FishSoundVorbisInfo));
  if (fsv == NULL) return NULL;

  fsv->packetno = 0;
  vorbis_info_init (&fsv->vi);
  vorbis_comment_init (&fsv->vc);
  fsv->ipcm = NULL;
  fsv->max_pcm = 0;

  fsound->codec_data = fsv;

  if (fsound->mode == FISH_SOUND_ENCODE) {
    fs_vorbis_enc_init (fsound);
  }

  return fsound;
}

static FishSound *
fs_vorbis_delete (FishSound * fsound)
{
  FishSoundVorbisInfo * fsv = (FishSoundVorbisInfo *)fsound->codec_data;

  vorbis_block_clear (&fsv->vb);
  vorbis_dsp_clear (&fsv->vd);
  vorbis_comment_clear (&fsv->vc);
  vorbis_info_clear (&fsv->vi);

  free (fsv);
  fsound->codec_data = NULL;

  return fsound;
}

static FishSoundFormat fs_vorbis_format = {
  FISH_SOUND_VORBIS,
  "Vorbis (Xiph.Org)",
  "ogg"
};

FishSoundCodec fish_sound_vorbis = {
  &fs_vorbis_format,
  fs_vorbis_identify,
  fs_vorbis_init,
  fs_vorbis_delete,
  fs_vorbis_reset,
  fs_vorbis_command,
  fs_vorbis_decode,
  fs_vorbis_encode_i,
  fs_vorbis_encode_n,
  NULL /* flush */
};

#else /* !HAVE_VORBIS */

static int
fs_novorbis_identify (unsigned char * buf, long bytes)
{
  return FISH_SOUND_UNKNOWN;
}

FishSoundCodec fish_sound_vorbis = {
  NULL, /* format */
  fs_novorbis_identify, /* identify */
  NULL, /* init */
  NULL, /* delete */
  NULL, /* reset */
  NULL, /* command */
  NULL, /* decode */
  NULL, /* encode_i */
  NULL, /* encode_n */
  NULL  /* flush */
};

#endif
