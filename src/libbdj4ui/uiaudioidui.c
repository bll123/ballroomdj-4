/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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

#include "bdj4intl.h"
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
  UIAID_SEL_CURR,
  UIAID_SEL_NEW,
};

enum {
  UIAID_COL_ELLIPSIZE,
  UIAID_COL_FONT,
  UIAID_COL_COLOR,
  UIAID_COL_COLOR_SET,
  UIAID_COL_IDX,
  UIAID_COL_SCORE,
  UIAID_COL_MAX,
};

typedef struct {
  int             tagidx;
  uiwcont_t       *currrb;
  uiwcont_t       *selrb;
  callback_t      *callback;
  bool            selection : 1;
} uiaudioiditem_t;

enum {
  UIAID_CB_FIRST,
  UIAID_CB_SAVE,
  UIAID_CB_PREV,
  UIAID_CB_NEXT,
  UIAID_CB_KEYB,
  UIAID_CB_ROW_SELECT,
  UIAID_CB_ITEM_SELECT,
  UIAID_CB_MAX,
};

enum {
  UIAID_BUTTON_FIRST,
  UIAID_BUTTON_PREV,
  UIAID_BUTTON_NEXT,
  UIAID_BUTTON_SAVE,
  UIAID_BUTTON_MAX,
};

enum {
  UIAID_MAIN_TIMER = 40,
};

enum {
  UIAID_W_EDIT_ALL,
  UIAID_W_PARENT_WIN,
  UIAID_W_MAIN_VBOX,
  UIAID_W_AUDIOID_IMG,
  UIAID_W_FILE_DISP,
  UIAID_W_MAX,
};

enum {
  UIAID_SZGRP_MAX = 3,
};

typedef struct aid_internal {
  uiwcont_t           *wcont [UIAID_W_MAX];
  uiwcont_t           *szgrp [UIAID_SZGRP_MAX];
  callback_t          *callbacks [UIAID_CB_MAX];
  uibutton_t          *buttons [UIAID_BUTTON_MAX];
  uitree_t            *alistTree;
  song_t              *song;
  uiaudioiditem_t     *items;
  int                 *typelist;
  slist_t             *listsellist;
  nlist_t             *displaylist;
  dbidx_t             dbidx;
  int                 itemcount;
  int                 colcount;
  int                 rowcount;
  int                 currrow;
  int                 changed;
  uikey_t             *uikey;
  bool                selchgbypass : 1;
} aid_internal_t;

static void uiaudioidAddDisplay (uiaudioid_t *songedit, uiwcont_t *col);
static void uiaudioidAddItem (uiaudioid_t *uiaudioid, uiwcont_t *hbox, int tagidx);
static bool uiaudioidSaveCallback (void *udata);
static bool uiaudioidSave (void *udata, nlist_t *chglist);
static bool uiaudioidFirstSelection (void *udata);
static bool uiaudioidPreviousSelection (void *udata);
static bool uiaudioidNextSelection (void *udata);
static bool uiaudioidKeyEvent (void *udata);
static bool uiaudioidRowSelect (void *udata);
static void uiaudioidPopulateSelected (uiaudioid_t *uiaudioid, int idx);
static void uiaudioidDisplayTypeCallback (int type, void *udata);
static void uiaudioidSetSongDataCallback (int col, long num, const char *str, void *udata);
static bool uiaudioidItemSelectCallback (void *udata);

