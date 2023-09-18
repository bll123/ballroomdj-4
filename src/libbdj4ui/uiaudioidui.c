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

#include "audioid.h"
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
  UIAUID_SEL_CURR,
  UIAUID_SEL_NEW,
};

enum {
  UIAUID_COL_ELLIPSIZE,
  UIAUID_COL_FONT,
  UIAUID_COL_COLOR,
  UIAUID_COL_COLOR_SET,
  UIAUID_COL_IDX,
  UIAUID_COL_MAX,
};

typedef struct {
  int             tagidx;
  uiwcont_t       *currrb;
  uiwcont_t       *selrb;
  callback_t      *callback;
  bool            selection : 1;
} uiaudioiditem_t;

enum {
  UIAUID_CB_FIRST,
  UIAUID_CB_SAVE,
  UIAUID_CB_PREV,
  UIAUID_CB_NEXT,
  UIAUID_CB_KEYB,
  UIAUID_CB_ROW_SELECT,
  UIAUID_CB_MAX,
};

enum {
  UIAUID_BUTTON_FIRST,
  UIAUID_BUTTON_PREV,
  UIAUID_BUTTON_NEXT,
  UIAUID_BUTTON_SAVE,
  UIAUID_BUTTON_MAX,
};

enum {
  UIAUID_MAIN_TIMER = 40,
};

enum {
  UIAUID_W_EDIT_ALL,
  UIAUID_W_PARENT_WIN,
  UIAUID_W_MAIN_VBOX,
  UIAUID_W_AUDIOID_IMG,
  UIAUID_W_FILE_DISP,
  UIAUID_W_MUSICBRAINZ,
  UIAUID_W_MAX,
};

enum {
  UIAUID_SZGRP_LABEL,
  UIAUID_SZGRP_COL_A,
  UIAUID_SZGRP_COL_B,
  UIAUID_SZGRP_MAX,
};

