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
  LYR_POSITION_X,
  LYR_POSITION_Y,
  LYR_WORKSPACE,
  LYR_SIZE_X,
  LYR_SIZE_Y,
  LYR_FONT_SZ,
  LYR_KEY_MAX,
};

enum {
  LYR_CB_EXIT,
  LYR_CB_MAX,
};

enum {
  LYR_W_WINDOW,
  LYR_W_TEXTBOX,
  LYR_W_MAX,
};

/* sort by ascii values */
static datafilekey_t mqdfkeys [LYR_KEY_MAX] = {
  { "LYR_FONT_SZ",   LYR_FONT_SZ,       VALUE_NUM, NULL, DF_NORM },
  { "LYR_POS_X",     LYR_POSITION_X,    VALUE_NUM, NULL, DF_NORM },
  { "LYR_POS_Y",     LYR_POSITION_Y,    VALUE_NUM, NULL, DF_NORM },
  { "LYR_SIZE_X",    LYR_SIZE_X,        VALUE_NUM, NULL, DF_NORM },
  { "LYR_SIZE_Y",    LYR_SIZE_Y,        VALUE_NUM, NULL, DF_NORM },
  { "LYR_WORKSPACE", LYR_WORKSPACE,     VALUE_NUM, NULL, DF_NORM },
};

typedef struct {
  progstate_t     *progstate;
  char            *locknm;
  conn_t          *conn;
  datafile_t      *optiondf;
  nlist_t         *options;
  uiwcont_t       *window;
  callback_t      *callbacks [LYR_CB_MAX];
  uiwcont_t       *wcont [LYR_W_MAX];
  int             stopwaitcount;
  bool            optionsalloc;
  bool            hideonstart;
  bool            isiconified;
  bool            uibuilt;
} lyrics_t;

static bool lyrConnectingCallback (void *udata, programstate_t programState);
static bool lyrHandshakeCallback (void *udata, programstate_t programState);
static bool lyrStoppingCallback (void *udata, programstate_t programState);
static bool lyrStopWaitCallback (void *udata, programstate_t programState);
static bool lyrClosingCallback (void *udata, programstate_t programState);
static void lyrBuildUI (lyrics_t *lyr);
static int  lyrMainLoop  (void *tlyr);
static int  lyrProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                bdjmsgmsg_t msg, char *args, void *udata);
static bool lyrCloseCallback (void *udata);
static void lyrSaveWindowPosition (lyrics_t *);
static void lyrMoveWindow (lyrics_t *);
static void lyrSigHandler (int sig);

static int gKillReceived = 0;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  lyrics_t          lyr;
  char            tbuff [MAXPATHLEN];
  uint32_t        flags;
  uisetup_t       uisetup;

#if BDJ4_MEM_DEBUG
  mdebugInit ("lyr");
