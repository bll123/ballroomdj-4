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
#include "uiutils.h"

static valuetype_t uisongDetermineValueType (int tagidx);

void
uisongSetDisplayColumns (slist_t *sellist, song_t *song, int col,
    uisongcb_t cb, void *udata)
{
  slistidx_t    seliteridx;
  valuetype_t   vt;
  int           tagidx;
  char          *str = NULL;
  long          num;

  slistStartIterator (sellist, &seliteridx);
  while ((tagidx = slistIterateValueNum (sellist, &seliteridx)) >= 0) {
    vt = uisongDetermineValueType (tagidx);
    str = NULL;
    if (vt == VALUE_STR) {
      str = songDisplayString (song, tagidx);
    } else {
      num = songGetNum (song, tagidx);
      if (num == LIST_VALUE_INVALID) { num = 0; }
    }
    if (cb != NULL) {
      cb (col, num, str, udata);
    }
    col += 1;
    if (str != NULL) {
      free (str);
    }
  } /* for each tagidx in the display selection list */
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

    vt = uisongDetermineValueType (tagidx);
    type = vt == VALUE_STR ? UITREE_TYPE_STRING : UITREE_TYPE_NUM;
    if (cb != NULL) {
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
  if (tagdefs [tagidx].convfunc != NULL) {
    vt = VALUE_STR;
  }

  return vt;
}
