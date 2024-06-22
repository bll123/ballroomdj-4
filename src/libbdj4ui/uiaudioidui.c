/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
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
#include "uitreedisp.h"

enum {
  UIAUDID_SEL_CURR,
  UIAUDID_SEL_NEW,
};

enum {
  UIAUDID_COL_ELLIPSIZE,
  UIAUDID_COL_FONT,
  UIAUDID_COL_COLOR,
  UIAUDID_COL_COLOR_SET,
  UIAUDID_COL_IDX,
  UIAUDID_COL_MAX,
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
  UIAUDID_W_TREE,
  UIAUDID_W_MAX,
};

enum {
  UIAUDID_SZGRP_LABEL,
  UIAUDID_SZGRP_COL_A,
  UIAUDID_SZGRP_COL_B,
  UIAUDID_SZGRP_MAX,
};

typedef struct aid_internal {
  uiwcont_t           *wcont [UIAUDID_W_MAX];
  uiwcont_t           *szgrp [UIAUDID_SZGRP_MAX];
  callback_t          *callbacks [UIAUDID_CB_MAX];
  song_t              *song;
  uiaudioiditem_t     *items;
  nlist_t             *currlist;
  int                 *typelist;
  slist_t             *listsellist;
  slist_t             *sellist;
  nlist_t             *displaylist;
  dbidx_t             dbidx;
  int                 itemcount;
  int                 colcount;
  int                 treerowcount;
  int                 rowcount;
  /* selectedrow is which row has been selected by the user */
  int                 selectedrow;
  /* fillrow is used during the set-display-list processing */
  int                 fillrow;
  int                 paneposition;
  bool                selchgbypass : 1;
  bool                repeating : 1;
  bool                insave : 1;
} aid_internal_t;

static void uiaudioidAddDisplay (uiaudioid_t *uiaudioid, uiwcont_t *col);
static void uiaudioidAddItem (uiaudioid_t *uiaudioid, uiwcont_t *hbox, int tagidx);
static bool uiaudioidSaveCallback (void *udata);
static bool uiaudioidFirstSelection (void *udata);
static bool uiaudioidPreviousSelection (void *udata);
static bool uiaudioidNextSelection (void *udata);
static bool uiaudioidKeyEvent (void *udata);
static bool uiaudioidRowSelect (void *udata);
static void uiaudioidPopulateSelected (uiaudioid_t *uiaudioid, int idx);
static void uiaudioidDisplayTypeCallback (int type, void *udata);
static void uiaudioidSetSongDataCallback (int col, long num, const char *str, void *udata);
static void uiaudioidDisplayDuration (uiaudioid_t *uiaudioid, song_t *song);
static void uiaudioidBlankDisplayList (uiaudioid_t *uiaudioid);

