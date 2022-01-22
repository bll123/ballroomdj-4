#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "autosel.h"
#include "bdjvarsdf.h"
#include "level.h"
#include "nlist.h"
#include "log.h"
#include "musicdb.h"
#include "portability.h"
#include "queue.h"
#include "rating.h"
#include "song.h"
#include "songsel.h"
#include "tagdef.h"

static void  songselRemoveSong (songsel_t *songsel,
                songseldance_t *songseldance,
                songselsongdata_t *songdata);
static songselsongdata_t  *searchForPercentage (songseldance_t *songseldance,
                          double dval);
static songselperc_t  *songselInitPerc (list_t *plist, nlistidx_t idx,
                          ssize_t weight, nlistidx_t *ridx);
static void  songselDanceFree (void *titem);
static void  songselIdxFree (void *titem);
static void  songselSongDataFree (void *titem);
static void  songselPercFree (void *titem);
static void  calcPercentages (songseldance_t *songseldance);
static void  calcAttributePerc (songsel_t *songsel,
                songseldance_t *songseldance, songselattr_t attridx);

songsel_t *
songselAlloc (list_t *dancelist, songselFilter_t filterProc, void *userdata)
{
  songsel_t       *songsel;
  nlistidx_t       dkey;
  song_t          *song;
  dbidx_t         dbidx;
  songseldance_t  *songseldance;
  rating_t        *ratings;
  level_t         *levels;
  autosel_t       *autosel;
  nlistidx_t      iteridx;
  nlistidx_t      dbiteridx;


  logProcBegin (LOG_PROC, "songselAlloc");
  songsel = malloc (sizeof (songsel_t));
  assert (songsel != NULL);
  songsel->danceSelList = nlistAlloc ("songsel-sel", LIST_ORDERED, songselDanceFree);
  nlistSetSize (songsel->danceSelList, nlistGetCount (dancelist));

  autosel = bdjvarsdf [BDJVDF_AUTO_SEL];
  songsel->autoselWeight [SONGSEL_ATTR_RATING] = autoselGetDouble (autosel, AUTOSEL_RATING_WEIGHT);
  songsel->autoselWeight [SONGSEL_ATTR_LEVEL] = autoselGetDouble (autosel, AUTOSEL_LEVEL_WEIGHT);
  songsel->lastSelection = NULL;

  nlistStartIterator (dancelist, &iteridx);
    /* for each dance : first set up all the lists */
  while ((dkey = nlistIterateKey (dancelist, &iteridx)) >= 0) {
    logMsg (LOG_DBG, LOG_SONGSEL, "adding dance: %ld", dkey);
    songseldance = malloc (sizeof (songseldance_t));
    assert (songseldance != NULL);
    songseldance->danceKey = dkey;
    songseldance->songIdxList = nlistAlloc ("songsel-songidx",
        LIST_ORDERED, songselSongDataFree);
    songseldance->currentIndexes = queueAlloc (songselIdxFree);
    songseldance->currentIdxList = nlistAlloc ("songsel-curridx",
        LIST_UNORDERED, NULL);
    songseldance->attrList [SONGSEL_ATTR_RATING] =
        nlistAlloc ("songsel-rating", LIST_ORDERED, songselPercFree);
    songseldance->attrList [SONGSEL_ATTR_LEVEL] =
        nlistAlloc ("songsel-level", LIST_ORDERED, songselPercFree);
    nlistSetData (songsel->danceSelList, dkey, songseldance);
  }

  ratings = bdjvarsdf [BDJVDF_RATINGS];
  levels = bdjvarsdf [BDJVDF_LEVELS];

    /* for each song in the database */
  logMsg (LOG_DBG, LOG_SONGSEL, "processing songs");
  dbStartIterator (&dbiteridx);
  while ((song = dbIterate (&dbidx, &dbiteridx)) != NULL) {
    songselperc_t   *perc = NULL;
    songselsongdata_t   *songdata = NULL;
    songselidx_t    *songidx = NULL;
    nlistidx_t       rating = 0;
    nlistidx_t       level = 0;
    nlistidx_t       ratingIdx = 0;
    nlistidx_t       levelIdx = 0;
    ssize_t         weight = 0;


    dkey = songGetNum (song, TAG_DANCE);
    songseldance = nlistGetData (songsel->danceSelList, dkey);
    if (songseldance == NULL) {
      continue;
    }

    rating = songGetNum (song, TAG_DANCERATING);
    if (rating < 0) {
      logMsg (LOG_DBG, LOG_SONGSEL, "song %zd reject: unknown rating", dbidx);
      continue;
    }
    weight = ratingGetWeight (ratings, rating);
    if (weight == 0) {
      logMsg (LOG_DBG, LOG_SONGSEL, "song %zd reject: rating weight 0", dbidx);
      continue;
    }
    level = songGetNum (song, TAG_DANCELEVEL);
    if (level < 0) {
      logMsg (LOG_DBG, LOG_SONGSEL, "song %zd reject: unknown level", dbidx);
      continue;
    }
    weight = levelGetWeight (levels, level);
    if (weight == 0) {
      logMsg (LOG_DBG, LOG_SONGSEL, "song %zd reject: level weight 0", dbidx);
      continue;
    }

    if (! filterProc (dbidx, song, userdata)) {
      continue;
    }

    weight = ratingGetWeight (ratings, rating);
    perc = nlistGetData (songseldance->attrList [SONGSEL_ATTR_RATING], rating);
    if (perc == NULL) {
      perc = songselInitPerc (songseldance->attrList [SONGSEL_ATTR_RATING],
          rating, weight, &ratingIdx);
    }
    ++perc->origCount;
    ++perc->count;

    weight = levelGetWeight (levels, level);
    perc = nlistGetData (songseldance->attrList [SONGSEL_ATTR_LEVEL], level);
    if (perc == NULL) {
      perc = songselInitPerc (songseldance->attrList [SONGSEL_ATTR_LEVEL],
          level, weight, &levelIdx);
    }
    ++perc->origCount;
    ++perc->count;

    songdata = malloc (sizeof (songselsongdata_t));
    assert (songdata != NULL);
    songidx = malloc (sizeof (songselidx_t));
    assert (songidx != NULL);
    songdata->dbidx = dbidx;
    songdata->attrIdx [SONGSEL_ATTR_RATING] = ratingIdx;
    songdata->attrIdx [SONGSEL_ATTR_LEVEL] = levelIdx;
    songdata->percentage = 0.0;
    songidx->idx = nlistSetData (songseldance->songIdxList, dbidx, songdata);
    songdata->idx = songidx->idx;
    queuePush (songseldance->currentIndexes, songidx);
    nlistSetNum (songseldance->currentIdxList, songidx->idx, songidx->idx);
  }

  nlistStartIterator (songsel->danceSelList, &iteridx);
    /* for each dance */
  while ((songseldance =
        nlistIterateValueData (songsel->danceSelList, &iteridx)) != NULL) {
    logMsg (LOG_DBG, LOG_SONGSEL, "rating: calcAttributePerc");
    calcAttributePerc (songsel, songseldance, SONGSEL_ATTR_RATING);
    logMsg (LOG_DBG, LOG_SONGSEL, "level: calcAttributePerc");
    calcAttributePerc (songsel, songseldance, SONGSEL_ATTR_LEVEL);
    logMsg (LOG_DBG, LOG_SONGSEL, "calculate running totals");
    calcPercentages (songseldance);
    logMsg (LOG_DBG, LOG_SONGSEL, "dance %ld : %ld songs",
        songseldance->danceKey, nlistGetCount (songseldance->songIdxList));
  }

  logProcEnd (LOG_PROC, "songselAlloc", "");
  return songsel;
}

