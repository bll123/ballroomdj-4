/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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
#include "uiutils.h"

enum {
  MQ_POSITION_X,
  MQ_POSITION_Y,
  MQ_WORKSPACE,
  MQ_SIZE_X,
  MQ_SIZE_Y,
  MQ_FONT_SZ,
  MQ_FONT_SZ_FS,
  MQ_KEY_MAX,
};

enum {
  MQ_CB_EXIT,
  MQ_CB_DBL_CLICK,
  MQ_CB_WINSTATE,
  MQ_CB_WINMAP,
  MQ_CB_MAX,
};

enum {
  MQ_W_WINDOW,
  MQ_W_PBAR,
  MQ_W_INFOBOX,
  MQ_W_SEP,
  MQ_W_COUNTDOWN_TIMER,
  MQ_W_INFO_SZGRP,
  MQ_W_INFO_DISP_A,
  MQ_W_INFO_DISP_B,
  MQ_W_INFO_DISP_C,
  MQ_W_INFO_DISP_D,
  MQ_W_INFO_DISP_E,
  MQ_W_INFO_DANCE,
  MQ_W_MAX,
};

/* sort by ascii values */
static datafilekey_t mqdfkeys [MQ_KEY_MAX] = {
  { "MQ_FONT_SZ",   MQ_FONT_SZ,       VALUE_NUM, NULL, DF_NORM },
  { "MQ_FONT_SZ_FS",MQ_FONT_SZ_FS,    VALUE_NUM, NULL, DF_NORM },
  { "MQ_POS_X",     MQ_POSITION_X,    VALUE_NUM, NULL, DF_NORM },
  { "MQ_POS_Y",     MQ_POSITION_Y,    VALUE_NUM, NULL, DF_NORM },
  { "MQ_SIZE_X",    MQ_SIZE_X,        VALUE_NUM, NULL, DF_NORM },
  { "MQ_SIZE_Y",    MQ_SIZE_Y,        VALUE_NUM, NULL, DF_NORM },
  { "MQ_WORKSPACE", MQ_WORKSPACE,     VALUE_NUM, NULL, DF_NORM },
};

typedef struct {
  progstate_t     *progstate;
  char            *locknm;
  conn_t          *conn;
  int             stopwaitcount;
  datafile_t      *optiondf;
  nlist_t         *options;
  uiwcont_t       *window;
  callback_t      *callbacks [MQ_CB_MAX];
  uiwcont_t       *wcont [MQ_W_MAX];
  uiwcont_t       **marqueeLabs;   // array of uiwcont_t *
  int             marginTotal;
  double          fontAdjustment;
  int             mqLen;
  int             lastHeight;
  int             priorSize;
  int             unMaximize;
  bool            isMaximized : 1;
  bool            isIconified : 1;
  bool            userDoubleClicked : 1;
  bool            mqIconifyAction : 1;
  bool            setPrior : 1;
  bool            mqShowInfo : 1;
  bool            hideonstart : 1;
  bool            optionsalloc : 1;
  bool            uibuilt : 1;
} marquee_t;

enum {
  MARQUEE_UNMAX_WAIT_COUNT = 3,
};
#define INFO_LAB_HEIGHT_ADJUST  0.85
static const char * const MQ_TEXT_CLASS = "mqtext";
static const char * const MQ_INFO_CLASS = "mqinfo";

static bool marqueeConnectingCallback (void *udata, programstate_t programState);
static bool marqueeHandshakeCallback (void *udata, programstate_t programState);
static bool marqueeStoppingCallback (void *udata, programstate_t programState);
static bool marqueeStopWaitCallback (void *udata, programstate_t programState);
static bool marqueeClosingCallback (void *udata, programstate_t programState);
static void marqueeBuildUI (marquee_t *marquee);
static int  marqueeMainLoop  (void *tmarquee);
static int  marqueeProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                bdjmsgmsg_t msg, char *args, void *udata);
static bool marqueeCloseCallback (void *udata);
static bool marqueeToggleFullscreen (void *udata);
static void marqueeSetMaximized (marquee_t *marquee);
static void marqueeSetNotMaximized (marquee_t *marquee);
static void marqueeSetNotMaximizeFinish (marquee_t *marquee);
static void marqueeSendMaximizeState (marquee_t *marquee);
static bool marqueeWinState (void *udata, int isicon, int ismax);
static bool marqueeWinMapped (void *udata);
static void marqueeSaveWindowPosition (marquee_t *);
static void marqueeMoveWindow (marquee_t *);
static void marqueeSigHandler (int sig);
static void marqueeSetFontSize (marquee_t *marquee, uiwcont_t *lab, const char *font);
static void marqueePopulate (marquee_t *marquee, char *args);
static void marqueeSetTimer (marquee_t *marquee, char *args);
static void marqueeSetFont (marquee_t *marquee, int sz);
static void marqueeRecover (marquee_t *marquee);
static void marqueeDisplayCompletion (marquee_t *marquee);
static void marqueeSendFontSizes (marquee_t *marquee);
static void marqueeClearInfoDisplay (marquee_t *marquee);

