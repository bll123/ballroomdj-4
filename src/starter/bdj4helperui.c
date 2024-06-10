/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>

#include "bdj4.h"
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjregex.h"
#include "bdjvars.h"
#include "callback.h"
#include "conn.h"
#include "datafile.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "ossignal.h"
#include "osuiutils.h"
#include "pathbld.h"
#include "progstate.h"
#include "sockh.h"
#include "sysvars.h"
#include "ui.h"
#include "uiutils.h"

enum {
  HELPER_W_BUTTON_CLOSE,
  HELPER_W_BUTTON_NEXT,
  HELPER_W_TEXTBOX,
  HELPER_W_WINDOW,
  HELPER_W_MAX,
};

typedef struct {
  progstate_t     *progstate;
  conn_t          *conn;
  uiwcont_t       *wcont [HELPER_W_MAX];
  callback_t      *closeCallback;
  callback_t      *nextCallback;
  datafile_t      *helpdf;
  ilist_t         *helplist;
  ilistidx_t      helpiter;
  ilistidx_t      helpkey;
  bdjregex_t      *rx_br;
  bdjregex_t      *rx_dotnlsp;
  bdjregex_t      *rx_dot;
  bdjregex_t      *rx_eqgt;
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
  { "TEXT",   HELP_TEXT_TEXT,   VALUE_STR, NULL, DF_NORM },
  { "TITLE",  HELP_TEXT_TITLE,  VALUE_STR, NULL, DF_NORM },
};

int
main (int argc, char *argv[])
{
  int         status = 0;
  uint16_t    listenPort;
  helperui_t  helper;
  uint32_t    flags;
  char        tbuff [MAXPATHLEN];


#if BDJ4_MEM_DEBUG
  mdebugInit ("help");
#endif
  helper.conn = NULL;
  helper.helpdf = NULL;
  helper.helplist = NULL;
  helper.helpiter = 0;
  helper.scrollendflag = false;
  for (int i = 0; i < HELPER_W_MAX; ++i) {
    helper.wcont [i] = NULL;
  }
  helper.closeCallback = NULL;
  helper.nextCallback = NULL;
  helper.rx_br = regexInit ("<br>");
  helper.rx_dotnlsp = regexInit ("\\.\\n ");
  helper.rx_dot = regexInit ("\\.");
  helper.rx_eqgt = regexInit ("=>");

  helper.progstate = progstateInit ("helperui");
  progstateSetCallback (helper.progstate, STATE_STOPPING,
      helperStoppingCallback, &helper);
  progstateSetCallback (helper.progstate, STATE_CLOSING,
      helperClosingCallback, &helper);

  osSetStandardSignals (helperSigHandler);

  flags = BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  bdj4startup (argc, argv, NULL, "help", ROUTE_HELPERUI, &flags);
  logProcBegin ();

  listenPort = bdjvarsGetNum (BDJVL_HELPERUI_PORT);
  helper.conn = connInit (ROUTE_HELPERUI);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "helpdata", BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_TEMPLATE);
  helper.helpdf = datafileAllocParse ("helpdata",
        DFTYPE_INDIRECT, tbuff, helptextdfkeys, HELP_TEXT_MAX, DF_NO_OFFSET, NULL);
  helper.helplist = datafileGetList (helper.helpdf);
  ilistStartIterator (helper.helplist, &helper.helpiter);
  helper.helpkey = ilistIterateKey (helper.helplist, &helper.helpiter);

  uiUIInitialize (sysvarsGetNum (SVL_LOCALE_DIR));
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
  logProcEnd ("");
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

  logProcBegin ();

  uiCloseWindow (helper->wcont [HELPER_W_WINDOW]);
  uiCleanup ();

  for (int i = 0; i < HELPER_W_MAX; ++i) {
    uiwcontFree (helper->wcont [i]);
  }
  regexFree (helper->rx_br);
  regexFree (helper->rx_dotnlsp);
  regexFree (helper->rx_dot);
  regexFree (helper->rx_eqgt);

  bdj4shutdown (ROUTE_HELPERUI, NULL);

  datafileFree (helper->helpdf);

  logProcEnd ("");
  return STATE_FINISHED;
}

