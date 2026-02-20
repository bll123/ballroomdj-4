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
#include "configui.h"
#include "log.h"
#include "ui.h"
#include "validate.h"

static bool confuiMobmqTypeChg (void *udata);
static bool confuiMobmqPortChg (void *udata);
static int  confuiMobmqTitleChg (uiwcont_t *entry, const char *label, void *udata);
static int  confuiMobmqTagChg (uiwcont_t *entry, const char *label, void *udata);
static int  confuiMobmqKeyChg (uiwcont_t *entry, const char *label, void *udata);
static void confuiMobmqSetWidgetStates (confuigui_t *gui, int type);

void
confuiInitMobileMarquee (confuigui_t *gui)
{
  confuiSpinboxTextInitDataNum (gui, "cu-mobmq-type",
      CONFUI_SPINBOX_MOBMQ_TYPE,
      /* CONTEXT: configuration: mobile marquee type: off */
      MOBMQ_TYPE_OFF, _("Off"),
      /* CONTEXT: configuration: mobile marquee type: local */
      MOBMQ_TYPE_LOCAL, _("Local"),
      /* CONTEXT: configuration: mobile marquee type: internet */
      MOBMQ_TYPE_INTERNET, _("Internet"),
      -1);

  return;
}

void
confuiBuildUIMobileMarquee (confuigui_t *gui)
{
  uiwcont_t   *vbox;
  uiwcont_t   *szgrp;

  logProcBegin ();
  vbox = uiCreateVertBox ();

  szgrp = uiCreateSizeGroupHoriz ();

  /* mobile marquee */
  confuiMakeNotebookTab (vbox, gui,
      /* CONTEXT: configuration: options associated with the mobile marquee */
      _("Mobile Marquee"), CONFUI_ID_MOBILE_MQ);

  /* CONTEXT: configuration: mobile marquee: mode: (off/local/internet)*/
  confuiMakeItemSpinboxText (gui, vbox, szgrp, NULL, _("Mode"),
      CONFUI_SPINBOX_MOBMQ_TYPE, OPT_P_MOBMQ_TYPE,
      CONFUI_OUT_NUM, bdjoptGetNum (OPT_P_MOBMQ_TYPE),
      confuiMobmqTypeChg);

  /* CONTEXT: configuration: mobile marquee: internet: tag */
  confuiMakeItemEntry (gui, vbox, szgrp, _("Tag"),
      CONFUI_ENTRY_MOBMQ_TAG, OPT_P_MOBMQ_TAG,
      bdjoptGetStr (OPT_P_MOBMQ_TAG), CONFUI_NO_INDENT);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_MOBMQ_TAG].uiwidgetp,
      _("Tag"), confuiMobmqTagChg, gui, UIENTRY_IMMEDIATE);

  /* CONTEXT: configuration: mobile marquee: internet: password */
  confuiMakeItemEntry (gui, vbox, szgrp, _("Password"),
      CONFUI_ENTRY_MOBMQ_KEY, OPT_P_MOBMQ_KEY,
      bdjoptGetStr (OPT_P_MOBMQ_KEY), CONFUI_NO_INDENT);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_MOBMQ_KEY].uiwidgetp,
      _("Password"), confuiMobmqKeyChg, gui, UIENTRY_IMMEDIATE);

  /* CONTEXT: configuration: the port to use for the mobile marquee */
  confuiMakeItemSpinboxNum (gui, vbox, szgrp, NULL, _("Port"),
      CONFUI_WIDGET_MOBMQ_PORT, OPT_P_MOBMQ_PORT,
      8000, 30000, bdjoptGetNum (OPT_P_MOBMQ_PORT),
      confuiMobmqPortChg);

  /* CONTEXT: configuration: the title to display on the mobile marquee */
  confuiMakeItemEntry (gui, vbox, szgrp, _("Title"),
      CONFUI_ENTRY_MOBMQ_TITLE, OPT_P_MOBMQ_TITLE,
      bdjoptGetStr (OPT_P_MOBMQ_TITLE), CONFUI_NO_INDENT);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_MOBMQ_TITLE].uiwidgetp,
      _("Title"), confuiMobmqTitleChg, gui, UIENTRY_IMMEDIATE);

  /* CONTEXT: configuration: mobile marquee: the link to display the QR code for the mobile marquee */
  confuiMakeItemLink (gui, vbox, szgrp, _("QR Code"),
      CONFUI_WIDGET_MOBMQ_QR_CODE, "");

  uiBoxPostProcess (vbox);
  uiwcontFree (vbox);
  uiwcontFree (szgrp);

  confuiMobmqSetWidgetStates (gui, bdjoptGetNum (OPT_P_MOBMQ_TYPE));

  logProcEnd ("");
}

