/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include <libmp4tag.h>

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
  libmp4tagpreserve_t   *preserved;
} atisaved_t;

void
atibdj4LogMP4Version (void)
{
  logMsg (LOG_DBG, LOG_INFO, "libmp4tag version %s", mp4tag_version ());
}

void
atibdj4ParseMP4Tags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  libmp4tag_t *libmp4tag;
  mp4tagpub_t mp4tagpub;
  int         mp4error;
  int64_t     duration;
  char        tmp [40];

  libmp4tag = mp4tag_open (ffn, &mp4error);
  if (libmp4tag == NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  open %d %s", mp4error, ffn);
    return;
  }
  mdextalloc (libmp4tag);
  if (mp4tag_parse (libmp4tag) != MP4TAG_OK) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  open %s %s", mp4tag_error_str (libmp4tag), ffn);
    mdextfree (libmp4tag);
    mp4tag_free (libmp4tag);
    return;
  }
  mp4tag_iterate_init (libmp4tag);

  while (mp4tag_iterate (libmp4tag, &mp4tagpub) == MP4TAG_OK) {
    const char    *tagname = NULL;
    const char    *p = NULL;
    char          pbuff [100];

    tagname = atidata->tagLookup (tagtype, mp4tagpub.tag);
    if (tagname == NULL) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw: unk %s=%s", mp4tagpub.tag, mp4tagpub.data);
      continue;
    }

    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw: %s %s=%s", tagname, mp4tagpub.tag, mp4tagpub.data);

    p = mp4tagpub.data;
    if (strcmp (tagname, atidata->tagName (TAG_DISCNUMBER)) == 0) {
      p = atiParsePair (tagdata, atidata->tagName (TAG_DISCTOTAL),
          p, pbuff, sizeof (pbuff));
    }
    if (strcmp (tagname, atidata->tagName (TAG_TRACKNUMBER)) == 0) {
      p = atiParsePair (tagdata, atidata->tagName (TAG_TRACKTOTAL),
          p, pbuff, sizeof (pbuff));
    }
    slistSetStr (tagdata, tagname, p);
  }

  duration = mp4tag_duration (libmp4tag);
  snprintf (tmp, sizeof (tmp), "%" PRId64, duration);
  slistSetStr (tagdata, atidata->tagName (TAG_DURATION), tmp);

  mdextfree (libmp4tag);
  mp4tag_free (libmp4tag);
  return;
}

int
atibdj4WriteMP4Tags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype, int32_t flags)
{
  libmp4tag_t     *libmp4tag;
  int             mp4error;
  slistidx_t      iteridx;
  const char      *key;
  const tagaudiotag_t *audiotag;
  int             rc = -1;
  int             writetags;
  int             tagkey;
  char            tbuff [100];
  bool            write = false;

  writetags = atidata->writetags;

  libmp4tag = mp4tag_open (ffn, &mp4error);
  if (libmp4tag == NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  open %d %s", mp4error, ffn);
    return rc;
  }
  mdextalloc (libmp4tag);
  if (mp4tag_parse (libmp4tag) != MP4TAG_OK) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  open %s %s", mp4tag_error_str (libmp4tag), ffn);
    mdextfree (libmp4tag);
    mp4tag_free (libmp4tag);
    return rc;
  }

  slistStartIterator (dellist, &iteridx);
  while ((key = slistIterateKey (dellist, &iteridx)) != NULL) {
    tagkey = atidata->tagCheck (writetags, tagtype, key, AF_REWRITE_NONE);
    audiotag = atidata->audioTagLookup (tagkey, tagtype);
    if (audiotag == NULL) {
      continue;
    }
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  delete: %s", audiotag->tag);
    if (mp4tag_delete_tag (libmp4tag, audiotag->tag) != MP4TAG_OK) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  unable to delete tag %s %s %s", key, audiotag->tag, mp4tag_error_str (libmp4tag));
    } else {
      write = true;
    }
  }

  slistStartIterator (updatelist, &iteridx);
  while ((key = slistIterateKey (updatelist, &iteridx)) != NULL) {
    const char    *val;

    tagkey = atidata->tagCheck (writetags, tagtype, key, AF_REWRITE_NONE);
    val = slistGetStr (updatelist, key);

    if (tagkey == TAG_TRACKTOTAL || tagkey == TAG_DISCTOTAL) {
      continue;
    }

    if (tagkey == TAG_TRACKNUMBER || tagkey == TAG_DISCNUMBER) {
      if (val != NULL && *val) {
        const char  *tot = NULL;

        if (tagkey == TAG_TRACKNUMBER) {
          tot = nlistGetStr (datalist, TAG_TRACKTOTAL);
        }
        if (tagkey == TAG_DISCNUMBER) {
          tot = nlistGetStr (datalist, TAG_DISCTOTAL);
        }
        if (tot != NULL && *tot) {
          snprintf (tbuff, sizeof (tbuff), "%s/%s", val, tot);
          val = tbuff;
        }
      }
    }

    if (val == NULL || ! *val) {
      continue;
    }

    audiotag = atidata->audioTagLookup (tagkey, tagtype);
    if (audiotag == NULL) {
      continue;
    }

    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: %s=%s", audiotag->tag, val);
    if (mp4tag_set_tag (libmp4tag, audiotag->tag, val, false) != MP4TAG_OK) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  unable to set tag %s %s %s", key, audiotag->tag, mp4tag_error_str (libmp4tag));
    } else {
      write = true;
    }
  }

  if ((flags & ATI_FLAGS_FORCE_WRITE) == ATI_FLAGS_FORCE_WRITE) {
    write = true;
  }

  rc = 0;
  if (write) {
    if (mp4tag_write_tags (libmp4tag) != MP4TAG_OK) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  unable to write tags %s", mp4tag_error_str (libmp4tag));
      rc = -1;
    }
  }
  mdextfree (libmp4tag);
  mp4tag_free (libmp4tag);

  return rc;
}

