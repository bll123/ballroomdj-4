/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#define OV_EXCLUDE_STATIC_CALLBACKS 1
#include <vorbis/vorbisfile.h>
#undef OV_EXCLUDE_STATIC_CALLBACKS

#include "ati.h"
#include "atibdj4.h"
#include "atioggutil.h"
#include "audiofile.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "tagdef.h"

typedef struct atisaved {
  bool                  hasdata;
  int                   filetype;
  int                   tagtype;
  struct vorbis_comment *vc;
} atisaved_t;

static void atibdj4OggAddVorbisComment (struct vorbis_comment *newvc, int tagkey, const char *tagname, const char *val);

void
atibdj4ParseOggTags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  OggVorbis_File        ovf;
  int                   rc;
  struct vorbis_comment *vc;
  double                ddur;
  char                  tmp [40];

  rc = ov_fopen (ffn, &ovf);
  if (rc < 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "ov_fopen %d %s", rc, ffn);
    return;
  }

  vc = ovf.vc;
  if (vc == NULL) {
    return;
  }

  ddur = ov_time_total (&ovf, -1);
  snprintf (tmp, sizeof (tmp), "%.0f", ddur * 1000.0);
  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "duration: %s", tmp);
  slistSetStr (tagdata, atidata->tagName (TAG_DURATION), tmp);

  for (int i = 0; i < vc->comments; ++i) {
    const char  *kw;

    kw = vc->user_comments [i];
    atioggProcessVorbisCommentCombined (atidata->tagLookup, tagdata, tagtype, kw);
  }
  ov_clear (&ovf);
  return;
}

int
atibdj4WriteOggTags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  OggVorbis_File        ovf;
  int                   rc = -1;
  struct vorbis_comment *vc;
  struct vorbis_comment newvc;
  slistidx_t            iteridx;
  slist_t               *upddone;
  const char            *key;
  int                   writetags;
  int                   tagkey;
  const tagaudiotag_t   *audiotag = NULL;

  writetags = atidata->writetags;
  rc = ov_fopen (ffn, &ovf);
  if (rc < 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "ov_fopen %d %s", rc, ffn);
    return -1;
  }

  vc = ovf.vc;
  if (vc == NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "no vc %s", ffn);
    return -1;
  }

  vorbis_comment_init (&newvc);
  upddone = slistAlloc ("upd-done", LIST_ORDERED, NULL);

  for (int i = 0; i < vc->comments; ++i) {
    const char  *kw;
    const char  *val;
    char        ttag [300];     /* vorbis tag name */
    const char  *tagname;

    kw = vc->user_comments [i];
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
      atibdj4OggAddVorbisComment (&newvc, tagkey, audiotag->tag, val);
      slistSetNum (upddone, tagname, 1);
    } else {
      /* the tag has not changed, or is unknown to bdj4 */
      vorbis_comment_add (&newvc, kw);
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
    atibdj4OggAddVorbisComment (&newvc, tagkey, audiotag->tag, tval);
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: new: %s=%s", audiotag->tag, tval);
  }
  slistFree (upddone);

  ov_clear (&ovf);

  rc = atioggWriteOggFile (ffn, &newvc, filetype);

  vorbis_comment_clear (&newvc);

  return rc;
}

atisaved_t *
atibdj4SaveOggTags (atidata_t *atidata, const char *ffn,
    int tagtype, int filetype)
{
  atisaved_t            *atisaved;
  OggVorbis_File        ovf;
  int                   rc = -1;
  struct vorbis_comment *vc;
  struct vorbis_comment *newvc;

  rc = ov_fopen (ffn, &ovf);
  if (rc < 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "ov_fopen %d %s", rc, ffn);
    return NULL;
  }

  vc = ovf.vc;
  if (vc == NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "no vc %s", ffn);
    return NULL;
  }

  newvc = mdmalloc (sizeof (struct vorbis_comment));
  vorbis_comment_init (newvc);

  for (int i = 0; i < vc->comments; ++i) {
    const char  *kw;

    kw = vc->user_comments [i];
    vorbis_comment_add (newvc, kw);
  }

  ov_clear (&ovf);
  atisaved = mdmalloc (sizeof (atisaved_t));
  atisaved->hasdata = true;
  atisaved->tagtype = tagtype;
  atisaved->filetype = filetype;
  atisaved->vc = newvc;

  return atisaved;
}

void
atibdj4FreeSavedOggTags (atisaved_t *atisaved, int tagtype, int filetype)
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

  vorbis_comment_clear (atisaved->vc);
  atisaved->hasdata = false;
  mdfree (atisaved->vc);
  mdfree (atisaved);
}

int
atibdj4RestoreOggTags (atidata_t *atidata,
    atisaved_t *atisaved, const char *ffn, int tagtype, int filetype)
{
  int   rc = -1;

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

  rc = atioggWriteOggFile (ffn, atisaved->vc, filetype);

  return rc;
}

void
atibdj4CleanOggTags (atidata_t *atidata,
    const char *ffn, int tagtype, int filetype)
{
  struct vorbis_comment newvc;

  vorbis_comment_init (&newvc);
  atioggWriteOggFile (ffn, &newvc, filetype);
  vorbis_comment_clear (&newvc);

  return;
}

void
atibdj4LogOggVersion (void)
{
  logMsg (LOG_DBG, LOG_INFO, "libvorbis version %s", vorbis_version_string());
}

void
atibdj4OggAddVorbisComment (struct vorbis_comment *newvc, int tagkey,
    const char *tagname, const char *val)
{
  slist_t     *vallist;
  slistidx_t  viteridx;
  const char  *tval;

  vallist = atioggSplitVorbisComment (tagkey, tagname, val);
  slistStartIterator (vallist, &viteridx);
  while ((tval = slistIterateKey (vallist, &viteridx)) != NULL) {
    vorbis_comment_add_tag (newvc, tagname, tval);
  }
  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: update: %s=%s", tagname, val);
  slistFree (vallist);
}