#endif

  lyr.progstate = progstateInit ("lyr");
  progstateSetCallback (lyr.progstate, PROGSTATE_CONNECTING,
      lyrConnectingCallback, &lyr);
  progstateSetCallback (lyr.progstate, PROGSTATE_WAIT_HANDSHAKE,
      lyrHandshakeCallback, &lyr);
  progstateSetCallback (lyr.progstate, PROGSTATE_STOPPING,
      lyrStoppingCallback, &lyr);
  progstateSetCallback (lyr.progstate, PROGSTATE_STOP_WAIT,
      lyrStopWaitCallback, &lyr);
  progstateSetCallback (lyr.progstate, PROGSTATE_CLOSING,
      lyrClosingCallback, &lyr);

  lyr.stopwaitcount = 0;
  for (int i = 0; i < LYR_W_MAX; ++i) {
    lyr.wcont [i] = NULL;
  }
  lyr.uibuilt = false;
  for (int i = 0; i < LYR_CB_MAX; ++i) {
    lyr.callbacks [i] = NULL;
  }
  lyr.hideonstart = false;
  lyr.isiconified = false;

  osSetStandardSignals (lyrSigHandler);

  flags = BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  bdj4startup (argc, argv, NULL, "lyr", ROUTE_LYRICS, &flags);
  if (bdjoptGetNum (OPT_P_LYRICS_SHOW) == BDJWIN_SHOW_MINIMIZE) {
    lyr.hideonstart = true;
  }

  listenPort = bdjvarsGetNum (BDJVL_PORT_LYRICS);
  lyr.conn = connInit (ROUTE_LYRICS);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "lyr", BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  lyr.optiondf = datafileAllocParse ("ui-lyr", DFTYPE_KEY_VAL, tbuff,
      mqdfkeys, LYR_KEY_MAX, DF_NO_OFFSET, NULL);
  lyr.options = datafileGetList (lyr.optiondf);
  lyr.optionsalloc = false;
  if (lyr.options == NULL || nlistGetCount (lyr.options) == 0) {
    lyr.optionsalloc = true;
    lyr.options = nlistAlloc ("ui-lyr", LIST_ORDERED, NULL);

    nlistSetNum (lyr.options, LYR_WORKSPACE, -1);
    nlistSetNum (lyr.options, LYR_POSITION_X, -1);
    nlistSetNum (lyr.options, LYR_POSITION_Y, -1);
    nlistSetNum (lyr.options, LYR_SIZE_X, 600);
    nlistSetNum (lyr.options, LYR_SIZE_Y, -1);
    nlistSetNum (lyr.options, LYR_FONT_SZ, 36);
  }

  uiUIInitialize (sysvarsGetNum (SVL_LOCALE_DIR));
  uiutilsInitSetup (&uisetup);
  uiSetUICSS (&uisetup);

  lyrBuildUI (&lyr);
  osuiFinalize ();

  sockhMainLoop (listenPort, lyrProcessMsg, lyrMainLoop, &lyr);
  connFree (lyr.conn);
  progstateFree (lyr.progstate);
  for (int i = 0; i < LYR_CB_MAX; ++i) {
    callbackFree (lyr.callbacks [i]);
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
lyrStoppingCallback (void *udata, programstate_t programState)
{
  lyrics_t     *lyr = udata;
  int           x, y;

  logProcBegin ();

  uiWindowGetSize (lyr->wcont [LYR_W_WINDOW], &x, &y);
  nlistSetNum (lyr->options, LYR_SIZE_X, x);
  nlistSetNum (lyr->options, LYR_SIZE_Y, y);
  lyrSaveWindowPosition (lyr);

  connDisconnectAll (lyr->conn);
  logProcEnd ("");
  return STATE_FINISHED;
}

static bool
lyrStopWaitCallback (void *tlyr, programstate_t programState)
{
  lyrics_t   *lyr = tlyr;
  bool        rc;

  rc = connWaitClosed (lyr->conn, &lyr->stopwaitcount);
  return rc;
}

static bool
lyrClosingCallback (void *udata, programstate_t programState)
{
  lyrics_t   *lyr = udata;

  logProcBegin ();

  /* these are moved here so that the window can be un-maximized and */
  /* the size/position saved */
  uiCloseWindow (lyr->wcont [LYR_W_WINDOW]);
  uiCleanup ();

  datafileSave (lyr->optiondf, NULL, lyr->options, DF_NO_OFFSET, 1);

  bdj4shutdown (ROUTE_LYRICS, NULL);

  for (int i = 0; i < LYR_W_MAX; ++i) {
    uiwcontFree (lyr->wcont [i]);
  }

  if (lyr->optionsalloc) {
    nlistFree (lyr->options);
  }
  datafileFree (lyr->optiondf);

  logProcEnd ("");
  return STATE_FINISHED;
}

static void
lyrBuildUI (lyrics_t *lyr)
{
  char        imgbuff [MAXPATHLEN];
  uiwcont_t   *uiwidgetp;
  uiwcont_t   *vbox;
  int         x, y;

  logProcBegin ();

  lyr->callbacks [LYR_CB_EXIT] = callbackInit (
      lyrCloseCallback, lyr, NULL);
  lyr->wcont [LYR_W_WINDOW] = uiCreateMainWindow (
      lyr->callbacks [LYR_CB_EXIT],
      "", "bdj4_icon_lyr");
  uiWindowNoFocusOnStartup (lyr->wcont [LYR_W_WINDOW]);

  uiWindowNoDim (lyr->wcont [LYR_W_WINDOW]);

  x = nlistGetNum (lyr->options, LYR_SIZE_X);
  y = nlistGetNum (lyr->options, LYR_SIZE_Y);
  uiWindowSetDefaultSize (lyr->wcont [LYR_W_WINDOW], x, y);

  vbox = uiCreateVertBox ();
  uiWindowPackInWindow (lyr->wcont [LYR_W_WINDOW], vbox);
  uiWidgetSetAllMargins (vbox, 2);
  uiWidgetExpandHoriz (vbox);
  uiWidgetExpandVert (vbox);

  uiwidgetp = uiTextBoxCreate (100, NULL);
  uiTextBoxHorizExpand (uiwidgetp);
  uiTextBoxVertExpand (uiwidgetp);
  uiBoxPackStartExpand (vbox, uiwidgetp);
  lyr->wcont [LYR_W_TEXTBOX] = uiwidgetp;

  if (lyr->hideonstart) {
    uiWindowIconify (lyr->wcont [LYR_W_WINDOW]);
    lyr->isiconified = true;
  }

  uiWidgetShowAll (lyr->wcont [LYR_W_WINDOW]);

  lyrMoveWindow (lyr);

  progstateLogTime (lyr->progstate, "time-to-start-gui");

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_lyr", BDJ4_IMG_PNG_EXT, PATHBLD_MP_DIR_IMG);
  osuiSetIcon (imgbuff);

  uiwcontFree (vbox);

  lyr->uibuilt = true;

  logProcEnd ("");
}

static int
lyrMainLoop (void *tlyr)
{
  lyrics_t   *lyr = tlyr;
  int         stop = SOCKH_CONTINUE;

  uiUIProcessEvents ();

  if (! progstateIsRunning (lyr->progstate)) {
    progstateProcess (lyr->progstate);
    if (progstateCurrState (lyr->progstate) == PROGSTATE_CLOSED) {
      stop = SOCKH_STOP;
    }
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      progstateShutdownProcess (lyr->progstate);
    }
    return stop;
  }

  connProcessUnconnected (lyr->conn);

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (lyr->progstate);
  }
  return stop;
}

