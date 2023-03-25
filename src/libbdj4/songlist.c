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
  { "DANCE",    SONGLIST_DANCE,     VALUE_NUM, danceConvDance, SONGLIST_DANCESTR },
  { "DANCESTR", SONGLIST_DANCESTR,  VALUE_STR, NULL, DATAFILE_NO_WRITE },
  { "FILE",     SONGLIST_FILE,      VALUE_STR, NULL, DATAFILE_NO_BACKUPKEY },
  { "TITLE",    SONGLIST_TITLE,     VALUE_STR, NULL, DATAFILE_NO_BACKUPKEY },
};

songlist_t *
songlistAlloc (const char *fname)
{
  songlist_t    *sl;
  char          tfn [MAXPATHLEN];

  sl = mdmalloc (sizeof (songlist_t));
  sl->df = NULL;
  sl->songlist = NULL;
  sl->fname = mdstrdup (fname);
  pathbldMakePath (tfn, sizeof (tfn), fname,
      BDJ4_SONGLIST_EXT, PATHBLD_MP_DREL_DATA);
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
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: songlist: missing %s", sl->path);
    songlistFree (sl);
    return NULL;
  }
  sl->df = datafileAllocParse ("songlist", DFTYPE_INDIRECT, sl->path,
      songlistdfkeys, SONGLIST_KEY_MAX);
  if (sl->df == NULL) {
    songlistFree (sl);
    return NULL;
  }
  ilistFree (sl->songlist);
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
songlistSave (songlist_t *sl, int tmflag)
{
  time_t    origtm = 0;

  if (sl == NULL) {
    return;
  }

  origtm = fileopModTime (sl->path);
  ilistSetVersion (sl->songlist, SONGLIST_VERSION);
  datafileSaveIndirect ("songlist", sl->path, songlistdfkeys,
      SONGLIST_KEY_MAX, sl->songlist);
  if (tmflag == SONGLIST_PRESERVE_TIMESTAMP) {
    fileopSetModTime (sl->path, origtm);
  }
}
