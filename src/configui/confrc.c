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

static bool confuiRemctrlChg (void *udata, int value);
static bool confuiRemctrlPortChg (void *udata);

void
confuiBuildUIMobileRemoteControl (confuigui_t *gui)
{
  UIWidget      vbox;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIMobileRemoteControl");
  uiCreateVertBox (&vbox);

  /* mobile remote control */
  confuiMakeNotebookTab (&vbox, gui,
      /* CONTEXT: configuration: options associated with mobile remote control */
      _("Mobile Remote Control"), CONFUI_ID_REM_CONTROL);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: remote control: checkbox: enable/disable */
  confuiMakeItemSwitch (gui, &vbox, &sg, _("Enable Remote Control"),
      CONFUI_SWITCH_RC_ENABLE, OPT_P_REMOTECONTROL,
      bdjoptGetNum (OPT_P_REMOTECONTROL),
      confuiRemctrlChg);

  /* CONTEXT: configuration: remote control: the HTML template to use */
  confuiMakeItemSpinboxText (gui, &vbox, &sg, NULL, _("HTML Template"),
      CONFUI_SPINBOX_RC_HTML_TEMPLATE, OPT_G_REMCONTROLHTML,
      CONFUI_OUT_STR, gui->uiitem [CONFUI_SPINBOX_RC_HTML_TEMPLATE].listidx, NULL);

  /* CONTEXT: configuration: remote control: the user ID for sign-on to remote control */
  confuiMakeItemEntry (gui, &vbox, &sg, _("User ID"),
      CONFUI_ENTRY_RC_USER_ID,  OPT_P_REMCONTROLUSER,
      bdjoptGetStr (OPT_P_REMCONTROLUSER));

  /* CONTEXT: configuration: remote control: the password for sign-on to remote control */
  confuiMakeItemEntry (gui, &vbox, &sg, _("Password"),
      CONFUI_ENTRY_RC_PASS, OPT_P_REMCONTROLPASS,
      bdjoptGetStr (OPT_P_REMCONTROLPASS));

  /* CONTEXT: configuration: remote control: the port to use for remote control */
  confuiMakeItemSpinboxNum (gui, &vbox, &sg, NULL, _("Port"),
      CONFUI_WIDGET_RC_PORT, OPT_P_REMCONTROLPORT,
      8000, 30000, bdjoptGetNum (OPT_P_REMCONTROLPORT),
      confuiRemctrlPortChg);

  /* CONTEXT: configuration: remote control: the link to display the QR code for remote control */
  confuiMakeItemLink (gui, &vbox, &sg, _("QR Code"),
      CONFUI_WIDGET_RC_QR_CODE, "");
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


