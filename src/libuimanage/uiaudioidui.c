/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include "audioid.h"
#include "bdj4intl.h"
#include "bdj4ui.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "callback.h"
#include "dance.h"
#include "level.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "pathbld.h"
#include "pathdisp.h"
#include "slist.h"
#include "song.h"
#include "songutil.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"
#include "ui.h"
#include "uiaudioid.h"
#include "uisong.h"
#include "uisongsel.h"
#include "uivirtlist.h"
#include "uivlutil.h"

enum {
  UIAUDID_SEL_CURR,
  UIAUDID_SEL_NEW,
};

enum {
  UIAUDID_INIT_DISP_SZ = 4,
  UIAUDID_NO_DUR = -1,
};

typedef struct {
  int             tagidx;
  uiwcont_t       *currrb;
  uiwcont_t       *selrb;
  callback_t      *callback;
  bool            selection : 1;
} uiaudioiditem_t;

enum {
  UIAUDID_CB_FIRST,
  UIAUDID_CB_SAVE,
  UIAUDID_CB_PREV,
  UIAUDID_CB_NEXT,
  UIAUDID_CB_KEYB,
  UIAUDID_CB_ROW_SELECT,
  UIAUDID_CB_MAX,
};

enum {
  UIAUDID_MAIN_TIMER = 40,
};

enum {
  UIAUDID_W_BUTTON_FIRST,
  UIAUDID_W_BUTTON_PREV,
  UIAUDID_W_BUTTON_NEXT,
  UIAUDID_W_BUTTON_SAVE,
  UIAUDID_W_EDIT_ALL,
  UIAUDID_W_PARENT_WIN,
  UIAUDID_W_MAIN_VBOX,
  UIAUDID_W_AUDIOID_IMG,
  UIAUDID_W_FILE_DISP,
  UIAUDID_W_PANED_WINDOW,
  UIAUDID_W_KEY_HNDLR,
  UIAUDID_W_MB_LOGO,
  UIAUDID_W_MAX,
};

enum {
  UIAUDID_SZGRP_LABEL,
  UIAUDID_SZGRP_ITEM_COL_A,
  UIAUDID_SZGRP_ITEM_COL_B,
  UIAUDID_SZGRP_ITEM_A,
  UIAUDID_SZGRP_ITEM_B,
  UIAUDID_SZGRP_ITEM_C,
  UIAUDID_SZGRP_ITEM_D,
  UIAUDID_SZGRP_MAX,
};

typedef struct aid_internal {
  uiwcont_t           *wcont [UIAUDID_W_MAX];
  uivirtlist_t        *uivl;
  uiwcont_t           *szgrp [UIAUDID_SZGRP_MAX];
  callback_t          *callbacks [UIAUDID_CB_MAX];
  song_t              *song;
  uiaudioiditem_t     *items;
  nlist_t             *currlist;
  slist_t             *dlsellist;     // display-list
  slist_t             *itemsellist;
  nlist_t             *datalist;
  dbidx_t             dbidx;
  int                 itemcount;
  int                 colcount;
  int                 rowcount;
  int                 durationcol;
  /* selectedrow is which row has been selected by the user */
  int                 selectedrow;
  /* fillrow is used during the set-display-list processing */
  int                 fillrow;
  int                 paneposition;
  bool                repeating : 1;
  bool                insave : 1;
  bool                inchange : 1;
} aid_internal_t;

static void uiaudioidAddItemDisplay (uiaudioid_t *uiaudioid, uiwcont_t *col);
static void uiaudioidAddItem (uiaudioid_t *uiaudioid, uiwcont_t *hbox, int tagidx, int idx);
static bool uiaudioidSaveCallback (void *udata);
static bool uiaudioidFirstSelection (void *udata);
static bool uiaudioidPreviousSelection (void *udata);
static bool uiaudioidNextSelection (void *udata);
static bool uiaudioidKeyEvent (void *udata);
static void uiaudioidRowSelect (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx);
static void uiaudioidPopulateSelected (uiaudioid_t *uiaudioid, int idx);
static void uiaudioidDisplayDuration (uiaudioid_t *uiaudioid, song_t *song, nlist_t *tdlist);
static void uiaudioidFillRow (void *udata, uivirtlist_t *uivl, int32_t rownum);
static void uiaudioidCopyData (nlist_t *dlist, nlist_t *ndlist, tagdefkey_t tagidx);

