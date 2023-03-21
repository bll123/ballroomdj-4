/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>
#include <stdarg.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "configui.h"
#include "log.h"
#include "sysvars.h"
#include "tmutil.h"
#include "ui.h"
#include "callback.h"
#include "uiduallist.h"
#include "uinbutil.h"
#include "validate.h"

enum {
  CONFUI_EXPAND,
  CONFUI_NO_EXPAND,
};

static void confuiMakeItemEntryBasic (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp, const char *txt, int widx, int bdjoptIdx, const char *disp, int indent, int expand);
static bool confuiLinkCallback (void *udata);
static long confuiValMSCallback (void *udata, const char *txt);
static long confuiValHMCallback (void *udata, const char *txt);

void
confuiMakeNotebookTab (uiwcont_t *boxp, confuigui_t *gui, const char *txt, int id)
{
  uiwcont_t  uiwidget;

  logProcBegin (LOG_PROC, "confuiMakeNotebookTab");
  uiCreateLabel (&uiwidget, txt);
  uiWidgetSetAllMargins (&uiwidget, 0);
  uiWidgetExpandHoriz (boxp);
  uiWidgetExpandVert (boxp);
  uiWidgetSetAllMargins (boxp, 2);
  uiNotebookAppendPage (gui->notebook, boxp, &uiwidget);
  uinbutilIDAdd (gui->nbtabid, id);

  logProcEnd (LOG_PROC, "confuiMakeNotebookTab", "");
}

void
confuiMakeItemEntry (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    const char *txt, int widx, int bdjoptIdx, const char *disp, int indent)
{
  uiwcont_t  hbox;

  logProcBegin (LOG_PROC, "confuiMakeItemEntry");
  uiCreateHorizBox (&hbox);
  confuiMakeItemEntryBasic (gui, &hbox, szgrp, txt, widx, bdjoptIdx, disp, indent, CONFUI_NO_EXPAND);
  uiBoxPackStart (boxp, &hbox);
  logProcEnd (LOG_PROC, "confuiMakeItemEntry", "");
}

void
confuiMakeItemEntryChooser (confuigui_t *gui, uiwcont_t *boxp,
    uiwcont_t *szgrp, const char *txt, int widx, int bdjoptIdx,
    const char *disp, void *dialogFunc)
{
  uiwcont_t  hbox;
  uiwcont_t  *uiwidgetp;
  uibutton_t  *uibutton;

  logProcBegin (LOG_PROC, "confuiMakeItemEntryChooser");
  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  confuiMakeItemEntryBasic (gui, &hbox, szgrp, txt, widx, bdjoptIdx, disp, CONFUI_NO_INDENT, CONFUI_EXPAND);
  gui->uiitem [widx].callback = callbackInit (dialogFunc, gui, NULL);
  uibutton = uiCreateButton (gui->uiitem [widx].callback, "", NULL);
  gui->uiitem [widx].uibutton = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiButtonSetImageIcon (uibutton, "folder");
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiBoxPackStart (&hbox, uiwidgetp);
  uiBoxPackStart (boxp, &hbox);
  logProcEnd (LOG_PROC, "confuiMakeItemEntryChooser", "");
}

void
confuiMakeItemCombobox (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    const char *txt, int widx, int bdjoptIdx, callbackFuncLong ddcb,
    char *value)
{
  uiwcont_t  hbox;
  uiwcont_t  uiwidget;
  uiwcont_t  *uiwidgetp;

  logProcBegin (LOG_PROC, "confuiMakeItemCombobox");
  gui->uiitem [widx].basetype = CONFUI_COMBOBOX;
  gui->uiitem [widx].outtype = CONFUI_OUT_STR;

  uiCreateHorizBox (&hbox);
  confuiMakeItemLabel (&hbox, szgrp, txt, CONFUI_NO_INDENT);

  gui->uiitem [widx].callback = callbackInitLong (ddcb, gui);
  uiwidgetp = uiComboboxCreate (gui->uiitem [widx].dropdown,
      &gui->window, txt, gui->uiitem [widx].callback, gui);

  uiDropDownSetList (gui->uiitem [widx].dropdown,
      gui->uiitem [widx].displist, NULL);
  uiDropDownSelectionSetStr (gui->uiitem [widx].dropdown, value);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  uiBoxPackStart (&hbox, uiwidgetp);
  uiBoxPackStart (boxp, &hbox);

  uiwcontCopy (&gui->uiitem [widx].uiwidget, &uiwidget);
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemCombobox", "");
}

