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

#include "bdj4.h"
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjvars.h"
#include "conn.h"
#include "filedata.h"
#include "datafile.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "ossignal.h"
#include "osuiutils.h"
#include "pathbld.h"
#include "progstate.h"
#include "sockh.h"
#include "ui.h"
#include "uiutils.h"
#include "callback.h"

enum {
  HELPER_BUTTON_CLOSE,
  HELPER_BUTTON_NEXT,
  HELPER_BUTTON_MAX,
};

typedef struct {
  progstate_t     *progstate;
  conn_t          *conn;
  uiwidget_t      window;
  uitextbox_t     *tb;
  uibutton_t      *buttons [HELPER_BUTTON_MAX];
  callback_t      *closeCallback;
  callback_t      *nextCallback;
  datafile_t      *helpdf;
  ilist_t         *helplist;
  ilistidx_t      helpiter;
  ilistidx_t      helpkey;
  bool            scrollendflag : 1;
} helperui_t;

static bool     helperStoppingCallback (void *udata, programstate_t programState);
static bool     helperClosingCallback (void *udata, programstate_t programState);
static void     helperBuildUI (helperui_t *helper);
static int      helperMainLoop  (void *thelper);
static int      helperProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static void     helperSigHandler (int sig);
static bool     helperCloseCallback (void *udata);
static bool     helperNextCallback (void *udata);
static void     helpDisplay (helperui_t *helper);

static int gKillReceived = 0;

enum {
  HELP_TEXT_TITLE,
  HELP_TEXT_TEXT,
  HELP_TEXT_MAX,
};

static datafilekey_t helptextdfkeys [HELP_TEXT_MAX] = {
  { "TEXT",   HELP_TEXT_TEXT,   VALUE_STR, NULL, -1 },
  { "TITLE",  HELP_TEXT_TITLE,  VALUE_STR, NULL, -1 },
};

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  helperui_t      helper;
  long            flags;
  char            tbuff [MAXPATHLEN];


#if BDJ4_MEM_DEBUG
  mdebugInit ("help");
#endif
  helper.tb = NULL;
  helper.conn = NULL;
  helper.helpdf = NULL;
  helper.helplist = NULL;
  helper.helpiter = 0;
  helper.scrollendflag = false;
  uiutilsUIWidgetInit (&helper.window);
  for (int i = 0; i < HELPER_BUTTON_MAX; ++i) {
    helper.buttons [i] = NULL;
  }
  helper.closeCallback = NULL;
  helper.nextCallback = NULL;

  helper.progstate = progstateInit ("helperui");
  progstateSetCallback (helper.progstate, STATE_STOPPING,
      helperStoppingCallback, &helper);
  progstateSetCallback (helper.progstate, STATE_CLOSING,
      helperClosingCallback, &helper);

  osSetStandardSignals (helperSigHandler);

  flags = BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  bdj4startup (argc, argv, NULL, "help", ROUTE_HELPERUI, &flags);
  logProcBegin (LOG_PROC, "helperui");

  listenPort = bdjvarsGetNum (BDJVL_HELPERUI_PORT);
  helper.conn = connInit (ROUTE_HELPERUI);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "helpdata", BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_TEMPLATE);
  helper.helpdf = datafileAllocParse ("helpdata",
        DFTYPE_INDIRECT, tbuff, helptextdfkeys, HELP_TEXT_MAX);
  helper.helplist = datafileGetList (helper.helpdf);
  ilistStartIterator (helper.helplist, &helper.helpiter);
  helper.helpkey = ilistIterateKey (helper.helplist, &helper.helpiter);

  uiUIInitialize ();
  uiSetUICSS (uiutilsGetCurrentFont (),
      bdjoptGetStr (OPT_P_UI_ACCENT_COL),
      bdjoptGetStr (OPT_P_UI_ERROR_COL));

  helperBuildUI (&helper);
  osuiFinalize ();

  helpDisplay (&helper);
  sockhMainLoop (listenPort, helperProcessMsg, helperMainLoop, &helper);
  callbackFree (helper.closeCallback);
  callbackFree (helper.nextCallback);
  connFree (helper.conn);
  progstateFree (helper.progstate);
  logProcEnd (LOG_PROC, "helperui", "");
  logEnd ();
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return status;
}

/* internal routines */

static bool
helperStoppingCallback (void *udata, programstate_t programState)
{
  helperui_t   *helper = udata;

  connDisconnectAll (helper->conn);
  return STATE_FINISHED;
}

static bool
helperClosingCallback (void *udata, programstate_t programState)
{
  helperui_t   *helper = udata;

  logProcBegin (LOG_PROC, "helperClosingCallback");
  uiCloseWindow (&helper->window);
  uiTextBoxFree (helper->tb);
  for (int i = 0; i < HELPER_BUTTON_MAX; ++i) {
    uiButtonFree (helper->buttons [i]);
  }
  uiCleanup ();

  bdj4shutdown (ROUTE_HELPERUI, NULL);

  datafileFree (helper->helpdf);

  logProcEnd (LOG_PROC, "helperClosingCallback", "");
  return STATE_FINISHED;
}

