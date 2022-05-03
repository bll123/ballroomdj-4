#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdj4.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "ilist.h"
#include "level.h"
#include "log.h"
#include "musicdb.h"
#include "nlist.h"
#include "rating.h"
#include "slist.h"
#include "song.h"
#include "songfilter.h"
#include "status.h"
#include "tagdef.h"

enum {
  SONG_FILTER_STR,
  SONG_FILTER_ILIST,
  SONG_FILTER_SLIST,
  SONG_FILTER_NUM,
};

typedef struct songfilter {
  char        *sortselection;
  void        *datafilter [SONG_FILTER_MAX];
  nlistidx_t  numfilter [SONG_FILTER_MAX];
  bool        inuse [SONG_FILTER_MAX];
  /* indexed by the sort string; points to the internal index */
  slist_t     *sortList;
  /* indexed by the internal index; points to the database index */
  nlist_t     *indexList;
} songfilter_t;

typedef struct {
  int     filterType;
  int     valueType;
} songfilteridx_t;

/* must be in the same order as defined in songfilter.h */
songfilteridx_t valueTypeLookup [SONG_FILTER_MAX] = {
  { SONG_FILTER_BPM_HIGH,   SONG_FILTER_NUM },
  { SONG_FILTER_BPM_LOW,    SONG_FILTER_NUM },
  { SONG_FILTER_DANCE,      SONG_FILTER_ILIST },
  { SONG_FILTER_FAVORITE,   SONG_FILTER_NUM },
  { SONG_FILTER_GENRE,      SONG_FILTER_NUM },
  { SONG_FILTER_KEYWORD,    SONG_FILTER_SLIST },
  { SONG_FILTER_LEVEL_HIGH, SONG_FILTER_NUM },
  { SONG_FILTER_LEVEL_LOW,  SONG_FILTER_NUM },
  { SONG_FILTER_RATING,     SONG_FILTER_NUM },
  { SONG_FILTER_SEARCH,     SONG_FILTER_STR },
  { SONG_FILTER_STATUS,     SONG_FILTER_NUM },
  { SONG_FILTER_STATUS_PLAYABLE, SONG_FILTER_NUM },
};

#define SONG_FILTER_SORT_UNSORTED "UNSORTED"

static void songfilterFreeData (songfilter_t *sf, int i);
static bool songfilterCheckStr (char *str, char *searchstr);
static void songfilterMakeSortKey (songfilter_t *sf, slist_t *songselParsed,
    song_t *song, char *sortkey, ssize_t sz);
static slist_t *songfilterParseSortKey (songfilter_t *sf);

songfilter_t *
songfilterAlloc (void)
{
  songfilter_t    *sf;

  sf = malloc (sizeof (songfilter_t));
  assert (sf != NULL);

  sf->sortselection = SONG_FILTER_SORT_UNSORTED;
  for (int i = 0; i < SONG_FILTER_MAX; ++i) {
    sf->datafilter [i] = NULL;
    sf->numfilter [i] = 0;
    sf->inuse [i] = false;
  }
  sf->indexList = NULL;
  sf->sortList = NULL;
  songfilterReset (sf);

  return sf;
}

void
songfilterFree (songfilter_t *sf)
{
  if (sf != NULL) {
    songfilterReset (sf);
    if (strcmp (sf->sortselection, SONG_FILTER_SORT_UNSORTED) != 0) {
      free (sf->sortselection);
    }
    if (sf->sortList != NULL) {
      slistFree (sf->sortList);
    }
    if (sf->indexList != NULL) {
      nlistFree (sf->indexList);
    }
    free (sf);
  }
}

void
songfilterSetSort (songfilter_t *sf, char *sortselection)
{
  sf->sortselection = strdup (sortselection);
}

