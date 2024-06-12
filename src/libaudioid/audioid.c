/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
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
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "song.h"
#include "tagdef.h"
#include "tmutil.h"

/* the duration is often rounded off to the nearest second, */
/* so there can be a guaranteed difference of 500 milliseconds */
enum {
  AUDIOID_DUR_DIFF = 2000,
};
#define AUDIOID_SCORE_ADJUST 1.0
#define AUDIOID_MIN_SCORE 85.0

enum {
  /* used for debugging */
  /* must be set to audioid_id_acoustid for production */
  /* if the initial identification service is changed, update prepkg.sh */
  AUDIOID_START = AUDIOID_ID_ACOUSTID,
};

typedef struct audioid {
  audioidmb_t       *mb;
  audioidacoustid_t *acoustid;
  audioidacr_t      *acr;
  nlist_t           *respidx;
  nlistidx_t        respiter;
  audioid_resp_t    *resp;
  nlist_t           *dupchklist;
  int               state;
  int               idstate;
  int               respcount [AUDIOID_ID_MAX];
} audioid_t;

static double audioidAdjustScoreNum (audioid_t *audioid, int key, int tagidx, const song_t *song, double score);
static double audioidAdjustScoreStr (audioid_t *audioid, int key, int tagidx, const song_t *song, double score);
static audioid_resp_t * audioidResponseAlloc (void);
static void audioidResponseReset (audioid_resp_t *resp);
static void audioidResponseFree (audioid_resp_t *resp);
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
  audioid->resp = audioidResponseAlloc ();
  audioid->state = BDJ4_STATE_OFF;
  audioid->idstate = AUDIOID_ID_ACOUSTID;
  for (int i = 0; i < AUDIOID_ID_MAX; ++i) {
    audioid->respcount [i] = 0;
  }
  audioidParseXMLInit ();

  audioid->dupchklist = nlistAlloc ("auid-chk-list", LIST_UNORDERED, NULL);
  for (int i = 0; i < TAG_KEY_MAX; ++i) {
    if (i == TAG_AUDIOID_IDENT ||
        i == TAG_AUDIOID_SCORE) {
      /* do not add score or ident to the duplicate check list */
      continue;
    }
    if (tagdefs [i].isAudioID) {
      nlistSetNum (audioid->dupchklist, i, i);
    }
  }
  nlistSort (audioid->dupchklist);

  return audioid;
}

void
audioidFree (audioid_t *audioid)
{
  if (audioid == NULL) {
    return;
  }

  audioidParseXMLCleanup ();
  nlistFree (audioid->respidx);
  audioidResponseFree (audioid->resp);
  audioid->resp = NULL;
  mbFree (audioid->mb);
  acoustidFree (audioid->acoustid);
  acrFree (audioid->acr);
  nlistFree (audioid->dupchklist);
  mdfree (audioid);
}

