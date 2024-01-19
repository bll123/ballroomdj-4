/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "datafile.h"
#include "ilist.h"
#include "istring.h"
#include "level.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"
#include "nlist.h"
#include "pathbld.h"
#include "playlist.h"
#include "rating.h"
#include "slist.h"
#include "song.h"
#include "songfilter.h"
#include "songlist.h"
#include "status.h"
#include "tagdef.h"
#include "tmutil.h"

enum {
  SONG_FILTER_STR,
  SONG_FILTER_ILIST,
  SONG_FILTER_SLIST,
  SONG_FILTER_NUM,
};

typedef struct songfilter {
  char        *sortselection;
  dance_t     *dances;
  ilist_t     *danceList;
  void        *datafilter [SONG_FILTER_MAX];
  nlistidx_t  numfilter [SONG_FILTER_MAX];
  bool        inuse [SONG_FILTER_MAX];
  /* indexed by the sort string; points to the internal index */
  /* this is the full sort string for each song */
  slist_t     *sortList;
  /* indexed by the internal index; points to the database index */
  nlist_t     *indexList;
  /* filter display selection */
  datafile_t  *filterDisplayDf;
  nlist_t     *filterDisplaySel;
  /* sort key */
  nlist_t     *parsed;
  /* change test */
  time_t      changeTime;
} songfilter_t;

/* these are the user configurable filter displays */
datafilekey_t filterdisplaydfkeys [FILTER_DISP_MAX] = {
  { "DANCELEVEL",     FILTER_DISP_DANCELEVEL,      VALUE_NUM, convBoolean, DF_NORM },
  { "FAVORITE",       FILTER_DISP_FAVORITE,        VALUE_NUM, convBoolean, DF_NORM },
  { "GENRE",          FILTER_DISP_GENRE,           VALUE_NUM, convBoolean, DF_NORM },
  { "STATUS",         FILTER_DISP_STATUS,          VALUE_NUM, convBoolean, DF_NORM },
  { "STATUSPLAYABLE", FILTER_DISP_STATUS_PLAYABLE, VALUE_NUM, convBoolean, DF_NORM },
};

static int valueTypeLookup [SONG_FILTER_MAX] = {
  [SONG_FILTER_MPM_HIGH] =          SONG_FILTER_NUM,
  [SONG_FILTER_MPM_LOW] =           SONG_FILTER_NUM,
  [SONG_FILTER_DANCE_LIST] =        SONG_FILTER_ILIST,
  [SONG_FILTER_DANCE_IDX] =         SONG_FILTER_NUM,
  [SONG_FILTER_FAVORITE] =          SONG_FILTER_NUM,
  [SONG_FILTER_GENRE] =             SONG_FILTER_NUM,
  [SONG_FILTER_KEYWORD] =           SONG_FILTER_SLIST,
  [SONG_FILTER_LEVEL_HIGH] =        SONG_FILTER_NUM,
  [SONG_FILTER_LEVEL_LOW] =         SONG_FILTER_NUM,
  [SONG_FILTER_PLAYLIST] =          SONG_FILTER_STR,
  [SONG_FILTER_PL_TYPE] =           SONG_FILTER_NUM,
  [SONG_FILTER_RATING] =            SONG_FILTER_NUM,
  [SONG_FILTER_SEARCH] =            SONG_FILTER_STR,
  [SONG_FILTER_STATUS] =            SONG_FILTER_NUM,
  [SONG_FILTER_STATUS_PLAYABLE] =   SONG_FILTER_NUM,
};

#define SONG_FILTER_SORT_DEFAULT "TITLE"

static void songfilterFreeData (songfilter_t *sf, int i);
static bool songfilterCheckStr (const char *str, char *searchstr);
static void songfilterMakeSortKey (songfilter_t *sf,
    song_t *song, char *sortkey, ssize_t sz);
static nlist_t *songfilterParseSortKey (songfilter_t *sf);
static void songfilterLoadFilterDisplay (songfilter_t *sf);

songfilter_t *
songfilterAlloc (void)
{
  songfilter_t    *sf;

  logProcBegin (LOG_PROC, "songfilterAlloc");

  sf = mdmalloc (sizeof (songfilter_t));

  sf->sortselection = mdstrdup (SONG_FILTER_SORT_DEFAULT);
  sf->parsed = songfilterParseSortKey (sf);
  for (int i = 0; i < SONG_FILTER_MAX; ++i) {
    sf->datafilter [i] = NULL;
    sf->numfilter [i] = 0;
    sf->inuse [i] = false;
  }
  sf->sortList = NULL;
  sf->indexList = NULL;
  sf->filterDisplayDf = NULL;
  sf->filterDisplaySel = NULL;
  songfilterLoadFilterDisplay (sf);
  songfilterReset (sf);
  sf->dances = bdjvarsdfGet (BDJVDF_DANCES);
  sf->danceList = danceGetDanceList (sf->dances);

  logProcEnd (LOG_PROC, "songfilterAlloc", "");
  return sf;
}

