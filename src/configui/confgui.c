/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
#include "bdjopt.h"
#include "bdjstring.h"
#include "callback.h"
#include "configui.h"
#include "istring.h"
#include "log.h"
#include "mdebug.h"
#include "sysvars.h"
#include "tmutil.h"
#include "ui.h"
#include "uidd.h"
#include "uiduallist.h"
#include "uisbnum.h"
#include "uisbtext.h"
#include "uiheadline.h"
#include "validate.h"

enum {
  CONFUI_EXPAND,
  CONFUI_NO_EXPAND,
};

static void confuiMakeItemEntryBasic (confuigui_t *gui, uiwcont_t *grid, const char *txt, int widx, int bdjoptIdx, const char *disp, int indent, int expand);
static bool confuiLinkCallback (void *udata);
static bool confuiValidateCallback (void *udata);

void
confuiMakeNotebookTab (uiwcont_t *grid, confuigui_t *gui, const char *txt, int id)
{
  uivnbAppendPage (gui->mainvnb, grid, txt, id);

  uiWidgetExpandHoriz (grid);
  uiWidgetExpandVert (grid);
  uiWidgetSetAllMargins (grid, 2);
}

void
confuiEntrySetSize (confuigui_t *gui, int widx, int sz, int maxsz)
{
  gui->uiitem [widx].entrysz = sz;
  gui->uiitem [widx].entrymaxsz = maxsz;
}

void
confuiMakeItemEntry (confuigui_t *gui, uiwcont_t *grid,
    const char *txt, int widx, int bdjoptIdx, const char *disp, int indent)
{
  confuiMakeItemEntryBasic (gui, grid, txt, widx, bdjoptIdx, disp, indent, CONFUI_NO_EXPAND);
}

void
confuiMakeItemEntryEncrypt (confuigui_t *gui, uiwcont_t *grid,
    const char *txt, int widx, int bdjoptIdx, const char *disp, int indent)
{
  confuiMakeItemEntry (gui, grid, txt, widx, bdjoptIdx, disp, indent);
  gui->uiitem [widx].basetype = CONFUI_ENTRY_ENCRYPT;
}

void
confuiMakeItemEntryChooser (confuigui_t *gui, uiwcont_t *grid,
    const char *txt, int widx, int bdjoptIdx,
    const char *disp, void *dialogFunc)
{
  uiwcont_t  *uiwidgetp;

//  uiWidgetExpandHoriz (hbox);
  confuiMakeItemEntryBasic (gui, grid, txt, widx, bdjoptIdx, disp, CONFUI_NO_INDENT, CONFUI_EXPAND);
  gui->uiitem [widx].sfcb.entry = gui->uiitem [widx].uiwidgetp;
  gui->uiitem [widx].sfcb.window = gui->window;
  gui->uiitem [widx].callback = callbackInit (dialogFunc, &gui->uiitem [widx].sfcb, NULL);

  uiwidgetp = uiCreateButton (
      gui->uiitem [widx].callback, NULL, NULL, NULL);
  uiButtonSetImageIcon (uiwidgetp, "folder");
  uiGridPack (grid, uiwidgetp, WCONT_KEEP);
  uiWidgetSetMarginStart (uiwidgetp, 0);
  gui->uiitem [widx].uibutton = uiwidgetp;
}

void
confuiMakeItemDropdown (confuigui_t *gui, uiwcont_t *grid,
    const char *txt, int widx, int bdjoptIdx, void *ddcb)
{
  int     type;

  gui->uiitem [widx].basetype = CONFUI_DD;
  /* the selection handler sets bdjopt to the appropriate value */
  /* and also listidx is not set */
  gui->uiitem [widx].outtype = CONFUI_OUT_NONE;

  confuiMakeItemLabel (gui, widx, grid, txt, CONFUI_NO_INDENT);

  type = DD_LIST_TYPE_STR;
  if (widx == CONFUI_DD_FADE_TYPE || widx == CONFUI_DD_CLOCK) {
    type = DD_LIST_TYPE_NUM;
  }
  if (type == DD_LIST_TYPE_STR) {
    gui->uiitem [widx].callback = callbackInitS (ddcb, gui);
  }
  if (type == DD_LIST_TYPE_NUM) {
    gui->uiitem [widx].callback = callbackInitI (ddcb, gui);
  }
  gui->uiitem [widx].uidd = uiddCreate ("confgui",
      gui->window, NULL, DD_PACK_START,
      gui->uiitem [widx].ddlist, type,
      NULL, DD_REPLACE_TITLE, gui->uiitem [widx].callback);
  uiddPackGrid (gui->uiitem [widx].uidd, grid);
  /* this is the only time the index is used */
  uiddSetSelection (gui->uiitem [widx].uidd, gui->uiitem [widx].listidx);

  uiddSetMarginStart (gui->uiitem [widx].uidd, 4);

  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
}

