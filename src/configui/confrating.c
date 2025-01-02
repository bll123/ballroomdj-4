/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
/* the conversion routines use the internal rating-list, but it */
/* is not necessary to update this for editing or for the save */

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

static void confuiRatingSave (confuigui_t *gui);
static void confuiRatingFillRow (void *udata, uivirtlist_t *vl, int32_t rownum);
static bool confuiRatingChangeCB (void *udata);
static int confuiRatingEntryChangeCB (uiwcont_t *entry, const char *label, void *udata);
static void confuiRatingAdd (confuigui_t *gui);
static void confuiRatingRemove (confuigui_t *gui, ilistidx_t idx);
static void confuiRatingMove (confuigui_t *gui, ilistidx_t idx, int dir);

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
  uiBoxPackStartExpand (vbox, hbox);
  uiWidgetAlignHorizStart (hbox);

  confuiMakeItemTable (gui, hbox, CONFUI_ID_RATINGS, CONFUI_TABLE_KEEP_FIRST);
  gui->tables [CONFUI_ID_RATINGS].savefunc = confuiRatingSave;
  gui->tables [CONFUI_ID_RATINGS].addfunc = confuiRatingAdd;
  gui->tables [CONFUI_ID_RATINGS].removefunc = confuiRatingRemove;
  gui->tables [CONFUI_ID_RATINGS].movefunc = confuiRatingMove;
  confuiCreateRatingTable (gui);

  uiwcontFree (vbox);
  uiwcontFree (hbox);

  logProcEnd ("");
}

void
confuiCreateRatingTable (confuigui_t *gui)
{
  rating_t          *ratings;
  uivirtlist_t      *uivl;

  logProcBegin ();

  ratings = bdjvarsdfGet (BDJVDF_RATINGS);

  uivl = gui->tables [CONFUI_ID_RATINGS].uivl;
  uivlSetNumColumns (uivl, CONFUI_RATING_COL_MAX);
  uivlMakeColumnEntry (uivl, "rating", CONFUI_RATING_COL_RATING, 15, 30);
  uivlSetColumnHeading (uivl, CONFUI_RATING_COL_RATING,
      tagdefs [TAG_DANCERATING].shortdisplayname);
  uivlMakeColumnSpinboxNum (uivl, "rweight", CONFUI_RATING_COL_WEIGHT, 0.0, 100.0, 1.0, 5.0);
  /* CONTEXT: configuration: rating: title of the weight column */
  uivlSetColumnHeading (uivl, CONFUI_RATING_COL_WEIGHT, _("Weight"));

  uivlSetNumRows (uivl, ratingGetCount (ratings));
  gui->tables [CONFUI_ID_RATINGS].currcount = ratingGetCount (ratings);

  gui->tables [CONFUI_ID_RATINGS].callbacks [CONFUI_RATING_CB_WEIGHT] =
      callbackInit (confuiRatingChangeCB, gui, NULL);
  uivlSetEntryValidation (uivl, CONFUI_RATING_COL_RATING,
      confuiRatingEntryChangeCB, gui);
  uivlSetSpinboxChangeCallback (uivl, CONFUI_RATING_COL_WEIGHT,
      gui->tables [CONFUI_ID_RATINGS].callbacks [CONFUI_RATING_CB_WEIGHT]);

  uivlSetRowFillCallback (uivl, confuiRatingFillRow, gui);
  uivlDisplay (uivl);

  logProcEnd ("");
}

/* internal routines */

static void
confuiRatingSave (confuigui_t *gui)
{
  rating_t      *ratings;
  ilist_t       *ratinglist;
  ilistidx_t    count;

  logProcBegin ();

  if (gui->tables [CONFUI_ID_RATINGS].changed == false) {
    return;
  }

  ratings = bdjvarsdfGet (BDJVDF_RATINGS);

  ratinglist = ilistAlloc ("rating-save", LIST_ORDERED);
  count = ratingGetCount (ratings);
  for (int rowidx = 0; rowidx < count; ++rowidx) {
    const char  *ratingdisp;
    int         weight;

    ratingdisp = ratingGetRating (ratings, rowidx);
    weight = ratingGetWeight (ratings, rowidx);
    ilistSetStr (ratinglist, rowidx, RATING_RATING, ratingdisp);
    ilistSetNum (ratinglist, rowidx, RATING_WEIGHT, weight);
  }

  ratingSave (ratings, ratinglist);
  ilistFree (ratinglist);
  logProcEnd ("");
}

