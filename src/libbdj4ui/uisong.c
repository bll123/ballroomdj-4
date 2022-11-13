#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include "slist.h"
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

