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

#include "ati.h"
#include "atibdj4.h"
#include "audiofile.h"
#include "filemanip.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "tagdef.h"

static void atibdj4LogCallback (void *avcl, int level, const char *fmt, va_list vl);
static void atibdj4LogVersion (void);
static bool gversionlogged = false;

const char *
atiiDesc (void)
{
  return "BDJ4 Internal";
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

  /* turn off logging for ffmpeg */
  av_log_set_callback (atibdj4LogCallback);

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
  bool              needduration = true;

  atibdj4LogVersion ();

  if (! fileopFileExists (ffn)) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "no file %s", ffn);
    return;
  }

  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "parse tags %s", ffn);

  if (tagtype == TAG_TYPE_MP3) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: mp3");
    atibdj4ParseMP3Tags (atidata, tagdata, ffn, tagtype, rewrite);
  }
  if (filetype == AFILE_TYPE_OGG) {
    needduration = false;
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: ogg");
    atibdj4ParseOggTags (atidata, tagdata, ffn, tagtype, rewrite);
  }
  if (filetype == AFILE_TYPE_OPUS) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: opus");
    atibdj4ParseOpusTags (atidata, tagdata, ffn, tagtype, rewrite);
  }
  if (filetype == AFILE_TYPE_FLAC) {
    needduration = false;
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: flac");
    atibdj4ParseFlacTags (atidata, tagdata, ffn, tagtype, rewrite);
  }
  if (tagtype == TAG_TYPE_MP4) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: mp4");
    atibdj4ParseMP4Tags (atidata, tagdata, ffn, tagtype, rewrite);
  }

  if (needduration) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "duration: use avformat");

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
  }

  return;
}

int
atiiWriteTags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  int         rc = -1;

  atibdj4LogVersion ();

  if (! fileopFileExists (ffn)) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "no file %s", ffn);
    return -1;
  }

  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "write tags %s", ffn);

  if (tagtype == TAG_TYPE_MP3) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: mp3");
    rc = atibdj4WriteMP3Tags (atidata, ffn, updatelist, dellist, datalist, tagtype, filetype);
  }
  if (filetype == AFILE_TYPE_OGG) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: ogg");
    rc = atibdj4WriteOggTags (atidata, ffn, updatelist, dellist, datalist, tagtype, filetype);
  }
  if (filetype == AFILE_TYPE_OPUS) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: opus");
    rc = atibdj4WriteOpusTags (atidata, ffn, updatelist, dellist, datalist, tagtype, filetype);
  }
  if (filetype == AFILE_TYPE_FLAC) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: flac");
    rc = atibdj4WriteFlacTags (atidata, ffn, updatelist, dellist, datalist, tagtype, filetype);
  }
  if (tagtype == TAG_TYPE_MP4) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: mp4");
    rc = atibdj4WriteMP4Tags (atidata, ffn, updatelist, dellist, datalist, tagtype, filetype);
  }

  return rc;
}

atisaved_t *
atiiSaveTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype)
{
  atisaved_t *atisaved = NULL;

  if (tagtype == TAG_TYPE_MP3) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: mp3");
    atisaved = atibdj4SaveMP3Tags (atidata, ffn, tagtype, filetype);
  }
  return atisaved;
}

int
atiiRestoreTags (atidata_t *atidata, atisaved_t *atisaved,
    const char *ffn, int tagtype, int filetype)
{
  if (tagtype == TAG_TYPE_MP3) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: mp3");
    atibdj4RestoreMP3Tags (atidata, atisaved, ffn, tagtype, filetype);
  }
  return 0;
}

void
atibdj4ProcessVorbisComment (atidata_t *atidata, slist_t *tagdata,
    int tagtype, const char *kw)
{
  const char  *val;
  const char  *tagname;
  char        ttag [300];

  val = atibdj4ParseVorbisComment (kw, ttag, sizeof (ttag));
  tagname = atidata->tagLookup (tagtype, ttag);
  if (tagname == NULL) {
    tagname = ttag;
  }
  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw: %s %s", tagname, kw);
  slistSetStr (tagdata, tagname, val);
}

const char *
atibdj4ParseVorbisComment (const char *kw, char *buff, size_t sz)
{
  const char  *val;
  size_t      len;

  val = strstr (kw, "=");
  if (val == NULL) {
    return NULL;
  }
  len = val - kw;
  if (len >= sz) {
    len = sz - 1;
  }
  strlcpy (buff, kw, len + 1);
  buff [len] = '\0';
  ++val;

  return val;
}

/* internal routines */

/* for logging ffmpeg output */
static void
atibdj4LogCallback (void *avcl, int level, const char *fmt, va_list vl)
{
//  vfprintf (stderr, fmt, vl);
  return;
}

static void
atibdj4LogVersion (void)
{
  if (! gversionlogged) {
    atibdj4LogMP3Version ();
    atibdj4LogOggVersion ();
    atibdj4LogOpusVersion ();
    gversionlogged = true;
  }
}