void
songfilterFree (songfilter_t *sf)
{
  logProcBegin (LOG_PROC, "songfilterFree");

  if (sf != NULL) {
    songfilterReset (sf);
    datafileFree (sf->filterDisplayDf);
    dataFree (sf->sortselection);
    slistFree (sf->sortList);
    nlistFree (sf->indexList);
    nlistFree (sf->parsed);
    mdfree (sf);
  }
  logProcEnd (LOG_PROC, "songfilterFree", "");
}

void
songfilterSetSort (songfilter_t *sf, const char *sortselection)
{
  logProcBegin (LOG_PROC, "songfilterSetSort");

  dataFree (sf->sortselection);
  sf->sortselection = mdstrdup (sortselection);
  nlistFree (sf->parsed);
  sf->parsed = songfilterParseSortKey (sf);
  logProcEnd (LOG_PROC, "songfilterSetSort", "");
}

void
songfilterReset (songfilter_t *sf)
{
  logProcBegin (LOG_PROC, "songfilterReset");

  for (int i = 0; i < SONG_FILTER_MAX; ++i) {
    songfilterFreeData (sf, i);
  }
  for (int i = 0; i < SONG_FILTER_MAX; ++i) {
    sf->inuse [i] = false;
  }
  logProcEnd (LOG_PROC, "songfilterReset", "");
}

/* checks to see if the song-filter is turned on in the display settings */
bool
songfilterCheckSelection (songfilter_t *sf, int type)
{
  return nlistGetNum (sf->filterDisplaySel, type);
}

void
songfilterClear (songfilter_t *sf, int filterType)
{
  logProcBegin (LOG_PROC, "songfilterClear");

  if (sf == NULL) {
    logProcEnd (LOG_PROC, "songfilterClear", "null");
    return;
  }

  songfilterFreeData (sf, filterType);
  sf->numfilter [filterType] = 0;
  sf->inuse [filterType] = false;
  logProcEnd (LOG_PROC, "songfilterClear", "");
}

bool
songfilterInUse (songfilter_t *sf, int filterType)
{
  logProcBegin (LOG_PROC, "songfilterInUse");

  if (sf == NULL) {
    logProcEnd (LOG_PROC, "songfilterInUse", "null");
    return false;
  }
  logProcEnd (LOG_PROC, "songfilterInUse", "");
  return sf->inuse [filterType];
}

/* currently used for playlists */
/* turns the filter off without resetting the data */
void
songfilterOff (songfilter_t *sf, int filterType)
{

  logProcBegin (LOG_PROC, "songfilterOff");

  if (sf == NULL) {
    logProcEnd (LOG_PROC, "songfilterOff", "null");
    return;
  }
  sf->inuse [filterType] = false;
  logProcEnd (LOG_PROC, "songfilterOff", "");
}

/* currently used for playlists */
void
songfilterOn (songfilter_t *sf, int filterType)
{
  int     valueType;

  logProcBegin (LOG_PROC, "songfilterOn");

  if (sf == NULL) {
    logProcEnd (LOG_PROC, "songfilterOn", "null");
    return;
  }
  valueType = valueTypeLookup [filterType];
  if (valueType == SONG_FILTER_NUM) {
    /* this may not be valid */
    if (sf->numfilter [filterType] >= 0) {
      sf->inuse [filterType] = true;
    }
  } else {
    if (sf->datafilter [filterType] != NULL) {
      sf->inuse [filterType] = true;
    }
  }
  logProcEnd (LOG_PROC, "songfilterOn", "");
}