static int gKillReceived = 0;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  marquee_t       marquee;
  char            tbuff [MAXPATHLEN];
  uint32_t        flags;

#if BDJ4_MEM_DEBUG
  mdebugInit ("mq");
#endif

  marquee.progstate = progstateInit ("marquee");
  progstateSetCallback (marquee.progstate, STATE_CONNECTING,
      marqueeConnectingCallback, &marquee);
  progstateSetCallback (marquee.progstate, STATE_WAIT_HANDSHAKE,
      marqueeHandshakeCallback, &marquee);
  progstateSetCallback (marquee.progstate, STATE_STOPPING,
      marqueeStoppingCallback, &marquee);
  progstateSetCallback (marquee.progstate, STATE_STOP_WAIT,
      marqueeStopWaitCallback, &marquee);
  progstateSetCallback (marquee.progstate, STATE_CLOSING,
      marqueeClosingCallback, &marquee);

  for (int i = 0; i < MQ_W_MAX; ++i) {
    marquee.wcont [i] = NULL;
  }
  marquee.marqueeLabs = NULL;
  marquee.lastHeight = 0;
  marquee.priorSize = 0;
  marquee.isMaximized = false;
  marquee.unMaximize = 0;
  marquee.isIconified = false;
  marquee.userDoubleClicked = false;
  marquee.mqIconifyAction = false;
  marquee.setPrior = false;
  marquee.uibuilt = false;
  marquee.marginTotal = 0;
  marquee.fontAdjustment = 0.0;
  marquee.hideonstart = false;
  marquee.stopwaitcount = 0;
  for (int i = 0; i < MQ_CB_MAX; ++i) {
    marquee.callbacks [i] = NULL;
  }

  osSetStandardSignals (marqueeSigHandler);

  flags = BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  bdj4startup (argc, argv, NULL, "mq", ROUTE_MARQUEE, &flags);
  if (bdjoptGetNum (OPT_P_MARQUEE_SHOW) == MARQUEE_SHOW_MINIMIZE ||
      ((flags & BDJ4_INIT_HIDE_MARQUEE) == BDJ4_INIT_HIDE_MARQUEE)) {
    marquee.hideonstart = true;
  }

  listenPort = bdjvarsGetNum (BDJVL_PORT_MARQUEE);
  marquee.mqLen = bdjoptGetNum (OPT_P_MQQLEN);
  marquee.mqShowInfo = bdjoptGetNum (OPT_P_MQ_SHOW_INFO);
  marquee.conn = connInit (ROUTE_MARQUEE);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "marquee", BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  marquee.optiondf = datafileAllocParse ("ui-marquee", DFTYPE_KEY_VAL, tbuff,
      mqdfkeys, MQ_KEY_MAX, DF_NO_OFFSET, NULL);
  marquee.options = datafileGetList (marquee.optiondf);
  marquee.optionsalloc = false;
  if (marquee.options == NULL || nlistGetCount (marquee.options) == 0) {
    marquee.optionsalloc = true;
    marquee.options = nlistAlloc ("ui-marquee", LIST_ORDERED, NULL);

    nlistSetNum (marquee.options, MQ_WORKSPACE, -1);
    nlistSetNum (marquee.options, MQ_POSITION_X, -1);
    nlistSetNum (marquee.options, MQ_POSITION_Y, -1);
    nlistSetNum (marquee.options, MQ_SIZE_X, 600);
    nlistSetNum (marquee.options, MQ_SIZE_Y, -1);
    nlistSetNum (marquee.options, MQ_FONT_SZ, 36);
    nlistSetNum (marquee.options, MQ_FONT_SZ_FS, 60);
  }

  uiUIInitialize (sysvarsGetNum (SVL_LOCALE_DIR));
  uiSetUICSS (uiutilsGetCurrentFont (),
      uiutilsGetListingFont (),
      bdjoptGetStr (OPT_P_UI_ACCENT_COL),
      bdjoptGetStr (OPT_P_UI_ERROR_COL),
      bdjoptGetStr (OPT_P_UI_MARK_COL),
      bdjoptGetStr (OPT_P_UI_ROWSEL_COL),
      bdjoptGetStr (OPT_P_UI_ROW_HL_COL));

  marqueeBuildUI (&marquee);
  osuiFinalize ();

  sockhMainLoop (listenPort, marqueeProcessMsg, marqueeMainLoop, &marquee);
  connFree (marquee.conn);
  progstateFree (marquee.progstate);
  for (int i = 0; i < MQ_CB_MAX; ++i) {
    callbackFree (marquee.callbacks [i]);
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
marqueeStoppingCallback (void *udata, programstate_t programState)
{
  marquee_t     *marquee = udata;
  int           x, y;

  logProcBegin ();

  if (marquee->isMaximized) {
    marqueeSetNotMaximized (marquee);
    logProcEnd ("is-maximized-a");
    return STATE_NOT_FINISH;
  }

  if (uiWindowIsMaximized (marquee->wcont [MQ_W_WINDOW])) {
    logProcEnd ("is-maximized-b");
    return STATE_NOT_FINISH;
  }

  uiWindowGetSize (marquee->wcont [MQ_W_WINDOW], &x, &y);
  nlistSetNum (marquee->options, MQ_SIZE_X, x);
  nlistSetNum (marquee->options, MQ_SIZE_Y, y);
  if (! marquee->isIconified) {
    marqueeSaveWindowPosition (marquee);
  }

  connDisconnectAll (marquee->conn);
  logProcEnd ("");
  return STATE_FINISHED;
}

static bool
marqueeStopWaitCallback (void *tmarquee, programstate_t programState)
{
  marquee_t   *marquee = tmarquee;
  bool        rc;

  rc = connWaitClosed (marquee->conn, &marquee->stopwaitcount);
  return rc;
}

static bool
marqueeClosingCallback (void *udata, programstate_t programState)
{
  marquee_t   *marquee = udata;

  logProcBegin ();

  /* these are moved here so that the window can be un-maximized and */
  /* the size/position saved */
  uiCloseWindow (marquee->wcont [MQ_W_WINDOW]);
  uiCleanup ();

  datafileSave (marquee->optiondf, NULL, marquee->options, DF_NO_OFFSET, 1);

  bdj4shutdown (ROUTE_MARQUEE, NULL);

  for (int i = 0; i < MQ_W_MAX; ++i) {
    uiwcontFree (marquee->wcont [i]);
  }
  for (int i = 0; i < marquee->mqLen; ++i) {
    uiwcontFree (marquee->marqueeLabs [i]);
  }
  dataFree (marquee->marqueeLabs);

  if (marquee->optionsalloc) {
    nlistFree (marquee->options);
  }
  datafileFree (marquee->optiondf);

  logProcEnd ("");
  return STATE_FINISHED;
}

static void
marqueeBuildUI (marquee_t *marquee)
{
  char        imgbuff [MAXPATHLEN];
  uiwcont_t   *uiwidgetp;
  uiwcont_t   *mainvbox;
  uiwcont_t   *hbox;
  uiwcont_t   *vbox;
  int         x, y;

  logProcBegin ();

  uiLabelAddClass (MQ_ACCENT_CLASS, bdjoptGetStr (OPT_P_MQ_ACCENT_COL));
  uiLabelAddClass (MQ_TEXT_CLASS, bdjoptGetStr (OPT_P_MQ_TEXT_COL));
  uiLabelAddClass (MQ_INFO_CLASS, bdjoptGetStr (OPT_P_MQ_INFO_COL));
  uiSeparatorAddClass (MQ_ACCENT_CLASS, bdjoptGetStr (OPT_P_MQ_ACCENT_COL));

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_marquee", BDJ4_IMG_SVG_EXT, PATHBLD_MP_DIR_IMG);

  marquee->callbacks [MQ_CB_EXIT] = callbackInit (
      marqueeCloseCallback, marquee, NULL);
  marquee->wcont [MQ_W_WINDOW] = uiCreateMainWindow (marquee->callbacks [MQ_CB_EXIT],
      /* CONTEXT: marquee: marquee window title (suggested: song display) */
      _("Marquee"), imgbuff);
  uiWindowNoFocusOnStartup (marquee->wcont [MQ_W_WINDOW]);

  marquee->callbacks [MQ_CB_DBL_CLICK] = callbackInit (
      marqueeToggleFullscreen, marquee, NULL);
  uiWindowSetDoubleClickCallback (marquee->wcont [MQ_W_WINDOW], marquee->callbacks [MQ_CB_DBL_CLICK]);

  marquee->callbacks [MQ_CB_WINSTATE] = callbackInitII (
      marqueeWinState, marquee);
  uiWindowSetWinStateCallback (marquee->wcont [MQ_W_WINDOW], marquee->callbacks [MQ_CB_WINSTATE]);

  marquee->callbacks [MQ_CB_WINMAP] = callbackInit (marqueeWinMapped, marquee, NULL);
  uiWindowSetMappedCallback (marquee->wcont [MQ_W_WINDOW], marquee->callbacks [MQ_CB_WINMAP]);

  uiWindowNoDim (marquee->wcont [MQ_W_WINDOW]);

  x = nlistGetNum (marquee->options, MQ_SIZE_X);
  y = nlistGetNum (marquee->options, MQ_SIZE_Y);
  uiWindowSetDefaultSize (marquee->wcont [MQ_W_WINDOW], x, y);

  mainvbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (mainvbox, 10);
  uiWindowPackInWindow (marquee->wcont [MQ_W_WINDOW], mainvbox);
  uiWidgetExpandHoriz (mainvbox);
  uiWidgetExpandVert (mainvbox);
  marquee->marginTotal = 20;

  marquee->wcont [MQ_W_PBAR] = uiCreateProgressBar ();
  uiAddProgressbarClass (MQ_ACCENT_CLASS, bdjoptGetStr (OPT_P_MQ_ACCENT_COL));
  uiWidgetAddClass (marquee->wcont [MQ_W_PBAR], MQ_ACCENT_CLASS);
  uiBoxPackStart (mainvbox, marquee->wcont [MQ_W_PBAR]);

  vbox = uiCreateVertBox ();
  uiWidgetExpandHoriz (vbox);
  uiBoxPackStart (mainvbox, vbox);

  hbox = uiCreateHorizBox ();
  uiWidgetAlignHorizFill (hbox);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: marquee: displayed when nothing is set to be played */
  uiwidgetp = uiCreateLabel (_("Not Playing"));
  uiWidgetAlignHorizStart (uiwidgetp);
  uiWidgetDisableFocus (uiwidgetp);
  uiWidgetAddClass (uiwidgetp, MQ_ACCENT_CLASS);
  uiBoxPackStart (hbox, uiwidgetp);
  marquee->wcont [MQ_W_INFO_DANCE] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("0:00");
  uiLabelSetMaxWidth (uiwidgetp, 6);
  uiWidgetAlignHorizEnd (uiwidgetp);
  uiWidgetDisableFocus (uiwidgetp);
  uiWidgetAddClass (uiwidgetp, MQ_ACCENT_CLASS);
  uiBoxPackEnd (hbox, uiwidgetp);
  marquee->wcont [MQ_W_COUNTDOWN_TIMER] = uiwidgetp;

  uiwcontFree (hbox);

  /* info line */

  hbox = uiCreateHorizBox ();
  uiWidgetAlignHorizFill (hbox);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);
  marquee->wcont [MQ_W_INFOBOX] = hbox;

  for (int i = MQ_W_INFO_DISP_A; i <= MQ_W_INFO_DISP_E; ++i) {
    uiwidgetp = uiCreateLabel ("");
    uiWidgetAlignHorizStart (uiwidgetp);
    uiWidgetAlignVertBaseline (uiwidgetp);
    uiWidgetDisableFocus (uiwidgetp);
    if ((i - MQ_W_INFO_DISP_A) % 2 == 0) {
      uiLabelEllipsizeOn (uiwidgetp);
    }
    uiBoxPackStart (hbox, uiwidgetp);
    marquee->wcont [i] = uiwidgetp;
    uiWidgetAddClass (marquee->wcont [i], MQ_INFO_CLASS);
  }

  marquee->wcont [MQ_W_SEP] = uiCreateHorizSeparator ();
  uiWidgetAddClass (marquee->wcont [MQ_W_SEP], MQ_ACCENT_CLASS);
  uiWidgetExpandHoriz (marquee->wcont [MQ_W_SEP]);
  uiWidgetSetMarginTop (marquee->wcont [MQ_W_SEP], 2);
  uiWidgetSetMarginBottom (marquee->wcont [MQ_W_SEP], 4);
  uiBoxPackEnd (vbox, marquee->wcont [MQ_W_SEP]);

  marquee->marqueeLabs = mdmalloc (sizeof (uiwcont_t *) * marquee->mqLen);

  for (int i = 0; i < marquee->mqLen; ++i) {
    marquee->marqueeLabs [i] = uiCreateLabel ("");
    uiWidgetAlignHorizStart (marquee->marqueeLabs [i]);
    uiWidgetExpandHoriz (marquee->marqueeLabs [i]);
    uiWidgetDisableFocus (marquee->marqueeLabs [i]);
    uiWidgetSetMarginTop (marquee->marqueeLabs [i], 4);
    uiBoxPackStart (mainvbox, marquee->marqueeLabs [i]);
  }

  marqueeSetFont (marquee, nlistGetNum (marquee->options, MQ_FONT_SZ));

  if (marquee->hideonstart) {
    uiWindowIconify (marquee->wcont [MQ_W_WINDOW]);
    marquee->isIconified = true;
  }

  if (! marquee->mqShowInfo) {
    uiWidgetHide (marquee->wcont [MQ_W_INFOBOX]);
  }
  uiWidgetShowAll (marquee->wcont [MQ_W_WINDOW]);

  marqueeMoveWindow (marquee);

  progstateLogTime (marquee->progstate, "time-to-start-gui");

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_marquee", BDJ4_IMG_PNG_EXT, PATHBLD_MP_DIR_IMG);
  osuiSetIcon (imgbuff);

  /* do not free the infobox hbox */
  uiwcontFree (mainvbox);
  uiwcontFree (vbox);

  marquee->uibuilt = true;

  logProcEnd ("");
}

