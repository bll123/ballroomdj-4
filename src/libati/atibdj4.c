/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include <libavformat/avformat.h>
#include <id3tag.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <opus/opusfile.h>
#include <FLAC/metadata.h>

#include "ati.h"
#include "audiofile.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "tagdef.h"

#define MB_TAG      "http://musicbrainz.org"

typedef struct atidata {
  int               writetags;
  taglookup_t       tagLookup;
  tagcheck_t        tagCheck;
  tagname_t         tagName;
  audiotaglookup_t  audioTagLookup;
} atidata_t;

static void atibdj4ParseMP3Tags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
static void atibdj4ParseOggTags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
static void atibdj4ParseOpusTags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
static void atibdj4ParseFlacTags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
static void atibdj4ParseMP4Tags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
static void atiffmpegLogCallback (void *avcl, int level, const char *fmt, va_list vl);

const char *
atiiDesc (void)
{
  return "BDJ4: MP3/Ogg/Opus/FLAC";
}

atidata_t *
atiiInit (const char *atipkg, int writetags,
    taglookup_t tagLookup, tagcheck_t tagCheck,
    tagname_t tagName, audiotaglookup_t audioTagLookup)
{
  atidata_t *atidata;

  atidata = mdmalloc (sizeof (atidata_t));
  atidata->writetags = writetags;
  atidata->tagLookup = tagLookup;
  atidata->tagCheck = tagCheck;
  atidata->tagName = tagName;
  atidata->audioTagLookup = audioTagLookup;

  /* turn off logging */
  av_log_set_callback (atiffmpegLogCallback);

  return atidata;
}

void
atiiFree (atidata_t *atidata)
{
  if (atidata != NULL) {
    mdfree (atidata);
  }
}

bool
atiiUseReader (void)
{
  return false;
}

char *
atiiReadTags (atidata_t *atidata, const char *ffn)
{
  return NULL;
}

void
atiiParseTags (atidata_t *atidata, slist_t *tagdata, const char *ffn,
    char *data, int filetype, int tagtype, int *rewrite)
{
  AVFormatContext   *ictx = NULL;
  char              pbuff [100];
  int32_t           duration;
  int               rc;

  if (! fileopFileExists (ffn)) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "no file %s", ffn);
    return;
  }

  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "parse tags %s", ffn);

  if ((rc = avformat_open_input (&ictx, ffn, NULL, NULL))) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "unable to open %d %s", rc, ffn);
    return;
  }

  if ((rc = avformat_find_stream_info (ictx, NULL)) < 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "unable to load stream info %d %s", rc, ffn);
    avformat_close_input (&ictx);
    return;
  }

  duration = ictx->duration;
  duration /= 1000;
  snprintf (pbuff, sizeof (pbuff), "%ld", (long) duration);
  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "duration: %s", pbuff);
  slistSetStr (tagdata, atidata->tagName (TAG_DURATION), pbuff);

  avformat_close_input (&ictx);

  if (tagtype == TAG_TYPE_MP3) {
    atibdj4ParseMP3Tags (atidata, tagdata, ffn, tagtype, rewrite);
  }
  if (filetype == AFILE_TYPE_OGG) {
    atibdj4ParseOggTags (atidata, tagdata, ffn, tagtype, rewrite);
  }
  if (filetype == AFILE_TYPE_OPUS) {
    atibdj4ParseOpusTags (atidata, tagdata, ffn, tagtype, rewrite);
  }
  if (filetype == AFILE_TYPE_FLAC) {
    atibdj4ParseFlacTags (atidata, tagdata, ffn, tagtype, rewrite);
  }
  if (tagtype == TAG_TYPE_MP4) {
    atibdj4ParseMP4Tags (atidata, tagdata, ffn, tagtype, rewrite);
  }

  return;
}

int
atiiWriteTags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  int         rc = -1;

  return rc;
}

