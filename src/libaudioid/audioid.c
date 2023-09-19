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
#include "tmutil.h"

enum {
  AUDIOID_TYPE_ACOUSTID,
  AUDIOID_TYPE_MUSICBRAINZ,
  AUDIOID_TYPE_ACRCLOUD,
  AUDIOID_TYPE_MAX,
};

/* the duration is often rounded off to the nearest second, */
/* so there can be a guaranteed difference of 500 milliseconds */
enum {
  AUDIOID_DUR_DIFF = 2000,
};
#define AUDIOID_SCORE_ADJUST 1.0
#define AUDIOID_MIN_SCORE 85.0

typedef struct audioid {
  audioidmb_t       *mb;
  audioidacoustid_t *acoustid;
  audioidacr_t      *acr;
  nlist_t           *respidx;
  nlistidx_t        respiter;
  ilist_t           *resp;
  int               state;
  int               statecount;
  int               respcount [AUDIOID_TYPE_MAX];
  bool              mbmatch : 1;
} audioid_t;

static double audioidAdjustScoreNum (audioid_t *audioid, int key, int tagidx, const song_t *song, double score);
static double audioidAdjustScoreStr (audioid_t *audioid, int key, int tagidx, const song_t *song, double score);
static void dumpResults (audioid_t *audioid);

audioid_t *
audioidInit (void)
{
  audioid_t *audioid;

  audioid = mdmalloc (sizeof (audioid_t));
  audioid->mb = NULL;
  audioid->acoustid = NULL;
  audioid->acr = NULL;
  audioid->respidx = NULL;
  audioid->resp = NULL;
  audioid->state = BDJ4_STATE_OFF;
  audioid->statecount = AUDIOID_TYPE_ACOUSTID;
  audioid->mbmatch = false;
  for (int i = 0; i < AUDIOID_TYPE_MAX; ++i) {
    audioid->respcount [i] = 0;
  }
  audioidParseInit ();
  return audioid;
}

void
audioidFree (audioid_t *audioid)
{
  if (audioid == NULL) {
    return;
  }

  audioidParseCleanup ();
  nlistFree (audioid->respidx);
  ilistFree (audioid->resp);
  mbFree (audioid->mb);
  acoustidFree (audioid->acoustid);
  acrFree (audioid->acr);
  mdfree (audioid);
}