void
songfilterReset (songfilter_t *sf)
{
  logMsg (LOG_DBG, LOG_SONGSEL, "songfilter: reset filters");
  for (int i = 0; i < SONG_FILTER_MAX; ++i) {
    songfilterFreeData (sf, i);
  }
  for (int i = 0; i < SONG_FILTER_MAX; ++i) {
    sf->inuse [i] = false;
  }
}

void
songfilterClear (songfilter_t *sf, int filterType)
{
  if (sf == NULL) {
    return;
  }

  songfilterFreeData (sf, filterType);
  sf->numfilter [filterType] = 0;
  sf->inuse [filterType] = false;
}

void
songfilterSetData (songfilter_t *sf, int filterType, void *value)
{
  int     valueType;

  if (sf == NULL) {
    return;
  }

  valueType = valueTypeLookup [filterType].valueType;

  if (valueType == SONG_FILTER_SLIST) {
    if (sf->datafilter [filterType] != NULL) {
      slistFree (sf->datafilter [filterType]);
      sf->datafilter [filterType] = NULL;
    }
    sf->datafilter [filterType] = value;
  }
  if (valueType == SONG_FILTER_ILIST) {
    if (sf->datafilter [filterType] != NULL) {
      ilistFree (sf->datafilter [filterType]);
      sf->datafilter [filterType] = NULL;
    }
    sf->datafilter [filterType] = value;
  }
  if (valueType == SONG_FILTER_STR) {
    if (sf->datafilter [filterType] != NULL) {
      free (sf->datafilter [filterType]);
      sf->datafilter [filterType] = NULL;
    }
    sf->datafilter [filterType] = strdup (value);
    if (filterType == SONG_FILTER_SEARCH) {
      stringToLower ((char *) sf->datafilter [filterType]);
    }
  }
  sf->inuse [filterType] = true;
}

void
songfilterSetNum (songfilter_t *sf, int filterType, ssize_t value)
{
  int     valueType;

  if (sf == NULL) {
    return;
  }

  valueType = valueTypeLookup [filterType].valueType;

  if (valueType == SONG_FILTER_NUM) {
    sf->numfilter [filterType] = (nlistidx_t) value;
  }
  sf->inuse [filterType] = true;
}

void
songfilterDanceSet (songfilter_t *sf, ilistidx_t danceIdx,
    int filterType, ssize_t value)
{
  int         valueType;
  ilist_t     *danceList;

  if (sf == NULL) {
    return;
  }

  valueType = valueTypeLookup [filterType].valueType;
  danceList = sf->datafilter [SONG_FILTER_DANCE];
  if (danceList == NULL) {
    return;
  }
  if (! ilistExists (danceList, danceIdx)) {
    return;
  }

  if (valueType == SONG_FILTER_NUM) {
    ilistSetNum (danceList, danceIdx, filterType, (ssize_t) value);
  }
  sf->inuse [filterType] = true;
}

ssize_t
songfilterProcess (songfilter_t *sf, musicdb_t *musicdb)
{
  dbidx_t     dbidx;
  slistidx_t  dbiteridx;
  nlistidx_t  idx;
  char        sortkey [MAXPATHLEN];
  song_t      *song;
  slist_t     *sortselParsed;

  if (sf == NULL) {
    return 0;
  }

  if (sf->sortList != NULL) {
    slistFree (sf->sortList);
    sf->sortList = NULL;
  }
  if (sf->indexList != NULL) {
    nlistFree (sf->indexList);
    sf->indexList = NULL;
  }

  sortselParsed = songfilterParseSortKey (sf);

  idx = 0;
  if (strcmp (sf->sortselection, SONG_FILTER_SORT_UNSORTED) != 0) {
    sf->sortList = slistAlloc ("songfilter-sort-idx", LIST_UNORDERED, NULL);
  }
  sf->indexList = nlistAlloc ("songfilter-num-idx", LIST_UNORDERED, NULL);
  dbStartIterator (musicdb, &dbiteridx);
  while ((song = dbIterate (musicdb, &dbidx, &dbiteridx)) != NULL) {
    if (! songfilterFilterSong (sf, song)) {
      continue;
    }

    dbidx = songGetNum (song, TAG_DBIDX);
    if (strcmp (sf->sortselection, SONG_FILTER_SORT_UNSORTED) != 0) {
      songfilterMakeSortKey (sf, sortselParsed, song, sortkey, MAXPATHLEN);
      slistSetNum (sf->sortList, sortkey, idx);
    }
    nlistSetNum (sf->indexList, idx, dbidx);
    ++idx;
  }
  if (strcmp (sf->sortselection, SONG_FILTER_SORT_UNSORTED) != 0) {
    assert (nlistGetCount (sf->indexList) == slistGetCount (sf->sortList));
  }
  logMsg (LOG_DBG, LOG_SONGSEL, "selected: %zd songs from db", nlistGetCount (sf->indexList));

  if (strcmp (sf->sortselection, SONG_FILTER_SORT_UNSORTED) != 0) {
    slistSort (sf->sortList);
  }
  nlistSort (sf->indexList);
  slistFree (sortselParsed);

  return nlistGetCount (sf->indexList);
}