void
confuiMakeItemLink (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    const char *txt, int widx, const char *disp)
{
  uiwcont_t  hbox;
  uiwcont_t  *uiwidgetp = NULL;

  logProcBegin (LOG_PROC, "confuiMakeItemLink");
  uiCreateHorizBox (&hbox);
  confuiMakeItemLabel (&hbox, szgrp, txt, CONFUI_NO_INDENT);
  uiwidgetp = uiCreateLink (disp, NULL);
  if (isMacOS ()) {
    gui->uiitem [widx].callback = callbackInit (
        confuiLinkCallback, gui, NULL);
    gui->uiitem [widx].uri = NULL;
    uiLinkSetActivateCallback (uiwidgetp, gui->uiitem [widx].callback);
  }
  uiBoxPackStart (&hbox, uiwidgetp);
  uiBoxPackStart (boxp, &hbox);
  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  logProcEnd (LOG_PROC, "confuiMakeItemLink", "");
}

void
confuiMakeItemFontButton (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    const char *txt, int widx, int bdjoptIdx, const char *fontname)
{
  uiwcont_t  hbox;
  uiwcont_t  *uiwidgetp;

  logProcBegin (LOG_PROC, "confuiMakeItemFontButton");
  gui->uiitem [widx].basetype = CONFUI_FONT;
  gui->uiitem [widx].outtype = CONFUI_OUT_STR;
  uiCreateHorizBox (&hbox);
  confuiMakeItemLabel (&hbox, szgrp, txt, CONFUI_NO_INDENT);

  uiwidgetp = uiCreateFontButton (fontname);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  uiBoxPackStart (&hbox, uiwidgetp);

  uiBoxPackStart (boxp, &hbox);
  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemFontButton", "");
}

void
confuiMakeItemColorButton (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    const char *txt, int widx, int bdjoptIdx, const char *color)
{
  uiwcont_t  hbox;
  uiwcont_t  *uiwidgetp;

  logProcBegin (LOG_PROC, "confuiMakeItemColorButton");

  gui->uiitem [widx].basetype = CONFUI_COLOR;
  gui->uiitem [widx].outtype = CONFUI_OUT_STR;

  uiCreateHorizBox (&hbox);
  confuiMakeItemLabel (&hbox, szgrp, txt, CONFUI_NO_INDENT);

  uiwidgetp = uiCreateColorButton (color);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  uiBoxPackStart (&hbox, uiwidgetp);

  uiBoxPackStart (boxp, &hbox);
  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemColorButton", "");
}

void
confuiMakeItemSpinboxText (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    uiwcont_t *szgrpB, const char *txt, int widx, int bdjoptIdx,
    confuiouttype_t outtype, ssize_t value, void *cb)
{
  uiwcont_t  hbox;
  uiwcont_t  *uiwidgetp;
  nlist_t     *list;
  nlist_t     *keylist;
  size_t      maxWidth;

  logProcBegin (LOG_PROC, "confuiMakeItemSpinboxText");

  gui->uiitem [widx].basetype = CONFUI_SPINBOX_TEXT;
  gui->uiitem [widx].outtype = outtype;
  uiCreateHorizBox (&hbox);
  confuiMakeItemLabel (&hbox, szgrp, txt, CONFUI_NO_INDENT);
  uiSpinboxTextCreate (gui->uiitem [widx].spinbox, gui);
  list = gui->uiitem [widx].displist;
  keylist = gui->uiitem [widx].sbkeylist;
  if (outtype == CONFUI_OUT_STR) {
    keylist = NULL;
  }
  maxWidth = 0;
  if (list != NULL) {
    nlistidx_t    iteridx;
    nlistidx_t    key;
    char          *val;

    nlistStartIterator (list, &iteridx);
    while ((key = nlistIterateKey (list, &iteridx)) >= 0) {
      size_t      len;

      val = nlistGetStr (list, key);
      len = strlen (val);
      maxWidth = len > maxWidth ? len : maxWidth;
    }
  }

  uiSpinboxTextSet (gui->uiitem [widx].spinbox, 0,
      nlistGetCount (list), maxWidth, list, keylist, NULL);
  uiSpinboxTextSetValue (gui->uiitem [widx].spinbox, value);
  uiwidgetp = uiSpinboxGetWidgetContainer (gui->uiitem [widx].spinbox);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  if (szgrpB != NULL) {
    uiSizeGroupAdd (szgrpB, uiwidgetp);
  }
  uiBoxPackStart (&hbox, uiwidgetp);
  uiBoxPackStart (boxp, &hbox);

  uiwcontCopy (&gui->uiitem [widx].uiwidget, uiwidgetp);
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;

  if (cb != NULL) {
    gui->uiitem [widx].callback = callbackInit (cb, gui, NULL);
    uiSpinboxTextSetValueChangedCallback (gui->uiitem [widx].spinbox,
        gui->uiitem [widx].callback);
  }
  logProcEnd (LOG_PROC, "confuiMakeItemSpinboxText", "");
}