void
confuiMakeItemLink (confuigui_t *gui, uiwcont_t *grid,
    const char *txt, int widx, const char *disp)
{
  uiwcont_t  *uiwidgetp = NULL;

  confuiMakeItemLabel (gui, widx, grid, txt, CONFUI_NO_INDENT);
  uiwidgetp = uiCreateLink (disp, NULL);
  if (isMacOS ()) {
    gui->uiitem [widx].callback = callbackInit (
        confuiLinkCallback, gui, NULL);
    gui->uiitem [widx].uri = NULL;
    uiLinkSetActivateCallback (uiwidgetp, gui->uiitem [widx].callback);
  }
  uiGridPack (grid, uiwidgetp, WCONT_KEEP);
  gui->uiitem [widx].uiwidgetp = uiwidgetp;
}

void
confuiMakeItemFontButton (confuigui_t *gui, uiwcont_t *grid,
    const char *txt, int widx, int bdjoptIdx, const char *fontname)
{
  uiwcont_t  *uiwidgetp;

  gui->uiitem [widx].basetype = CONFUI_FONT;
  gui->uiitem [widx].outtype = CONFUI_OUT_STR;

  confuiMakeItemLabel (gui, widx, grid, txt, CONFUI_NO_INDENT);

  uiwidgetp = uiCreateFontButton (fontname);
  uiGridPack (grid, uiwidgetp, WCONT_KEEP);
  uiWidgetSetMarginStart (uiwidgetp, 4);

  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
}

void
confuiMakeItemColorButton (confuigui_t *gui, uiwcont_t *grid,
    const char *txt, int widx, int bdjoptIdx, const char *color)
{
  uiwcont_t  *uiwidgetp;


  gui->uiitem [widx].basetype = CONFUI_COLOR;
  gui->uiitem [widx].outtype = CONFUI_OUT_STR;

  confuiMakeItemLabel (gui, widx, grid, txt, CONFUI_NO_INDENT);

  uiwidgetp = uiCreateColorButton (color);
  uiGridPack (grid, uiwidgetp, WCONT_KEEP);
  uiWidgetSetMarginStart (uiwidgetp, 4);

  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
}

void
confuiMakeItemButton (confuigui_t *gui, uiwcont_t *grid,
    const char *txt, int widx, int bdjoptIdx, callbackFunc cb)
{
  uiwcont_t  *uiwidgetp;

  gui->uiitem [widx].basetype = CONFUI_NONE;
  gui->uiitem [widx].outtype = CONFUI_OUT_NONE;
  if (cb != NULL) {
    gui->uiitem [widx].callback = callbackInit (cb, gui, NULL);
  }

  confuiMakeItemLabel (gui, widx, grid, "", CONFUI_NO_INDENT);

  uiwidgetp = uiCreateButton (
      gui->uiitem [widx].callback, txt, NULL, NULL);
  uiGridPack (grid, uiwidgetp, WCONT_KEEP);
  uiWidgetSetMarginStart (uiwidgetp, 4);

  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
}

void
confuiMakeItemSpinboxText (confuigui_t *gui, uiwcont_t *grid,
    const char *txt, int widx, int bdjoptIdx,
    confuiouttype_t outtype, ssize_t value, void *cb)
{
  uisbtext_t  *sb;
  nlist_t     *list;


  gui->uiitem [widx].basetype = CONFUI_SB_TXT;
  gui->uiitem [widx].outtype = outtype;

  confuiMakeItemLabel (gui, widx, grid, txt, CONFUI_NO_INDENT);

  sb = uisbtextCreate (NULL, 4);
  uisbtextPackGrid (sb, grid);
  gui->uiitem [widx].sbtxt = sb;
  list = gui->uiitem [widx].displist;

  uisbtextSetList (sb, list);
  uisbtextSetValue (sb, value);

  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;

  if (cb != NULL) {
    gui->uiitem [widx].sbcallback = callbackInit (cb, gui, NULL);
    uisbtextSetChangeCallback (sb, gui->uiitem [widx].sbcallback);
  }
}

