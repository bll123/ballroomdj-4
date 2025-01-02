/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include <opus/opusfile.h>

#include "ati.h"
#include "atibdj4.h"
#include "audiofile.h"
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "tagdef.h"

typedef struct atisaved {
  bool                  hasdata;
  int                   filetype;
  int                   tagtype;
  OpusTags              *tags;
} atisaved_t;

static void atibdj4OpusAddVorbisComment (OpusTags *newtags, int tagkey, const char *tagname, const char *val);

void
atibdj4ParseOpusTags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  OggOpusFile           *of;
  int                   linknum;
  const OpusTags        *tags = NULL;
  int                   rc;
  double                ddur;
  char                  tmp [40];

  of = op_open_file (ffn, &rc);
  if (rc < 0 || of == NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "op_open_file %d %s", rc, ffn);
    return;
  }

  linknum = op_current_link (of);
  tags = op_tags (of, linknum);
  if (tags == NULL) {
    return;
  }

  ddur = op_pcm_total (of, -1);
  ddur *= 1000.0;
  /* opus has a fixed sample rate of 48kHz */
  ddur /= 48000.0;
  snprintf (tmp, sizeof (tmp), "%.0f", ddur);
  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "duration: %s", tmp);
  slistSetStr (tagdata, atidata->tagName (TAG_DURATION), tmp);

  for (int i = 0; i < tags->comments; ++i) {
    const char  *kw;

    kw = tags->user_comments [i];
    atioggProcessVorbisCommentCombined (atidata->tagLookup, tagdata, tagtype, kw);
  }
  op_free (of);
  return;
}

int
atibdj4WriteOpusTags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype, int32_t flags)
{
  OggOpusFile           *of;
  int                   linknum;
  const OpusTags        *tags = NULL;
  int                   rc = -1;
  OpusTags              newtags;
  slistidx_t            iteridx;
  slist_t               *upddone;
  const char            *key;
  int                   writetags;
  int                   tagkey;
  const tagaudiotag_t   *audiotag = NULL;

  writetags = atidata->writetags;
  of = op_open_file (ffn, &rc);
  if (rc < 0 || of == NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "op_open_file %d %s", rc, ffn);
    return -1;
  }

  linknum = op_current_link (of);
  tags = op_tags (of, linknum);

  opus_tags_init (&newtags);
  upddone = slistAlloc ("upd-done", LIST_ORDERED, NULL);

  for (int i = 0; i < tags->comments; ++i) {
    const char  *kw;
    const char  *val;
    char        ttag [300];     /* vorbis tag name */
    const char  *tagname;

    kw = tags->user_comments [i];
    val = atioggParseVorbisComment (kw, ttag, sizeof (ttag));
    tagname = atidata->tagLookup (tagtype, ttag);

    if (slistGetStr (dellist, tagname) != NULL) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: del: %s", audiotag->tag);
      continue;
    }

    tagkey = atidata->tagCheck (writetags, tagtype, tagname, AF_REWRITE_NONE);
    if (tagkey >= 0) {
      audiotag = atidata->audioTagLookup (tagkey, tagtype);
    }

    if (slistGetStr (updatelist, tagname) != NULL) {
      if (slistGetNum (upddone, tagname) == 1) {
        continue;
      }
      if (tagkey < 0) {
        continue;
      }
      val = slistGetStr (updatelist, tagname);
      atibdj4OpusAddVorbisComment (&newtags, tagkey, audiotag->tag, val);
      slistSetNum (upddone, tagname, 1);
    } else {
      /* the tag has not changed, or is unknown to bdj4 */
      opus_tags_add_comment (&newtags, kw);
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: existing: %s", kw);
    }
  }

  slistStartIterator (updatelist, &iteridx);
  while ((key = slistIterateKey (updatelist, &iteridx)) != NULL) {
    const char  *tval;

    if (slistGetNum (upddone, key) > 0) {
      continue;
    }

    tagkey = atidata->tagCheck (writetags, tagtype, key, AF_REWRITE_NONE);
    if (tagkey < 0) {
      continue;
    }
    audiotag = atidata->audioTagLookup (tagkey, tagtype);

    tval = slistGetStr (updatelist, key);
    atibdj4OpusAddVorbisComment (&newtags, tagkey, audiotag->tag, tval);
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: new: %s=%s", audiotag->tag, tval);
  }
  slistFree (upddone);

  op_free (of);

  rc = atioggWriteOggFile (ffn, &newtags, filetype);

  opus_tags_clear (&newtags);

  return rc;
}

atisaved_t *
atibdj4SaveOpusTags (atidata_t *atidata,
    const char *ffn, int tagtype, int filetype)
{
  OggOpusFile           *of;
  int                   linknum;
  atisaved_t            *atisaved;
  int                   rc = -1;
  const OpusTags        *tags = NULL;

  of = op_open_file (ffn, &rc);
  if (rc < 0 || of == NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "op_open_file %d %s", rc, ffn);
    return NULL;
  }
  linknum = op_current_link (of);
  tags = op_tags (of, linknum);
  if (tags == NULL) {
    return NULL;
  }

  atisaved = mdmalloc (sizeof (atisaved_t));
  atisaved->hasdata = true;
  atisaved->tagtype = tagtype;
  atisaved->filetype = filetype;
  atisaved->tags = mdmalloc (sizeof (OpusTags));
  opus_tags_init (atisaved->tags);
  opus_tags_copy (atisaved->tags, tags);
  op_free (of);

  return atisaved;
}

void
atibdj4FreeSavedOpusTags (atisaved_t *atisaved, int tagtype, int filetype)
{
  if (atisaved == NULL) {
    return;
  }
  if (! atisaved->hasdata) {
    return;
  }
  if (atisaved->tagtype != tagtype) {
    return;
  }
  if (atisaved->filetype != filetype) {
    return;
  }

  atisaved->hasdata = false;
  opus_tags_clear (atisaved->tags);
  mdfree (atisaved->tags);
  mdfree (atisaved);
}

int
atibdj4RestoreOpusTags (atidata_t *atidata,
    atisaved_t *atisaved, const char *ffn, int tagtype, int filetype)
{
  int     rc = -1;

  if (atisaved == NULL) {
    return -1;
  }
  if (! atisaved->hasdata) {
    return -1;
  }
  if (atisaved->tagtype != tagtype) {
    return -1;
  }
  if (atisaved->filetype != filetype) {
    return -1;
  }

  rc = atioggWriteOggFile (ffn, atisaved->tags, filetype);

  return rc;
}

void
atibdj4CleanOpusTags (atidata_t *atidata,
    const char *ffn, int tagtype, int filetype)
{
  OpusTags    tags;

  opus_tags_init (&tags);
  atioggWriteOggFile (ffn, &tags, filetype);
  opus_tags_clear (&tags);
  return;
}

void
atibdj4LogOpusVersion (void)
{
  logMsg (LOG_DBG, LOG_INFO, "opus version %s", opus_get_version_string());
}

/* internal routines */

static void
atibdj4OpusAddVorbisComment (OpusTags *newtags, int tagkey,
    const char *tagname, const char *val)
{
  slist_t     *vallist;
  slistidx_t  viteridx;
  const char  *tval;

  vallist = atioggSplitVorbisComment (tagkey, tagname, val);
  slistStartIterator (vallist, &viteridx);
  while ((tval = slistIterateKey (vallist, &viteridx)) != NULL) {
    opus_tags_add (newtags, tagname, tval);
  }
  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: update: %s=%s", tagname, val);
  slistFree (vallist);
}
