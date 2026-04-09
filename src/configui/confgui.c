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
#include "uiutils.h"
#include "validate.h"

enum {
  CONFUI_EXPAND,
  CONFUI_NO_EXPAND,
};

static const char * INDENT_STR = "      ";


static void confuiMakeItemEntryBasic (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp, const char *txt, int widx, int bdjoptIdx, const char *disp, int indent, int expand);
static bool confuiLinkCallback (void *udata);
static bool confuiValidateCallback (void *udata);

void
confuiMakeNotebookTab (uiwcont_t *boxp, confuigui_t *gui, const char *txt, int id)
{
  logProcBegin ();
  uivnbAppendPage (gui->mainvnb, boxp, txt, id);

  uiWidgetExpandHoriz (boxp);
  uiWidgetExpandVert (boxp);
  uiWidgetSetAllMargins (boxp, 2);

  logProcEnd ("");
}

void
confuiEntrySetSize (confuigui_t *gui, int widx, int sz, int maxsz)
{
  gui->uiitem [widx].entrysz = sz;
  gui->uiitem [widx].entrymaxsz = maxsz;
}

void
confuiMakeItemEntry (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    const char *txt, int widx, int bdjoptIdx, const char *disp, int indent)
{
  uiwcont_t  *hbox;

  logProcBegin ();
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (boxp, hbox, WCONT_FREE);
  confuiMakeItemEntryBasic (gui, hbox, szgrp, txt, widx, bdjoptIdx, disp, indent, CONFUI_NO_EXPAND);
  uiBoxPostProcess (hbox);
  logProcEnd ("");
}

void
confuiMakeItemEntryEncrypt (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    const char *txt, int widx, int bdjoptIdx, const char *disp, int indent)
{
  logProcBegin ();
  confuiMakeItemEntry (gui, boxp, szgrp, txt, widx, bdjoptIdx, disp, indent);
  gui->uiitem [widx].basetype = CONFUI_ENTRY_ENCRYPT;
  logProcEnd ("");
}

void
confuiMakeItemEntryChooser (confuigui_t *gui, uiwcont_t *boxp,
    uiwcont_t *szgrp, const char *txt, int widx, int bdjoptIdx,
    const char *disp, void *dialogFunc)
{
  uiwcont_t  *hbox;
  uiwcont_t  *uiwidgetp;

  logProcBegin ();
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (boxp, hbox, WCONT_FREE);
  uiWidgetExpandHoriz (hbox);
  confuiMakeItemEntryBasic (gui, hbox, szgrp, txt, widx, bdjoptIdx, disp, CONFUI_NO_INDENT, CONFUI_EXPAND);
  gui->uiitem [widx].sfcb.entry = gui->uiitem [widx].uiwidgetp;
  gui->uiitem [widx].sfcb.window = gui->window;
  gui->uiitem [widx].callback = callbackInit (dialogFunc, &gui->uiitem [widx].sfcb, NULL);

  uiwidgetp = uiCreateButton (
      gui->uiitem [widx].callback, NULL, NULL, NULL);
  uiButtonSetImageIcon (uiwidgetp, "folder");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uiWidgetSetMarginStart (uiwidgetp, 0);
  gui->uiitem [widx].uibutton = uiwidgetp;

  uiBoxPostProcess (hbox);
  logProcEnd ("");
}

void
confuiMakeItemDropdown (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    const char *txt, int widx, int bdjoptIdx, callbackFuncS ddcb)
{
  uiwcont_t   *hbox;

  logProcBegin ();
  gui->uiitem [widx].basetype = CONFUI_DD;
  /* the selection handler sets bdjopt to the appropriate value */
  /* and also listidx is not set */
  gui->uiitem [widx].outtype = CONFUI_OUT_NONE;

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (boxp, hbox, WCONT_FREE);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, txt, CONFUI_NO_INDENT);

  gui->uiitem [widx].callback = callbackInitS (ddcb, gui);
  gui->uiitem [widx].uidd = uiddCreate ("confgui",
      gui->window, hbox, DD_PACK_START,
      gui->uiitem [widx].ddlist, DD_LIST_TYPE_STR,
      NULL, DD_REPLACE_TITLE, gui->uiitem [widx].callback);
  /* this is the only time the index is used */
  uiddSetSelection (gui->uiitem [widx].uidd, gui->uiitem [widx].listidx);

  uiddSetMarginStart (gui->uiitem [widx].uidd, 4);

  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  uiBoxPostProcess (hbox);
  logProcEnd ("");
}