bool
songfilterFilterSong (songfilter_t *sf, song_t *song)
{
  dbidx_t       dbidx;

  if (sf == NULL) {
    return false;
  }

  dbidx = songGetNum (song, TAG_DBIDX);

  if (sf->inuse [SONG_FILTER_DANCE]) {
    ilistidx_t    danceIdx;
    ilist_t       *danceList;

    danceIdx = songGetNum (song, TAG_DANCE);
    danceList = sf->datafilter [SONG_FILTER_DANCE];
    if (danceList != NULL && ! ilistExists (danceList, danceIdx)) {
      logMsg (LOG_DBG, LOG_SONGSEL, "reject: %zd dance %ld", dbidx, danceIdx);
      return false;
    }
  }

  if (sf->inuse [SONG_FILTER_GENRE]) {
    nlistidx_t    genre;

    genre = songGetNum (song, TAG_GENRE);
    if (genre != sf->numfilter [SONG_FILTER_GENRE]) {
      logMsg (LOG_DBG, LOG_SONGSEL, "reject: %zd genre %ld != %ld", dbidx, genre, sf->numfilter [SONG_FILTER_GENRE]);
      return false;
    }
  }

  /* rating checks to make sure the song rating */
  /* is greater or equal to the selected rating */
  /* use this for both for-playback and not-for-playback */
  if (sf->inuse [SONG_FILTER_RATING]) {
    nlistidx_t    rating;

    rating = songGetNum (song, TAG_DANCERATING);
    if (rating < sf->numfilter [SONG_FILTER_RATING]) {
      logMsg (LOG_DBG, LOG_SONGSEL, "reject: %zd rating %ld < %ld", dbidx, rating, sf->numfilter [SONG_FILTER_RATING]);
      return false;
    }
  }

  if (sf->inuse [SONG_FILTER_LEVEL_LOW] && sf->inuse [SONG_FILTER_LEVEL_HIGH]) {
    nlistidx_t    level;

    level = songGetNum (song, TAG_DANCELEVEL);
    if (level < sf->numfilter [SONG_FILTER_LEVEL_LOW] ||
        level > sf->numfilter [SONG_FILTER_LEVEL_HIGH]) {
      logMsg (LOG_DBG, LOG_SONGSEL, "reject: %zd level %ld < %ld / > %ld", dbidx, level,
          sf->numfilter [SONG_FILTER_LEVEL_LOW], sf->numfilter [SONG_FILTER_LEVEL_HIGH]);
      return false;
    }
  }

  if (sf->inuse [SONG_FILTER_STATUS]) {
    nlistidx_t      sstatus;

    sstatus = songGetNum (song, TAG_STATUS);
    if (sstatus != sf->numfilter [SONG_FILTER_STATUS]) {
      logMsg (LOG_DBG, LOG_SONGSEL, "reject: %zd status %ld != %ld", dbidx, sstatus, sf->numfilter [SONG_FILTER_STATUS]);
      return false;
    }
  }

  if (sf->inuse [SONG_FILTER_FAVORITE]) {
    nlistidx_t      fav;

    fav = songGetNum (song, TAG_FAVORITE);
    if (fav != sf->numfilter [SONG_FILTER_FAVORITE]) {
      logMsg (LOG_DBG, LOG_SONGSEL, "reject: %zd favorite %ld != %ld", dbidx, fav, sf->numfilter [SONG_FILTER_FAVORITE]);
      return false;
    }
  }

  /* if the filter is for playback, check to make sure the song's status */
  /* is marked as playable */
  if (sf->inuse [SONG_FILTER_STATUS_PLAYABLE]) {
    status_t      *status;
    listidx_t     sstatus;

    status = bdjvarsdfGet (BDJVDF_STATUS);

    sstatus = songGetNum (song, TAG_STATUS);
    if (status != NULL &&
        ! statusGetPlayFlag (status, sstatus)) {
      logMsg (LOG_DBG, LOG_SONGSEL, "reject %zd status not playable", dbidx);
      return false;
    }
  }

  if (sf->inuse [SONG_FILTER_BPM_LOW] && sf->inuse [SONG_FILTER_BPM_HIGH]) {
    ilistidx_t    danceIdx;
    ssize_t       bpmlow;
    ssize_t       bpmhigh;
    ilist_t       *danceList;


    danceIdx = songGetNum (song, TAG_DANCE);
    danceList = sf->datafilter [SONG_FILTER_DANCE];

    bpmlow = ilistGetNum (danceList, danceIdx, SONG_FILTER_BPM_LOW);
    bpmhigh = ilistGetNum (danceList, danceIdx, SONG_FILTER_BPM_HIGH);

    if (bpmlow > 0 && bpmhigh > 0) {
      ssize_t     bpm;

      bpm = songGetNum (song, TAG_BPM);
      if (bpm < 0 || bpm < bpmlow || bpm > bpmhigh) {
        logMsg (LOG_DBG, LOG_SONGSEL, "reject %zd dance %zd bpm %zd [%zd,%zd]", dbidx, danceIdx, bpm, bpmlow, bpmhigh);
        return false;
      }
    }
  }

  /* put the most expensive filters last */

  if (sf->inuse [SONG_FILTER_KEYWORD]) {
    char    *keyword;

    keyword = songGetStr (song, TAG_KEYWORD);

    if (keyword != NULL && *keyword) {
      slist_t        *keywordList;
      slistidx_t     idx;

      keywordList = sf->datafilter [SONG_FILTER_KEYWORD];

      idx = slistGetIdx (keywordList, keyword);
      if (slistGetCount (keywordList) > 0 && idx < 0) {
        logMsg (LOG_DBG, LOG_SONGSEL, "reject: %zd keyword %s not in allowed", dbidx, keyword);
        return false;
      }
    }
  }

  if (sf->inuse [SONG_FILTER_SEARCH]) {
    char      *searchstr;
    bool      found;
    dance_t   *dances;

    dances = bdjvarsdfGet (BDJVDF_DANCES);

    found = false;
    searchstr = (char *) sf->datafilter [SONG_FILTER_SEARCH];

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
      char        *dancestr;
      ilistidx_t  idx;

      idx = songGetNum (song, TAG_DANCE);
      dancestr = danceGetStr (dances, idx, DANCE_DANCE);
      found = songfilterCheckStr (dancestr, searchstr);
    }
    if (! found) {
      found = songfilterCheckStr (songGetStr (song, TAG_KEYWORD), searchstr);
    }
    if (! found) {
      slist_t     *tagList;
      slistidx_t  iteridx;
      char        *tag;

      tagList = (slist_t *) songGetList (song, TAG_TAGS);
      slistStartIterator (tagList, &iteridx);
      while (! found && (tag = slistIterateKey (tagList, &iteridx)) != NULL) {
        found = songfilterCheckStr (tag, searchstr);
      }
    }
    if (! found) {
      found = songfilterCheckStr (songGetStr (song, TAG_NOTES), searchstr);
    }
    if (! found) {
      found = songfilterCheckStr (songGetStr (song, TAG_ALBUM), searchstr);
    }
    if (! found) {
      return false;
    }
  }

  logMsg (LOG_DBG, LOG_SONGSEL, "ok: %zd %s", dbidx, songGetStr (song, TAG_TITLE));
  return true;
}

