/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "autosel.h"
#include "bdjvarsdf.h"
#include "dance.h"    // for debugging
#include "ilist.h"
#include "level.h"
#include "nlist.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"
#include "osrandom.h"
#include "queue.h"
#include "rating.h"
#include "song.h"
#include "songfilter.h"
#include "songsel.h"
#include "tagdef.h"

enum {
  SONGSEL_ATTR_RATING,
  SONGSEL_ATTR_LEVEL,
  SONGSEL_ATTR_TAGS,
  SONGSEL_ATTR_MAX,
};

typedef struct {
  nlistidx_t     idx;
  nlistidx_t     ssidx;
} songselidx_t;

typedef struct {
  nlistidx_t    idx;
  dbidx_t       dbidx;
  int32_t       weights [SONGSEL_ATTR_MAX];
  nlistidx_t    ssidx;
  double        percentage;
} songselsongdata_t;

typedef struct {
  nlistidx_t  danceIdx;
  nlist_t     *songIdxList;
  queue_t     *currentIndexes;
  nlist_t     *currentIdxList;
  int32_t     origWeights [SONGSEL_ATTR_MAX];
  int32_t     weights [SONGSEL_ATTR_MAX];
  nlistidx_t  currcount;              // not really used
} songseldance_t;

typedef struct songsel {
  dance_t             *dances;          // for debugging
  nlist_t             *danceSelList;
  double              autoselWeight [SONGSEL_ATTR_MAX];
  songselsongdata_t   *lastSelection;
  musicdb_t           *musicdb;
  slist_t             *tagList;
  int                 tagWeight;
  bool                processed : 1;
} songsel_t;

static void songselAllocAddSong (songsel_t *songsel, dbidx_t dbidx, song_t *song);
static void songselRemoveSong (songsel_t *songsel, songseldance_t *songseldance, songselsongdata_t *songdata);
static void songselRebuild (songsel_t *songsel, songseldance_t *songseldance);
static songselsongdata_t * searchForPercentage (songseldance_t *songseldance, double dval);
static void songselDanceFree (void *titem);
static void songselIdxFree (void *titem);
static void songselSongDataFree (void *titem);
static void songselCalcPercentages (songsel_t *songsel, songseldance_t *songseldance);
static void songselProcessDances (songsel_t *songsel);

/*
 *  danceSelList:
 *    indexed by the dance index.
 *    contains a 'songseldance_t'; the song selection list for the dance.
 *  song selection dance:
 *    contains:
 *      danceidx
 *      song index list (master, points to songdata)
 *      current indexes
 *      current index list
 *      origWeights : rating/level/tags
 *      weights : rating/level/tags
 *  song index list:
 *    indexed by the list index.
 *    this is the master list, point to song-data
 *    and the current index list will be rebuilt from this.
 *  current indexes:
 *    the current set of list indexes to choose from.
 *    starts as a copy of a song index list.
 *    this is a queue so that entries may be removed.
 *    when emptied, it is re-copied from the song index list.
 *  current index list:
 *    a list of list indexes.
 *      the same-song index is duplicated here
 *    this is rebuilt entirely on every removal.
 *
 */

songsel_t *
songselAlloc (musicdb_t *musicdb, nlist_t *dancelist)
{
  songsel_t       *songsel;
  ilistidx_t      danceIdx;
  songseldance_t  *songseldance;
  autosel_t       *autosel;
  nlistidx_t      iteridx;


  logProcBegin (LOG_PROC, "songselAlloc");
  songsel = mdmalloc (sizeof (songsel_t));
  songsel->tagList = NULL;
  songsel->tagWeight = BDJ4_DFLT_TAG_WEIGHT;
  songsel->processed = false;

  songsel->dances = bdjvarsdfGet (BDJVDF_DANCES);
  songsel->danceSelList = nlistAlloc ("songsel-sel", LIST_ORDERED, songselDanceFree);
  nlistSetSize (songsel->danceSelList, nlistGetCount (dancelist));

  autosel = bdjvarsdfGet (BDJVDF_AUTO_SEL);
  songsel->autoselWeight [SONGSEL_ATTR_RATING] = autoselGetDouble (autosel, AUTOSEL_RATING_WEIGHT);
  songsel->autoselWeight [SONGSEL_ATTR_LEVEL] = autoselGetDouble (autosel, AUTOSEL_LEVEL_WEIGHT);
  songsel->autoselWeight [SONGSEL_ATTR_TAGS] = autoselGetDouble (autosel, AUTOSEL_TAG_WEIGHT);
  songsel->lastSelection = NULL;

  nlistStartIterator (dancelist, &iteridx);
  /* for each dance : first set up all the lists */
  while ((danceIdx = nlistIterateKey (dancelist, &iteridx)) >= 0) {
    logMsg (LOG_DBG, LOG_SONGSEL, "adding dance: %d/%s", danceIdx,
        danceGetStr (songsel->dances, danceIdx, DANCE_DANCE));
    songseldance = mdmalloc (sizeof (songseldance_t));
    songseldance->danceIdx = danceIdx;
    songseldance->songIdxList = nlistAlloc ("songsel-songidx",
        LIST_ORDERED, songselSongDataFree);
    songseldance->currentIndexes = queueAlloc ("songsel-curr-idxs", songselIdxFree);
    songseldance->currentIdxList = nlistAlloc ("songsel-curridx",
        LIST_UNORDERED, NULL);
    songseldance->currcount = 0;
    for (int i = 0; i < SONGSEL_ATTR_MAX; ++i) {
      songseldance->origWeights [i] = 0;
      songseldance->weights [i] = 0;
    }
    nlistSetData (songsel->danceSelList, danceIdx, songseldance);
  }

  songsel->musicdb = musicdb;

  logProcEnd (LOG_PROC, "songselAlloc", "");
  return songsel;
}