void
confuiMakeItemLink (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    const char *txt, int widx, const char *disp)
{
  uiwcont_t  *hbox;
  uiwcont_t  *uiwidgetp = NULL;

  logProcBegin ();
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (boxp, hbox, WCONT_FREE);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, txt, CONFUI_NO_INDENT);
  uiwidgetp = uiCreateLink (disp, NULL);
  if (isMacOS ()) {
    gui->uiitem [widx].callback = callbackInit (
        confuiLinkCallback, gui, NULL);
    gui->uiitem [widx].uri = NULL;
    uiLinkSetActivateCallback (uiwidgetp, gui->uiitem [widx].callback);
  }
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  uiBoxPostProcess (hbox);
  logProcEnd ("");
}

void
confuiMakeItemFontButton (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    const char *txt, int widx, int bdjoptIdx, const char *fontname)
{
  uiwcont_t  *hbox;
  uiwcont_t  *uiwidgetp;

  logProcBegin ();
  gui->uiitem [widx].basetype = CONFUI_FONT;
  gui->uiitem [widx].outtype = CONFUI_OUT_STR;
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (boxp, hbox, WCONT_FREE);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, txt, CONFUI_NO_INDENT);

  uiwidgetp = uiCreateFontButton (fontname);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uiWidgetSetMarginStart (uiwidgetp, 4);

  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  uiBoxPostProcess (hbox);
  logProcEnd ("");
}

void
confuiMakeItemColorButton (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    const char *txt, int widx, int bdjoptIdx, const char *color)
{
  uiwcont_t  *hbox;
  uiwcont_t  *uiwidgetp;

  logProcBegin ();

  gui->uiitem [widx].basetype = CONFUI_COLOR;
  gui->uiitem [widx].outtype = CONFUI_OUT_STR;

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (boxp, hbox, WCONT_FREE);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, txt, CONFUI_NO_INDENT);

  uiwidgetp = uiCreateColorButton (color);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uiWidgetSetMarginStart (uiwidgetp, 4);

  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  uiBoxPostProcess (hbox);
  logProcEnd ("");
}

void
confuiMakeItemButton (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    const char *txt, int widx, int bdjoptIdx, callbackFunc cb)
{
  uiwcont_t  *hbox;
  uiwcont_t  *uiwidgetp;

  logProcBegin ();

  gui->uiitem [widx].basetype = CONFUI_NONE;
  gui->uiitem [widx].outtype = CONFUI_OUT_NONE;
  if (cb != NULL) {
    gui->uiitem [widx].callback = callbackInit (cb, gui, NULL);
  }

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (boxp, hbox, WCONT_FREE);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, "", CONFUI_NO_INDENT);

  uiwidgetp = uiCreateButton (
      gui->uiitem [widx].callback, txt, NULL, NULL);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uiWidgetSetMarginStart (uiwidgetp, 4);

  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  uiBoxPostProcess (hbox);
  logProcEnd ("");
}

void
confuiMakeItemSpinboxText (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    uiwcont_t *szgrpB, const char *txt, int widx, int bdjoptIdx,
    confuiouttype_t outtype, ssize_t value, void *cb)
{
  uiwcont_t   *hbox;
  uisbtext_t  *sb;
  nlist_t     *list;

  logProcBegin ();

  gui->uiitem [widx].basetype = CONFUI_SB_TXT;
  gui->uiitem [widx].outtype = outtype;
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (boxp, hbox, WCONT_FREE);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, txt, CONFUI_NO_INDENT);

  sb = uisbtextCreate (hbox, 4);
  gui->uiitem [widx].sbtxt = sb;
  list = gui->uiitem [widx].displist;

  uisbtextSetList (sb, list);
  uisbtextSetValue (sb, value);
  if (szgrpB != NULL) {
    uisbtextSizeGroupAdd (sb, szgrpB);
  }

  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;

  if (cb != NULL) {
    gui->uiitem [widx].sbcallback = callbackInit (cb, gui, NULL);
    uisbtextSetChangeCallback (sb, gui->uiitem [widx].sbcallback);
  }
  uiBoxPostProcess (hbox);
  logProcEnd ("");
}

