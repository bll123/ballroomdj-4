#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "songsel.h"
#include "song.h"
#include "list.h"
#include "musicdb.h"
#include "tagdef.h"
#include "rating.h"
#include "level.h"
#include "bdjvarsdf.h"
#include "log.h"
#include "portability.h"
#include "autosel.h"

static songselsong_t  *searchForPercentage (list_t *currIdxList, double dval);
static songselperc_t  *songselInitPerc (list_t *plist, long idx, long weight);
static void           songselDanceFree (void *titem);
static void           songselIdxFree (void *titem);
static void           songselPercFree (void *titem);
static void           calcPercentages (songseldance_t *songseldance);
static void           calcAttributePerc (list_t *list, autoselkey_t idx);

songsel_t *
songselAlloc (list_t *dancelist, songselFilter_t filterProc, void *userdata)
{
  songsel_t       *songsel;
  long            dkey;
  song_t          *song;
  size_t          dbidx;
  songseldance_t  *songseldance;
  rating_t        *ratings;
  level_t         *levels;


  logProcBegin (LOG_PROC, "songselAlloc");
  songsel = malloc (sizeof (songsel));
  assert (songsel != NULL);
  songsel->danceSelList = llistAlloc ("songsel-sel", LIST_ORDERED, songselDanceFree);
  llistSetSize (songsel->danceSelList, llistGetSize (dancelist));

  llistStartIterator (dancelist);
    /* for each dance : first set up all the lists */
  while ((dkey = llistIterateKeyLong (dancelist)) >= 0) {
    logMsg (LOG_DBG, LOG_SONGSEL, "adding dance: %ld", dkey);
    songseldance = malloc (sizeof (songseldance_t));
    assert (songseldance != NULL);
    songseldance->danceKey = dkey;
    songseldance->songIdxList = llistAlloc ("songsel-songidx",
        LIST_ORDERED, songselIdxFree);
    songseldance->currIdxList = llistAlloc ("songsel-curridx",
        LIST_ORDERED, NULL);
    songseldance->ratingList = llistAlloc ("songsel-rating",
        LIST_ORDERED, songselPercFree);
    songseldance->levelList = llistAlloc ("songsel-level",
        LIST_ORDERED, songselPercFree);
    llistSetData (songsel->danceSelList, dkey, songseldance);
  }

  ratings = bdjvarsdf [BDJVDF_RATINGS];
  levels = bdjvarsdf [BDJVDF_LEVELS];

    /* for each song in the database */
  logMsg (LOG_DBG, LOG_SONGSEL, "processing songs");
  dbStartIterator ();
  while ((song = dbIterate(&dbidx)) != NULL) {
    songselperc_t   *perc;
    songselsong_t   *songidx;
    long            ratingIdx;
    long            levelIdx;
    long            weight;

    dkey = songGetLong (song, TAG_DANCE);
    songseldance = llistGetData (songsel->danceSelList, dkey);
    if (songseldance == NULL) {
      logMsg (LOG_DBG, LOG_SONGSEL, "song %zd reject: incorrect dance", dbidx);
        /* for the song selector, we don't need to track dances */
        /* that the playlist does not need. */
      continue;
    }

    ratingIdx = songGetLong (song, TAG_DANCERATING);
    if (ratingIdx < 0) {
      logMsg (LOG_DBG, LOG_SONGSEL, "song %zd reject: unknown rating", dbidx);
      continue;
    }
    weight = ratingGetWeight (ratings, ratingIdx);
    if (weight == 0) {
      logMsg (LOG_DBG, LOG_SONGSEL, "song %zd reject: rating weight 0", dbidx);
      continue;
    }
    levelIdx = songGetLong (song, TAG_DANCELEVEL);
    if (levelIdx < 0) {
      logMsg (LOG_DBG, LOG_SONGSEL, "song %zd reject: unknown level", dbidx);
      continue;
    }
    weight = levelGetWeight (levels, levelIdx);
    if (weight == 0) {
      logMsg (LOG_DBG, LOG_SONGSEL, "song %zd reject: level weight 0", dbidx);
      continue;
    }

    if (! filterProc (song, userdata)) {
      logMsg (LOG_DBG, LOG_SONGSEL, "song %zd reject: filter", dbidx);
      continue;
    }

    weight = ratingGetWeight (ratings, ratingIdx);
    perc = llistGetData (songseldance->ratingList, ratingIdx);
    if (perc == NULL) {
      perc = songselInitPerc (songseldance->ratingList, ratingIdx, weight);
    }
    ++perc->origCount;
    ++perc->count;

    weight = levelGetWeight (levels, levelIdx);
    perc = llistGetData (songseldance->levelList, levelIdx);
    if (perc == NULL) {
      perc = songselInitPerc (songseldance->levelList, levelIdx, weight);
    }
    ++perc->origCount;
    ++perc->count;

    songidx = malloc (sizeof (songselsong_t));
    assert (songidx != NULL);
    songidx->dbidx = dbidx;
    songidx->ratingIdx = ratingIdx;
    songidx->levelIdx = levelIdx;
    songidx->percentage = 0.0;
    llistSetData (songseldance->songIdxList, dbidx, songidx);
    llistSetData (songseldance->currIdxList, dbidx, songidx);
  }

  llistStartIterator (songsel->danceSelList);
    /* for each dance */
  while ((songseldance = llistIterateValue (songsel->danceSelList)) != NULL) {
    logMsg (LOG_DBG, LOG_SONGSEL, "rating: calcAttributePerc");
    calcAttributePerc (songseldance->ratingList, AUTOSEL_RATING_WEIGHT);
    logMsg (LOG_DBG, LOG_SONGSEL, "level: calcAttributePerc");
    calcAttributePerc (songseldance->levelList, AUTOSEL_LEVEL_WEIGHT);
    logMsg (LOG_DBG, LOG_SONGSEL, "calculate running totals");
    calcPercentages (songseldance);
    logMsg (LOG_DBG, LOG_SONGSEL, "dance %ld : %ld songs",
        songseldance->danceKey, llistGetSize (songseldance->songIdxList));
  }

  logProcEnd (LOG_PROC, "songselAlloc", "");
  return songsel;
}