typedef struct aid_internal {
  uiwcont_t           *wcont [UIAUID_W_MAX];
  uiwcont_t           *szgrp [UIAUID_SZGRP_MAX];
  callback_t          *callbacks [UIAUID_CB_MAX];
  uibutton_t          *buttons [UIAUID_BUTTON_MAX];
  uitree_t            *alistTree;
  song_t              *song;
  uiaudioiditem_t     *items;
  int                 *typelist;
  slist_t             *listsellist;
  slist_t             *sellist;
  nlist_t             *displaylist;
  uikey_t             *uikey;
  dbidx_t             dbidx;
  int                 itemcount;
  int                 colcount;
  int                 rowcount;
  int                 currrow;
  int                 setrow;
  int                 changed;
  bool                selchgbypass : 1;
  bool                repeating : 1;
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
static void uiaudioidDisplayDuration (uiaudioid_t *uiaudioid, song_t *song);

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
  audioidint->repeating = false;
  audioidint->listsellist = dispselGetList (uiaudioid->dispsel, DISP_SEL_AUDIOID_LIST);
  audioidint->sellist = dispselGetList (uiaudioid->dispsel, DISP_SEL_AUDIOID);

  for (int i = 0; i < UIAUID_BUTTON_MAX; ++i) {
    audioidint->buttons [i] = NULL;
  }
  for (int i = 0; i < UIAUID_CB_MAX; ++i) {
    audioidint->callbacks [i] = NULL;
  }
  for (int i = 0; i < UIAUID_SZGRP_MAX; ++i) {
    audioidint->szgrp [i] = uiCreateSizeGroupHoriz ();
  }
  for (int i = 0; i < UIAUID_W_MAX; ++i) {
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
    uiWidgetClearPersistent (audioidint->wcont [UIAUID_W_MUSICBRAINZ]);

    for (int count = 0; count < audioidint->itemcount; ++count) {
      callbackFree (audioidint->items [count].callback);
      uiwcontFree (audioidint->items [count].currrb);
      uiwcontFree (audioidint->items [count].selrb);
    }
    dataFree (audioidint->typelist);

    for (int i = 0; i < UIAUID_SZGRP_MAX; ++i) {
      uiwcontFree (audioidint->szgrp [i]);
    }
    for (int i = 0; i < UIAUID_W_MAX; ++i) {
      if (i == UIAUID_W_PARENT_WIN) {
        continue;
      }
      uiwcontFree (audioidint->wcont [i]);
    }
    for (int i = 0; i < UIAUID_BUTTON_MAX; ++i) {
      uiButtonFree (audioidint->buttons [i]);
    }
    for (int i = 0; i < UIAUID_CB_MAX; ++i) {
      callbackFree (audioidint->callbacks [i]);
    }

    nlistFree (audioidint->displaylist);
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
  char              tbuff [MAXPATHLEN];

  logProcBegin (LOG_PROC, "uiaudioidBuildUI");

  uiaudioid->statusMsg = statusMsg;
  uiaudioid->uisongsel = uisongsel;
  audioidint = uiaudioid->audioidInternalData;
  audioidint->wcont [UIAUID_W_PARENT_WIN] = parentwin;

  audioidint->wcont [UIAUID_W_MAIN_VBOX] = uiCreateVertBox ();
  uiWidgetExpandHoriz (audioidint->wcont [UIAUID_W_MAIN_VBOX]);

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiWidgetAlignHorizFill (hbox);
  uiBoxPackStart (audioidint->wcont [UIAUID_W_MAIN_VBOX], hbox);

  audioidint->callbacks [UIAUID_CB_FIRST] = callbackInit (
      uiaudioidFirstSelection, uiaudioid, "songedit: first");
  uibutton = uiCreateButton (audioidint->callbacks [UIAUID_CB_FIRST],
      /* CONTEXT: song editor : first song */
      _("First"), NULL);
  audioidint->buttons [UIAUID_BUTTON_FIRST] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackStart (hbox, uiwidgetp);

  audioidint->callbacks [UIAUID_CB_PREV] = callbackInit (
      uiaudioidPreviousSelection, uiaudioid, "songedit: previous");
  uibutton = uiCreateButton (audioidint->callbacks [UIAUID_CB_PREV],
      /* CONTEXT: song editor : previous song */
      _("Previous"), NULL);
  audioidint->buttons [UIAUID_BUTTON_PREV] = uibutton;
  uiButtonSetRepeat (uibutton, REPEAT_TIME);
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackStart (hbox, uiwidgetp);

  audioidint->callbacks [UIAUID_CB_NEXT] = callbackInit (
      uiaudioidNextSelection, uiaudioid, "songedit: next");
  uibutton = uiCreateButton (audioidint->callbacks [UIAUID_CB_NEXT],
      /* CONTEXT: song editor : next song */
      _("Next"), NULL);
  audioidint->buttons [UIAUID_BUTTON_NEXT] = uibutton;
  uiButtonSetRepeat (uibutton, REPEAT_TIME);
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackStart (hbox, uiwidgetp);

  audioidint->callbacks [UIAUID_CB_SAVE] = callbackInit (
      uiaudioidSaveCallback, uiaudioid, "songedit: save");
  uibutton = uiCreateButton (audioidint->callbacks [UIAUID_CB_SAVE],
      /* CONTEXT: song editor : save data */
      _("Save"), NULL);
  audioidint->buttons [UIAUID_BUTTON_SAVE] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackEnd (hbox, uiwidgetp);

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetSetMarginEnd (uiwidgetp, 6);
  uiWidgetSetClass (uiwidgetp, DARKACCENT_CLASS);
  audioidint->wcont [UIAUID_W_EDIT_ALL] = uiwidgetp;

  uiwcontFree (hbox);

  /* begin line */

  /* audio-identification logo, modified indicator, */
  /* copy button, file label, filename */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiWidgetAlignHorizFill (hbox);
  uiBoxPackStart (audioidint->wcont [UIAUID_W_MAIN_VBOX], hbox);

  pathbldMakePath (tbuff, sizeof (tbuff), "musicbrainz-logo", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DIR_IMG);
  audioidint->wcont [UIAUID_W_MUSICBRAINZ] = uiImageFromFile (tbuff);
  uiImageConvertToPixbuf (audioidint->wcont [UIAUID_W_MUSICBRAINZ]);
  uiWidgetMakePersistent (audioidint->wcont [UIAUID_W_MUSICBRAINZ]);

  audioidint->wcont [UIAUID_W_AUDIOID_IMG] = uiImageNew ();
  uiImageClear (audioidint->wcont [UIAUID_W_AUDIOID_IMG]);
  uiWidgetSetSizeRequest (audioidint->wcont [UIAUID_W_AUDIOID_IMG], 24, -1);
  uiWidgetSetMarginStart (audioidint->wcont [UIAUID_W_AUDIOID_IMG], 1);
  uiBoxPackStart (hbox, audioidint->wcont [UIAUID_W_AUDIOID_IMG]);

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
  audioidint->wcont [UIAUID_W_FILE_DISP] = uiwidgetp;

  uiwcontFree (hbox);

  pw = uiPanedWindowCreateVert ();
  uiWidgetExpandHoriz (pw);
  uiWidgetAlignHorizFill (pw);
  uiBoxPackStartExpand (audioidint->wcont [UIAUID_W_MAIN_VBOX], pw);
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

  audioidint->callbacks [UIAUID_CB_ROW_SELECT] = callbackInit (
        uiaudioidRowSelect, uiaudioid, NULL);
  uiTreeViewSetSelectChangedCallback (audioidint->alistTree,
        audioidint->callbacks [UIAUID_CB_ROW_SELECT]);

  uitreedispAddDisplayColumns (audioidint->alistTree, audioidint->listsellist,
      UIAUID_COL_MAX, UIAUID_COL_FONT, UIAUID_COL_ELLIPSIZE,
      UIAUID_COL_COLOR, UIAUID_COL_COLOR_SET);

  /* current/selected box */

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiWidgetAlignHorizFill (hbox);
  uiPanedWindowPackEnd (pw, hbox);

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

  /* headings */

  hhbox = uiCreateHorizBox ();
  uiBoxPackStart (col, hhbox);

  uiwidgetp = uiCreateLabel ("");
  uiWidgetSetMarginEnd (uiwidgetp, 4);
  uiBoxPackStart (hhbox, uiwidgetp);
  uiSizeGroupAdd (audioidint->szgrp [UIAUID_SZGRP_LABEL], uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel (" ");
  uiBoxPackStart (hhbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  snprintf (tbuff, sizeof (tbuff), "%s bold", bdjoptGetStr (OPT_MP_UIFONT));

  uiwidgetp = uiCreateLabel (_("Current"));
  uiLabelSetFont (uiwidgetp, tbuff);
  uiWidgetSetMarginEnd (uiwidgetp, 4);
  uiSizeGroupAdd (audioidint->szgrp [UIAUID_SZGRP_COL_A], uiwidgetp);
  uiBoxPackStartExpand (hhbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel (" ");
  uiBoxPackStart (hhbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel (_("Selected"));
  uiLabelSetFont (uiwidgetp, tbuff);
  uiSizeGroupAdd (audioidint->szgrp [UIAUID_SZGRP_COL_B], uiwidgetp);
  uiBoxPackStartExpand (hhbox, uiwidgetp);
  uiwcontFree (uiwidgetp);
  uiwcontFree (hhbox);

  uiaudioidAddDisplay (uiaudioid, col);

  uiwcontFree (col);
  uiwcontFree (hbox);
  uiwcontFree (pw);

  audioidint->uikey = uiKeyAlloc ();
  audioidint->callbacks [UIAUID_CB_KEYB] = callbackInit (
      uiaudioidKeyEvent, uiaudioid, NULL);
  uiKeySetKeyCallback (audioidint->uikey, audioidint->wcont [UIAUID_W_MAIN_VBOX],
      audioidint->callbacks [UIAUID_CB_KEYB]);

  /* create tree value store */

  audioidint->typelist = mdmalloc (sizeof (int) * UIAUID_COL_MAX);
  audioidint->colcount = 0;
  audioidint->rowcount = 0;
  audioidint->typelist [UIAUID_COL_ELLIPSIZE] = TREE_TYPE_ELLIPSIZE;
  audioidint->typelist [UIAUID_COL_FONT] = TREE_TYPE_STRING;
  audioidint->typelist [UIAUID_COL_COLOR] = TREE_TYPE_STRING;
  audioidint->typelist [UIAUID_COL_COLOR_SET] = TREE_TYPE_BOOLEAN;
  audioidint->typelist [UIAUID_COL_IDX] = TREE_TYPE_NUM;
  audioidint->colcount = UIAUID_COL_MAX;

  uisongAddDisplayTypes (audioidint->listsellist, uiaudioidDisplayTypeCallback,
      uiaudioid);
  uiTreeViewCreateValueStoreFromList (audioidint->alistTree, audioidint->colcount, audioidint->typelist);

  logProcEnd (LOG_PROC, "uiaudioidBuildUI", "");
  return audioidint->wcont [UIAUID_W_MAIN_VBOX];
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

  logProcBegin (LOG_PROC, "uiaudioidLoadData");
  audioidint = uiaudioid->audioidInternalData;
  audioidint->song = song;
  audioidint->dbidx = dbidx;
  audioidint->changed = 0;
  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);
  audioidint->selchgbypass = true;

  tval = uisongGetDisplay (song, TAG_FILE, &val, &dval);
  uiLabelSetText (audioidint->wcont [UIAUID_W_FILE_DISP], tval);
  dataFree (tval);
  tval = NULL;

  uiImageClear (audioidint->wcont [UIAUID_W_AUDIOID_IMG]);
  data = songGetStr (song, TAG_RECORDING_ID);
  if (data != NULL && *data) {
    uiImageSetFromPixbuf (audioidint->wcont [UIAUID_W_AUDIOID_IMG], audioidint->wcont [UIAUID_W_MUSICBRAINZ]);
  }

  for (int count = 0; count < audioidint->itemcount; ++count) {
    int   tagidx = audioidint->items [count].tagidx;
    char  tmp [40];

    tval = uisongGetDisplay (song, tagidx, &val, &dval);
    if (tval == NULL && val != LIST_VALUE_INVALID) {
      snprintf (tmp, sizeof (tmp), "%ld", val);
      dataFree (tval);
      tval = mdstrdup (tmp);
    }
    uiToggleButtonSetText (audioidint->items [count].currrb, tval);
    uiToggleButtonSetText (audioidint->items [count].selrb, "");
    dataFree (tval);
    tval = NULL;
  }

  row = 0;

  if (row >= audioidint->rowcount) {
    uiTreeViewValueAppend (audioidint->alistTree);
    uiTreeViewSetValueEllipsize (audioidint->alistTree, UIAUID_COL_ELLIPSIZE);
    uiTreeViewSetValues (audioidint->alistTree,
        UIAUID_COL_FONT, listingFont,
        UIAUID_COL_COLOR, bdjoptGetStr (OPT_P_UI_ACCENT_COL),
        UIAUID_COL_COLOR_SET, (treebool_t) false,
        UIAUID_COL_IDX, (treenum_t) 0,
        TREE_VALUE_END);
  } else {
    uiTreeViewSelectSet (audioidint->alistTree, row);
    /* no data in row 0 to update */
  }

  uiTreeViewSelectSet (audioidint->alistTree, row);
  audioidint->currrow = row;
  uisongSetDisplayColumns (audioidint->listsellist, song, UIAUID_COL_MAX,
      uiaudioidSetSongDataCallback, uiaudioid);
  uiaudioidDisplayDuration (uiaudioid, song);
  uiTreeViewSetValues (audioidint->alistTree,
      UIAUID_COL_COLOR_SET, (treebool_t) true, TREE_VALUE_END);

  ++row;
  uiTreeViewValueClear (audioidint->alistTree, row);
  audioidint->rowcount = row;

  if (audioidint->rowcount > 1) {
    uiTreeViewSelectFirst (audioidint->alistTree);
    audioidint->selchgbypass = false;
    uiTreeViewSelectNext (audioidint->alistTree);
  }
  audioidint->selchgbypass = false;

  audioidint->setrow = 1;
  nlistFree (audioidint->displaylist);
  audioidint->displaylist = nlistAlloc ("uiaudioid-disp", LIST_UNORDERED, NULL);
  logProcEnd (LOG_PROC, "uiaudioidLoadData", "");
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

  audioidint = uiaudioid->audioidInternalData;
  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);
  audioidint->selchgbypass = true;
  ndlist = nlistAlloc ("uiaudio-ndlist", LIST_UNORDERED, NULL);
  nlistSetSize (ndlist, nlistGetCount (dlist));

  if (audioidint->setrow >= audioidint->rowcount) {
    uiTreeViewValueAppend (audioidint->alistTree);
    uiTreeViewSetValueEllipsize (audioidint->alistTree, UIAUID_COL_ELLIPSIZE);
    /* color-set isn't working for me within this module */
    /* so set the color to null */
    uiTreeViewSetValues (audioidint->alistTree,
        UIAUID_COL_FONT, listingFont,
        UIAUID_COL_COLOR, NULL,
        UIAUID_COL_COLOR_SET, (treebool_t) false,
        UIAUID_COL_IDX, (treenum_t) audioidint->setrow,
        TREE_VALUE_END);
  }

  /* all data must be copied */
  slistStartIterator (audioidint->sellist, &seliteridx);
  while ((tagidx = slistIterateValueNum (audioidint->sellist, &seliteridx)) >= 0) {
    const char  *str;

    str = nlistGetStr (dlist, tagidx);
    nlistSetStr (ndlist, tagidx, str);
  }

  uiTreeViewSelectSet (audioidint->alistTree, audioidint->setrow);
  col = UIAUID_COL_MAX;
  slistStartIterator (audioidint->listsellist, &seliteridx);
  while ((tagidx = slistIterateValueNum (audioidint->listsellist, &seliteridx)) >= 0) {
    audioidint->currrow = audioidint->setrow;
    if (tagidx == TAG_AUDIOID_SCORE) {
      char    tmp [40];
      double  dval;

      dval = nlistGetDouble (dlist, tagidx);
      nlistSetDouble (ndlist, tagidx, dval);
      tmp [0] = '\0';
      if (dval > 0.0) {
        snprintf (tmp, sizeof (tmp), "%.1f", dval);
      }
      uitreedispSetDisplayColumn (audioidint->alistTree, col, 0, tmp);
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
      uitreedispSetDisplayColumn (audioidint->alistTree, col, 0, tmp);
    } else {
      const char  *str;

      str = nlistGetStr (dlist, tagidx);
      uitreedispSetDisplayColumn (audioidint->alistTree, col, 0, str);
    }
    ++col;
  }

  nlistSort (ndlist);
  nlistSetList (audioidint->displaylist, audioidint->setrow, ndlist);
  ++audioidint->setrow;
  logProcEnd (LOG_PROC, "uiaudioidLoadData", "");
}

void
uiaudioidFinishDisplayList (uiaudioid_t *uiaudioid)
{
  aid_internal_t  *audioidint;

  audioidint = uiaudioid->audioidInternalData;

  uiTreeViewValueClear (audioidint->alistTree, audioidint->setrow);
  audioidint->rowcount = audioidint->setrow;
  nlistSort (audioidint->displaylist);

  if (audioidint->rowcount > 1) {
    uiTreeViewSelectFirst (audioidint->alistTree);
    audioidint->selchgbypass = false;
    uiTreeViewSelectNext (audioidint->alistTree);
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
  repeat = uiButtonCheckRepeat (audioidint->buttons [UIAUID_BUTTON_NEXT]);
  if (repeat) {
    audioidint->repeating = true;
  }
  repeat = uiButtonCheckRepeat (audioidint->buttons [UIAUID_BUTTON_PREV]);
  if (repeat) {
    audioidint->repeating = true;
  }
  return;
}

/* internal routines */

static void
uiaudioidAddDisplay (uiaudioid_t *uiaudioid, uiwcont_t *col)
{
  slistidx_t      dsiteridx;
  aid_internal_t  *audioidint;
  int             tagidx;

  logProcBegin (LOG_PROC, "uiaudioidAddDisplay");
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
  uiSizeGroupAdd (audioidint->szgrp [UIAUID_SZGRP_LABEL], uiwidgetp);
  uiwcontFree (uiwidgetp);

  rb = uiCreateRadioButton (NULL, "", UI_TOGGLE_BUTTON_ON);
  uiBoxPackStartExpand (hbox, rb);
  uiSizeGroupAdd (audioidint->szgrp [UIAUID_SZGRP_COL_A], rb);
  audioidint->items [audioidint->itemcount].currrb = rb;

  uiwidgetp = uiCreateRadioButton (rb, "", UI_TOGGLE_BUTTON_OFF);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiSizeGroupAdd (audioidint->szgrp [UIAUID_SZGRP_COL_B], uiwidgetp);
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
  idx = uiTreeViewGetValue (audioidint->alistTree, UIAUID_COL_IDX);
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

      uiToggleButtonSetText (audioidint->items [count].selrb, tval);
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

    col += UIAUID_COL_MAX;
    tmp = songDisplayString (song, TAG_DURATION, SONG_UNADJUSTED_DURATION);
    uitreedispSetDisplayColumn (audioidint->alistTree, col, 0, tmp);
    dataFree (tmp);
  }
}
