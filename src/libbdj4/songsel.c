#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "autosel.h"
#include "bdjvarsdf.h"
#include "ilist.h"
#include "level.h"
#include "nlist.h"
#include "log.h"
#include "musicdb.h"
#include "osrandom.h"
#include "queue.h"
#include "rating.h"
#include "song.h"
#include "songfilter.h"
#include "songsel.h"
#include "tagdef.h"

typedef struct songselidx {
  nlistidx_t     idx;
} songselidx_t;

typedef struct songselsongdata {
  nlistidx_t    idx;
  dbidx_t       dbidx;
  nlistidx_t    rating;
  nlistidx_t    level;
  nlistidx_t    attrIdx [SONGSEL_ATTR_MAX];
  double        percentage;
} songselsongdata_t;

/* used for both ratings and levels */
typedef struct songselperc {
  ssize_t       origCount;      // count of number of songs for this idx
  ssize_t       weight;         // weight for this idx
  ssize_t       count;          // current count of number of songs
  double        calcperc;       // current percentage adjusted by weight
} songselperc_t;

typedef struct songseldance {
  nlistidx_t  danceIdx;
  nlist_t     *songIdxList;
  queue_t     *currentIndexes;
  nlist_t     *currentIdxList;
  nlist_t     *attrList [SONGSEL_ATTR_MAX];
} songseldance_t;

typedef struct songsel {
  nlist_t             *danceSelList;
  double              autoselWeight [SONGSEL_ATTR_MAX];
  songselsongdata_t   *lastSelection;
  musicdb_t           *musicdb;
} songsel_t;

static void   songselAllocAddSong (songsel_t *songsel, dbidx_t dbidx, song_t *song, songfilter_t *songfilter);
static void   songselRemoveSong (songsel_t *songsel, songseldance_t *songseldance, songselsongdata_t *songdata);
static songselsongdata_t * searchForPercentage (songseldance_t *songseldance, double dval);
static songselperc_t * songselInitPerc (list_t *plist, nlistidx_t idx, ssize_t weight);
static void   songselDanceFree (void *titem);
static void   songselIdxFree (void *titem);
static void   songselSongDataFree (void *titem);
static void   songselPercFree (void *titem);
static void   calcPercentages (songseldance_t *songseldance);
static void   calcAttributePerc (songsel_t *songsel, songseldance_t *songseldance, songselattr_t attridx);

/*
 *  danceSelList:
 *    indexed by the dance index.
 *    contains a 'songseldance_t'; the song selection list for the dance.
 *  song selection dance:
 *    indexed by the dbidx.
 *    contains:
 *      danceidx
 *      song index list
 *      current index list
 *      current indexes
 *      attribute lists (rating, level)
 *        these have the weights by rating/level.
 *  song index list:
 *    indexed by the list index.
 *    this is the master list, and the current index list will be rebuilt
 *    from this.
 *  current indexes:
 *    the current set of list indexes to choose from.
 *    starts as a copy of a song index list.
 *    this is a queue so that entries may be removed.
 *    when emptied, it is re-copied from the song index list.
 *  current index list:
 *    a simple list of list indexes.
 *    this is rebuilt entirely on every removal.
 *
 */