void
confuiMakeItemSpinboxTime (confuigui_t *gui, uiwcont_t *grid,
    const char *txt, int widx,
    int bdjoptIdx, ssize_t value, int indent)
{
  uisbnum_t   *sb;
  int         sbtype;
  double      maxlimit;

  gui->uiitem [widx].basetype = CONFUI_SB_TIME;
  gui->uiitem [widx].outtype = CONFUI_OUT_NUM;

  confuiMakeItemLabel (gui, widx, grid, txt, indent);

  gui->uiitem [widx].sbcallback = callbackInit (
      confuiValidateCallback, &gui->uiitem [widx], NULL);
  if (bdjoptIdx == OPT_Q_STOP_AT_TIME) {
    /* convert value to mm:ss for display */
    value /= 60;
  }

  sb = uisbnumCreate (NULL, gui->uiitem [widx].labeltxt, 4);
  uisbnumPackGrid (sb, grid);
  /* the default increments are set in uisbnumSetTime() */
  /* two hours */
  maxlimit = 3600 * 2.0;
  sbtype = SBNUM_TIME_MS;
  if (bdjoptIdx == OPT_Q_STOP_AT_TIME) {
    /* stop-at : 24 hours max */
    maxlimit = 3600 * 24.0;
    sbtype = SBNUM_TIME_HM;
  }
  uisbnumSetTime (sb, 0.0, maxlimit, sbtype);
  uisbnumSetChangeCallback (sb, gui->uiitem [widx].sbcallback);
  gui->uiitem [widx].sbnum = sb;
  uisbnumSetValue (sb, value);

  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
}

void
confuiMakeItemSpinboxNum (confuigui_t *gui, uiwcont_t *grid,
    const char *txt, int widx, int bdjoptIdx,
    int min, int max, int value, void *cb)
{
  uisbnum_t   *sb;


  gui->uiitem [widx].basetype = CONFUI_SB_NUM;
  gui->uiitem [widx].outtype = CONFUI_OUT_NUM;

  confuiMakeItemLabel (gui, widx, grid, txt, CONFUI_NO_INDENT);

  gui->uiitem [widx].sbcallback = callbackInit (
      confuiValidateCallback, &gui->uiitem [widx], NULL);
  sb = uisbnumCreate (NULL, gui->uiitem [widx].labeltxt, 4);
  uisbnumPackGrid (sb, grid);
  uisbnumSetLimits (sb, (double) min, (double) max, 0);
  uisbnumSetValue (sb, (double) value);
  uisbnumSetChangeCallback (sb, gui->uiitem [widx].sbcallback);
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  if (cb != NULL) {
    gui->uiitem [widx].callback = callbackInit (cb, gui, NULL);
    uisbnumSetChangeCallback (sb, gui->uiitem [widx].callback);
  }
  gui->uiitem [widx].sbnum = sb;

}

void
confuiMakeItemSpinboxDouble (confuigui_t *gui, uiwcont_t *grid,
    const char *txt, int widx, int bdjoptIdx,
    double min, double max, double value, int indent)
{
  uisbnum_t   *sb;

  gui->uiitem [widx].basetype = CONFUI_SB_DOUBLE;
  gui->uiitem [widx].outtype = CONFUI_OUT_DOUBLE;

  gui->uiitem [widx].sbcallback = callbackInit (
      confuiValidateCallback, &gui->uiitem [widx], NULL);
  confuiMakeItemLabel (gui, widx, grid, txt, indent);
  sb = uisbnumCreate (NULL, gui->uiitem [widx].labeltxt, 4);
  uisbnumPackGrid (sb, grid);
  uisbnumSetLimits (sb, min, max, 1);
  uisbnumSetIncrements (sb, 0.1, 5.0);
  uisbnumSetValue (sb, value);
  uisbnumSetChangeCallback (sb, gui->uiitem [widx].sbcallback);
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  gui->uiitem [widx].sbnum = sb;
}

void
confuiMakeItemSwitch (confuigui_t *gui, uiwcont_t *grid,
    const char *txt, int widx, int bdjoptIdx, int value, void *cb, int indent)
{
  uiwcont_t  *uiwidgetp;


  gui->uiitem [widx].basetype = CONFUI_SWITCH;
  gui->uiitem [widx].outtype = CONFUI_OUT_BOOL;

  confuiMakeItemLabel (gui, widx, grid, txt, indent);

  uiwidgetp = uiCreateSwitch (value);
  uiGridPack (grid, uiwidgetp, WCONT_KEEP);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  gui->uiitem [widx].uiwidgetp = uiwidgetp;

  if (cb != NULL) {
    gui->uiitem [widx].callback = callbackInitI (cb, gui);
    uiSwitchSetCallback (gui->uiitem [widx].uiwidgetp,
        gui->uiitem [widx].callback);
  }

}

void
confuiMakeItemLabelDisp (confuigui_t *gui, uiwcont_t *grid,
    const char *txt, int widx, int indent)
{
  uiwcont_t   *uiwidgetp;

  gui->uiitem [widx].basetype = CONFUI_NONE;
  gui->uiitem [widx].outtype = CONFUI_OUT_NONE;

  confuiMakeItemLabel (gui, widx, grid, txt, indent);
  uiwidgetp = uiCreateLabel ("");
  uiGridPack (grid, uiwidgetp, WCONT_KEEP);
  uiWidgetSetMarginStart (uiwidgetp, 4);

  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  gui->uiitem [widx].bdjoptIdx = -1;
}

