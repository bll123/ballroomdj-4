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

void
atibdj4ParseOpusTags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  OggOpusFile           *of;
  int                   linknum;
  const OpusTags        *tags = NULL;
  int                   rc;

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

  for (int i = 0; i < tags->comments; ++i) {
    const char  *kw;

    kw = tags->user_comments [i];
    if (opus_tagncompare ("METADATA_BLOCK_PICTURE", 22, kw) == 0) {
      continue;
    }

    atiProcessVorbisCommentCombined (atidata->tagLookup, tagdata, tagtype, kw);
  }
  op_free (of);
  return;
}

int
atibdj4WriteOpusTags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
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

    kw = tags->user_comments [i];
    if (opus_tagncompare ("METADATA_BLOCK_PICTURE", 22, kw) == 0) {
      continue;
    }

    val = atiParseVorbisComment (kw, ttag, sizeof (ttag));

    if (slistGetStr (dellist, ttag) != NULL) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: del: %s", ttag);
      continue;
    }

    if (slistGetStr (updatelist, ttag) != NULL) {
      if (slistGetNum (upddone, ttag) == 1) {
        continue;
      }

      tagkey = atidata->tagCheck (writetags, tagtype, ttag, AF_REWRITE_NONE);
      if (tagkey < 0) {
        continue;
      }

      val = slistGetStr (updatelist, ttag);
      atibdj4OggAddVorbisComment (&newtags, tagkey, ttag, val);
      slistSetNum (upddone, ttag, 1);
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

    tval = slistGetStr (updatelist, key);
    atibdj4OggAddVorbisComment (&newtags, tagkey, key, tval);
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: new: %s=%s", key, tval);
  }
  slistFree (upddone);

  op_free (of);

  rc = atibdj4WriteOggFile (ffn, &newtags);

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
  atisaved->hasdata = false;
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

  rc = atibdj4WriteOggFile (ffn, atisaved->tags);

  return -1;
}

void
atibdj4CleanOpusTags (atidata_t *atidata,
    const char *ffn, int tagtype, int filetype)
{
  OpusTags    tags;
  int         rc = -1;

  opus_tags_init (&tags);
  rc = atibdj4WriteOggFile (ffn, &tags);
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

  vallist = atiSplitVorbisComment (tagkey, tagname, val);
  slistStartIterator (vallist, &viteridx);
  while ((tval = slistIterateKey (vallist, &viteridx)) != NULL) {
    opus_tags_add (newtags, tagname, tval);
  }
  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: update: %s=%s", tagname, val);
  slistFree (vallist);
}