void
songfilterSetData (songfilter_t *sf, int filterType, void *value)
{
  int     valueType;

  logProcBegin (LOG_PROC, "songfilterSetData");

  if (sf == NULL) {
    logProcEnd (LOG_PROC, "songfilterSetData", "null");
    return;
  }

  valueType = valueTypeLookup [filterType];

  if (valueType == SONG_FILTER_SLIST) {
    if (filterType != SONG_FILTER_KEYWORD) {
      /* songfilter is not the owner of the keyword list */
      slistFree (sf->datafilter [filterType]);
      sf->datafilter [filterType] = NULL;
    }
    sf->datafilter [filterType] = value;
  }
  if (valueType == SONG_FILTER_ILIST) {
    ilistFree (sf->datafilter [filterType]);
    sf->datafilter [filterType] = value;
  }
  if (valueType == SONG_FILTER_STR) {
    dataFree (sf->datafilter [filterType]);
    sf->datafilter [filterType] = mdstrdup (value);
    if (filterType == SONG_FILTER_SEARCH) {
      istringToLower ((char *) sf->datafilter [filterType]);
    }
  }
  sf->inuse [filterType] = true;
  logProcEnd (LOG_PROC, "songfilterSetData", "");
}

void
songfilterSetNum (songfilter_t *sf, int filterType, ssize_t value)
{
  int     valueType;

  logProcBegin (LOG_PROC, "songfilterSetNum");

  if (sf == NULL) {
    logProcEnd (LOG_PROC, "songfilterSetNum", "null");
    return;
  }

  valueType = valueTypeLookup [filterType];

  if (valueType == SONG_FILTER_NUM) {
    sf->numfilter [filterType] = (nlistidx_t) value;
    sf->inuse [filterType] = true;
  }
  logProcEnd (LOG_PROC, "songfilterSetNum", "");
}

void
songfilterDanceSet (songfilter_t *sf, ilistidx_t danceIdx,
    int filterType, ssize_t value)
{
  int         valueType;
  ilist_t     *danceFilterList;

  logProcBegin (LOG_PROC, "songfilterDanceSet");

  if (sf == NULL) {
    return;
  }

  valueType = valueTypeLookup [filterType];
  danceFilterList = sf->datafilter [SONG_FILTER_DANCE_LIST];
  if (danceFilterList == NULL) {
    logProcEnd (LOG_PROC, "songfilterDanceSet", "no-dancelist");
    return;
  }
  if (! ilistExists (danceFilterList, danceIdx)) {
    logProcEnd (LOG_PROC, "songfilterDanceSet", "not-in-list");
    return;
  }

  if (valueType == SONG_FILTER_NUM) {
    ilistSetNum (danceFilterList, danceIdx, filterType, (ssize_t) value);
  }
  sf->inuse [filterType] = true;
  logProcEnd (LOG_PROC, "songfilterDanceSet", "");
}

dbidx_t
songfilterProcess (songfilter_t *sf, musicdb_t *musicdb)
{
  dbidx_t     dbidx;
  slistidx_t  dbiteridx;
  nlistidx_t  idx;
  char        sortkey [1024];
  song_t      *song;
  pltype_t    pltype = PLTYPE_NONE;

  logProcBegin (LOG_PROC, "songfilterProcess");

  if (sf == NULL) {
    logProcEnd (LOG_PROC, "songfilterProcess", "null");
    return 0;
  }
  if (musicdb == NULL) {
    logProcEnd (LOG_PROC, "songfilterProcess", "null-db");
    return 0;
  }

  slistFree (sf->sortList);
  sf->sortList = NULL;
  nlistFree (sf->indexList);
  sf->indexList = NULL;

  idx = 0;
  sf->sortList = slistAlloc ("songfilter-sort-idx", LIST_UNORDERED, NULL);
  sf->indexList = nlistAlloc ("songfilter-num-idx", LIST_UNORDERED, NULL);

  if (sf->inuse [SONG_FILTER_PLAYLIST]) {
    pltype = sf->numfilter [SONG_FILTER_PL_TYPE];
  }

  /* A song list filter overrides any other filter setting */
  /* simply traverse the song list and add those songs. */
  /* Currently it is assumed that the playlist being */
  /* filtered is a song list. */
  /* Sequences and automatic playlists are not supported at this time */
  /* (and would not be handled in this fashion). */
  if (sf->inuse [SONG_FILTER_PLAYLIST] && pltype == PLTYPE_SONGLIST) {
    songlist_t  *sl;
    ilistidx_t  sliter;
    ilistidx_t  slkey;

    sl = songlistLoad (sf->datafilter [SONG_FILTER_PLAYLIST]);
    songlistStartIterator (sl, &sliter);
    while ((slkey = songlistIterate (sl, &sliter)) >= 0) {
      const char  *sfname;

      sfname = songlistGetStr (sl, slkey, SONGLIST_URI);
      song = dbGetByName (musicdb, sfname);

      if (song != NULL) {
        dbidx = songGetNum (song, TAG_DBIDX);
        snprintf (sortkey, sizeof (sortkey), "%08d", idx);
        slistSetNum (sf->sortList, sortkey, idx);
        nlistSetNum (sf->indexList, idx, dbidx);
        ++idx;
      }
    }

    songlistFree (sl);
    logMsg (LOG_DBG, LOG_SONGSEL, "selected: %d songs from playlist", nlistGetCount (sf->indexList));
  }

  if (! sf->inuse [SONG_FILTER_PLAYLIST] || pltype != PLTYPE_SONGLIST) {
    dbStartIterator (musicdb, &dbiteridx);
    while ((song = dbIterate (musicdb, &dbidx, &dbiteridx)) != NULL) {
      if (! songfilterFilterSong (sf, song)) {
        continue;
      }

      dbidx = songGetNum (song, TAG_DBIDX);
      songfilterMakeSortKey (sf, song, sortkey, MAXPATHLEN);
      logMsg (LOG_DBG, LOG_SONGSEL, "%d sortkey: %s", dbidx, sortkey);
      slistSetNum (sf->sortList, sortkey, idx);
      nlistSetNum (sf->indexList, idx, dbidx);
      ++idx;
    }
    logMsg (LOG_DBG, LOG_SONGSEL, "selected: %d songs from db", nlistGetCount (sf->indexList));
  }

  slistSort (sf->sortList);
  nlistSort (sf->indexList);

  logProcEnd (LOG_PROC, "songfilterProcess", "");
  return nlistGetCount (sf->indexList);
}