songsel_t *
songselAlloc (musicdb_t *musicdb, nlist_t *dancelist,
    nlist_t *songlist, songfilter_t *songfilter)
{
  songsel_t       *songsel;
  ilistidx_t      danceIdx;
  song_t          *song;
  dbidx_t         dbidx;
  songseldance_t  *songseldance;
  autosel_t       *autosel;
  nlistidx_t      iteridx;
  nlistidx_t      siteridx;
  nlistidx_t      dbiteridx;


  logProcBegin (LOG_PROC, "songselAlloc");
  songsel = malloc (sizeof (songsel_t));
  assert (songsel != NULL);
  songsel->danceSelList = nlistAlloc ("songsel-sel", LIST_ORDERED, songselDanceFree);
  nlistSetSize (songsel->danceSelList, nlistGetCount (dancelist));

  autosel = bdjvarsdfGet (BDJVDF_AUTO_SEL);
  songsel->autoselWeight [SONGSEL_ATTR_RATING] = autoselGetDouble (autosel, AUTOSEL_RATING_WEIGHT);
  songsel->autoselWeight [SONGSEL_ATTR_LEVEL] = autoselGetDouble (autosel, AUTOSEL_LEVEL_WEIGHT);
  songsel->lastSelection = NULL;

  nlistStartIterator (dancelist, &iteridx);
  /* for each dance : first set up all the lists */
  while ((danceIdx = nlistIterateKey (dancelist, &iteridx)) >= 0) {
    logMsg (LOG_DBG, LOG_SONGSEL, "adding dance: %ld", danceIdx);
    songseldance = malloc (sizeof (songseldance_t));
    assert (songseldance != NULL);
    songseldance->danceIdx = danceIdx;
    songseldance->songIdxList = nlistAlloc ("songsel-songidx",
        LIST_ORDERED, songselSongDataFree);
    songseldance->currentIndexes = queueAlloc (songselIdxFree);
    songseldance->currentIdxList = nlistAlloc ("songsel-curridx",
        LIST_UNORDERED, NULL);
    songseldance->attrList [SONGSEL_ATTR_RATING] =
        nlistAlloc ("songsel-rating", LIST_ORDERED, songselPercFree);
    songseldance->attrList [SONGSEL_ATTR_LEVEL] =
        nlistAlloc ("songsel-level", LIST_ORDERED, songselPercFree);
    nlistSetData (songsel->danceSelList, danceIdx, songseldance);
  }

  songsel->musicdb = musicdb;

  if (songlist != NULL) {
    /* for each song in the supplied song list */
    logMsg (LOG_DBG, LOG_SONGSEL, "processing songs from songlist (%d)",
        nlistGetCount (songlist));
    nlistStartIterator (songlist, &iteridx);
    while ((dbidx = nlistIterateKey (songlist, &iteridx)) >= 0) {
      song = dbGetByIdx (musicdb, dbidx);
      songselAllocAddSong (songsel, dbidx, song, songfilter);
    }
  } else {
    /* for each song in the database */
    logMsg (LOG_DBG, LOG_SONGSEL, "processing songs from database");
    dbStartIterator (musicdb, &dbiteridx);
    while ((song = dbIterate (musicdb, &dbidx, &dbiteridx)) != NULL) {
      songselAllocAddSong (songsel, dbidx, song, songfilter);
    }
  }

  /* for each dance */
  logMsg (LOG_DBG, LOG_SONGSEL, "process dances");
  nlistStartIterator (songsel->danceSelList, &iteridx);
  while ((songseldance =
      nlistIterateValueData (songsel->danceSelList, &iteridx)) != NULL) {
    ssize_t           idx;
    songselidx_t      *songidx = NULL;
    songselsongdata_t *songdata = NULL;

    logMsg (LOG_DBG, LOG_SONGSEL, "dance: %zd count: %zd ", songseldance->danceIdx, nlistGetCount (songseldance->songIdxList));

    /* for each selected song for that dance */
    /* the rating/level attribute indexes must be set afterwards */
    /* the indexes may only be determined after the list has been built */
    logMsg (LOG_DBG, LOG_SONGSEL, "save list indexes.");
    idx = 0;
    nlistStartIterator (songseldance->songIdxList, &siteridx);
    while ((songdata =
        nlistIterateValueData (songseldance->songIdxList, &siteridx)) != NULL) {
      /* save the list index */
      songidx = malloc (sizeof (songselidx_t));
      assert (songidx != NULL);
      songidx->idx = idx;
      songdata->idx = idx;
      logMsg (LOG_DBG, LOG_SONGSEL, "  push song idx: %zd", songidx->idx);
      queuePush (songseldance->currentIndexes, songidx);
      nlistSetNum (songseldance->currentIdxList, songidx->idx, songidx->idx);
      ++idx;
    }

    logMsg (LOG_DBG, LOG_SONGSEL, "rating: calcAttributePerc");
    calcAttributePerc (songsel, songseldance, SONGSEL_ATTR_RATING);

    logMsg (LOG_DBG, LOG_SONGSEL, "level: calcAttributePerc");
    calcAttributePerc (songsel, songseldance, SONGSEL_ATTR_LEVEL);

    logMsg (LOG_DBG, LOG_SONGSEL, "calculate running totals");
    calcPercentages (songseldance);

    logMsg (LOG_DBG, LOG_SONGSEL, "dance %ld : %ld songs",
        songseldance->danceIdx, nlistGetCount (songseldance->songIdxList));
  }

  logProcEnd (LOG_PROC, "songselAlloc", "");
  return songsel;
}

