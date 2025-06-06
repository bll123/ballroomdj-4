/*
 * Copyright 2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <math.h>
#include <time.h>

#include "bdj4.h"
#include "ilist.h"
#include "musicdb.h"
#include "podcast.h"
#include "podcastutil.h"
#include "songlist.h"
#include "tagdef.h"

static void podcastutilApplyDelete (musicdb_t *musicdb, const char *plname, int retain, bool force);

pcretain_t
podcastutilCheckRetain (song_t *song, int retain)
{
  time_t      currtm;
  time_t      podtm;
  time_t      diff = 0;
  time_t      retdays = 0;
  pcretain_t  rc;

  if (retain <= 0 || song == NULL) {
    /* retain is not set or is set to keep all */
    return PODCAST_KEEP;
  }

  currtm = time (NULL);
  podtm = songGetNum (song, TAG_DBADDDATE);
  diff = currtm - podtm;
  retdays = (time_t) retain * 24 * 3600;
  rc = PODCAST_KEEP;
  if (diff > retdays) {
    rc = PODCAST_DELETE;
  }

  return rc;
}

void
podcastutilApplyRetain (musicdb_t *musicdb, const char *plname)
{
  int         retain;
  podcast_t   *podcast;

  podcast = podcastLoad (plname);
  if (podcast == NULL) {
    return;
  }

  retain = podcastGetNum (podcast, PODCAST_RETAIN);
  if (retain <= 0) {
    /* retain is not set or is set to keep all */
    return;
  }

  podcastutilApplyDelete (musicdb, plname, retain, false);
}

void
podcastutilDelete (musicdb_t *musicdb, const char *plname)
{
  /* set the force-flag to true */
  /* test will remove everything */
  podcastutilApplyDelete (musicdb, plname, 0, true);
}

static void
podcastutilApplyDelete (musicdb_t *musicdb, const char *plname,
    int retain, bool force)
{
  songlist_t  *sl;
  ilistidx_t  iteridx;
  ilistidx_t  slkey;
  time_t      currtime;

  sl = songlistLoad (plname);
  if (sl == NULL) {
    return;
  }

  currtime = time (NULL);

  songlistStartIterator (sl, &iteridx);
  while ((slkey = songlistIterate (sl, &iteridx)) >= 0) {
    const char  *sfname;
    song_t      *song;
    dbidx_t     dbidx;
    int         type;
    pcretain_t  pcrc = PODCAST_KEEP;

    sfname = songlistGetStr (sl, slkey, SONGLIST_URI);
    if (sfname == NULL) {
      continue;
    }

    song = dbGetByName (musicdb, sfname);
    if (song == NULL) {
      continue;
    }
    dbidx = songGetNum (song, TAG_DBIDX);
    if (dbidx < 0) {
      continue;
    }
    type = songGetNum (song, TAG_SONG_TYPE);
    if (type != SONG_TYPE_PODCAST) {
      continue;
    }
    if (! force) {
      pcrc = podcastutilCheckRetain (song, retain);
    }
    if (force || pcrc == PODCAST_DELETE) {
      dbRemoveSong (musicdb, dbidx);
    }
  }
}