static bool
lyrConnectingCallback (void *udata, programstate_t programState)
{
  lyrics_t   *lyr = udata;
  bool        rc = STATE_NOT_FINISH;

  logProcBegin ();

  connProcessUnconnected (lyr->conn);

  if (! connIsConnected (lyr->conn, ROUTE_MAIN)) {
    connConnect (lyr->conn, ROUTE_MAIN);
  }
  if (! connIsConnected (lyr->conn, ROUTE_PLAYERUI)) {
    connConnect (lyr->conn, ROUTE_PLAYERUI);
  }

  if (connIsConnected (lyr->conn, ROUTE_MAIN) &&
      connIsConnected (lyr->conn, ROUTE_PLAYERUI)) {
    rc = STATE_FINISHED;
  }

  logProcEnd ("");
  return rc;
}

static bool
lyrHandshakeCallback (void *udata, programstate_t programState)
{
  lyrics_t   *lyr = udata;
  bool        rc = STATE_NOT_FINISH;

  logProcBegin ();

  connProcessUnconnected (lyr->conn);

  if (connHaveHandshake (lyr->conn, ROUTE_MAIN) &&
      connHaveHandshake (lyr->conn, ROUTE_PLAYERUI)) {
    rc = STATE_FINISHED;
  }

  logProcEnd ("");
  return rc;
}

static int
lyrProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  lyrics_t   *lyr = udata;

  logProcBegin ();

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_LYRICS: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (lyr->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          progstateShutdownProcess (lyr->progstate);
          break;
        }
        case MSG_WINDOW_SHOW: {
          if (lyr->isiconified) {
            uiWindowDeIconify (lyr->wcont [LYR_W_WINDOW]);
          }
          break;
        }
        case MSG_WINDOW_HIDE: {
          if (! lyr->isiconified) {
            uiWindowIconify (lyr->wcont [LYR_W_WINDOW]);
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
lyrCloseCallback (void *udata)
{
  lyrics_t   *lyr = udata;

  logProcBegin ();

  if (progstateCurrState (lyr->progstate) <= PROGSTATE_RUNNING) {
    lyrSaveWindowPosition (lyr);

    uiWindowIconify (lyr->wcont [LYR_W_WINDOW]);
    lyr->isiconified = true;
    logProcEnd ("user-close-win");
    return UICB_STOP;
  }

  logProcEnd ("");
  return UICB_CONT;
}


static void
lyrSaveWindowPosition (lyrics_t *lyr)
{
  int   x, y, ws;

  uiWindowGetPosition (lyr->wcont [LYR_W_WINDOW], &x, &y, &ws);
  /* on windows, when the window is iconified, the position cannot be */
  /* fetched like on linux; -32000 is returned for the position */
  if (x != -32000 && y != -32000 ) {
    nlistSetNum (lyr->options, LYR_POSITION_X, x);
    nlistSetNum (lyr->options, LYR_POSITION_Y, y);
    nlistSetNum (lyr->options, LYR_WORKSPACE, ws);
  }
}

static void
lyrMoveWindow (lyrics_t *lyr)
{
  int   x, y, ws;

  x = nlistGetNum (lyr->options, LYR_POSITION_X);
  y = nlistGetNum (lyr->options, LYR_POSITION_Y);
  ws = nlistGetNum (lyr->options, LYR_WORKSPACE);
  uiWindowMove (lyr->wcont [LYR_W_WINDOW], x, y, ws);
}

static void
lyrSigHandler (int sig)
{
  gKillReceived = 1;
}

