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
#include "uivirtlist.h"
#include "uivlutil.h"

static const char * const VL_FAV_CLASS = "bdj-list-fav";

void
uivlAddDisplayColumns (uivirtlist_t *vl, slist_t *sellist, int startcol)
{
  slistidx_t  seliteridx;
  int         tagidx;
  int         col;

  if (vl == NULL) {
    return;
  }

  col = startcol;
  slistStartIterator (sellist, &seliteridx);
  while ((tagidx = slistIterateValueNum (sellist, &seliteridx)) >= 0) {
    int         minwidth = 10;
    const char  *title;

    if (tagidx == TAG_ALBUM) {
      title = "Album";
    } else
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
      uivlSetColumnMinWidth (vl, col, minwidth);
      uivlSetColumnEllipsizeOn (vl, col);
    }
    if (tagidx == TAG_AUDIOID_IDENT) {
      uivlSetColumnGrow (vl, col, VL_COL_WIDTH_GROW_ONLY);
    }

    uivlMakeColumn (vl, tagdefs [tagidx].tag, col, VL_TYPE_LABEL);
    uivlSetColumnHeading (vl, col, title);
    if (tagidx == TAG_FAVORITE) {
      uivlSetColumnClass (vl, col, VL_FAV_CLASS);
      uivlSetColumnAlignCenter (vl, col);
    }

    if (tagdefs [tagidx].alignend) {
      uivlSetColumnAlignEnd (vl, col);
    }

    ++col;
  }

  return;
}