void
songselFree (songsel_t *songsel)
{
  logProcBegin (LOG_PROC, "songselFree");
  if (songsel == NULL) {
    return;
  }

  nlistFree (songsel->danceSelList);
  mdfree (songsel);
  logProcEnd (LOG_PROC, "songselFree", "");
  return;
}

void
songselSetTags (songsel_t *songsel, slist_t *taglist, int tagweight)
{
  if (songsel == NULL) {
    return;
  }

  songsel->tagList = taglist;
  songsel->tagWeight = tagweight;
}

void
songselInitialize (songsel_t *songsel, nlist_t *songlist, songfilter_t *songfilter)
{
  nlistidx_t      iteridx;
  song_t          *song;
  dbidx_t         dbidx;
  nlistidx_t      dbiteridx;
  songseldance_t  *songseldance;

  /* when songlist is not null, it is a song-list, and all songs should */
  /* be used. */
  if (songlist != NULL) {
    logMsg (LOG_DBG, LOG_SONGSEL, "processing songs from songlist (%d)",
        nlistGetCount (songlist));

    nlistStartIterator (songlist, &iteridx);
    while ((dbidx = nlistIterateKey (songlist, &iteridx)) >= 0) {
      song = dbGetByIdx (songsel->musicdb, dbidx);
      if (song != NULL) {
        songselAllocAddSong (songsel, dbidx, song);
      }
    }
  } else {
    /* for each song in the supplied song list */
    /* check to make sure this is dance that needs to be processed */
    /* apply the song filters */
    /* at the same time, build a list of dbidx's so that the songfilter */
    /* does not have to be checked yet again. */
    dbStartIterator (songsel->musicdb, &dbiteridx);
    while ((song = dbIterate (songsel->musicdb, &dbidx, &dbiteridx)) != NULL) {
      ilistidx_t    danceIdx;

      if (song == NULL) {
        continue;
      }

      /* if the dance is not in the songsel dance list, don't bother */
      /* with it. */
      danceIdx = songGetNum (song, TAG_DANCE);
      songseldance = nlistGetData (songsel->danceSelList, danceIdx);
      if (songseldance == NULL) {
        continue;
      }

      /* check this song with the song filter */
      if (songfilter != NULL &&
          ! songfilterFilterSong (songfilter, song)) {
        continue;
      }

      /* this song is a viable candidate.  Add it. */
      songselAllocAddSong (songsel, dbidx, song);
    }
  }
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
  if (! songsel->processed) {
    /* dance processing is delayed in case the tags get set */
    songselProcessDances (songsel);
  }

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
    logMsg (LOG_DBG, LOG_SONGSEL, "selected idx:%d dbidx:%d from %d/%s",
        songdata->idx, songdata->dbidx, songseldance->danceIdx,
        danceGetStr (songsel->dances, songseldance->danceIdx, DANCE_DANCE));
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
  logMsg (LOG_DBG, LOG_SONGSEL, "finalize: dance-idx: %d", danceIdx);
  songseldance = nlistGetData (songsel->danceSelList, danceIdx);
  if (songseldance == NULL) {
    logProcEnd (LOG_PROC, "songselSelect", "no dance");
    return;
  }

  if (songsel->lastSelection != NULL) {
    songdata = songsel->lastSelection;
    logMsg (LOG_DBG, LOG_SONGSEL, "removing idx:%d dbidx:%d ssidx:%d from %d/%s",
        songdata->idx, songdata->dbidx, songdata->ssidx, songseldance->danceIdx,
        danceGetStr (songsel->dances, songseldance->danceIdx, DANCE_DANCE));
    songselRemoveSong (songsel, songseldance, songdata);
    songsel->lastSelection = NULL;
  }
  logProcEnd (LOG_PROC, "songselSelect", "");
  return;
}

