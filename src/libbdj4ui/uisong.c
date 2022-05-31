#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>

#include "slist.h"
#include "tagdef.h"
#include "ui.h"
#include "uisong.h"
#include "uiutils.h"

static valuetype_t uisongDetermineValueType (int tagidx);

void
uisongSetDisplayColumns (slist_t *sellist, song_t *song, int col,
    GtkTreeModel *model, GtkTreeIter *iter)
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
    uiSetDisplayColumn (GTK_LIST_STORE (model), iter,
        col++, num, str);
    if (str != NULL) {
      free (str);
    }
  } /* for each tagidx in the display selection list */
}

GType *
uisongAddDisplayTypes (GType *typelist, slist_t *sellist, int *colcount)
{
  slistidx_t    seliteridx;
  int           tagidx;

  slistStartIterator (sellist, &seliteridx);
  while ((tagidx = slistIterateValueNum (sellist, &seliteridx)) >= 0) {
    valuetype_t vt;
    int         type;

    vt = uisongDetermineValueType (tagidx);
    type = vt == VALUE_STR ? UITREE_TYPE_STRING : UITREE_TYPE_NUM;
    typelist = uiAddDisplayType (typelist, type, *colcount);
    ++(*colcount);
  }

  return typelist;
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


