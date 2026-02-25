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
#include "ilist.h"
#include "log.h"
#include "nlist.h"
#include "pathbld.h"
#include "sysvars.h"
#include "templateutil.h"
#include "ui.h"
#include "uidd.h"

static bool confuiRemctrlChg (void *udata, int value);
static bool confuiRemctrlPortChg (void *udata);
static void confuiLoadHTMLList (confuigui_t *gui);

enum {
  HTML_LOCALE,
  HTML_TITLE,
  HTML_FILE,
  HTML_KEY_MAX,
};

static datafilekey_t htmldfkeys [HTML_KEY_MAX] = {
  { "FILE",      HTML_FILE,     VALUE_STR, NULL, DF_NORM },
  { "LOCALE",    HTML_LOCALE,   VALUE_STR, NULL, DF_NORM },
  { "TITLE",     HTML_TITLE,    VALUE_STR, NULL, DF_NORM },
};

static int32_t confuiUIHTMLSelect (void *udata, const char *sval);

void
confuiInitMobileRemoteControl (confuigui_t *gui)
{
  confuiLoadHTMLList (gui);
}

void
confuiBuildUIMobileRemoteControl (confuigui_t *gui)
{
  uiwcont_t   *vbox;
  uiwcont_t   *szgrp;

  logProcBegin ();
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
  confuiMakeItemDropdown (gui, vbox, szgrp, _("HTML Template"),
      CONFUI_DD_RC_HTML_TEMPLATE, OPT_G_REMCONTROLHTML, confuiUIHTMLSelect);

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
  confuiMakeItemLink (gui, vbox, szgrp, "",
      CONFUI_WIDGET_RC_QR_CODE_B, "");

  uiwcontFree (vbox);
  uiwcontFree (szgrp);

  logProcEnd ("");
}

/* internal routines */

static bool
confuiRemctrlChg (void *udata, int value)
{
  confuigui_t *gui = udata;

  logProcBegin ();
  bdjoptSetNum (OPT_P_REMOTECONTROL, value);
  confuiUpdateRemctrlQrcode (gui);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
confuiRemctrlPortChg (void *udata)
{
  confuigui_t   *gui = udata;
  double        value;
  long          nval;

  logProcBegin ();
  value = uiSpinboxGetValue (gui->uiitem [CONFUI_WIDGET_RC_PORT].uiwidgetp);
  nval = (long) value;
  bdjoptSetNum (OPT_P_REMCONTROLPORT, nval);
  confuiUpdateRemctrlQrcode (gui);
  logProcEnd ("");
  return UICB_CONT;
}

static void
confuiLoadHTMLList (confuigui_t *gui)
{
  char          tbuff [BDJ4_PATH_MAX];
  datafile_t    *df = NULL;
  ilist_t       *list = NULL;
  ilistidx_t    iteridx;
  ilistidx_t    key;
  const char    *origlocale;
  const char    *origlocaleshort;
  const char    *locale;
  const char    *localeshort;
  const char    *currfn;
  int           count;
  ilist_t       *ddlist;

  logProcBegin ();

  origlocale = sysvarsGetStr (SV_LOCALE_ORIG);
  origlocaleshort = sysvarsGetStr (SV_LOCALE_ORIG_SHORT);
  locale = sysvarsGetStr (SV_LOCALE);
  localeshort = sysvarsGetStr (SV_LOCALE_SHORT);

  ddlist = ilistAlloc ("c-html-dd", LIST_ORDERED);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "html-list", BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_TEMPLATE);
  df = datafileAllocParse ("html-list", DFTYPE_INDIRECT, tbuff,
      htmldfkeys, HTML_KEY_MAX, DF_NO_OFFSET, NULL);
  list = datafileGetList (df);

  currfn = bdjoptGetStr (OPT_G_REMCONTROLHTML);

  ilistStartIterator (list, &iteridx);
  count = 0;
  while ((key = ilistIterateKey (list, &iteridx)) >= 0) {
    const char  *tlocale;
    const char  *title;
    const char  *htmlfn;
    bool        ok;

    tlocale = ilistGetStr (list, key, HTML_LOCALE);
    ok = false;
    /* include files matching original system locale, */
    /* display locale, and data locale */
    /* and english locale */
    if (strcmp (tlocale, locale) == 0 ||
        strcmp (tlocale, localeshort) == 0 ||
        strcmp (tlocale, origlocale) == 0 ||
        strcmp (tlocale, origlocaleshort) == 0 ||
        strcmp (tlocale, "en") == 0) {
      ok = true;
    }
    if (! ok) {
      continue;
    }

    title = ilistGetStr (list, key, HTML_TITLE);
    htmlfn = ilistGetStr (list, key, HTML_FILE);

    if (strcmp (htmlfn, currfn) == 0) {
      gui->uiitem [CONFUI_DD_RC_HTML_TEMPLATE].listidx = count;
    }

    ilistSetStr (ddlist, count, DD_LIST_DISP, title);
    ilistSetStr (ddlist, count, DD_LIST_KEY_STR, htmlfn);
    ilistSetNum (ddlist, count, DD_LIST_KEY_NUM, count);
    ++count;
  }
  datafileFree (df);

  gui->uiitem [CONFUI_DD_RC_HTML_TEMPLATE].ddlist = ddlist;
  logProcEnd ("");
}

static int32_t
confuiUIHTMLSelect (void *udata, const char *sval)
{
  if (sval != NULL && *sval) {
    bdjoptSetStr (OPT_G_REMCONTROLHTML, sval);
    templateHttpCopy (sval, "bdj4remote.html");
  }

  return UICB_CONT;
}