bool
songfilterFilterSong (songfilter_t *sf, song_t *song)
{
  dbidx_t       dbidx;
  rating_t      *ratings;
  level_t       *levels;

  logProcBegin (LOG_PROC, "songfilterFilterSong");

  if (sf == NULL) {
    logProcEnd (LOG_PROC, "songfilterFilterSong", "null");
    return false;
  }

  ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  levels = bdjvarsdfGet (BDJVDF_LEVELS);

  dbidx = songGetNum (song, TAG_DBIDX);
  logMsg (LOG_DBG, LOG_SONGSEL, "check: %d", dbidx);

  if (sf->inuse [SONG_FILTER_DANCE_IDX]) {
    ilistidx_t    danceIdx;

    /* the dance idx filter is one dance, or all */
    danceIdx = songGetNum (song, TAG_DANCE);
    if (danceIdx != sf->numfilter [SONG_FILTER_DANCE_IDX]) {
      logMsg (LOG_DBG, LOG_SONGSEL, "dance-idx: reject: %d %d", dbidx, danceIdx);
      logProcEnd (LOG_PROC, "songfilterFilterSong", "dance-idx-reject");
      return false;
    } else {
      logMsg (LOG_DBG, LOG_SONGSEL, "dance-idx: ok: %d %d", dbidx, danceIdx);
    }
  }

  /* used by playlist.c */
  if (sf->inuse [SONG_FILTER_DANCE_LIST]) {
    ilistidx_t    danceIdx;
    ilist_t       *danceFilterList;

    /* the dance list filter is a list of dances */
    /* an unknown dance will be rejected */
    danceIdx = songGetNum (song, TAG_DANCE);
    danceFilterList = sf->datafilter [SONG_FILTER_DANCE_LIST];
    if (danceFilterList != NULL && ! ilistExists (danceFilterList, danceIdx)) {
      logMsg (LOG_DBG, LOG_SONGSEL, "dance-list: reject: %d %d", dbidx, danceIdx);
      logProcEnd (LOG_PROC, "songfilterFilterSong", "dance-list-reject");
      return false;
    } else {
      logMsg (LOG_DBG, LOG_SONGSEL, "dance-list: ok: %d %d", dbidx, danceIdx);
    }
  }

  if (sf->inuse [SONG_FILTER_GENRE]) {
    nlistidx_t    genre;

    genre = songGetNum (song, TAG_GENRE);
    if (genre != sf->numfilter [SONG_FILTER_GENRE]) {
      logMsg (LOG_DBG, LOG_SONGSEL, "genre: reject: %d %d != %d", dbidx, genre, sf->numfilter [SONG_FILTER_GENRE]);
      logProcEnd (LOG_PROC, "songfilterFilterSong", "genre-reject");
      return false;
    }
  }

  /* rating checks to make sure the song rating */
  /* is greater or equal to the selected rating */
  /* use this for both for-playback and not-for-playback */
  if (sf->inuse [SONG_FILTER_RATING]) {
    nlistidx_t    rating;
    int           weight;

    rating = songGetNum (song, TAG_DANCERATING);
    if (rating < 0) {
      logMsg (LOG_DBG, LOG_SONGSEL, "rating: reject: %d unknown %d", dbidx, rating);
      logProcEnd (LOG_PROC, "songfilterFilterSong", "rating-reject-unknown");
      return false;
    }
    weight = ratingGetWeight (ratings, rating);
    if (weight == 0) {
      logMsg (LOG_DBG, LOG_SONGSEL, "rating: reject: %d %d weight 0", dbidx, rating);
      logProcEnd (LOG_PROC, "songfilterFilterSong", "rating-reject-weight-0");
      return false;
    }
    if (rating < sf->numfilter [SONG_FILTER_RATING]) {
      logMsg (LOG_DBG, LOG_SONGSEL, "rating: reject: %d %d < %d", dbidx, rating, sf->numfilter [SONG_FILTER_RATING]);
      logProcEnd (LOG_PROC, "songfilterFilterSong", "rating-reject");
      return false;
    }
  }

  if (sf->inuse [SONG_FILTER_LEVEL_LOW] && sf->inuse [SONG_FILTER_LEVEL_HIGH]) {
    nlistidx_t    level;
    int           weight;

    level = songGetNum (song, TAG_DANCELEVEL);
    if (level < 0) {
      logMsg (LOG_DBG, LOG_SONGSEL, "level: reject: %d unknown %d", dbidx, level);
      logProcEnd (LOG_PROC, "songfilterFilterSong", "level-reject-unknown");
      return false;
    }
    weight = levelGetWeight (levels, level);
    if (weight == 0) {
      logMsg (LOG_DBG, LOG_SONGSEL, "level: reject: %d %d weight 0", dbidx, level);
      logProcEnd (LOG_PROC, "songfilterFilterSong", "level-reject-weight-0");
      return false;
    }
    if (level < sf->numfilter [SONG_FILTER_LEVEL_LOW] ||
        level > sf->numfilter [SONG_FILTER_LEVEL_HIGH]) {
      logMsg (LOG_DBG, LOG_SONGSEL, "level: reject: %d %d < %d / > %d", dbidx, level,
          sf->numfilter [SONG_FILTER_LEVEL_LOW], sf->numfilter [SONG_FILTER_LEVEL_HIGH]);
      logProcEnd (LOG_PROC, "songfilterFilterSong", "level-reject");
      return false;
    }
  }

  if (sf->inuse [SONG_FILTER_STATUS]) {
    nlistidx_t      sstatus;

    sstatus = songGetNum (song, TAG_STATUS);
    if (sstatus != sf->numfilter [SONG_FILTER_STATUS]) {
      logMsg (LOG_DBG, LOG_SONGSEL, "status: reject: %d %d != %d", dbidx, sstatus, sf->numfilter [SONG_FILTER_STATUS]);
      logProcEnd (LOG_PROC, "songfilterFilterSong", "status-reject");
      return false;
    }
  }

  if (sf->inuse [SONG_FILTER_FAVORITE]) {
    nlistidx_t      fav;

    fav = songGetNum (song, TAG_FAVORITE);
    if (fav != sf->numfilter [SONG_FILTER_FAVORITE]) {
      logMsg (LOG_DBG, LOG_SONGSEL, "favorite: reject: %d %d != %d", dbidx, fav, sf->numfilter [SONG_FILTER_FAVORITE]);
      logProcEnd (LOG_PROC, "songfilterFilterSong", "favorite-reject");
      return false;
    }
  }

  /* check to make sure the song's status is marked as playable */
  if (sf->inuse [SONG_FILTER_STATUS_PLAYABLE]) {
    status_t      *status;
    listidx_t     sstatus;

    status = bdjvarsdfGet (BDJVDF_STATUS);
    sstatus = songGetNum (song, TAG_STATUS);

    if (sf->numfilter [SONG_FILTER_STATUS_PLAYABLE] == SONG_FILTER_FOR_PLAYBACK &&
        ! statusGetPlayFlag (status, sstatus)) {
      logMsg (LOG_DBG, LOG_SONGSEL, "reject %d status not playable", dbidx);
      logProcEnd (LOG_PROC, "songfilterFilterSong", "playable-reject");
      return false;
    }
  }

  /* used by playlist.c */
  /* note that the dance filter list must also be set */
  if (sf->inuse [SONG_FILTER_MPM_LOW] && sf->inuse [SONG_FILTER_MPM_HIGH]) {
    ilistidx_t    danceIdx;
    int           bpmlow = 0;
    int           bpmhigh = 0;
    ilist_t       *danceFilterList = NULL;


    danceFilterList = sf->datafilter [SONG_FILTER_DANCE_LIST];
    if (danceFilterList != NULL) {
      danceIdx = songGetNum (song, TAG_DANCE);

      bpmlow = ilistGetNum (danceFilterList, danceIdx, SONG_FILTER_MPM_LOW);
      bpmhigh = ilistGetNum (danceFilterList, danceIdx, SONG_FILTER_MPM_HIGH);
    }

    if (bpmlow > 0 && bpmhigh > 0) {
      int       bpm;

      bpm = songGetNum (song, TAG_BPM);
      if (bpm < 0 || bpm < bpmlow || bpm > bpmhigh) {
        logMsg (LOG_DBG, LOG_SONGSEL, "reject %d dance %d bpm %d [%d,%d]", dbidx, danceIdx, bpm, bpmlow, bpmhigh);
        logProcEnd (LOG_PROC, "songfilterFilterSong", "bpm-reject");
        return false;
      }
    }
  }

  /* put the most expensive filters last */

  /* keywords are a bit of a pain to handle */
  /* auto song selection needs to reject any song with a keyword */
  /*  unless it is in the keyword list */
  /*  auto song selection will need to mark the keyword filter as in use */
  /*  and use an empty keyword list if necessary */
  /* song selection display on the other hand, should show everything */
  /* unless it is in playlist mode. */
  if (sf->inuse [SONG_FILTER_KEYWORD]) {
    const char  *keyword;

    keyword = songGetStr (song, TAG_KEYWORD);

    if (keyword != NULL && *keyword) {
      slist_t     *keywordList;
      slistidx_t  idx = -1;  /* automatic rejection */
      int         kwcount;

      keywordList = sf->datafilter [SONG_FILTER_KEYWORD];
      kwcount = slistGetCount (keywordList);

      if (kwcount > 0) {
        idx = slistGetIdx (keywordList, keyword);
        /* if idx >= 0, no rejection */
      }
      if (idx < 0) {
        logMsg (LOG_DBG, LOG_SONGSEL, "keyword: reject: %d %s not in allowed", dbidx, keyword);
        logProcEnd (LOG_PROC, "songfilterFilterSong", "keyword-reject");
        return false;
      }
    }
  }

  if (sf->inuse [SONG_FILTER_SEARCH]) {
    char      *searchstr;
    bool      found;

    found = false;
    searchstr = (char *) sf->datafilter [SONG_FILTER_SEARCH];

    /* put the most likely places first */
    if (! found) {
      found = songfilterCheckStr (songGetStr (song, TAG_TITLE), searchstr);
    }
    if (! found) {
      found = songfilterCheckStr (songGetStr (song, TAG_ARTIST), searchstr);
    }
    if (! found) {
      found = songfilterCheckStr (songGetStr (song, TAG_ALBUMARTIST), searchstr);
    }
    if (! found) {
      found = songfilterCheckStr (songGetStr (song, TAG_NOTES), searchstr);
    }
    if (! found) {
      found = songfilterCheckStr (songGetStr (song, TAG_KEYWORD), searchstr);
    }
    if (! found) {
      slist_t     *tagList;
      slistidx_t  iteridx;
      const char  *tag;

      tagList = (slist_t *) songGetList (song, TAG_TAGS);
      slistStartIterator (tagList, &iteridx);
      while (! found && (tag = slistIterateKey (tagList, &iteridx)) != NULL) {
        found = songfilterCheckStr (tag, searchstr);
      }
    }
    if (! found) {
      found = songfilterCheckStr (songGetStr (song, TAG_ALBUM), searchstr);
    }
    if (! found) {
      found = songfilterCheckStr (songGetStr (song, TAG_COMPOSER), searchstr);
    }
    if (! found) {
      found = songfilterCheckStr (songGetStr (song, TAG_CONDUCTOR), searchstr);
    }
    if (! found) {
      logProcEnd (LOG_PROC, "songfilterFilterSong", "search-reject");
      return false;
    }
  }

  logMsg (LOG_DBG, LOG_SONGSEL, "ok: %d %s", dbidx, songGetStr (song, TAG_TITLE));
  logProcEnd (LOG_PROC, "songfilterFilterSong", "");
  return true;
}