void
songselFree (songsel_t *songsel)
{
  if (songsel != NULL) {
    if (songsel->danceSelList != NULL) {
      llistFree (songsel->danceSelList);
    }
    free (songsel);
  }
  return;
}

song_t *
songselSelect (songsel_t *songsel, long danceIdx)
{
  songseldance_t      *songseldance = NULL;
  songselsong_t       *songidx = NULL;
  double              dval = 0.0;
  song_t              *song = NULL;

  if (songsel == NULL) {
    return NULL;
  }

  songseldance = llistGetData (songsel->danceSelList, danceIdx);
  if (songseldance == NULL) {
    return NULL;
  }

  dval = dRandom ();
    /* since the percentages are in order, do a binary search */
  songidx = searchForPercentage (songseldance->currIdxList, dval);
  if (songidx != NULL) {
    song = dbGetByIdx (songidx->dbidx);
  }
  return song;
}

/* internal routines */

static songselsong_t *
searchForPercentage (list_t *currIdxList, double dval)
{
  long            l = 0;
  long            r = (long) currIdxList->count - 1;
  long            m = 0;
  int             rca = 0;
  int             rcb = 0;
  songselsong_t   *tsongidx = NULL;


  while (l <= r) {
    m = l + (r - l) / 2;

    if (m != 0) {
      tsongidx = llistGetDataByIdx (currIdxList, m - 1);
      rca = tsongidx->percentage < dval;
    }
    tsongidx = llistGetDataByIdx (currIdxList, m);
    rcb = tsongidx->percentage >= dval;
    if ((m == 0 || rca) && rcb) {
      return tsongidx;
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
  list_t          *ratingList = songseldance->ratingList;
  list_t          *levelList = songseldance->levelList;
  list_t          *currIdxList = songseldance->currIdxList;
  songselperc_t   *perc = NULL;
  double          wsum = 0.0;
  songselsong_t   *songidx;
  songselsong_t   *lastsongidx;


  logProcBegin (LOG_PROC, "calcPercentages");
  llistStartIterator (currIdxList);
  lastsongidx = NULL;
  while ((songidx = llistIterateValue (currIdxList)) != NULL) {
    perc = llistGetData (ratingList, songidx->ratingIdx);
    wsum += perc->calcperc;
    perc = llistGetData (levelList, songidx->levelIdx);
    wsum += perc->calcperc;
    songidx->percentage = wsum;
    lastsongidx = songidx;
    logMsg (LOG_DBG, LOG_SONGSEL, "percentage: %.2f", wsum);
  }
  if (lastsongidx != NULL) {
    lastsongidx->percentage = 100.0;
  }

  logProcEnd (LOG_PROC, "calcPercentages", "");
}

static void
calcAttributePerc (list_t *list, autoselkey_t idx)
{
  songselperc_t   *perc;
  double          tot;
  double          weight;
  autosel_t       *autosel;


  autosel = bdjvarsdf [BDJVDF_AUTO_SEL];
  weight = autoselGetDouble (autosel, idx);

  logProcBegin (LOG_PROC, "calcAttributePerc");
  tot = 0.0;
  llistStartIterator (list);
  while ((perc = llistIterateValue (list)) != NULL) {
    tot += (double) perc->weight;
    logMsg (LOG_DBG, LOG_SONGSEL, "weight: %ld tot: %f", perc->weight, tot);
  }

  llistStartIterator (list);
  while ((perc = llistIterateValue (list)) != NULL) {
    if (perc->count == 0) {
      perc->calcperc = 0.0;
    } else {
      perc->calcperc = (double) perc->weight / tot /
          (double) perc->count * weight;
    }
    logMsg (LOG_DBG, LOG_SONGSEL, "calcperc: %.2f", perc->calcperc);
  }
  logProcEnd (LOG_PROC, "calcAttributePerc", "");
}

static songselperc_t *
songselInitPerc (list_t *plist, long idx, long weight)
{
  songselperc_t       *perc;

  perc = malloc (sizeof (songselperc_t));
  assert (perc != NULL);
  perc->origCount = 0;
  perc->weight = weight;
  perc->count = 0;
  perc->calcperc = 0.0;
  llistSetData (plist, idx, perc);
  return perc;
}

static void
songselDanceFree (void *titem)
{
  songseldance_t       *songseldance = titem;

  if (songseldance != NULL) {
    if (songseldance->songIdxList != NULL) {
      llistFree (songseldance->songIdxList);
    }
    if (songseldance->currIdxList != NULL) {
      llistFree (songseldance->currIdxList);
    }
    if (songseldance->ratingList != NULL) {
      llistFree (songseldance->ratingList);
    }
    if (songseldance->levelList != NULL) {
      llistFree (songseldance->levelList);
    }
    free (songseldance);
  }
}

static void
songselIdxFree (void *titem)
{
  songselsong_t       *songselidx = titem;

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
