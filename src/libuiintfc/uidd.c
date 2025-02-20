/*
 * Copyright 2024-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bdj4.h"
#include "callback.h"
#include "ilist.h"
#include "istring.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "slist.h"
#include "sysvars.h"
#include "ui.h"
#include "uidd.h"
#include "uivirtlist.h"
#include "uiwcont.h"

enum {
  DD_IDENT = 0xeeddccbbaa006464,
};

enum {
  DD_W_BUTTON,
  DD_W_DIALOG_WIN,
  DD_W_MAX,
};

enum {
  DD_CB_BUTTON,
  DD_CB_WIN_CLOSE,
  DD_CB_MAX,
};

enum {
  DD_COL_DISP,
  DD_COL_MAX,
};

enum {
  DD_MAX_ROWS = 10,
};

typedef struct uidd {
  uint64_t      ident;
  const char    *tag;
  uiwcont_t     *parentwin;
  uiwcont_t     *wcont [DD_W_MAX];
  char          *title;
  int           titleflag;
  callback_t    *callbacks [DD_CB_MAX];
  callback_t    *ddcb;
  uivirtlist_t  *uivl;
  ilist_t       *ddlist;
  nlist_t       *ddnumlookup;
  slist_t       *ddstrlookup;
  int           listtype;
  size_t        dispwidth;
  ilistidx_t    selectedidx;
  bool          dialogcreated;
  bool          inchange;
  bool          open;
} uidd_t;

static const char * const DD_DARKBG_CLASS = "bdj-dark-bg";

static void uiddCreateDialog (uidd_t *dd);
static bool uiddDisplay (void *udata);
static bool uiddWinClose (void *udata);
static void uiddFillRow (void *udata, uivirtlist_t *vl, int32_t rownum);
static void uiddSelected (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx);
static void uiddSetSelectionInternal (uidd_t *dd, ilistidx_t idx);
static void uiddSetButtonText (uidd_t *dd, const char *str);
static void uiddProcessList (uidd_t *dd);

uidd_t *
uiddCreate (const char *tag, uiwcont_t *parentwin, uiwcont_t *boxp, int where,
    ilist_t *ddlist, int listtype, const char *title, int titleflag,
    callback_t *ddcb)
{
  uidd_t      *dd = NULL;
  uiwcont_t   *uiwidget;

  dd = mdmalloc (sizeof (uidd_t));
  dd->ident = DD_IDENT;
  dd->tag = tag;
  for (int i = 0; i < DD_W_MAX; ++i) {
    dd->wcont [i] = NULL;
  }
  for (int i = 0; i < DD_CB_MAX; ++i) {
    dd->callbacks [i] = NULL;
  }
  dd->title = mdstrdup (title);
  dd->titleflag = titleflag;
  dd->ddcb = ddcb;
  dd->ddlist = ddlist;
  dd->ddnumlookup = NULL;
  dd->ddstrlookup = NULL;
  dd->dispwidth = 0;
  dd->listtype = listtype;
  dd->parentwin = parentwin;
  dd->selectedidx = 0;
  dd->dialogcreated = false;
  dd->open = false;
  dd->uivl = NULL;

  uiddProcessList (dd);

  dd->callbacks [DD_CB_BUTTON] = callbackInit (uiddDisplay, dd, NULL);

  uiwidget = uiCreateButton ("dd-down",
      dd->callbacks [DD_CB_BUTTON], NULL,
      "button_down_small");
  if (where == DD_PACK_START) {
    uiBoxPackStart (boxp, uiwidget);
  }
  if (where == DD_PACK_END) {
    uiBoxPackEnd (boxp, uiwidget);
  }
  if (isMacOS () || isWindows ()) {
    /* work around gtk not vertically centering the image */
    uiButtonSetImageMarginTop (uiwidget, 3);
  }
  uiButtonAlignLeft (uiwidget);
  uiButtonSetImagePosRight (uiwidget);
  uiWidgetAlignHorizStart (uiwidget);
  uiWidgetSetMarginTop (uiwidget, 1);
  uiWidgetSetMarginStart (uiwidget, 1);
  dd->wcont [DD_W_BUTTON] = uiwidget;
  uiddSetButtonText (dd, dd->title);

  dd->callbacks [DD_CB_WIN_CLOSE] = callbackInit (uiddWinClose, dd, NULL);

  return dd;
}

void
uiddFree (uidd_t *dd)
{
  if (dd == NULL || dd->ident != DD_IDENT) {
    return;
  }

  if (dd->open) {
    uiddWinClose (dd);
  }

  uivlFree (dd->uivl);
  for (int i = 0; i < DD_W_MAX; ++i) {
    uiwcontFree (dd->wcont [i]);
  }
  for (int i = 0; i < DD_CB_MAX; ++i) {
    callbackFree (dd->callbacks [i]);
  }
  nlistFree (dd->ddnumlookup);
  slistFree (dd->ddstrlookup);
  dataFree (dd->title);
  dd->dialogcreated = false;

  dd->ident = BDJ4_IDENT_FREE;
  mdfree (dd);
}