static void
helperBuildUI (helperui_t  *helper)
{
  uiwcont_t           *uiwidgetp;
  uiwcont_t           *vbox;
  uiwcont_t           *hbox;
  char                tbuff [MAXPATHLEN];
  char                imgbuff [MAXPATHLEN];

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", BDJ4_IMG_SVG_EXT, PATHBLD_MP_DIR_IMG);
  helper->closeCallback = callbackInit (helperCloseCallback, helper, NULL);
  /* CONTEXT: helperui: the window title for the BDJ4 helper */
  snprintf (tbuff, sizeof (tbuff), _("%s Helper"), BDJ4_LONG_NAME);
  helper->wcont [HELPER_W_WINDOW] = uiCreateMainWindow (helper->closeCallback, tbuff, imgbuff);

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 2);
  uiWindowPackInWindow (helper->wcont [HELPER_W_WINDOW], vbox);

  helper->wcont [HELPER_W_TEXTBOX] = uiTextBoxCreate (400, bdjoptGetStr (OPT_P_UI_ACCENT_COL));
  uiTextBoxSetParagraph (helper->wcont [HELPER_W_TEXTBOX], 0, 5);
  uiTextBoxHorizExpand (helper->wcont [HELPER_W_TEXTBOX]);
  uiTextBoxVertExpand (helper->wcont [HELPER_W_TEXTBOX]);
  uiTextBoxSetReadonly (helper->wcont [HELPER_W_TEXTBOX]);
  uiBoxPackStartExpand (vbox, helper->wcont [HELPER_W_TEXTBOX]);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  helper->nextCallback = callbackInit (helperNextCallback, helper, NULL);
  uiwidgetp = uiCreateButton (helper->nextCallback,
      /* CONTEXT: helperui: proceed to the next step */
      _("Next"), NULL);
  uiBoxPackEnd (hbox, uiwidgetp);
  helper->wcont [HELPER_W_BUTTON_NEXT] = uiwidgetp;

  uiwidgetp = uiCreateButton (helper->closeCallback,
      /* CONTEXT: helperui: close the helper window */
      _("Close"), NULL);
  uiBoxPackEnd (hbox, uiwidgetp);
  helper->wcont [HELPER_W_BUTTON_CLOSE] = uiwidgetp;

  uiWindowSetDefaultSize (helper->wcont [HELPER_W_WINDOW], 1100, 550);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", BDJ4_IMG_PNG_EXT, PATHBLD_MP_DIR_IMG);
  osuiSetIcon (imgbuff);

  uiWidgetShowAll (helper->wcont [HELPER_W_WINDOW]);

  uiwcontFree (vbox);
  uiwcontFree (hbox);
}

static int
helperMainLoop (void *thelper)
{
  helperui_t    *helper = thelper;
  int           stop = false;

  /* support message handling */

  if (! stop) {
    uiUIProcessEvents ();
  }

  if (! stop && helper->scrollendflag) {
    uiTextBoxScrollToEnd (helper->wcont [HELPER_W_TEXTBOX]);
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

  logProcBegin ();

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

  logProcEnd ("");

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
  const char  *title;
  const char  *text;
  char        *ttext;
  char        *ntext;

  if (helper->helpkey >= 0) {
    title = ilistGetStr (helper->helplist, helper->helpkey, HELP_TEXT_TITLE);
    text = ilistGetStr (helper->helplist, helper->helpkey, HELP_TEXT_TEXT);
    ttext = _(text);
    ntext = regexReplace (helper->rx_br, ttext, "\n");

    ttext = ntext;
    ntext = regexReplace (helper->rx_dotnlsp, ttext, ".\n");
    mdfree (ttext);

    ttext = ntext;
    ntext = regexReplace (helper->rx_dot, ttext, ".  ");
    mdfree (ttext);

    ttext = ntext;
    ntext = regexReplace (helper->rx_eqgt, ttext, "\n=>");
    mdfree (ttext);

    if (helper->helpkey > 0) {
      uiTextBoxAppendStr (helper->wcont [HELPER_W_TEXTBOX], "\n\n");
    }
    uiTextBoxAppendHighlightStr (helper->wcont [HELPER_W_TEXTBOX], _(title));
    uiTextBoxAppendStr (helper->wcont [HELPER_W_TEXTBOX], "\n\n");
    ttext = ntext;
    while (*ttext == ' ' || *ttext == '\n') {
      ++ttext;
    }
    uiTextBoxAppendStr (helper->wcont [HELPER_W_TEXTBOX], ttext);
    helper->scrollendflag = true;
    mdfree (ntext);
  }
}

