/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "mdebug.h"
#include "slist.h"
#include "songutil.h"
#include "tagdef.h"
#include "ui.h"
#include "uisong.h"

static valuetype_t uisongDetermineValueType (int tagidx);

void
uisongSetDisplayColumns (slist_t *sellist, song_t *song, int col,
    uisongcb_t cb, void *udata)
{
  slistidx_t    seliteridx;
  int           tagidx;
  char          *str = NULL;
  long          num;
  double        dval;

  slistStartIterator (sellist, &seliteridx);
  while ((tagidx = slistIterateValueNum (sellist, &seliteridx)) >= 0) {
    if (tagidx == TAG_AUDIOID_IDENT) {
      col += 1;
      continue;
    }
    if (song != NULL) {
      str = uisongGetDisplay (song, tagidx, &num, &dval);
    } else {
      str = "";
      num = LIST_VALUE_INVALID;
    }
    if (cb != NULL) {
      cb (col, num, str, udata);
    }
    col += 1;
    if (song != NULL) {
      dataFree (str);
    }
  } /* for each tagidx in the display selection list */
}

char *
uisongGetDisplay (song_t *song, int tagidx, long *num, double *dval)
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
uisongGetValue (song_t *song, int tagidx, long *num, double *dval)
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
    /* is never store in the song */
    str = mdstrdup ("");
  }

  return str;
}


void
uisongAddDisplayTypes (slist_t *sellist, uisongdtcb_t cb, void *udata)
{
  slistidx_t    seliteridx;
  int           tagidx;

  slistStartIterator (sellist, &seliteridx);
  while ((tagidx = slistIterateValueNum (sellist, &seliteridx)) >= 0) {
    if (cb != NULL) {
      int   type = TREE_TYPE_STRING;

      /* gtk doesn't have a method to display a blank numeric afaik */
      cb (type, udata);
    }
  }
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