void
uiaudioidUIInit (uiaudioid_t *uiaudioid)
{
  aid_internal_t  *audioidint;

  logProcBegin (LOG_PROC, "uiaudioidUIInit");

  audioidint = mdmalloc (sizeof (aid_internal_t));
  audioidint->itemcount = 0;
  audioidint->items = NULL;
  audioidint->changed = 0;
  audioidint->uikey = NULL;
  audioidint->dbidx = -1;
  audioidint->typelist = NULL;
  audioidint->displaylist = NULL;
  audioidint->colcount = 0;
  audioidint->rowcount = 0;
  /* select-change bypass will be true until the data is first loaded */
  audioidint->selchgbypass = true;
  audioidint->listsellist = dispselGetList (uiaudioid->dispsel, DISP_SEL_AUDIOID_LIST);

  for (int i = 0; i < UIAID_BUTTON_MAX; ++i) {
    audioidint->buttons [i] = NULL;
  }
  for (int i = 0; i < UIAID_CB_MAX; ++i) {
    audioidint->callbacks [i] = NULL;
  }
  for (int i = 0; i < UIAID_SZGRP_MAX; ++i) {
    audioidint->szgrp [i] = uiCreateSizeGroupHoriz ();
  }
  for (int i = 0; i < UIAID_W_MAX; ++i) {
    audioidint->wcont [i] = NULL;
  }

  uiaudioid->audioidInternalData = audioidint;
  logProcEnd (LOG_PROC, "uiaudioidUIInit", "");
}

void
uiaudioidUIFree (uiaudioid_t *uiaudioid)
{
  aid_internal_t *audioidint;

  logProcBegin (LOG_PROC, "uiaudioidUIFree");
  if (uiaudioid == NULL) {
    logProcEnd (LOG_PROC, "uiaudioidUIFree", "null");
    return;
  }

  audioidint = uiaudioid->audioidInternalData;
  if (audioidint != NULL) {
    for (int count = 0; count < audioidint->itemcount; ++count) {
      callbackFree (audioidint->items [count].callback);
      uiwcontFree (audioidint->items [count].currrb);
      uiwcontFree (audioidint->items [count].selrb);
    }
    dataFree (audioidint->typelist);

    for (int i = 0; i < UIAID_SZGRP_MAX; ++i) {
      uiwcontFree (audioidint->szgrp [i]);
    }
    for (int i = 0; i < UIAID_W_MAX; ++i) {
      if (i == UIAID_W_PARENT_WIN) {
        continue;
      }
      uiwcontFree (audioidint->wcont [i]);
    }
    for (int i = 0; i < UIAID_BUTTON_MAX; ++i) {
      uiButtonFree (audioidint->buttons [i]);
    }
    for (int i = 0; i < UIAID_CB_MAX; ++i) {
      callbackFree (audioidint->callbacks [i]);
    }

    uiTreeViewFree (audioidint->alistTree);
    dataFree (audioidint->items);
    uiKeyFree (audioidint->uikey);
    mdfree (audioidint);
    uiaudioid->audioidInternalData = NULL;
  }

  logProcEnd (LOG_PROC, "uiaudioidUIFree", "");
}