void
songselFree (songsel_t *songsel)
{
  logProcBegin (LOG_PROC, "songselFree");
  if (songsel != NULL) {
    if (songsel->danceSelList != NULL) {
      nlistFree (songsel->danceSelList);
    }
    free (songsel);
  }
  logProcEnd (LOG_PROC, "songselFree", "");
  return;
}

song_t *
songselSelect (songsel_t *songsel, ilistidx_t danceIdx)
{
  songseldance_t      *songseldance = NULL;
  songselsongdata_t   *songdata = NULL;
  double              dval = 0.0;
  song_t              *song = NULL;

  if (songsel == NULL) {
    return NULL;
  }

  logProcBegin (LOG_PROC, "songselSelect");
  songseldance = nlistGetData (songsel->danceSelList, danceIdx);
  if (songseldance == NULL) {
    songsel->lastSelection = NULL;
    logProcEnd (LOG_PROC, "songselSelect", "no dance");
    return NULL;
  }

  dval = dRandom ();
  /* since the percentages are in order, do a binary search */
  songdata = searchForPercentage (songseldance, dval);
  if (songdata != NULL) {
    song = dbGetByIdx (songsel->musicdb, songdata->dbidx);
    logMsg (LOG_DBG, LOG_SONGSEL, "selected idx:%zd dbidx:%zd from %zd",
        songdata->idx, songdata->dbidx, songseldance->danceIdx);
    songsel->lastSelection = songdata;
  }
  logProcEnd (LOG_PROC, "songselSelect", "");
  return song;
}

void
songselSelectFinalize (songsel_t *songsel, ilistidx_t danceIdx)
{
  songseldance_t      *songseldance = NULL;
  songselsongdata_t   *songdata = NULL;

  if (songsel == NULL) {
    return;
  }

  logProcBegin (LOG_PROC, "songselSelect");
  songseldance = nlistGetData (songsel->danceSelList, danceIdx);
  if (songseldance == NULL) {
    logProcEnd (LOG_PROC, "songselSelect", "no dance");
    return;
  }

  if (songsel->lastSelection != NULL) {
    songdata = songsel->lastSelection;
    logMsg (LOG_DBG, LOG_SONGSEL, "removing idx:%zd dbidx:%zd from %zd",
        songdata->idx, songdata->dbidx, songseldance->danceIdx);
    songselRemoveSong (songsel, songseldance, songdata);
    songsel->lastSelection = NULL;
  }
  logProcEnd (LOG_PROC, "songselSelect", "");
  return;
}

/* internal routines */

