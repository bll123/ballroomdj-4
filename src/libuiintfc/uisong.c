/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "mdebug.h"
#include "slist.h"
#include "songutil.h"
#include "tagdef.h"
#include "ui.h"
#include "uisong.h"
#include "uivirtlist.h"

static valuetype_t uisongDetermineValueType (int tagidx);

nlist_t *
uisongGetDisplayList (slist_t *sellistA, slist_t *sellistB, song_t *song)
{
  slistidx_t    seliteridx;
  int           tagidx;
  char          *str = NULL;
  nlist_t       *dlist;
  int32_t       num;
  double        dval;

  dlist = nlistAlloc ("song-disp", LIST_UNORDERED, NULL);
  nlistSetSize (dlist, slistGetCount (sellistA));

  slistStartIterator (sellistA, &seliteridx);
  while ((tagidx = slistIterateValueNum (sellistA, &seliteridx)) >= 0) {
    if (tagidx == TAG_AUDIOID_IDENT || tagidx == TAG_AUDIOID_SCORE) {
      nlistSetStr (dlist, tagidx, "");
      continue;
    }
    if (song != NULL) {
      str = uisongGetDisplay (song, tagidx, &num, &dval);
    } else {
      nlistSetStr (dlist, tagidx, "");
      num = LIST_VALUE_INVALID;
    }

    if (str != NULL) {
      nlistSetStr (dlist, tagidx, str);
      mdfree (str);
    } else if (num != LIST_VALUE_INVALID) {
      char  tmp [40];

      snprintf (tmp, sizeof (tmp), "%" PRId32, num);
      nlistSetStr (dlist, tagidx, tmp);
    }
  } /* for each tagidx in the display selection list */

  nlistSort (dlist);

  if (sellistB != NULL) {
    /* and also any data needed to display in the item display */
    /* this is a bit inefficient, as there are many duplicates */
    slistStartIterator (sellistB, &seliteridx);
    while ((tagidx = slistIterateValueNum (sellistB, &seliteridx)) >= 0) {
      if (song != NULL) {
        str = uisongGetDisplay (song, tagidx, &num, &dval);
      } else {
        nlistSetStr (dlist, tagidx, "");
        num = LIST_VALUE_INVALID;
      }

      if (str != NULL) {
        nlistSetStr (dlist, tagidx, str);
        mdfree (str);
      } else if (num != LIST_VALUE_INVALID) {
        char  tmp [40];

        snprintf (tmp, sizeof (tmp), "%" PRId32, num);
        nlistSetStr (dlist, tagidx, tmp);
      }
    }
  }

  return dlist;
}

char *
uisongGetDisplay (song_t *song, int tagidx, int32_t *num, double *dval)
{
  char          *str;

  *num = 0;
  *dval = 0.0;
  /* get the numeric values also in addition to the display string */
  str = uisongGetValue (song, tagidx, num, dval);
  /* duration is a special case. it needs to be converted to a string. */
  /* but no conversion is defined, as it is never converted from a string. */
  if (tagdefs [tagidx].convfunc != NULL ||
      tagidx == TAG_DURATION || tagidx == TAG_DBADDDATE) {
    dataFree (str);
    str = songDisplayString (song, tagidx, SONG_NORM);
  }
  return str;
}

char *
uisongGetValue (song_t *song, int tagidx, int32_t *num, double *dval)
{
  valuetype_t   vt;
  char          *str = NULL;

  vt = uisongDetermineValueType (tagidx);
  *num = 0;
  *dval = 0.0;

  if (vt == VALUE_STR) {
    str = songDisplayString (song, tagidx, SONG_NORM);
  } else if (vt == VALUE_NUM) {
    *num = songGetNum (song, tagidx);
    if (tagidx == TAG_SONGSTART || tagidx == TAG_SONGEND) {
      int     speed;
      ssize_t val;

      speed = songGetNum (song, TAG_SPEEDADJUSTMENT);
      if (speed > 0 && speed != 100) {
        val = *num;
        if (val > 0) {
          val = songutilAdjustPosReal (val, speed);
          *num = val;
        }
      }
    }
  } else if (vt == VALUE_DOUBLE) {
    *dval = songGetDouble (song, tagidx);
    /* the only double that is displayed is the score, and that value */
    /* is never stored in the song */
  }

  return str;
}


/* internal routines */

static valuetype_t
uisongDetermineValueType (int tagidx)
{
  valuetype_t   vt;

  vt = tagdefs [tagidx].valueType;
  if (tagidx == TAG_DB_LOC_LOCK) {
    return vt;
  }
  if (tagdefs [tagidx].convfunc != NULL) {
    if (vt == VALUE_LIST) {
      vt = VALUE_STR;
    }
    if (vt == VALUE_NUM) {
      vt = VALUE_STR;
    }
    if (vt == VALUE_STR) {
      vt = VALUE_NUM;
    }
  }

  return vt;
}

