/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include "callback.h"
#include "mdebug.h"
#include "slist.h"
#include "tagdef.h"
#include "uiclass.h"
#include "uitreedisp.h"

#include "ui/uigeneral.h"
#include "ui/uitreeview.h"

/* the tree display routines are used by song selection and the music queue */
/* to create the display listings */

void
uitreedispAddDisplayColumns (uitree_t *uitree, slist_t *sellist, int col,
    int fontcol, int ellipsizeColumn)
{
  slistidx_t  seliteridx;
  int         tagidx;

  if (uitree == NULL) {
    return;
  }

  slistStartIterator (sellist, &seliteridx);
  while ((tagidx = slistIterateValueNum (sellist, &seliteridx)) >= 0) {
    int         alignment;
    const char  *title;

    alignment = TREE_ALIGN_NORM;
    if (tagdefs [tagidx].alignRight) {
      alignment = TREE_ALIGN_RIGHT;
    }

    if (tagdefs [tagidx].shortdisplayname != NULL) {
      title = tagdefs [tagidx].shortdisplayname;
    } else {
      title = tagdefs [tagidx].displayname;
    }

    if (tagidx == TAG_TITLE) {
      uiTreeViewPreColumnSetMinWidth (uitree, 200);
      uiTreeViewPreColumnSetEllipsizeColumn (uitree, ellipsizeColumn);
    }
    if (tagidx == TAG_ARTIST) {
      uiTreeViewPreColumnSetMinWidth (uitree, 100);
      uiTreeViewPreColumnSetEllipsizeColumn (uitree, ellipsizeColumn);
    }

    if (tagidx == TAG_FAVORITE) {
      /* use the normal sized UI font here */
      uiTreeViewAppendColumn (uitree,
          TREE_WIDGET_TEXT, TREE_ALIGN_CENTER,
          TREE_COL_DISP_GROW, "\xE2\x98\x86",
          TREE_COL_TYPE_MARKUP, col,
          TREE_COL_TYPE_END);
    } else {
      uiTreeViewAppendColumn (uitree,
          TREE_WIDGET_TEXT, alignment,
          TREE_COL_DISP_GROW, title,
          TREE_COL_TYPE_TEXT, col,
          TREE_COL_TYPE_FONT, fontcol,
          TREE_COL_TYPE_END);
    }

    ++col;
  }

  return;
}

void
uitreedispSetDisplayColumn (uitree_t *uitree, int col, long num,
    const char *str)
{
  if (str != NULL) {
    uiTreeViewSetValues (uitree, col, str, TREE_VALUE_END);
  } else {
    char    tstr [40];

    *tstr = '\0';
    if (num >= 0) {
      snprintf (tstr, sizeof (tstr), "%ld", num);
    }
    uiTreeViewSetValues (uitree, col, tstr, TREE_VALUE_END);
  }
}
