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
#include "sysvars.h"
#include "tmutil.h"
#include "ui.h"
#include "uidd.h"
#include "uiduallist.h"
#include "uinbutil.h"
#include "uiutils.h"
#include "validate.h"

enum {
  CONFUI_EXPAND,
  CONFUI_NO_EXPAND,
};

static const char * INDENT_STR = "      ";


static void confuiMakeItemEntryBasic (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp, const char *txt, int widx, int bdjoptIdx, const char *disp, int indent, int expand);
static bool confuiLinkCallback (void *udata);
static int32_t confuiValHMCallback (void *udata, const char *label, const char *txt);
static int32_t confuiValHMSCallback (void *udata, const char *label, const char *txt);

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
  hbox = uiCreateHorizBox (NULL);
  uiBoxPackStart (boxp, hbox);
  confuiMakeItemEntryBasic (gui, hbox, szgrp, txt, widx, bdjoptIdx, disp, indent, CONFUI_NO_EXPAND);
  uiwcontFree (hbox);
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
  hbox = uiCreateHorizBox (NULL);
  uiBoxPackStart (boxp, hbox);
  uiWidgetExpandHoriz (hbox);
  confuiMakeItemEntryBasic (gui, hbox, szgrp, txt, widx, bdjoptIdx, disp, CONFUI_NO_INDENT, CONFUI_EXPAND);
  gui->uiitem [widx].sfcb.entry = gui->uiitem [widx].uiwidgetp;
  gui->uiitem [widx].sfcb.window = gui->window;
  gui->uiitem [widx].callback = callbackInit (dialogFunc, &gui->uiitem [widx].sfcb, NULL);

  uiwidgetp = uiCreateButton ("b-conf-folder",
      gui->uiitem [widx].callback, "", NULL);
  uiButtonSetImageIcon (uiwidgetp, "folder");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 0);
  gui->uiitem [widx].uibutton = uiwidgetp;

  uiwcontFree (hbox);
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

  hbox = uiCreateHorizBox (NULL);
  uiBoxPackStart (boxp, hbox);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, txt, CONFUI_NO_INDENT);

  gui->uiitem [widx].callback = callbackInitS (ddcb, gui);
  gui->uiitem [widx].uidd = uiddCreate ("confgui",
      gui->window, hbox, DD_PACK_START,
      gui->uiitem [widx].ddlist, DD_LIST_TYPE_STR,
      txt, DD_REPLACE_TITLE, gui->uiitem [widx].callback);
  /* this is the only time the index is used */
  uiddSetSelection (gui->uiitem [widx].uidd, gui->uiitem [widx].listidx);

  uiddSetMarginStart (gui->uiitem [widx].uidd, 4);

  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  uiwcontFree (hbox);
  logProcEnd ("");
}

void
confuiMakeItemLink (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    const char *txt, int widx, const char *disp)
{
  uiwcont_t  *hbox;
  uiwcont_t  *uiwidgetp = NULL;

  logProcBegin ();
  hbox = uiCreateHorizBox (NULL);
  uiBoxPackStart (boxp, hbox);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, txt, CONFUI_NO_INDENT);
  uiwidgetp = uiCreateLink (disp, NULL);
  if (isMacOS ()) {
    gui->uiitem [widx].callback = callbackInit (
        confuiLinkCallback, gui, NULL);
    gui->uiitem [widx].uri = NULL;
    uiLinkSetActivateCallback (uiwidgetp, gui->uiitem [widx].callback);
  }
  uiBoxPackStart (hbox, uiwidgetp);
  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  uiwcontFree (hbox);
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
  hbox = uiCreateHorizBox (NULL);
  uiBoxPackStart (boxp, hbox);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, txt, CONFUI_NO_INDENT);

  uiwidgetp = uiCreateFontButton (fontname);
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 4);

  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  uiwcontFree (hbox);
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

  hbox = uiCreateHorizBox (NULL);
  uiBoxPackStart (boxp, hbox);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, txt, CONFUI_NO_INDENT);

  uiwidgetp = uiCreateColorButton (color);
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 4);

  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  uiwcontFree (hbox);
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

  hbox = uiCreateHorizBox (NULL);
  uiBoxPackStart (boxp, hbox);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, "", CONFUI_NO_INDENT);

  uiwidgetp = uiCreateButton ("b-conf-as-chk-conn",
      gui->uiitem [widx].callback, txt, NULL);
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 4);

  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  uiwcontFree (hbox);
  logProcEnd ("");
}