void
confuiMakeItemSpinboxTime (confuigui_t *gui, uiwcont_t *boxp,
    uiwcont_t *szgrp, uiwcont_t *szgrpB, const char *txt, int widx,
    int bdjoptIdx, ssize_t value, int indent)
{
  uiwcont_t   *hbox;
  uisbnum_t   *sb;
  int         sbtype;
  double      maxlimit;

  logProcBegin ();

  gui->uiitem [widx].basetype = CONFUI_SB_TIME;
  gui->uiitem [widx].outtype = CONFUI_OUT_NUM;

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (boxp, hbox, WCONT_FREE);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, txt, indent);

  gui->uiitem [widx].sbcallback = callbackInit (
      confuiValidateCallback, &gui->uiitem [widx], NULL);
  if (bdjoptIdx == OPT_Q_STOP_AT_TIME) {
    /* convert value to mm:ss for display */
    value /= 60;
  }

  sb = uisbnumCreate (hbox, gui->uiitem [widx].labeltxt, 4);
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
  if (szgrpB != NULL) {
    uisbnumSizeGroupAdd (sb, szgrpB);
  }
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  uiBoxPostProcess (hbox);
  logProcEnd ("");
}

void
confuiMakeItemSpinboxNum (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    uiwcont_t *szgrpB, const char *txt, int widx, int bdjoptIdx,
    int min, int max, int value, void *cb)
{
  uiwcont_t   *hbox;
  uisbnum_t   *sb;

  logProcBegin ();

  gui->uiitem [widx].basetype = CONFUI_SB_NUM;
  gui->uiitem [widx].outtype = CONFUI_OUT_NUM;
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (boxp, hbox, WCONT_FREE);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, txt, CONFUI_NO_INDENT);

  gui->uiitem [widx].sbcallback = callbackInit (
      confuiValidateCallback, &gui->uiitem [widx], NULL);
  sb = uisbnumCreate (hbox, gui->uiitem [widx].labeltxt, 4);
  uisbnumSetLimits (sb, (double) min, (double) max, 0);
  uisbnumSetValue (sb, (double) value);
  uisbnumSetChangeCallback (sb, gui->uiitem [widx].sbcallback);
  if (szgrpB != NULL) {
    uisbnumSizeGroupAdd (sb, szgrpB);
  }
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  if (cb != NULL) {
    gui->uiitem [widx].callback = callbackInit (cb, gui, NULL);
    uisbnumSetChangeCallback (sb, gui->uiitem [widx].callback);
  }
  gui->uiitem [widx].sbnum = sb;

  uiBoxPostProcess (hbox);

  logProcEnd ("");
}

void
confuiMakeItemSpinboxDouble (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    uiwcont_t *szgrpB, const char *txt, int widx, int bdjoptIdx,
    double min, double max, double value, int indent)
{
  uiwcont_t  *hbox;
  uisbnum_t   *sb;

  logProcBegin ();

  gui->uiitem [widx].basetype = CONFUI_SB_DOUBLE;
  gui->uiitem [widx].outtype = CONFUI_OUT_DOUBLE;
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (boxp, hbox, WCONT_FREE);

  gui->uiitem [widx].sbcallback = callbackInit (
      confuiValidateCallback, &gui->uiitem [widx], NULL);
  confuiMakeItemLabel (gui, widx, hbox, szgrp, txt, indent);
  sb = uisbnumCreate (hbox, gui->uiitem [widx].labeltxt, 4);
  uisbnumSetLimits (sb, min, max, 1);
  uisbnumSetIncrements (sb, 0.1, 5.0);
  uisbnumSetValue (sb, value);
  uisbnumSetChangeCallback (sb, gui->uiitem [widx].sbcallback);
  if (szgrpB != NULL) {
    uisbnumSizeGroupAdd (sb, szgrpB);
  }
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  gui->uiitem [widx].sbnum = sb;
  uiBoxPostProcess (hbox);
  logProcEnd ("");
}

void
confuiMakeItemSwitch (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    const char *txt, int widx, int bdjoptIdx, int value, void *cb, int indent)
{
  uiwcont_t  *hbox;
  uiwcont_t  *uiwidgetp;

  logProcBegin ();

  gui->uiitem [widx].basetype = CONFUI_SWITCH;
  gui->uiitem [widx].outtype = CONFUI_OUT_BOOL;
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (boxp, hbox, WCONT_FREE);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, txt, indent);

  uiwidgetp = uiCreateSwitch (value);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  gui->uiitem [widx].uiwidgetp = uiwidgetp;

  if (cb != NULL) {
    gui->uiitem [widx].callback = callbackInitI (cb, gui);
    uiSwitchSetCallback (gui->uiitem [widx].uiwidgetp,
        gui->uiitem [widx].callback);
  }

  uiBoxPostProcess (hbox);
  logProcEnd ("");
}

