/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include "audiosrc.h"
#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
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

typedef struct asdata {
  const char    *musicdir;
  const char    *delpfx;
  const char    *origext;
  size_t        musicdirlen;
  slist_t       *songlist;
} asdata_t;

void
asiDesc (const char **ret, int max)
{
  int         c = 0;

  if (max < 2) {
    return;
  }

  ret [c++] = "file";
  ret [c++] = NULL;
}

asdata_t *
asiInit (const char *delpfx, const char *origext)
{
  asdata_t    *asdata;

  asdata = mdmalloc (sizeof (asdata_t));
  asdata->musicdir = "";
  asdata->musicdirlen = 0;
  asdata->delpfx = delpfx;
  asdata->origext = origext;
  return asdata;
}

void
asiPostInit (asdata_t *asdata, const char *uri)
{
  if (uri == NULL) {
    return;
  }
  asdata->musicdir = uri;
  asdata->musicdirlen = strlen (asdata->musicdir);
}

void
asiFree (asdata_t *asdata)
{
  if (asdata == NULL) {
    return;
  }

  mdfree (asdata);
}

int
asiTypeIdent (void)
{
  return AUDIOSRC_TYPE_FILE;
}

bool
asiIsTypeMatch (asdata_t *asdata, const char *nm)
{
  bool    rc = false;

  if (strncmp (nm, AS_FILE_PFX, AS_FILE_PFX_LEN) == 0) {
    rc = true;
  } else if (fileopIsAbsolutePath (nm)) {
    rc = true;
  }
  /* audiosrc.c handles relative path names by assuming that any uri */
  /* without a protocol identifier is a 'file' type */

  return rc;
}

bool
asiExists (asdata_t *asdata, const char *nm)
{
  bool    exists;
  char    ffn [BDJ4_PATH_MAX];

  asiFullPath (asdata, nm, ffn, sizeof (ffn), NULL, 0);
  exists = fileopFileExists (ffn);
  return exists;
}

bool
asiOriginalExists (asdata_t *asdata, const char *nm)
{
  bool    exists;
  char    ffn [BDJ4_PATH_MAX];
  char    origfn [BDJ4_PATH_MAX];

  asiFullPath (asdata, nm, ffn, sizeof (ffn), NULL, 0);
  snprintf (origfn, sizeof (origfn), "%s%s",
      ffn, asdata->origext);
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
asiRemove (asdata_t *asdata, const char *nm)
{
  int           rc = false;
  char          ffn [BDJ4_PATH_MAX];
  char          newnm [BDJ4_PATH_MAX];
  pathinfo_t    *pi;

  asiFullPath (asdata, nm, ffn, sizeof (ffn), NULL, 0);
  if (! fileopFileExists (ffn)) {
    return false;
  }

  pi = pathInfo (ffn);
  snprintf (newnm, sizeof (newnm), "%.*s/%s%.*s",
      (int) pi->dlen, pi->dirname, asdata->delpfx,
      (int) pi->flen, pi->filename);
  pathInfoFree (pi);

  /* never overwrite an existing file */
  if (fileopFileExists (newnm)) {
    return false;
  }

  rc = filemanipMove (ffn, newnm);
  return rc == 0 ? true : false;
}

/* asiPrep is called in a multi-threaded context */
bool
asiPrep (asdata_t *asdata, const char *sfname, char *tempnm, size_t sz)
{
  char      ffn [BDJ4_PATH_MAX];
  mstime_t  mstm;
  time_t    tm;
  int       rc = false;

  if (sfname == NULL || tempnm == NULL) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "WARN: prep: null-data");
    return false;
  }

  mstimestart (&mstm);
  asiFullPath (asdata, sfname, ffn, sizeof (ffn), NULL, 0);
  audiosrcutilMakeTempName (ffn, tempnm, sz);

  /* VLC still cannot handle internationalized names. */
  /* I wonder how they handle them internally. */
  /* Symlinks work on Linux/Mac OS. */
  fileopDelete (tempnm);
  filemanipLinkCopy (ffn, tempnm);
  if (! fileopFileExists (tempnm)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: file copy failed: %s", tempnm);
    return false;
  }

  rc = audiosrcutilPreCacheFile (tempnm);

  tm = mstimeend (&mstm);
  logMsg (LOG_DBG, LOG_BASIC, "asfile: prep-time (%" PRIu64 ") %s", (uint64_t) tm, sfname);

  return rc;
}

