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
#include "mdebug.h"
#include "rating.h"
#include "ui.h"

static bool confuiRatingListCreate (void *udata);
static void confuiRatingSave (confuigui_t *gui);
static void confuiRatingFillRow (void *udata, uivirtlist_t *vl, int32_t rownum);

void
confuiBuildUIEditRatings (confuigui_t *gui)
{
  uiwcont_t    *vbox;
  uiwcont_t    *hbox;
  uiwcont_t    *uiwidgetp;

  logProcBegin ();
  vbox = uiCreateVertBox ();

  /* edit ratings */
  confuiMakeNotebookTab (vbox, gui,
      /* CONTEXT: configuration: edit the dance ratings table */
      _("Edit Ratings"), CONFUI_ID_RATINGS);

  /* CONTEXT: configuration: dance ratings: information on how to order the ratings */
  uiwidgetp = uiCreateLabel (_("Order from the lowest rating to the highest rating."));
  uiBoxPackStart (vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  hbox = uiCreateHorizBox ();
  uiWidgetAlignHorizStart (hbox);
  uiBoxPackStartExpand (vbox, hbox);

  confuiMakeItemTable (gui, hbox, CONFUI_ID_RATINGS, CONFUI_TABLE_KEEP_FIRST);
  gui->tables [CONFUI_ID_RATINGS].listcreatefunc = confuiRatingListCreate;
  gui->tables [CONFUI_ID_RATINGS].savefunc = confuiRatingSave;
  confuiCreateRatingTable (gui);

  uiwcontFree (vbox);
  uiwcontFree (hbox);

  logProcEnd ("");
}

void
confuiCreateRatingTable (confuigui_t *gui)
{
//  ilistidx_t        iteridx;
//  ilistidx_t        key;
  rating_t          *ratings;
//  uiwcont_t         *uitree;
  uivirtlist_t      *uivl;
//  int               editable;

  logProcBegin ();

  ratings = bdjvarsdfGet (BDJVDF_RATINGS);

  uivl = gui->tables [CONFUI_ID_RATINGS].uivl;
  uivlSetNumColumns (uivl, CONFUI_RATING_COL_MAX);
  uivlMakeColumnEntry (uivl, CONFUI_RATING_COL_RATING, 15, 30);
  uivlMakeColumnSpinboxNum (uivl, CONFUI_RATING_COL_WEIGHT, 0.0, 100.0, 1.0, 5.0);
  uivlSetColumnHeading (uivl, CONFUI_RATING_COL_RATING,
      tagdefs [TAG_DANCERATING].shortdisplayname);
  /* CONTEXT: configuration: rating: title of the weight column */
  uivlSetColumnHeading (uivl, CONFUI_RATING_COL_WEIGHT, _("Weight"));
  uivlSetNumRows (uivl, ratingGetCount (ratings));

  gui->tables [CONFUI_ID_RATINGS].callbacks [CONFUI_TABLE_CB_CHANGED] =
      callbackInitI (confuiTableChanged, gui);

  uivlSetRowFillCallback (uivl, confuiRatingFillRow, gui);
  uivlDisplay (uivl);
  /* the first entry field is read-only */
  uivlSetRowColumnReadonly (uivl, 0, CONFUI_RATING_COL_RATING);

#if 0
  uiTreeViewSetEditedCallback (uitree,
      gui->tables [CONFUI_ID_RATINGS].callbacks [CONFUI_TABLE_CB_CHANGED]);

  uiTreeViewCreateValueStore (uitree, CONFUI_RATING_COL_MAX,
      TREE_TYPE_INT,        // rating-editable
      TREE_TYPE_INT,        // weight-editable
      TREE_TYPE_STRING,     // rating-disp
      TREE_TYPE_NUM,        // weight
      TREE_TYPE_WIDGET,     // adjustment
      TREE_TYPE_INT,        // digits
      TREE_TYPE_END);

  ratingStartIterator (ratings, &iteridx);

  editable = false;
  while ((key = ratingIterate (ratings, &iteridx)) >= 0) {
    const char  *ratingdisp;
    long        weight;

    ratingdisp = ratingGetRating (ratings, key);
    weight = ratingGetWeight (ratings, key);

    uiTreeViewValueAppend (uitree);
    confuiRatingSet (uitree, editable, ratingdisp, weight);
    /* all cells other than the very first (Unrated) are editable */
    editable = true;
    gui->tables [CONFUI_ID_RATINGS].currcount += 1;
  }

  uiTreeViewAppendColumn (uitree, TREE_NO_COLUMN,
      TREE_WIDGET_TEXT, TREE_ALIGN_NORM,
      TREE_COL_DISP_GROW, tagdefs [TAG_DANCERATING].shortdisplayname,
      TREE_COL_TYPE_TEXT, CONFUI_RATING_COL_RATING,
      TREE_COL_TYPE_EDITABLE, CONFUI_RATING_COL_R_EDITABLE,
      TREE_COL_TYPE_END);

  uiTreeViewAppendColumn (uitree, TREE_NO_COLUMN,
      TREE_WIDGET_SPINBOX, TREE_ALIGN_RIGHT,
      /* CONTEXT: configuration: rating: title of the weight column */
      TREE_COL_DISP_GROW, _("Weight"),
      TREE_COL_TYPE_TEXT, CONFUI_RATING_COL_WEIGHT,
      TREE_COL_TYPE_EDITABLE, CONFUI_RATING_COL_W_EDITABLE,
      TREE_COL_TYPE_ADJUSTMENT, CONFUI_RATING_COL_ADJUST,
      TREE_COL_TYPE_DIGITS, CONFUI_RATING_COL_DIGITS,
      TREE_COL_TYPE_END);
#endif

  logProcEnd ("");
}

/* internal routines */

static bool
confuiRatingListCreate (void *udata)
{
  confuigui_t *gui = udata;
  char        *ratingdisp;
  int         weight;

  logProcBegin ();
  ratingdisp = uiTreeViewGetValueStr (gui->tables [CONFUI_ID_RATINGS].uitree,
      CONFUI_RATING_COL_RATING);
  weight = uiTreeViewGetValue (gui->tables [CONFUI_ID_RATINGS].uitree,
      CONFUI_RATING_COL_WEIGHT);
  ilistSetStr (gui->tables [CONFUI_ID_RATINGS].savelist,
      gui->tables [CONFUI_ID_RATINGS].saveidx, RATING_RATING, ratingdisp);
  ilistSetNum (gui->tables [CONFUI_ID_RATINGS].savelist,
      gui->tables [CONFUI_ID_RATINGS].saveidx, RATING_WEIGHT, weight);
  dataFree (ratingdisp);
  gui->tables [CONFUI_ID_RATINGS].saveidx += 1;
  logProcEnd ("");
  return UI_FOREACH_CONT;
}

static void
confuiRatingSave (confuigui_t *gui)
{
  rating_t    *ratings;

  logProcBegin ();
  ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  ratingSave (ratings, gui->tables [CONFUI_ID_RATINGS].savelist);
  ilistFree (gui->tables [CONFUI_ID_RATINGS].savelist);
  logProcEnd ("");
}

static void
confuiRatingFillRow (void *udata, uivirtlist_t *vl, int32_t rownum)
{
  confuigui_t *gui = udata;
  rating_t    *ratings;
  const char  *ratingdisp;
  int         weight;

  ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  if (rownum >= ratingGetCount (ratings)) {
    return;
  }

  ratingdisp = ratingGetRating (ratings, rownum);
  weight = ratingGetWeight (ratings, rownum);
  uivlSetRowColumnValue (gui->tables [CONFUI_ID_RATINGS].uivl, rownum,
      CONFUI_RATING_COL_RATING, ratingdisp);
  uivlSetRowColumnNum (gui->tables [CONFUI_ID_RATINGS].uivl, rownum,
      CONFUI_RATING_COL_WEIGHT, weight);
}

