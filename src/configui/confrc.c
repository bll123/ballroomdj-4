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
#include "nlist.h"
#include "pathbld.h"
#include "ui.h"

static bool confuiRemctrlChg (void *udata, int value);
static bool confuiRemctrlPortChg (void *udata);
static void confuiLoadHTMLList (confuigui_t *gui);

void
confuiInitMobileRemoteControl (confuigui_t *gui)
{
  confuiLoadHTMLList (gui);
}

void
confuiBuildUIMobileRemoteControl (confuigui_t *gui)
{
  uiwcont_t    *vbox;
  uiwcont_t    *szgrp;

  logProcBegin (LOG_PROC, "confuiBuildUIMobileRemoteControl");
  vbox = uiCreateVertBox ();

  szgrp = uiCreateSizeGroupHoriz ();

  /* mobile remote control */
  confuiMakeNotebookTab (vbox, gui,
      /* CONTEXT: configuration: options associated with mobile remote control */
      _("Mobile Remote Control"), CONFUI_ID_REM_CONTROL);

  /* CONTEXT: configuration: remote control: checkbox: enable/disable */
  confuiMakeItemSwitch (gui, vbox, szgrp, _("Enable Remote Control"),
      CONFUI_SWITCH_RC_ENABLE, OPT_P_REMOTECONTROL,
      bdjoptGetNum (OPT_P_REMOTECONTROL),
      confuiRemctrlChg, CONFUI_NO_INDENT);

  /* CONTEXT: configuration: remote control: the HTML template to use */
  confuiMakeItemSpinboxText (gui, vbox, szgrp, NULL, _("HTML Template"),
      CONFUI_SPINBOX_RC_HTML_TEMPLATE, OPT_G_REMCONTROLHTML,
      CONFUI_OUT_STR, gui->uiitem [CONFUI_SPINBOX_RC_HTML_TEMPLATE].listidx, NULL);

  /* CONTEXT: configuration: remote control: the user ID for sign-on to remote control */
  confuiMakeItemEntry (gui, vbox, szgrp, _("User ID"),
      CONFUI_ENTRY_RC_USER_ID,  OPT_P_REMCONTROLUSER,
      bdjoptGetStr (OPT_P_REMCONTROLUSER), CONFUI_NO_INDENT);

  /* CONTEXT: configuration: remote control: the password for sign-on to remote control */
  confuiMakeItemEntry (gui, vbox, szgrp, _("Password"),
      CONFUI_ENTRY_RC_PASS, OPT_P_REMCONTROLPASS,
      bdjoptGetStr (OPT_P_REMCONTROLPASS), CONFUI_NO_INDENT);

  /* CONTEXT: configuration: remote control: the port to use for remote control */
  confuiMakeItemSpinboxNum (gui, vbox, szgrp, NULL, _("Port"),
      CONFUI_WIDGET_RC_PORT, OPT_P_REMCONTROLPORT,
      8000, 30000, bdjoptGetNum (OPT_P_REMCONTROLPORT),
      confuiRemctrlPortChg);

  /* CONTEXT: configuration: remote control: the link to display the QR code for remote control */
  confuiMakeItemLink (gui, vbox, szgrp, _("QR Code"),
      CONFUI_WIDGET_RC_QR_CODE, "");

  uiwcontFree (vbox);
  uiwcontFree (szgrp);

  logProcEnd (LOG_PROC, "confuiBuildUIMobileRemoteControl", "");
}

/* internal routines */

static bool
confuiRemctrlChg (void *udata, int value)
{
  confuigui_t  *gui = udata;

  logProcBegin (LOG_PROC, "confuiRemctrlChg");
  bdjoptSetNum (OPT_P_REMOTECONTROL, value);
  confuiUpdateRemctrlQrcode (gui);
  logProcEnd (LOG_PROC, "confuiRemctrlChg", "");
  return UICB_CONT;
}

static bool
confuiRemctrlPortChg (void *udata)
{
  confuigui_t   *gui = udata;
  double        value;
  long          nval;

  logProcBegin (LOG_PROC, "confuiRemctrlPortChg");
  value = uiSpinboxGetValue (&gui->uiitem [CONFUI_WIDGET_RC_PORT].uiwidget);
  nval = (long) value;
  bdjoptSetNum (OPT_P_REMCONTROLPORT, nval);
  confuiUpdateRemctrlQrcode (gui);
  logProcEnd (LOG_PROC, "confuiRemctrlPortChg", "");
  return UICB_CONT;
}


static void
confuiLoadHTMLList (confuigui_t *gui)
{
  char          tbuff [MAXPATHLEN];
  nlist_t       *tlist = NULL;
  datafile_t    *df = NULL;
  slist_t       *list = NULL;
  slistidx_t    iteridx;
  char          *key;
  char          *data;
  char          *tstr;
  nlist_t       *llist;
  int           count;

  logProcBegin (LOG_PROC, "confuiLoadHTMLList");

  tlist = nlistAlloc ("cu-html-list", LIST_ORDERED, NULL);
  llist = nlistAlloc ("cu-html-list-l", LIST_ORDERED, NULL);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "html-list", BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_TEMPLATE);
  df = datafileAllocParse ("conf-html-list", DFTYPE_KEY_VAL, tbuff, NULL, 0);
  list = datafileGetList (df);

  tstr = bdjoptGetStr (OPT_G_REMCONTROLHTML);
  slistStartIterator (list, &iteridx);
  count = 0;
  while ((key = slistIterateKey (list, &iteridx)) != NULL) {
    data = slistGetStr (list, key);
    if (tstr != NULL && strcmp (data, bdjoptGetStr (OPT_G_REMCONTROLHTML)) == 0) {
      gui->uiitem [CONFUI_SPINBOX_RC_HTML_TEMPLATE].listidx = count;
    }
    nlistSetStr (tlist, count, key);
    nlistSetStr (llist, count, data);
    ++count;
  }
  datafileFree (df);

  gui->uiitem [CONFUI_SPINBOX_RC_HTML_TEMPLATE].displist = tlist;
  gui->uiitem [CONFUI_SPINBOX_RC_HTML_TEMPLATE].sbkeylist = llist;
  logProcEnd (LOG_PROC, "confuiLoadHTMLList", "");
}

