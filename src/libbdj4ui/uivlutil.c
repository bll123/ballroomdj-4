/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bdj4.h"
#include "bdjvarsdf.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "songfav.h"
#include "tagdef.h"
#include "uiclass.h"
#include "uivirtlist.h"
#include "uivlutil.h"

void
uivlAddDisplayColumns (uivirtlist_t *vl, slist_t *sellist, int startcol)
{
  slistidx_t  seliteridx;
  int         tagidx;
  int         colidx;

  if (vl == NULL) {
    return;
  }

  colidx = startcol;
  slistStartIterator (sellist, &seliteridx);
  while ((tagidx = slistIterateValueNum (sellist, &seliteridx)) >= 0) {
    int         minwidth = 10;
    const char  *title;

    if (tagidx == TAG_FAVORITE) {
      songfav_t   *songfav;

      songfav = bdjvarsdfGet (BDJVDF_FAVORITES);
      title = songFavoriteGetStr (songfav, 0, SONGFAV_DISPLAY);
    } else if (tagdefs [tagidx].shortdisplayname != NULL) {
      title = tagdefs [tagidx].shortdisplayname;
    } else {
      title = tagdefs [tagidx].displayname;
    }

    if (tagdefs [tagidx].ellipsize) {
      minwidth = 10;
    }
    if (tagidx == TAG_TITLE || tagidx == TAG_NOTES) {
      minwidth = 20;
    }
    if (tagidx == TAG_ARTIST || tagidx == TAG_ALBUMARTIST ||
        tagidx == TAG_TAGS) {
      minwidth = 15;
    }
    if (tagdefs [tagidx].ellipsize) {
      uivlSetColumnMinWidth (vl, colidx, minwidth);
      uivlSetColumnEllipsizeOn (vl, colidx);
    }
    if (! tagdefs [tagidx].ellipsize &&
        tagdefs [tagidx].valueType == VALUE_STR) {
      /* pure fixed width columns are rare */
      /* most of the numeric columns have a heading that is wider */
      /* than their width potential */
      uivlSetColumnGrow (vl, colidx, VL_COL_WIDTH_GROW_ONLY);
    }
    if (tagidx == TAG_DANCE || tagidx == TAG_DANCERATING ||
        tagidx == TAG_DANCELEVEL || tagidx == TAG_GENRE) {
      uivlSetColumnGrow (vl, colidx, VL_COL_WIDTH_GROW_ONLY);
    }

    uivlMakeColumn (vl, tagdefs [tagidx].tag, colidx, VL_TYPE_LABEL);
    uivlSetColumnHeading (vl, colidx, title);
    if (tagidx == TAG_FAVORITE) {
      uivlSetColumnClass (vl, colidx, LIST_FAV_CLASS);
      uivlSetColumnAlignCenter (vl, colidx);
      uivlSetColumnGrow (vl, colidx, VL_COL_WIDTH_FIXED);
    }

    if (tagdefs [tagidx].alignend) {
      uivlSetColumnAlignEnd (vl, colidx);
    }

    ++colidx;
  }

  return;
}