void
uiddSetList (uidd_t *dd, ilist_t *list)
{
  if (dd == NULL) {
    return;
  }

  dd->ddlist = list;
  uiddProcessList (dd);
  uivlSetColumnMinWidth (dd->uivl, DD_COL_DISP, dd->dispwidth);
  uivlSetNumRows (dd->uivl, ilistGetCount (dd->ddlist));
}

void
uiddSetSelection (uidd_t *dd, ilistidx_t idx)
{
  if (dd == NULL || dd->ident != DD_IDENT) {
    return;
  }

  if (idx < 0 || idx >= ilistGetCount (dd->ddlist)) {
    return;
  }

  dd->selectedidx = idx;
  if (dd->titleflag == DD_REPLACE_TITLE) {
    uiddSetButtonText (dd, ilistGetStr (dd->ddlist, idx, DD_LIST_DISP));
  }
  if (dd->dialogcreated) {
    uiddSetSelectionInternal (dd, idx);
  }
}

void
uiddSetSelectionByNumKey (uidd_t *dd, ilistidx_t key)
{
  ilistidx_t    idx;

  if (dd == NULL || dd->ident != DD_IDENT || dd->ddnumlookup == NULL) {
    return;
  }

  idx = nlistGetNum (dd->ddnumlookup, key);
  uiddSetSelection (dd, idx);
}

void
uiddSetSelectionByStrKey (uidd_t *dd, const char *key)
{
  ilistidx_t    idx;

  if (dd == NULL || dd->ident != DD_IDENT || dd->ddstrlookup == NULL) {
    return;
  }

  idx = slistGetNum (dd->ddstrlookup, key);
  uiddSetSelection (dd, idx);
}

void
uiddSetState (uidd_t *dd, int state)
{
  if (dd == NULL || dd->ident != DD_IDENT) {
    return;
  }

  uiWidgetSetState (dd->wcont [DD_W_BUTTON], state);
}

const char *
uiddGetSelectionStr (uidd_t *dd)
{
  if (dd == NULL || dd->ident != DD_IDENT) {
    return NULL;
  }

  return ilistGetStr (dd->ddlist, dd->selectedidx, DD_LIST_KEY_STR);
}

/* internal routines */

static void
uiddCreateDialog (uidd_t *dd)
{
  uiwcont_t   *vbox;
  int         count;
  int         dispcount;

  if (dd->dialogcreated) {
    return;
  }

  dd->wcont [DD_W_DIALOG_WIN] = uiCreateDialogWindow (dd->parentwin,
      dd->wcont [DD_W_BUTTON], dd->callbacks [DD_CB_WIN_CLOSE], "");
  uiWidgetAddClass (dd->wcont [DD_W_DIALOG_WIN], DD_DARKBG_CLASS);

  vbox = uiCreateVertBox ();
  uiWindowPackInWindow (dd->wcont [DD_W_DIALOG_WIN], vbox);
  uiWidgetSetAllMargins (vbox, 4);

  count = ilistGetCount (dd->ddlist);
  dispcount = count;
  if (dispcount > DD_MAX_ROWS) {
    dispcount = DD_MAX_ROWS;
  }
  dd->uivl = uivlCreate (dd->tag, NULL, vbox, dispcount,
      VL_NO_WIDTH, VL_NO_HEADING | VL_ENABLE_KEYS);
  uivlSetDropdownBackground (dd->uivl);
  uivlSetNumColumns (dd->uivl, DD_COL_MAX);
  uivlMakeColumn (dd->uivl, "disp", DD_COL_DISP, VL_TYPE_LABEL);
  uivlSetColumnMinWidth (dd->uivl, DD_COL_DISP, dd->dispwidth);
  uivlSetNumRows (dd->uivl, count);
  uivlSetRowFillCallback (dd->uivl, uiddFillRow, dd);
  uivlSetRowClickCallback (dd->uivl, uiddSelected, dd);

  uivlDisplay (dd->uivl);

  uiwcontFree (vbox);

  uiddSetSelectionInternal (dd, dd->selectedidx);

  dd->dialogcreated = true;
}


static bool
uiddDisplay (void *udata)
{
  uidd_t    *dd = udata;
  int       x, y, ws;
  int       h, w;
  int       dh, dw;
  int       mh, mw;
  int       bx, by;
  int       nx, ny;
  bool      changed;


  uiddCreateDialog (dd);

  bx = 0;
  by = 0;
  uiWindowGetPosition (dd->parentwin, &x, &y, &ws);
  uiWindowGetSize (dd->parentwin, &w, &h);
  uiWidgetGetPosition (dd->wcont [DD_W_BUTTON], &bx, &by);
  uiWidgetShowAll (dd->wcont [DD_W_DIALOG_WIN]);
  uivlPopulate (dd->uivl);
  nx = bx + x + 4;
  ny = by + y + 30;
  uiWindowMove (dd->wcont [DD_W_DIALOG_WIN], nx, ny, -1);
  uiWindowPresent (dd->wcont [DD_W_DIALOG_WIN]);

  uiWindowGetSize (dd->wcont [DD_W_DIALOG_WIN], &dw, &dh);
  uiWindowGetMonitorSize (dd->parentwin, &mw, &mh);
  changed = false;
  if (nx + dw > mw) {
    nx -= ((nx + dw) - mw);
    changed = true;
  }
  /* add 32 to account for the window title-bar */
  if (ny + dh > mh - 32) {
    ny -= ((ny + dh) - (mh - 32));
    changed = true;
  }
  if (changed) {
    uiWindowMove (dd->wcont [DD_W_DIALOG_WIN], nx, ny, -1);
  }

  dd->open = true;
  uivlUpdateDisplay (dd->uivl);

  return UICB_CONT;
}