static int
marqueeMainLoop (void *tmarquee)
{
  marquee_t   *marquee = tmarquee;
  int         stop = SOCKH_CONTINUE;

  uiUIProcessEvents ();

  if (! progstateIsRunning (marquee->progstate)) {
    progstateProcess (marquee->progstate);
    if (progstateCurrState (marquee->progstate) == STATE_CLOSED) {
      stop = SOCKH_STOP;
    }
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      progstateShutdownProcess (marquee->progstate);
    }
    return stop;
  }

  connProcessUnconnected (marquee->conn);

  if (marquee->unMaximize == 1) {
    marqueeSetNotMaximizeFinish (marquee);
    marquee->unMaximize = 0;
  }
  if (marquee->unMaximize > 0) {
    --marquee->unMaximize;
  }

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (marquee->progstate);
  }
  return stop;
}

static bool
marqueeConnectingCallback (void *udata, programstate_t programState)
{
  marquee_t   *marquee = udata;
  bool        rc = STATE_NOT_FINISH;

  logProcBegin ();

  connProcessUnconnected (marquee->conn);

  if (! connIsConnected (marquee->conn, ROUTE_MAIN)) {
    connConnect (marquee->conn, ROUTE_MAIN);
  }
  if (! connIsConnected (marquee->conn, ROUTE_PLAYERUI)) {
    connConnect (marquee->conn, ROUTE_PLAYERUI);
  }

  if (connIsConnected (marquee->conn, ROUTE_MAIN) &&
      connIsConnected (marquee->conn, ROUTE_PLAYERUI)) {
    rc = STATE_FINISHED;
  }

  logProcEnd ("");
  return rc;
}

