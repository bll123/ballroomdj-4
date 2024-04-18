/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>
#include <errno.h>

#include "audiosrc.h"
#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "dirlist.h"
#include "fileop.h"
#include "filemanip.h"
#include "log.h"
#include "mdebug.h"
#include "pathinfo.h"
#include "slist.h"
#include "sysvars.h"
#include "tmutil.h"

typedef struct asiterdata {
  const char      *dir;
  slist_t         *filelist;
  slistidx_t      fliter;
} asiterdata_t;

static void audiosrcfileMakeTempName (const char *ffn, char *tempnm, size_t maxlen);

static long globalcount = 0;

bool
audiosrcfileExists (const char *nm)
{
  bool    exists;
  char    ffn [MAXPATHLEN];

  audiosrcfileFullPath (nm, ffn, sizeof (ffn), 0, NULL);
  exists = fileopFileExists (ffn);
  return exists;
}

bool
audiosrcfileOriginalExists (const char *nm)
{
  bool    exists;
  char    ffn [MAXPATHLEN];
  char    origfn [MAXPATHLEN];

  audiosrcfileFullPath (nm, ffn, sizeof (ffn), 0, NULL);
  snprintf (origfn, sizeof (origfn), "%s%s",
      ffn, bdjvarsGetStr (BDJV_ORIGINAL_EXT));
  exists = fileopFileExists (origfn);
  if (! exists) {
    snprintf (origfn, sizeof (origfn), "%s%s", ffn, BDJ4_GENERIC_ORIG_EXT);
    if (fileopFileExists (origfn)) {
      exists = true;
    }
  }
  return exists;
}

/* does not actually remove the file, renames it with a 'delete-' prefix */
bool
audiosrcfileRemove (const char *nm)
{
  int           rc = false;
  char          ffn [MAXPATHLEN];
  char          newnm [MAXPATHLEN];
  pathinfo_t    *pi;

  audiosrcfileFullPath (nm, ffn, sizeof (ffn), 0, NULL);
  if (! fileopFileExists (ffn)) {
    return false;
  }

  pi = pathInfo (ffn);
  snprintf (newnm, sizeof (newnm), "%.*s/%s%.*s",
      (int) pi->dlen, pi->dirname,
      bdjvarsGetStr (BDJV_DELETE_PFX),
      (int) pi->flen, pi->filename);
  pathInfoFree (pi);

  /* never overwrite an existing file */
  if (fileopFileExists (newnm)) {
    return false;
  }

  rc = filemanipMove (ffn, newnm);
  return rc == 0 ? true : false;
}

bool
audiosrcfilePrep (const char *sfname, char *tempnm, size_t sz)
{
  char      ffn [MAXPATHLEN];
  mstime_t  mstm;
  char      *buff;
  size_t    frc;
  ssize_t   fsz;
  time_t    tm;
  FILE      *fh;

  if (sfname == NULL || tempnm == NULL) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "WARN: prep: null-data");
    return false;
  }

  mstimestart (&mstm);
  audiosrcfileFullPath (sfname, ffn, sizeof (ffn), 0, NULL);
  audiosrcfileMakeTempName (ffn, tempnm, sz);

  /* VLC still cannot handle internationalized names. */
  /* I wonder how they handle them internally. */
  /* Symlinks work on Linux/Mac OS. */
  fileopDelete (tempnm);
  filemanipLinkCopy (ffn, tempnm);
  if (! fileopFileExists (tempnm)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: file copy failed: %s", tempnm);
    return false;
  }

  /* read the entire file in order to get it into the operating system's */
  /* filesystem cache */
  fsz = fileopSize (ffn);
  if (fsz <= 0) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: file size 0: %s", ffn);
    return false;
  }
  buff = mdmalloc (fsz);
  fh = fileopOpen (ffn, "rb");
  frc = fread (buff, fsz, 1, fh);
  mdextfclose (fh);
  fclose (fh);
  mdfree (buff);
  if (frc != 1) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: file read failed: %s", tempnm);
    return false;
  }

  tm = mstimeend (&mstm);
  logMsg (LOG_DBG, LOG_BASIC, "prep-time (%" PRIu64 ") %s", (uint64_t) tm, sfname);

  return true;
}