char *
songfilterGetSort (songfilter_t *sf)
{

  logProcBegin (LOG_PROC, "songfilterGetSort");

  if (sf == NULL) {
    logProcEnd (LOG_PROC, "songfilterGetSort", "null");
    return NULL;
  }

  logProcEnd (LOG_PROC, "songfilterGetSort", "");
  return sf->sortselection;
}

dbidx_t
songfilterGetByIdx (songfilter_t *sf, nlistidx_t lookupIdx)
{
  nlistidx_t      internalIdx;
  dbidx_t         dbidx;

  logProcBegin (LOG_PROC, "songfilterGetByIdx");

  if (sf == NULL) {
    logProcEnd (LOG_PROC, "songfilterGetByIdx", "null");
    return -1;
  }
  if (lookupIdx < 0 || lookupIdx >= nlistGetCount (sf->indexList)) {
    logProcEnd (LOG_PROC, "songfilterGetByIdx", "bad-lookup");
    return -1;
  }

  if (sf->sortList != NULL) {
    internalIdx = slistGetNumByIdx (sf->sortList, lookupIdx);
  } else {
    internalIdx = nlistGetNumByIdx (sf->indexList, lookupIdx);
  }
  dbidx = nlistGetNum (sf->indexList, internalIdx);
  logProcEnd (LOG_PROC, "songfilterGetByIdx", "");
  return dbidx;
}