static bool
marqueeHandshakeCallback (void *udata, programstate_t programState)
{
  marquee_t   *marquee = udata;
  bool        rc = STATE_NOT_FINISH;

  logProcBegin ();

  connProcessUnconnected (marquee->conn);

  if (connHaveHandshake (marquee->conn, ROUTE_MAIN) &&
      connHaveHandshake (marquee->conn, ROUTE_PLAYERUI)) {
    marqueeSendFontSizes (marquee);
    rc = STATE_FINISHED;
  }

  logProcEnd ("");
  return rc;
}

static int
marqueeProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  marquee_t   *marquee = udata;
  char        *targs = NULL;

  logProcBegin ();
  if (args != NULL) {
    targs = mdstrdup (args);
  }

  if (msg != MSG_MARQUEE_TIMER) {
    logMsg (LOG_DBG, LOG_MSGS, "rcvd: from:%d/%s route:%d/%s msg:%d/%s args:%s",
        routefrom, msgRouteDebugText (routefrom),
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
  }

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MARQUEE: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (marquee->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          progstateShutdownProcess (marquee->progstate);
          break;
        }
        case MSG_MARQUEE_DATA: {
          marqueePopulate (marquee, targs);
          break;
        }
        case MSG_MARQUEE_TIMER: {
          marqueeSetTimer (marquee, targs);
          break;
        }
        case MSG_MARQUEE_SET_FONT_SZ: {
          marqueeSetFont (marquee, atoi (targs));
          break;
        }
        case MSG_WINDOW_FIND: {
          marqueeRecover (marquee);
          break;
        }
        case MSG_FINISHED: {
          marqueeDisplayCompletion (marquee);
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

  dataFree (targs);

  logProcEnd ("");

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  logProcEnd ("");
  return gKillReceived;
}


static bool
marqueeCloseCallback (void *udata)
{
  marquee_t   *marquee = udata;

  logProcBegin ();

  if (progstateCurrState (marquee->progstate) <= STATE_RUNNING) {
    if (! marquee->isMaximized && ! marquee->isIconified) {
      marqueeSaveWindowPosition (marquee);
    }

    marquee->mqIconifyAction = true;
    uiWindowIconify (marquee->wcont [MQ_W_WINDOW]);
    marquee->isIconified = true;
    logProcEnd ("user-close-win");
    return UICB_STOP;
  }

  logProcEnd ("");
  return UICB_CONT;
}

static bool
marqueeToggleFullscreen (void *udata)
{
  marquee_t   *marquee = udata;

  marquee->userDoubleClicked = true;
  if (marquee->isMaximized) {
    marqueeSetNotMaximized (marquee);
  } else {
    marqueeSetMaximized (marquee);
  }

  return UICB_CONT;
}

static void
marqueeSetMaximized (marquee_t *marquee)
{
  if (marquee->isMaximized) {
    return;
  }

  if (! marquee->uibuilt) {
    return;
  }

  marquee->isMaximized = true;
  if (! isWindows()) {
    /* decorations are not recovered after disabling on windows */
    uiWindowDisableDecorations (marquee->wcont [MQ_W_WINDOW]);
  }
  uiWindowMaximize (marquee->wcont [MQ_W_WINDOW]);
  marqueeSetFont (marquee, nlistGetNum (marquee->options, MQ_FONT_SZ_FS));
  marqueeSendMaximizeState (marquee);
}

static void
marqueeSetNotMaximized (marquee_t *marquee)
{
  logProcBegin ();

  if (! marquee->isMaximized) {
    logProcEnd ("not-max");
    return;
  }

  marquee->isMaximized = false;
  marqueeSetFont (marquee, nlistGetNum (marquee->options, MQ_FONT_SZ));
  marquee->unMaximize = MARQUEE_UNMAX_WAIT_COUNT;
  logProcEnd ("");
}

static void
marqueeSetNotMaximizeFinish (marquee_t *marquee)
{
  if (! marquee->uibuilt) {
    return;
  }

  marquee->setPrior = true;
  uiWindowUnMaximize (marquee->wcont [MQ_W_WINDOW]);
  if (! isWindows()) {
    /* does not work on windows platforms */
    uiWindowEnableDecorations (marquee->wcont [MQ_W_WINDOW]);
  }
  marqueeSendMaximizeState (marquee);
  logProcEnd ("");
}

static void
marqueeSendMaximizeState (marquee_t *marquee)
{
  char        tbuff [40];

  snprintf (tbuff, sizeof (tbuff), "%d", marquee->isMaximized);
  connSendMessage (marquee->conn, ROUTE_PLAYERUI, MSG_MARQUEE_IS_MAX, tbuff);
}

static bool
marqueeWinState (void *udata, int isIconified, int isMaximized)
{
  marquee_t         *marquee = udata;

  logProcBegin ();

  if (isIconified >= 0) {
    if (marquee->mqIconifyAction) {
      marquee->mqIconifyAction = false;
      logProcEnd ("close-button");
      return UICB_CONT;
    }

    if (isIconified) {
      marqueeSaveWindowPosition (marquee);
      marquee->isIconified = true;
    } else {
      marquee->isIconified = false;
//      marqueeMoveWindow (marquee);
    }
    logProcEnd ("iconified/deiconified");
    return UICB_CONT;
  }

  if (isMaximized >= 0) {
    /* if the user double-clicked, this is a known maximize change and */
    /* no processing needs to be done here */
    if (marquee->userDoubleClicked) {
      marquee->userDoubleClicked = false;
      logProcEnd ("user-double-clicked");
      return UICB_CONT;
    }

    /* user selected the maximize button */
    if (isMaximized) {
      marqueeSetMaximized (marquee);
    }
  }

  logProcEnd ("");
  return UICB_CONT;
}

static bool
marqueeWinMapped (void *udata)
{
  marquee_t         *marquee = udata;

  logProcBegin ();

// is this process needed?
// need to check on windows and mac os.
  if (! marquee->isMaximized && ! marquee->isIconified) {
//    marqueeMoveWindow (marquee);
  }

  logProcEnd ("");
  return UICB_CONT;
}

static void
marqueeSaveWindowPosition (marquee_t *marquee)
{
  int   x, y, ws;

  uiWindowGetPosition (marquee->wcont [MQ_W_WINDOW], &x, &y, &ws);
  /* on windows, when the window is iconified, the position cannot be */
  /* fetched like on linux; -32000 is returned for the position */
  if (x != -32000 && y != -32000 ) {
    nlistSetNum (marquee->options, MQ_POSITION_X, x);
    nlistSetNum (marquee->options, MQ_POSITION_Y, y);
    nlistSetNum (marquee->options, MQ_WORKSPACE, ws);
  }
}

static void
marqueeMoveWindow (marquee_t *marquee)
{
  int   x, y, ws;

  x = nlistGetNum (marquee->options, MQ_POSITION_X);
  y = nlistGetNum (marquee->options, MQ_POSITION_Y);
  ws = nlistGetNum (marquee->options, MQ_WORKSPACE);
  uiWindowMove (marquee->wcont [MQ_W_WINDOW], x, y, ws);
}

static void
marqueeSigHandler (int sig)
{
  gKillReceived = 1;
}

static void
marqueeSetFontSize (marquee_t *marquee, uiwcont_t *uilab, const char *font)
{
  logProcBegin ();

  if (uilab == NULL) {
    logProcEnd ("no-lab");
    return;
  }

  uiLabelSetFont (uilab, font);
  logProcEnd ("");
}

static void
marqueePopulate (marquee_t *marquee, char *args)
{
  char        *p;
  char        *tokptr;
  int         idx;
  const char  *sep = "";
  char        sepstr [20] = { " " };

  logProcBegin ();

  if (! marquee->uibuilt) {
    logProcEnd ("no-ui");
    return;
  }

  if (! marquee->mqShowInfo) {
    uiWidgetHide (marquee->wcont [MQ_W_INFOBOX]);
  }

  marqueeClearInfoDisplay (marquee);

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokptr);

  /* first entry is the main dance */
  if (p == NULL || *p == MSG_ARGS_EMPTY) {
    p = "";
  }
  uiLabelSetText (marquee->wcont [MQ_W_INFO_DANCE], p);

  for (int i = 0; i < marquee->mqLen; ++i) {
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
    if (p == NULL || *p == MSG_ARGS_EMPTY) {
      p = "";
    }
    uiLabelSetText (marquee->marqueeLabs [i], p);
  }

  idx = MQ_W_INFO_DISP_A;

  /* the pointer was left pointing at the last dance display */
  /* grab the next item */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  while (p != NULL && idx <= MQ_W_INFO_DISP_E) {
    if (*p == MSG_ARGS_EMPTY) {
      /* no data */
      p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
      continue;
    }

    if (*sep) {
      uiLabelSetText (marquee->wcont [idx], sepstr);
      ++idx;
    }

    if (idx > MQ_W_INFO_DISP_E) {
      break;
    }

    uiLabelSetText (marquee->wcont [idx], p);
    if (! *sep) {
      sep = bdjoptGetStr (OPT_P_MQ_INFO_SEP);
      if (sep != NULL) {
        snprintf (sepstr, sizeof (sepstr), " %s ", sep);
      }
    }

    ++idx;
    if (idx > MQ_W_INFO_DISP_E) {
      break;
    }

    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  }

  logProcEnd ("");
}

