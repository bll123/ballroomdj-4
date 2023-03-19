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
#include "configui.h"
#include "log.h"
#include "ui.h"

static bool confuiMobmqTypeChg (void *udata);
static bool confuiMobmqPortChg (void *udata);
static int  confuiMobmqTitleChg (uientry_t *entry, void *udata);

void
confuiInitMobileMarquee (confuigui_t *gui)
{
}

void
confuiBuildUIMobileMarquee (confuigui_t *gui)
{
  uiwidget_t    vbox;
  uiwidget_t    sg;

  logProcBegin (LOG_PROC, "confuiBuildUIMobileMarquee");
  uiCreateVertBox (&vbox);

  /* mobile marquee */
  confuiMakeNotebookTab (&vbox, gui,
      /* CONTEXT: configuration: options associated with the mobile marquee */
      _("Mobile Marquee"), CONFUI_ID_MOBILE_MQ);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: enable mobile marquee */
  confuiMakeItemSwitch (gui, &vbox, &sg, _("Enable Mobile Marquee"),
      CONFUI_SWITCH_MOBILE_MQ, OPT_P_MOBILEMARQUEE,
      bdjoptGetNum (OPT_P_MOBILEMARQUEE), confuiMobmqTypeChg, CONFUI_NO_INDENT);

  /* CONTEXT: configuration: the port to use for the mobile marquee */
  confuiMakeItemSpinboxNum (gui, &vbox, &sg, NULL, _("Port"),
      CONFUI_WIDGET_MMQ_PORT, OPT_P_MOBILEMQPORT,
      8000, 30000, bdjoptGetNum (OPT_P_MOBILEMQPORT),
      confuiMobmqPortChg);

  /* CONTEXT: configuration: the title to display on the mobile marquee */
  confuiMakeItemEntry (gui, &vbox, &sg, _("Title"),
      CONFUI_ENTRY_MM_TITLE, OPT_P_MOBILEMQTITLE,
      bdjoptGetStr (OPT_P_MOBILEMQTITLE), CONFUI_NO_INDENT);
  uiEntrySetValidate (gui->uiitem [CONFUI_ENTRY_MM_TITLE].entry,
      confuiMobmqTitleChg, gui, UIENTRY_IMMEDIATE);

  /* CONTEXT: configuration: mobile marquee: the link to display the QR code for the mobile marquee */
  confuiMakeItemLink (gui, &vbox, &sg, _("QR Code"),
      CONFUI_WIDGET_MMQ_QR_CODE, "");
  logProcEnd (LOG_PROC, "confuiBuildUIMobileMarquee", "");
}

/* internal routines */

static bool
confuiMobmqTypeChg (void *udata)
{
  confuigui_t   *gui = udata;
  double        value;
  long          nval;

  logProcBegin (LOG_PROC, "confuiMobmqTypeChg");
  value = uiSwitchGetValue (gui->uiitem [CONFUI_SWITCH_MOBILE_MQ].uiswitch);
  nval = (long) value;
  bdjoptSetNum (OPT_P_MOBILEMARQUEE, nval);
  confuiUpdateMobmqQrcode (gui);
  logProcEnd (LOG_PROC, "confuiMobmqTypeChg", "");
  return UICB_CONT;
}

static bool
confuiMobmqPortChg (void *udata)
{
  confuigui_t   *gui = udata;
  double        value;
  long          nval;

  logProcBegin (LOG_PROC, "confuiMobmqPortChg");
  value = uiSpinboxGetValue (&gui->uiitem [CONFUI_WIDGET_MMQ_PORT].uiwidget);
  nval = (long) value;
  bdjoptSetNum (OPT_P_MOBILEMQPORT, nval);
  confuiUpdateMobmqQrcode (gui);
  logProcEnd (LOG_PROC, "confuiMobmqPortChg", "");
  return UICB_CONT;
}

static int
confuiMobmqTitleChg (uientry_t *entry, void *udata)
{
  confuigui_t     *gui = udata;
  const char      *sval;

  logProcBegin (LOG_PROC, "confuiMobmqTitleChg");
  sval = uiEntryGetValue (entry);
  bdjoptSetStr (OPT_P_MOBILEMQTITLE, sval);
  confuiUpdateMobmqQrcode (gui);
  logProcEnd (LOG_PROC, "confuiMobmqTitleChg", "");
  return UIENTRY_OK;
}

