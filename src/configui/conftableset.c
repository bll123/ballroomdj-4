/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
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
confuiDanceSet (uitree_t *uitree, char *dancedisp, ilistidx_t key)
{
  logProcBegin (LOG_PROC, "confuiDanceSet");
  uiTreeViewSetValues (uitree,
      CONFUI_DANCE_COL_DANCE, dancedisp,
      CONFUI_DANCE_COL_SB_PAD, "    ",
      CONFUI_DANCE_COL_DANCE_IDX, (treenum_t) key,
      TREE_VALUE_END);
  logProcEnd (LOG_PROC, "confuiDanceSet", "");
}

void
confuiGenreSet (uitree_t *uitree,
    int editable, char *genredisp, int clflag)
{
  logProcBegin (LOG_PROC, "confuiGenreSet");
  uiTreeViewSetValues (uitree,
      CONFUI_GENRE_COL_EDITABLE, (treeint_t) editable,
      CONFUI_GENRE_COL_GENRE, genredisp,
      CONFUI_GENRE_COL_CLASSICAL, (treeint_t) clflag,
      TREE_VALUE_END);
  logProcEnd (LOG_PROC, "confuiGenreSet", "");
}

void
confuiLevelSet (uitree_t *uitree,
    int editable, char *leveldisp, long weight, int def)
{
  uiwcont_t      adjustment;

  logProcBegin (LOG_PROC, "confuiLevelSet");
  uiCreateAdjustment (&adjustment, weight, 0.0, 100.0, 1.0, 5.0, 0.0);
  uiTreeViewSetValues (uitree,
      CONFUI_LEVEL_COL_EDITABLE, (treeint_t) editable,
      CONFUI_LEVEL_COL_LEVEL, leveldisp,
      CONFUI_LEVEL_COL_WEIGHT, (treenum_t) weight,
      CONFUI_LEVEL_COL_ADJUST, uiAdjustmentGetAdjustment (&adjustment),
      CONFUI_LEVEL_COL_DIGITS, (treeint_t) 0,
      CONFUI_LEVEL_COL_DEFAULT, (treebool_t) def,
      TREE_VALUE_END);
  logProcEnd (LOG_PROC, "confuiLevelSet", "");
}

void
confuiRatingSet (uitree_t *uitree,
    int editable, char *ratingdisp, long weight)
{
  uiwcont_t  adjustment;

  logProcBegin (LOG_PROC, "confuiRatingSet");
  uiCreateAdjustment (&adjustment, weight, 0.0, 100.0, 1.0, 5.0, 0.0);
  uiTreeViewSetValues (uitree,
      CONFUI_RATING_COL_R_EDITABLE, (treeint_t) editable,
      CONFUI_RATING_COL_W_EDITABLE, (treeint_t) TRUE,
      CONFUI_RATING_COL_RATING, ratingdisp,
      CONFUI_RATING_COL_WEIGHT, (treenum_t) weight,
      CONFUI_RATING_COL_ADJUST, adjustment,
      CONFUI_RATING_COL_DIGITS, (treeint_t) 0,
      TREE_VALUE_END);
  logProcEnd (LOG_PROC, "confuiRatingSet", "");
}

void
confuiStatusSet (uitree_t *uitree,
    int editable, char *statusdisp, int playflag)
{
  logProcBegin (LOG_PROC, "confuiStatusSet");
  uiTreeViewSetValues (uitree,
      CONFUI_STATUS_COL_EDITABLE, (treeint_t) editable,
      CONFUI_STATUS_COL_STATUS, statusdisp,
      CONFUI_STATUS_COL_PLAY_FLAG, (treebool_t) playflag,
      TREE_VALUE_END);
  logProcEnd (LOG_PROC, "confuiStatusSet", "");
}


