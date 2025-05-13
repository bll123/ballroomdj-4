/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
#include "pathbld.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "podcast.h"

enum {
  PODCAST_VERSION = 1,
};

typedef struct podcast {
  datafile_t  *df;
  char        *name;
  char        *path;
  int         distvers;
  nlist_t     *podcast;
} podcast_t;

/* must be sorted in ascii order */
static datafilekey_t podcastdfkeys [PODCAST_KEY_MAX] = {
  { "LASTBLDDATE",    PODCAST_LAST_BLD_DATE,  VALUE_STR, NULL, DF_NORM },
  { "PASSWORD",       PODCAST_PASSWORD,       VALUE_STR, NULL, DF_NORM },
  { "RETAIN",         PODCAST_RETAIN,         VALUE_NUM, NULL, DF_NORM },
  { "TITLE",          PODCAST_TITLE,          VALUE_STR, NULL, DF_NORM },
  { "URI",            PODCAST_URI,            VALUE_STR, NULL, DF_NORM },
  { "USER",           PODCAST_USER,           VALUE_STR, NULL, DF_NORM },
};

podcast_t *
podcastLoad (const char *fname)
{
  podcast_t     *podcast;
  char          fn [MAXPATHLEN];

  pathbldMakePath (fn, sizeof (fn), fname, BDJ4_PODCAST_EXT, PATHBLD_MP_DREL_DATA);
  if (! fileopFileExists (fn)) {
    // logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: podcast: missing %s", fname);
    return NULL;
  }

  podcast = mdmalloc (sizeof (podcast_t));
  podcast->name = NULL;
  podcast->path = NULL;
  podcast->podcast = NULL;
  podcastSetName (podcast, fname);

  podcast->df = datafileAllocParse ("podcast", DFTYPE_KEY_VAL, fn,
      podcastdfkeys, PODCAST_KEY_MAX, DF_NO_OFFSET, NULL);
  podcast->podcast = datafileGetList (podcast->df);
  podcast->distvers = datafileDistVersion (podcast->df);

  return podcast;
}

podcast_t *
podcastCreate (const char *fname)
{
  podcast_t    *podcast;
  char          fn [MAXPATHLEN];


  pathbldMakePath (fn, sizeof (fn), fname, BDJ4_PODCAST_EXT, PATHBLD_MP_DREL_DATA);

  podcast = mdmalloc (sizeof (podcast_t));
  podcast->name = mdstrdup (fname);
  podcast->path = mdstrdup (fn);
  podcast->distvers = 1;
  podcast->df = datafileAlloc ("podcast", DFTYPE_KEY_VAL,
      podcast->path, podcastdfkeys, PODCAST_KEY_MAX);
  podcast->podcast = nlistAlloc ("podcast", LIST_UNORDERED, NULL);
  nlistSetSize (podcast->podcast, PODCAST_KEY_MAX);
  nlistSetNum (podcast->podcast, PODCAST_RETAIN, 0);
  nlistSetNum (podcast->podcast, PODCAST_LAST_BLD_DATE, 0);
  nlistSetStr (podcast->podcast, PODCAST_URI, "");
  nlistSetStr (podcast->podcast, PODCAST_TITLE, "");
  nlistSetStr (podcast->podcast, PODCAST_USER, "");
  nlistSetStr (podcast->podcast, PODCAST_PASSWORD, "");
  nlistSort (podcast->podcast);

  return podcast;
}


void
podcastFree (podcast_t *podcast)
{
  if (podcast == NULL) {
    return;
  }

  dataFree (podcast->path);
  dataFree (podcast->name);
  if (podcast->podcast != datafileGetList (podcast->df)) {
    nlistFree (podcast->podcast);
  }
  datafileFree (podcast->df);
  mdfree (podcast);
}

bool
podcastExists (const char *name)
{
  char  fn [MAXPATHLEN];

  pathbldMakePath (fn, sizeof (fn), name, BDJ4_PODCAST_EXT, PATHBLD_MP_DREL_DATA);
  return fileopFileExists (fn);
}

void
podcastSetName (podcast_t *podcast, const char *newname)
{
  char    fn [MAXPATHLEN];

  if (podcast == NULL || newname == NULL) {
    return;
  }

  dataFree (podcast->name);
  dataFree (podcast->path);
  podcast->name = mdstrdup (newname);
  pathbldMakePath (fn, sizeof (fn), newname,
      BDJ4_PODCAST_EXT, PATHBLD_MP_DREL_DATA);
  podcast->path = mdstrdup (fn);
}

void
podcastSave (podcast_t *podcast)
{
  char  tfn [MAXPATHLEN];

  if (podcast == NULL || podcast->podcast == NULL) {
    return;
  }

  if (nlistGetCount (podcast->podcast) <= 0) {
    return;
  }

  nlistSetVersion (podcast->podcast, PODCAST_VERSION);
  pathbldMakePath (tfn, sizeof (tfn), podcast->name,
      BDJ4_PODCAST_EXT, PATHBLD_MP_DREL_DATA);
  datafileSave (podcast->df, tfn, podcast->podcast, DF_NO_OFFSET,
      podcast->distvers);
}

void
podcastSetNum (podcast_t *podcast, int key, ssize_t value)
{
  if (podcast == NULL || podcast->podcast == NULL) {
    return;
  }
  if (key < 0 || key >= PODCAST_KEY_MAX) {
    return;
  }

  nlistSetNum (podcast->podcast, key, value);
}

void
podcastSetStr (podcast_t *podcast, int key, const char *str)
{
  if (podcast == NULL || podcast->podcast == NULL) {
    return;
  }
  if (key < 0 || key >= PODCAST_KEY_MAX) {
    return;
  }

  nlistSetStr (podcast->podcast, key, str);
}

ssize_t
podcastGetNum (podcast_t *podcast, int key)
{
  if (podcast == NULL || podcast->podcast == NULL) {
    return 0;
  }
  if (key < 0 || key >= PODCAST_KEY_MAX) {
    return 0;
  }

  return nlistGetNum (podcast->podcast, key);
}

const char *
podcastGetStr (podcast_t *podcast, int key)
{
  if (podcast == NULL || podcast->podcast == NULL) {
    return NULL;
  }
  if (key < 0 || key >= PODCAST_KEY_MAX) {
    return NULL;
  }

  return nlistGetStr (podcast->podcast, key);
}

