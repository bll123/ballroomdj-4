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
#include "podcast.h"
#include "slist.h"

enum {
  PODCAST_VERSION = 1,
};

typedef struct podcast {
  datafile_t  *df;
  char        *name;
  char        *path;
  char        *uri;
  char        *user;
  char        *password;
  int32_t     retain;
  int         distvers;
} podcast_t;

podcast_t *
podcastLoad (const char *fname)
{
  podcast_t     *podcast;
  slist_t       *tlist;
  char          fn [MAXPATHLEN];


  pathbldMakePath (fn, sizeof (fn), fname, BDJ4_PODCAST_EXT, PATHBLD_MP_DREL_DATA);
  if (! fileopFileExists (fn)) {
    // logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: podcast: missing %s", fname);
    return false;
  }

  podcast = mdmalloc (sizeof (podcast_t));
  podcast->name = mdstrdup (fname);
  podcast->path = mdstrdup (fn);

  podcast->df = datafileAllocParse ("podcast", DFTYPE_LIST, fn, NULL, 0, DF_NO_OFFSET, NULL);
  tlist = datafileGetList (podcast->df);
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
  podcast->uri = NULL;
  podcast->user = NULL;
  podcast->password = NULL;
  podcast->distvers = 1;
  podcast->df = datafileAlloc ("podcast", DFTYPE_LIST,
      podcast->path, NULL, 0);

  return podcast;
}


void
podcastFree (podcast_t *podcast)
{
  if (podcast != NULL) {
    dataFree (podcast->path);
    dataFree (podcast->name);
    dataFree (podcast->uri);
    dataFree (podcast->user);
    dataFree (podcast->password);
    datafileFree (podcast->df);
    mdfree (podcast);
  }
}

bool
podcastExists (const char *name)
{
  char  fn [MAXPATHLEN];

  pathbldMakePath (fn, sizeof (fn), name, BDJ4_PODCAST_EXT, PATHBLD_MP_DREL_DATA);
  return fileopFileExists (fn);
}

void
podcastSave (podcast_t *podcast, slist_t *slist)
{
  if (slistGetCount (slist) <= 0) {
    return;
  }

  slistSetVersion (slist, PODCAST_VERSION);
  datafileSave (podcast->df, NULL, slist, DF_NO_OFFSET, podcast->distvers);
}