dbidx_t
songfilterGetCount (songfilter_t *sf)
{
  logProcBegin (LOG_PROC, "songfilterGetCount");

  if (sf == NULL) {
    logProcEnd (LOG_PROC, "songfilterGetCount", "null");
    return 0;
  }
  logProcEnd (LOG_PROC, "songfilterGetCount", "");
  return nlistGetCount (sf->indexList);
}


/* internal routines */

static void
songfilterFreeData (songfilter_t *sf, int i)
{
  logProcBegin (LOG_PROC, "songfilterFreeData");

  if (sf->datafilter [i] != NULL) {
    if (valueTypeLookup [i] == SONG_FILTER_STR) {
      mdfree (sf->datafilter [i]);
    }
    if (valueTypeLookup [i] == SONG_FILTER_SLIST) {
      /* songfilter is not the owner of the keyword list */
      if (i != SONG_FILTER_KEYWORD) {
        slistFree (sf->datafilter [i]);
      }
    }
    if (valueTypeLookup [i] == SONG_FILTER_ILIST) {
      ilistFree (sf->datafilter [i]);
    }
  }
  sf->datafilter [i] = NULL;
  logProcEnd (LOG_PROC, "songfilterFreeData", "");
}

static bool
songfilterCheckStr (const char *str, char *searchstr)
{
  bool  found = false;
  char  tbuff [MAXPATHLEN];

  logProcBegin (LOG_PROC, "songfilterCheckStr");

  if (str == NULL || searchstr == NULL) {
    logProcEnd (LOG_PROC, "songfilterCheckStr", "null-str");
    return found;
  }
  if (*searchstr == '\0') {
    logProcEnd (LOG_PROC, "songfilterCheckStr", "no-data");
    return found;
  }

  strlcpy (tbuff, str, sizeof (tbuff));
  istringToLower (tbuff);
  if (strstr (tbuff, searchstr) != NULL) {
    found = true;
  }

  logProcEnd (LOG_PROC, "songfilterCheckStr", "");
  return found;
}

