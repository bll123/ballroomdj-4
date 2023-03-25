/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "colorutils.h"
#include "mdebug.h"
#include "musicdb.h"
#include "nlist.h"
#include "samesong.h"
#include "slist.h"
#include "song.h"
#include "tagdef.h"

typedef struct samesong {
  musicdb_t *musicdb;
  nlist_t   *sscolors;
  nlist_t   *sscounts;
  ssize_t   nextssidx;
} samesong_t;

static ssize_t  samesongGetSSIdx (samesong_t *ss, dbidx_t dbidx);
static void     samesongCleanSingletons (samesong_t *ss);

samesong_t *
samesongAlloc (musicdb_t *musicdb)
{
  samesong_t  *ss;
  slistidx_t  dbiteridx;
  dbidx_t     dbidx;
  song_t      *song;
  ssize_t     ssidx;
  char        *sscolor;
  char        tbuff [80];
  int         val;


  ss = mdmalloc (sizeof (samesong_t));

  ss->nextssidx = 1;
  ss->musicdb = musicdb;
  ss->sscolors = nlistAlloc ("samesong-colors", LIST_ORDERED, NULL);
  ss->sscounts = nlistAlloc ("samesong-count", LIST_ORDERED, NULL);

  dbStartIterator (musicdb, &dbiteridx);
  while ((song = dbIterate (musicdb, &dbidx, &dbiteridx)) != NULL) {
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (ssidx > 0) {
      sscolor = nlistGetStr (ss->sscolors, ssidx);
      if (sscolor == NULL) {
        createRandomColor (tbuff, sizeof (tbuff));
        nlistSetStr (ss->sscolors, ssidx, tbuff);
        val = 1;
      } else {
        val = nlistGetNum (ss->sscounts, ssidx);
        ++val;
      }
      nlistSetNum (ss->sscounts, ssidx, val);
      if (ssidx >= ss->nextssidx) {
        ss->nextssidx = ssidx + 1;
      }
    }
  }

  /* clean up singletons for display purposes */
  /* the real clean-up of the database will go elsewhere */
  samesongCleanSingletons (ss);

  return ss;
}

void
samesongFree (samesong_t *ss)
{
  if (ss != NULL) {
    nlistFree (ss->sscolors);
    ss->sscolors = NULL;
    nlistFree (ss->sscounts);
    ss->sscounts = NULL;
    mdfree (ss);
  }
}

const char *
samesongGetColorByDBIdx (samesong_t *ss, dbidx_t dbidx)
{
  const char  *sscolor = NULL;
  ssize_t     ssidx;

  ssidx = samesongGetSSIdx (ss, dbidx);
  sscolor = samesongGetColorBySSIdx (ss, ssidx);
  return sscolor;
}

const char *
samesongGetColorBySSIdx  (samesong_t *ss, ssize_t ssidx)
{
  const char  *sscolor = NULL;

  if (ss == NULL || ss->sscolors == NULL) {
    return sscolor;
  }
  if (ssidx > 0) {
    sscolor = nlistGetStr (ss->sscolors, ssidx);
  }
  return sscolor;
}

