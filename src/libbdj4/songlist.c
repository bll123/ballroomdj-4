/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "dance.h"
#include "datafile.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "pathbld.h"
#include "pathutil.h"
#include "songlist.h"

enum {
  SONGLIST_VERSION = 1,
};

typedef struct songlist {
  datafile_t      *df;
  ilist_t         *songlist;
  ilistidx_t      sliteridx;
  char            *fname;
  char            *path;
} songlist_t;

/* must be sorted in ascii order */
static datafilekey_t songlistdfkeys [SONGLIST_KEY_MAX] = {
  { "DANCE",    SONGLIST_DANCE,     VALUE_NUM, danceConvDance, DF_NORM },
  { "FILE",     SONGLIST_FILE,      VALUE_STR, NULL, DF_NORM },
  { "TITLE",    SONGLIST_TITLE,     VALUE_STR, NULL, DF_NORM },
};

songlist_t *
songlistAlloc (const char *fname)
{
  songlist_t    *sl;
  char          tfn [MAXPATHLEN];
  pathinfo_t    *pi;

  sl = mdmalloc (sizeof (songlist_t));
  sl->songlist = NULL;
  sl->df = NULL;
  if (fileopIsAbsolutePath (fname)) {
    pi = pathInfo (fname);
    snprintf (tfn, sizeof (tfn), "%.*s", (int) pi->blen, pi->basename);
    sl->fname = mdstrdup (tfn);
    pathInfoFree (pi);
    strlcpy (tfn, fname, sizeof (tfn));
  } else {
    sl->fname = mdstrdup (fname);
    pathbldMakePath (tfn, sizeof (tfn), fname,
        BDJ4_SONGLIST_EXT, PATHBLD_MP_DREL_DATA);
  }
  sl->path = mdstrdup (tfn);
  sl->songlist = ilistAlloc (fname, LIST_ORDERED);
  ilistSetVersion (sl->songlist, SONGLIST_VERSION);
  return sl;
}

songlist_t *
songlistLoad (const char *fname)
{
  songlist_t    *sl;

  sl = songlistAlloc (fname);

  if (! fileopFileExists (sl->path)) {
    // logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: songlist: missing %s", sl->path);
    songlistFree (sl);
    return NULL;
  }
  sl->df = datafileAllocParse ("songlist", DFTYPE_INDIRECT, sl->path,
      songlistdfkeys, SONGLIST_KEY_MAX, DF_NO_OFFSET, NULL);
  if (sl->df == NULL) {
    songlistFree (sl);
    return NULL;
  }
  if (sl->songlist != datafileGetList (sl->df)) {
    ilistFree (sl->songlist);
    sl->songlist = NULL;
  }
  sl->songlist = datafileGetList (sl->df);
  ilistDumpInfo (sl->songlist);
  return sl;
}

void
songlistFree (songlist_t *sl)
{
  if (sl != NULL) {
    if (sl->df == NULL ||
        sl->songlist != datafileGetList (sl->df)) {
      ilistFree (sl->songlist);
    }
    datafileFree (sl->df);
    dataFree (sl->fname);
    dataFree (sl->path);
    mdfree (sl);
  }
}

bool
songlistExists (const char *name)
{
  char    tfn [MAXPATHLEN];
  pathbldMakePath (tfn, sizeof (tfn), name,
      BDJ4_SONGLIST_EXT, PATHBLD_MP_DREL_DATA);
  return fileopFileExists (tfn);
}

int
songlistGetCount (songlist_t *sl)
{
  if (sl == NULL || sl->songlist == NULL) {
    return 0;
  }
  return ilistGetCount (sl->songlist);
}

void
songlistStartIterator (songlist_t *sl, ilistidx_t *iteridx)
{
  if (sl == NULL || sl->songlist == NULL) {
    return;
  }
  ilistStartIterator (sl->songlist, iteridx);
}

ilistidx_t
songlistIterate (songlist_t *sl, ilistidx_t *iteridx)
{
  ilistidx_t    key = LIST_VALUE_INVALID;

  if (sl == NULL || sl->songlist == NULL) {
    return key;
  }

  key = ilistIterateKey (sl->songlist, iteridx);
  return key;
}

ilistidx_t
songlistGetNum (songlist_t *sl, ilistidx_t ikey, ilistidx_t lidx)
{
  ilistidx_t    val;

  if (sl == NULL || sl->songlist == NULL) {
    return LIST_VALUE_INVALID;
  }
  val = ilistGetNum (sl->songlist, ikey, lidx);
  return val;
}

char *
songlistGetStr (songlist_t *sl, ilistidx_t ikey, ilistidx_t lidx)
{
  char    *val = NULL;

  if (sl == NULL || sl->songlist == NULL) {
    return NULL;
  }
  val = ilistGetStr (sl->songlist, ikey, lidx);
  return val;
}

void
songlistSetNum (songlist_t *sl, ilistidx_t ikey, ilistidx_t lidx, ilistidx_t val)
{
  if (sl == NULL || sl->songlist == NULL) {
    return;
  }
  ilistSetNum (sl->songlist, ikey, lidx, val);
}

void
songlistSetStr (songlist_t *sl, ilistidx_t ikey, ilistidx_t lidx, const char *sval)
{
  if (sl == NULL || sl->songlist == NULL) {
    return;
  }
  ilistSetStr (sl->songlist, ikey, lidx, sval);
}

void
songlistClear (songlist_t *sl)
{
  if (sl == NULL) {
    return;
  }

  if (sl->df == NULL ||
        sl->songlist != datafileGetList (sl->df)) {
    ilistFree (sl->songlist);
    sl->songlist = NULL;
  }
  sl->songlist = ilistAlloc (sl->fname, LIST_ORDERED);
  ilistSetVersion (sl->songlist, SONGLIST_VERSION);
}

void
songlistSave (songlist_t *sl, int tmflag, int distvers)
{
  time_t    origtm = 0;

  if (sl == NULL) {
    return;
  }

  if (sl->df == NULL) {
    /* new songlist */
    sl->df = datafileAlloc ("songlist", DFTYPE_INDIRECT, sl->path,
        songlistdfkeys, SONGLIST_KEY_MAX);
  }

  origtm = fileopModTime (sl->path);
  ilistSetVersion (sl->songlist, SONGLIST_VERSION);
  if (distvers == SONGLIST_USE_DIST_VERSION) {
    distvers = datafileDistVersion (sl->df);
  }
  datafileSave (sl->df, sl->path, sl->songlist, DF_NO_OFFSET, distvers);
  if (tmflag == SONGLIST_PRESERVE_TIMESTAMP) {
    fileopSetModTime (sl->path, origtm);
  }
}

int
songlistDistVersion (songlist_t *sl)
{
  if (sl == NULL) {
    return 1;
  }

  return datafileDistVersion (sl->df);
}