void
asiPrepClean (asdata_t *asdata, const char *tempnm)
{
  if (tempnm == NULL) {
    return;
  }

  if (fileopFileExists (tempnm)) {
    fileopDelete (tempnm);
  }
}

const char *
asiPrefix (asdata_t *asdata)
{
  return AS_FILE_PFX;
}

void
asiURI (asdata_t *asdata, const char *sfname, char *buff, size_t sz,
    const char *prefix, int pfxlen)
{
  *buff = '\0';

  if (sfname == NULL || buff == NULL) {
    return;
  }

  stpecpy (buff, buff + sz, AS_FILE_PFX);
  asiFullPath (asdata, sfname, buff + AS_FILE_PFX_LEN, sz - AS_FILE_PFX_LEN,
      prefix, pfxlen);
}

void
asiFullPath (asdata_t *asdata, const char *sfname, char *buff, size_t sz,
    const char *prefix, int pfxlen)
{
  *buff = '\0';

  if (sfname == NULL || buff == NULL) {
    return;
  }

  if (strncmp (sfname, AS_FILE_PFX, AS_FILE_PFX_LEN) == 0) {
    sfname += AS_FILE_PFX_LEN;
  }

  if (fileopIsAbsolutePath (sfname)) {
    stpecpy (buff, buff + sz, sfname);
  } else {
    /* in most cases, the sfname passed in is either relative to */
    /* the music-dir, or is a full path, and the prefix-length and old name */
    /* do not need to be used. but when a new path needs to be generated */
    /* from a relative filename, */
    /* the prefix length and old filename must be set */
    /* the prefix length includes the trailing slash */
    if (pfxlen > 0 && prefix != NULL) {
      snprintf (buff, sz, "%.*s%s", pfxlen, prefix, sfname);
    } else {
      snprintf (buff, sz, "%s/%s", asdata->musicdir, sfname);
    }
  }
}

const char *
asiRelativePath (asdata_t *asdata, const char *sfname, int pfxlen)
{
  const char  *p = sfname;

  if (sfname == NULL) {
    return NULL;
  }

  if (strncmp (p, AS_FILE_PFX, AS_FILE_PFX_LEN) == 0) {
    p += AS_FILE_PFX_LEN;
  }

  if (fileopIsAbsolutePath (p)) {
    if (pfxlen < 0) {
      pfxlen = 0;
    }
    if (pfxlen > 0) {
      p += pfxlen;
    } else {
      if (strncmp (p, asdata->musicdir, asdata->musicdirlen) == 0) {
        p += asdata->musicdirlen + 1;
      }
    }
  }

  return p;
}

size_t
asiDir (asdata_t *asdata, const char *sfname, char *buff, size_t sz, int pfxlen)
{
  size_t    rc = 0;

  *buff = '\0';

  if (sfname == NULL) {
    return rc;
  }

  if (pfxlen > 0) {
    snprintf (buff, sz, "%.*s", pfxlen, sfname);
    rc = pfxlen;
  } else {
    snprintf (buff, sz, "%s", asdata->musicdir);
    rc = asdata->musicdirlen;
  }

  return rc;
}

asiterdata_t *
asiStartIterator (asdata_t *asdata, asitertype_t asitertype,
    const char *dir, const char *nm, int askey)
{
  asiterdata_t  *asidata;

  if (dir == NULL) {
    return NULL;
  }
  if (asitertype != AS_ITER_DIR) {
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
asiCleanIterator (asdata_t *asdata, asiterdata_t *asidata)
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
asiIterCount (asdata_t *asdata, asiterdata_t *asidata)
{
  int32_t   c = 0;

  if (asidata == NULL) {
    return c;
  }

  c = slistGetCount (asidata->filelist);
  return c;
}

const char *
asiIterate (asdata_t *asdata, asiterdata_t *asidata)
{
  const char    *rval = NULL;

  if (asidata == NULL) {
    return NULL;
  }

  rval = slistIterateKey (asidata->filelist, &asidata->fliter);
  return rval;
}