bool
audioidLookup (audioid_t *audioid, const song_t *song)
{
  const char  *mbrecid;
  nlistidx_t  iteridx;
  nlistidx_t  key;
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
    /* audioid_start is set to audioid_id_acoustid unless debugging */
    audioid->idstate = AUDIOID_START;
    audioid->state = BDJ4_STATE_START;
  }

  if (audioid->state == BDJ4_STATE_START) {
    audioidResponseReset (audioid->resp);
    nlistFree (audioid->respidx);
    audioid->respidx = nlistAlloc ("audioid-resp-idx", LIST_UNORDERED, NULL);
    audioid->state = BDJ4_STATE_WAIT;
    logMsg (LOG_DBG, LOG_AUDIO_ID, "process: %s", songGetStr (song, TAG_URI));
  }

  if (audioid->state == BDJ4_STATE_WAIT) {
    /* process acoustid first rather than musicbrainz, as it returns */
    /* more data.  */
    /* musicbrainz will be used if a musicbrainz recording-id is present */
    /* musicbrainz only returns the first 25 matches */
    switch (audioid->idstate) {
      case AUDIOID_ID_ACOUSTID: {
        char      tmp [40];
        nlist_t   *respdata;

        snprintf (tmp, sizeof (tmp), "%ld", (long) songGetNum (song, TAG_DURATION));
        respdata = audioidGetResponseData (audioid->resp, 0);
        nlistSetStr (respdata, TAG_DURATION, tmp);
        audioid->respcount [AUDIOID_ID_ACOUSTID] =
            acoustidLookup (audioid->acoustid, song, audioid->resp);
        logMsg (LOG_DBG, LOG_AUDIO_ID, "acoustid: matches: %d",
            audioid->respcount [AUDIOID_ID_ACOUSTID]);

        ++audioid->idstate;
        break;
      }
      case AUDIOID_ID_MB_LOOKUP: {
        mbrecid = songGetStr (song, TAG_RECORDING_ID);

        if (mbrecid != NULL) {
          audioid->respcount [AUDIOID_ID_MB_LOOKUP] =
              mbRecordingIdLookup (audioid->mb, mbrecid, audioid->resp);
          logMsg (LOG_DBG, LOG_AUDIO_ID, "musicbrainz: matches: %d",
              audioid->respcount [AUDIOID_ID_MB_LOOKUP]);
          if (audioid->respcount [AUDIOID_ID_MB_LOOKUP] > 0) {
            audioid->state = BDJ4_STATE_PROCESS;
            audioid->idstate = AUDIOID_ID_MAX;
          }
        }

        ++audioid->idstate;
        break;
      }
      case AUDIOID_ID_ACRCLOUD: {
        audioid->respcount [AUDIOID_ID_ACRCLOUD] =
            acrLookup (audioid->acr, song, audioid->resp);
        logMsg (LOG_DBG, LOG_AUDIO_ID, "acrcloud: matches: %d",
            audioid->respcount [AUDIOID_ID_ACRCLOUD]);

        ++audioid->idstate;
        break;
      }
      case AUDIOID_ID_MAX: {
        audioid->idstate = AUDIOID_ID_ACOUSTID;
        audioid->state = BDJ4_STATE_PROCESS;
        break;
      }
    }
    logMsg (LOG_DBG, LOG_IMPORTANT, "audioid: state %d/%s: time: %" PRId64 "ms",
        audioid->state, logStateDebugText (audioid->state), (int64_t) mstimeend (&starttm));
    return false;
  }

  if (logCheck (LOG_DBG, LOG_AUDIOID_DUMP)) {
    dumpResults (audioid);
  }

  if (audioid->state == BDJ4_STATE_PROCESS) {
    nlist_t     *orespidx;
    nlistidx_t  prevkey = -1;
    nlistidx_t  idxkey = -1;
    nlistidx_t  riiter;

    nlistSetSize (audioid->respidx, nlistGetCount (audioid->resp->respdatalist));

    logMsg (LOG_DBG, LOG_IMPORTANT, "audioid: total responses: %d",
        nlistGetCount (audioid->resp->respdatalist));
    nlistStartIterator (audioid->resp->respdatalist, &iteridx);
    while ((key = nlistIterateKey (audioid->resp->respdatalist, &iteridx)) >= 0) {
      audioid_id_t  ident;
      double        score;
      const char    *str;
      long          dur;
      long          tdur = -1;
      nlist_t       *respdata;

      respdata = audioidGetResponseData (audioid->resp, key);
      score = nlistGetDouble (respdata, TAG_AUDIOID_SCORE);

      str = nlistGetStr (respdata, TAG_TITLE);
      if (str == NULL || ! *str) {
        logMsg (LOG_DBG, LOG_AUDIO_ID, "%" PRId32 " no title, reject", key);
        /* acoustid can return results with id and score and no data */
        /* do not add this to the response index list */
        continue;
      }

      if (score == LIST_VALUE_INVALID || score < AUDIOID_MIN_SCORE) {
        logMsg (LOG_DBG, LOG_AUDIO_ID, "%" PRId32 " score %.1f < %.1f, reject",
            key, score, AUDIOID_MIN_SCORE);
        /* do not add this to the response index list */
        continue;
      }

      /* musicbrainz matches do not have a duration check */
      ident = nlistGetNum (respdata, TAG_AUDIOID_IDENT);
      if (ident != AUDIOID_ID_MB_LOOKUP) {
        str = nlistGetStr (respdata, TAG_DURATION);
        if (str != NULL) {
          tdur = atol (str);
        }
        dur = songGetNum (song, TAG_DURATION);
        if (labs (dur - tdur) > AUDIOID_DUR_DIFF) {
          logMsg (LOG_DBG, LOG_AUDIO_ID, "%" PRId32 " duration reject %ld/%ld", key, dur, tdur);
          continue;
        }
      }

      score = audioidAdjustScoreStr (audioid, key, TAG_ALBUM, song, score);
      score = audioidAdjustScoreStr (audioid, key, TAG_TITLE, song, score);
      /* acrcloud doesn't return track and disc information */
      /* (though there's a mention of track in the doc) */
      /* don't reduce the score for something that's not there */
      if (ident != AUDIOID_ID_ACRCLOUD) {
        score = audioidAdjustScoreNum (audioid, key, TAG_TRACKNUMBER, song, score);
        score = audioidAdjustScoreNum (audioid, key, TAG_DISCNUMBER, song, score);
      }
      nlistSetDouble (respdata, TAG_AUDIOID_SCORE, score);
      nlistSetNum (audioid->respidx, 1000 - (int) (score * 10.0), key);
    }
    nlistSort (audioid->respidx);

    orespidx = audioid->respidx;
    audioid->respidx = nlistAlloc ("audioid-resp-idx", LIST_UNORDERED, NULL);

    /* duplicate removal */
    /* traverse the response index list and build a new response index */
    /* this is very simplistic and may not do a great job */
    /* it's also quite slow */
    nlistStartIterator (orespidx, &riiter);
    while ((idxkey = nlistIterateKey (orespidx, &riiter)) >= 0) {
      nlistidx_t  dupiter;
      nlistidx_t  duptag;
      nlistidx_t  rikey;
      bool        match = true;

      rikey = nlistGetNum (orespidx, idxkey);
      if (prevkey < 0) {
        nlistSetNum (audioid->respidx, idxkey, rikey);
        prevkey = rikey;
        continue;
      }
      // logMsg (LOG_DBG, LOG_AUDIO_ID, "dup chk compare %" PRId32 "/%" PRId32, prevkey, rikey);

      nlistStartIterator (audioid->dupchklist, &dupiter);
      while ((duptag = nlistIterateKey (audioid->dupchklist, &dupiter)) >= 0) {
        const char  *vala;
        const char  *valb;
        nlist_t     *respdata;

        respdata = audioidGetResponseData (audioid->resp, prevkey);
        vala = nlistGetStr (respdata, duptag);
        respdata = audioidGetResponseData (audioid->resp, rikey);
        valb = nlistGetStr (respdata, duptag);
        // logMsg (LOG_DBG, LOG_AUDIO_ID, "%" PRId32 "/%" PRId32 " dup chk %d/%s %s %s", prevkey, rikey, duptag, tagdefs [duptag].tag, vala, valb);
        if (vala != NULL && valb != NULL &&
            strcmp (vala, valb) != 0) {
          match = false;
          break;
        }
      }

      if (match) {
        /* there is no difference in data between this and the last */
        /* do not add it to the respidx list */
        logMsg (LOG_DBG, LOG_AUDIO_ID, "%" PRId32 " dup data, reject", rikey);
      } else {
        nlistSetNum (audioid->respidx, idxkey, rikey);
      }

      prevkey = rikey;
    }
    nlistFree (orespidx);
    nlistSort (audioid->respidx);

    logMsg (LOG_DBG, LOG_IMPORTANT, "audioid: usable responses: %" PRId32,
        nlistGetCount (audioid->respidx));
    audioid->state = BDJ4_STATE_FINISH;
  }

  logMsg (LOG_DBG, LOG_IMPORTANT, "audioid: state %d/%s time: %" PRId64 "ms",
      audioid->state, logStateDebugText (audioid->state), (int64_t) mstimeend (&starttm));
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