void
confuiMakeItemCheckButton (confuigui_t *gui, uiwcont_t *grid,
    const char *txt, int widx, int bdjoptIdx, int value)
{
  uiwcont_t  *uiwidgetp;


  gui->uiitem [widx].basetype = CONFUI_CHECK_BUTTON;
  gui->uiitem [widx].outtype = CONFUI_OUT_BOOL;
  uiwidgetp = uiCreateCheckButton (txt, value);
  uiGridPackRow (grid, uiwidgetp, 1, WCONT_KEEP);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
}

void
confuiMakeItemLabel (confuigui_t *gui, int widx,
    uiwcont_t *grid, const char *txt, int indent)
{
  uiwcont_t   *uiwidgetp;
  char        ntxt [300];
  char        *p = ntxt;
  char        *end = ntxt + sizeof (ntxt);

  *ntxt = '\0';
  if (indent == CONFUI_INDENT) {
    *ntxt = '\0';
    p = stpecpy (p, end, "   ");
  }
  p = stpecpy (p, end, txt);

  if (*txt == '\0') {
    uiwidgetp = uiCreateLabel (ntxt);
  } else {
    uiwidgetp = uiCreateColonLabel (ntxt);
  }
  uiGridNewRow (grid);
  uiGridPack (grid, uiwidgetp, WCONT_KEEP);

  gui->uiitem [widx].uilabelp = uiwidgetp;
  /* the labeltxt may not necessarily be static (low/high bpm) */
  gui->uiitem [widx].labeltxt = mdstrdup (txt);
}

void
confuiSBTextInitDataNum (confuigui_t *gui, char *tag, int widx, ...)
{
  va_list     valist;
  nlistidx_t  key;
  char        *disp;
  nlist_t     *tlist;

  va_start (valist, widx);

  tlist = nlistAlloc (tag, LIST_ORDERED, NULL);
  while ((key = va_arg (valist, nlistidx_t)) != -1) {
    disp = va_arg (valist, char *);
    nlistSetStr (tlist, key, disp);
  }
  gui->uiitem [widx].displist = tlist;

  va_end (valist);
}

/* internal routines */

static void
confuiMakeItemEntryBasic (confuigui_t *gui, uiwcont_t *grid,
    const char *txt, int widx, int bdjoptIdx, const char *disp,
    int indent, int expand)
{
  uiwcont_t   *uiwidgetp;

  gui->uiitem [widx].basetype = CONFUI_ENTRY;
  gui->uiitem [widx].outtype = CONFUI_OUT_STR;
  confuiMakeItemLabel (gui, widx, grid, txt, indent);
  uiwidgetp = uiEntryInit (gui->uiitem [widx].entrysz, gui->uiitem [widx].entrymaxsz);
  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  uiGridPack (grid, uiwidgetp, WCONT_KEEP);
  if (expand == CONFUI_EXPAND) {
//    uiBoxPackStartExpandChildren (boxp, uiwidgetp, WCONT_KEEP);
    uiWidgetAlignHorizFill (uiwidgetp);
    uiWidgetExpandHoriz (uiwidgetp);
  }
  uiWidgetSetMarginStart (uiwidgetp, 4);

  if (disp != NULL) {
    uiEntrySetValue (gui->uiitem [widx].uiwidgetp, disp);
  } else {
    uiEntrySetValue (gui->uiitem [widx].uiwidgetp, "");
  }
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
}

static bool
confuiLinkCallback (void *udata)
{
  confuigui_t *gui = udata;
  char        *uri;
  char        tmp [200];
  int         widx = -1;

  if (gui->tablecurr == CONFUI_ID_MOBILE_MQ) {
    widx = CONFUI_WIDGET_MOBMQ_QR_CODE;
  }
  if (gui->tablecurr == CONFUI_ID_REM_CONTROL) {
    widx = CONFUI_WIDGET_RC_QR_CODE;
  }
  if (widx < 0) {
    return UICB_CONT;
  }

  uri = gui->uiitem [widx].uri;
  if (uri != NULL) {
    snprintf (tmp, sizeof (tmp), "open %s", uri);
    (void) ! system (tmp);
    return UICB_STOP;
  }
  return UICB_CONT;
}

static bool
confuiValidateCallback (void *udata)
{
  confuiitem_t  *uiitem = udata;
  confuigui_t   *gui = uiitem->gui;


  if (! uisbnumIsValid (uiitem->sbnum)) {
    confuiMarkNotValid (gui, uiitem->widx);
    return UICB_STOP;
  }
  confuiSetErrorMsg (gui, "");
  confuiMarkValid (gui, uiitem->widx);

  return UICB_CONT;
}

