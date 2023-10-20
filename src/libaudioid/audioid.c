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
  /* if changed, update prepkg.sh */
  AUDIOID_START = AUDIOID_ID_ACOUSTID,
};

typedef struct audioid {
  audioidmb_t       *mb;
  audioidacoustid_t *acoustid;
  audioidacr_t      *acr;
  nlist_t           *respidx;
  nlistidx_t        respiter;
  ilist_t           *resp;
  nlist_t           *dupchklist;
  int               state;
  int               idstate;
  int               respcount [AUDIOID_ID_MAX];
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
  audioid->idstate = AUDIOID_ID_ACOUSTID;
  for (int i = 0; i < AUDIOID_ID_MAX; ++i) {
    audioid->respcount [i] = 0;
  }
  audioidParseXMLInit ();

  audioid->dupchklist = nlistAlloc ("auid-chk-list", LIST_UNORDERED, NULL);
  for (int i = 0; i < TAG_KEY_MAX; ++i) {
    if (i == TAG_AUDIOID_SCORE) {
      /* score is not added to the duplicate check */
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
  ilistFree (audioid->resp);
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
    /* audioid_start is set to audioid_id_acoustid unless debugging */
    audioid->idstate = AUDIOID_START;
    audioid->state = BDJ4_STATE_START;
  }

  if (audioid->state == BDJ4_STATE_START) {
    ilistFree (audioid->resp);
    audioid->resp = ilistAlloc ("audioid-resp", LIST_ORDERED);
    ilistSetNum (audioid->resp, 0, AUDIOID_TYPE_RESPIDX, 0);
    nlistFree (audioid->respidx);
    audioid->respidx = nlistAlloc ("audioid-resp-idx", LIST_UNORDERED, NULL);
    audioid->state = BDJ4_STATE_WAIT;
    logMsg (LOG_DBG, LOG_AUDIO_ID, "process: %s", songGetStr (song, TAG_FILE));
  }

  if (audioid->state == BDJ4_STATE_WAIT) {
    /* process acoustid first rather than musicbrainz, as it returns */
    /* more data.  if there are no matches, then musicbrainz will be */
    /* used if a musicbrainz recording-id is present */
    /* musicbrainz only returns the first 25 matches */
    switch (audioid->idstate) {
      case AUDIOID_ID_ACOUSTID: {
        char    tmp [40];

        snprintf (tmp, sizeof (tmp), "%ld", (long) songGetNum (song, TAG_DURATION));
        ilistSetStr (audioid->resp, 0, TAG_DURATION, tmp);
        audioid->respcount [AUDIOID_ID_ACOUSTID] =
            acoustidLookup (audioid->acoustid, song, audioid->resp);
        logMsg (LOG_DBG, LOG_AUDIO_ID, "acoustid: matches: %d",
            audioid->respcount [AUDIOID_ID_ACOUSTID]);

        ++audioid->idstate;
        break;
      }
      case AUDIOID_ID_MB_LOOKUP: {
        mbrecid = songGetStr (song, TAG_RECORDING_ID);

        if (mbrecid != NULL &&
            audioid->respcount [AUDIOID_ID_ACOUSTID] <= 1) {
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
    ilistidx_t  prevkey = -1;
    nlistidx_t  idxkey = -1;
    nlistidx_t  riiter;

    nlistSetSize (audioid->respidx, ilistGetCount (audioid->resp));

    logMsg (LOG_DBG, LOG_IMPORTANT, "audioid: total responses: %d",
        ilistGetCount (audioid->resp));
    ilistStartIterator (audioid->resp, &iteridx);
    while ((key = ilistIterateKey (audioid->resp, &iteridx)) >= 0) {
      audioid_id_t  ident;
      double        score;
      const char    *str;
      long          dur;
      long          tdur = -1;

      score = ilistGetDouble (audioid->resp, key, TAG_AUDIOID_SCORE);

      str = ilistGetStr (audioid->resp, key, TAG_TITLE);
      if (str == NULL || ! *str) {
        logMsg (LOG_DBG, LOG_AUDIO_ID, "%d no title, reject", key);
        /* acoustid can return results with id and score and no data */
        /* do not add this to the response index list */
        continue;
      }

      if (score < AUDIOID_MIN_SCORE) {
        logMsg (LOG_DBG, LOG_AUDIO_ID, "%d score %.1f < %.1f, reject",
            key, score, AUDIOID_MIN_SCORE);
        /* do not add this to the response index list */
        continue;
      }

      /* musicbrainz matches do not have a duration check */
      ident = ilistGetNum (audioid->resp, key, AUDIOID_TYPE_IDENT);
      if (ident != AUDIOID_ID_MB_LOOKUP) {
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
      /* acrcloud doesn't return track and disc information */
      /* (though there's a mention of track in the doc */
      /* don't reduce the score for something that's not there */
      if (ident != AUDIOID_ID_ACRCLOUD) {
        score = audioidAdjustScoreNum (audioid, key, TAG_TRACKNUMBER, song, score);
        score = audioidAdjustScoreNum (audioid, key, TAG_DISCNUMBER, song, score);
      }
      ilistSetDouble (audioid->resp, key, TAG_AUDIOID_SCORE, score);
      nlistSetNum (audioid->respidx, 1000 - (int) (score * 10.0), key);
    }
    nlistSort (audioid->respidx);

    orespidx = audioid->respidx;
    audioid->respidx = nlistAlloc ("audioid-resp-idx", LIST_UNORDERED, NULL);

    /* duplicate removal */
    /* traverse the response index list and build a new response index */
    /* this is very simplistic and may not do a great job */
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

      nlistStartIterator (audioid->dupchklist, &dupiter);
      while ((duptag = nlistIterateKey (audioid->dupchklist, &dupiter)) >= 0) {
        const char  *vala;
        const char  *valb;

        vala = ilistGetStr (audioid->resp, prevkey, duptag);
        valb = ilistGetStr (audioid->resp, rikey, duptag);
        if (vala != NULL && valb != NULL &&
            strcmp (vala, valb) != 0) {
          match = false;
          break;
        }
      }

      if (match) {
        /* there is no difference in data between this and the last */
        /* do not add it to the respidx list */
        logMsg (LOG_DBG, LOG_AUDIO_ID, "%d dup data, reject", rikey);
      } else {
        nlistSetNum (audioid->respidx, idxkey, rikey);
      }

      prevkey = rikey;
    }
    nlistFree (orespidx);
    nlistSort (audioid->respidx);

    logMsg (LOG_DBG, LOG_IMPORTANT, "audioid: usable responses: %d",
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
  list = ilistGetDatalist (audioid->resp, key, ILIST_GET);
  return list;
}

bool
audioidSetResponseData (int level, ilist_t *resp, int respidx, int tagidx,
    const char *data, const char *joinphrase)
{
  bool    rc = false;

  if (data == NULL || ! *data) {
    return rc;
  }

  if (joinphrase == NULL) {
    ilistSetStr (resp, respidx, tagidx, data);
    if (tagidx < TAG_KEY_MAX) {
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set respidx(a): %d tagidx: %d %s %s", level*2, "", respidx, tagidx, tagdefs [tagidx].tag, data);
    } else {
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set respidx(b): %d tagidx: %d %s", level*2, "", respidx, tagidx, data);
    }
  } else {
    const char  *tstr;

    tstr = ilistGetStr (resp, respidx, tagidx);
    if (tstr != NULL && *tstr) {
      char        tbuff [400];

      /* the joinphrase may be set before any value has been processed */
      /* if the data for tagidx already exists, */
      /* use the join phrase and return true */
      snprintf (tbuff, sizeof (tbuff), "%s%s%s", tstr, joinphrase, data);
      ilistSetStr (resp, respidx, tagidx, tbuff);
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set respidx(c): %d tagidx: %d %s %s", level*2, "", respidx, tagidx, tagdefs [tagidx].tag, tbuff);
      rc = true;
    } else {
      ilistSetStr (resp, respidx, tagidx, data);
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set respidx(d): %d tagidx: %d %s %s", level*2, "", respidx, tagidx, tagdefs [tagidx].tag, data);
    }
  }

  return rc;
}

/* internal routines */

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
  int         val;
  double      score;
  const char  *tstr;

  ilistStartIterator (audioid->resp, &iteridx);
  while ((key = ilistIterateKey (audioid->resp, &iteridx)) >= 0) {
    logMsg (LOG_DBG, LOG_AUDIOID_DUMP, "== resp key: %d", key);

    score = ilistGetDouble (audioid->resp, key, TAG_AUDIOID_SCORE);
    logMsg (LOG_DBG, LOG_AUDIOID_DUMP, "   %d SCORE %.1f", key, score);
    val = ilistGetNum (audioid->resp, key, AUDIOID_TYPE_IDENT);
    tstr = "Unknown";
    switch (val) {
      case AUDIOID_ID_ACOUSTID: {
        tstr = "AcoustID";
        break;
      }
      case AUDIOID_ID_MB_LOOKUP: {
        tstr = "MusicBrainz";
        break;
      }
      case AUDIOID_ID_ACRCLOUD: {
        tstr = "ACRCloud";
        break;
      }
      default: {
        break;
      }
    }
    logMsg (LOG_DBG, LOG_AUDIOID_DUMP, "   %d IDENT %s", key, tstr);

    l = ilistGetDatalist (audioid->resp, key, ILIST_GET);
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

