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

#include "ati.h"
#include "audiofile.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "tagdef.h"

#define MB_TAG      "http://musicbrainz.org"

typedef struct {
  char          *ffn;
} atibdj4data_t;

typedef struct atidata {
  int               writetags;
  taglookup_t       tagLookup;
  tagcheck_t        tagCheck;
  tagname_t         tagName;
  audiotaglookup_t  audioTagLookup;
  atibdj4data_t     *data;
} atidata_t;

static void atibdj4ParseMP3Tags (atidata_t *atidata, slist_t *tagdata, atibdj4data_t *data, int tagtype, int *rewrite);
static void atibdj4ParseOggVorbisTags (atidata_t *atidata, slist_t *tagdata, atibdj4data_t *data, int tagtype, int *rewrite);
static void atibdj4ParseFlacVorbisTags (atidata_t *atidata, slist_t *tagdata, atibdj4data_t *data, int tagtype, int *rewrite);
static void atibdj4ParseMP4Tags (atidata_t *atidata, slist_t *tagdata, atibdj4data_t *data, int tagtype, int *rewrite);
static void atiffmpegLogCallback (void *avcl, int level, const char *fmt, va_list vl);

const char *
atiiDesc (void)
{
  return "BDJ4: MP3 Only";
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
  atidata->data = NULL;

  /* turn off logging */
  av_log_set_callback (atiffmpegLogCallback);

  return atidata;
}

void
atiiFree (atidata_t *atidata)
{
  if (atidata != NULL) {
    if (atidata->data != NULL) {
      dataFree (atidata->data->ffn);
      dataFree (atidata->data);
    }
    mdfree (atidata);
  }
}

void *
atiiReadTags (atidata_t *atidata, const char *ffn)
{
  atibdj4data_t   *data = NULL;

  data = mdmalloc (sizeof (atibdj4data_t));
  data->ffn = mdstrdup (ffn);
  return data;
}

void
atiiParseTags (atidata_t *atidata, slist_t *tagdata, void *tdata,
    int filetype, int tagtype, int *rewrite)
{
  atibdj4data_t     *data = tdata;
  AVFormatContext   *ictx = NULL;
  char              pbuff [100];
  int32_t           duration;
  int               rc;

  if ((rc = avformat_open_input (&ictx, data->ffn, NULL, NULL))) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "unable to open %d %s\n", rc, data->ffn);
    return;
  }

  if ((rc = avformat_find_stream_info (ictx, NULL)) < 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "unable to load stream info %d %s\n", rc, data->ffn);
    avformat_close_input (&ictx);
    return;
  }

  duration = ictx->duration;
  duration /= 1000;
  snprintf (pbuff, sizeof (pbuff), "%ld", (long) duration);
  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "duration: %s\n", pbuff);
  slistSetStr (tagdata, atidata->tagName (TAG_DURATION), pbuff);

  avformat_close_input (&ictx);

  if (tagtype == TAG_TYPE_MP3) {
    atibdj4ParseMP3Tags (atidata, tagdata, data, tagtype, rewrite);
  }
  if ((filetype == AFILE_TYPE_OGGVORBIS ||
      filetype == AFILE_TYPE_OGGOPUS) &&
      tagtype == TAG_TYPE_VORBIS) {
    atibdj4ParseOggVorbisTags (atidata, tagdata, data, tagtype, rewrite);
  }
  if (filetype == AFILE_TYPE_FLAC && tagtype == TAG_TYPE_VORBIS) {
    atibdj4ParseFlacVorbisTags (atidata, tagdata, data, tagtype, rewrite);
  }
  if (tagtype == TAG_TYPE_MP4) {
    atibdj4ParseMP4Tags (atidata, tagdata, data, tagtype, rewrite);
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
    atibdj4data_t *data, int tagtype, int *rewrite)
{
  struct id3_file   *id3file;
  struct id3_tag    *id3tag;
  struct id3_frame  *id3frame;
  int               idx;

  id3file = id3_file_open (data->ffn, ID3_FILE_MODE_READWRITE);
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
          id3_latin1_t const *str;

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

            ustr = id3_field_getstrings (field, j);
            str = id3_ucs4_utf8duplicate (ustr);
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "%s raw: %s=%s", tagname, id3frame->id, str);
            slistSetStr (tagdata, tagname, (const char *) str);
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
atibdj4ParseOggVorbisTags (atidata_t *atidata, slist_t *tagdata,
    atibdj4data_t *data, int tagtype, int *rewrite)
{
  OggVorbis_File        ovf;
  int                   rc;
  struct vorbis_comment *vc;

  rc = ov_fopen (data->ffn, &ovf);
  vc = ovf.vc;
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
atibdj4ParseFlacVorbisTags (atidata_t *atidata, slist_t *tagdata,
    atibdj4data_t *data, int tagtype, int *rewrite)
{
  return;
}

static void
atibdj4ParseMP4Tags (atidata_t *atidata, slist_t *tagdata,
    atibdj4data_t *data, int tagtype, int *rewrite)
{
  return;
}

static void
atiffmpegLogCallback (void *avcl, int level, const char *fmt, va_list vl)
{
//  vfprintf (stderr, fmt, vl);
  return;
}