static void
confuiRatingFillRow (void *udata, uivirtlist_t *vl, int32_t rownum)
{
  confuigui_t   *gui = udata;
  uivirtlist_t  *uivl;
  rating_t      *ratings;
  const char    *ratingdisp;
  int           weight;

  uivl = gui->tables [CONFUI_ID_RATINGS].uivl;
  ratings = bdjvarsdfGet (BDJVDF_RATINGS);

  if (rownum >= ratingGetCount (ratings)) {
    return;
  }

  gui->inchange = true;

  ratingdisp = ratingGetRating (ratings, rownum);
  weight = ratingGetWeight (ratings, rownum);
  uivlSetRowColumnStr (uivl, rownum, CONFUI_RATING_COL_RATING, ratingdisp);
  uivlSetRowColumnNum (uivl, rownum, CONFUI_RATING_COL_WEIGHT, weight);
  if (rownum == 0) {
    /* the first entry field, if displayed, is read-only */
    uivlSetRowColumnEditable (uivl, rownum, CONFUI_STATUS_COL_STATUS, UIWIDGET_DISABLE);
  } else {
    uivlSetRowColumnEditable (uivl, rownum, CONFUI_STATUS_COL_STATUS, UIWIDGET_ENABLE);
  }

  gui->inchange = false;
}

static bool
confuiRatingChangeCB (void *udata)
{
  confuigui_t   *gui = udata;
  uivirtlist_t  *uivl;
  int32_t       rownum;
  int           weight;
  rating_t      *ratings;

  if (gui->inchange) {
    return UICB_CONT;
  }

  ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  uivl = gui->tables [CONFUI_ID_RATINGS].uivl;
  rownum = uivlGetCurrSelection (uivl);
  weight = uivlGetRowColumnNum (uivl, rownum, CONFUI_RATING_COL_WEIGHT);
  ratingSetWeight (ratings, rownum, weight);

  gui->tables [CONFUI_ID_RATINGS].changed = true;
  return UICB_CONT;
}

static int
confuiRatingEntryChangeCB (uiwcont_t *entry, const char *label, void *udata)
{
  confuigui_t   *gui = udata;
  uivirtlist_t  *uivl;
  int32_t       rownum;
  const char    *ratingdisp;
  rating_t      *ratings;

  if (gui->inchange) {
    return UIENTRY_OK;
  }

  ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  uivl = gui->tables [CONFUI_ID_RATINGS].uivl;
  rownum = uivlGetCurrSelection (uivl);
  ratingdisp = uivlGetRowColumnEntry (uivl, rownum, CONFUI_RATING_COL_RATING);
  ratingSetRating (ratings, rownum, ratingdisp);
  gui->tables [CONFUI_ID_RATINGS].changed = true;
  return UIENTRY_OK;
}

static void
confuiRatingAdd (confuigui_t *gui)
{
  rating_t      *ratings;
  int           count;
  uivirtlist_t  *uivl;

  ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  uivl = gui->tables [CONFUI_ID_RATINGS].uivl;

  count = ratingGetCount (ratings);
  /* CONTEXT: configuration: rating name that is set when adding a new rating */
  ratingSetRating (ratings, count, _("New Rating"));
  ratingSetWeight (ratings, count, 0);
  count += 1;
  uivlSetNumRows (uivl, count);
  gui->tables [CONFUI_ID_RATINGS].currcount = count;
  uivlPopulate (uivl);
  uivlSetSelection (uivl, count - 1);
}

static void
confuiRatingRemove (confuigui_t *gui, ilistidx_t delidx)
{
  rating_t      *ratings;
  uivirtlist_t  *uivl;
  int           count;
  bool          docopy = false;

  ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  uivl = gui->tables [CONFUI_ID_RATINGS].uivl;
  count = ratingGetCount (ratings);

  for (int idx = 0; idx < count - 1; ++idx) {
    if (idx == delidx) {
      docopy = true;
    }
    if (docopy) {
      const char  *ratingdisp;
      int         weight;

      ratingdisp = ratingGetRating (ratings, idx + 1);
      weight = ratingGetWeight (ratings, idx + 1);
      ratingSetRating (ratings, idx, ratingdisp);
      ratingSetWeight (ratings, idx, weight);
    }
  }

  ratingDeleteLast (ratings);
  count = ratingGetCount (ratings);
  uivlSetNumRows (uivl, count);
  gui->tables [CONFUI_ID_RATINGS].currcount = count;
  uivlPopulate (uivl);
}

static void
confuiRatingMove (confuigui_t *gui, ilistidx_t idx, int dir)
{
  rating_t      *ratings;
  uivirtlist_t  *uivl;
  ilistidx_t    toidx;
  char          *ratingdisp;
  int           weight;

  ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  uivl = gui->tables [CONFUI_ID_RATINGS].uivl;

  toidx = idx;
  if (dir == CONFUI_MOVE_PREV) {
    toidx -= 1;
    uivlMoveSelection (uivl, VL_DIR_PREV);
  }
  if (dir == CONFUI_MOVE_NEXT) {
    toidx += 1;
    uivlMoveSelection (uivl, VL_DIR_NEXT);
  }

  ratingdisp = mdstrdup (ratingGetRating (ratings, toidx));
  weight = ratingGetWeight (ratings, toidx);
  ratingSetRating (ratings, toidx, ratingGetRating (ratings, idx));
  ratingSetWeight (ratings, toidx, ratingGetWeight (ratings, idx));
  ratingSetRating (ratings, idx, ratingdisp);
  ratingSetWeight (ratings, idx, weight);
  dataFree (ratingdisp);

  uivlPopulate (uivl);

  return;
}