static void
marqueeSetTimer (marquee_t *marquee, char *args)
{
  ssize_t     played;
  ssize_t     dur;
  double      dplayed;
  double      ddur;
  double      dratio;
  ssize_t     timeleft;
  char        *p = NULL;
  char        *tokptr = NULL;
  char        tbuff [40];

  logProcBegin ();


  p = strtok_r (args, MSG_ARGS_RS_STR, &tokptr);
  played = atol (p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  dur = atol (p);

  timeleft = dur - played;
  tmutilToMS (timeleft, tbuff, sizeof (tbuff));
  uiLabelSetText (marquee->wcont [MQ_W_COUNTDOWN_TIMER], tbuff);

  ddur = (double) dur;
  dplayed = (double) played;
  if (dur != 0.0) {
    dratio = dplayed / ddur;
  } else {
    dratio = 0.0;
  }
  uiProgressBarSet (marquee->wcont [MQ_W_PBAR], dratio);
  logProcEnd ("");
}

static void
marqueeSetFont (marquee_t *marquee, int sz)
{
  const char  *f;
  char        newfont [200];

  logProcBegin ();

  f = bdjoptGetStr (OPT_MP_MQFONT);
  if (f == NULL) {
    f = bdjoptGetStr (OPT_MP_UIFONT);
  }
  if (f == NULL) {
    f = "";
  }
  uiutilsNewFontSize (newfont, sizeof (newfont), f, "bold", sz);

  if (marquee->isMaximized) {
    nlistSetNum (marquee->options, MQ_FONT_SZ_FS, sz);
  } else {
    nlistSetNum (marquee->options, MQ_FONT_SZ, sz);
  }

  marqueeSetFontSize (marquee, marquee->wcont [MQ_W_INFO_DANCE], newfont);
  marqueeSetFontSize (marquee, marquee->wcont [MQ_W_COUNTDOWN_TIMER], newfont);

  /* not bold */
  uiutilsNewFontSize (newfont, sizeof (newfont), f, NULL, sz);
  for (int i = 0; i < marquee->mqLen; ++i) {
    marqueeSetFontSize (marquee, marquee->marqueeLabs [i], newfont);
    uiWidgetAddClass (marquee->marqueeLabs [i], MQ_TEXT_CLASS);
  }

  sz = (int) round ((double) sz * 0.7);
  uiutilsNewFontSize (newfont, sizeof (newfont), f, NULL, sz);
  for (int i = MQ_W_INFO_DISP_A; i <= MQ_W_INFO_DISP_E; ++i) {
    marqueeSetFontSize (marquee, marquee->wcont [i], newfont);
  }

  logProcEnd ("");
}

static void
marqueeRecover (marquee_t *marquee)
{
  /* on linux XFCE, this will position the window at this location in */
  /* its current workspace */
  nlistSetNum (marquee->options, MQ_POSITION_X, 200);
  nlistSetNum (marquee->options, MQ_POSITION_Y, 200);

  if (marquee->isIconified) {
    uiWindowDeIconify (marquee->wcont [MQ_W_WINDOW]);
  }
  nlistSetNum (marquee->options, MQ_FONT_SZ, 36);
  nlistSetNum (marquee->options, MQ_FONT_SZ_FS, 60);
  nlistSetNum (marquee->options, MQ_SIZE_X, 600);
  nlistSetNum (marquee->options, MQ_SIZE_Y, -1);
  uiWindowSetDefaultSize (marquee->wcont [MQ_W_WINDOW], 600, -1);
  marqueeSetNotMaximized (marquee);
  marqueeMoveWindow (marquee);
  uiWindowFind (marquee->wcont [MQ_W_WINDOW]);
  uiWindowPresent (marquee->wcont [MQ_W_WINDOW]);
  marqueeSaveWindowPosition (marquee);
  marqueeSendFontSizes (marquee);
}

static void
marqueeDisplayCompletion (marquee_t *marquee)
{
  const char  *disp;

  if (! marquee->uibuilt) {
    return;
  }

  uiLabelSetText (marquee->wcont [MQ_W_INFO_DANCE], "");
  marqueeClearInfoDisplay (marquee);
  disp = bdjoptGetStr (OPT_P_COMPLETE_MSG);
  uiLabelSetText (marquee->wcont [MQ_W_INFO_DISP_A], disp);

  if (! marquee->mqShowInfo) {
    uiWidgetShowAll (marquee->wcont [MQ_W_INFOBOX]);
  }
}

static void
marqueeSendFontSizes (marquee_t *marquee)
{
  char    tbuff [100];

  snprintf (tbuff, sizeof (tbuff), "%" PRId64 "%c%" PRId64,
      nlistGetNum (marquee->options, MQ_FONT_SZ),
      MSG_ARGS_RS,
      nlistGetNum (marquee->options, MQ_FONT_SZ_FS));
  connSendMessage (marquee->conn, ROUTE_PLAYERUI,
      MSG_MARQUEE_FONT_SIZES, tbuff);
}

static void
marqueeClearInfoDisplay (marquee_t *marquee)
{
  for (int i = MQ_W_INFO_DISP_A; i <= MQ_W_INFO_DISP_E; ++i) {
    uiLabelSetText (marquee->wcont [i], "");
  }
}
