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
#include "conn.h"
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
#include "uisong.h"
#include "uisongsel.h"
#include "uiaudioid.h"

enum {
  UIAID_SEL_CURR,
  UIAID_SEL_NEW,
};

typedef struct {
  int             tagkey;
  uichgind_t      *currchgind;
  uiwcont_t       *currdisp;
  uichgind_t      *newchgind;
  uiwcont_t       *newdisp;
  callback_t      *callback;
  bool            selection : 1;
} uiaudioiditem_t;

enum {
  UIAID_CB_FIRST,
  UIAID_CB_SAVE,
  UIAID_CB_PREV,
  UIAID_CB_NEXT,
  UIAID_CB_KEYB,
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
  song_t              *song;
  dbidx_t             dbidx;
  int                 itemcount;
  uiaudioiditem_t     *items;
  int                 changed;
  uikey_t             *uikey;
} aid_internal_t;

static void uiaudioidAddDisplay (uiaudioid_t *songedit, uiwcont_t *col);
static void uiaudioidAddItem (uiaudioid_t *uiaudioid, uiwcont_t *hbox, int tagkey);
static bool uiaudioidSaveCallback (void *udata);
static bool uiaudioidSave (void *udata, nlist_t *chglist);
static bool uiaudioidFirstSelection (void *udata);
static bool uiaudioidPreviousSelection (void *udata);
static bool uiaudioidNextSelection (void *udata);
static bool uiaudioidKeyEvent (void *udata);

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
      uichgindFree (audioidint->items [count].currchgind);
      uiwcontFree (audioidint->items [count].currdisp);
      uichgindFree (audioidint->items [count].newchgind);
      uiwcontFree (audioidint->items [count].newdisp);
    }

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
  uiwcont_t         *hbox;
  uibutton_t        *uibutton;
  uiwcont_t         *uiwidgetp;
  int               count;
  uiwcont_t         *col;
  uiwcont_t         *hhbox;
  slist_t           *sellist;

  logProcBegin (LOG_PROC, "uiaudioidBuildUI");
  logProcBegin (LOG_PROC, "uiaudioidBuildUI");

  uiaudioid->statusMsg = statusMsg;
  uiaudioid->uisongsel = uisongsel;
  audioidint = uiaudioid->audioidInternalData;
  audioidint->wcont [UIAID_W_PARENT_WIN] = parentwin;

  audioidint->wcont [UIAID_W_MAIN_VBOX] = uiCreateVertBox ();
  uiWidgetExpandHoriz (audioidint->wcont [UIAID_W_MAIN_VBOX]);

  audioidint->uikey = uiKeyAlloc ();
  audioidint->callbacks [UIAID_CB_KEYB] = callbackInit (
      uiaudioidKeyEvent, uiaudioid, NULL);
  uiKeySetKeyCallback (audioidint->uikey, audioidint->wcont [UIAID_W_MAIN_VBOX],
      audioidint->callbacks [UIAID_CB_KEYB]);

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

  /* CONTEXT: song editor: label for displaying the audio file path */
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

  /* begin line */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiWidgetAlignHorizFill (hbox);
  uiBoxPackStartExpand (audioidint->wcont [UIAID_W_MAIN_VBOX], hbox);

  sellist = dispselGetList (uiaudioid->dispsel, DISP_SEL_AUDIOID);
  count = slistGetCount (sellist);

  /* the items must all be alloc'd beforehand so that the callback */
  /* pointer is static */
  /* need to add 1 for the BPM display secondary display */
  audioidint->items = mdmalloc (sizeof (uiaudioiditem_t) * (count + 1));
  for (int i = 0; i < count + 1; ++i) {
    audioidint->items [i].tagkey = 0;
    audioidint->items [i].currchgind = NULL;
    audioidint->items [i].currdisp = NULL;
    audioidint->items [i].newchgind = NULL;
    audioidint->items [i].newdisp = NULL;
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

  uiwidgetp = uiCreateLabel (_("Current"));
  uiWidgetSetMarginEnd (uiwidgetp, 4);
  uiSizeGroupAdd (audioidint->szgrp [1], uiwidgetp);
  uiBoxPackStartExpand (hhbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel (" ");
  uiBoxPackStart (hhbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel (_("New"));
  uiSizeGroupAdd (audioidint->szgrp [2], uiwidgetp);
  uiBoxPackStartExpand (hhbox, uiwidgetp);
  uiwcontFree (uiwidgetp);
  uiwcontFree (hhbox);

  uiaudioidAddDisplay (uiaudioid, col);

  uiwcontFree (col);
  uiwcontFree (hbox);

  logProcEnd (LOG_PROC, "uiaudioidBuildUI", "");
  return audioidint->wcont [UIAID_W_MAIN_VBOX];
}

void
uiaudioidLoadData (uiaudioid_t *uiaudioid, song_t *song,
    dbidx_t dbidx)
{
  aid_internal_t   *audioidint;
  char            *tval;
  long            val;
  double          dval;

  logProcBegin (LOG_PROC, "uiaudioidLoadData");
  audioidint = uiaudioid->audioidInternalData;
  audioidint->song = song;
  audioidint->dbidx = dbidx;
  audioidint->changed = 0;

  tval = uisongGetDisplay (song, TAG_FILE, &val, &dval);
  uiLabelSetText (audioidint->wcont [UIAID_W_FILE_DISP], tval);
  dataFree (tval);
  tval = NULL;

  for (int count = 0; count < audioidint->itemcount; ++count) {
    int tagkey = audioidint->items [count].tagkey;

    tval = uisongGetDisplay (song, tagkey, &val, &dval);
    uiLabelSetText (audioidint->items [count].currdisp, tval);
    uiLabelSetText (audioidint->items [count].newdisp, tval);
    dataFree (tval);
    tval = NULL;
  }

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
  const char      *keystr;
  slistidx_t      dsiteridx;
  aid_internal_t  *audioidint;

  logProcBegin (LOG_PROC, "uiaudioidAddDisplay");
  audioidint = uiaudioid->audioidInternalData;
  sellist = dispselGetList (uiaudioid->dispsel, DISP_SEL_AUDIOID);

  slistStartIterator (sellist, &dsiteridx);
  while ((keystr = slistIterateKey (sellist, &dsiteridx)) != NULL) {
    uiwcont_t   *hbox;
    int         tagkey;

    tagkey = slistGetNum (sellist, keystr);

    if (tagkey < 0 || tagkey >= TAG_KEY_MAX) {
      continue;
    }
    if (! tagdefs [tagkey].isAudioID) {
      continue;
    }

    hbox = uiCreateHorizBox ();
    uiWidgetExpandHoriz (hbox);
    uiBoxPackStart (col, hbox);

    uiaudioidAddItem (uiaudioid, hbox, tagkey);
    audioidint->items [audioidint->itemcount].tagkey = tagkey;
    ++audioidint->itemcount;

    uiwcontFree (hbox);
  }

  logProcEnd (LOG_PROC, "uiaudioidAddDisplay", "");
}

static void
uiaudioidAddItem (uiaudioid_t *uiaudioid, uiwcont_t *hbox, int tagkey)
{
  uiwcont_t       *uiwidgetp;
  aid_internal_t  *audioidint;

  logProcBegin (LOG_PROC, "uiaudioidAddItem");
  audioidint = uiaudioid->audioidInternalData;

  /* line: label, curr-change-ind, curr-display, new-change-ind, new-disp */

  uiwidgetp = uiCreateColonLabel (tagdefs [tagkey].displayname);
  uiWidgetSetMarginEnd (uiwidgetp, 4);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (audioidint->szgrp [0], uiwidgetp);
  uiwcontFree (uiwidgetp);

  audioidint->items [audioidint->itemcount].currchgind = uiCreateChangeIndicator (hbox);
  uichgindMarkNormal (audioidint->items [audioidint->itemcount].currchgind);

  uiwidgetp = uiCreateLabel ("");
  uiLabelEllipsizeOn (uiwidgetp);
  uiWidgetSetMarginEnd (uiwidgetp, 4);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiSizeGroupAdd (audioidint->szgrp [1], uiwidgetp);
  audioidint->items [audioidint->itemcount].currdisp = uiwidgetp;

  audioidint->items [audioidint->itemcount].newchgind = uiCreateChangeIndicator (hbox);
  uichgindMarkNormal (audioidint->items [audioidint->itemcount].newchgind);

  uiwidgetp = uiCreateLabel ("");
  uiLabelEllipsizeOn (uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiSizeGroupAdd (audioidint->szgrp [2], uiwidgetp);
  audioidint->items [audioidint->itemcount].newdisp = uiwidgetp;

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

