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

static void confuiRatingSave (confuigui_t *gui);
static void confuiRatingFillRow (void *udata, uivirtlist_t *vl, int32_t rownum);
static bool confuiRatingChangeCB (void *udata);
static int confuiRatingEntryChangeCB (uiwcont_t *entry, const char *label, void *udata);

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
  gui->tables [CONFUI_ID_RATINGS].savefunc = confuiRatingSave;
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
  uivlMakeColumnEntry (uivl, CONFUI_RATING_COL_RATING, 15, 30);
  uivlMakeColumnSpinboxNum (uivl, CONFUI_RATING_COL_WEIGHT, 0.0, 100.0, 1.0, 5.0);
  uivlSetColumnHeading (uivl, CONFUI_RATING_COL_RATING,
      tagdefs [TAG_DANCERATING].shortdisplayname);
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
  /* the first entry field is read-only */
  uivlSetRowColumnReadonly (uivl, 0, CONFUI_RATING_COL_RATING);

  logProcEnd ("");
}

/* internal routines */

static void
confuiRatingSave (confuigui_t *gui)
{
  rating_t      *ratings;
  ilist_t       *ratinglist;
  uivirtlist_t  *uivl;

  logProcBegin ();

  if (gui->tables [CONFUI_ID_RATINGS].changed == false) {
    return;
  }

  ratings = bdjvarsdfGet (BDJVDF_RATINGS);

  uivl = gui->tables [CONFUI_ID_RATINGS].uivl;
  ratinglist = ilistAlloc ("rating-save", LIST_ORDERED);
  for (int rowidx = 0; rowidx < gui->tables [CONFUI_ID_RATINGS].currcount; ++rowidx) {
    const char  *ratingdisp;
    int         weight;

    ratingdisp = uivlGetRowColumnEntry (uivl, rowidx, CONFUI_RATING_COL_RATING);
    weight = uivlGetRowColumnNum (uivl, rowidx, CONFUI_RATING_COL_WEIGHT);
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
  confuigui_t *gui = udata;
  rating_t    *ratings;
  const char  *ratingdisp;
  int         weight;

  gui->inchange = true;

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

  gui->inchange = false;
}

static bool
confuiRatingChangeCB (void *udata)
{
  confuigui_t *gui = udata;

  if (gui->inchange) {
    return UICB_CONT;
  }

  gui->tables [CONFUI_ID_RATINGS].changed = true;
  return UICB_CONT;
}

static int
confuiRatingEntryChangeCB (uiwcont_t *entry, const char *label, void *udata)
{
  confuigui_t *gui = udata;

  if (gui->inchange) {
    return UIENTRY_OK;
  }

  gui->tables [CONFUI_ID_RATINGS].changed = true;
  return UIENTRY_OK;
}