void
confuiMakeItemLabelDisp (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    const char *txt, int widx, int indent)
{
  uiwcont_t   *hbox;
  uiwcont_t   *uiwidgetp;

  logProcBegin ();

  gui->uiitem [widx].basetype = CONFUI_NONE;
  gui->uiitem [widx].outtype = CONFUI_OUT_NONE;
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (boxp, hbox, WCONT_FREE);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, txt, indent);
  uiwidgetp = uiCreateLabel ("");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uiWidgetSetMarginStart (uiwidgetp, 4);

  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  gui->uiitem [widx].bdjoptIdx = -1;
  uiBoxPostProcess (hbox);
  logProcEnd ("");
}

void
confuiMakeItemCheckButton (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    const char *txt, int widx, int bdjoptIdx, int value)
{
  uiwcont_t  *uiwidgetp;

  logProcBegin ();

  gui->uiitem [widx].basetype = CONFUI_CHECK_BUTTON;
  gui->uiitem [widx].outtype = CONFUI_OUT_BOOL;
  uiwidgetp = uiCreateCheckButton (txt, value);
  uiBoxPackStart (boxp, uiwidgetp, WCONT_KEEP);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd ("");
}

void
confuiMakeItemLabel (confuigui_t *gui, int widx,
    uiwcont_t *boxp, uiwcont_t *szgrp, const char *txt, int indent)
{
  uiwcont_t   *uiwidgetp;
  char        ntxt [300];
  const char  *ttxt;

  logProcBegin ();
  *ntxt = '\0';
  ttxt = txt;
  if (indent == CONFUI_INDENT) {
    char    *p = ntxt;
    char    *end = ntxt + sizeof (ntxt);

    *ntxt = '\0';
    p = stpecpy (p, end, "   ");
    p = stpecpy (p, end, txt);
    ttxt = ntxt;
  }

  if (*txt == '\0') {
    uiwidgetp = uiCreateLabel (ttxt);
  } else {
    uiwidgetp = uiCreateColonLabel (ttxt);
  }
  uiBoxPackStart (boxp, uiwidgetp, WCONT_KEEP);
  if (szgrp != NULL) {
    uiSizeGroupAdd (szgrp, uiwidgetp);
  }
  gui->uiitem [widx].uilabelp = uiwidgetp;
  /* the labeltxt may not necessarily be static (low/high bpm) */
  gui->uiitem [widx].labeltxt = mdstrdup (txt);

  if (indent == CONFUI_INDENT || indent == CONFUI_INDENT_FIELD) {
    *ntxt = '\0';
    stpecpy (ntxt, ntxt + sizeof (ntxt), INDENT_STR);
    uiwidgetp = uiCreateLabel (ntxt);
    uiBoxPackStart (boxp, uiwidgetp, WCONT_FREE);
  }
  logProcEnd ("");
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
confuiMakeItemEntryBasic (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    const char *txt, int widx, int bdjoptIdx, const char *disp,
    int indent, int expand)
{
  uiwcont_t   *uiwidgetp;

  gui->uiitem [widx].basetype = CONFUI_ENTRY;
  gui->uiitem [widx].outtype = CONFUI_OUT_STR;
  confuiMakeItemLabel (gui, widx, boxp, szgrp, txt, indent);
  uiwidgetp = uiEntryInit (gui->uiitem [widx].entrysz, gui->uiitem [widx].entrymaxsz);
  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  if (expand == CONFUI_EXPAND) {
    uiBoxPackStartExpandChildren (boxp, uiwidgetp, WCONT_KEEP);
    uiWidgetAlignHorizFill (uiwidgetp);
    uiWidgetExpandHoriz (uiwidgetp);
  }
  if (expand == CONFUI_NO_EXPAND) {
    uiBoxPackStart (boxp, uiwidgetp, WCONT_KEEP);
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

  logProcBegin ();

  if (! uisbnumIsValid (uiitem->sbnum)) {
    confuiMarkNotValid (gui, uiitem->widx);
    return UICB_STOP;
  }
  confuiMarkValid (gui, uiitem->widx);

  logProcEnd ("");
  return UICB_CONT;
}