static void
songfilterMakeSortKey (songfilter_t *sf,
    song_t *song, char *sortkey, ssize_t sz)
{
  int         tagkey;
  char        tbuff [100];
  nlistidx_t  iteridx;

  logProcBegin (LOG_PROC, "songfilterMakeSortKey");

  sortkey [0] = '\0';
  if (song == NULL) {
    logProcEnd (LOG_PROC, "songfilterMakeSortKey", "null-song");
    return;
  }

  if (sf->parsed == NULL) {
    logProcEnd (LOG_PROC, "songfilterMakeSortKey", "bad-parse");
    return;
  }

  nlistStartIterator (sf->parsed, &iteridx);
  while ((tagkey = nlistIterateKey (sf->parsed, &iteridx)) >= 0) {
    if (tagkey == TAG_DANCE) {
      ilistidx_t  danceIdx;
      const char  *danceStr = NULL;

      danceIdx = songGetNum (song, tagkey);
      if (danceIdx >= 0) {
        danceStr = danceGetStr (sf->dances, danceIdx, DANCE_DANCE);
      } else {
        danceStr = "";
      }
      snprintf (tbuff, sizeof (tbuff), "/%s", danceStr);
      strlcat (sortkey, tbuff, sz);
    } else if (tagkey == TAG_DANCELEVEL ||
        tagkey == TAG_DANCERATING ||
        tagkey == TAG_GENRE) {
      dbidx_t     idx;

      idx = songGetNum (song, tagkey);
      if (idx < 0) {
        idx = 0;
      }
      snprintf (tbuff, sizeof (tbuff), "/%02d", idx);
      strlcat (sortkey, tbuff, sz);
    } else if (tagkey == TAG_DBADDDATE) {
      const char  *str;
      int         yr, mon, day;

      str = songGetStr (song, tagkey);
      /* unknown date */
      strlcpy (tbuff, "/9999-99-99", sizeof (tbuff));
      if (str != NULL && *str) {
        if (sscanf (str, "%4d-%2d-%2d", &yr, &mon, &day) == 3) {
          snprintf (tbuff, sizeof (tbuff), "/%4d-%2d-%2d",
              9999 - yr, 99 - mon, 99 - day);
        }
      }
      strlcat (sortkey, tbuff, sz);
    } else if (tagkey == TAG_LAST_UPDATED) {
      size_t    tval;

      tval = songGetNum (song, tagkey);
      /* the newest will be the largest number */
      /* reverse sort */
      tval = ~tval;
      snprintf (tbuff, sizeof (tbuff), "/%10zx", tval);
      strlcat (sortkey, tbuff, sz);
    } else if (tagkey == TAG_TITLE ||
        tagkey == TAG_ARTIST ||
        tagkey == TAG_ALBUMARTIST) {
      snprintf (tbuff, sizeof (tbuff), "/%s",
            songGetStr (song, tagkey));
      strlcat (sortkey, tbuff, sz);
    } else if (tagkey == TAG_TRACKNUMBER) {
      dbidx_t   tval;

      tval = songGetNum (song, TAG_DISCNUMBER);
      if (tval == LIST_VALUE_INVALID) {
        tval = 1;
      }
      snprintf (tbuff, sizeof (tbuff), "/%03d", tval);
      strlcat (sortkey, tbuff, sz);

      tval = songGetNum (song, TAG_TRACKNUMBER);
      if (tval == LIST_VALUE_INVALID) {
        tval = 1;
      }
      snprintf (tbuff, sizeof (tbuff), "/%04d", tval);
      strlcat (sortkey, tbuff, sz);
    } else if (tagkey == TAG_ALBUM) {
      dbidx_t     tval;

      snprintf (tbuff, sizeof (tbuff), "/%s",
          songGetStr (song, tagkey));
      strlcat (sortkey, tbuff, sz);

      tval = songGetNum (song, TAG_DISCNUMBER);
      if (tval == LIST_VALUE_INVALID) {
        tval = 1;
      }
      snprintf (tbuff, sizeof (tbuff), "/%03d", tval);
      strlcat (sortkey, tbuff, sz);
    }
  }
  logProcEnd (LOG_PROC, "songfilterMakeSortKey", "");
}