void
confuiMakeItemSpinboxText (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    uiwcont_t *szgrpB, const char *txt, int widx, int bdjoptIdx,
    confuiouttype_t outtype, ssize_t value, void *cb)
{
  uiwcont_t  *hbox;
  uiwcont_t  *uiwidgetp;
  nlist_t     *list;
  nlist_t     *keylist;
  size_t      maxWidth;

  logProcBegin ();

  gui->uiitem [widx].basetype = CONFUI_SPINBOX_TEXT;
  gui->uiitem [widx].outtype = outtype;
  hbox = uiCreateHorizBox (NULL);
  uiBoxPackStart (boxp, hbox);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, txt, CONFUI_NO_INDENT);

  uiwidgetp = uiSpinboxTextCreate (gui);
  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  list = gui->uiitem [widx].displist;
  keylist = gui->uiitem [widx].sbkeylist;
  if (outtype == CONFUI_OUT_STR) {
    keylist = NULL;
  }
  maxWidth = 0;

  if (list != NULL) {
    nlistCalcMaxValueWidth (list);
    maxWidth = nlistGetMaxValueWidth (list);
  }

  uiSpinboxTextSet (uiwidgetp, 0,
      nlistGetCount (list), maxWidth, list, keylist, NULL);
  uiSpinboxTextSetValue (uiwidgetp, value);
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  if (szgrpB != NULL) {
    uiSizeGroupAdd (szgrpB, uiwidgetp);
  }

  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;

  if (cb != NULL) {
    gui->uiitem [widx].callback = callbackInit (cb, gui, NULL);
    uiSpinboxTextSetValueChangedCallback (gui->uiitem [widx].uiwidgetp,
        gui->uiitem [widx].callback);
  }
  uiwcontFree (hbox);
  logProcEnd ("");
}

void
confuiMakeItemSpinboxTime (confuigui_t *gui, uiwcont_t *boxp,
    uiwcont_t *szgrp, uiwcont_t *szgrpB, const char *txt, int widx,
    int bdjoptIdx, ssize_t value, int indent)
{
  uiwcont_t  *hbox;
  uiwcont_t  *uiwidgetp;

  logProcBegin ();

  gui->uiitem [widx].basetype = CONFUI_SPINBOX_TIME;
  gui->uiitem [widx].outtype = CONFUI_OUT_NUM;
  hbox = uiCreateHorizBox (NULL);
  uiBoxPackStart (boxp, hbox);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, txt, indent);

  if (bdjoptIdx == OPT_Q_STOP_AT_TIME) {
    gui->uiitem [widx].callback = callbackInitSS (
        confuiValHMCallback, &gui->uiitem [widx]);
    /* convert value to mm:ss for display */
    value /= 60;
  } else if (bdjoptIdx == OPT_Q_MAXPLAYTIME) {
    gui->uiitem [widx].callback = callbackInitSS (
        confuiValHMSCallback, &gui->uiitem [widx]);
  }
  uiwidgetp = uiSpinboxTimeCreate (SB_TIME_BASIC, gui,
      txt, gui->uiitem [widx].callback);
  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  if (bdjoptIdx == OPT_Q_STOP_AT_TIME) {
    uiSpinboxSetRange (uiwidgetp, 0.0, 1440000.0);
  }
  uiSpinboxTimeSetValue (uiwidgetp, value);
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  if (szgrpB != NULL) {
    uiSizeGroupAdd (szgrpB, uiwidgetp);
  }
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  uiwcontFree (hbox);
  logProcEnd ("");
}

void
confuiMakeItemSpinboxNum (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    uiwcont_t *szgrpB, const char *txt, int widx, int bdjoptIdx,
    int min, int max, int value, void *cb)
{
  uiwcont_t  *hbox;
  uiwcont_t  *uiwidgetp;

  logProcBegin ();

  gui->uiitem [widx].basetype = CONFUI_SPINBOX_NUM;
  gui->uiitem [widx].outtype = CONFUI_OUT_NUM;
  hbox = uiCreateHorizBox (NULL);
  uiBoxPackStart (boxp, hbox);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, txt, CONFUI_NO_INDENT);
  uiwidgetp = uiSpinboxIntCreate ();
  uiSpinboxSet (uiwidgetp, (double) min, (double) max);
  uiSpinboxSetValue (uiwidgetp, (double) value);
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  if (szgrpB != NULL) {
    uiSizeGroupAdd (szgrpB, uiwidgetp);
  }
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  if (cb != NULL) {
    gui->uiitem [widx].callback = callbackInit (cb, gui, NULL);
    uiSpinboxSetValueChangedCallback (uiwidgetp, gui->uiitem [widx].callback);
  }
  gui->uiitem [widx].uiwidgetp = uiwidgetp;

  uiwcontFree (hbox);

  logProcEnd ("");
}