nlistidx_t
audioidIterate (audioid_t *audioid)
{
  nlistidx_t  idxkey;
  nlistidx_t  key;

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
  list = audioidGetResponseData (audioid->resp, key);
  return list;
}

/* internal routines */

static double
audioidAdjustScoreNum (audioid_t *audioid, int key, int tagidx,
    const song_t *song, double score)
{
  const char  *tstr;
  int         sval = -1;
  int         val = -1;
  nlist_t     *respdata;

  respdata = audioidGetResponseData (audioid->resp, key);
  tstr = nlistGetStr (respdata, tagidx);
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
  nlist_t     *respdata;

  respdata = audioidGetResponseData (audioid->resp, key);
  tstr = nlistGetStr (respdata, tagidx);
  sstr = songGetStr (song, tagidx);
  if (sstr != NULL && tstr != NULL && *sstr && *tstr) {
    if (tstr == NULL || strcmp (tstr, sstr) != 0) {
      score -= AUDIOID_SCORE_ADJUST;
    }
  }

  return score;
}

static audioid_resp_t *
audioidResponseAlloc (void)
{
  audioid_resp_t *resp;

  resp = mdmalloc (sizeof (audioid_resp_t));
  resp->joinphrase = NULL;
  resp->respidx = 0;
  resp->respdatalist = NULL;
  resp->tagidx_add = -1;
  audioidResponseReset (resp);
  return resp;
}

static void
audioidResponseReset (audioid_resp_t *resp)
{
  resp->respidx = 0;
  dataFree (resp->joinphrase);
  resp->joinphrase = NULL;
  resp->tagidx_add = -1;
  nlistFree (resp->respdatalist);
  resp->respdatalist = nlistAlloc ("audioid-resp-data-list", LIST_ORDERED, NULL);
}

static void
audioidResponseFree (audioid_resp_t *resp)
{
  if (resp == NULL) {
    return;
  }

  dataFree (resp->joinphrase);
  resp->joinphrase = NULL;
  resp->tagidx_add = -1;
  nlistFree (resp->respdatalist);
  resp->respdatalist = NULL;
  mdfree (resp);
}

static void
dumpResults (audioid_t *audioid)
{
  nlistidx_t  iteridx;
  nlistidx_t  key;

  nlistStartIterator (audioid->resp->respdatalist, &iteridx);
  while ((key = nlistIterateKey (audioid->resp->respdatalist, &iteridx)) >= 0) {
    nlist_t     *respdata;

    logMsg (LOG_DBG, LOG_AUDIOID_DUMP, "== resp key: %" PRId32, key);

    respdata = audioidGetResponseData (audioid->resp, key);
    audioidDumpResult (respdata);
  }
}

