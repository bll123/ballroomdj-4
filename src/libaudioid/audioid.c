/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include "audioid.h"
#include "bdj4.h"
#include "bdjstring.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "song.h"
#include "tagdef.h"

enum {
  AUDIOID_TYPE_ACOUSTID,
  AUDIOID_TYPE_ACRCLOUD,
  AUDIOID_TYPE_MAX,
};

/* the duration is often rounded off to the nearest second, */
/* so there can be a guaranteed difference of 500 milliseconds */
enum {
  AUDIOID_DUR_DIFF = 2000,
};
#define AUDIOID_SCORE_ADJUST 1.0

typedef struct audioid {
  audioidmb_t       *mb;
  audioidacoustid_t *acoustid;
  nlist_t           *respidx;
  nlistidx_t        respiter;
  ilist_t           *resp;
  int               state;
  int               statecount;
  bool              mbmatch : 1;
} audioid_t;

static double audioidAdjustScoreNum (audioid_t *audioid, int key, int tagidx, const song_t *song, double score);
static double audioidAdjustScoreStr (audioid_t *audioid, int key, int tagidx, const song_t *song, double score);

audioid_t *
audioidInit (void)
{
  audioid_t *audioid;

  audioid = mdmalloc (sizeof (audioid_t));
  audioid->mb = NULL;
  audioid->acoustid = NULL;
  audioid->respidx = NULL;
  audioid->resp = NULL;
  audioid->state = BDJ4_STATE_OFF;
  audioid->statecount = AUDIOID_TYPE_ACOUSTID;
  audioid->mbmatch = false;
  return audioid;
}

void
audioidFree (audioid_t *audioid)
{
  if (audioid == NULL) {
    return;
  }

  nlistFree (audioid->respidx);
  ilistFree (audioid->resp);
  mbFree (audioid->mb);
  acoustidFree (audioid->acoustid);
  mdfree (audioid);
}

bool
audioidLookup (audioid_t *audioid, const song_t *song)
{
  const char  *mbrecid;
  ilistidx_t  iteridx;
  ilistidx_t  key;

  if (audioid->mb == NULL) {
    audioid->mb = mbInit ();
    audioid->acoustid = acoustidInit ();
  }

  if (audioid == NULL || song == NULL) {
    return 0;
  }

  if (audioid->state == BDJ4_STATE_OFF ||
      audioid->state == BDJ4_STATE_FINISH) {
    ilistFree (audioid->resp);
    audioid->resp = ilistAlloc ("audioid-resp", LIST_ORDERED);
    nlistFree (audioid->respidx);
    audioid->respidx = nlistAlloc ("audioid-resp-idx", LIST_UNORDERED, NULL);
    audioid->state = BDJ4_STATE_START;
    audioid->statecount = AUDIOID_TYPE_ACOUSTID;
    audioid->mbmatch = false;
  }

  if (audioid->state == BDJ4_STATE_START) {
    mbrecid = songGetStr (song, TAG_RECORDING_ID);
    if (mbrecid != NULL) {
      mbRecordingIdLookup (audioid->mb, mbrecid, audioid->resp);
      audioid->state = BDJ4_STATE_PROCESS;
      audioid->mbmatch = true;
      return false;
    } else {
      audioid->state = BDJ4_STATE_WAIT;
    }
  }

  if (audioid->state == BDJ4_STATE_WAIT) {
    if (audioid->statecount == AUDIOID_TYPE_ACOUSTID) {
      acoustidLookup (audioid->acoustid, song, audioid->resp);
      ++audioid->statecount;
    } else if (audioid->statecount == AUDIOID_TYPE_ACRCLOUD) {
      ++audioid->statecount;
    } else if (audioid->statecount == AUDIOID_TYPE_MAX) {
      audioid->statecount = 0;
      audioid->state = BDJ4_STATE_PROCESS;
    }
    return false;
  }

  if (audioid->state == BDJ4_STATE_PROCESS) {
    nlistSetSize (audioid->respidx, ilistGetCount (audioid->resp));

    ilistStartIterator (audioid->resp, &iteridx);
    while ((key = ilistIterateKey (audioid->resp, &iteridx)) >= 0) {
      double      score;
      const char  *str;
      long        dur;
      long        tdur = -1;

      score = ilistGetDouble (audioid->resp, key, TAG_AUDIOID_SCORE);
      if (score < 90.0) {
        /* do not add this to the response index list */
        continue;
      }

      /* for a musicbrainz match, do not matching checks */
      if (! audioid->mbmatch) {
        str = ilistGetStr (audioid->resp, key, TAG_DURATION);
        if (str != NULL) {
          tdur = atol (str);
        }
        dur = songGetNum (song, TAG_DURATION);
        if (labs (dur - tdur) > AUDIOID_DUR_DIFF) {
          continue;
        }
      }

      score = audioidAdjustScoreStr (audioid, key, TAG_ALBUM, song, score);
      score = audioidAdjustScoreStr (audioid, key, TAG_TITLE, song, score);
      score = audioidAdjustScoreNum (audioid, key, TAG_TRACKNUMBER, song, score);
      score = audioidAdjustScoreNum (audioid, key, TAG_DISCNUMBER, song, score);
      ilistSetDouble (audioid->resp, key, TAG_AUDIOID_SCORE, score);
      nlistSetNum (audioid->respidx, 1000 - (int) (score * 10.0), key);
    }
    nlistSort (audioid->respidx);
    audioid->state = BDJ4_STATE_FINISH;
  }

  return audioid->state == BDJ4_STATE_FINISH ? true : false;
}

void
audioidStartIterator (audioid_t *audioid)
{
  if (audioid == NULL) {
    return;
  }
  nlistStartIterator (audioid->respidx, &audioid->respiter);
}

ilistidx_t
audioidIterate (audioid_t *audioid)
{
  nlistidx_t  idxkey;
  ilistidx_t  key;

  if (audioid == NULL) {
    return -1;
  }
  idxkey = nlistIterateKey (audioid->respidx, &audioid->respiter);
  if (idxkey < 0) {
    return -1;
  }
  key = nlistGetNum (audioid->respidx, idxkey);
  return key;
}

nlist_t *
audioidGetList (audioid_t *audioid, int key)
{
  nlist_t   *list;

  if (audioid == NULL) {
    return NULL;
  }
  list = ilistGetDatalist (audioid->resp, key);
  return list;
}

static double
audioidAdjustScoreNum (audioid_t *audioid, int key, int tagidx,
    const song_t *song, double score)
{
  const char  *tstr;
  int         sval = -1;
  int         val = -1;

  tstr = ilistGetStr (audioid->resp, key, tagidx);
  if (tstr != NULL) {
    val = atoi (tstr);
  }
  sval = songGetNum (song, tagidx);
  if (sval != LIST_VALUE_INVALID) {
    if (sval != val) {
      score -= AUDIOID_SCORE_ADJUST;
    }
  }

  return score;
}

static double
audioidAdjustScoreStr (audioid_t *audioid, int key, int tagidx,
    const song_t *song, double score)
{
  const char  *sstr;
  const char  *tstr;

  tstr = ilistGetStr (audioid->resp, key, tagidx);
  sstr = songGetStr (song, tagidx);
  if (sstr != NULL) {
    if (tstr == NULL || strcmp (tstr, sstr) != 0) {
      score -= AUDIOID_SCORE_ADJUST;
    }
  }

  return score;
}