static nlist_t *
songfilterParseSortKey (songfilter_t *sf)
{
  char        *sortsel;
  char        *p;
  char        *tokstr;
  nlist_t     *parsed;
  int         tagkey;

  logProcBegin (LOG_PROC, "songfilterParseSortKey");

  parsed = nlistAlloc ("songfilter-sortkey-parse", LIST_UNORDERED, NULL);

  sortsel = mdstrdup (sf->sortselection);
  p = strtok_r (sortsel, " ", &tokstr);
  while (p != NULL) {
    tagkey = tagdefLookup (p);
    if (tagkey >= 0) {
      nlistSetNum (parsed, tagkey, 0);
      if (tagkey == TAG_DBADDDATE) {
        nlistSetNum (parsed, TAG_TITLE, 0);
      }
    } else {
      logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: invalid sort key: %s", p);
    }
    p = strtok_r (NULL, " ", &tokstr);
  }
  mdfree (sortsel);

  logProcEnd (LOG_PROC, "songfilterParseSortKey", "");
  return parsed;
}

static void
songfilterLoadFilterDisplay (songfilter_t *sf)
{
  char    tbuff [MAXPATHLEN];

  logProcBegin (LOG_PROC, "songfilterLoadFilterDisplay");

  pathbldMakePath (tbuff, sizeof (tbuff),
      "ds-songfilter", BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  sf->filterDisplayDf = datafileAllocParse ("sf-songfilter",
      DFTYPE_KEY_VAL, tbuff, filterdisplaydfkeys, FILTER_DISP_MAX, DF_NO_OFFSET, NULL);
  sf->filterDisplaySel = datafileGetList (sf->filterDisplayDf);
  logProcEnd (LOG_PROC, "songfilterLoadFilterDisplay", "");
}