char *
songfilterGetSort (songfilter_t *sf)
{
  if (sf == NULL) {
    return NULL;
  }

  return sf->sortselection;
}

ssize_t
songfilterGetNum (songfilter_t *sf, int filterType)
{
  if (sf == NULL) {
    return -1;
  }

  if (! sf->inuse [filterType]) {
    return -1;
  }

  return sf->numfilter [filterType];
}


dbidx_t
songfilterGetByIdx (songfilter_t *sf, nlistidx_t lookupIdx)
{
  nlistidx_t      internalIdx;
  dbidx_t         dbidx;

  if (sf == NULL) {
    return -1;
  }
  if (lookupIdx < 0 || lookupIdx >= nlistGetCount (sf->indexList)) {
    return -1;
  }

  if (sf->sortList != NULL) {
    internalIdx = slistGetNumByIdx (sf->sortList, lookupIdx);
  } else {
    internalIdx = nlistGetNumByIdx (sf->indexList, lookupIdx);
  }
  dbidx = nlistGetNum (sf->indexList, internalIdx);
  return dbidx;
}

/* internal routines */

static void
songfilterFreeData (songfilter_t *sf, int i)
{
  if (sf->datafilter [i] != NULL) {
    if (valueTypeLookup [i].valueType == SONG_FILTER_STR) {
      free (sf->datafilter [i]);
    }
    if (valueTypeLookup [i].valueType == SONG_FILTER_SLIST) {
      slistFree (sf->datafilter [i]);
    }
    if (valueTypeLookup [i].valueType == SONG_FILTER_ILIST) {
      ilistFree (sf->datafilter [i]);
    }
  }
  sf->datafilter [i] = NULL;
}