static void
songselAllocAddSong (songsel_t *songsel, dbidx_t dbidx,
    song_t *song, songfilter_t *songfilter)
{
  songselperc_t     *perc = NULL;
  songselsongdata_t *songdata = NULL;
  songseldance_t    *songseldance;
  nlistidx_t        rating = 0;
  nlistidx_t        level = 0;
  int               weight = 0;
  int               danceIdx;
  rating_t          *ratings;
  level_t           *levels;


  ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  levels = bdjvarsdfGet (BDJVDF_LEVELS);

  danceIdx = songGetNum (song, TAG_DANCE);
  songseldance = nlistGetData (songsel->danceSelList, danceIdx);
  if (songseldance == NULL) {
    return;
  }

  if (songfilter != NULL &&
      ! songfilterFilterSong (songfilter, song)) {
    return;
  }

  weight = ratingGetWeight (ratings, rating);
  perc = nlistGetData (songseldance->attrList [SONGSEL_ATTR_RATING], rating);
  if (perc == NULL) {
    perc = songselInitPerc (songseldance->attrList [SONGSEL_ATTR_RATING],
        rating, weight);
  }
  ++perc->origCount;
  ++perc->count;

  weight = levelGetWeight (levels, level);
  perc = nlistGetData (songseldance->attrList [SONGSEL_ATTR_LEVEL], level);
  if (perc == NULL) {
    perc = songselInitPerc (songseldance->attrList [SONGSEL_ATTR_LEVEL],
        level, weight);
  }
  ++perc->origCount;
  ++perc->count;

  songdata = malloc (sizeof (songselsongdata_t));
  assert (songdata != NULL);
  songdata->dbidx = dbidx;
  songdata->percentage = 0.0;
  songdata->attrIdx [SONGSEL_ATTR_RATING] = rating;
  songdata->attrIdx [SONGSEL_ATTR_LEVEL] = level;
  nlistSetData (songseldance->songIdxList, dbidx, songdata);
}

static void
songselRemoveSong (songsel_t *songsel,
    songseldance_t *songseldance, songselsongdata_t *songdata)
{
  songselidx_t    *songidx = NULL;
  list_t          *nlist = NULL;
  ssize_t         count = 0;
  nlistidx_t      iteridx;
  ssize_t         qiteridx;


  logProcBegin (LOG_PROC, "songselRemoveSong");
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
  logProcEnd (LOG_PROC, "songselRemoveSong", "");
  return;
}

static songselsongdata_t *
searchForPercentage (songseldance_t *songseldance, double dval)
{
  nlistidx_t      l = 0;
  nlistidx_t      r = 0;
  nlistidx_t      m = 0;
  int             rca = 0;
  int             rcb = 0;
  songselsongdata_t   *tsongdata = NULL;
  nlistidx_t       dataidx = 0;


  logProcBegin (LOG_PROC, "searchForPercentage");
  r = nlistGetCount (songseldance->currentIdxList) - 1;

  while (l <= r) {
    m = l + (r - l) / 2;

    if (m != 0) {
      dataidx = nlistGetNumByIdx (songseldance->currentIdxList, m - 1);
      tsongdata = nlistGetDataByIdx (songseldance->songIdxList, dataidx);
      rca = tsongdata->percentage < dval;
    }
    dataidx = nlistGetNumByIdx (songseldance->currentIdxList, m);
    if (dataidx == LIST_VALUE_INVALID) {
      return NULL;
    }
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

  logProcEnd (LOG_PROC, "searchForPercentage", "");
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
      perc = nlistGetData (songseldance->attrList [i], songdata->attrIdx [i]);
      if (perc != NULL) {
        wsum += perc->calcperc;
      }
    }
    songdata->percentage = wsum;
    lastsongdata = songdata;
    logMsg (LOG_DBG, LOG_SONGSEL, "%zd percentage: %.6f", dataidx, wsum);
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
songselInitPerc (list_t *attrlist, nlistidx_t idx, ssize_t weight)
{
  songselperc_t       *perc;

  logProcBegin (LOG_PROC, "songselInitPerc");
  perc = malloc (sizeof (songselperc_t));
  assert (perc != NULL);
  perc->origCount = 0;
  perc->weight = weight;
  perc->count = 0;
  perc->calcperc = 0.0;
  logMsg (LOG_DBG, LOG_SONGSEL, "  init attr %zd %zd", idx, weight);
  nlistSetData (attrlist, idx, perc);
  logProcEnd (LOG_PROC, "songselInitPerc", "");
  return perc;
}

static void
songselDanceFree (void *titem)
{
  songseldance_t       *songseldance = titem;

  logProcBegin (LOG_PROC, "songselDanceFree");
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
  logProcEnd (LOG_PROC, "songselDanceFree", "");
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