/* internal routines */

static void
songselAllocAddSong (songsel_t *songsel, dbidx_t dbidx, song_t *song)
{
  songselsongdata_t *songdata = NULL;
  ilistidx_t        danceIdx;
  songseldance_t    *songseldance;
  nlistidx_t        rating = 0;
  nlistidx_t        level = 0;
  nlistidx_t        tagidx = 0;
  nlistidx_t        ss = -1;
  int               weight = 0;
  rating_t          *ratings;
  level_t           *levels;


  ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  levels = bdjvarsdfGet (BDJVDF_LEVELS);

  /* the dance and song filter are processed in the main loop */

  songdata = mdmalloc (sizeof (songselsongdata_t));
  for (int i = 0; i < SONGSEL_ATTR_MAX; ++i) {
    songdata->weights [i] = 0;
  }

  danceIdx = songGetNum (song, TAG_DANCE);
  songseldance = nlistGetData (songsel->danceSelList, danceIdx);

  ss = songGetNum (song, TAG_SAMESONG);

  logMsg (LOG_DBG, LOG_SONGSEL, "add-song: dbidx:%d ss:%d", dbidx, ss);

  rating = songGetNum (song, TAG_DANCERATING);
  weight = ratingGetWeight (ratings, rating);
  songdata->weights [SONGSEL_ATTR_RATING] = weight;
  songseldance->origWeights [SONGSEL_ATTR_RATING] += weight;
  logMsg (LOG_DBG, LOG_SONGSEL, "  rating: %d %d", weight, songseldance->origWeights [SONGSEL_ATTR_RATING]);

  level = songGetNum (song, TAG_DANCELEVEL);
  weight = levelGetWeight (levels, level);
  songdata->weights [SONGSEL_ATTR_LEVEL] = weight;
  songseldance->origWeights [SONGSEL_ATTR_LEVEL] += weight;
  logMsg (LOG_DBG, LOG_SONGSEL, "  level: %d %d", weight, songseldance->origWeights [SONGSEL_ATTR_LEVEL]);

  tagidx = 0;
  /* need a non-zero weight here, otherwise the percentage calculations */
  /* will not add up properly. */
  weight = 1;
  if (songsel->tagList != NULL && slistGetCount (songsel->tagList) > 0) {
    slist_t     *songTags;

    songTags = songGetList (song, TAG_TAGS);
    if (songTags != NULL && slistGetCount (songTags) > 0) {
      slistidx_t  titer;
      slistidx_t  siter;
      const char  *tag;
      const char  *songtag;
      const char  *kw;

      slistStartIterator (songsel->tagList, &titer);
      slistStartIterator (songTags, &siter);
      kw = songGetStr (song, TAG_KEYWORD);
      while (tagidx == 0 && (tag = slistIterateKey (songsel->tagList, &titer)) != NULL) {
        /* check the keyword first */
        if (kw != NULL && *kw) {
          if (strcmp (tag, kw) == 0) {
            logMsg (LOG_DBG, LOG_SONGSEL, "  tags: kw: %s", tag);
            tagidx = 1;
          }
        }
        while (tagidx == 0 && (songtag = slistIterateKey (songTags, &siter)) != NULL) {
          if (strcmp (tag, songtag) == 0) {
            logMsg (LOG_DBG, LOG_SONGSEL, "  tags: %s", tag);
            tagidx = 1;
          }
        }
      }
    }

    if (tagidx == 1) {
      weight = songsel->tagWeight;
    }
  }
  songdata->weights [SONGSEL_ATTR_TAGS] = weight;
  songseldance->origWeights [SONGSEL_ATTR_TAGS] += weight;
  logMsg (LOG_DBG, LOG_SONGSEL, "  tags: %d %d", weight, songseldance->origWeights [SONGSEL_ATTR_TAGS]);

  ++songseldance->currcount;

  songdata->dbidx = dbidx;
  songdata->ssidx = ss;
  songdata->percentage = 0.0;
  nlistSetData (songseldance->songIdxList, dbidx, songdata);
}