static bool
songfilterCheckStr (char *str, char *searchstr)
{
  bool  found = false;
  char  tbuff [MAXPATHLEN];

  if (str == NULL) {
    return found;
  }

  strlcpy (tbuff, str, sizeof (tbuff));
  stringToLower (tbuff);
  if (strstr (tbuff, searchstr) != NULL) {
    found = true;
  }

  return found;
}

static void
songfilterMakeSortKey (songfilter_t *sf, slist_t *sortselParsed,
    song_t *song, char *sortkey, ssize_t sz)
{
  char        *p;
  char        tbuff [100];
  dance_t     *dances;
  slistidx_t  iteridx;

  dances = bdjvarsdfGet (BDJVDF_DANCES);

  sortkey [0] = '\0';

  slistStartIterator (sortselParsed, &iteridx);
  while ((p = slistIterateKey (sortselParsed, &iteridx)) != NULL) {
    if (strcmp (p, "TITLE") == 0) {
      snprintf (tbuff, sizeof (tbuff), "/%s",
          songGetStr (song, TAG_TITLE));
      strlcat (sortkey, tbuff, sz);
    } else if (strcmp (p, "DANCE") == 0) {
      ilistidx_t  danceIdx;
      char        *danceStr = NULL;

      danceIdx = songGetNum (song, TAG_DANCE);
      if (danceIdx >= 0) {
        danceStr = danceGetStr (dances, danceIdx, DANCE_DANCE);
      } else {
        danceStr = "";
      }
      snprintf (tbuff, sizeof (tbuff), "/%s", danceStr);
      strlcat (sortkey, tbuff, sz);
    } else if (strcmp (p, "DANCELEVEL") == 0) {
      snprintf (tbuff, sizeof (tbuff), "/%02zd",
          songGetNum (song, TAG_DANCELEVEL));
      strlcat (sortkey, tbuff, sz);
    } else if (strcmp (p, "DANCERATING") == 0) {
      snprintf (tbuff, sizeof (tbuff), "/%02zd",
          songGetNum (song, TAG_DANCERATING));
      strlcat (sortkey, tbuff, sz);
    } else if (strcmp (p, "DATEADDED") == 0) {
      size_t    tval;

      tval = songGetNum (song, TAG_DBADDDATE);
      /* the newest will be the largest number */
      /* reverse sort */
      tval = ~tval;
      snprintf (tbuff, sizeof (tbuff), "/%10zx", tval);
      strlcat (sortkey, tbuff, sz);
    } else if (strcmp (p, "UPDATETIME") == 0) {
      size_t    tval;

      tval = songGetNum (song, TAG_DBADDDATE);
      /* the newest will be the largest number */
      /* reverse sort */
      tval = ~tval;
      snprintf (tbuff, sizeof (tbuff), "/%10zx", tval);
      strlcat (sortkey, tbuff, sz);
    } else if (strcmp (p, "GENRE") == 0) {
      ssize_t     idx;

      idx = songGetNum (song, TAG_GENRE);
      if (idx < 0) {
        idx = 0;
      }
      snprintf (tbuff, sizeof (tbuff), "/%02zd", idx);
      strlcat (sortkey, tbuff, sz);
    } else if (strcmp (p, "ARTIST") == 0) {
      snprintf (tbuff, sizeof (tbuff), "/%s",
            songGetStr (song, TAG_ARTIST));
      strlcat (sortkey, tbuff, sz);
    } else if (strcmp (p, "TRACK") == 0) {
      ssize_t   tval;

      tval = songGetNum (song, TAG_DISCNUMBER);
      if (tval == LIST_VALUE_INVALID) {
        tval = 0;
      }
      snprintf (tbuff, sizeof (tbuff), "/%02zd", tval);
      strlcat (sortkey, tbuff, sz);

      tval = songGetNum (song, TAG_TRACKNUMBER);
      if (tval == LIST_VALUE_INVALID) {
        tval = 0;
      }
      snprintf (tbuff, sizeof (tbuff), "/%04zd", tval);
      strlcat (sortkey, tbuff, sz);
    } else if (strcmp (p, "ALBUMARTIST") == 0) {
      snprintf (tbuff, sizeof (tbuff), "/%s",
          songGetStr (song, TAG_ALBUMARTIST));
      strlcat (sortkey, tbuff, sz);
    } else if (strcmp (p, "ALBUM") == 0) {
      ssize_t     tval;

      snprintf (tbuff, sizeof (tbuff), "/%s",
          songGetStr (song, TAG_ALBUM));
      strlcat (sortkey, tbuff, sz);

      tval = songGetNum (song, TAG_DISCNUMBER);
      if (tval == LIST_VALUE_INVALID) {
        tval = 0;
      }
      snprintf (tbuff, sizeof (tbuff), "/%03zd", tval);
      strlcat (sortkey, tbuff, sz);
    }
  }
}

static slist_t *
songfilterParseSortKey (songfilter_t *sf)
{
  char    *sortsel;
  char    *p;
  char    *tokstr;
  slist_t *parsed;

  parsed = slistAlloc ("songfilter-sortkey-parse", LIST_UNORDERED, NULL);

  sortsel = strdup (sf->sortselection);
  p = strtok_r (sortsel, " ", &tokstr);
  while (p != NULL) {
    slistSetNum (parsed, p, 0);
    p = strtok_r (NULL, " ", &tokstr);
  }
  free (sortsel);

  return parsed;
}
