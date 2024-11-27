/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "bdjvarsdf.h"
#include "genre.h"
#include "grouping.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "slist.h"
#include "song.h"
#include "tagdef.h"

typedef struct grouping {
  nlist_t     *groupIndex;
  nlist_t     *groups;
  genre_t     *genres;
  int32_t     groupnum;
} grouping_t;

static void groupingAdd (grouping_t *grp, song_t *song, dbidx_t dbidx, slist_t *groupSort, nlist_t *groupName);

grouping_t *
groupingAlloc (musicdb_t *musicdb)
{
  grouping_t    *grp;
  slist_t       *groupSort;
  nlist_t       *groupName;
  slistidx_t    dbiter;
  slistidx_t    sortiter;
  dbidx_t       dbidx;
  song_t        *song;
  const char    *key;
  const char    *lastkey;
  nlist_t       *dbidxlist = NULL;

  grp = mdmalloc (sizeof (grouping_t));
  grp->groupIndex = nlistAlloc ("grpidx", LIST_UNORDERED, NULL);
  grp->groups = nlistAlloc ("groups", LIST_ORDERED, NULL);
  grp->genres = bdjvarsdfGet (BDJVDF_GENRES);
  grp->groupnum = 0;

  groupSort = slistAlloc ("grpsort", LIST_UNORDERED, NULL);
  groupName = nlistAlloc ("grpname", LIST_UNORDERED, NULL);
  dbStartIterator (musicdb, &dbiter);
  while ((song = dbIterate (musicdb, &dbidx, &dbiter)) != NULL) {
    groupingAdd (grp, song, dbidx, groupSort, groupName);
  }

  slistSort (groupSort);
  nlistSort (groupName);

  nlistSetSize (grp->groupIndex, slistGetCount (groupSort));

  slistStartIterator (groupSort, &sortiter);
  lastkey = NULL;
  while ((dbidx = slistIterateValueNum (groupSort, &sortiter)) >= 0) {
    key = nlistGetStr (groupName, dbidx);
    if (lastkey == NULL || strcmp (key, lastkey) != 0) {
      if (lastkey != NULL) {
        nlistSetList (grp->groups, grp->groupnum, dbidxlist);
      }
fprintf (stderr, "-- new group %s\n", key);
      ++grp->groupnum;
      dbidxlist = nlistAlloc ("grp-dbidxlist", LIST_UNORDERED, NULL);
      lastkey = key;
    }

fprintf (stderr, "   gnum:%d dbidx:%d %s\n", grp->groupnum, dbidx, key);
    nlistSetNum (grp->groupIndex, dbidx, grp->groupnum);
    nlistSetNum (dbidxlist, dbidx, grp->groupnum);
  }
  nlistSetList (grp->groups, grp->groupnum, dbidxlist);
  nlistSort (grp->groupIndex);

  slistFree (groupSort);
  nlistFree (groupName);

  return grp;
}

void
groupingFree (grouping_t *grp)
{
  if (grp == NULL) {
    return;
  }

  nlistFree (grp->groupIndex);
  nlistFree (grp->groups);

  mdfree (grp);
}

nlist_t *
groupingGet (grouping_t *grp, dbidx_t dbidx)
{
  int32_t   groupnum;
  nlist_t   *dbidxlist;

  if (grp == NULL || dbidx < 0) {
    return NULL;
  }

  groupnum = nlistGetNum (grp->groupIndex, dbidx);
  if (groupnum < 0) {
    return NULL;
  }
fprintf (stderr, "-- get: dbidx:%d gnum:%d\n", dbidx, groupnum);

  dbidxlist = nlistGetList (grp->groups, groupnum);
  return dbidxlist;
}

/* internal routines */

static void
groupingAdd (grouping_t *grp, song_t *song, dbidx_t dbidx, slist_t *groupSort, nlist_t *groupName)
{
  bool        hasgrp = false;
  bool        haswork = false;
  bool        hastitlework = false;
  bool        isclassical = false;
  const char  *tval = NULL;
  const char  *title = NULL;
  int         trknum = LIST_VALUE_INVALID;
  int         mvnum = LIST_VALUE_INVALID;
  int         usenum;
  nlistidx_t  genreidx;
  char        temp [200];
  char        sortkey [300];

  genreidx = songGetNum (song, TAG_GENRE);
  isclassical = genreGetClassicalFlag (grp->genres, genreidx);

  trknum = songGetNum (song, TAG_TRACKNUMBER);
  mvnum = songGetNum (song, TAG_MOVEMENTNUM);

  if (isclassical) {
    tval = songGetStr (song, TAG_WORK);
    if (tval != NULL && *tval) {
fprintf (stderr, "work: %s / %s\n", tval, songGetStr (song, TAG_TITLE));
      haswork = true;
    }

    if (! haswork) {
      *temp = '\0';
      songGetClassicalWork (song, temp, sizeof (temp));
      if (*temp) {
fprintf (stderr, "title-work: %s / %s\n", temp, songGetStr (song, TAG_TITLE));
        hastitlework = true;
        tval = temp;
      }
    }
  }

  if (! haswork && ! hastitlework) {
    tval = songGetStr (song, TAG_GROUPING);
    if (tval != NULL && *tval) {
fprintf (stderr, "group: %s / %s\n", tval, songGetStr (song, TAG_TITLE));
      hasgrp = true;
    }
    title = songGetStr (song, TAG_TITLE);
  }

  if (! hasgrp && ! haswork && ! hastitlework) {
    return;
  }

  usenum = mvnum;
  if (usenum < 0) {
    usenum = trknum;
  }
  if (usenum < 0) {
    usenum = 0;
  }

  if (title != NULL) {
    snprintf (sortkey, sizeof (sortkey), "%s/%03d/%s", tval, usenum, title);
  } else {
    snprintf (sortkey, sizeof (sortkey), "%s/%03d", tval, usenum);
  }

  slistSetNum (groupSort, sortkey, dbidx);
  nlistSetStr (groupName, dbidx, tval);
}