static bool
uiddWinClose (void *udata)
{
  uidd_t    *dd = udata;

  if (dd->open) {
    uiWidgetHide (dd->wcont [DD_W_DIALOG_WIN]);
    dd->open = false;
  }
  uiWindowPresent (dd->parentwin);

  return UICB_CONT;
}

static void
uiddFillRow (void *udata, uivirtlist_t *uivl, int32_t rownum)
{
  uidd_t      *dd = udata;
  const char  *str;

  dd->inchange = true;
  str = ilistGetStr (dd->ddlist, rownum, DD_LIST_DISP);
  uivlSetRowColumnStr (dd->uivl, rownum, DD_COL_DISP, str);
  dd->inchange = false;
}

static void
uiddSelected (void *udata, uivirtlist_t *uivl, int32_t rownum, int colidx)
{
  uidd_t    *dd = udata;

  if (dd->inchange) {
    return;
  }

  dd->selectedidx = rownum;
  if (dd->titleflag == DD_REPLACE_TITLE) {
    uiddSetButtonText (dd, ilistGetStr (dd->ddlist, rownum, DD_LIST_DISP));
  }
  if (dd->ddcb != NULL) {
    if (dd->listtype == DD_LIST_TYPE_STR) {
      callbackHandlerS (dd->ddcb,
          ilistGetStr (dd->ddlist, rownum, DD_LIST_KEY_STR));
    }
    if (dd->listtype == DD_LIST_TYPE_NUM) {
      callbackHandlerI (dd->ddcb,
          ilistGetNum (dd->ddlist, rownum, DD_LIST_KEY_NUM));
    }
  }
  uiddWinClose (dd);
}

static void
uiddSetSelectionInternal (uidd_t *dd, ilistidx_t idx)
{
  dd->inchange = true;
  uivlSetSelection (dd->uivl, idx);
  dd->inchange = false;
}

static void
uiddSetButtonText (uidd_t *dd, const char *str)
{
  char    tbuff [100];

  snprintf (tbuff, sizeof (tbuff), "%-*s", (int) (dd->dispwidth), str);
  uiButtonSetText (dd->wcont [DD_W_BUTTON], tbuff);
}

static void
uiddProcessList (uidd_t *dd)
{
  ilistidx_t    iter;
  ilistidx_t    key;
  nlist_t       *ddnumlookup = NULL;
  slist_t       *ddstrlookup = NULL;

  if (dd->listtype == DD_LIST_TYPE_NUM) {
    ddnumlookup = nlistAlloc ("dd-num-lookup", LIST_UNORDERED, NULL);
    nlistSetSize (ddnumlookup, ilistGetCount (dd->ddlist));
  }
  if (dd->listtype == DD_LIST_TYPE_STR) {
    ddstrlookup = slistAlloc ("dd-str-lookup", LIST_UNORDERED, NULL);
    slistSetSize (ddnumlookup, ilistGetCount (dd->ddlist));
  }

  ilistStartIterator (dd->ddlist, &iter);
  while ((key = ilistIterateKey (dd->ddlist, &iter)) >= 0) {
    const char  *disp;
    size_t      len;

    disp = ilistGetStr (dd->ddlist, key, DD_LIST_DISP);
    len = istrlen (disp);
    if (len > dd->dispwidth) {
      dd->dispwidth = len;
    }
    if (dd->listtype == DD_LIST_TYPE_NUM) {
      nlistSetNum (ddnumlookup, ilistGetNum (dd->ddlist, key, DD_LIST_KEY_NUM), key);
    }
    if (dd->listtype == DD_LIST_TYPE_STR) {
      slistSetNum (ddstrlookup, ilistGetStr (dd->ddlist, key, DD_LIST_KEY_STR), key);
    }
  }

  if (dd->listtype == DD_LIST_TYPE_NUM) {
    nlistFree (dd->ddnumlookup);
    nlistSort (ddnumlookup);
    dd->ddnumlookup = ddnumlookup;
  }
  if (dd->listtype == DD_LIST_TYPE_STR) {
    slistFree (dd->ddstrlookup);
    slistSort (ddstrlookup);
    dd->ddstrlookup = ddstrlookup;
  }
}
