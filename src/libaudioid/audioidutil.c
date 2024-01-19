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
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "tagdef.h"

bool
audioidSetResponseData (int level, audioid_resp_t *resp,
    int tagidx, const char *data)
{
  bool    rc = false;
  nlist_t *respdata;

  if (data == NULL || ! *data) {
    return rc;
  }

  respdata = audioidGetResponseData (resp, resp->respidx);
  logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set-resp-data(%d)", level*2, "", resp->respidx);
nlistDumpInfo (respdata);
  if (resp->respidx != nlistGetNum (respdata, AUDIOID_TYPE_RESPIDX)) {
    logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s ERR: incorrect list %d/%d", level*2, "", resp->respidx, (int) nlistGetNum (respdata, AUDIOID_TYPE_RESPIDX));
    return rc;
  }

  if (resp->joinphrase == NULL) {
    nlistSetStr (respdata, tagidx, data);
    if (tagidx < TAG_KEY_MAX) {
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set resp(%d):a: tagidx: %d %s %s", level*2, "", resp->respidx, tagidx, tagdefs [tagidx].tag, data);
    } else {
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set resp(%d):b: tagidx: %d %s", level*2, "", resp->respidx, tagidx, data);
    }
  } else {
    const char  *tstr;

    tstr = nlistGetStr (respdata, tagidx);
    if (tstr != NULL && *tstr) {
      char        tbuff [400];

      /* the joinphrase may be set before any value has been processed */
      /* if the data for tagidx already exists, */
      /* use the join phrase and return true */
      snprintf (tbuff, sizeof (tbuff), "%s%s%s", tstr, resp->joinphrase, data);
      nlistSetStr (respdata, tagidx, tbuff);
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set resp(%d):c: tagidx: %d %s %s", level*2, "", resp->respidx, tagidx, tagdefs [tagidx].tag, tbuff);
      dataFree (resp->joinphrase);
      resp->joinphrase = NULL;
      rc = true;
    } else {
      nlistSetStr (respdata, tagidx, data);
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set resp(%d):d: tagidx: %d %s %s", level*2, "", resp->respidx, tagidx, tagdefs [tagidx].tag, data);
    }
  }

  return rc;
}

nlist_t *
audioidGetResponseData (audioid_resp_t *resp, int respidx)
{
  nlist_t   *respdata;
  char      tmp [40];

  respdata = nlistGetList (resp->respdatalist, respidx);
  if (respdata == NULL) {
    snprintf (tmp, sizeof (tmp), "respdata-%d", respidx);
    respdata = nlistAlloc (tmp, LIST_ORDERED, NULL);
    nlistSetNum (respdata, AUDIOID_TYPE_RESPIDX, respidx);
    nlistSetList (resp->respdatalist, respidx, respdata);
  }

  return respdata;
}


/* for debugging */
void
audioidDumpResult (nlist_t *respdata)
{
  nlistidx_t  i;
  nlistidx_t  tagidx;
  int         val;
  double      score;
  const char  *tstr;

  val = nlistGetNum (respdata, AUDIOID_TYPE_RESPIDX);
  logMsg (LOG_DBG, LOG_AUDIOID_DUMP, "   RESPIDX %d", val);

  score = nlistGetDouble (respdata, TAG_AUDIOID_SCORE);
  logMsg (LOG_DBG, LOG_AUDIOID_DUMP, "   SCORE %.1f", score);

  val = nlistGetNum (respdata, TAG_AUDIOID_IDENT);
  tstr = "Unknown";
  switch (val) {
    case AUDIOID_ID_ACOUSTID: {
      tstr = "AcoustID";
      break;
    }
    case AUDIOID_ID_MB_LOOKUP: {
      tstr = "MB-Lookup";
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
  logMsg (LOG_DBG, LOG_AUDIOID_DUMP, "   IDENT %s", tstr);

  nlistStartIterator (respdata, &i);
  while ((tagidx = nlistIterateKey (respdata, &i)) >= 0) {
    if (tagidx >= TAG_KEY_MAX) {
      continue;
    }
    if (tagidx == TAG_AUDIOID_IDENT ||
        tagidx == TAG_AUDIOID_SCORE) {
      continue;
    }

    logMsg (LOG_DBG, LOG_AUDIOID_DUMP, "   %d/%s %s", tagidx, tagdefs [tagidx].tag, nlistGetStr (respdata, tagidx));
  }
}

