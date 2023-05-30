/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include <libavformat/avformat.h>
#include <libavutil/tree.h>

#include "ati.h"
#include "audiofile.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "osprocess.h"
#include "pathbld.h"
#include "sysvars.h"
#include "tagdef.h"

typedef struct atidata {
  int               writetags;
  taglookup_t       tagLookup;
  tagcheck_t        tagCheck;
  tagname_t         tagName;
  audiotaglookup_t  audioTagLookup;
} atidata_t;

const char *
atiiDesc (void)
{
  return "ffmpeg";
}

static void atiffmpegLogCallback (void *avcl, int level, const char *fmt, va_list vl);

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

char *
atiiReadTags (atidata_t *atidata, const char *ffn)
{
  char    *data;

  data = mdstrdup (ffn);
  return data;
}

void
atiiParseTags (atidata_t *atidata, slist_t *tagdata, char *data,
    int tagtype, int *rewrite)
{
  AVFormatContext         *fctx = NULL;
  const AVDictionary      *dict = NULL;
  const AVDictionaryEntry *tag = NULL;
  int                     rc;
  int64_t                 duration;
  char                    pbuff [100];
  int                     writetags;

  writetags = atidata->writetags;

  if ((rc = avformat_open_input (&fctx, data, NULL, NULL))) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "unable to open %d %s\n", rc, data);
    return;
  }

  if ((rc = avformat_find_stream_info (fctx, NULL)) < 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "unable to load stream info %d %s\n", rc, data);
    avformat_close_input (&fctx);
    return;
  }

  duration = fctx->duration;
  duration /= 1000;
  snprintf (pbuff, sizeof (pbuff), "%ld", (long) duration);
  slistSetStr (tagdata, atidata->tagName (TAG_DURATION), pbuff);

  dict = fctx->metadata;
  /* ogg stores the metadata in with the stream, apparently */
  if (av_dict_count (dict) == 0) {
    for (unsigned int i = 0; i < fctx->nb_streams; ++i) {
      dict = fctx->streams [i]->metadata;
      if (av_dict_count (dict) > 0) {
        break;
      }
    }
  }

  while ((tag = av_dict_get (dict, "", tag, AV_DICT_IGNORE_SUFFIX)) != NULL) {
    const char  *tagname;
    int         tagkey;

    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw: %s=%s", tag->key, tag->value);

    /* use the ffmpeg tag type, not the audio file type, */
    /* as ffmpeg converts the tag names */
    tagname = atidata->tagLookup (TAG_TYPE_FFMPEG, tag->key);
    if (tagname == NULL) {
      tagname = atidata->tagLookup (tagtype, tag->key);
    }
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag: %s raw-tag: %s", tagname, tag->key);

    if (tagname != NULL && *tagname) {
      const char  *p;

      p = tag->value;

      if (strcmp (tag->key, "VARIOUSARTISTS") == 0) {
        logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "rewrite: various");
        *rewrite |= AF_REWRITE_VARIOUS;
        continue;
      }

      /* again, use the ffmpeg tag type here */
      tagkey = atidata->tagCheck (writetags, TAG_TYPE_FFMPEG, tagname, AF_REWRITE_NONE);

      if (tagkey == TAG_TRACKNUMBER) {
        p = (char *) atiParsePair (tagdata, atidata->tagName (TAG_TRACKTOTAL),
            p, pbuff, sizeof (pbuff));
      }

      /* disc number / disc total handling */
      if (tagkey == TAG_DISCNUMBER) {
        p = (char *) atiParsePair (tagdata, atidata->tagName (TAG_DISCTOTAL),
            p, pbuff, sizeof (pbuff));
      }

      slistSetStr (tagdata, tagname, p);
    }
  }

  avformat_close_input (&fctx);
  return;
}


int
atiiWriteTags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  AVFormatContext   *fctx = NULL;
  int               rc = -1;

  if ((rc = avformat_open_input (&fctx, ffn, NULL, NULL))) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "unable to open %d %s\n", rc, ffn);
    return -1;
  }

  if ((rc = avformat_find_stream_info (fctx, NULL)) < 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "unable to load stream info %d %s\n", rc, ffn);
    avformat_close_input (&fctx);
    return -1;
  }

  avformat_close_input (&fctx);

  return rc;
}


static void
atiffmpegLogCallback (void *avcl, int level, const char *fmt, va_list vl)
{
  return;
}