atisaved_t *
atibdj4SaveMP4Tags (atidata_t *atidata,
    const char *ffn, int tagtype, int filetype)
{
  libmp4tag_t         *libmp4tag;
  libmp4tagpreserve_t *preserved = NULL;
  atisaved_t          *atisaved = NULL;
  int                 mp4error;

  libmp4tag = mp4tag_open (ffn, &mp4error);
  if (libmp4tag == NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  open %d %s", mp4error, ffn);
    return NULL;
  }
  mdextalloc (libmp4tag);
  if (mp4tag_parse (libmp4tag) != MP4TAG_OK) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  open %s %s", mp4tag_error_str (libmp4tag), ffn);
    mdextfree (libmp4tag);
    mp4tag_free (libmp4tag);
    return NULL;
  }

  preserved = mp4tag_preserve_tags (libmp4tag);
  if (preserved != NULL) {
    mdextalloc (preserved);
    atisaved = malloc (sizeof (atisaved_t));
    atisaved->hasdata = true;
    atisaved->tagtype = tagtype;
    atisaved->filetype = filetype;
    atisaved->preserved = preserved;
  }

  mdextfree (libmp4tag);
  mp4tag_free (libmp4tag);

  return atisaved;
}

void
atibdj4FreeSavedMP4Tags (atisaved_t *atisaved, int tagtype, int filetype)
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

  mdextfree (atisaved->preserved);
  mp4tag_preserve_free (atisaved->preserved);
  atisaved->hasdata = false;
  mdfree (atisaved);
}

int
atibdj4RestoreMP4Tags (atidata_t *atidata,
    atisaved_t *atisaved, const char *ffn, int tagtype, int filetype)
{
  libmp4tag_t         *libmp4tag;
  int                 mp4error;
  int                 rc = -1;

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

  libmp4tag = mp4tag_open (ffn, &mp4error);
  if (libmp4tag == NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  open %d %s", mp4error, ffn);
    return -1;
  }
  mdextalloc (libmp4tag);
  if (mp4tag_parse (libmp4tag) != MP4TAG_OK) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  open %s %s", mp4tag_error_str (libmp4tag), ffn);
    mdextfree (libmp4tag);
    mp4tag_free (libmp4tag);
    return -1;
  }

  if (mp4tag_restore_tags (libmp4tag, atisaved->preserved) == MP4TAG_OK) {
    rc = 0;
    if (mp4tag_write_tags (libmp4tag) != MP4TAG_OK) {
      rc = -1;
    }
  }
  mdextfree (libmp4tag);
  mp4tag_free (libmp4tag);

  return rc;
}

void
atibdj4CleanMP4Tags (atidata_t *atidata,
    const char *ffn, int tagtype, int filetype)
{
  libmp4tag_t   *libmp4tag;
  int           mp4error;

  libmp4tag = mp4tag_open (ffn, &mp4error);
  if (libmp4tag == NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  open %d %s", mp4error, ffn);
    return;
  }
  mdextalloc (libmp4tag);
  if (mp4tag_parse (libmp4tag) != MP4TAG_OK) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  open %s %s", mp4tag_error_str (libmp4tag), ffn);
    mdextfree (libmp4tag);
    mp4tag_free (libmp4tag);
    return;
  }

  if (mp4tag_clean_tags (libmp4tag) == MP4TAG_OK) {
    if (mp4tag_write_tags (libmp4tag) != MP4TAG_OK) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  unable to write tags %s", mp4tag_error_str (libmp4tag));
    }
  }
  mdextfree (libmp4tag);
  mp4tag_free (libmp4tag);

  return;
}
