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

#include "private.h"

/*#define DEBUG*/

static char *
fs_strdup (const char * s)
{
  char * ret;
  if (!s) return NULL;
  ret = fs_malloc (strlen(s) + 1);
  return strcpy (ret, s);
}

static char *
fs_strdup_len (const char * s, int len)
{
  char * ret;
  if (!s) return NULL;
  ret = fs_malloc (len + 1);
  if (!strncpy (ret, s, len)) {
    fs_free (ret);
    return NULL;
  }

  ret[len] = '\0';
  return ret;
}

static char *
fs_index_len (const char * s, char c, int len)
{
  int i;

  for (i = 0; *s && i < len; i++, s++) {
    if (*s == c) return (char *)s;
  }

  return NULL;
}

#if 0
static void comment_init(char **comments, int* length, char *vendor_string);
static void comment_add(char **comments, int* length, char *tag, char *val);
#endif

/*                 
 Comments will be stored in the Vorbis style.            
 It is describled in the "Structure" section of
    http://www.xiph.org/ogg/vorbis/doc/v-comment.html

The comment header is decoded as follows:
  1) [vendor_length] = read an unsigned integer of 32 bits
  2) [vendor_string] = read a UTF-8 vector as [vendor_length] octets
  3) [user_comment_list_length] = read an unsigned integer of 32 bits
  4) iterate [user_comment_list_length] times {
     5) [length] = read an unsigned integer of 32 bits
     6) this iteration's user comment = read a UTF-8 vector as [length] octets
     }
  7) [framing_bit] = read a single bit as boolean
  8) if ( [framing_bit]  unset or end of packet ) then ERROR
  9) done.

  If you have troubles, please write to ymnk@jcraft.com.
 */

#define readint(buf, base) (((buf[base+3]<<24)&0xff000000)| \
                           ((buf[base+2]<<16)&0xff0000)| \
                           ((buf[base+1]<<8)&0xff00)| \
  	           	    (buf[base]&0xff))
#define writeint(buf, base, val) do{ buf[base+3]=((val)>>24)&0xff; \
                                     buf[base+2]=((val)>>16)&0xff; \
                                     buf[base+1]=((val)>>8)&0xff; \
                                     buf[base]=(val)&0xff; \
                                 }while(0)

#if 0
static void
comment_init(char **comments, int* length, char *vendor_string)
{
  int vendor_length=strlen(vendor_string);
  int user_comment_list_length=0;
  int len=4+vendor_length+4;
  char *p=(char*)fs_malloc(len);
  if(p==NULL){
  }
  writeint(p, 0, vendor_length);
  memcpy(p+4, vendor_string, vendor_length);
  writeint(p, 4+vendor_length, user_comment_list_length);
  *length=len;
  *comments=p;
}

static void
comment_add(char **comments, int* length, char *tag, char *val)
{
  char* p=*comments;
  int vendor_length=readint(p, 0);
  int user_comment_list_length=readint(p, 4+vendor_length);
  int tag_len=(tag?strlen(tag):0);
  int val_len=strlen(val);
  int len=(*length)+4+tag_len+val_len;

  p=(char*)fs_realloc(p, len);
  if(p==NULL){
  }

  writeint(p, *length, tag_len+val_len);      /* length of comment */
  if(tag) memcpy(p+*length+4, tag, tag_len);  /* comment */
  memcpy(p+*length+4+tag_len, val, val_len);  /* comment */
  writeint(p, 4+vendor_length, user_comment_list_length+1);

  *comments=p;
  *length=len;
}
#endif

static int
fs_comment_validate_byname (const char * name, const char * value)
{
  const char * c;

  if (!name || !value) return 0;

  for (c = name; *c; c++) {
    if (*c < 0x20 || *c > 0x7D || *c == 0x3D) {
#ifdef DEBUG
      printf ("XXX char %c in %s invalid\n", *c, name);
#endif
      return 0;
    }
  }

  /* XXX: we really should validate value as UTF-8 here, but ... */

  return 1;
}

static FishSoundComment *
fs_comment_new (const char * name, const char * value)
{
  FishSoundComment * comment;

  if (!fs_comment_validate_byname (name, value)) return NULL;

  comment = fs_malloc (sizeof (FishSoundComment));
  comment->name = fs_strdup (name);
  comment->value = fs_strdup (value);

  return comment;
}

static void
fs_comment_free (FishSoundComment * comment)
{
  if (!comment) return;
  if (comment->name) fs_free (comment->name);
  if (comment->value) fs_free (comment->value);
  fs_free (comment);
}

static int
fs_comment_cmp (const FishSoundComment * comment1, const FishSoundComment * comment2)
{
  if (comment1 == comment2) return 1;
  if (!comment1 || !comment2) return 0;

  if (strcasecmp (comment1->name, comment2->name)) return 0;
  if (strcmp (comment1->value, comment2->value)) return 0;

  return 1;
}

/* Public API */

