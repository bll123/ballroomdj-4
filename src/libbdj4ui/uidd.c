/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
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
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "slist.h"
#include "ui.h"
#include "uidd.h"
#include "uivirtlist.h"
#include "uiwcont.h"

enum {
  DD_IDENT = 0x646400aabbccddee,
};

enum {
  DD_W_BUTTON,
  DD_W_WINDOW,
  DD_W_MAX,
};

enum {
  DD_CB_BUTTON,
  DD_CB_WIN_CLOSE,
  DD_CB_LEAVE,
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
  int64_t       ident;
  const char    *tag;
  uiwcont_t     *parentwin;
  uiwcont_t     *wcont [DD_W_MAX];
  char          *title;
  int           titleflag;
  callback_t    *callbacks [DD_CB_MAX];
  callback_t    *ddcb;
  uivirtlist_t  *uivl;
  nlist_t       *keylist;
  nlist_t       *displist;
  int           listtype;
  int           dispwidth;
  nlistidx_t    selectedidx;
  bool          dialogcreated : 1;
  bool          inchange : 1;
  bool          open : 1;
} uidd_t;

static void uiddCreateDialog (uidd_t *dd);
static bool uiddDisplay (void *udata);
static bool uiddWinClose (void *udata);
static void uiddFillRow (void *udata, uivirtlist_t *vl, int32_t rownum);
static void uiddSelected (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx);
static void uiddSetSelectionInternal (uidd_t *dd, nlistidx_t idx);
static void uiddSetButtonText (uidd_t *dd, const char *str);

uidd_t *
uiddCreate (const char *tag, uiwcont_t *parentwin, uiwcont_t *boxp,
    nlist_t *keylist, nlist_t *displist, int listtype,
    const char *title, int titleflag,
    callback_t *ddcb)
{
  uidd_t    *dd = NULL;
  uiwcont_t *uiwidget;

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
  dd->keylist = keylist;
  dd->displist = displist;
  nlistCalcMaxValueWidth (dd->displist);
  dd->dispwidth = nlistGetMaxValueWidth (dd->displist);
  dd->listtype = listtype;
  dd->parentwin = parentwin;
  dd->selectedidx = 0;
  dd->dialogcreated = false;
  dd->open = false;

  dd->callbacks [DD_CB_BUTTON] = callbackInit (uiddDisplay, dd, NULL);

  uiwidget = uiCreateButton (dd->callbacks [DD_CB_BUTTON], NULL,
      "button_down_small");
  uiButtonAlignLeft (uiwidget);
  uiButtonSetImagePosRight (uiwidget);
  uiWidgetAlignHorizStart (uiwidget);
  uiWidgetSetMarginTop (uiwidget, 1);
  uiWidgetSetMarginStart (uiwidget, 1);
  uiBoxPackStart (boxp, uiwidget);
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

  dd->open = false;
  dd->dialogcreated = false;

  for (int i = 0; i < DD_CB_MAX; ++i) {
    callbackFree (dd->callbacks [i]);
  }
  for (int i = 0; i < DD_W_MAX; ++i) {
    uiwcontFree (dd->wcont [i]);
  }
  dataFree (dd->title);
  uivlFree (dd->uivl);

  dd->ident = BDJ4_IDENT_FREE;
  mdfree (dd);
}

void
uiddSetSelection (uidd_t *dd, nlistidx_t idx)
{
  if (dd == NULL || dd->ident != DD_IDENT) {
    return;
  }

  if (idx < 0 || idx >= nlistGetCount (dd->keylist)) {
    return;
  }

  dd->selectedidx = idx;
  if (dd->titleflag == DD_REPLACE_TITLE) {
    uiddSetButtonText (dd, nlistGetStr (dd->displist, idx));
  }
  if (dd->dialogcreated) {
    uiddSetSelectionInternal (dd, idx);
  }
}

void
uiddSetState (uidd_t *dd, int state)
{
  if (dd == NULL || dd->ident != DD_IDENT) {
    return;
  }
}

