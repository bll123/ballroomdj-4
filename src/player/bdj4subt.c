/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>

#include "bdj4.h"
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "datafile.h"
#include "callback.h"
#include "conn.h"
#include "localeutil.h"
#include "lock.h"
#include "log.h"
#include "mdebug.h"
#include "osuiutils.h"
#include "ossignal.h"
#include "pathbld.h"
#include "progstate.h"
#include "sock.h"
#include "sockh.h"
#include "sysvars.h"
#include "tagdef.h"
#include "ui.h"
#include "uiclass.h"
#include "uiutils.h"

enum {
  SUBT_POSITION_X,
  SUBT_POSITION_Y,
  SUBT_WORKSPACE,
  SUBT_SIZE_X,
  SUBT_SIZE_Y,
  SUBT_FONT_SZ,
  SUBT_KEY_MAX,
};

enum {
  SUBT_CB_EXIT,
  SUBT_CB_MAX,
};

enum {
  SUBT_W_WINDOW,
  SUBT_W_TEXTBOX,
  SUBT_W_MAX,
};

/* sort by ascii values */
static datafilekey_t mqdfkeys [SUBT_KEY_MAX] = {
  { "SUBT_FONT_SZ",   SUBT_FONT_SZ,       VALUE_NUM, NULL, DF_NORM },
  { "SUBT_POS_X",     SUBT_POSITION_X,    VALUE_NUM, NULL, DF_NORM },
  { "SUBT_POS_Y",     SUBT_POSITION_Y,    VALUE_NUM, NULL, DF_NORM },
  { "SUBT_SIZE_X",    SUBT_SIZE_X,        VALUE_NUM, NULL, DF_NORM },
  { "SUBT_SIZE_Y",    SUBT_SIZE_Y,        VALUE_NUM, NULL, DF_NORM },
  { "SUBT_WORKSPACE", SUBT_WORKSPACE,     VALUE_NUM, NULL, DF_NORM },
};

typedef struct {
  progstate_t     *progstate;
  char            *locknm;
  conn_t          *conn;
  datafile_t      *optiondf;
  nlist_t         *options;
  uiwcont_t       *window;
  callback_t      *callbacks [SUBT_CB_MAX];
  uiwcont_t       *wcont [SUBT_W_MAX];
  int             stopwaitcount;
  bool            optionsalloc;
  bool            hideonstart;
  bool            isiconified;
  bool            uibuilt;
} subt_t;

static bool subtConnectingCallback (void *udata, programstate_t programState);
static bool subtHandshakeCallback (void *udata, programstate_t programState);
static bool subtStoppingCallback (void *udata, programstate_t programState);
static bool subtStopWaitCallback (void *udata, programstate_t programState);
static bool subtClosingCallback (void *udata, programstate_t programState);
static void subtBuildUI (subt_t *subt);
static int  subtMainLoop  (void *tsubt);
static int  subtProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                bdjmsgmsg_t msg, char *args, void *udata);
static bool subtCloseCallback (void *udata);
static void subtSaveWindowPosition (subt_t *);
static void subtMoveWindow (subt_t *);
static void subtSigHandler (int sig);

static int gKillReceived = 0;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  subt_t          subt;
  char            tbuff [MAXPATHLEN];
  uint32_t        flags;
  uisetup_t       uisetup;

#if BDJ4_MEM_DEBUG
  mdebugInit ("subt");