static void
atibdj4ParseMP3Tags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  struct id3_file   *id3file;
  struct id3_tag    *id3tag;
  struct id3_frame  *id3frame;
  int               idx;
  char              pbuff [100];

  id3file = id3_file_open (ffn, ID3_FILE_MODE_READWRITE);
  id3tag = id3_file_tag (id3file);

  idx = 0;
  while ((id3frame = id3_tag_findframe (id3tag, "", idx)) != NULL) {
    const char          *tagname;
    id3_latin1_t const  *ufid;

    tagname = NULL;
    ufid = NULL;
    if (strcmp (id3frame->id, "TXXX") != 0 &&
        strcmp (id3frame->id, "UFID") != 0) {
      tagname = atidata->tagLookup (tagtype, id3frame->id);
      if (tagname == NULL) {
        tagname = id3frame->id;
      }
    }

    for (size_t i = 0; i < id3frame->nfields; ++i) {
      const union id3_field   *field;

      field = &id3frame->fields [i];

      switch (id3frame->fields [i].type) {
        case ID3_FIELD_TYPE_TEXTENCODING: {
          enum id3_field_textencoding   te;

          te = id3_field_gettextencoding (field);
          break;
        }
        case ID3_FIELD_TYPE_FRAMEID: {
          char const *str;

          str = id3_field_getframeid (field);
          break;
        }
        case ID3_FIELD_TYPE_LATIN1: {
          id3_latin1_t const  *str;

          str = id3_field_getlatin1 (field);
          if (strcmp (id3frame->id, "UFID") == 0) {
            ufid = str;
            tagname = atidata->tagLookup (tagtype, (const char *) str);
          } else {
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "%s raw: %s=%s", tagname, id3frame->id, str);
            slistSetStr (tagdata, tagname, (const char *) str);
          }
          break;
        }
        case ID3_FIELD_TYPE_LATIN1FULL: {
          id3_latin1_t const *str;

          str = id3_field_getfulllatin1 (field);
          logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "%s raw: %s=%s", tagname, id3frame->id, str);
          slistSetStr (tagdata, tagname, (const char *) str);
          break;
        }
        case ID3_FIELD_TYPE_LATIN1LIST: {
          break;
        }
        case ID3_FIELD_TYPE_STRING: {
          id3_ucs4_t const *ustr;
          id3_utf8_t        *str;

          ustr = id3_field_getstring (field);
          str = id3_ucs4_utf8duplicate (ustr);
          if (tagname == NULL) {
            tagname = atidata->tagLookup (tagtype, (const char *) str);
          } else {
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "%s raw: %s=%s", tagname, id3frame->id, str);
            slistSetStr (tagdata, tagname, (const char *) str);
          }
          break;
        }
        case ID3_FIELD_TYPE_STRINGFULL: {
          id3_ucs4_t const  *ustr;
          id3_utf8_t        *str;

          ustr = id3_field_getstring (field);
          str = id3_ucs4_utf8duplicate (ustr);
          logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "%s raw: %s=%s", tagname, id3frame->id, str);
          slistSetStr (tagdata, tagname, (const char *) str);
          break;
        }
        case ID3_FIELD_TYPE_STRINGLIST: {
          size_t    nstr;

          nstr = id3_field_getnstrings (field);
          for (size_t j = 0; j < nstr; ++j) {
            id3_ucs4_t const  *ustr;
            id3_utf8_t        *str;
            const char        *p;

            ustr = id3_field_getstrings (field, j);
            str = id3_ucs4_utf8duplicate (ustr);
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "%s raw: %s=%s", tagname, id3frame->id, str);
            p = (const char *) str;
            if (strcmp (tagname, tagdefs [TAG_DISCNUMBER].tag) == 0) {
                p = atiParsePair (tagdata, atidata->tagName (TAG_DISCTOTAL),
                    p, pbuff, sizeof (pbuff));
            }
            if (strcmp (tagname, tagdefs [TAG_TRACKNUMBER].tag) == 0) {
                p = atiParsePair (tagdata, atidata->tagName (TAG_TRACKTOTAL),
                    p, pbuff, sizeof (pbuff));
            }
            slistSetStr (tagdata, tagname, p);
          }
          break;
        }
        case ID3_FIELD_TYPE_LANGUAGE: {
          break;
        }
        case ID3_FIELD_TYPE_DATE: {
          id3_latin1_t const *str;

          str = id3_field_getlatin1 (field);
          logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "%s raw: %s=%s", tagname, id3frame->id, str);
          slistSetStr (tagdata, tagname, (const char *) str);
          break;
        }
        case ID3_FIELD_TYPE_INT8:
        case ID3_FIELD_TYPE_INT16:
        case ID3_FIELD_TYPE_INT24:
        case ID3_FIELD_TYPE_INT32:
        case ID3_FIELD_TYPE_INT32PLUS: {
          long    val;
          char    tmp [40];

          val = id3_field_getint (field);
          snprintf (tmp, sizeof (tmp), "%ld", val);
          logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "%s raw: %s=%s", tagname, id3frame->id, tmp);
          slistSetStr (tagdata, tagname, tmp);
          break;
        }
        case ID3_FIELD_TYPE_BINARYDATA: {
          id3_byte_t const  *bstr;
          id3_length_t      blen;
          char              tmp [100];

          if (ufid != NULL && strcmp ((const char *) ufid, MB_TAG) == 0) {
            bstr = id3_field_getbinarydata (field, &blen);
            memcpy (tmp, bstr, blen);
            tmp [blen] = '\0';
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "%s raw: %s=%s=%s", tagname, id3frame->id, ufid, tmp);
            slistSetStr (tagdata, tagname, tmp);
          }
          break;
        }
      }
    }
    ++idx;
  }

  id3_file_close (id3file);
}