void
uiaudioidUIInit (uiaudioid_t *uiaudioid)
{
  aid_internal_t  *audioidint;
  int             col;
  slistidx_t      seliteridx;
  int             tagidx;

  logProcBegin ();

  audioidint = mdmalloc (sizeof (aid_internal_t));
  audioidint->uivl = NULL;
  audioidint->itemcount = 0;
  audioidint->items = NULL;
  audioidint->currlist = nlistAlloc ("curr-list", LIST_ORDERED, NULL);
  audioidint->dbidx = -1;
  audioidint->datalist = NULL;
  audioidint->colcount = 0;
  audioidint->rowcount = 0;
  audioidint->selectedrow = -1;
  audioidint->fillrow = 0;
  audioidint->repeating = false;
  audioidint->insave = false;
  audioidint->inchange = false;

  audioidint->dlsellist = dispselGetList (uiaudioid->dispsel, DISP_SEL_AUDIOID_LIST);
  audioidint->itemsellist = dispselGetList (uiaudioid->dispsel, DISP_SEL_AUDIOID);
  audioidint->paneposition = nlistGetNum (uiaudioid->options, MANAGE_AUDIOID_PANE_POSITION);
  audioidint->colcount = slistGetCount (audioidint->dlsellist);

  audioidint->durationcol = UIAUDID_NO_DUR;
  col = 0;
  slistStartIterator (audioidint->dlsellist, &seliteridx);
  while ((tagidx = slistIterateValueNum (audioidint->dlsellist, &seliteridx)) >= 0) {
    if (tagidx == TAG_DURATION) {
      audioidint->durationcol = col;
      break;
    }
    ++col;
  }

  for (int i = 0; i < UIAUDID_CB_MAX; ++i) {
    audioidint->callbacks [i] = NULL;
  }
  for (int i = 0; i < UIAUDID_SZGRP_MAX; ++i) {
    audioidint->szgrp [i] = uiCreateSizeGroupHoriz ();
  }
  for (int i = 0; i < UIAUDID_W_MAX; ++i) {
    audioidint->wcont [i] = NULL;
  }

  uiaudioid->audioidInternalData = audioidint;
  logProcEnd ("");
}

void
uiaudioidUIFree (uiaudioid_t *uiaudioid)
{
  aid_internal_t *audioidint;


  logProcBegin ();
  if (uiaudioid == NULL) {
    logProcEnd ("null");
    return;
  }

  audioidint = uiaudioid->audioidInternalData;

  if (audioidint != NULL) {
    uivlFree (audioidint->uivl);
    nlistFree (audioidint->currlist);

    uiWidgetClearPersistent (audioidint->wcont [UIAUDID_W_MB_LOGO]);

    for (int count = 0; count < audioidint->itemcount; ++count) {
      callbackFree (audioidint->items [count].callback);
      uiwcontFree (audioidint->items [count].currrb);
      uiwcontFree (audioidint->items [count].selrb);
    }

    for (int i = 0; i < UIAUDID_SZGRP_MAX; ++i) {
      uiwcontFree (audioidint->szgrp [i]);
    }
    for (int i = 0; i < UIAUDID_W_MAX; ++i) {
      if (i == UIAUDID_W_PARENT_WIN) {
        continue;
      }
      uiwcontFree (audioidint->wcont [i]);
    }
    for (int i = 0; i < UIAUDID_CB_MAX; ++i) {
      callbackFree (audioidint->callbacks [i]);
    }

    nlistFree (audioidint->datalist);
    dataFree (audioidint->items);
    mdfree (audioidint);
    uiaudioid->audioidInternalData = NULL;
  }

  logProcEnd ("");
}

void
uiaudioidSavePanePosition (uiaudioid_t *uiaudioid)
{
  aid_internal_t    *audioidint;

  audioidint = uiaudioid->audioidInternalData;

  audioidint->paneposition =
      uiPanedWindowGetPosition (audioidint->wcont [UIAUDID_W_PANED_WINDOW]);
  nlistSetNum (uiaudioid->options, MANAGE_AUDIOID_PANE_POSITION,
      audioidint->paneposition);
}