void
audiosrcfilePrepClean (const char *tempnm)
{
  if (tempnm == NULL) {
    return;
  }

  if (fileopFileExists (tempnm)) {
    fileopDelete (tempnm);
  }
}

void
audiosrcfileFullPath (const char *sfname, char *buff, size_t sz,
    int pfxlen, const char *oldfn)
{
  *buff = '\0';

  if (sfname == NULL) {
    return;
  }

  if (fileopIsAbsolutePath (sfname)) {
    strlcpy (buff, sfname, sz);
  } else {
    /* in most cases, the sfname passed in is either relative to */
    /* the music-dir, or is a full path, and the prefix-length and old name */
    /* do not need to be used. but when a new path needs to be generated */
    /* from a relative filename, */
    /* the prefix length and old filename must be set */
    /* the prefix length includes the trailing slash */
    if (pfxlen > 0 && oldfn != NULL) {
      snprintf (buff, sz, "%.*s%s", pfxlen, oldfn, sfname);
    } else {
      snprintf (buff, sz, "%s/%s", bdjoptGetStr (OPT_M_DIR_MUSIC), sfname);
    }
  }
}

const char *
audiosrcfileRelativePath (const char *sfname, int pfxlen)
{
  const char  *musicdir;
  size_t      musicdirlen;
  const char  *p = sfname;

  if (fileopIsAbsolutePath (sfname)) {
    if (pfxlen < 0) {
      pfxlen = 0;
    }
    if (pfxlen > 0) {
      p += pfxlen;
    } else {
      musicdir = bdjoptGetStr (OPT_M_DIR_MUSIC);
      musicdirlen = strlen (musicdir);
      if (strncmp (sfname, musicdir, musicdirlen) == 0) {
        p += musicdirlen + 1;
      }
    }
  }

  return p;
}

asiterdata_t *
audiosrcfileStartIterator (const char *dir)
{
  asiterdata_t  *asidata;

  if (dir == NULL) {
    return NULL;
  }

  if (! fileopIsDirectory (dir)) {
    return NULL;
  }

  asidata = mdmalloc (sizeof (asiterdata_t));
  asidata->dir = dir;

  asidata->filelist = dirlistRecursiveDirList (dir, DIRLIST_FILES);
  slistStartIterator (asidata->filelist, &asidata->fliter);

  return asidata;
}

void
audiosrcfileCleanIterator (asiterdata_t *asidata)
{
  if (asidata == NULL) {
    return;
  }

  if (asidata->filelist != NULL) {
    slistFree (asidata->filelist);
  }
  mdfree (asidata);
}

int32_t
audiosrcfileIterCount (asiterdata_t *asidata)
{
  int32_t   c = 0;

  if (asidata == NULL) {
    return c;
  }

  c = slistGetCount (asidata->filelist);
  return c;
}

const char *
audiosrcfileIterate (asiterdata_t *asidata)
{
  const char    *rval = NULL;

  if (asidata == NULL) {
    return NULL;
  }

  rval = slistIterateKey (asidata->filelist, &asidata->fliter);
  return rval;
}

/* internal routines */

static void
audiosrcfileMakeTempName (const char *ffn, char *tempnm, size_t maxlen)
{
  char        tnm [MAXPATHLEN];
  size_t      idx;
  pathinfo_t  *pi;

  pi = pathInfo (ffn);

  idx = 0;
  for (const char *p = pi->filename; *p && idx < maxlen && idx < pi->flen; ++p) {
    if ((isascii (*p) && isalnum (*p)) ||
        *p == '.' || *p == '-' || *p == '_') {
      tnm [idx++] = *p;
    }
  }
  tnm [idx] = '\0';
  pathInfoFree (pi);

  /* the profile index so we don't stomp on other bdj instances   */
  /* the global count so we don't stomp on ourselves              */
  snprintf (tempnm, maxlen, "tmp/%02" PRId64 "-%03ld-%s",
      sysvarsGetNum (SVL_PROFILE_IDX), globalcount, tnm);
  ++globalcount;
}