void
songselFree (songsel_t *songsel)
{
  if (songsel != NULL) {
    if (songsel->danceSelList != NULL) {
      nlistFree (songsel->danceSelList);
    }
    free (songsel);
  }
  return;
}

song_t *
songselSelect (songsel_t *songsel, nlistidx_t danceIdx)
{
  songseldance_t      *songseldance = NULL;
  songselsongdata_t   *songdata = NULL;
  double              dval = 0.0;
  song_t              *song = NULL;

  if (songsel == NULL) {
    return NULL;
  }

  songseldance = nlistGetData (songsel->danceSelList, danceIdx);
  if (songseldance == NULL) {
    songsel->lastSelection = NULL;
    return NULL;
  }

  dval = dRandom ();
    /* since the percentages are in order, do a binary search */
  songdata = searchForPercentage (songseldance, dval);
  if (songdata != NULL) {
    song = dbGetByIdx (songdata->dbidx);
    logMsg (LOG_DBG, LOG_SONGSEL, "selected idx:%zd dbidx:%zd from %zd",
        songdata->idx, songdata->dbidx, songseldance->danceKey);
    songsel->lastSelection = songdata;
  }
  return song;
}

void
songselSelectFinalize (songsel_t *songsel, nlistidx_t danceIdx)
{
  songseldance_t      *songseldance = NULL;
  songselsongdata_t   *songdata = NULL;

  if (songsel == NULL) {
    return;
  }

  songseldance = nlistGetData (songsel->danceSelList, danceIdx);
  if (songseldance == NULL) {
    return;
  }

  if (songsel->lastSelection != NULL) {
    songdata = songsel->lastSelection;
    logMsg (LOG_DBG, LOG_SONGSEL, "removing idx:%zd dbidx:%zd from %zd",
        songdata->idx, songdata->dbidx, songseldance->danceKey);
    songselRemoveSong (songsel, songseldance, songdata);
    songsel->lastSelection = NULL;
  }
  return;
}

