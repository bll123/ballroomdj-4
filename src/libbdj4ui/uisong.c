/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include "mdebug.h"
#include "slist.h"
#include "songutil.h"
#include "tagdef.h"
#include "ui.h"
#include "uisong.h"

static valuetype_t uisongDetermineDisplayValueType (int tagidx);
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
    str = uisongGetDisplay (song, tagidx, &num, &dval);
    if (cb != NULL) {
      cb (col, num, str, udata);
    }
    col += 1;
    dataFree (str);
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
  if (tagdefs [tagidx].convfunc != NULL) {
    dataFree (str);
    str = songDisplayString (song, tagidx);
  }

  return str;
}

char *
uisongGetValue (song_t *song, int tagidx, long *num, double *dval)
{
  valuetype_t   vt;
  char          *str;

  vt = uisongDetermineValueType (tagidx);
  *num = 0;
  *dval = 0.0;
  str = NULL;
  if (vt == VALUE_STR) {
    str = songDisplayString (song, tagidx);
  } else if (vt == VALUE_NUM) {
    *num = songGetNum (song, tagidx);
    if (tagidx == TAG_SONGSTART || tagidx == TAG_SONGEND) {
      int     speed;
      ssize_t val;

      speed = songGetNum (song, TAG_SPEEDADJUSTMENT);
      if (speed > 0 && speed != 100) {
        val = *num;
        if (val > 0) {
          val = songAdjustPosition (val, speed);
          *num = val;
        }
      }
    }
  } else if (vt == VALUE_DOUBLE) {
    *dval = songGetDouble (song, tagidx);
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
    valuetype_t vt;
    int         type;

    vt = uisongDetermineDisplayValueType (tagidx);
    type = vt == VALUE_STR ? UITREE_TYPE_STRING : UITREE_TYPE_NUM;
    if (cb != NULL) {
      cb (type, udata);
    }
  }
}

/* internal routines */

static valuetype_t
uisongDetermineDisplayValueType (int tagidx)
{
  valuetype_t   vt;

  vt = tagdefs [tagidx].valueType;
  if (tagdefs [tagidx].convfunc != NULL) {
    vt = VALUE_STR;
  }

  return vt;
}

static valuetype_t
uisongDetermineValueType (int tagidx)
{
  valuetype_t   vt;

  vt = tagdefs [tagidx].valueType;
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