#endif

  subt.progstate = progstateInit ("subt");
  progstateSetCallback (subt.progstate, PROGSTATE_CONNECTING,
      subtConnectingCallback, &subt);
  progstateSetCallback (subt.progstate, PROGSTATE_WAIT_HANDSHAKE,
      subtHandshakeCallback, &subt);
  progstateSetCallback (subt.progstate, PROGSTATE_STOPPING,
      subtStoppingCallback, &subt);
  progstateSetCallback (subt.progstate, PROGSTATE_STOP_WAIT,
      subtStopWaitCallback, &subt);
  progstateSetCallback (subt.progstate, PROGSTATE_CLOSING,
      subtClosingCallback, &subt);

  subt.stopwaitcount = 0;
  for (int i = 0; i < SUBT_W_MAX; ++i) {
    subt.wcont [i] = NULL;
  }
  subt.uibuilt = false;
  for (int i = 0; i < SUBT_CB_MAX; ++i) {
    subt.callbacks [i] = NULL;
  }
  subt.hideonstart = false;
  subt.isiconified = false;

  osSetStandardSignals (subtSigHandler);

  flags = BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  bdj4startup (argc, argv, NULL, "subt", ROUTE_SUBT, &flags);
  if (bdjoptGetNum (OPT_P_SUBT_SHOW) == BDJWIN_SHOW_MINIMIZE) {
    subt.hideonstart = true;
  }

  listenPort = bdjvarsGetNum (BDJVL_PORT_SUBT);
  subt.conn = connInit (ROUTE_SUBT);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "subt", BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  subt.optiondf = datafileAllocParse ("ui-subt", DFTYPE_KEY_VAL, tbuff,
      mqdfkeys, SUBT_KEY_MAX, DF_NO_OFFSET, NULL);
  subt.options = datafileGetList (subt.optiondf);
  subt.optionsalloc = false;
  if (subt.options == NULL || nlistGetCount (subt.options) == 0) {
    subt.optionsalloc = true;
    subt.options = nlistAlloc ("ui-subt", LIST_ORDERED, NULL);

    nlistSetNum (subt.options, SUBT_WORKSPACE, -1);
    nlistSetNum (subt.options, SUBT_POSITION_X, -1);
    nlistSetNum (subt.options, SUBT_POSITION_Y, -1);
    nlistSetNum (subt.options, SUBT_SIZE_X, 600);
    nlistSetNum (subt.options, SUBT_SIZE_Y, -1);
    nlistSetNum (subt.options, SUBT_FONT_SZ, 36);
  }

  uiUIInitialize (sysvarsGetNum (SVL_LOCALE_DIR));
  uiutilsInitSetup (&uisetup);
  uiSetUICSS (&uisetup);

  subtBuildUI (&subt);
  osuiFinalize ();

  sockhMainLoop (listenPort, subtProcessMsg, subtMainLoop, &subt);
  connFree (subt.conn);
  progstateFree (subt.progstate);
  for (int i = 0; i < SUBT_CB_MAX; ++i) {
    callbackFree (subt.callbacks [i]);
  }
  logEnd ();
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return status;
}

/* internal routines */

static bool
subtStoppingCallback (void *udata, programstate_t programState)
{
  subt_t     *subt = udata;
  int           x, y;

  logProcBegin ();

  uiWindowGetSize (subt->wcont [SUBT_W_WINDOW], &x, &y);
  nlistSetNum (subt->options, SUBT_SIZE_X, x);
  nlistSetNum (subt->options, SUBT_SIZE_Y, y);
  subtSaveWindowPosition (subt);

  connDisconnectAll (subt->conn);
  logProcEnd ("");
  return STATE_FINISHED;
}

static bool
subtStopWaitCallback (void *tsubt, programstate_t programState)
{
  subt_t   *subt = tsubt;
  bool        rc;

  rc = connWaitClosed (subt->conn, &subt->stopwaitcount);
  return rc;
}

static bool
subtClosingCallback (void *udata, programstate_t programState)
{
  subt_t   *subt = udata;

  logProcBegin ();

  /* these are moved here so that the window can be un-maximized and */
  /* the size/position saved */
  uiCloseWindow (subt->wcont [SUBT_W_WINDOW]);
  uiCleanup ();

  datafileSave (subt->optiondf, NULL, subt->options, DF_NO_OFFSET, 1);

  bdj4shutdown (ROUTE_SUBT, NULL);

  for (int i = 0; i < SUBT_W_MAX; ++i) {
    uiwcontFree (subt->wcont [i]);
  }

  if (subt->optionsalloc) {
    nlistFree (subt->options);
  }
  datafileFree (subt->optiondf);

  logProcEnd ("");
  return STATE_FINISHED;
}

static void
subtBuildUI (subt_t *subt)
{
  char        imgbuff [MAXPATHLEN];
//  uiwcont_t   *uiwidgetp;
  uiwcont_t   *mainvbox;
//  uiwcont_t   *hbox;
//  uiwcont_t   *vbox;
  int         x, y;

  logProcBegin ();

  subt->callbacks [SUBT_CB_EXIT] = callbackInit (
      subtCloseCallback, subt, NULL);
  subt->wcont [SUBT_W_WINDOW] = uiCreateMainWindow (
      subt->callbacks [SUBT_CB_EXIT],
      "", "bdj4_icon_subt");
  uiWindowNoFocusOnStartup (subt->wcont [SUBT_W_WINDOW]);

  uiWindowNoDim (subt->wcont [SUBT_W_WINDOW]);

  x = nlistGetNum (subt->options, SUBT_SIZE_X);
  y = nlistGetNum (subt->options, SUBT_SIZE_Y);
  uiWindowSetDefaultSize (subt->wcont [SUBT_W_WINDOW], x, y);

  mainvbox = uiCreateVertBox ();
  uiWindowPackInWindow (subt->wcont [SUBT_W_WINDOW], mainvbox);
  uiWidgetSetAllMargins (mainvbox, 10);
  uiWidgetExpandHoriz (mainvbox);
  uiWidgetExpandVert (mainvbox);

  subt->wcont [SUBT_W_TEXTBOX] = uiTextBoxCreate (200, NULL);

  if (subt->hideonstart) {
    uiWindowIconify (subt->wcont [SUBT_W_WINDOW]);
    subt->isiconified = true;
  }

  uiWidgetShowAll (subt->wcont [SUBT_W_WINDOW]);

  subtMoveWindow (subt);

  progstateLogTime (subt->progstate, "time-to-start-gui");

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_subt", BDJ4_IMG_PNG_EXT, PATHBLD_MP_DIR_IMG);
  osuiSetIcon (imgbuff);

  uiwcontFree (mainvbox);
//  uiwcontFree (vbox);

  subt->uibuilt = true;

  logProcEnd ("");
}