/* internal routines */

static void
songselRemoveSong (songsel_t *songsel,
    songseldance_t *songseldance, songselsongdata_t *songdata)
{
  songselidx_t    *songidx = NULL;
  list_t          *nlist = NULL;
  ssize_t         count = 0;
  nlistidx_t      iteridx;
  ssize_t         qiteridx;


  count = queueGetCount (songseldance->currentIndexes);
  nlist = nlistAlloc ("songsel-curridx", LIST_UNORDERED, NULL);
  if (count == 1) {
      /* need a complete rebuild */
    queueFree (songseldance->currentIndexes);
    songseldance->currentIndexes = queueAlloc (songselIdxFree);
    count = nlistGetCount (songseldance->songIdxList);
    nlistSetSize (nlist, count);
    nlistStartIterator (songseldance->songIdxList, &iteridx);
    while ((songdata = nlistIterateValueData (songseldance->songIdxList, &iteridx)) != NULL) {
      songidx = malloc (sizeof (songselidx_t));
      assert (songidx != NULL);
      songidx->idx = songdata->idx;
      queuePush (songseldance->currentIndexes, songidx);
      nlistSetNum (nlist, songdata->idx, songdata->idx);
    }
  } else {
    nlistSetSize (nlist, count - 1);
    queueStartIterator (songseldance->currentIndexes, &qiteridx);
    while ((songidx =
          queueIterateData (songseldance->currentIndexes, &qiteridx)) != NULL) {
      if (songidx->idx == songdata->idx) {
        queueIterateRemoveNode (songseldance->currentIndexes, &qiteridx);
        songselIdxFree (songidx);
        continue;
      }
      nlistSetNum (nlist, songidx->idx, songidx->idx);
    }
  }

  nlistFree (songseldance->currentIdxList);
  songseldance->currentIdxList = nlist;

  calcAttributePerc (songsel, songseldance, SONGSEL_ATTR_RATING);
  calcAttributePerc (songsel, songseldance, SONGSEL_ATTR_LEVEL);
  calcPercentages (songseldance);
  return;
}

static songselsongdata_t *
searchForPercentage (songseldance_t *songseldance, double dval)
{
  nlistidx_t       l = 0;
  nlistidx_t       r = 0;
  nlistidx_t       m = 0;
  int             rca = 0;
  int             rcb = 0;
  songselsongdata_t   *tsongdata = NULL;
  nlistidx_t       dataidx = 0;


  r = nlistGetCount (songseldance->currentIdxList);

  while (l <= r) {
    m = l + (r - l) / 2;

    if (m != 0) {
      dataidx = nlistGetNumByIdx (songseldance->currentIdxList, m - 1);
      tsongdata = nlistGetDataByIdx (songseldance->songIdxList, dataidx);
      rca = tsongdata->percentage < dval;
    }
    dataidx = nlistGetNumByIdx (songseldance->currentIdxList, m);
    tsongdata = nlistGetDataByIdx (songseldance->songIdxList, dataidx);
    rcb = tsongdata->percentage >= dval;
    if ((m == 0 || rca) && rcb) {
      return tsongdata;
    }

    if (! rcb) {
      l = m + 1;
    } else {
      r = m - 1;
    }
  }

  return NULL;
}

