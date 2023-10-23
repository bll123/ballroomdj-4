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
#include "ilist.h"
#include "log.h"
#include "tagdef.h"

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

