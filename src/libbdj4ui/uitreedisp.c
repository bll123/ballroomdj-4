/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>

#include "bdjvarsdf.h"
#include "callback.h"
#include "mdebug.h"
#include "slist.h"
#include "songfav.h"
#include "tagdef.h"
#include "uiclass.h"
#include "uitreedisp.h"
#include "uiwcont.h"

#include "ui/uitreeview.h"

/* the tree display routines are used by song selection and the music queue */
/* to create the display listings */

void
uitreedispAddDisplayColumns (uiwcont_t *uiwidget,
    slist_t *sellist, int col, int fontcol, int ellipsizeColumn,
    int colorcol, int colorsetcol)
{
  slistidx_t  seliteridx;
  int         tagidx;

  if (uiwidget == NULL) {
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

    if (colorcol != TREE_NO_COLUMN) {
      uiTreeViewPreColumnSetColorColumn (uiwidget, colorcol, colorsetcol);
    }

    if (tagidx == TAG_TITLE) {
      uiTreeViewPreColumnSetMinWidth (uiwidget, 200);
      uiTreeViewPreColumnSetEllipsizeColumn (uiwidget, ellipsizeColumn);
    }
    if (tagidx == TAG_ARTIST) {
      uiTreeViewPreColumnSetMinWidth (uiwidget, 100);
      uiTreeViewPreColumnSetEllipsizeColumn (uiwidget, ellipsizeColumn);
    }
    if (tagidx == TAG_ALBUM) {
      uiTreeViewPreColumnSetMinWidth (uiwidget, 40);
      uiTreeViewPreColumnSetEllipsizeColumn (uiwidget, ellipsizeColumn);
    }
    if (tagidx == TAG_ALBUMARTIST) {
      uiTreeViewPreColumnSetMinWidth (uiwidget, 100);
      uiTreeViewPreColumnSetEllipsizeColumn (uiwidget, ellipsizeColumn);
    }

    if (tagidx == TAG_FAVORITE) {
      songfav_t   *songfav;

      /* use the normal sized UI font here */
      songfav = bdjvarsdfGet (BDJVDF_FAVORITES);
      uiTreeViewAppendColumn (uiwidget, col,
          TREE_WIDGET_TEXT, TREE_ALIGN_CENTER,
          TREE_COL_DISP_GROW,
            songFavoriteGetStr (songfav, SONG_FAVORITE_NONE, SONGFAV_DISPLAY),
          TREE_COL_TYPE_MARKUP, col,
          TREE_COL_TYPE_END);
    } else {
      uiTreeViewAppendColumn (uiwidget, TREE_NO_COLUMN,
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
uitreedispSetDisplayColumn (uiwcont_t *uiwidget, int col, long num,
    const char *str)
{
  if (str != NULL) {
    uiTreeViewSetValues (uiwidget, col, str, TREE_VALUE_END);
  } else {
    char    tstr [40];

    *tstr = '\0';
    if (num >= 0) {
      snprintf (tstr, sizeof (tstr), "%ld", num);
    }
    uiTreeViewSetValues (uiwidget, col, tstr, TREE_VALUE_END);
  }
}
