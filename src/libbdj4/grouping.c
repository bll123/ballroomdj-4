/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
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
#include "tmutil.h"

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

  grp = mdmalloc (sizeof (grouping_t));
  grp->groupIndex = NULL;
  grp->groups = NULL;
  grp->genres = bdjvarsdfGet (BDJVDF_GENRES);
  grp->groupnum = 0;

  groupingRebuild (grp, musicdb);

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

void
groupingStartIterator (grouping_t *grp, nlistidx_t *iteridx)
{
  *iteridx = 0;
}

dbidx_t
groupingIterate (grouping_t *grp, dbidx_t seldbidx, nlistidx_t *iteridx)
{
  int32_t   groupnum;
  int       grpcount;
  nlist_t   *dbidxlist;
  dbidx_t   dbidx;

  if (grp == NULL || seldbidx < 0) {
    return -1;
  }

  groupnum = nlistGetNum (grp->groupIndex, seldbidx);
  if (groupnum < 0) {
    return -1;
  }

  dbidxlist = nlistGetList (grp->groups, groupnum);
  if (dbidxlist == NULL) {
    return -1;
  }

  grpcount = nlistGetCount (dbidxlist);
  if (*iteridx >= grpcount) {
    return -1;
  }

  /* dbidxlist is not sorted */
  dbidx = nlistGetKeyByIdx (dbidxlist, *iteridx);
  ++(*iteridx);

  return dbidx;
}

/* returns the count for the group or zero */
int
groupingCheck (grouping_t *grp, dbidx_t seldbidx, dbidx_t chkdbidx)
{
  int32_t     groupnum;
  nlist_t     *dbidxlist;
  nlistidx_t  iteridx;
  dbidx_t     tdbidx;
  int         rc = 0;

  if (grp == NULL || seldbidx < 0 || chkdbidx < 0) {
    return 0;
  }

  groupnum = nlistGetNum (grp->groupIndex, seldbidx);
  if (groupnum < 0) {
    return 0;
  }

  dbidxlist = nlistGetList (grp->groups, groupnum);
  if (dbidxlist == NULL) {
    return 0;
  }

  /* dbidxlist is not sorted */
  /* it is an unordered list ordered by the group-sort */
  nlistStartIterator (dbidxlist, &iteridx);
  while ((tdbidx = nlistIterateKey (dbidxlist, &iteridx)) >= 0) {
    if (tdbidx == chkdbidx) {
      rc = nlistGetCount (dbidxlist);
      break;
    }
  }

  return rc;
}

void
groupingRebuild (grouping_t *grp, musicdb_t *musicdb)
{
  slist_t       *groupSort;
  nlist_t       *groupName;
  slistidx_t    dbiter;
  slistidx_t    sortiter;
  dbidx_t       dbidx;
  song_t        *song;
  const char    *key;
  const char    *lastkey;
  nlist_t       *dbidxlist = NULL;
  mstime_t      ttm;

  mstimestart (&ttm);
  nlistFree (grp->groupIndex);
  nlistFree (grp->groups);
  grp->groupIndex = nlistAlloc ("grpidx", LIST_UNORDERED, NULL);
  grp->groups = nlistAlloc ("groups", LIST_ORDERED, NULL);
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
      ++grp->groupnum;
      dbidxlist = nlistAlloc ("grp-dbidxlist", LIST_UNORDERED, NULL);
      lastkey = key;
    }

    nlistSetNum (grp->groupIndex, dbidx, grp->groupnum);
    nlistSetNum (dbidxlist, dbidx, grp->groupnum);
  }
  nlistSetList (grp->groups, grp->groupnum, dbidxlist);
  nlistSort (grp->groupIndex);

  slistFree (groupSort);
  nlistFree (groupName);
  logMsg (LOG_DBG, LOG_INFO, "grouping: %" PRId64 "ms", mstimeend (&ttm));
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
  int         discnum = LIST_VALUE_INVALID;
  int         trknum = LIST_VALUE_INVALID;
  int         mvnum = LIST_VALUE_INVALID;
  int         usenum;
  nlistidx_t  genreidx;
  char        temp [200];
  char        sortkey [300];

  genreidx = songGetNum (song, TAG_GENRE);
  isclassical = genreGetClassicalFlag (grp->genres, genreidx);

  /* check the grouping tag in all cases.  it will override */
  /* a work or the title-work */
  tval = songGetStr (song, TAG_GROUPING);
  if (tval != NULL && *tval) {
    hasgrp = true;
  }

  if (! hasgrp && isclassical) {
    /* by preference, use any 'work' that is set */
    tval = songGetStr (song, TAG_WORK);
    if (tval != NULL && *tval) {
      haswork = true;
    }

    if (! haswork) {
      /* if 'work' is not set, pull the work from the song title */
      *temp = '\0';
      songGetClassicalWork (song, temp, sizeof (temp));
      if (*temp) {
        hastitlework = true;
        tval = temp;
      }
    }
  }

  if (! hasgrp && ! haswork && ! hastitlework) {
    return;
  }

  discnum = songGetNum (song, TAG_DISCNUMBER);
  trknum = songGetNum (song, TAG_TRACKNUMBER);
  mvnum = songGetNum (song, TAG_MOVEMENTNUM);

  /* sort by movement number */
  /* if the movement number is set, set the disc-number to zero */
  /* if movement number is not set, use the disc-number/track-number combo */
  usenum = mvnum;
  if (discnum < 0) {
    discnum = 0;
  }
  if (mvnum >= 0) {
    discnum = 0;
  }
  if (usenum < 0) {
    usenum = trknum;
  }
  if (usenum < 0) {
    usenum = 0;
  }

  title = songGetStr (song, TAG_TITLE);

  if (title != NULL) {
    snprintf (sortkey, sizeof (sortkey), "%s/%03d%03d/%s",
        tval, discnum, usenum, title);
  } else {
    snprintf (sortkey, sizeof (sortkey), "%s/%03d%03d",
        tval, discnum, usenum);
  }

  slistSetNum (groupSort, sortkey, dbidx);
  nlistSetStr (groupName, dbidx, tval);
}