void
samesongSet (samesong_t *ss, nlist_t *dbidxlist)
{
  dbidx_t     dbidx;
  nlistidx_t  iteridx;
  const char  *sscolor = NULL;
  char        tbuff [80];
  int         count = 0;
  bool        hasunset = false;
  ssize_t     lastssidx;
  ssize_t     usessidx = -1;
  ssize_t     ssidx;

  /* this is more complicated than it might seem */
  /* using the following process makes the same-song */
  /* marks more user-friendly */
  /* a) if there is a single same-song mark already set in the target list */
  /*   a-1) there are no unset marks in the target list */
  /*      change all of the targets to be a new mark */
  /*      set a new color */
  /*   a-2) there is an unset mark in the target list */
  /*      change all of the targets to be that same mark */
  /*      re-use the color of this mark */
  /*      the assumption here is that the unset mark is being added to */
  /*      the selected mark */
  /* b) if there is more than one same-song mark set in the target list */
  /*    remove all the marks set in the target list */
  /*    set a new color */

  /* count how many different same-song marks there are */
  /* only need to know 0, 1, or many */
  /* also check and see if there are any unset marks in the target list */
  lastssidx = -1;
  count = 0;
  nlistStartIterator (dbidxlist, &iteridx);
  while ((dbidx = nlistIterateKey (dbidxlist, &iteridx)) >= 0) {
    ssidx = samesongGetSSIdx (ss, dbidx);
    if (ssidx > 0) {
      if (lastssidx == -1) {
        ++count;
        lastssidx = ssidx;
      } else {
        if (lastssidx != ssidx) {
          ++count;
        }
      }
    } else {
      hasunset = true;
    }
  }

  /* more than one different mark */
  /* or a single mark with no unset marks */
  /* adjust the current counts for the entire target list */
  if (count > 1 ||
     (count == 1 && ! hasunset)) {
    int   val;

    nlistStartIterator (dbidxlist, &iteridx);
    while ((dbidx = nlistIterateKey (dbidxlist, &iteridx)) >= 0) {
      ssidx = samesongGetSSIdx (ss, dbidx);
      if (ssidx > 0) {
        val = nlistGetNum (ss->sscounts, ssidx);
        --val;
        nlistSetNum (ss->sscounts, ssidx, val);
      }
    }
  }

  *tbuff = '\0';

  /* more than one different mark or zero marks */
  /* or one mark and no unset */
  if (count > 1 ||
     (count == 1 && ! hasunset) ||
      count == 0) {
    createRandomColor (tbuff, sizeof (tbuff));
    sscolor = tbuff;
    usessidx = ss->nextssidx;
    nlistSetStr (ss->sscolors, usessidx, sscolor);
    ++ss->nextssidx;
  }
  /* only one mark found in the target list */
  /* and there are unset marks */
  if (count == 1 && hasunset) {
    sscolor = nlistGetStr (ss->sscolors, lastssidx);
    usessidx = lastssidx;
  }

  nlistStartIterator (dbidxlist, &iteridx);
  while ((dbidx = nlistIterateKey (dbidxlist, &iteridx)) >= 0) {
    int     val;
    ssize_t oldssidx;
    song_t  *song;

    song = dbGetByIdx (ss->musicdb, dbidx);
    if (song == NULL) {
      continue;
    }

    oldssidx = songGetNum (song, TAG_SAMESONG);
    songSetNum (song, TAG_SAMESONG, usessidx);
    val = nlistGetNum (ss->sscounts, usessidx);
    if (val < 0) {
      val = 1;
    } else {
      /* if not already set or changed */
      if (oldssidx != usessidx) {
        ++val;
      }
    }
    nlistSetNum (ss->sscounts, usessidx, val);
  }

  samesongCleanSingletons (ss);
}

void
samesongClear (samesong_t *ss, nlist_t *dbidxlist)
{
  int           val;
  dbidx_t       dbidx;
  nlistidx_t    iteridx;
  ssize_t       ssidx;
  song_t        *song;

  nlistStartIterator (dbidxlist, &iteridx);
  while ((dbidx = nlistIterateKey (dbidxlist, &iteridx)) >= 0) {
    song = dbGetByIdx (ss->musicdb, dbidx);
    if (song == NULL) {
      continue;
    }

    songSetNum (song, TAG_SAMESONG, LIST_VALUE_INVALID);
    ssidx = samesongGetSSIdx (ss, dbidx);
    val = nlistGetNum (ss->sscounts, ssidx);
    --val;
    nlistSetNum (ss->sscounts, ssidx, val);
  }

  samesongCleanSingletons (ss);
}

/* internal routines */

static inline ssize_t
samesongGetSSIdx (samesong_t *ss, dbidx_t dbidx)
{
  song_t  *song;
  ssize_t ssidx;

  song = dbGetByIdx (ss->musicdb, dbidx);
  if (song == NULL) {
    return LIST_VALUE_INVALID;
  }

  ssidx = songGetNum (song, TAG_SAMESONG);
  return ssidx;
}

static void
samesongCleanSingletons (samesong_t *ss)
{
  nlistidx_t  iteridx;
  ssize_t     ssidx;
  int         val;

  nlistStartIterator (ss->sscounts, &iteridx);
  while ((ssidx = nlistIterateKey (ss->sscounts, &iteridx)) >= 0) {
    val = nlistGetNum (ss->sscounts, ssidx);
    if (val == 1) {
      nlistSetStr (ss->sscolors, ssidx, NULL);
    }
  }
}


