/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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
  SONGSEL_SS_DBIDX_LIST,
  SONGSEL_SS_COUNT,
  SONGSEL_SS_DBIDX,
};

typedef struct {
  nlistidx_t     idx;
} songselidx_t;

typedef struct {
  nlistidx_t    idx;
  dbidx_t       dbidx;
  nlistidx_t    rating;
  nlistidx_t    level;
  nlistidx_t    attrIdx [SONGSEL_ATTR_MAX];
  double        percentage;
} songselsongdata_t;

/* used for both ratings and levels */
typedef struct {
  dbidx_t       origCount;      // count of number of songs for this idx
  int           weight;         // weight for this idx
  dbidx_t       count;          // current count of number of songs
  double        calcperc;       // current percentage adjusted by weight
} songselperc_t;

typedef struct {
  nlistidx_t  danceIdx;
  nlist_t     *songIdxList;
  queue_t     *currentIndexes;
  nlist_t     *currentIdxList;
  nlist_t     *attrList [SONGSEL_ATTR_MAX];
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
static songselsongdata_t * searchForPercentage (songseldance_t *songseldance, double dval);
static songselperc_t * songselInitPerc (list_t *plist, nlistidx_t idx, int weight);
static void songselDanceFree (void *titem);
static void songselIdxFree (void *titem);
static void songselSongDataFree (void *titem);
static void songselPercFree (void *titem);
static void songselCalcPercentages (songseldance_t *songseldance);
static void songselCalcAttrPerc (songsel_t *songsel, songseldance_t *songseldance, int attridx);
static void songselProcessPercentages (songsel_t *songsel, songseldance_t *songseldance);
static void songselProcessDances (songsel_t *songsel);

/*
 *  danceSelList:
 *    indexed by the dance index.
 *    contains a 'songseldance_t'; the song selection list for the dance.
 *  song selection dance:
 *    indexed by the dbidx.
 *    contains:
 *      danceidx
 *      song index list
 *      current indexes
 *      current index list
 *      attribute lists (rating, level, tags)
 *        these have the weights by rating/level/tags.
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
  songsel->tagWeight = 5;
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
    songseldance->attrList [SONGSEL_ATTR_RATING] =
        nlistAlloc ("songsel-rating", LIST_ORDERED, songselPercFree);
    songseldance->attrList [SONGSEL_ATTR_LEVEL] =
        nlistAlloc ("songsel-level", LIST_ORDERED, songselPercFree);
    songseldance->attrList [SONGSEL_ATTR_TAGS] =
        nlistAlloc ("songsel-tags", LIST_ORDERED, songselPercFree);
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
    ssize_t   ss;
    ilist_t   *ssmarks;
    nlist_t   *tmpdbidxlist;

    /* for each song in the supplied song list */
    /* check for samesong marks and save the count for that same-song mark. */
    /* also save a list of dbidx for that same-song mark. */
    /* at the same time, build a list of dbidx's so that the songfilter */
    /* does not have to be checked yet again. */
    ssmarks = ilistAlloc ("samesong-temp", LIST_ORDERED);
    /* the temp dbidx list doesn't need to be sorted */
    tmpdbidxlist = nlistAlloc ("dbidx-temp", LIST_UNORDERED, NULL);
    dbStartIterator (songsel->musicdb, &dbiteridx);
    while ((song = dbIterate (songsel->musicdb, &dbidx, &dbiteridx)) != NULL) {
      ilistidx_t    danceIdx;

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

      /* this song is a viable candidate.  Add it to the temp list */
      nlistSetNum (tmpdbidxlist, dbidx, 0);

      ss = songGetNum (song, TAG_SAMESONG);
      if (ss > 0) {
        int     val;
        nlist_t *dbidxlist;

        dbidxlist = ilistGetList (ssmarks, ss, SONGSEL_SS_DBIDX_LIST);
        if (dbidxlist == NULL) {
          dbidxlist = nlistAlloc ("ss-dbidx-list", LIST_ORDERED, NULL);
          ilistSetList (ssmarks, ss, SONGSEL_SS_DBIDX_LIST, dbidxlist);
        }
        nlistSetNum (dbidxlist, dbidx, 0);
        val = ilistGetNum (ssmarks, ss, SONGSEL_SS_COUNT);
        if (val < 0) {
          val = 1;
        } else {
          ++val;
        }
        ilistSetNum (ssmarks, ss, SONGSEL_SS_COUNT, val);
      }
    }

    /* now, for each same-song mark, choose one of the songs from the */
    /* dbidx-list at random */
    ilistStartIterator (ssmarks, &iteridx);
    while ((ss = ilistIterateKey (ssmarks, &iteridx)) >= 0) {
      int     count;
      int     idx;
      nlist_t *dbidxlist;

      count = ilistGetNum (ssmarks, ss, SONGSEL_SS_COUNT);
      /* note that it is quite possible for count to be 1, as */
      /* a song could have been kicked out due to the wrong dance, */
      /* or due to the song filters */
      idx = (int) (dRandom () * (double) count);
      dbidxlist = ilistGetData (ssmarks, ss, SONGSEL_SS_DBIDX_LIST);
      dbidx = nlistGetKeyByIdx (dbidxlist, idx);
      ilistSetNum (ssmarks, ss, SONGSEL_SS_DBIDX, dbidx);
    }

    /* now iterate through the temporary list */
    logMsg (LOG_DBG, LOG_SONGSEL, "processing songs from database");
    nlistStartIterator (tmpdbidxlist, &iteridx);
    while ((dbidx = nlistIterateKey (tmpdbidxlist, &iteridx)) >= 0) {
      song = dbGetByIdx (songsel->musicdb, dbidx);

      if (song == NULL) {
        continue;
      }

      /* and the final filter to skip any songs with same-song marks */
      ss = songGetNum (song, TAG_SAMESONG);
      if (ss > 0) {
        dbidx_t   tdbidx;

        tdbidx = ilistGetNum (ssmarks, ss, SONGSEL_SS_DBIDX);
        if (tdbidx != dbidx) {
          /* skip this same-song, dbidx was not chosen */
          continue;
        }
      }
      songselAllocAddSong (songsel, dbidx, song);
    }

    ilistFree (ssmarks);
    nlistFree (tmpdbidxlist);
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
    logMsg (LOG_DBG, LOG_SONGSEL, "removing idx:%d dbidx:%d from %d/%s",
        songdata->idx, songdata->dbidx, songseldance->danceIdx,
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
  songselperc_t     *perc = NULL;
  songselsongdata_t *songdata = NULL;
  ilistidx_t        danceIdx;
  songseldance_t    *songseldance;
  nlistidx_t        rating = 0;
  nlistidx_t        level = 0;
  nlistidx_t        tagidx = 0;
  int               weight = 0;
  rating_t          *ratings;
  level_t           *levels;
  nlist_t           *attrlist;


  ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  levels = bdjvarsdfGet (BDJVDF_LEVELS);

  /* the dance and song filter are processed in the main loop */

  danceIdx = songGetNum (song, TAG_DANCE);
  songseldance = nlistGetData (songsel->danceSelList, danceIdx);

  rating = songGetNum (song, TAG_DANCERATING);
  weight = ratingGetWeight (ratings, rating);
  attrlist = songseldance->attrList [SONGSEL_ATTR_RATING];
  perc = nlistGetData (attrlist, rating);
  if (perc == NULL) {
    perc = songselInitPerc (attrlist, rating, weight);
  }
  ++perc->origCount;
  ++perc->count;

  level = songGetNum (song, TAG_DANCELEVEL);
  weight = levelGetWeight (levels, level);
  attrlist = songseldance->attrList [SONGSEL_ATTR_LEVEL];
  perc = nlistGetData (attrlist, level);
  if (perc == NULL) {
    perc = songselInitPerc (attrlist, level, weight);
  }
  ++perc->origCount;
  ++perc->count;

  tagidx = 0;
  weight = 0;
  if (songsel->tagList != NULL && slistGetCount (songsel->tagList) > 0) {
    slist_t     *songTags;

    songTags = songGetList (song, TAG_TAGS);
    if (songTags != NULL && slistGetCount (songTags) > 0) {
      slistidx_t  titer;
      slistidx_t  siter;
      const char  *tag;
      const char  *songtag;

      slistStartIterator (songsel->tagList, &titer);
      slistStartIterator (songTags, &siter);
      while (tagidx == 0 && (tag = slistIterateKey (songsel->tagList, &titer)) != NULL) {
        while (tagidx == 0 && (songtag = slistIterateKey (songTags, &siter)) != NULL) {
          logMsg (LOG_DBG, LOG_SONGSEL, "  tags: %s %s", tag, songtag);
          if (strcmp (tag, songtag) == 0) {
            tagidx = 1;
          }
        }
      }
    }

    if (tagidx == 1) {
      weight = songsel->tagWeight;
    }
  }

  attrlist = songseldance->attrList [SONGSEL_ATTR_TAGS];
  perc = nlistGetData (attrlist, tagidx);
  if (perc == NULL) {
    perc = songselInitPerc (attrlist, tagidx, weight);
  }
  ++perc->origCount;
  ++perc->count;

  songdata = mdmalloc (sizeof (songselsongdata_t));
  songdata->dbidx = dbidx;
  songdata->percentage = 0.0;
  songdata->attrIdx [SONGSEL_ATTR_RATING] = rating;
  songdata->attrIdx [SONGSEL_ATTR_LEVEL] = level;
  songdata->attrIdx [SONGSEL_ATTR_TAGS] = tagidx;
  nlistSetData (songseldance->songIdxList, dbidx, songdata);
}

static void
songselRemoveSong (songsel_t *songsel,
    songseldance_t *songseldance, songselsongdata_t *songdata)
{
  songselidx_t    *songidx = NULL;
  list_t          *nlist = NULL;
  qidx_t          count = 0;
  nlistidx_t      iteridx;
  qidx_t          qiteridx;


  logProcBegin (LOG_PROC, "songselRemoveSong");
  count = queueGetCount (songseldance->currentIndexes);
  nlist = nlistAlloc ("songsel-curridx", LIST_UNORDERED, NULL);
  if (count == 1) {
    /* need a complete rebuild */
    queueFree (songseldance->currentIndexes);
    songseldance->currentIndexes = queueAlloc ("songsel-curr-idxs", songselIdxFree);
    count = nlistGetCount (songseldance->songIdxList);
    nlistSetSize (nlist, count);
    nlistStartIterator (songseldance->songIdxList, &iteridx);
    while ((songdata = nlistIterateValueData (songseldance->songIdxList, &iteridx)) != NULL) {
      songidx = mdmalloc (sizeof (songselidx_t));
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

  songselProcessPercentages (songsel, songseldance);

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
songselCalcPercentages (songseldance_t *songseldance)
{
  list_t              *songIdxList = songseldance->songIdxList;
  list_t              *currentIdxList = songseldance->currentIdxList;
  songselperc_t       *perc = NULL;
  double              wsum = 0.0;
  songselsongdata_t   *songdata;
  songselsongdata_t   *lastsongdata;
  nlistidx_t           dataidx;
  nlistidx_t          iteridx;


  logProcBegin (LOG_PROC, "songselCalcPercentages");
  lastsongdata = NULL;
  nlistStartIterator (currentIdxList, &iteridx);
  while ((dataidx = nlistIterateValueNum (currentIdxList, &iteridx)) >= 0) {
    songdata = nlistGetDataByIdx (songIdxList, dataidx);
    for (int i = 0; i < SONGSEL_ATTR_MAX; ++i) {
      perc = nlistGetData (songseldance->attrList [i], songdata->attrIdx [i]);
      if (perc != NULL) {
        wsum += perc->calcperc;
      }
    }
    songdata->percentage = wsum;
    lastsongdata = songdata;
    logMsg (LOG_DBG, LOG_SONGSEL, "%d percentage: %.6f", dataidx, wsum);
  }
  if (lastsongdata != NULL) {
    lastsongdata->percentage = 1.0;
  }

  logProcEnd (LOG_PROC, "songselCalcPercentages", "");
}

static void
songselCalcAttrPerc (songsel_t *songsel, songseldance_t *songseldance,
    int attridx)
{
  songselperc_t       *perc = NULL;
  double              tot = 0.0;
  list_t              *list = songseldance->attrList [attridx];
  nlistidx_t          iteridx;


  logProcBegin (LOG_PROC, "songselCalcAttrPerc");
  tot = 0.0;
  nlistStartIterator (list, &iteridx);
  while ((perc = nlistIterateValueData (list, &iteridx)) != NULL) {
    tot += (double) perc->weight;
    logMsg (LOG_DBG, LOG_SONGSEL, "weight: %d tot: %.6f", perc->weight, tot);
  }

  nlistStartIterator (list, &iteridx);
  while ((perc = nlistIterateValueData (list, &iteridx)) != NULL) {
    if (perc->count == 0 || tot == 0.0) {
      perc->calcperc = 0.0;
    } else {
      perc->calcperc = (double) perc->weight / tot /
          (double) perc->count * songsel->autoselWeight [attridx];
    }
    logMsg (LOG_DBG, LOG_SONGSEL, "calcperc: %.6f", perc->calcperc);
  }
  logProcEnd (LOG_PROC, "songselCalcAttrPerc", "");
}

static songselperc_t *
songselInitPerc (nlist_t *attrlist, nlistidx_t idx, int weight)
{
  songselperc_t       *perc;

  logProcBegin (LOG_PROC, "songselInitPerc");
  perc = mdmalloc (sizeof (songselperc_t));
  perc->origCount = 0;
  perc->weight = weight;
  perc->count = 0;
  perc->calcperc = 0.0;
  logMsg (LOG_DBG, LOG_SONGSEL, "  init attr %d %d", idx, weight);
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
    nlistFree (songseldance->songIdxList);
    if (songseldance->currentIndexes != NULL) {
      queueFree (songseldance->currentIndexes);
    }
    nlistFree (songseldance->currentIdxList);
    for (int i = 0; i < SONGSEL_ATTR_MAX; ++i) {
      nlistFree (songseldance->attrList [i]);
    }
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
  songselidx_t       *songselidx = titem;

  if (songselidx != NULL) {
    mdfree (songselidx);
  }
}

static void
songselPercFree (void *titem)
{
  songselperc_t       *songselperc = titem;

  if (songselperc != NULL) {
    mdfree (songselperc);
  }
}

static void
songselProcessPercentages (songsel_t *songsel, songseldance_t *songseldance)
{
  logMsg (LOG_DBG, LOG_SONGSEL, "rating: songselCalcAttrPerc");
  songselCalcAttrPerc (songsel, songseldance, SONGSEL_ATTR_RATING);
  logMsg (LOG_DBG, LOG_SONGSEL, "level: songselCalcAttrPerc");
  songselCalcAttrPerc (songsel, songseldance, SONGSEL_ATTR_LEVEL);
  logMsg (LOG_DBG, LOG_SONGSEL, "tags: songselCalcAttrPerc");
  songselCalcAttrPerc (songsel, songseldance, SONGSEL_ATTR_TAGS);

  logMsg (LOG_DBG, LOG_SONGSEL, "calculate running totals");
  songselCalcPercentages (songseldance);

  logMsg (LOG_DBG, LOG_SONGSEL, "dance %d/%s : %d songs",
      songseldance->danceIdx,
      danceGetStr (songsel->dances, songseldance->danceIdx, DANCE_DANCE),
      nlistGetCount (songseldance->songIdxList));
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
    /* the rating/level attribute indexes must be set afterwards */
    /* the indexes may only be determined after the list has been built */
    logMsg (LOG_DBG, LOG_SONGSEL, "save list indexes.");
    idx = 0;
    nlistStartIterator (songseldance->songIdxList, &siteridx);
    while ((songdata =
        nlistIterateValueData (songseldance->songIdxList, &siteridx)) != NULL) {
      /* save the list index */
      songidx = mdmalloc (sizeof (songselidx_t));
      songidx->idx = idx;
      songdata->idx = idx;
      logMsg (LOG_DBG, LOG_SONGSEL, "  push song idx: %d", songidx->idx);
      queuePush (songseldance->currentIndexes, songidx);
      nlistSetNum (songseldance->currentIdxList, songidx->idx, songidx->idx);
      ++idx;
    }

    songselProcessPercentages (songsel, songseldance);
  }

  songsel->processed = true;
}