static void
songselRemoveSong (songsel_t *songsel,
    songseldance_t *songseldance, songselsongdata_t *songdata)
{
  songselidx_t    *songidx = NULL;
  qidx_t          count = 0;
  qidx_t          qiteridx;
  nlistidx_t      ssidx = -1;


  logProcBegin (LOG_PROC, "songselRemoveSong");
  count = queueGetCount (songseldance->currentIndexes);
  logMsg (LOG_DBG, LOG_SONGSEL, "begin-count: %d %d/%s",
      queueGetCount (songseldance->currentIndexes),
      songseldance->danceIdx,
      danceGetStr (songsel->dances, songseldance->danceIdx, DANCE_DANCE));

  /* save the same-song index */
  ssidx = songdata->ssidx;

  if (count > 0) {
    nlist_t   *nlist = NULL;

    nlist = nlistAlloc ("songsel-curridx", LIST_UNORDERED, NULL);
    nlistSetSize (nlist, count - 1);
    queueStartIterator (songseldance->currentIndexes, &qiteridx);
    while ((songidx =
          queueIterateData (songseldance->currentIndexes, &qiteridx)) != NULL) {
      if (songidx->idx == songdata->idx) {
        logMsg (LOG_DBG, LOG_SONGSEL, "  remove idx:%d ssidx:%d",
            songidx->idx, ssidx);
        queueIterateRemoveNode (songseldance->currentIndexes, &qiteridx);
        songselIdxFree (songidx);
        for (int i = 0; i < SONGSEL_ATTR_MAX; ++i) {
          songseldance->weights [i] -= songdata->weights [i];
        }
        continue;
      }
      if (ssidx > 0 && songidx->ssidx == ssidx) {
        songselsongdata_t   *tsongdata;

        /* if the same-song mark was set, remove _all_ songs with the */
        /* matching same-song mark. */
        logMsg (LOG_DBG, LOG_SONGSEL, "  ss: remove idx:%d ssidx:%d",
            songidx->idx, ssidx);
        tsongdata = nlistGetDataByIdx (songseldance->songIdxList, songidx->idx);
        queueIterateRemoveNode (songseldance->currentIndexes, &qiteridx);
        songselIdxFree (songidx);
        for (int i = 0; i < SONGSEL_ATTR_MAX; ++i) {
          songseldance->weights [i] -= tsongdata->weights [i];
        }
        continue;
      }
      // logMsg (LOG_DBG, LOG_SONGSEL, "  remove-keep: idx:%d", songidx->idx);
      nlistSetNum (nlist, songidx->idx, songidx->idx);
    }

    nlistFree (songseldance->currentIdxList);
    songseldance->currentIdxList = nlist;
  }

  count = queueGetCount (songseldance->currentIndexes);
  logMsg (LOG_DBG, LOG_SONGSEL, "  count: %d", count);
  if (count <= 0) {
    songselRebuild (songsel, songseldance);
  }
  songseldance->currcount = count;

  songselCalcPercentages (songsel, songseldance);

  logProcEnd (LOG_PROC, "songselRemoveSong", "");
  return;
}

