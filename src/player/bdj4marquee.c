/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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
#include <ctype.h>

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
#include "tmutil.h"
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
  MQ_W_INFO_ARTIST,
  MQ_W_INFO_SEP,
  MQ_W_INFO_TITLE,
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
} marquee_t;

enum {
  MARQUEE_UNMAX_WAIT_COUNT = 3,
};
#define INFO_LAB_HEIGHT_ADJUST  0.85
#define MQ_TEXT_CLASS           "mqtext"
#define MQ_INFO_CLASS           "mqinfo"

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

static int gKillReceived = 0;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  marquee_t       marquee;
  char            tbuff [MAXPATHLEN];
  long            flags;

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

  listenPort = bdjvarsGetNum (BDJVL_MARQUEE_PORT);
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
      bdjoptGetStr (OPT_P_UI_ACCENT_COL),
      bdjoptGetStr (OPT_P_UI_ERROR_COL));

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

  logProcBegin (LOG_PROC, "marqueeStoppingCallback");

  if (marquee->isMaximized) {
    marqueeSetNotMaximized (marquee);
    logProcEnd (LOG_PROC, "marqueeStoppingCallback", "is-maximized-a");
    return STATE_NOT_FINISH;
  }

  if (uiWindowIsMaximized (marquee->wcont [MQ_W_WINDOW])) {
    logProcEnd (LOG_PROC, "marqueeStoppingCallback", "is-maximized-b");
    return STATE_NOT_FINISH;
  }

  uiWindowGetSize (marquee->wcont [MQ_W_WINDOW], &x, &y);
  nlistSetNum (marquee->options, MQ_SIZE_X, x);
  nlistSetNum (marquee->options, MQ_SIZE_Y, y);
  if (! marquee->isIconified) {
    marqueeSaveWindowPosition (marquee);
  }

  connDisconnectAll (marquee->conn);
  logProcEnd (LOG_PROC, "marqueeStoppingCallback", "");
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

  logProcBegin (LOG_PROC, "marqueeClosingCallback");

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

  logProcEnd (LOG_PROC, "marqueeClosingCallback", "");
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

  logProcBegin (LOG_PROC, "marqueeBuildUI");

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

  marquee->callbacks [MQ_CB_WINSTATE] = callbackInitIntInt (
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
  uiBoxPackInWindow (marquee->wcont [MQ_W_WINDOW], mainvbox);
  uiWidgetExpandHoriz (mainvbox);
  uiWidgetExpandVert (mainvbox);
  marquee->marginTotal = 20;

  marquee->wcont [MQ_W_PBAR] = uiCreateProgressBar ();
  uiAddProgressbarClass (MQ_ACCENT_CLASS, bdjoptGetStr (OPT_P_MQ_ACCENT_COL));
  uiWidgetSetClass (marquee->wcont [MQ_W_PBAR], MQ_ACCENT_CLASS);
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
  uiWidgetSetClass (uiwidgetp, MQ_ACCENT_CLASS);
  uiBoxPackStart (hbox, uiwidgetp);
  marquee->wcont [MQ_W_INFO_DANCE] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("0:00");
  uiLabelSetMaxWidth (uiwidgetp, 6);
  uiWidgetAlignHorizEnd (uiwidgetp);
  uiWidgetDisableFocus (uiwidgetp);
  uiWidgetSetClass (uiwidgetp, MQ_ACCENT_CLASS);
  uiBoxPackEnd (hbox, uiwidgetp);
  marquee->wcont [MQ_W_COUNTDOWN_TIMER] = uiwidgetp;

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiWidgetAlignHorizFill (hbox);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);
  marquee->wcont [MQ_W_INFOBOX] = hbox;

  uiwidgetp = uiCreateLabel ("");
  uiWidgetAlignHorizStart (uiwidgetp);
  uiWidgetDisableFocus (uiwidgetp);
  uiLabelEllipsizeOn (uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  marquee->wcont [MQ_W_INFO_ARTIST] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("");
  uiWidgetAlignHorizStart (uiwidgetp);
  uiWidgetDisableFocus (uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 2);
  uiWidgetSetMarginEnd (uiwidgetp, 2);
  marquee->wcont [MQ_W_INFO_SEP] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("");
  uiWidgetAlignHorizStart (uiwidgetp);
  uiWidgetDisableFocus (uiwidgetp);
  uiLabelEllipsizeOn (uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  marquee->wcont [MQ_W_INFO_TITLE] = uiwidgetp;

  marquee->wcont [MQ_W_SEP] = uiCreateHorizSeparator ();
  uiWidgetSetClass (marquee->wcont [MQ_W_SEP], MQ_ACCENT_CLASS);
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

  logProcEnd (LOG_PROC, "marqueeBuildUI", "");
}

static int
marqueeMainLoop (void *tmarquee)
{
  marquee_t   *marquee = tmarquee;
  int         stop = false;

  if (! stop) {
    uiUIProcessEvents ();
  }

  if (! progstateIsRunning (marquee->progstate)) {
    progstateProcess (marquee->progstate);
    if (progstateCurrState (marquee->progstate) == STATE_CLOSED) {
      stop = true;
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

  logProcBegin (LOG_PROC, "marqueeConnectingCallback");

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

  logProcEnd (LOG_PROC, "marqueeConnectingCallback", "");
  return rc;
}

static bool
marqueeHandshakeCallback (void *udata, programstate_t programState)
{
  marquee_t   *marquee = udata;
  bool        rc = STATE_NOT_FINISH;

  logProcBegin (LOG_PROC, "marqueeHandshakeCallback");

  connProcessUnconnected (marquee->conn);

  if (connHaveHandshake (marquee->conn, ROUTE_MAIN) &&
      connHaveHandshake (marquee->conn, ROUTE_PLAYERUI)) {
    marqueeSendFontSizes (marquee);
    rc = STATE_FINISHED;
  }

  logProcEnd (LOG_PROC, "marqueeHandshakeCallback", "");
  return rc;
}

static int
marqueeProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  marquee_t   *marquee = udata;
  char        *targs = NULL;

  logProcBegin (LOG_PROC, "marqueeProcessMsg");
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

  logProcEnd (LOG_PROC, "marqueeProcessMsg", "");

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  logProcEnd (LOG_PROC, "marqueeProcessMsg", "");
  return gKillReceived;
}


static bool
marqueeCloseCallback (void *udata)
{
  marquee_t   *marquee = udata;

  logProcBegin (LOG_PROC, "marqueeCloseCallback");

  if (progstateCurrState (marquee->progstate) <= STATE_RUNNING) {
    if (! marquee->isMaximized && ! marquee->isIconified) {
      marqueeSaveWindowPosition (marquee);
    }

    marquee->mqIconifyAction = true;
    uiWindowIconify (marquee->wcont [MQ_W_WINDOW]);
    marquee->isIconified = true;
    logProcEnd (LOG_PROC, "marqueeCloseWin", "user-close-win");
    return UICB_STOP;
  }

  logProcEnd (LOG_PROC, "marqueeCloseCallback", "");
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
  logProcBegin (LOG_PROC, "marqueeSetNotMaximized");

  if (! marquee->isMaximized) {
    logProcEnd (LOG_PROC, "marqueeSetNotMaximized", "not-max");
    return;
  }

  marquee->isMaximized = false;
  marqueeSetFont (marquee, nlistGetNum (marquee->options, MQ_FONT_SZ));
  marquee->unMaximize = MARQUEE_UNMAX_WAIT_COUNT;
  logProcEnd (LOG_PROC, "marqueeSetNotMaximized", "");
}

static void
marqueeSetNotMaximizeFinish (marquee_t *marquee)
{
  marquee->setPrior = true;
  uiWindowUnMaximize (marquee->wcont [MQ_W_WINDOW]);
  if (! isWindows()) {
    /* does not work on windows platforms */
    uiWindowEnableDecorations (marquee->wcont [MQ_W_WINDOW]);
  }
  marqueeSendMaximizeState (marquee);
  logProcEnd (LOG_PROC, "marqueeSetNotMaximized", "");
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

  logProcBegin (LOG_PROC, "marqueeWinState");

  if (isIconified >= 0) {
    if (marquee->mqIconifyAction) {
      marquee->mqIconifyAction = false;
      logProcEnd (LOG_PROC, "marqueeWinState", "close-button");
      return UICB_CONT;
    }

    if (isIconified) {
      marqueeSaveWindowPosition (marquee);
      marquee->isIconified = true;
    } else {
      marquee->isIconified = false;
//      marqueeMoveWindow (marquee);
    }
    logProcEnd (LOG_PROC, "marqueeWinState", "iconified/deiconified");
    return UICB_CONT;
  }

  if (isMaximized >= 0) {
    /* if the user double-clicked, this is a known maximize change and */
    /* no processing needs to be done here */
    if (marquee->userDoubleClicked) {
      marquee->userDoubleClicked = false;
      logProcEnd (LOG_PROC, "marqueeWinState", "user-double-clicked");
      return UICB_CONT;
    }

    /* user selected the maximize button */
    if (isMaximized) {
      marqueeSetMaximized (marquee);
    }
  }

  logProcEnd (LOG_PROC, "marqueeWinState", "");
  return UICB_CONT;
}

static bool
marqueeWinMapped (void *udata)
{
  marquee_t         *marquee = udata;

  logProcBegin (LOG_PROC, "marqueeWinMapped");

// is this process needed?
// need to check on windows and mac os.
  if (! marquee->isMaximized && ! marquee->isIconified) {
//    marqueeMoveWindow (marquee);
  }

  logProcEnd (LOG_PROC, "marqueeWinMapped", "");
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
  logProcBegin (LOG_PROC, "marqueeSetFontSize");

  if (uilab == NULL) {
    logProcEnd (LOG_PROC, "marqueeSetFontSize", "no-lab");
    return;
  }
  if (! uiWidgetIsValid (uilab)) {
    logProcEnd (LOG_PROC, "marqueeSetFontSize", "no-lab-widget");
    return;
  }

  uiLabelSetFont (uilab, font);
  logProcEnd (LOG_PROC, "marqueeSetFontSize", "");
}

static void
marqueePopulate (marquee_t *marquee, char *args)
{
  char      *p;
  char      *tokptr;
  int       showsep = 0;

  logProcBegin (LOG_PROC, "marqueePopulate");

  if (! marquee->mqShowInfo) {
    uiWidgetHide (marquee->wcont [MQ_W_INFOBOX]);
  }

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokptr);
  if (uiWidgetIsValid (marquee->wcont [MQ_W_INFO_ARTIST])) {
    if (p != NULL && *p == MSG_ARGS_EMPTY) {
      p = "";
    }
    if (p != NULL && *p != '\0') {
      ++showsep;
    }
    uiLabelSetText (marquee->wcont [MQ_W_INFO_ARTIST], p);
  }

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  if (uiWidgetIsValid (marquee->wcont [MQ_W_INFO_TITLE])) {
    if (p != NULL && *p == MSG_ARGS_EMPTY) {
      p = "";
    }
    if (p != NULL && *p != '\0') {
      ++showsep;
    }
    uiLabelSetText (marquee->wcont [MQ_W_INFO_TITLE], p);
  }

  if (uiWidgetIsValid (marquee->wcont [MQ_W_INFO_SEP])) {
    if (showsep == 2) {
      uiLabelSetText (marquee->wcont [MQ_W_INFO_SEP], "/");
    } else {
      uiLabelSetText (marquee->wcont [MQ_W_INFO_SEP], "");
    }
  }

  /* first entry is the main dance */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  if (p != NULL && *p == MSG_ARGS_EMPTY) {
    p = "";
  }
  uiLabelSetText (marquee->wcont [MQ_W_INFO_DANCE], p);

  for (int i = 0; i < marquee->mqLen; ++i) {
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
    if (p != NULL && *p != MSG_ARGS_EMPTY) {
      uiLabelSetText (marquee->marqueeLabs [i], p);
    } else {
      uiLabelSetText (marquee->marqueeLabs [i], "");
    }
  }
  logProcEnd (LOG_PROC, "marqueePopulate", "");
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

  logProcBegin (LOG_PROC, "marqueeSetTimer");


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
  logProcEnd (LOG_PROC, "marqueeSetTimer", "");
}

static void
marqueeSetFont (marquee_t *marquee, int sz)
{
  const char  *f;
  char        fontname [200];
  char        tbuff [200];
  int         i;

  logProcBegin (LOG_PROC, "marqueeSetFont");

  f = bdjoptGetStr (OPT_MP_MQFONT);
  if (f == NULL) {
    f = bdjoptGetStr (OPT_MP_UIFONT);
  }
  if (f == NULL) {
    f = "";
  }
  strlcpy (fontname, f, sizeof (fontname));
  i = strlen (fontname) - 1;
  while (i >= 0 && (isdigit (fontname [i]) || isspace (fontname [i]))) {
    fontname [i] = '\0';
    --i;
  }
  snprintf (tbuff, sizeof (tbuff), "%s bold %d", fontname, sz);

  if (marquee->isMaximized) {
    nlistSetNum (marquee->options, MQ_FONT_SZ_FS, sz);
  } else {
    nlistSetNum (marquee->options, MQ_FONT_SZ, sz);
  }

  marqueeSetFontSize (marquee, marquee->wcont [MQ_W_INFO_DANCE], tbuff);
  marqueeSetFontSize (marquee, marquee->wcont [MQ_W_COUNTDOWN_TIMER], tbuff);

  /* not bold */
  snprintf (tbuff, sizeof (tbuff), "%s %d", fontname, sz);
  for (int i = 0; i < marquee->mqLen; ++i) {
    marqueeSetFontSize (marquee, marquee->marqueeLabs [i], tbuff);
    uiWidgetSetClass (marquee->marqueeLabs [i], MQ_TEXT_CLASS);
  }

  sz = (int) round ((double) sz * 0.7);
  snprintf (tbuff, sizeof (tbuff), "%s %d", fontname, sz);
  if (uiWidgetIsValid (marquee->wcont [MQ_W_INFO_ARTIST])) {
    marqueeSetFontSize (marquee, marquee->wcont [MQ_W_INFO_ARTIST], tbuff);
    uiWidgetSetClass (marquee->wcont [MQ_W_INFO_ARTIST], MQ_INFO_CLASS);
    marqueeSetFontSize (marquee, marquee->wcont [MQ_W_INFO_SEP], tbuff);
    uiWidgetSetClass (marquee->wcont [MQ_W_INFO_SEP], MQ_INFO_CLASS);
    marqueeSetFontSize (marquee, marquee->wcont [MQ_W_INFO_TITLE], tbuff);
    uiWidgetSetClass (marquee->wcont [MQ_W_INFO_TITLE], MQ_INFO_CLASS);
  }

  logProcEnd (LOG_PROC, "marqueeSetFont", "");
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

  uiLabelSetText (marquee->wcont [MQ_W_INFO_DANCE], "");
  uiLabelSetText (marquee->wcont [MQ_W_INFO_ARTIST], "");
  uiLabelSetText (marquee->wcont [MQ_W_INFO_SEP], "");

  disp = bdjoptGetStr (OPT_P_COMPLETE_MSG);
  uiLabelSetText (marquee->wcont [MQ_W_INFO_TITLE], disp);

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