uiwcont_t *
uiaudioidBuildUI (uisongsel_t *uisongsel, uiaudioid_t *uiaudioid,
    uiwcont_t *parentwin, uiwcont_t *statusMsg)
{
  aid_internal_t    *audioidint;
  uiwcont_t         *pw;
  uiwcont_t         *hbox;
  uibutton_t        *uibutton;
  uiwcont_t         *uiwidgetp;
  uiwcont_t         *uitreewidgetp;
  int               count;
  uiwcont_t         *col;
  uiwcont_t         *hhbox;
  slist_t           *sellist;
  char              tbuff [MAXPATHLEN];

  logProcBegin (LOG_PROC, "uiaudioidBuildUI");

  uiaudioid->statusMsg = statusMsg;
  uiaudioid->uisongsel = uisongsel;
  audioidint = uiaudioid->audioidInternalData;
  audioidint->wcont [UIAID_W_PARENT_WIN] = parentwin;

  audioidint->wcont [UIAID_W_MAIN_VBOX] = uiCreateVertBox ();
  uiWidgetExpandHoriz (audioidint->wcont [UIAID_W_MAIN_VBOX]);

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiWidgetAlignHorizFill (hbox);
  uiBoxPackStart (audioidint->wcont [UIAID_W_MAIN_VBOX], hbox);

  audioidint->callbacks [UIAID_CB_FIRST] = callbackInit (
      uiaudioidFirstSelection, uiaudioid, "songedit: first");
  uibutton = uiCreateButton (audioidint->callbacks [UIAID_CB_FIRST],
      /* CONTEXT: song editor : first song */
      _("First"), NULL);
  audioidint->buttons [UIAID_BUTTON_FIRST] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackStart (hbox, uiwidgetp);

  audioidint->callbacks [UIAID_CB_PREV] = callbackInit (
      uiaudioidPreviousSelection, uiaudioid, "songedit: previous");
  uibutton = uiCreateButton (audioidint->callbacks [UIAID_CB_PREV],
      /* CONTEXT: song editor : previous song */
      _("Previous"), NULL);
  audioidint->buttons [UIAID_BUTTON_PREV] = uibutton;
  uiButtonSetRepeat (uibutton, REPEAT_TIME);
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackStart (hbox, uiwidgetp);

  audioidint->callbacks [UIAID_CB_NEXT] = callbackInit (
      uiaudioidNextSelection, uiaudioid, "songedit: next");
  uibutton = uiCreateButton (audioidint->callbacks [UIAID_CB_NEXT],
      /* CONTEXT: song editor : next song */
      _("Next"), NULL);
  audioidint->buttons [UIAID_BUTTON_NEXT] = uibutton;
  uiButtonSetRepeat (uibutton, REPEAT_TIME);
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackStart (hbox, uiwidgetp);

  audioidint->callbacks [UIAID_CB_SAVE] = callbackInit (
      uiaudioidSaveCallback, uiaudioid, "songedit: save");
  uibutton = uiCreateButton (audioidint->callbacks [UIAID_CB_SAVE],
      /* CONTEXT: song editor : save data */
      _("Save"), NULL);
  audioidint->buttons [UIAID_BUTTON_SAVE] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackEnd (hbox, uiwidgetp);

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetSetMarginEnd (uiwidgetp, 6);
  uiWidgetSetClass (uiwidgetp, DARKACCENT_CLASS);
  audioidint->wcont [UIAID_W_EDIT_ALL] = uiwidgetp;

  uiwcontFree (hbox);

  /* begin line */

  /* audio-identification logo, modified indicator, */
  /* copy button, file label, filename */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiWidgetAlignHorizFill (hbox);
  uiBoxPackStart (audioidint->wcont [UIAID_W_MAIN_VBOX], hbox);

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
  audioidint->wcont [UIAID_W_FILE_DISP] = uiwidgetp;

  uiwcontFree (hbox);

  pw = uiPanedWindowCreateVert ();
  uiWidgetExpandHoriz (pw);
  uiWidgetAlignHorizFill (pw);
  uiBoxPackStartExpand (audioidint->wcont [UIAID_W_MAIN_VBOX], pw);
  uiWidgetSetClass (pw, ACCENT_CLASS);

  /* listing */

  uiwidgetp = uiCreateScrolledWindow (160);
  uiWidgetExpandHoriz (uiwidgetp);
  uiPanedWindowPackStart (pw, uiwidgetp);

  audioidint->alistTree = uiCreateTreeView ();
  uitreewidgetp = uiTreeViewGetWidgetContainer (audioidint->alistTree);

  uiTreeViewEnableHeaders (audioidint->alistTree);
  uiWidgetAlignHorizFill (uitreewidgetp);
  uiWidgetExpandHoriz (uitreewidgetp);
  uiWidgetExpandVert (uitreewidgetp);
  uiBoxPackInWindow (uiwidgetp, uitreewidgetp);
  uiwcontFree (uiwidgetp);  // scrolled window

  audioidint->callbacks [UIAID_CB_ROW_SELECT] = callbackInit (
        uiaudioidRowSelect, uiaudioid, NULL);
  uiTreeViewSetSelectChangedCallback (audioidint->alistTree,
        audioidint->callbacks [UIAID_CB_ROW_SELECT]);

  uiTreeViewAppendColumn (audioidint->alistTree, TREE_NO_COLUMN,
      TREE_WIDGET_TEXT, TREE_ALIGN_RIGHT,
      TREE_COL_DISP_GROW, _("Score"),
      TREE_COL_TYPE_FONT, UIAID_COL_FONT,
      TREE_COL_TYPE_FOREGROUND, UIAID_COL_COLOR,
      TREE_COL_TYPE_FOREGROUND_SET, UIAID_COL_COLOR_SET,
      TREE_COL_TYPE_TEXT, UIAID_COL_SCORE,
      TREE_COL_TYPE_END);

  uitreedispAddDisplayColumns (audioidint->alistTree, audioidint->listsellist,
      UIAID_COL_MAX, UIAID_COL_FONT, UIAID_COL_ELLIPSIZE,
      UIAID_COL_COLOR, UIAID_COL_COLOR_SET);

  /* current/selected box */

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiWidgetAlignHorizFill (hbox);
  uiPanedWindowPackEnd (pw, hbox);

  sellist = dispselGetList (uiaudioid->dispsel, DISP_SEL_AUDIOID);
  count = slistGetCount (sellist);

  /* the items must all be alloc'd beforehand so that the callback */
  /* pointer is static */
  /* need to add 1 for the BPM display secondary display */
  audioidint->items = mdmalloc (sizeof (uiaudioiditem_t) * (count + 1));
  for (int i = 0; i < count + 1; ++i) {
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

  /* headings */

  hhbox = uiCreateHorizBox ();
  uiBoxPackStart (col, hhbox);

  uiwidgetp = uiCreateLabel ("");
  uiWidgetSetMarginEnd (uiwidgetp, 4);
  uiBoxPackStart (hhbox, uiwidgetp);
  uiSizeGroupAdd (audioidint->szgrp [0], uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel (" ");
  uiBoxPackStart (hhbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  snprintf (tbuff, sizeof (tbuff), "%s bold", bdjoptGetStr (OPT_MP_UIFONT));

  uiwidgetp = uiCreateLabel (_("Current"));
  uiLabelSetFont (uiwidgetp, tbuff);
  uiWidgetSetMarginEnd (uiwidgetp, 4);
  uiSizeGroupAdd (audioidint->szgrp [1], uiwidgetp);
  uiBoxPackStartExpand (hhbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel (" ");
  uiBoxPackStart (hhbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel (_("Selected"));
  uiLabelSetFont (uiwidgetp, tbuff);
  uiSizeGroupAdd (audioidint->szgrp [2], uiwidgetp);
  uiBoxPackStartExpand (hhbox, uiwidgetp);
  uiwcontFree (uiwidgetp);
  uiwcontFree (hhbox);

  audioidint->callbacks [UIAID_CB_ITEM_SELECT] = callbackInit (
      uiaudioidItemSelectCallback, uiaudioid, NULL);

  uiaudioidAddDisplay (uiaudioid, col);

  uiwcontFree (col);
  uiwcontFree (hbox);
  uiwcontFree (pw);

  audioidint->uikey = uiKeyAlloc ();
  audioidint->callbacks [UIAID_CB_KEYB] = callbackInit (
      uiaudioidKeyEvent, uiaudioid, NULL);
  uiKeySetKeyCallback (audioidint->uikey, audioidint->wcont [UIAID_W_MAIN_VBOX],
      audioidint->callbacks [UIAID_CB_KEYB]);

  /* create tree value store */

  audioidint->typelist = mdmalloc (sizeof (int) * UIAID_COL_MAX);
  audioidint->colcount = 0;
  audioidint->rowcount = 0;
  audioidint->typelist [UIAID_COL_ELLIPSIZE] = TREE_TYPE_ELLIPSIZE;
  audioidint->typelist [UIAID_COL_FONT] = TREE_TYPE_STRING;
  audioidint->typelist [UIAID_COL_COLOR] = TREE_TYPE_STRING;
  audioidint->typelist [UIAID_COL_COLOR_SET] = TREE_TYPE_BOOLEAN;
  audioidint->typelist [UIAID_COL_IDX] = TREE_TYPE_NUM;
  audioidint->typelist [UIAID_COL_SCORE] = TREE_TYPE_STRING;
  audioidint->colcount = UIAID_COL_MAX;

  uisongAddDisplayTypes (audioidint->listsellist, uiaudioidDisplayTypeCallback,
      uiaudioid);
  uiTreeViewCreateValueStoreFromList (audioidint->alistTree, audioidint->colcount, audioidint->typelist);

  logProcEnd (LOG_PROC, "uiaudioidBuildUI", "");
  return audioidint->wcont [UIAID_W_MAIN_VBOX];
}

void
uiaudioidLoadData (uiaudioid_t *uiaudioid, song_t *song, dbidx_t dbidx)
{
  aid_internal_t   *audioidint;
  char            *tval;
  long            val;
  double          dval;
  const char      *listingFont;
  int             row;

  logProcBegin (LOG_PROC, "uiaudioidLoadData");
  audioidint = uiaudioid->audioidInternalData;
  audioidint->song = song;
  audioidint->dbidx = dbidx;
  audioidint->changed = 0;
  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);
  audioidint->selchgbypass = true;

  tval = uisongGetDisplay (song, TAG_FILE, &val, &dval);
  uiLabelSetText (audioidint->wcont [UIAID_W_FILE_DISP], tval);
  dataFree (tval);
  tval = NULL;

  for (int count = 0; count < audioidint->itemcount; ++count) {
    int tagidx = audioidint->items [count].tagidx;

    tval = uisongGetDisplay (song, tagidx, &val, &dval);
    uiToggleButtonSetText (audioidint->items [count].currrb, tval);
    uiToggleButtonSetText (audioidint->items [count].selrb, "");
    dataFree (tval);
    tval = NULL;
  }

  row = 0;

  if (row >= audioidint->rowcount) {
    uiTreeViewValueAppend (audioidint->alistTree);
    uiTreeViewSetValueEllipsize (audioidint->alistTree, UIAID_COL_ELLIPSIZE);
    uiTreeViewSetValues (audioidint->alistTree,
        UIAID_COL_FONT, listingFont,
        UIAID_COL_COLOR, bdjoptGetStr (OPT_P_UI_ACCENT_COL),
        UIAID_COL_COLOR_SET, (treebool_t) false,
        UIAID_COL_IDX, (treenum_t) 0,
        UIAID_COL_SCORE, "",
        TREE_VALUE_END);
  } else {
    uiTreeViewSelectSet (audioidint->alistTree, row);
    /* no data in row 0 to update */
  }

  uiTreeViewSelectSet (audioidint->alistTree, row);
  audioidint->currrow = row;
  uisongSetDisplayColumns (audioidint->listsellist, song, UIAID_COL_MAX,
      uiaudioidSetSongDataCallback, uiaudioid);
  uiTreeViewSetValues (audioidint->alistTree,
      UIAID_COL_COLOR_SET, (treebool_t) true, TREE_VALUE_END);

  ++row;
  uiTreeViewValueClear (audioidint->alistTree, row);
  audioidint->rowcount = row;

// temporary
{
nlist_t *i;
nlist_t *d;
i = nlistAlloc ("temp-i", LIST_ORDERED, NULL);
d = nlistAlloc ("temp-d", LIST_ORDERED, NULL);
nlistSetStr (d, TAG_TEMPORARY, "100.0");
nlistSetStr (d, TAG_DATE, "2002");
nlistSetStr (d, TAG_TITLE, "title");
nlistSetStr (d, TAG_ARTIST, "artist");
nlistSetStr (d, TAG_ALBUMARTIST, "alb-artist");
nlistSetStr (d, TAG_ALBUM, "album");
nlistSetStr (d, TAG_GENRE, "genre");
nlistSetStr (d, TAG_TRACKNUMBER, "2");
nlistSetStr (d, TAG_TRACKTOTAL, "12");
nlistSetStr (d, TAG_DISCNUMBER, "1");
nlistSetStr (d, TAG_DISCTOTAL, "2");
nlistSetStr (d, TAG_DURATION, "2:46");
nlistSetList (i, 0, d);
d = nlistAlloc ("temp-d", LIST_ORDERED, NULL);
nlistSetStr (d, TAG_TEMPORARY, "90.0");
nlistSetStr (d, TAG_DATE, "2002");
nlistSetStr (d, TAG_TITLE, "title2");
nlistSetStr (d, TAG_ARTIST, "artist2");
nlistSetStr (d, TAG_ALBUMARTIST, "alb-artist2");
nlistSetStr (d, TAG_ALBUM, "album2");
nlistSetStr (d, TAG_GENRE, "genre2");
nlistSetStr (d, TAG_TRACKNUMBER, "3");
nlistSetStr (d, TAG_TRACKTOTAL, "12");
nlistSetStr (d, TAG_DISCNUMBER, "1");
nlistSetStr (d, TAG_DISCTOTAL, "2");
nlistSetStr (d, TAG_DURATION, "2:46");
nlistSetList (i, 1, d);
d = nlistAlloc ("temp-d", LIST_ORDERED, NULL);
nlistSetStr (d, TAG_TEMPORARY, "95.0");
nlistSetStr (d, TAG_DATE, "2002");
nlistSetStr (d, TAG_TITLE, "title3");
nlistSetStr (d, TAG_ARTIST, "artist3");
nlistSetStr (d, TAG_ALBUMARTIST, "alb-artist3");
nlistSetStr (d, TAG_ALBUM, "album3");
nlistSetStr (d, TAG_GENRE, "genre3");
nlistSetStr (d, TAG_TRACKNUMBER, "4");
nlistSetStr (d, TAG_TRACKTOTAL, "12");
nlistSetStr (d, TAG_DISCNUMBER, "1");
nlistSetStr (d, TAG_DISCTOTAL, "2");
nlistSetStr (d, TAG_DURATION, "2:46");
nlistSetList (i, 2, d);
uiaudioidSetDisplayList (uiaudioid, i);
}

  if (audioidint->rowcount > 1) {
    uiTreeViewSelectFirst (audioidint->alistTree);
    uiTreeViewSelectNext (audioidint->alistTree);
  }

  audioidint->selchgbypass = false;
  logProcEnd (LOG_PROC, "uiaudioidLoadData", "");
}

void
uiaudioidSetDisplayList (uiaudioid_t *uiaudioid, nlist_t *data)
{
  aid_internal_t  *audioidint;
  const char      *listingFont;
  int             tagidx;
  int             row;
  int             col;
  slistidx_t      seliteridx;
  nlistidx_t      key;
  nlistidx_t      iteridx;

  audioidint = uiaudioid->audioidInternalData;
  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);
  audioidint->selchgbypass = true;

  nlistStartIterator (data, &iteridx);

  row = 1;

  while ((key = nlistIterateKey (data, &iteridx)) >= 0) {
    nlist_t   *dlist;

    dlist = nlistGetList (data, key);

    if (row >= audioidint->rowcount) {
      uiTreeViewValueAppend (audioidint->alistTree);
      uiTreeViewSetValueEllipsize (audioidint->alistTree, UIAID_COL_ELLIPSIZE);
      /* color-set isn't working for me within this module */
      /* so set the color to null */
      uiTreeViewSetValues (audioidint->alistTree,
          UIAID_COL_FONT, listingFont,
          UIAID_COL_COLOR, NULL,
          UIAID_COL_COLOR_SET, (treebool_t) false,
          UIAID_COL_IDX, (treenum_t) row,
          UIAID_COL_SCORE, nlistGetStr (data, TAG_TEMPORARY),
          TREE_VALUE_END);
    } else {
      uiTreeViewSelectSet (audioidint->alistTree, row);
      /* all data must be updated, except font, ellipsize, color */
      uiTreeViewSetValues (audioidint->alistTree,
          UIAID_COL_SCORE, nlistGetStr (data, TAG_TEMPORARY),
          TREE_VALUE_END);
    }

    uiTreeViewSelectSet (audioidint->alistTree, row);
    col = UIAID_COL_MAX;
    slistStartIterator (audioidint->listsellist, &seliteridx);
    while ((tagidx = slistIterateValueNum (audioidint->listsellist, &seliteridx)) >= 0) {
      audioidint->currrow = row;
      uiaudioidSetSongDataCallback (col, 0,
          nlistGetStr (dlist, tagidx), uiaudioid);
      ++col;
    }

    ++row;
  }

  uiTreeViewValueClear (audioidint->alistTree, row);
  audioidint->rowcount = row;

  if (audioidint->rowcount > 1) {
    uiTreeViewSelectFirst (audioidint->alistTree);
    uiTreeViewSelectNext (audioidint->alistTree);
  }

  audioidint->selchgbypass = false;
  audioidint->displaylist = data;
  logProcEnd (LOG_PROC, "uiaudioidLoadData", "");
}

void
uiaudioidUIMainLoop (uiaudioid_t *uiaudioid)
{
  aid_internal_t   *audioidint;

  audioidint = uiaudioid->audioidInternalData;

  uiButtonCheckRepeat (audioidint->buttons [UIAID_BUTTON_NEXT]);
  uiButtonCheckRepeat (audioidint->buttons [UIAID_BUTTON_PREV]);
  return;
}

/* internal routines */

static void
uiaudioidAddDisplay (uiaudioid_t *uiaudioid, uiwcont_t *col)
{
  slist_t         *sellist;
  slistidx_t      dsiteridx;
  aid_internal_t  *audioidint;
  int             tagidx;

  logProcBegin (LOG_PROC, "uiaudioidAddDisplay");
  audioidint = uiaudioid->audioidInternalData;
  sellist = dispselGetList (uiaudioid->dispsel, DISP_SEL_AUDIOID);

  slistStartIterator (sellist, &dsiteridx);
  while ((tagidx = slistIterateValueNum (sellist, &dsiteridx)) >= 0) {
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

  logProcEnd (LOG_PROC, "uiaudioidAddDisplay", "");
}

static void
uiaudioidAddItem (uiaudioid_t *uiaudioid, uiwcont_t *hbox, int tagidx)
{
  uiwcont_t       *uiwidgetp;
  uiwcont_t       *rb;
  aid_internal_t  *audioidint;

  logProcBegin (LOG_PROC, "uiaudioidAddItem");
  audioidint = uiaudioid->audioidInternalData;

  /* line: label, curr-change-ind, curr-display, new-change-ind, new-disp */

  uiwidgetp = uiCreateColonLabel (tagdefs [tagidx].displayname);
  uiWidgetSetMarginEnd (uiwidgetp, 4);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (audioidint->szgrp [0], uiwidgetp);
  uiwcontFree (uiwidgetp);

  rb = uiCreateRadioButton (NULL, "", UI_TOGGLE_BUTTON_ON);
  uiBoxPackStartExpand (hbox, rb);
  uiSizeGroupAdd (audioidint->szgrp [1], rb);
  audioidint->items [audioidint->itemcount].currrb = rb;

  uiwidgetp = uiCreateRadioButton (rb, "", UI_TOGGLE_BUTTON_OFF);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiSizeGroupAdd (audioidint->szgrp [2], uiwidgetp);
  audioidint->items [audioidint->itemcount].selrb = uiwidgetp;

  logProcEnd (LOG_PROC, "uiaudioidAddItem", "");
}

static bool
uiaudioidSaveCallback (void *udata)
{
  uiaudioid_t    *uiaudioid = udata;

  return uiaudioidSave (uiaudioid, NULL);
}

static bool
uiaudioidSave (void *udata, nlist_t *chglist)
{
  uiaudioid_t    *uiaudioid = udata;
  aid_internal_t   *audioidint =  NULL;

  logProcBegin (LOG_PROC, "uiaudioidSaveCallback");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: song edit: save");
  audioidint = uiaudioid->audioidInternalData;

  uiLabelSetText (uiaudioid->statusMsg, "");

  if (uiaudioid->savecb != NULL) {
    /* the callback re-loads the song editor */
    callbackHandlerLong (uiaudioid->savecb, audioidint->dbidx);
  }

  logProcEnd (LOG_PROC, "uiaudioidSaveCallback", "");
  return UICB_CONT;
}

static bool
uiaudioidFirstSelection (void *udata)
{
  uiaudioid_t  *uiaudioid = udata;

  logProcBegin (LOG_PROC, "uiaudioidFirstSelection");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: song edit: first");
  uisongselFirstSelection (uiaudioid->uisongsel);
  logProcEnd (LOG_PROC, "uiaudioidFirstSelection", "");
  return UICB_CONT;
}

static bool
uiaudioidPreviousSelection (void *udata)
{
  uiaudioid_t  *uiaudioid = udata;

  logProcBegin (LOG_PROC, "uiaudioidPreviousSelection");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: song edit: previous");
  uisongselPreviousSelection (uiaudioid->uisongsel);
  logProcEnd (LOG_PROC, "uiaudioidPreviousSelection", "");
  return UICB_CONT;
}

static bool
uiaudioidNextSelection (void *udata)
{
  uiaudioid_t  *uiaudioid = udata;

  logProcBegin (LOG_PROC, "uiaudioidNextSelection");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: song edit: next");
  uisongselNextSelection (uiaudioid->uisongsel);
  logProcEnd (LOG_PROC, "uiaudioidNextSelection", "");
  return UICB_CONT;
}


static bool
uiaudioidKeyEvent (void *udata)
{
  uiaudioid_t    *uiaudioid = udata;
  aid_internal_t *audioidint;

  audioidint = uiaudioid->audioidInternalData;

  if (uiKeyIsPressEvent (audioidint->uikey) &&
      uiKeyIsAudioPlayKey (audioidint->uikey)) {
    uisongselPlayCallback (uiaudioid->uisongsel);
  }

  if (uiKeyIsPressEvent (audioidint->uikey)) {
    if (uiKeyIsControlPressed (audioidint->uikey)) {
      if (uiKeyIsKey (audioidint->uikey, 'S')) {
        uiaudioidSaveCallback (uiaudioid);
        return UICB_STOP;
      }
      if (uiKeyIsKey (audioidint->uikey, 'N')) {
        uiaudioidNextSelection (uiaudioid);
        return UICB_STOP;
      }
      if (uiKeyIsKey (audioidint->uikey, 'P')) {
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

  uiTreeViewSelectCurrent (audioidint->alistTree);
  idx = uiTreeViewGetValue (audioidint->alistTree, UIAID_COL_IDX);
  uiaudioidPopulateSelected (uiaudioid, idx - 1);

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

  if (idx >= 0 && idx < audioidint->rowcount) {
    dlist = nlistGetList (audioidint->displaylist, idx);
  }

  for (int count = 0; count < audioidint->itemcount; ++count) {
    int tagidx = audioidint->items [count].tagidx;
    uiToggleButtonSetState (audioidint->items [count].currrb, UI_TOGGLE_BUTTON_ON);
    uiToggleButtonSetState (audioidint->items [count].selrb, UI_TOGGLE_BUTTON_OFF);
    if (dlist == NULL) {
      uiToggleButtonSetText (audioidint->items [count].selrb, "");
    } else {
      uiToggleButtonSetText (audioidint->items [count].selrb,
          nlistGetStr (dlist, tagidx));
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

  uitreedispSetDisplayColumn (audioidint->alistTree, col, num, str);
}

static bool
uiaudioidItemSelectCallback (void *udata)
{
//  uiaudioid_t     *uiaudioid = udata;
fprintf (stderr, "item-sel callback\n");
  return UICB_CONT;
}