const char *
fish_sound_comment_get_vendor (FishSound * fsound)
{
  if (fsound == NULL) return NULL;

  return fsound->vendor;
}

int
fish_sound_comment_set_vendor (FishSound * fsound, const char * vendor_string)
{
  if (fsound == NULL) return FISH_SOUND_ERR_BAD;

  if (fsound->vendor) fs_free (fsound->vendor);

  fsound->vendor = fs_strdup (vendor_string);

  return 0;
}

const FishSoundComment *
fish_sound_comment_first (FishSound * fsound)
{
  if (fsound == NULL) return NULL;

  return fs_vector_nth (fsound->comments, 0);
}

const FishSoundComment *
fish_sound_comment_first_byname (FishSound * fsound, char * name)
{
  FishSoundComment * comment;
  int i;

  if (fsound == NULL) return NULL;

  if (name == NULL) return fs_vector_nth (fsound->comments, 0);

  if (!fs_comment_validate_byname (name, ""))
    return NULL;
  
  for (i = 0; i < fs_vector_size (fsound->comments); i++) {
    comment = (FishSoundComment *) fs_vector_nth (fsound->comments, i);
    if (comment->name && !strcasecmp (name, comment->name))
      return comment;
  }

  return NULL;
}

const FishSoundComment *
fish_sound_comment_next (FishSound * fsound, const FishSoundComment * comment)
{
  int i;

  if (fsound == NULL || comment == NULL) return NULL;

  i = fs_vector_find_index (fsound->comments, comment);

  return fs_vector_nth (fsound->comments, i+1);
}

const FishSoundComment *
fish_sound_comment_next_byname (FishSound * fsound,
				const FishSoundComment * comment)
{
  FishSoundComment * v_comment;
  int i;

  if (fsound == NULL || comment == NULL) return NULL;

  i = fs_vector_find_index (fsound->comments, comment);

  for (i++; i < fs_vector_size (fsound->comments); i++) {
    v_comment = (FishSoundComment *) fs_vector_nth (fsound->comments, i);
    if (v_comment->name && !strcasecmp (comment->name, v_comment->name))
      return v_comment;
  }

  return NULL;
}

#define _fs_comment_add(f,c) fs_vector_insert ((f)->comments, (c))

int
fish_sound_comment_add (FishSound * fsound, FishSoundComment * comment)
{
#if FS_ENCODE
  FishSoundComment * new_comment;
#endif

  if (fsound == NULL) return FISH_SOUND_ERR_BAD;

  if (fsound->mode != FISH_SOUND_ENCODE)
    return FISH_SOUND_ERR_INVALID;

#if FS_ENCODE
  if (!fs_comment_validate_byname (comment->name, comment->value))
    return FISH_SOUND_ERR_COMMENT_INVALID;

  new_comment = fs_comment_new (comment->name, comment->value);

  _fs_comment_add (fsound, new_comment);

  return 0;
#else
  return FISH_SOUND_ERR_DISABLED;
#endif
}

int
fish_sound_comment_add_byname (FishSound * fsound, const char * name,
			       const char * value)
{
#if FS_ENCODE
  FishSoundComment * comment;
#endif

  if (fsound == NULL) return FISH_SOUND_ERR_BAD;

  if (fsound->mode != FISH_SOUND_ENCODE)
    return FISH_SOUND_ERR_INVALID;

#if FS_ENCODE
  if (!fs_comment_validate_byname (name, value))
    return FISH_SOUND_ERR_COMMENT_INVALID;

  comment = fs_comment_new (name, value);

  _fs_comment_add (fsound, comment);

  return 0;

#else
  return FISH_SOUND_ERR_DISABLED;
#endif
}

int
fish_sound_comment_remove (FishSound * fsound, FishSoundComment * comment)
{
#if FS_ENCODE
  FishSoundComment * v_comment;
#endif

  if (fsound == NULL) return FISH_SOUND_ERR_BAD;

  if (fsound->mode != FISH_SOUND_ENCODE)
    return FISH_SOUND_ERR_INVALID;

#if FS_ENCODE

  v_comment = fs_vector_find (fsound->comments, comment);

  if (v_comment == NULL) return 0;

  fs_vector_remove (fsound->comments, v_comment);
  fs_comment_free (v_comment);

  return 1;

#else
  return FISH_SOUND_ERR_DISABLED;
#endif
}

int
fish_sound_comment_remove_byname (FishSound * fsound, char * name)
{
#if FS_ENCODE
  FishSoundComment * comment;
  int i, ret = 0;
#endif

  if (fsound == NULL) return FISH_SOUND_ERR_BAD;

  if (fsound->mode != FISH_SOUND_ENCODE)
    return FISH_SOUND_ERR_INVALID;

#if FS_ENCODE
  for (i = 0; i < fs_vector_size (fsound->comments); i++) {
    comment = (FishSoundComment *) fs_vector_nth (fsound->comments, i);
    if (!strcasecmp (name, comment->name)) {
      fish_sound_comment_remove (fsound, comment);
      i--;
      ret++;
    }
  }

  return ret;

#else
  return FISH_SOUND_ERR_DISABLED;
#endif
}