static void
songselRebuild (songsel_t *songsel, songseldance_t *songseldance)
{
  nlistidx_t        count;
  nlist_t           *nlist;
  songselsongdata_t *songdata;
  nlistidx_t        iteridx;

  logMsg (LOG_DBG, LOG_SONGSEL, "rebuild: %d/%s", songseldance->danceIdx,
      danceGetStr (songsel->dances, songseldance->danceIdx, DANCE_DANCE));
  queueFree (songseldance->currentIndexes);
  songseldance->currentIndexes = queueAlloc ("songsel-curr-idxs", songselIdxFree);
  count = nlistGetCount (songseldance->songIdxList);
  nlist = nlistAlloc ("songsel-curridx", LIST_UNORDERED, NULL);
  nlistSetSize (nlist, count);

  nlistStartIterator (songseldance->songIdxList, &iteridx);
  while ((songdata = nlistIterateValueData (songseldance->songIdxList, &iteridx)) != NULL) {
    songselidx_t      *songidx;

    songidx = mdmalloc (sizeof (songselidx_t));
    songidx->idx = songdata->idx;
    songidx->ssidx = songdata->ssidx;
    queuePush (songseldance->currentIndexes, songidx);
    nlistSetNum (nlist, songdata->idx, songdata->idx);
  }

  nlistFree (songseldance->currentIdxList);
  songseldance->currentIdxList = nlist;
  songseldance->currcount = queueGetCount (songseldance->currentIndexes);

  for (int i = 0; i < SONGSEL_ATTR_MAX; ++i) {
    songseldance->weights [i] = songseldance->origWeights [i];
  }
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
songselCalcPercentages (songsel_t *songsel, songseldance_t *songseldance)
{
  nlist_t             *songIdxList = songseldance->songIdxList;
  nlist_t             *currentIdxList = songseldance->currentIdxList;
  double              wsum = 0.0;
  songselsongdata_t   *songdata;
  songselsongdata_t   *lastsongdata;
  nlistidx_t          dataidx;
  nlistidx_t          iteridx;


  logProcBegin (LOG_PROC, "songselCalcPercentages");
  logMsg (LOG_DBG, LOG_SONGSEL, "calculate running totals");
  logMsg (LOG_DBG, LOG_SONGSEL, "  dance %d/%s : %d songs",
      songseldance->danceIdx,
      danceGetStr (songsel->dances, songseldance->danceIdx, DANCE_DANCE),
      songseldance->currcount);

  lastsongdata = NULL;
  nlistStartIterator (currentIdxList, &iteridx);
  while ((dataidx = nlistIterateValueNum (currentIdxList, &iteridx)) >= 0) {
    songdata = nlistGetDataByIdx (songIdxList, dataidx);

    for (int i = 0; i < SONGSEL_ATTR_MAX; ++i) {
      double      totweight;
      double      tperc;

      totweight = (double) songseldance->weights [i];
      tperc = (double) songdata->weights [i] / totweight *
          songsel->autoselWeight [i];
      wsum += tperc;
    }
    songdata->percentage = wsum;
    lastsongdata = songdata;
    logMsg (LOG_DBG, LOG_SONGSEL, "  %d percentage: %.6f", dataidx, wsum);
  }
  if (lastsongdata != NULL) {
    lastsongdata->percentage = 1.0;
  }

  logProcEnd (LOG_PROC, "songselCalcPercentages", "");
}

static void
songselDanceFree (void *titem)
{
  songseldance_t       *songseldance = titem;

  logProcBegin (LOG_PROC, "songselDanceFree");
  if (songseldance != NULL) {
    nlistFree (songseldance->songIdxList);
    if (songseldance->currentIndexes != NULL) {
      queueFree (songseldance->currentIndexes);
    }
    nlistFree (songseldance->currentIdxList);
    mdfree (songseldance);
  }
  logProcEnd (LOG_PROC, "songselDanceFree", "");
}

static void
songselIdxFree (void *titem)
{
  songselidx_t       *songselidx = titem;

  if (songselidx != NULL) {
    mdfree (songselidx);
  }
}

static void
songselSongDataFree (void *titem)
{
  songselsongdata_t   *songdata = titem;

  if (songdata == NULL) {
    return;
  }

  mdfree (songdata);
}

static void
songselProcessDances (songsel_t *songsel)
{
  nlistidx_t      iteridx;
  songseldance_t  *songseldance;

  /* for each dance */
  logMsg (LOG_DBG, LOG_SONGSEL, "process dances");
  nlistStartIterator (songsel->danceSelList, &iteridx);
  while ((songseldance =
      nlistIterateValueData (songsel->danceSelList, &iteridx)) != NULL) {
    dbidx_t           idx;
    nlistidx_t        siteridx;
    songselidx_t      *songidx = NULL;
    songselsongdata_t *songdata = NULL;

    logMsg (LOG_DBG, LOG_SONGSEL, "process dance: %d/%s count: %d ",
        songseldance->danceIdx,
        danceGetStr (songsel->dances, songseldance->danceIdx, DANCE_DANCE),
        nlistGetCount (songseldance->songIdxList));

    /* for each selected song for that dance */
    logMsg (LOG_DBG, LOG_SONGSEL, "save list indexes");
    idx = 0;
    nlistStartIterator (songseldance->songIdxList, &siteridx);
    while ((songdata =
        nlistIterateValueData (songseldance->songIdxList, &siteridx)) != NULL) {
      /* save the list index */
      songidx = mdmalloc (sizeof (songselidx_t));
      songidx->idx = idx;
      songidx->ssidx = songdata->ssidx;
      songdata->idx = idx;
      logMsg (LOG_DBG, LOG_SONGSEL, "  push song idx: %d", songidx->idx);
      queuePush (songseldance->currentIndexes, songidx);
      nlistSetNum (songseldance->currentIdxList, songidx->idx, songidx->idx);
      ++idx;
    }

    for (int i = 0; i < SONGSEL_ATTR_MAX; ++i) {
      songseldance->weights [i] = songseldance->origWeights [i];
    }

    songselCalcPercentages (songsel, songseldance);
  }

  songsel->processed = true;
}
