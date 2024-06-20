/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>
#include <stdarg.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjvarsdf.h"
#include "configui.h"
#include "log.h"
#include "ui.h"

void
confuiDanceSet (uiwcont_t *uitree, const char *dancedisp, ilistidx_t key)
{
  logProcBegin ();
#if 0
  uiTreeViewSetValues (uitree,
      CONFUI_DANCE_COL_DANCE, dancedisp,
      CONFUI_DANCE_COL_SB_PAD, "    ",
      CONFUI_DANCE_COL_DANCE_IDX, (treenum_t) key,
      TREE_VALUE_END);
#endif
  logProcEnd ("");
}

void
confuiGenreSet (uiwcont_t *uitree,
    int editable, const char *genredisp, int clflag)
{
  logProcBegin ();
  uiTreeViewSetValues (uitree,
      CONFUI_GENRE_COL_EDITABLE, (treeint_t) editable,
      CONFUI_GENRE_COL_GENRE, genredisp,
      CONFUI_GENRE_COL_CLASSICAL, (treeint_t) clflag,
      TREE_VALUE_END);
  logProcEnd ("");
}

void
confuiLevelSet (uiwcont_t *uitree,
    int editable, const char *leveldisp, long weight, int def)
{
  uiwcont_t      *adjustment;

  logProcBegin ();
  adjustment = uiCreateAdjustment (weight, 0.0, 100.0, 1.0, 5.0, 0.0);
  uiTreeViewSetValues (uitree,
      CONFUI_LEVEL_COL_EDITABLE, (treeint_t) editable,
      CONFUI_LEVEL_COL_LEVEL, leveldisp,
      CONFUI_LEVEL_COL_WEIGHT, (treenum_t) weight,
      CONFUI_LEVEL_COL_ADJUST, uiAdjustmentGetAdjustment (adjustment),
      CONFUI_LEVEL_COL_DIGITS, (treeint_t) 0,
      CONFUI_LEVEL_COL_DEFAULT, (treebool_t) def,
      TREE_VALUE_END);
  uiwcontFree (adjustment);
  logProcEnd ("");
}

void
confuiRatingSet (uiwcont_t *uitree,
    int editable, const char *ratingdisp, long weight)
{
  uiwcont_t   *adjustment;

  logProcBegin ();
  adjustment = uiCreateAdjustment (weight, 0.0, 100.0, 1.0, 5.0, 0.0);
  uiTreeViewSetValues (uitree,
      CONFUI_RATING_COL_R_EDITABLE, (treeint_t) editable,
      CONFUI_RATING_COL_W_EDITABLE, (treeint_t) true,
      CONFUI_RATING_COL_RATING, ratingdisp,
      CONFUI_RATING_COL_WEIGHT, (treenum_t) weight,
      CONFUI_RATING_COL_ADJUST, uiAdjustmentGetAdjustment (adjustment),
      CONFUI_RATING_COL_DIGITS, (treeint_t) 0,
      TREE_VALUE_END);
  uiwcontFree (adjustment);
  logProcEnd ("");
}

void
confuiStatusSet (uiwcont_t *uitree,
    int editable, const char *statusdisp, int playflag)
{
  logProcBegin ();
  uiTreeViewSetValues (uitree,
      CONFUI_STATUS_COL_EDITABLE, (treeint_t) editable,
      CONFUI_STATUS_COL_STATUS, statusdisp,
      CONFUI_STATUS_COL_PLAY_FLAG, (treebool_t) playflag,
      TREE_VALUE_END);
  logProcEnd ("");
}