void
confuiMakeItemSpinboxTime (confuigui_t *gui, uiwcont_t *boxp,
    uiwcont_t *szgrp, uiwcont_t *szgrpB, const char *txt, int widx,
    int bdjoptIdx, ssize_t value, int indent)
{
  uiwcont_t  hbox;
  uiwcont_t  *uiwidgetp;

  logProcBegin (LOG_PROC, "confuiMakeItemSpinboxTime");

  gui->uiitem [widx].basetype = CONFUI_SPINBOX_TIME;
  gui->uiitem [widx].outtype = CONFUI_OUT_NUM;
  uiCreateHorizBox (&hbox);
  confuiMakeItemLabel (&hbox, szgrp, txt, indent);

  if (bdjoptIdx == OPT_Q_STOP_AT_TIME) {
    gui->uiitem [widx].callback = callbackInitStr (
        confuiValHMCallback, gui);
    /* convert value to mm:ss */
    value /= 60;
  } else {
    gui->uiitem [widx].callback = callbackInitStr (
        confuiValMSCallback, gui);
  }
  uiSpinboxTimeCreate (gui->uiitem [widx].spinbox, gui,
      gui->uiitem [widx].callback);
  if (bdjoptIdx == OPT_Q_STOP_AT_TIME) {
    uiSpinboxSetRange (gui->uiitem [widx].spinbox, 0.0, 1440000.0);
  }
  uiSpinboxTimeSetValue (gui->uiitem [widx].spinbox, value);
  uiwidgetp = uiSpinboxGetWidgetContainer (gui->uiitem [widx].spinbox);
  uiwcontCopy (&gui->uiitem [widx].uiwidget, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  if (szgrpB != NULL) {
    uiSizeGroupAdd (szgrpB, uiwidgetp);
  }
  uiBoxPackStart (&hbox, uiwidgetp);
  uiBoxPackStart (boxp, &hbox);
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemSpinboxTime", "");
}

void
confuiMakeItemSpinboxNum (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    uiwcont_t *szgrpB, const char *txt, int widx, int bdjoptIdx,
    int min, int max, int value, void *cb)
{
  uiwcont_t  hbox;
  uiwcont_t  uiwidget;

  logProcBegin (LOG_PROC, "confuiMakeItemSpinboxNum");

  gui->uiitem [widx].basetype = CONFUI_SPINBOX_NUM;
  gui->uiitem [widx].outtype = CONFUI_OUT_NUM;
  uiCreateHorizBox (&hbox);
  confuiMakeItemLabel (&hbox, szgrp, txt, CONFUI_NO_INDENT);
  uiSpinboxIntCreate (&uiwidget);
  uiSpinboxSet (&uiwidget, (double) min, (double) max);
  uiSpinboxSetValue (&uiwidget, (double) value);
  uiWidgetSetMarginStart (&uiwidget, 4);
  if (szgrpB != NULL) {
    uiSizeGroupAdd (szgrpB, &uiwidget);
  }
  uiBoxPackStart (&hbox, &uiwidget);
  uiBoxPackStart (boxp, &hbox);
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  if (cb != NULL) {
    gui->uiitem [widx].callback = callbackInit (cb, gui, NULL);
    uiSpinboxSetValueChangedCallback (&uiwidget, gui->uiitem [widx].callback);
  }
  uiwcontCopy (&gui->uiitem [widx].uiwidget, &uiwidget);
  logProcEnd (LOG_PROC, "confuiMakeItemSpinboxNum", "");
}

void
confuiMakeItemSpinboxDouble (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    uiwcont_t *szgrpB, const char *txt, int widx, int bdjoptIdx,
    double min, double max, double value, int indent)
{
  uiwcont_t  hbox;
  uiwcont_t  uiwidget;

  logProcBegin (LOG_PROC, "confuiMakeItemSpinboxDouble");

  gui->uiitem [widx].basetype = CONFUI_SPINBOX_DOUBLE;
  gui->uiitem [widx].outtype = CONFUI_OUT_DOUBLE;
  uiCreateHorizBox (&hbox);
  confuiMakeItemLabel (&hbox, szgrp, txt, indent);
  uiSpinboxDoubleCreate (&uiwidget);
  uiSpinboxSet (&uiwidget, min, max);
  uiSpinboxSetValue (&uiwidget, value);
  uiWidgetSetMarginStart (&uiwidget, 4);
  if (szgrpB != NULL) {
    uiSizeGroupAdd (szgrpB, &uiwidget);
  }
  uiBoxPackStart (&hbox, &uiwidget);
  uiBoxPackStart (boxp, &hbox);
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  uiwcontCopy (&gui->uiitem [widx].uiwidget, &uiwidget);
  logProcEnd (LOG_PROC, "confuiMakeItemSpinboxDouble", "");
}

void
confuiMakeItemSwitch (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    const char *txt, int widx, int bdjoptIdx, int value, void *cb, int indent)
{
  uiwcont_t  hbox;
  uiwcont_t  *uiwidgetp;

  logProcBegin (LOG_PROC, "confuiMakeItemSwitch");

  gui->uiitem [widx].basetype = CONFUI_SWITCH;
  gui->uiitem [widx].outtype = CONFUI_OUT_BOOL;
  uiCreateHorizBox (&hbox);
  confuiMakeItemLabel (&hbox, szgrp, txt, indent);

  gui->uiitem [widx].uiswitch = uiCreateSwitch (value);
  uiwidgetp = uiSwitchGetWidgetContainer (gui->uiitem [widx].uiswitch);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  uiBoxPackStart (&hbox, uiwidgetp);
  uiBoxPackStart (boxp, &hbox);
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;

  if (cb != NULL) {
    gui->uiitem [widx].callback = callbackInit (cb, gui, NULL);
    uiSwitchSetCallback (gui->uiitem [widx].uiswitch,
        gui->uiitem [widx].callback);
  }

  logProcEnd (LOG_PROC, "confuiMakeItemSwitch", "");
}

void
confuiMakeItemLabelDisp (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    const char *txt, int widx, int bdjoptIdx)
{
  uiwcont_t  hbox;
  uiwcont_t  uiwidget;

  logProcBegin (LOG_PROC, "confuiMakeItemLabelDisp");

  gui->uiitem [widx].basetype = CONFUI_NONE;
  gui->uiitem [widx].outtype = CONFUI_OUT_NONE;
  uiCreateHorizBox (&hbox);
  confuiMakeItemLabel (&hbox, szgrp, txt, CONFUI_NO_INDENT);
  uiCreateLabel (&uiwidget, "");
  uiWidgetSetMarginStart (&uiwidget, 4);
  uiBoxPackStart (&hbox, &uiwidget);
  uiBoxPackStart (boxp, &hbox);
  uiwcontCopy (&gui->uiitem [widx].uiwidget, &uiwidget);
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemLabelDisp", "");
}

void
confuiMakeItemCheckButton (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *szgrp,
    const char *txt, int widx, int bdjoptIdx, int value)
{
  uiwcont_t  *uiwidgetp;

  logProcBegin (LOG_PROC, "confuiMakeItemCheckButton");

  gui->uiitem [widx].basetype = CONFUI_CHECK_BUTTON;
  gui->uiitem [widx].outtype = CONFUI_OUT_BOOL;
  uiwidgetp = uiCreateCheckButton (txt, value);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  uiBoxPackStart (boxp, uiwidgetp);
  gui->uiitem [widx].uiwidgetp = uiwidgetp;
  gui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemCheckButton", "");
}

void
confuiMakeItemLabel (uiwcont_t *boxp, uiwcont_t *szgrp, const char *txt, int indent)
{
  uiwcont_t  uiwidget;
  char        ntxt [200];
  const char  *ttxt;

  logProcBegin (LOG_PROC, "confuiMakeItemLabel");
  ttxt = txt;
  if (indent == CONFUI_INDENT) {
    strlcpy (ntxt, "   ", sizeof (ntxt));
    strlcat (ntxt, txt, sizeof (ntxt));
    ttxt = ntxt;
  }

  if (*ttxt == '\0') {
    uiCreateLabel (&uiwidget, ttxt);
  } else {
    uiCreateColonLabel (&uiwidget, ttxt);
  }
  uiBoxPackStart (boxp, &uiwidget);
  if (szgrp != NULL) {
    uiSizeGroupAdd (szgrp, &uiwidget);
  }
  if (indent == CONFUI_INDENT) {
    strlcpy (ntxt, "      ", sizeof (ntxt));
    uiCreateLabel (&uiwidget, ttxt);
    uiBoxPackStart (boxp, &uiwidget);
  }
  logProcEnd (LOG_PROC, "confuiMakeItemLabel", "");
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
  uiwcont_t*uiwidgetp;

  gui->uiitem [widx].basetype = CONFUI_ENTRY;
  gui->uiitem [widx].outtype = CONFUI_OUT_STR;
  confuiMakeItemLabel (boxp, szgrp, txt, indent);
  uiEntryCreate (gui->uiitem [widx].entry);
  uiwidgetp = uiEntryGetWidgetContainer (gui->uiitem [widx].entry);
  uiwcontCopy (&gui->uiitem [widx].uiwidget, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  if (expand == CONFUI_EXPAND) {
    uiWidgetAlignHorizFill (uiwidgetp);
    uiWidgetExpandHoriz (uiwidgetp);
    uiBoxPackStartExpand (boxp, uiwidgetp);
  }
  if (expand == CONFUI_NO_EXPAND) {
    uiBoxPackStart (boxp, uiwidgetp);
  }
  if (disp != NULL) {
    uiEntrySetValue (gui->uiitem [widx].entry, disp);
  } else {
    uiEntrySetValue (gui->uiitem [widx].entry, "");
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
    widx = CONFUI_WIDGET_MMQ_QR_CODE;
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


static long
confuiValMSCallback (void *udata, const char *txt)
{
  confuigui_t *gui = udata;
  const char  *valstr;
  char        tbuff [200];
  long        val;

  logProcBegin (LOG_PROC, "confuiValMSCallback");

  uiLabelSetText (&gui->statusMsg, "");
  valstr = validate (txt, VAL_MIN_SEC);
  if (valstr != NULL) {
    snprintf (tbuff, sizeof (tbuff), valstr, txt);
    uiLabelSetText (&gui->statusMsg, tbuff);
    return -1;
  }

  val = tmutilStrToMS (txt);
  logProcEnd (LOG_PROC, "confuiValMSCallback", "");
  return val;
}

static long
confuiValHMCallback (void *udata, const char *txt)
{
  confuigui_t *gui = udata;
  const char  *valstr;
  char        tbuff [200];
  long        val;

  logProcBegin (LOG_PROC, "confuiValHMCallback");

  uiLabelSetText (&gui->statusMsg, "");
  valstr = validate (txt, VAL_HOUR_MIN);
  if (valstr != NULL) {
    snprintf (tbuff, sizeof (tbuff), valstr, txt);
    uiLabelSetText (&gui->statusMsg, tbuff);
    return -1;
  }

  val = tmutilStrToHM (txt);
  logProcEnd (LOG_PROC, "confuiValHMCallback", "");
  return val;
}