static void
atibdj4ParseOggTags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  OggVorbis_File        ovf;
  int                   rc;
  struct vorbis_comment *vc;

  rc = ov_fopen (ffn, &ovf);
  if (rc < 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "bad return %d %s", rc, ffn);
    return;
  }

  vc = ovf.vc;
  if (vc == NULL) {
fprintf (stderr, "null %s\n", ffn);
    return;
  }

  for (int i = 0; i < vc->comments; ++i) {
    const char  *val;
    const char  *kw;
    const char  *tagname;
    int         len;
    char        tmp [100];

    kw = vc->user_comments [i];
    val = strstr (kw, "=");
    if (val == NULL) {
      continue;
    }
    len = val - kw;
    strlcpy (tmp, kw, len + 1);
    tmp [len] = '\0';
    ++val;
    tagname = atidata->tagLookup (tagtype, tmp);
    if (tagname == NULL) {
      tagname = tmp;
    }
    slistSetStr (tagdata, tagname, val);
  }
  ov_clear (&ovf);
  return;
}

static void
atibdj4ParseOpusTags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  OggOpusFile           *of;
  int                   linknum;
  const OpusTags        *tags = NULL;
  int                   rc;

  of = op_open_file (ffn, &rc);
  if (rc < 0 || of == NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "bad return %d %s", rc, ffn);
    return;
  }

  linknum = op_current_link (of);
  tags = op_tags (of, linknum);
  if (tags == NULL) {
fprintf (stderr, "null tags %s\n", ffn);
    return;
  }

  for (int i = 0; i < tags->comments; ++i) {
    const char  *val;
    const char  *kw;
    const char  *tagname;
    int         len;
    char        tmp [100];

    kw = tags->user_comments [i];
    if (opus_tagncompare ("METADATA_BLOCK_PICTURE", 22, kw) == 0) {
      continue;
    }

    val = strstr (kw, "=");
    if (val == NULL) {
      continue;
    }
    len = val - kw;
    strlcpy (tmp, kw, len + 1);
    tmp [len] = '\0';
    ++val;
    tagname = atidata->tagLookup (tagtype, tmp);
    if (tagname == NULL) {
      tagname = tmp;
    }
    slistSetStr (tagdata, tagname, val);
  }
  op_free (of);
  return;
}

static void
atibdj4ParseFlacTags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  return;
}

static void
atibdj4ParseMP4Tags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  return;
}

static void
atiffmpegLogCallback (void *avcl, int level, const char *fmt, va_list vl)
{
//  vfprintf (stderr, fmt, vl);
  return;
}