static void
helperBuildUI (helperui_t  *helper)
{
  uiwidget_t          uiwidget;
  uibutton_t          *uibutton;
  uiwidget_t          *uiwidgetp;
  uiwidget_t          vbox;
  uiwidget_t          hbox;
  char                tbuff [MAXPATHLEN];
  char                imgbuff [MAXPATHLEN];

  uiutilsUIWidgetInit (&uiwidget);
  uiutilsUIWidgetInit (&vbox);
  uiutilsUIWidgetInit (&hbox);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", BDJ4_IMG_SVG_EXT, PATHBLD_MP_DIR_IMG);
  helper->closeCallback = callbackInit (helperCloseCallback, helper, NULL);
  /* CONTEXT: helperui: the window title for the BDJ4 helper */
  snprintf (tbuff, sizeof (tbuff), _("%s Helper"), BDJ4_LONG_NAME);
  uiCreateMainWindow (&helper->window, helper->closeCallback,
      tbuff, imgbuff);

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, 2);
  uiBoxPackInWindow (&helper->window, &vbox);

  helper->tb = uiTextBoxCreate (400, NULL);
  uiTextBoxHorizExpand (helper->tb);
  uiTextBoxVertExpand (helper->tb);
  uiTextBoxSetReadonly (helper->tb);
  uiBoxPackStartExpand (&vbox, uiTextBoxGetScrolledWindow (helper->tb));

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  helper->nextCallback = callbackInit (helperNextCallback, helper, NULL);
  uibutton = uiCreateButton (helper->nextCallback,
      /* CONTEXT: helperui: proceed to the next step */
      _("Next"), NULL);
  helper->buttons [HELPER_BUTTON_NEXT] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackEnd (&hbox, uiwidgetp);

  uibutton = uiCreateButton (helper->closeCallback,
      /* CONTEXT: helperui: close the helper window */
      _("Close"), NULL);
  helper->buttons [HELPER_BUTTON_CLOSE] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackEnd (&hbox, uiwidgetp);

  uiWindowSetDefaultSize (&helper->window, 1100, 400);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", BDJ4_IMG_PNG_EXT, PATHBLD_MP_DIR_IMG);
  osuiSetIcon (imgbuff);

  uiWidgetShowAll (&helper->window);
}

static int
helperMainLoop (void *thelper)
{
  helperui_t   *helper = thelper;
  int         stop = FALSE;
  /* support message handling */


  if (! stop) {
    uiUIProcessEvents ();
  }

  if (! stop && helper->scrollendflag) {
    uiTextBoxScrollToEnd (helper->tb);
    helper->scrollendflag = false;
    uiUIProcessWaitEvents ();
    return stop;
  }

  if (! progstateIsRunning (helper->progstate)) {
    progstateProcess (helper->progstate);
    if (progstateCurrState (helper->progstate) == STATE_CLOSED) {
      stop = true;
    }
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      progstateShutdownProcess (helper->progstate);
      gKillReceived = 0;
    }
    return stop;
  }

  connProcessUnconnected (helper->conn);

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (helper->progstate);
    gKillReceived = 0;
  }
  return stop;
}

static int
helperProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  helperui_t       *helper = udata;

  logProcBegin (LOG_PROC, "helperProcessMsg");

  logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
      routefrom, msgRouteDebugText (routefrom),
      route, msgRouteDebugText (route), msg, msgDebugText (msg), args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_HELPERUI: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (helper->conn, routefrom);
          break;
        }
        case MSG_SOCKET_CLOSE: {
          connDisconnect (helper->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          progstateShutdownProcess (helper->progstate);
          gKillReceived = 0;
          break;
        }
        default: {
          break;
        }
      }
      break;
    }
    default: {
      break;
    }
  }

  logProcEnd (LOG_PROC, "helperProcessMsg", "");

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  return gKillReceived;
}


static void
helperSigHandler (int sig)
{
  gKillReceived = 1;
}


static bool
helperCloseCallback (void *udata)
{
  helperui_t   *helper = udata;

  if (progstateCurrState (helper->progstate) <= STATE_RUNNING) {
    progstateShutdownProcess (helper->progstate);
    logMsg (LOG_DBG, LOG_MSGS, "got: close win request");
  }

  return UICB_STOP;
}

static bool
helperNextCallback (void *udata)
{
  helperui_t *helper = udata;

  helper->helpkey = ilistIterateKey (helper->helplist, &helper->helpiter);
  helpDisplay (helper);
  return UICB_CONT;
}

static void
helpDisplay (helperui_t *helper)
{
  char    *title;
  char    *text;
  char    *ttext;
  char    *ntext;
  size_t  dlen;

  if (helper->helpkey >= 0) {
    title = ilistGetStr (helper->helplist, helper->helpkey, HELP_TEXT_TITLE);
    text = ilistGetStr (helper->helplist, helper->helpkey, HELP_TEXT_TEXT);
    ttext = _(text);
    dlen = strlen (ttext);
    ntext = filedataReplace (ttext, &dlen, ". ", ".\n");
    uiTextBoxAppendStr (helper->tb, "\n\n");
    uiTextBoxAppendBoldStr (helper->tb, _(title));
    uiTextBoxAppendStr (helper->tb, "\n\n");
    uiTextBoxAppendStr (helper->tb, ntext);
    helper->scrollendflag = true;
    mdfree (ntext);
  }
}