const char *
uiddGetSelectionStr (uidd_t *dd)
{
  if (dd == NULL || dd->ident != DD_IDENT) {
    return NULL;
  }

  return NULL;
}

int
uiddGetSelectionNum (uidd_t *dd)
{
  if (dd == NULL || dd->ident != DD_IDENT) {
    return DD_NO_SELECTION;
  }

  return DD_NO_SELECTION;
}

/* internal routines */

static void
uiddCreateDialog (uidd_t *dd)
{
  uiwcont_t   *vbox;
  uiwcont_t   *uiwidget;
  int         count;
  int         dispcount;

  if (dd->dialogcreated) {
    return;
  }

  dd->wcont [DD_W_WINDOW] = uiCreateDialogWindow (dd->parentwin,
      dd->wcont [DD_W_BUTTON], dd->callbacks [DD_CB_WIN_CLOSE], "");

  uiwidget = uiCreateVertBox ();
  uiWindowPackInWindow (dd->wcont [DD_W_WINDOW], uiwidget);

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (uiwidget, 4);
//  uiWidgetExpandHoriz (vbox);
  uiBoxPackStart (uiwidget, vbox);

  uiwcontFree (uiwidget);

  count = nlistGetCount (dd->keylist);
  dispcount = count;
  if (dispcount > DD_MAX_ROWS) {
    dispcount = DD_MAX_ROWS;
  }
  dd->uivl = uiCreateVirtList (dd->tag, vbox, dispcount, VL_NO_HEADING, VL_NO_WIDTH);
  uivlSetDarkBackground (dd->uivl);
  uivlSetNumColumns (dd->uivl, DD_COL_MAX);
  uivlMakeColumn (dd->uivl, "disp", DD_COL_DISP, VL_TYPE_LABEL);
  uivlSetColumnMinWidth (dd->uivl, DD_COL_DISP, dd->dispwidth);
  uivlSetNumRows (dd->uivl, count);
  uivlSetRowFillCallback (dd->uivl, uiddFillRow, dd);
  uivlSetSelectionCallback (dd->uivl, uiddSelected, dd);

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
  int       bx, by;


  uiddCreateDialog (dd);

  bx = 0;
  by = 0;
  uiWindowGetPosition (dd->parentwin, &x, &y, &ws);
  uiWidgetGetPosition (dd->wcont [DD_W_BUTTON], &bx, &by);
  uiWidgetShowAll (dd->wcont [DD_W_WINDOW]);
  uiWindowMove (dd->wcont [DD_W_WINDOW], bx + x + 4, by + y + 4 + 30, -1);
  uiWindowPresent (dd->wcont [DD_W_WINDOW]);
  dd->open = true;

  return UICB_CONT;
}

static bool
uiddWinClose (void *udata)
{
  uidd_t    *dd = udata;

  if (dd->open) {
    uiWidgetHide (dd->wcont [DD_W_WINDOW]);
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
  str = nlistGetStr (dd->displist, rownum);
  uivlSetRowColumnValue (dd->uivl, rownum, DD_COL_DISP, str);
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
    uiddSetButtonText (dd, nlistGetStr (dd->displist, rownum));
  }
  if (dd->ddcb != NULL) {
    if (dd->listtype == DD_LIST_TYPE_STR) {
      callbackHandlerS (dd->ddcb, nlistGetStr (dd->keylist, rownum));
    }
    if (dd->listtype == DD_LIST_TYPE_NUM) {
      callbackHandlerI (dd->ddcb, nlistGetNum (dd->keylist, rownum));
    }
  }
  uiddWinClose (dd);
}

static void
uiddSetSelectionInternal (uidd_t *dd, nlistidx_t idx)
{
  dd->inchange = true;
  uivlSetSelection (dd->uivl, idx);
  dd->inchange = false;
}

static void
uiddSetButtonText (uidd_t *dd, const char *str)
{
  char    tbuff [100];

  /* spaces are narrower than most characters, so add a bunch of padding */
  snprintf (tbuff, sizeof (tbuff), "%-*s", dd->dispwidth + 15, str);
  uiButtonSetText (dd->wcont [DD_W_BUTTON], tbuff);
}