/* Internal API */
int
fish_sound_comments_init (FishSound * fsound)
{
  fsound->vendor = NULL;
  fsound->comments = fs_vector_new ((FishSoundCmpFunc) fs_comment_cmp);

  return 0;
}

int
fish_sound_comments_free (FishSound * fsound)
{
  fs_vector_foreach (fsound->comments, (FishSoundFunc)fs_comment_free);
  fs_vector_delete (fsound->comments);
  fsound->comments = NULL;

  if (fsound->vendor) fs_free (fsound->vendor);
  fsound->vendor = NULL;

  return 0;
}

int
fish_sound_comments_decode (FishSound * fsound, unsigned char * comments,
			    long length)
{
   char *c= (char *)comments;
   int len, i, nb_fields, n;
   char *end;
   char * name, * value, * nvalue = NULL;
   FishSoundComment * comment;
   
   if (length<8)
      return -1;

   end = c+length;
   len=readint(c, 0);

   c+=4;
   if (c+len>end) return -1;

   /* Vendor */
   nvalue = fs_strdup_len (c, len);
   fish_sound_comment_set_vendor (fsound, nvalue);
#ifdef DEBUG
   fwrite(c, 1, len, stderr); fputc ('\n', stderr);
#endif
   c+=len;

   if (c+4>end) return -1;

   nb_fields=readint(c, 0);
   c+=4;
   for (i=0;i<nb_fields;i++)
   {
      if (c+4>end) return -1;

      len=readint(c, 0);

      c+=4;
      if (c+len>end) return -1;

      name = c;
      value = fs_index_len (c, '=', len);
      if (value) {
	*value = '\0';
	value++;

	n = c+len - value;
	nvalue = fs_strdup_len (value, n);
#ifdef DEBUG
	printf ("fish_sound_comments_decode: %s -> %s (length %d)\n",
		name, nvalue, n);
#endif
	comment = fs_comment_new (name, nvalue);
	_fs_comment_add (fsound, comment);
	fs_free (nvalue);
      } else {
	nvalue = fs_strdup_len (name, len);
	comment = fs_comment_new (nvalue, NULL);
	_fs_comment_add (fsound, comment);
	fs_free (nvalue);
      }

      c+=len;
   }

#ifdef DEBUG
   printf ("fish_sound_comments_decode: done\n");
#endif

   return 0;
}

long
fish_sound_comments_encode (FishSound * fsound, unsigned char * buf,
			    long length)
{
  char * c = (char *)buf;
  const FishSoundComment * comment;
  int nb_fields = 0, vendor_length, field_length;
  long actual_length, remaining = length;

  /* Vendor string */
  vendor_length = strlen (fsound->vendor);
  actual_length = 4 + vendor_length;

  /* user comment list length */
  actual_length += 4;

  for (comment = fish_sound_comment_first (fsound); comment;
       comment = fish_sound_comment_next (fsound, comment)) {
    actual_length += 4 + strlen (comment->name);    /* [size]"name" */
    if (comment->value)
      actual_length += 1 + strlen (comment->value); /* "=value" */

#ifdef DEBUG
    printf ("fish_sound_comments_encode: %s = %s\n",
	    comment->name, comment->value);
#endif

    nb_fields++;
  }

  actual_length++; /* framing bit */

  if (buf == NULL) return actual_length;

  remaining -= 4;
  if (remaining <= 0) return actual_length;
  writeint (c, 0, vendor_length);
  c += 4;

  field_length = strlen (fsound->vendor);
  memcpy (c, fsound->vendor, MIN (field_length, remaining));
  c += field_length; remaining -= field_length;
  if (remaining <= 0 ) return actual_length;

  remaining -= 4;
  if (remaining <= 0) return actual_length;
  writeint (c, 0, nb_fields);
  c += 4;

  for (comment = fish_sound_comment_first (fsound); comment;
       comment = fish_sound_comment_next (fsound, comment)) {

    field_length = strlen (comment->name);     /* [size]"name" */
    if (comment->value)
      field_length += 1 + strlen (comment->value); /* "=value" */

    remaining -= 4;
    if (remaining <= 0) return actual_length;
    writeint (c, 0, field_length);
    c += 4;

    field_length = strlen (comment->name);
    memcpy (c, comment->name, MIN (field_length, remaining));
    c += field_length; remaining -= field_length;
    if (remaining <= 0) return actual_length;

    if (comment->value) {
      remaining --;
      if (remaining <= 0) return actual_length;
      *c = '=';
      c++;

      field_length = strlen (comment->value);
      memcpy (c, comment->value, MIN (field_length, remaining));
      c += field_length; remaining -= field_length;
      if (remaining <= 0) return actual_length;
    }
  }

  if (remaining <= 0) return actual_length;
  *c = 0x01;

  return actual_length;
}