void
uiaudioidUIInit (uiaudioid_t *uiaudioid)
{
  aid_internal_t  *audioidint;

  logProcBegin ();

  audioidint = mdmalloc (sizeof (aid_internal_t));
  audioidint->itemcount = 0;
  audioidint->items = NULL;
  audioidint->currlist = nlistAlloc ("curr-list", LIST_ORDERED, NULL);
  audioidint->dbidx = -1;
  audioidint->typelist = NULL;
  audioidint->displaylist = NULL;
  audioidint->colcount = 0;
  audioidint->treerowcount = 0;
  audioidint->rowcount = 0;
  audioidint->selectedrow = -1;
  audioidint->fillrow = 0;
  /* select-change bypass will be true until the data is first loaded */
  audioidint->selchgbypass = true;
  audioidint->repeating = false;
  audioidint->insave = false;
  audioidint->listsellist = dispselGetList (uiaudioid->dispsel, DISP_SEL_AUDIOID_LIST);
  audioidint->sellist = dispselGetList (uiaudioid->dispsel, DISP_SEL_AUDIOID);
  audioidint->paneposition = nlistGetNum (uiaudioid->options, MANAGE_AUDIOID_PANE_POSITION);

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
    nlistFree (audioidint->currlist);

    uiWidgetClearPersistent (audioidint->wcont [UIAUDID_W_MB_LOGO]);

    for (int count = 0; count < audioidint->itemcount; ++count) {
      callbackFree (audioidint->items [count].callback);
      uiwcontFree (audioidint->items [count].currrb);
      uiwcontFree (audioidint->items [count].selrb);
    }
    dataFree (audioidint->typelist);

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

    nlistFree (audioidint->displaylist);
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
  uiwcont_t         *uiwidgetp;
  int               count;
  uiwcont_t         *col;
  char              tbuff [MAXPATHLEN];

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
  uiWidgetSetClass (uiwidgetp, DARKACCENT_CLASS);
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
  uiWidgetSetMarginStart (audioidint->wcont [UIAUDID_W_AUDIOID_IMG], 1);
  uiBoxPackStart (hbox, audioidint->wcont [UIAUDID_W_AUDIOID_IMG]);

  /* CONTEXT: audio identification: label for displaying the audio file path */
  uiwidgetp = uiCreateColonLabel (_("File"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetState (uiwidgetp, UIWIDGET_DISABLE);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("");
  uiLabelEllipsizeOn (uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetClass (uiwidgetp, DARKACCENT_CLASS);
  uiLabelSetSelectable (uiwidgetp);
  audioidint->wcont [UIAUDID_W_FILE_DISP] = uiwidgetp;

  uiwcontFree (hbox);

  pw = uiPanedWindowCreateVert ();
  uiWidgetExpandHoriz (pw);
  uiWidgetAlignHorizFill (pw);
  uiBoxPackStartExpand (audioidint->wcont [UIAUDID_W_MAIN_VBOX], pw);
  uiWidgetSetClass (pw, ACCENT_CLASS);
  audioidint->wcont [UIAUDID_W_PANED_WINDOW] = pw;

  /* match listing */

  uiwidgetp = uiCreateScrolledWindow (160);
  uiWidgetExpandHoriz (uiwidgetp);
  uiPanedWindowPackStart (pw, uiwidgetp);

  audioidint->wcont [UIAUDID_W_TREE] = uiCreateTreeView ();

  uiTreeViewEnableHeaders (audioidint->wcont [UIAUDID_W_TREE]);
  uiWidgetAlignHorizFill (audioidint->wcont [UIAUDID_W_TREE]);
  uiWidgetExpandHoriz (audioidint->wcont [UIAUDID_W_TREE]);
  uiWidgetExpandVert (audioidint->wcont [UIAUDID_W_TREE]);
  uiWindowPackInWindow (uiwidgetp, audioidint->wcont [UIAUDID_W_TREE]);
  uiwcontFree (uiwidgetp);  // scrolled window

  audioidint->callbacks [UIAUDID_CB_ROW_SELECT] = callbackInit (
        uiaudioidRowSelect, uiaudioid, NULL);
  uiTreeViewSetSelectChangedCallback (audioidint->wcont [UIAUDID_W_TREE],
        audioidint->callbacks [UIAUDID_CB_ROW_SELECT]);

  uitreedispAddDisplayColumns (audioidint->wcont [UIAUDID_W_TREE],
      audioidint->listsellist, UIAUDID_COL_MAX, UIAUDID_COL_FONT,
      UIAUDID_COL_ELLIPSIZE, UIAUDID_COL_COLOR, UIAUDID_COL_COLOR_SET);

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

  count = slistGetCount (audioidint->sellist);

  /* the items must all be alloc'd beforehand so that the callback */
  /* pointer is static */
  audioidint->items = mdmalloc (sizeof (uiaudioiditem_t) * count);
  for (int i = 0; i < count; ++i) {
    audioidint->items [i].tagidx = 0;
    audioidint->items [i].currrb = NULL;
    audioidint->items [i].selrb = NULL;
    audioidint->items [i].callback = NULL;
  }

  col = uiCreateVertBox ();
  uiWidgetSetAllMargins (col, 4);
  uiWidgetExpandHoriz (col);
  uiWidgetExpandVert (col);
  uiBoxPackStartExpand (hbox, col);

  uiwcontFree (hbox);

  /* headings */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (col, hbox);

  uiwidgetp = uiCreateLabel ("");
  uiWidgetSetMarginEnd (uiwidgetp, 4);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (audioidint->szgrp [UIAUDID_SZGRP_LABEL], uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel (" ");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  snprintf (tbuff, sizeof (tbuff), "%s bold", bdjoptGetStr (OPT_MP_UIFONT));

  /* CONTEXT: audio identification: the data for the current song */
  uiwidgetp = uiCreateLabel (_("Current"));
  uiLabelSetFont (uiwidgetp, tbuff);
  uiWidgetSetMarginEnd (uiwidgetp, 4);
  uiSizeGroupAdd (audioidint->szgrp [UIAUDID_SZGRP_COL_A], uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel (" ");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  /* CONTEXT: audio identification: the data for the selected matched song */
  uiwidgetp = uiCreateLabel (_("Selected"));
  uiLabelSetFont (uiwidgetp, tbuff);
  uiSizeGroupAdd (audioidint->szgrp [UIAUDID_SZGRP_COL_B], uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);
  uiwcontFree (hbox);

  uiaudioidAddDisplay (uiaudioid, col);

  uiwcontFree (col);

  audioidint->wcont [UIAUDID_W_KEY_HNDLR] = uiEventAlloc ();
  audioidint->callbacks [UIAUDID_CB_KEYB] = callbackInit (
      uiaudioidKeyEvent, uiaudioid, NULL);
  uiEventSetKeyCallback (audioidint->wcont [UIAUDID_W_KEY_HNDLR],
      audioidint->wcont [UIAUDID_W_MAIN_VBOX],
      audioidint->callbacks [UIAUDID_CB_KEYB]);

  /* create tree value store */

  audioidint->typelist = mdmalloc (sizeof (int) * UIAUDID_COL_MAX);
  audioidint->colcount = 0;
  audioidint->typelist [UIAUDID_COL_ELLIPSIZE] = TREE_TYPE_ELLIPSIZE;
  audioidint->typelist [UIAUDID_COL_FONT] = TREE_TYPE_STRING;
  audioidint->typelist [UIAUDID_COL_COLOR] = TREE_TYPE_STRING;
  audioidint->typelist [UIAUDID_COL_COLOR_SET] = TREE_TYPE_BOOLEAN;
  audioidint->typelist [UIAUDID_COL_IDX] = TREE_TYPE_NUM;
  audioidint->colcount = UIAUDID_COL_MAX;

  uisongAddDisplayTypes (audioidint->listsellist, uiaudioidDisplayTypeCallback,
      uiaudioid);
  uiTreeViewCreateValueStoreFromList (audioidint->wcont [UIAUDID_W_TREE],
      audioidint->colcount, audioidint->typelist);
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
  long            val;
  double          dval;
  const char      *listingFont;
  const char      *data;
  int             row;

  logProcBegin ();

  if (uiaudioid == NULL) {
    return;
  }

  audioidint = uiaudioid->audioidInternalData;
  audioidint->song = song;
  audioidint->dbidx = dbidx;
  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);
  audioidint->selchgbypass = true;

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
      snprintf (tmp, sizeof (tmp), "%ld", val);
      dataFree (tval);
      tval = mdstrdup (tmp);
    }
    if (tagidx == TAG_DURATION) {
      dataFree (tval);
      tval = songDisplayString (song, tagidx, SONG_UNADJUSTED_DURATION);
    }
    nlistSetStr (audioidint->currlist, tagidx, tval);
    uiToggleButtonSetState (audioidint->items [count].currrb, UI_TOGGLE_BUTTON_ON);
    uiToggleButtonSetState (audioidint->items [count].selrb, UI_TOGGLE_BUTTON_OFF);
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

  row = 0;

  if (row >= audioidint->treerowcount) {
    uiTreeViewValueAppend (audioidint->wcont [UIAUDID_W_TREE]);
    uiTreeViewSetValueEllipsize (audioidint->wcont [UIAUDID_W_TREE],
        UIAUDID_COL_ELLIPSIZE);
    uiTreeViewSetValues (audioidint->wcont [UIAUDID_W_TREE],
        UIAUDID_COL_FONT, listingFont,
        UIAUDID_COL_COLOR, bdjoptGetStr (OPT_P_UI_ACCENT_COL),
        UIAUDID_COL_COLOR_SET, (treebool_t) false,
        UIAUDID_COL_IDX, (treenum_t) row,
        TREE_VALUE_END);
    ++audioidint->treerowcount;
  }

  uiTreeViewSelectSet (audioidint->wcont [UIAUDID_W_TREE], row);
  uisongSetDisplayColumns (audioidint->listsellist, song, UIAUDID_COL_MAX,
      uiaudioidSetSongDataCallback, uiaudioid);
  uiaudioidDisplayDuration (uiaudioid, song);
  uiTreeViewSetValues (audioidint->wcont [UIAUDID_W_TREE],
      UIAUDID_COL_COLOR_SET, (treebool_t) true, TREE_VALUE_END);

  if (audioidint->selectedrow > 0 &&
      audioidint->selectedrow < audioidint->rowcount) {
    /* reset the selection in case this is a save/reload */
    uiTreeViewSelectSet (audioidint->wcont [UIAUDID_W_TREE],
        audioidint->selectedrow);
  }

  audioidint->selchgbypass = false;

  logProcEnd ("");
}

void
uiaudioidResetDisplayList (uiaudioid_t *uiaudioid)
{
  aid_internal_t  *audioidint;

  if (uiaudioid == NULL) {
    return;
  }

  audioidint = uiaudioid->audioidInternalData;

  audioidint->selchgbypass = true;

  nlistFree (audioidint->displaylist);
  audioidint->displaylist = nlistAlloc ("uiaudioid-disp", LIST_UNORDERED, NULL);

  /* blank the field selection */
  for (int count = 0; count < audioidint->itemcount; ++count) {
    uiToggleButtonSetText (audioidint->items [count].selrb, "");
  }

  /* this makes the ui look nice when the next/previous song button */
  /* is selected */
  uiaudioidBlankDisplayList (uiaudioid);

  audioidint->selectedrow = -1;
  audioidint->fillrow = 0;
}

void
uiaudioidSetDisplayList (uiaudioid_t *uiaudioid, nlist_t *dlist)
{
  aid_internal_t  *audioidint;
  const char      *listingFont;
  int             tagidx;
  int             col;
  slistidx_t      seliteridx;
  nlist_t         *ndlist;
  const char      *str;


  if (uiaudioid == NULL) {
    return;
  }

  audioidint = uiaudioid->audioidInternalData;

  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);

  ndlist = nlistAlloc ("uiaudio-ndlist", LIST_UNORDERED, NULL);
  nlistSetSize (ndlist, nlistGetCount (dlist));

  ++audioidint->fillrow;
  if (audioidint->fillrow >= audioidint->treerowcount) {
    uiTreeViewValueAppend (audioidint->wcont [UIAUDID_W_TREE]);
    uiTreeViewSetValueEllipsize (audioidint->wcont [UIAUDID_W_TREE],
        UIAUDID_COL_ELLIPSIZE);
    /* color-set isn't working for me within this module */
    /* so set the color to null */
    uiTreeViewSetValues (audioidint->wcont [UIAUDID_W_TREE],
        UIAUDID_COL_FONT, listingFont,
        UIAUDID_COL_COLOR, NULL,
        UIAUDID_COL_COLOR_SET, (treebool_t) false,
        UIAUDID_COL_IDX, (treenum_t) audioidint->fillrow,
        TREE_VALUE_END);
    ++audioidint->treerowcount;
  }

  /* all data use in the display must be cloned */
  slistStartIterator (audioidint->sellist, &seliteridx);
  while ((tagidx = slistIterateValueNum (audioidint->sellist, &seliteridx)) >= 0) {
    str = nlistGetStr (dlist, tagidx);
    nlistSetStr (ndlist, tagidx, str);
  }

  /* also save the recording id and work id */
  str = nlistGetStr (dlist, TAG_RECORDING_ID);
  nlistSetStr (ndlist, TAG_RECORDING_ID, str);
  str = nlistGetStr (dlist, TAG_WORK_ID);
  nlistSetStr (ndlist, TAG_WORK_ID, str);

  uiTreeViewSelectSet (audioidint->wcont [UIAUDID_W_TREE],
      audioidint->fillrow);
  col = UIAUDID_COL_MAX;
  slistStartIterator (audioidint->listsellist, &seliteridx);
  while ((tagidx = slistIterateValueNum (audioidint->listsellist, &seliteridx)) >= 0) {
    if (tagidx == TAG_AUDIOID_IDENT) {
      int         val;
      const char  *idstr;

      val = nlistGetNum (dlist, tagidx);
      nlistSetNum (ndlist, tagidx, val);
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
      uitreedispSetDisplayColumn (audioidint->wcont [UIAUDID_W_TREE],
          col, 0, idstr);
    } else if (tagidx == TAG_AUDIOID_SCORE) {
      char    tmp [40];
      double  dval;

      dval = nlistGetDouble (dlist, tagidx);
      nlistSetDouble (ndlist, tagidx, dval);
      tmp [0] = '\0';
      if (dval > 0.0) {
        snprintf (tmp, sizeof (tmp), "%.1f", dval);
      }
      uitreedispSetDisplayColumn (audioidint->wcont [UIAUDID_W_TREE],
          col, 0, tmp);
    } else if (tagidx == TAG_DURATION) {
      const char  *str;
      char        tmp [40];
      long        dur = 0;

      /* duration must be converted */
      str = nlistGetStr (dlist, tagidx);
      if (str != NULL) {
        dur = atol (str);
      }
      tmutilToMSD (dur, tmp, sizeof (tmp), 1);
      uitreedispSetDisplayColumn (audioidint->wcont [UIAUDID_W_TREE],
          col, 0, tmp);
    } else {
      const char  *str;

      str = nlistGetStr (dlist, tagidx);
      if (str == NULL) {
        str = "";
      }
      uitreedispSetDisplayColumn (audioidint->wcont [UIAUDID_W_TREE],
        col, 0, str);
    }
    ++col;
  }

  nlistSort (ndlist);
  nlistSetList (audioidint->displaylist, audioidint->fillrow, ndlist);

  logProcEnd ("");
}

void
uiaudioidFinishDisplayList (uiaudioid_t *uiaudioid)
{
  aid_internal_t  *audioidint;

  audioidint = uiaudioid->audioidInternalData;

  audioidint->rowcount = audioidint->fillrow + 1;
  if (audioidint->rowcount > 1) {
    uiTreeViewValueClear (audioidint->wcont [UIAUDID_W_TREE],
        audioidint->rowcount);
  }
  nlistSort (audioidint->displaylist);

  audioidint->selectedrow = -1;

  if (audioidint->rowcount > 1) {
    /* the row-selection callbacks will set the selected-row */
    uiTreeViewSelectFirst (audioidint->wcont [UIAUDID_W_TREE]);
    audioidint->selchgbypass = false;
    uiTreeViewSelectNext (audioidint->wcont [UIAUDID_W_TREE]);
  }

  audioidint->selchgbypass = false;
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
uiaudioidAddDisplay (uiaudioid_t *uiaudioid, uiwcont_t *col)
{
  slistidx_t      dsiteridx;
  aid_internal_t  *audioidint;
  int             tagidx;

  logProcBegin ();
  audioidint = uiaudioid->audioidInternalData;

  slistStartIterator (audioidint->sellist, &dsiteridx);
  while ((tagidx = slistIterateValueNum (audioidint->sellist, &dsiteridx)) >= 0) {
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

    uiaudioidAddItem (uiaudioid, hbox, tagidx);
    audioidint->items [audioidint->itemcount].tagidx = tagidx;
    ++audioidint->itemcount;

    uiwcontFree (hbox);
  }

  logProcEnd ("");
}

static void
uiaudioidAddItem (uiaudioid_t *uiaudioid, uiwcont_t *hbox, int tagidx)
{
  uiwcont_t       *uiwidgetp;
  uiwcont_t       *rb;
  aid_internal_t  *audioidint;

  logProcBegin ();
  audioidint = uiaudioid->audioidInternalData;

  /* line: label, curr-rb, sel-rb */

  uiwidgetp = uiCreateColonLabel (tagdefs [tagidx].displayname);
  uiWidgetSetMarginEnd (uiwidgetp, 4);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (audioidint->szgrp [UIAUDID_SZGRP_LABEL], uiwidgetp);
  uiwcontFree (uiwidgetp);

  rb = uiCreateRadioButton (NULL, "", UI_TOGGLE_BUTTON_ON);
  /* ellipsize set after text is set */
  uiBoxPackStartExpand (hbox, rb);
  uiSizeGroupAdd (audioidint->szgrp [UIAUDID_SZGRP_COL_A], rb);
  audioidint->items [audioidint->itemcount].currrb = rb;

  uiwidgetp = uiCreateRadioButton (rb, "", UI_TOGGLE_BUTTON_OFF);
  /* ellipsize set after text is set */
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiSizeGroupAdd (audioidint->szgrp [UIAUDID_SZGRP_COL_B], uiwidgetp);
  audioidint->items [audioidint->itemcount].selrb = uiwidgetp;

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

  dlist = nlistGetList (audioidint->displaylist, audioidint->selectedrow);
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

static bool
uiaudioidRowSelect (void *udata)
{
  uiaudioid_t     * uiaudioid = udata;
  aid_internal_t  * audioidint;
  int             idx;

  audioidint = uiaudioid->audioidInternalData;

  if (audioidint->selchgbypass) {
    return UICB_CONT;
  }

  uiTreeViewSelectCurrent (audioidint->wcont [UIAUDID_W_TREE]);
  idx = uiTreeViewGetValue (audioidint->wcont [UIAUDID_W_TREE],
      UIAUDID_COL_IDX);
  audioidint->selectedrow = idx;
  uiaudioidPopulateSelected (uiaudioid, idx);

  return UICB_CONT;
}

static void
uiaudioidPopulateSelected (uiaudioid_t *uiaudioid, int idx)
{
  aid_internal_t  *audioidint;
  nlist_t         *dlist = NULL;

  audioidint = uiaudioid->audioidInternalData;
  if (audioidint->displaylist == NULL) {
    return;
  }

  if (idx < 0 || idx >= audioidint->rowcount) {
    return;
  }

  dlist = nlistGetList (audioidint->displaylist, idx);

  for (int count = 0; count < audioidint->itemcount; ++count) {
    int         tagidx = audioidint->items [count].tagidx;
    const char  *ttval;

    uiToggleButtonSetState (audioidint->items [count].currrb, UI_TOGGLE_BUTTON_ON);
    uiToggleButtonSetState (audioidint->items [count].selrb, UI_TOGGLE_BUTTON_OFF);

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
uiaudioidDisplayTypeCallback (int type, void *udata)
{
  uiaudioid_t    *uiaudioid = udata;
  aid_internal_t *audioidint;

  audioidint = uiaudioid->audioidInternalData;
  audioidint->typelist = mdrealloc (audioidint->typelist, sizeof (int) * (audioidint->colcount + 1));
  audioidint->typelist [audioidint->colcount] = type;
  ++audioidint->colcount;
}

static void
uiaudioidSetSongDataCallback (int col, long num, const char *str, void *udata)
{
  uiaudioid_t     *uiaudioid = udata;
  aid_internal_t  *audioidint;

  audioidint = uiaudioid->audioidInternalData;
  uitreedispSetDisplayColumn (audioidint->wcont [UIAUDID_W_TREE],
      col, num, str);
}

static void
uiaudioidDisplayDuration (uiaudioid_t *uiaudioid, song_t *song)
{
  slistidx_t  seliteridx;
  int         tagidx;
  int         col;
  bool        found = false;
  aid_internal_t  *audioidint;

  audioidint = uiaudioid->audioidInternalData;

  col = 0;
  slistStartIterator (audioidint->listsellist, &seliteridx);
  while ((tagidx = slistIterateValueNum (audioidint->listsellist, &seliteridx)) >= 0) {
    if (tagidx == TAG_DURATION) {
      found = true;
      break;
    }
    ++col;
  }
  if (found) {
    char    *tmp;

    col += UIAUDID_COL_MAX;
    tmp = songDisplayString (song, TAG_DURATION, SONG_UNADJUSTED_DURATION);
    uitreedispSetDisplayColumn (audioidint->wcont [UIAUDID_W_TREE],
        col, 0, tmp);
    dataFree (tmp);
  }
}

static void
uiaudioidBlankDisplayList (uiaudioid_t *uiaudioid)
{
  aid_internal_t  *audioidint;

  if (uiaudioid == NULL) {
    return;
  }

  audioidint = uiaudioid->audioidInternalData;

  for (int i = 1; i < audioidint->rowcount; ++i) {
    ilistidx_t    seliteridx;
    int           tagidx;
    int           col;

    uiTreeViewSelectSet (audioidint->wcont [UIAUDID_W_TREE], i);

    col = UIAUDID_COL_MAX;
    slistStartIterator (audioidint->listsellist, &seliteridx);
    while ((tagidx = slistIterateValueNum (audioidint->listsellist, &seliteridx)) >= 0) {
      uitreedispSetDisplayColumn (audioidint->wcont [UIAUDID_W_TREE],
          col, 0, "");
      ++col;
    }
  }
}