/* internal routines */

static bool
confuiMobmqTypeChg (void *udata)
{
  confuigui_t   *gui = udata;
  int           type;

  logProcBegin ();
  if (gui->tablecurr != CONFUI_ID_MOBILE_MQ) {
    logProcEnd ("not-table-mobmq");
    return UIENTRY_OK;
  }
  type = uisbtextGetValue (gui->uiitem [CONFUI_SPINBOX_MOBMQ_TYPE].sb);
  bdjoptSetNum (OPT_P_MOBMQ_TYPE, type);

  /* on a type change, clear any existing validation errors */
  confuiMarkValid (gui, CONFUI_ENTRY_MOBMQ_TAG);
  confuiMarkValid (gui, CONFUI_ENTRY_MOBMQ_KEY);

  confuiMobmqSetWidgetStates (gui, type);
  confuiUpdateMobmqQrcode (gui);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
confuiMobmqPortChg (void *udata)
{
  confuigui_t   *gui = udata;
  double        value;
  long          nval;

  logProcBegin ();
  if (gui->tablecurr != CONFUI_ID_MOBILE_MQ) {
    logProcEnd ("not-table-mobmq");
    return UIENTRY_OK;
  }
  value = uisbtextGetValue (gui->uiitem [CONFUI_WIDGET_MOBMQ_PORT].sb);
  nval = (long) value;
  bdjoptSetNum (OPT_P_MOBMQ_PORT, nval);
  confuiUpdateMobmqQrcode (gui);
  logProcEnd ("");
  return UICB_CONT;
}

static int
confuiMobmqTitleChg (uiwcont_t *entry, const char *label, void *udata)
{
  confuigui_t     *gui = udata;
  const char      *sval;

  logProcBegin ();
  if (gui->tablecurr != CONFUI_ID_MOBILE_MQ) {
    logProcEnd ("not-table-mobmq");
    return UIENTRY_OK;
  }
  sval = uiEntryGetValue (entry);
  bdjoptSetStr (OPT_P_MOBMQ_TITLE, sval);
  confuiUpdateMobmqQrcode (gui);
  logProcEnd ("");
  return UIENTRY_OK;
}

static int
confuiMobmqTagChg (uiwcont_t *entry, const char *label, void *udata)
{
  confuigui_t     *gui = udata;
  const char      *sval;
  bool            vrc;
  char            tmsg [200];
  int             type;
  int             widx;

  logProcBegin ();
  if (gui->tablecurr != CONFUI_ID_MOBILE_MQ) {
    logProcEnd ("not-table-mobmq");
    return UIENTRY_OK;
  }
  type = bdjoptGetNum (OPT_P_MOBMQ_TYPE);
  if (type != MOBMQ_TYPE_INTERNET) {
    return UIENTRY_OK;
  }
  widx = CONFUI_ENTRY_MOBMQ_TAG;

  sval = uiEntryGetValue (entry);
  vrc = validate (tmsg, sizeof (tmsg), label, sval,
      VAL_NOT_EMPTY | VAL_NO_SPACES);
  if (vrc == false) {
    confuiSetErrorMsg (gui, tmsg);
    confuiMarkNotValid (gui, widx);
    return UIENTRY_ERROR;
  }
  confuiMarkValid (gui, widx);
  bdjoptSetStr (OPT_P_MOBMQ_TAG, sval);
  confuiUpdateMobmqQrcode (gui);
  logProcEnd ("");
  return UIENTRY_OK;
}

static int
confuiMobmqKeyChg (uiwcont_t *entry, const char *label, void *udata)
{
  confuigui_t     *gui = udata;
  const char      *sval;
  bool            vrc;
  char            tmsg [200];
  int             type;
  int             widx;

  logProcBegin ();
  if (gui->tablecurr != CONFUI_ID_MOBILE_MQ) {
    logProcEnd ("not-table-mobmq");
    return UIENTRY_OK;
  }
  type = bdjoptGetNum (OPT_P_MOBMQ_TYPE);
  if (type != MOBMQ_TYPE_INTERNET) {
    return UIENTRY_OK;
  }
  widx = CONFUI_ENTRY_MOBMQ_KEY;

  sval = uiEntryGetValue (entry);
  vrc = validate (tmsg, sizeof (tmsg), label, sval,
      VAL_NOT_EMPTY | VAL_NO_SPACES);
  if (vrc == false) {
    confuiSetErrorMsg (gui, tmsg);
    confuiMarkNotValid (gui, widx);
    return UIENTRY_ERROR;
  }
  confuiMarkValid (gui, widx);
  bdjoptSetStr (OPT_P_MOBMQ_KEY, sval);
  logProcEnd ("");
  return UIENTRY_OK;
}

static void
confuiMobmqSetWidgetStates (confuigui_t *gui, int type)
{
  if (type == MOBMQ_TYPE_LOCAL || type == MOBMQ_TYPE_OFF) {
    uiWidgetSetState (gui->uiitem [CONFUI_ENTRY_MOBMQ_TAG].uilabelp, UIWIDGET_DISABLE);
    uiWidgetSetState (gui->uiitem [CONFUI_ENTRY_MOBMQ_KEY].uilabelp, UIWIDGET_DISABLE);
    uiWidgetSetState (gui->uiitem [CONFUI_WIDGET_MOBMQ_PORT].uilabelp, UIWIDGET_ENABLE);

    uiWidgetSetState (gui->uiitem [CONFUI_ENTRY_MOBMQ_TAG].uiwidgetp, UIWIDGET_DISABLE);
    uiWidgetSetState (gui->uiitem [CONFUI_ENTRY_MOBMQ_KEY].uiwidgetp, UIWIDGET_DISABLE);
    uiWidgetSetState (gui->uiitem [CONFUI_WIDGET_MOBMQ_PORT].uiwidgetp, UIWIDGET_ENABLE);
  }
  if (type == MOBMQ_TYPE_INTERNET) {
    uiWidgetSetState (gui->uiitem [CONFUI_ENTRY_MOBMQ_TAG].uilabelp, UIWIDGET_ENABLE);
    uiWidgetSetState (gui->uiitem [CONFUI_ENTRY_MOBMQ_KEY].uilabelp, UIWIDGET_ENABLE);
    uiWidgetSetState (gui->uiitem [CONFUI_WIDGET_MOBMQ_PORT].uilabelp, UIWIDGET_DISABLE);

    uiWidgetSetState (gui->uiitem [CONFUI_ENTRY_MOBMQ_TAG].uiwidgetp, UIWIDGET_ENABLE);
    uiWidgetSetState (gui->uiitem [CONFUI_ENTRY_MOBMQ_KEY].uiwidgetp, UIWIDGET_ENABLE);
    uiWidgetSetState (gui->uiitem [CONFUI_WIDGET_MOBMQ_PORT].uiwidgetp, UIWIDGET_DISABLE);
  }
}