static void
calcPercentages (songseldance_t *songseldance)
{
  list_t              *songIdxList = songseldance->songIdxList;
  list_t              *currentIdxList = songseldance->currentIdxList;
  songselperc_t       *perc = NULL;
  double              wsum = 0.0;
  songselsongdata_t   *songdata;
  songselsongdata_t   *lastsongdata;
  nlistidx_t           dataidx;
  nlistidx_t          iteridx;


  logProcBegin (LOG_PROC, "calcPercentages");
  lastsongdata = NULL;
  nlistStartIterator (currentIdxList, &iteridx);
  while ((dataidx = nlistIterateValueNum (currentIdxList, &iteridx)) >= 0) {
    songdata = nlistGetDataByIdx (songIdxList, dataidx);
    for (songselattr_t i = 0; i < SONGSEL_ATTR_MAX; ++i) {
      perc = nlistGetDataByIdx (songseldance->attrList [i], songdata->attrIdx [i]);
      if (perc != NULL) {
        wsum += perc->calcperc;
      }
    }
    songdata->percentage = wsum;
    lastsongdata = songdata;
    logMsg (LOG_DBG, LOG_SONGSEL, "percentage: %.6f", wsum);
  }
  if (lastsongdata != NULL) {
    lastsongdata->percentage = 1.0;
  }

  logProcEnd (LOG_PROC, "calcPercentages", "");
}

static void
calcAttributePerc (songsel_t *songsel, songseldance_t *songseldance,
    songselattr_t attridx)
{
  songselperc_t       *perc = NULL;
  double              tot = 0.0;
  list_t              *list = songseldance->attrList [attridx];
  nlistidx_t          iteridx;


  logProcBegin (LOG_PROC, "calcAttributePerc");
  tot = 0.0;
  nlistStartIterator (list, &iteridx);
  while ((perc = nlistIterateValueData (list, &iteridx)) != NULL) {
    tot += (double) perc->weight;
    logMsg (LOG_DBG, LOG_SONGSEL, "weight: %zd tot: %f", perc->weight, tot);
  }

  nlistStartIterator (list, &iteridx);
  while ((perc = nlistIterateValueData (list, &iteridx)) != NULL) {
    if (perc->count == 0) {
      perc->calcperc = 0.0;
    } else {
      perc->calcperc = (double) perc->weight / tot /
          (double) perc->count * songsel->autoselWeight [attridx];
    }
    logMsg (LOG_DBG, LOG_SONGSEL, "calcperc: %.6f", perc->calcperc);
  }
  logProcEnd (LOG_PROC, "calcAttributePerc", "");
}

static songselperc_t *
songselInitPerc (list_t *attrlist, nlistidx_t idx,
    ssize_t weight, nlistidx_t *ridx)
{
  songselperc_t       *perc;

  perc = malloc (sizeof (songselperc_t));
  assert (perc != NULL);
  perc->origCount = 0;
  perc->weight = weight;
  perc->count = 0;
  perc->calcperc = 0.0;
  *ridx = nlistSetData (attrlist, idx, perc);
  return perc;
}

static void
songselDanceFree (void *titem)
{
  songseldance_t       *songseldance = titem;

  if (songseldance != NULL) {
    if (songseldance->songIdxList != NULL) {
      nlistFree (songseldance->songIdxList);
    }
    if (songseldance->currentIndexes != NULL) {
      queueFree (songseldance->currentIndexes);
    }
    if (songseldance->currentIdxList != NULL) {
      nlistFree (songseldance->currentIdxList);
    }
    for (songselattr_t i = 0; i < SONGSEL_ATTR_MAX; ++i) {
      if (songseldance->attrList [i] != NULL) {
        nlistFree (songseldance->attrList [i]);
      }
    }
    free (songseldance);
  }
}

static void
songselIdxFree (void *titem)
{
  songselidx_t       *songselidx = titem;

  if (songselidx != NULL) {
    free (songselidx);
  }
}

static void
songselSongDataFree (void *titem)
{
  songselidx_t       *songselidx = titem;

  if (songselidx != NULL) {
    free (songselidx);
  }
}

static void
songselPercFree (void *titem)
{
  songselperc_t       *songselperc = titem;

  if (songselperc != NULL) {
    free (songselperc);
  }
}
