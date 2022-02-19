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

typedef struct {
  int     filterType;
  int     valueType;
} songfilteridx_t;

/* must be in the same order as defined in songfilter.h */
songfilteridx_t valueTypeLookup [SONG_FILTER_MAX] = {
  { SONG_FILTER_BPM_LOW,    SONG_FILTER_NUM },
  { SONG_FILTER_BPM_HIGH,   SONG_FILTER_NUM },
  { SONG_FILTER_DANCE,      SONG_FILTER_ILIST },
  { SONG_FILTER_KEYWORD,    SONG_FILTER_SLIST },
  { SONG_FILTER_LEVEL_HIGH, SONG_FILTER_NUM },
  { SONG_FILTER_LEVEL_LOW,  SONG_FILTER_NUM },
  { SONG_FILTER_RATING,     SONG_FILTER_NUM },
  { SONG_FILTER_SEARCH,     SONG_FILTER_STR },
  { SONG_FILTER_STATUS,     SONG_FILTER_NUM },
};

#define SONG_FILTER_SORT_UNSORTED "UNSORTED"

static bool songfilterCheckStr (char *str, char *searchstr);

songfilter_t *
songfilterAlloc (songfilterpb_t pbflag)
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
  sf->indexList = slistAlloc ("songfilter-idx", LIST_UNORDERED, free, NULL);
  sf->forplayback = pbflag;
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
  for (int i = 0; i < SONG_FILTER_MAX_DATA; ++i) {
    if (sf->datafilter [i] != NULL) {
      if (valueTypeLookup [i].valueType == SONG_FILTER_STR) {
        free (sf->datafilter);
      }
      if (valueTypeLookup [i].valueType == SONG_FILTER_SLIST) {
        slistFree (sf->datafilter);
      }
      if (valueTypeLookup [i].valueType == SONG_FILTER_ILIST) {
        ilistFree (sf->datafilter);
      }
    }
    sf->datafilter [i] = NULL;
  }
  for (int i = 0; i < SONG_FILTER_MAX; ++i) {
    sf->inuse [i] = false;
  }
}

void
songfilterSetData (songfilter_t *sf, int filterType, void *value)
{
  int     valueType;

  if (sf == NULL) {
    return;
  }

  valueType = valueTypeLookup [filterType].valueType;

  if (valueType == SONG_FILTER_SLIST ||
      valueType == SONG_FILTER_ILIST) {
    sf->datafilter [filterType] = value;
  }
  if (valueType == SONG_FILTER_STR) {
    sf->datafilter [filterType] = value;
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

void
songfilterProcess (songfilter_t *sf)
{
  if (sf == NULL) {
    return;
  }

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
    if (sstatus == sf->numfilter [SONG_FILTER_STATUS]) {
      logMsg (LOG_DBG, LOG_SONGSEL, "reject: %zd status %ld != %ld", dbidx, sstatus, sf->numfilter [SONG_FILTER_STATUS]);
      return false;
    }
  }

  /* if the filter is for playback, check to make sure the song's status */
  /* is marked as playable */
  if (sf->forplayback == SONG_FILTER_FOR_PLAYBACK) {
    status_t      *status;
    listidx_t     sstatus;

    status = bdjvarsdfGet (BDJVDF_STATUS);

    sstatus = songGetNum (song, TAG_STATUS);
    if (status != NULL && ! statusPlayCheck (status, sstatus)) {
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


    keyword = songGetData (song, TAG_KEYWORD);

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
    char      str [MAXPATHLEN];
    char      *searchstr;
    bool      found;

    found = false;
    searchstr = (char *) sf->datafilter [SONG_FILTER_SEARCH];

    strlcpy (str, songGetData (song, TAG_KEYWORD), MAXPATHLEN);
    found = songfilterCheckStr (str, searchstr);
    if (! found) {
      slist_t   *tagList;

      tagList = (slist_t *) songGetData (song, TAG_TAGS);
    }
    if (! found) {
      strlcpy (str, songGetData (song, TAG_TITLE), MAXPATHLEN);
      found = songfilterCheckStr (str, searchstr);
    }
    if (! found) {
      strlcpy (str, songGetData (song, TAG_NOTES), MAXPATHLEN);
      found = songfilterCheckStr (str, searchstr);
    }
    if (! found) {
      strlcpy (str, songGetData (song, TAG_ARTIST), MAXPATHLEN);
      found = songfilterCheckStr (str, searchstr);
    }
    if (! found) {
      strlcpy (str, songGetData (song, TAG_ALBUMARTIST), MAXPATHLEN);
      found = songfilterCheckStr (str, searchstr);
    }
    if (! found) {
      strlcpy (str, songGetData (song, TAG_ALBUM), MAXPATHLEN);
      found = songfilterCheckStr (str, searchstr);
    }
    if (! found) {
      return false;
    }
  }

  return true;
}

dbidx_t
songfilterGetByIdx (songfilter_t *sf, nlistidx_t idx)
{
  return 0;
}

/* internal routines */

static bool
songfilterCheckStr (char *str, char *searchstr)
{
  bool found = false;

  stringToLower (str);
  if (strstr (str, searchstr) != NULL) {
    found = true;
  }

  return found;
}