static int
subtMainLoop (void *tsubt)
{
  subt_t   *subt = tsubt;
  int         stop = SOCKH_CONTINUE;

  uiUIProcessEvents ();

  if (! progstateIsRunning (subt->progstate)) {
    progstateProcess (subt->progstate);
    if (progstateCurrState (subt->progstate) == PROGSTATE_CLOSED) {
      stop = SOCKH_STOP;
    }
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      progstateShutdownProcess (subt->progstate);
    }
    return stop;
  }

  connProcessUnconnected (subt->conn);

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (subt->progstate);
  }
  return stop;
}

static bool
subtConnectingCallback (void *udata, programstate_t programState)
{
  subt_t   *subt = udata;
  bool        rc = STATE_NOT_FINISH;

  logProcBegin ();

  connProcessUnconnected (subt->conn);

  if (! connIsConnected (subt->conn, ROUTE_MAIN)) {
    connConnect (subt->conn, ROUTE_MAIN);
  }
  if (! connIsConnected (subt->conn, ROUTE_PLAYERUI)) {
    connConnect (subt->conn, ROUTE_PLAYERUI);
  }

  if (connIsConnected (subt->conn, ROUTE_MAIN) &&
      connIsConnected (subt->conn, ROUTE_PLAYERUI)) {
    rc = STATE_FINISHED;
  }

  logProcEnd ("");
  return rc;
}

static bool
subtHandshakeCallback (void *udata, programstate_t programState)
{
  subt_t   *subt = udata;
  bool        rc = STATE_NOT_FINISH;

  logProcBegin ();

  connProcessUnconnected (subt->conn);

  if (connHaveHandshake (subt->conn, ROUTE_MAIN) &&
      connHaveHandshake (subt->conn, ROUTE_PLAYERUI)) {
    rc = STATE_FINISHED;
  }

  logProcEnd ("");
  return rc;
}

static int
subtProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  subt_t   *subt = udata;

  logProcBegin ();

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_SUBT: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (subt->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          progstateShutdownProcess (subt->progstate);
          break;
        }
        case MSG_WINDOW_SHOW: {
          if (subt->isiconified) {
            uiWindowDeIconify (subt->wcont [SUBT_W_WINDOW]);
          }
          break;
        }
        case MSG_WINDOW_HIDE: {
          if (! subt->isiconified) {
            uiWindowIconify (subt->wcont [SUBT_W_WINDOW]);
          }
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
  logProcEnd ("");
  return gKillReceived;
}


static bool
subtCloseCallback (void *udata)
{
  subt_t   *subt = udata;

  logProcBegin ();

  if (progstateCurrState (subt->progstate) <= PROGSTATE_RUNNING) {
    subtSaveWindowPosition (subt);

    uiWindowIconify (subt->wcont [SUBT_W_WINDOW]);
    subt->isiconified = true;
    logProcEnd ("user-close-win");
    return UICB_STOP;
  }

  logProcEnd ("");
  return UICB_CONT;
}


static void
subtSaveWindowPosition (subt_t *subt)
{
  int   x, y, ws;

  uiWindowGetPosition (subt->wcont [SUBT_W_WINDOW], &x, &y, &ws);
  /* on windows, when the window is iconified, the position cannot be */
  /* fetched like on linux; -32000 is returned for the position */
  if (x != -32000 && y != -32000 ) {
    nlistSetNum (subt->options, SUBT_POSITION_X, x);
    nlistSetNum (subt->options, SUBT_POSITION_Y, y);
    nlistSetNum (subt->options, SUBT_WORKSPACE, ws);
  }
}

static void
subtMoveWindow (subt_t *subt)
{
  int   x, y, ws;

  x = nlistGetNum (subt->options, SUBT_POSITION_X);
  y = nlistGetNum (subt->options, SUBT_POSITION_Y);
  ws = nlistGetNum (subt->options, SUBT_WORKSPACE);
  uiWindowMove (subt->wcont [SUBT_W_WINDOW], x, y, ws);
}

static void
subtSigHandler (int sig)
{
  gKillReceived = 1;
}