uiwcont_t *
uiaudioidBuildUI (uiaudioid_t *uiaudioid, uisongsel_t *uisongsel,
    uiwcont_t *parentwin, uiwcont_t *statusMsg)
{
  aid_internal_t    *audioidint;
  uiwcont_t         *pw;
  uiwcont_t         *hbox;
  uiwcont_t         *vbox;
  uiwcont_t         *uiwidgetp;
  uiwcont_t         *col;
  char              tbuff [MAXPATHLEN];
  uivirtlist_t      *uivl;

  logProcBegin ();

  uiaudioid->statusMsg = statusMsg;
  uiaudioid->uisongsel = uisongsel;
  audioidint = uiaudioid->audioidInternalData;
  audioidint->wcont [UIAUDID_W_PARENT_WIN] = parentwin;

  audioidint->wcont [UIAUDID_W_MAIN_VBOX] = uiCreateVertBox ();
  uiWidgetExpandHoriz (audioidint->wcont [UIAUDID_W_MAIN_VBOX]);

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiWidgetAlignHorizFill (hbox);
  uiBoxPackStart (audioidint->wcont [UIAUDID_W_MAIN_VBOX], hbox);

  audioidint->callbacks [UIAUDID_CB_FIRST] = callbackInit (
      uiaudioidFirstSelection, uiaudioid, "audioid: first");
  uiwidgetp = uiCreateButton (audioidint->callbacks [UIAUDID_CB_FIRST],
      /* CONTEXT: audio identification: first song */
      _("First"), NULL);
  uiBoxPackStart (hbox, uiwidgetp);
  audioidint->wcont [UIAUDID_W_BUTTON_FIRST] = uiwidgetp;

  audioidint->callbacks [UIAUDID_CB_PREV] = callbackInit (
      uiaudioidPreviousSelection, uiaudioid, "audioid: previous");
  uiwidgetp = uiCreateButton (audioidint->callbacks [UIAUDID_CB_PREV],
      /* CONTEXT: audio identification: previous song */
      _("Previous"), NULL);
  uiButtonSetRepeat (uiwidgetp, REPEAT_TIME);
  uiBoxPackStart (hbox, uiwidgetp);
  audioidint->wcont [UIAUDID_W_BUTTON_PREV] = uiwidgetp;

  audioidint->callbacks [UIAUDID_CB_NEXT] = callbackInit (
      uiaudioidNextSelection, uiaudioid, "audioid: next");
  uiwidgetp = uiCreateButton (audioidint->callbacks [UIAUDID_CB_NEXT],
      /* CONTEXT: audio identification: next song */
      _("Next"), NULL);
  uiButtonSetRepeat (uiwidgetp, REPEAT_TIME);
  uiBoxPackStart (hbox, uiwidgetp);
  audioidint->wcont [UIAUDID_W_BUTTON_NEXT] = uiwidgetp;

  audioidint->callbacks [UIAUDID_CB_SAVE] = callbackInit (
      uiaudioidSaveCallback, uiaudioid, "audioid: save");
  uiwidgetp = uiCreateButton (audioidint->callbacks [UIAUDID_CB_SAVE],
      /* CONTEXT: audio identification: save data */
      _("Save"), NULL);
  uiBoxPackEnd (hbox, uiwidgetp);
  audioidint->wcont [UIAUDID_W_BUTTON_SAVE] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetSetMarginEnd (uiwidgetp, 6);
  uiWidgetAddClass (uiwidgetp, DARKACCENT_CLASS);
  audioidint->wcont [UIAUDID_W_EDIT_ALL] = uiwidgetp;

  uiwcontFree (hbox);

  /* begin line */

  /* audio-identification logo, modified indicator, */
  /* copy button, file label, filename */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiWidgetAlignHorizFill (hbox);
  uiBoxPackStart (audioidint->wcont [UIAUDID_W_MAIN_VBOX], hbox);

  audioidint->wcont [UIAUDID_W_AUDIOID_IMG] = uiImageNew ();
  uiImageClear (audioidint->wcont [UIAUDID_W_AUDIOID_IMG]);
  uiWidgetSetSizeRequest (audioidint->wcont [UIAUDID_W_AUDIOID_IMG], 24, -1);
  uiBoxPackStart (hbox, audioidint->wcont [UIAUDID_W_AUDIOID_IMG]);
  uiWidgetSetMarginStart (audioidint->wcont [UIAUDID_W_AUDIOID_IMG], 1);

  /* CONTEXT: audio identification: label for displaying the audio file path */
  uiwidgetp = uiCreateColonLabel (_("File"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetState (uiwidgetp, UIWIDGET_DISABLE);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("");
  uiLabelEllipsizeOn (uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, DARKACCENT_CLASS);
  uiLabelSetSelectable (uiwidgetp);
  audioidint->wcont [UIAUDID_W_FILE_DISP] = uiwidgetp;

  uiwcontFree (hbox);

  pw = uiPanedWindowCreateVert ();
  uiWidgetExpandHoriz (pw);
  uiWidgetAlignHorizFill (pw);
  uiBoxPackStartExpand (audioidint->wcont [UIAUDID_W_MAIN_VBOX], pw);
  uiWidgetAddClass (pw, ACCENT_CLASS);
  audioidint->wcont [UIAUDID_W_PANED_WINDOW] = pw;

  vbox = uiCreateVertBox ();
  uiPanedWindowPackStart (pw, vbox);

  /* match listing */

  uivl = uivlCreate ("audioid", NULL, vbox, UIAUDID_INIT_DISP_SZ,
      VL_NO_WIDTH, VL_ENABLE_KEYS);
  audioidint->uivl = uivl;

  uivlSetUseListingFont (uivl);
  uivlSetNumColumns (uivl, audioidint->colcount);
  uivlAddDisplayColumns (uivl, audioidint->dlsellist, 0);
  uivlSetRowFillCallback (uivl, uiaudioidFillRow, audioidint);

  /* the first row is highlighted, it always displays the current song data */
  uivlSetRowClass (audioidint->uivl, 0, ACCENT_CLASS);
  /* lock the first row display */
  uivlSetRowLock (audioidint->uivl, 0);;

  uiwcontFree (vbox);

  uivlSetNumRows (audioidint->uivl, 0);
  uivlDisplay (audioidint->uivl);
  uivlSetSelectChgCallback (audioidint->uivl, uiaudioidRowSelect, uiaudioid);

  /* current/selected box */

  uiwidgetp = uiCreateScrolledWindow (300);
  uiWidgetExpandHoriz (uiwidgetp);
  uiWidgetExpandVert (uiwidgetp);
  uiPanedWindowPackEnd (pw, uiwidgetp);

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiWidgetAlignHorizFill (hbox);
  uiWindowPackInWindow (uiwidgetp, hbox);

  uiwcontFree (uiwidgetp);

  /* the items must all be alloc'd beforehand so that the callback */
  /* pointer is static */
  audioidint->itemcount = slistGetCount (audioidint->itemsellist);
  audioidint->items = mdmalloc (sizeof (uiaudioiditem_t) * audioidint->itemcount);
  for (int i = 0; i < audioidint->itemcount; ++i) {
    audioidint->items [i].tagidx = 0;
    audioidint->items [i].currrb = NULL;
    audioidint->items [i].selrb = NULL;
    audioidint->items [i].callback = NULL;
  }

  col = uiCreateVertBox ();
  uiWidgetExpandHoriz (col);
  uiWidgetExpandVert (col);
  uiBoxPackStartExpand (hbox, col);
  uiWidgetSetAllMargins (col, 4);

  uiwcontFree (hbox);

  /* headings */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (col, hbox);

  uiwidgetp = uiCreateLabel (" ");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginEnd (uiwidgetp, 4);
  uiSizeGroupAdd (audioidint->szgrp [UIAUDID_SZGRP_LABEL], uiwidgetp);
  uiwcontFree (uiwidgetp);

  snprintf (tbuff, sizeof (tbuff), "%s bold", bdjoptGetStr (OPT_MP_UIFONT));

  /* CONTEXT: audio identification: the data for the current song */
  uiwidgetp = uiCreateLabel (_("Current"));
  uiLabelSetFont (uiwidgetp, tbuff);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiWidgetSetMarginEnd (uiwidgetp, 4);
  uiSizeGroupAdd (audioidint->szgrp [UIAUDID_SZGRP_ITEM_COL_A], uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel (" ");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  /* CONTEXT: audio identification: the data for the selected matched song */
  uiwidgetp = uiCreateLabel (_("Selected"));
  uiLabelSetFont (uiwidgetp, tbuff);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiSizeGroupAdd (audioidint->szgrp [UIAUDID_SZGRP_ITEM_COL_B], uiwidgetp);
  uiwcontFree (uiwidgetp);
  uiwcontFree (hbox);

  uiaudioidAddItemDisplay (uiaudioid, col);

  uiwcontFree (col);

  audioidint->wcont [UIAUDID_W_KEY_HNDLR] = uiEventAlloc ();
  audioidint->callbacks [UIAUDID_CB_KEYB] = callbackInit (
      uiaudioidKeyEvent, uiaudioid, NULL);
  uiEventSetKeyCallback (audioidint->wcont [UIAUDID_W_KEY_HNDLR],
      audioidint->wcont [UIAUDID_W_MAIN_VBOX],
      audioidint->callbacks [UIAUDID_CB_KEYB]);

  if (audioidint->paneposition >= 0) {
    uiPanedWindowSetPosition (pw, audioidint->paneposition);
  }

  pathbldMakePath (tbuff, sizeof (tbuff), "musicbrainz-logo", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DIR_IMG);
  audioidint->wcont [UIAUDID_W_MB_LOGO] = uiImageFromFile (tbuff);
  uiImageConvertToPixbuf (audioidint->wcont [UIAUDID_W_MB_LOGO]);
  uiWidgetMakePersistent (audioidint->wcont [UIAUDID_W_MB_LOGO]);

  logProcEnd ("");
  return audioidint->wcont [UIAUDID_W_MAIN_VBOX];
}

void
uiaudioidLoadData (uiaudioid_t *uiaudioid, song_t *song, dbidx_t dbidx)
{
  aid_internal_t   *audioidint;
  char            *tval;
  int32_t         val;
  double          dval;
  const char      *data;

  logProcBegin ();

  if (uiaudioid == NULL) {
    return;
  }

  audioidint = uiaudioid->audioidInternalData;
  audioidint->song = song;
  audioidint->dbidx = dbidx;

  tval = uisongGetDisplay (song, TAG_URI, &val, &dval);
  uiLabelSetText (audioidint->wcont [UIAUDID_W_FILE_DISP], tval);
  dataFree (tval);
  tval = NULL;

  uiImageClear (audioidint->wcont [UIAUDID_W_AUDIOID_IMG]);
  data = songGetStr (song, TAG_RECORDING_ID);
  if (data != NULL && *data) {
    uiImageSetFromPixbuf (audioidint->wcont [UIAUDID_W_AUDIOID_IMG], audioidint->wcont [UIAUDID_W_MB_LOGO]);
  }

  for (int count = 0; count < audioidint->itemcount; ++count) {
    int         tagidx = audioidint->items [count].tagidx;
    char        tmp [40];
    const char  *ttval;

    tval = uisongGetDisplay (song, tagidx, &val, &dval);
    if (tval == NULL && val != LIST_VALUE_INVALID) {
      snprintf (tmp, sizeof (tmp), "%" PRId32, val);
      dataFree (tval);
      tval = mdstrdup (tmp);
    }
    if (tagidx == TAG_DURATION) {
      dataFree (tval);
      tval = songDisplayString (song, tagidx, SONG_UNADJUSTED_DURATION);
    }
    nlistSetStr (audioidint->currlist, tagidx, tval);
    uiToggleButtonSetValue (audioidint->items [count].currrb, UI_TOGGLE_BUTTON_ON);
    uiToggleButtonSetValue (audioidint->items [count].selrb, UI_TOGGLE_BUTTON_OFF);
    ttval = tval;
    if (tval == NULL) {
      ttval = "";
    }
    uiToggleButtonSetText (audioidint->items [count].currrb, ttval);

    if (tagdefs [tagidx].valueType == VALUE_STR) {
      uiWidgetSetTooltip (audioidint->items [count].currrb, ttval);
      /* gtk appears to re-allocate the radio button label, */
      /* so set the ellipsize on after setting the text value */
      uiToggleButtonEllipsize (audioidint->items [count].currrb);
    }
    dataFree (tval);
    tval = NULL;
  }

  if (audioidint->selectedrow > 0 &&
      audioidint->selectedrow < audioidint->rowcount) {
    /* reset the selection in case this is a save/reload */
    uivlSetSelection (audioidint->uivl, audioidint->selectedrow);
  }

  logProcEnd ("");
}

void
uiaudioidResetItemDisplay (uiaudioid_t *uiaudioid)
{
  aid_internal_t  *audioidint;

  if (uiaudioid == NULL) {
    return;
  }

  audioidint = uiaudioid->audioidInternalData;

  nlistFree (audioidint->datalist);
  audioidint->datalist = nlistAlloc ("uiaudioid-disp", LIST_UNORDERED, NULL);

  /* blank the field selection */
  for (int count = 0; count < audioidint->itemcount; ++count) {
    uiToggleButtonSetText (audioidint->items [count].selrb, "");
  }

  audioidint->selectedrow = -1;
  /* leave the first row (0) available for the original song display */
  /* if any data is returned, the first row will be populated */
  audioidint->fillrow = 1;
}

void
uiaudioidSetItemDisplay (uiaudioid_t *uiaudioid, nlist_t *dlist)
{
  aid_internal_t  *audioidint;
  int             tagidx;
  slistidx_t      seliteridx;
  nlist_t         *ndlist;

  if (uiaudioid == NULL) {
    return;
  }

  audioidint = uiaudioid->audioidInternalData;

  ndlist = nlistAlloc ("uiaudio-ndlist", LIST_UNORDERED, NULL);
  nlistSetSize (ndlist, nlistGetCount (dlist));

  /* all data must be cloned, the input data-list is temporary */
  nlistStartIterator (dlist, &seliteridx);
  while ((tagidx = nlistIterateKey (dlist, &seliteridx)) >= 0) {
    uiaudioidCopyData (dlist, ndlist, tagidx);
  }
  nlistSort (ndlist);

  nlistSetList (audioidint->datalist, audioidint->fillrow, ndlist);
  audioidint->fillrow += 1;
  logProcEnd ("");
}

void
uiaudioidFinishItemDisplay (uiaudioid_t *uiaudioid)
{
  aid_internal_t  *audioidint;
  nlist_t         *tdlist;

  audioidint = uiaudioid->audioidInternalData;

  if (audioidint->fillrow == 1) {
    audioidint->rowcount = 0;
  } else {
    audioidint->rowcount = audioidint->fillrow;
    tdlist = uisongGetDisplayList (audioidint->itemsellist,
        audioidint->dlsellist, audioidint->song);
    nlistSetList (audioidint->datalist, 0, tdlist);
    uiaudioidDisplayDuration (uiaudioid, audioidint->song, tdlist);
  }

  nlistSort (audioidint->datalist);
  uivlSetNumRows (audioidint->uivl, audioidint->rowcount);
}

void
uiaudioidUIMainLoop (uiaudioid_t *uiaudioid)
{
  aid_internal_t  *audioidint;
  bool            repeat;

  audioidint = uiaudioid->audioidInternalData;

  audioidint->repeating = false;
  repeat = uiButtonCheckRepeat (audioidint->wcont [UIAUDID_W_BUTTON_NEXT]);
  if (repeat) {
    audioidint->repeating = true;
  }
  repeat = uiButtonCheckRepeat (audioidint->wcont [UIAUDID_W_BUTTON_PREV]);
  if (repeat) {
    audioidint->repeating = true;
  }
  return;
}

bool
uiaudioidIsRepeating (uiaudioid_t *uiaudioid)
{
  aid_internal_t  *audioidint;

  if (uiaudioid == NULL) {
    return false;
  }

  audioidint = uiaudioid->audioidInternalData;

  return audioidint->repeating;
}

bool
uiaudioidInSave (uiaudioid_t *uiaudioid)
{
  aid_internal_t  *audioidint;

  if (uiaudioid == NULL) {
    return false;
  }

  audioidint = uiaudioid->audioidInternalData;

  return audioidint->insave;
}

/* internal routines */

static void
uiaudioidAddItemDisplay (uiaudioid_t *uiaudioid, uiwcont_t *col)
{
  slistidx_t      dsiteridx;
  aid_internal_t  *audioidint;
  int             tagidx;
  int             count;

  logProcBegin ();
  audioidint = uiaudioid->audioidInternalData;

  count = 0;
  slistStartIterator (audioidint->itemsellist, &dsiteridx);
  while ((tagidx = slistIterateValueNum (audioidint->itemsellist, &dsiteridx)) >= 0) {
    uiwcont_t   *hbox;

    if (tagidx < 0 || tagidx >= TAG_KEY_MAX) {
      continue;
    }
    if (! tagdefs [tagidx].isAudioID) {
      continue;
    }

    hbox = uiCreateHorizBox ();
    uiWidgetExpandHoriz (hbox);
    uiBoxPackStart (col, hbox);

    uiaudioidAddItem (uiaudioid, hbox, tagidx, count);
    audioidint->items [count].tagidx = tagidx;
    ++count;

    uiwcontFree (hbox);
  }

  logProcEnd ("");
}

static void
uiaudioidAddItem (uiaudioid_t *uiaudioid, uiwcont_t *hbox, int tagidx, int idx)
{
  uiwcont_t       *uiwidgetp;
  uiwcont_t       *rb;
  aid_internal_t  *audioidint;

  logProcBegin ();
  audioidint = uiaudioid->audioidInternalData;

  /* line: label, curr-rb, sel-rb */

  uiwidgetp = uiCreateColonLabel (tagdefs [tagidx].displayname);
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginEnd (uiwidgetp, 4);
  uiSizeGroupAdd (audioidint->szgrp [UIAUDID_SZGRP_LABEL], uiwidgetp);
  uiwcontFree (uiwidgetp);

  rb = uiCreateRadioButton (NULL, "", UI_TOGGLE_BUTTON_ON);
  /* ellipsize set after text is set */
  uiBoxPackStartExpand (hbox, rb);
  uiSizeGroupAdd (audioidint->szgrp [UIAUDID_SZGRP_ITEM_COL_A], rb);
  if (idx + UIAUDID_SZGRP_ITEM_A < UIAUDID_SZGRP_MAX) {
    uiSizeGroupAdd (audioidint->szgrp [idx + UIAUDID_SZGRP_ITEM_A], rb);
  }
  audioidint->items [idx].currrb = rb;

  uiwidgetp = uiCreateLabel (" ");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateRadioButton (rb, "", UI_TOGGLE_BUTTON_OFF);
  /* ellipsize set after text is set */
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiSizeGroupAdd (audioidint->szgrp [UIAUDID_SZGRP_ITEM_COL_B], uiwidgetp);
  if (idx + UIAUDID_SZGRP_ITEM_A < UIAUDID_SZGRP_MAX) {
    uiSizeGroupAdd (audioidint->szgrp [idx + UIAUDID_SZGRP_ITEM_A], uiwidgetp);
  }
  audioidint->items [idx].selrb = uiwidgetp;

  logProcEnd ("");
}

static bool
uiaudioidSaveCallback (void *udata)
{
  uiaudioid_t       *uiaudioid = udata;
  aid_internal_t    *audioidint = NULL;
  nlist_t           *dlist;
  const char        *val;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: audioid: save");

  audioidint = uiaudioid->audioidInternalData;

  uiLabelSetText (uiaudioid->statusMsg, "");
  if (audioidint->selectedrow < 0) {
    return UICB_CONT;
  }

  dlist = nlistGetList (audioidint->datalist, audioidint->selectedrow);
  for (int count = 0; count < audioidint->itemcount; ++count) {
    int     tagidx;

    tagidx = audioidint->items [count].tagidx;
    if (uiToggleButtonIsActive (audioidint->items [count].selrb)) {
      int32_t     nval;

      val = nlistGetStr (dlist, tagidx);
      nval = LIST_VALUE_INVALID;
      if (val != NULL && *val) {
        if (tagdefs [tagidx].valueType == VALUE_NUM) {
          nval = atol (val);
          songSetNum (audioidint->song, tagidx, nval);
        } else {
          songSetStr (audioidint->song, tagidx, val);
        }
      }
    }
  }

  /* on a save, also save the recording id and work id if present */
  val = nlistGetStr (dlist, TAG_RECORDING_ID);
  if (val != NULL && *val) {
    songSetStr (audioidint->song, TAG_RECORDING_ID, val);
  }
  val = nlistGetStr (dlist, TAG_WORK_ID);
  if (val != NULL && *val) {
    songSetStr (audioidint->song, TAG_WORK_ID, val);
  }

  audioidint->insave = true;

  if (uiaudioid->savecb != NULL) {
    /* the callback re-loads the song editor and audio id */
    callbackHandlerI (uiaudioid->savecb, audioidint->dbidx);
  }

  audioidint->insave = false;

  logProcEnd ("");
  return UICB_CONT;
}

static bool
uiaudioidFirstSelection (void *udata)
{
  uiaudioid_t  *uiaudioid = udata;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: audioid: first");
  uisongselFirstSelection (uiaudioid->uisongsel);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
uiaudioidPreviousSelection (void *udata)
{
  uiaudioid_t  *uiaudioid = udata;

  uiLabelSetText (uiaudioid->statusMsg, "");
  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: audioid: previous");
  uisongselPreviousSelection (uiaudioid->uisongsel);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
uiaudioidNextSelection (void *udata)
{
  uiaudioid_t  *uiaudioid = udata;

  uiLabelSetText (uiaudioid->statusMsg, "");
  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: audioid: next");
  uisongselNextSelection (uiaudioid->uisongsel);
  logProcEnd ("");
  return UICB_CONT;
}


static bool
uiaudioidKeyEvent (void *udata)
{
  uiaudioid_t    *uiaudioid = udata;
  aid_internal_t *audioidint;

  audioidint = uiaudioid->audioidInternalData;

  if (uiEventIsKeyPressEvent (audioidint->wcont [UIAUDID_W_KEY_HNDLR]) &&
      uiEventIsAudioPlayKey (audioidint->wcont [UIAUDID_W_KEY_HNDLR])) {
    uisongselPlayCallback (uiaudioid->uisongsel);
  }

  if (uiEventIsKeyPressEvent (audioidint->wcont [UIAUDID_W_KEY_HNDLR])) {
    if (uiEventIsControlPressed (audioidint->wcont [UIAUDID_W_KEY_HNDLR])) {
      if (uiEventIsKey (audioidint->wcont [UIAUDID_W_KEY_HNDLR], 'S')) {
        uiaudioidSaveCallback (uiaudioid);
        return UICB_STOP;
      }
      if (uiEventIsKey (audioidint->wcont [UIAUDID_W_KEY_HNDLR], 'N')) {
        uiaudioidNextSelection (uiaudioid);
        return UICB_STOP;
      }
      if (uiEventIsKey (audioidint->wcont [UIAUDID_W_KEY_HNDLR], 'P')) {
        uiaudioidPreviousSelection (uiaudioid);
        return UICB_STOP;
      }
    }
  }

  return UICB_CONT;
}

static void
uiaudioidRowSelect (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx)
{
  uiaudioid_t     * uiaudioid = udata;
  aid_internal_t  * audioidint;

  audioidint = uiaudioid->audioidInternalData;

  if (audioidint->inchange) {
    return;
  }

  audioidint->selectedrow = rownum;
  uiaudioidPopulateSelected (uiaudioid, rownum);

  return;
}

static void
uiaudioidPopulateSelected (uiaudioid_t *uiaudioid, int idx)
{
  aid_internal_t  *audioidint;
  nlist_t         *dlist = NULL;

  audioidint = uiaudioid->audioidInternalData;
  if (audioidint->datalist == NULL) {
    return;
  }

  if (idx < 0 || idx >= audioidint->rowcount) {
    return;
  }

  dlist = nlistGetList (audioidint->datalist, idx);

  for (int count = 0; count < audioidint->itemcount; ++count) {
    int         tagidx = audioidint->items [count].tagidx;
    const char  *ttval;

    uiToggleButtonSetValue (audioidint->items [count].currrb, UI_TOGGLE_BUTTON_ON);
    uiToggleButtonSetValue (audioidint->items [count].selrb, UI_TOGGLE_BUTTON_OFF);

    if (dlist == NULL) {
      uiToggleButtonSetText (audioidint->items [count].selrb, "");
    } else {
      const char  *tval;
      char        tmp [40];

      tval = nlistGetStr (dlist, tagidx);
      if (tagidx == TAG_DURATION) {
        long        dur = 0;

        /* duration must be converted */
        if (tval != NULL) {
          dur = atol (tval);
        }
        tmutilToMSD (dur, tmp, sizeof (tmp), 1);
        tval = tmp;
      }

      ttval = tval;
      if (tval == NULL) {
        ttval = "";
      }
      uiToggleButtonSetText (audioidint->items [count].selrb, ttval);
      if (tagdefs [tagidx].valueType == VALUE_STR) {
        uiWidgetSetTooltip (audioidint->items [count].selrb, ttval);
        /* gtk appears to re-allocate the radio button label, */
        /* so set the ellipsize on after setting the text value */
        uiToggleButtonEllipsize (audioidint->items [count].selrb);
      }
    }
  }
}

static void
uiaudioidDisplayDuration (uiaudioid_t *uiaudioid, song_t *song, nlist_t *tdlist)
{
  aid_internal_t  *audioidint;
  char            *tmp;

  audioidint = uiaudioid->audioidInternalData;

  if (audioidint->durationcol == UIAUDID_NO_DUR) {
    return;
  }

  tmp = songDisplayString (song, TAG_DURATION, SONG_UNADJUSTED_DURATION);
  nlistSetStr (tdlist, TAG_DURATION, tmp);
  dataFree (tmp);
}


static void
uiaudioidFillRow (void *udata, uivirtlist_t *uivl, int32_t rownum)
{
  aid_internal_t  *audioidint = udata;
  nlist_t         *dlist;
  slist_t         *dlsellist;
  slistidx_t      seliteridx;
  int             colcount;

  if (audioidint->datalist == NULL) {
    return;
  }

  dlist = nlistGetList (audioidint->datalist, rownum);
  if (dlist == NULL) {
    return;
  }
  dlsellist = audioidint->dlsellist;
  colcount = audioidint->colcount;

  slistStartIterator (dlsellist, &seliteridx);
  for (int col = 0; col < colcount; ++col) {
    int   tagidx;

    tagidx = slistIterateValueNum (dlsellist, &seliteridx);

    if (tagidx == TAG_AUDIOID_IDENT) {
      int         val;
      const char  *idstr;

      val = nlistGetNum (dlist, tagidx);
      idstr = "";
      switch (val) {
        case AUDIOID_ID_ACOUSTID: {
          idstr = "AcoustID";
          break;
        }
        case AUDIOID_ID_MB_LOOKUP: {
          idstr = "MusicBrainz";
          break;
        }
        case AUDIOID_ID_ACRCLOUD: {
          idstr = "ACRCloud";
          break;
        }
        default: {
          break;
        }
      }
      uivlSetRowColumnStr (audioidint->uivl, rownum, col, idstr);
    } else if (tagidx == TAG_AUDIOID_SCORE) {
      char    tmp [40];
      double  dval;

      dval = nlistGetDouble (dlist, tagidx);
      tmp [0] = '\0';
      if (dval > 0.01) {
        snprintf (tmp, sizeof (tmp), "%.1f", dval);
      }
      uivlSetRowColumnStr (audioidint->uivl, rownum, col, tmp);
    } else if (tagidx == TAG_DURATION) {
      const char  *str;
      char        tmp [40];
      int32_t     dur = 0;

      /* duration must be converted, except in the case of the original, */
      /* which has already been converted */
      str = nlistGetStr (dlist, tagidx);
      if (rownum == 0) {
        stpecpy (tmp, tmp + sizeof (tmp), str);
      } else {
        if (str != NULL) {
          dur = atol (str);
        }
        tmutilToMSD (dur, tmp, sizeof (tmp), 1);
      }
      uivlSetRowColumnStr (audioidint->uivl, rownum, col, tmp);
    } else {
      const char  *str;

      str = nlistGetStr (dlist, tagidx);
      if (str == NULL) {
        str = "";
      }
      uivlSetRowColumnStr (audioidint->uivl, rownum, col, str);
    }
  }
}

static void
uiaudioidCopyData (nlist_t *dlist, nlist_t *ndlist, tagdefkey_t tagidx)
{
  if (tagidx >= TAG_KEY_MAX) {
    /* the data-list returned from audio identification has some */
    /* specialized tag indices that are greater than tag-key-max */
    return;
  }

  if (tagidx == TAG_AUDIOID_IDENT) {
    int   val;

    val = nlistGetNum (dlist, tagidx);
    nlistSetNum (ndlist, tagidx, val);
  } else if (tagidx == TAG_AUDIOID_SCORE) {
    double  dval;

    dval = nlistGetDouble (dlist, tagidx);
    nlistSetDouble (ndlist, tagidx, dval);
  } else {
    const char  *str;

    str = nlistGetStr (dlist, tagidx);
    nlistSetStr (ndlist, tagidx, str);
  }
}