void
confuiMakeItemSpinboxDouble (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    uiwcont_t *szgrpB, const char *txt, int widx, int bdjoptIdx,
    double min, double max, double value, int indent)
{
  uiwcont_t  *hbox;
  uiwcont_t  *uiwidgetp;

  logProcBegin ();

  gui->uiitem [widx].basetype = CONFUI_SPINBOX_DOUBLE;
  gui->uiitem [widx].outtype = CONFUI_OUT_DOUBLE;
  hbox = uiCreateHorizBox (NULL);
  uiBoxPackStart (boxp, hbox);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, txt, indent);
  uiwidgetp = uiSpinboxDoubleCreate ();
  uiSpinboxSet (uiwidgetp, min, max);
  uiSpinboxSetValue (uiwidgetp, value);
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  if (szgrpB != NULL) {
    uiSizeGroupAdd (szgrpB, uiwidgetp);
  }
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  uiwcontFree (hbox);
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
  hbox = uiCreateHorizBox (NULL);
  uiBoxPackStart (boxp, hbox);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, txt, indent);

  uiwidgetp = uiCreateSwitch (value);
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  gui->uiitem [widx].uiwidgetp = uiwidgetp;

  if (cb != NULL) {
    gui->uiitem [widx].callback = callbackInitI (cb, gui);
    uiSwitchSetCallback (gui->uiitem [widx].uiwidgetp,
        gui->uiitem [widx].callback);
  }

  uiwcontFree (hbox);
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
  hbox = uiCreateHorizBox (NULL);
  uiBoxPackStart (boxp, hbox);

  confuiMakeItemLabel (gui, widx, hbox, szgrp, txt, indent);
  uiwidgetp = uiCreateLabel ("");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 4);

  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  gui->uiitem [widx].bdjoptIdx = -1;
  uiwcontFree (hbox);
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
  uiBoxPackStart (boxp, uiwidgetp);
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
  uiBoxPackStart (boxp, uiwidgetp);
  if (szgrp != NULL) {
    uiSizeGroupAdd (szgrp, uiwidgetp);
  }
  gui->uiitem [widx].uilabelp = uiwidgetp;
  gui->uiitem [widx].labeltxt = txt;

  if (indent == CONFUI_INDENT || indent == CONFUI_INDENT_FIELD) {
    *ntxt = '\0';
    stpecpy (ntxt, ntxt + sizeof (ntxt), INDENT_STR);
    uiwidgetp = uiCreateLabel (ntxt);
    uiBoxPackStart (boxp, uiwidgetp);
    uiwcontFree (uiwidgetp);
  }
  logProcEnd ("");
}

void
confuiSpinboxTextInitDataNum (confuigui_t *gui, char *tag, int widx, ...)
{
  va_list     valist;
  nlistidx_t  key;
  char        *disp;
  int         sbidx;
  nlist_t     *tlist;
  nlist_t     *llist;

  va_start (valist, widx);

  tlist = nlistAlloc (tag, LIST_ORDERED, NULL);
  llist = nlistAlloc (tag, LIST_ORDERED, NULL);
  sbidx = 0;
  while ((key = va_arg (valist, nlistidx_t)) != -1) {
    disp = va_arg (valist, char *);

    nlistSetStr (tlist, sbidx, disp);
    nlistSetNum (llist, sbidx, key);
    ++sbidx;
  }
  gui->uiitem [widx].displist = tlist;
  gui->uiitem [widx].sbkeylist = llist;

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
    uiBoxPackStartExpand (boxp, uiwidgetp);
    uiWidgetAlignHorizFill (uiwidgetp);
    uiWidgetExpandHoriz (uiwidgetp);
  }
  if (expand == CONFUI_NO_EXPAND) {
    uiBoxPackStart (boxp, uiwidgetp);
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

static int32_t
confuiValHMCallback (void *udata, const char *label, const char *txt)
{
  confuiitem_t  *uiitem = udata;
  confuigui_t   *gui = uiitem->gui;
  char          tbuff [200];
  int32_t       value;
  bool          val;

  logProcBegin ();

  val = validate (tbuff, sizeof (tbuff), label, txt, VAL_HOUR_MIN);
  if (val == false) {
    int32_t oval;

    oval = uiSpinboxTimeGetValue (uiitem->uiwidgetp);
    confuiSetErrorMsg (gui, tbuff);
    confuiMarkNotValid (gui, uiitem->widx);
    return oval;
  }
  confuiMarkValid (gui, uiitem->widx);

  value = tmutilStrToHM (txt);
  logProcEnd ("");
  return value;
}

static int32_t
confuiValHMSCallback (void *udata, const char *label, const char *txt)
{
  confuiitem_t  *uiitem = udata;
  confuigui_t   *gui = uiitem->gui;
  char          tbuff [200];
  int32_t       value;
  bool          val;

  logProcBegin ();

  val = validate (tbuff, sizeof (tbuff), label, txt, VAL_HMS);
  if (val == false) {
    int32_t oval;

    oval = uiSpinboxTimeGetValue (uiitem->uiwidgetp);
    confuiSetErrorMsg (gui, tbuff);
    confuiMarkNotValid (gui, uiitem->widx);
    return oval;
  }
  confuiSetErrorMsg (gui, "");
  confuiMarkValid (gui, uiitem->widx);

  value = tmutilStrToMS (txt);
  logProcEnd ("");
  return value;
}