bool
audioidLookup (audioid_t *audioid, const song_t *song)
{
  const char  *mbrecid;
  ilistidx_t  iteridx;
  ilistidx_t  key;
  mstime_t    starttm;

  mstimestart (&starttm);
  if (audioid->mb == NULL) {
    audioid->mb = mbInit ();
    audioid->acoustid = acoustidInit ();
    audioid->acr = acrInit ();
  }

  if (audioid == NULL || song == NULL) {
    return 0;
  }

  if (audioid->state == BDJ4_STATE_OFF ||
      audioid->state == BDJ4_STATE_FINISH) {
    audioid->state = BDJ4_STATE_START;
    audioid->statecount = AUDIOID_TYPE_ACOUSTID;
//    audioid->statecount = AUDIOID_TYPE_ACRCLOUD;
    audioid->mbmatch = false;
  }

  if (audioid->state == BDJ4_STATE_START) {
    ilistFree (audioid->resp);
    audioid->resp = ilistAlloc ("audioid-resp", LIST_ORDERED);
    ilistSetNum (audioid->resp, 0, AUDIOID_TYPE_RESPIDX, 0);
    nlistFree (audioid->respidx);
    audioid->respidx = nlistAlloc ("audioid-resp-idx", LIST_UNORDERED, NULL);
    audioid->state = BDJ4_STATE_WAIT;
    logMsg (LOG_DBG, LOG_AUDIO_ID, "process: %s\n", songGetStr (song, TAG_FILE));
  }

  if (audioid->state == BDJ4_STATE_WAIT) {
    /* process acoustid first rather than musicbrainz, as it returns */
    /* more data.  if there are no matches, then musicbrainz will be */
    /* used if a musicbrainz recording-id is present */
    if (audioid->statecount == AUDIOID_TYPE_ACOUSTID) {
      char    tmp [40];

      snprintf (tmp, sizeof (tmp), "%ld", (long) songGetNum (song, TAG_DURATION));
      ilistSetStr (audioid->resp, 0, TAG_DURATION, tmp);
      audioid->respcount [AUDIOID_TYPE_ACOUSTID] =
          acoustidLookup (audioid->acoustid, song, audioid->resp);
      logMsg (LOG_DBG, LOG_AUDIO_ID, "acoustid: matches: %d",
          audioid->respcount [AUDIOID_TYPE_ACOUSTID]);
      ++audioid->statecount;
    } else if (audioid->statecount == AUDIOID_TYPE_MUSICBRAINZ) {
      mbrecid = songGetStr (song, TAG_RECORDING_ID);
      if (mbrecid != NULL && audioid->respcount [AUDIOID_TYPE_ACOUSTID] == 0) {
        audioid->respcount [AUDIOID_TYPE_MUSICBRAINZ] =
          mbRecordingIdLookup (audioid->mb, mbrecid, audioid->resp);
        logMsg (LOG_DBG, LOG_AUDIO_ID, "musicbrainz: matches: %d",
            audioid->respcount [AUDIOID_TYPE_MUSICBRAINZ]);
        if (audioid->respcount [AUDIOID_TYPE_MUSICBRAINZ] > 0) {
          audioid->state = BDJ4_STATE_PROCESS;
          audioid->mbmatch = true;
        }
      }
      ++audioid->statecount;
    } else if (audioid->statecount == AUDIOID_TYPE_ACRCLOUD) {
      audioid->respcount [AUDIOID_TYPE_ACRCLOUD] =
          acrLookup (audioid->acr, song, audioid->resp);
      logMsg (LOG_DBG, LOG_AUDIO_ID, "acrcloud: matches: %d",
          audioid->respcount [AUDIOID_TYPE_ACRCLOUD]);
      ++audioid->statecount;
    } else if (audioid->statecount == AUDIOID_TYPE_MAX) {
      audioid->statecount = AUDIOID_TYPE_ACOUSTID;
      audioid->state = BDJ4_STATE_PROCESS;
    }
    logMsg (LOG_DBG, LOG_IMPORTANT, "audioid: state %d time: %" PRId64 "ms",
        audioid->state, (int64_t) mstimeend (&starttm));
    return false;
  }

  if (logCheck (LOG_DBG, LOG_AUDIOID_DUMP)) {
    dumpResults (audioid);
  }

  if (audioid->state == BDJ4_STATE_PROCESS) {
    nlistSetSize (audioid->respidx, ilistGetCount (audioid->resp));

    logMsg (LOG_DBG, LOG_IMPORTANT, "audioid: total responses: %d",
        ilistGetCount (audioid->resp));
    ilistStartIterator (audioid->resp, &iteridx);
    while ((key = ilistIterateKey (audioid->resp, &iteridx)) >= 0) {
      double      score;
      const char  *str;
      long        dur;
      long        tdur = -1;

      if (audioid->mbmatch) {
        score = 100.0;
      } else {
        score = ilistGetDouble (audioid->resp, key, TAG_AUDIOID_SCORE);
      }
      if (score < AUDIOID_MIN_SCORE) {
        logMsg (LOG_DBG, LOG_AUDIO_ID, "%d score %.1f < %.1f, reject",
            key, score, AUDIOID_MIN_SCORE);
        /* do not add this to the response index list */
        continue;
      }

      /* for a musicbrainz match, do not process matching checks */
      if (! audioid->mbmatch) {
        str = ilistGetStr (audioid->resp, key, TAG_DURATION);
        if (str != NULL) {
          tdur = atol (str);
        }
        dur = songGetNum (song, TAG_DURATION);
        if (labs (dur - tdur) > AUDIOID_DUR_DIFF) {
          logMsg (LOG_DBG, LOG_AUDIO_ID, "%d duration reject %ld/%ld", key, dur, tdur);
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

  logMsg (LOG_DBG, LOG_IMPORTANT, "audioid: state %d time: %" PRId64 "ms",
      audioid->state, (int64_t) mstimeend (&starttm));
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
  if (sstr != NULL && tstr != NULL && *sstr && *tstr) {
    if (tstr == NULL || strcmp (tstr, sstr) != 0) {
      score -= AUDIOID_SCORE_ADJUST;
    }
  }

  return score;
}

static void
dumpResults (audioid_t *audioid)
{
  nlist_t     *l;
  nlistidx_t  i;
  nlistidx_t  tagidx;
  nlistidx_t  iteridx;
  nlistidx_t  key;
  double      score;

  ilistStartIterator (audioid->resp, &iteridx);
  while ((key = ilistIterateKey (audioid->resp, &iteridx)) >= 0) {
    logMsg (LOG_DBG, LOG_AUDIOID_DUMP, "== resp key: %d", key);

    score = ilistGetDouble (audioid->resp, key, TAG_AUDIOID_SCORE);
    logMsg (LOG_DBG, LOG_AUDIOID_DUMP, "   %d SCORE %.1f", key, score);

    l = ilistGetDatalist (audioid->resp, key);
    nlistStartIterator (l, &i);
    while ((tagidx = nlistIterateKey (l, &i)) >= 0) {
      if (tagidx >= TAG_KEY_MAX) {
        continue;
      }
      if (tagidx == TAG_AUDIOID_SCORE) {
        continue;
      }
      logMsg (LOG_DBG, LOG_AUDIOID_DUMP, "   %d %d/%s %s", key, tagidx, tagdefs [tagidx].tag, nlistGetStr (l, tagidx));
    }
  }
}

