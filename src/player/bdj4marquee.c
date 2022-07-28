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
#include <ctype.h>

#include "bdj4.h"
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "datafile.h"
#include "conn.h"
#include "localeutil.h"
#include "lock.h"
#include "log.h"
#include "osuiutils.h"
#include "osutils.h"
#include "pathbld.h"
#include "progstate.h"
#include "sock.h"
#include "sockh.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"
#include "ui.h"

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

/* sort by ascii values */
static datafilekey_t mqdfkeys [MQ_KEY_MAX] = {
  { "MQ_FONT_SZ",   MQ_FONT_SZ,       VALUE_NUM, NULL, -1 },
  { "MQ_FONT_SZ_FS",MQ_FONT_SZ_FS,    VALUE_NUM, NULL, -1 },
  { "MQ_POS_X",     MQ_POSITION_X,    VALUE_NUM, NULL, -1 },
  { "MQ_POS_Y",     MQ_POSITION_Y,    VALUE_NUM, NULL, -1 },
  { "MQ_SIZE_X",    MQ_SIZE_X,        VALUE_NUM, NULL, -1 },
  { "MQ_SIZE_Y",    MQ_SIZE_Y,        VALUE_NUM, NULL, -1 },
  { "MQ_WORKSPACE", MQ_WORKSPACE,     VALUE_NUM, NULL, -1 },
};

typedef struct {
  progstate_t     *progstate;
  char            *locknm;
  conn_t          *conn;
  int             stopwaitcount;
  datafile_t      *optiondf;
  nlist_t         *options;
  UIWidget        window;
  UICallback      exitcb;
  UICallback      dclickcb;
  UICallback      winstatecb;
  UICallback      winmapcb;
  UIWidget        pbar;
  UIWidget        infoBox;
  UIWidget        sep;
  UIWidget        countdownTimerLab;
  UIWidget        infoArtistLab;
  UIWidget        infoSepLab;
  UIWidget        infoTitleLab;
  UIWidget        danceLab;
  UIWidget        *marqueeLabs;   // array of UIWidget
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
} marquee_t;

#define INFO_LAB_HEIGHT_ADJUST    0.85
enum {
  MARQUEE_UNMAX_WAIT_COUNT = 3,
};

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
static void marqueeSetFontSize (marquee_t *marquee, UIWidget *lab, const char *font);
static void marqueePopulate (marquee_t *marquee, char *args);
static void marqueeSetTimer (marquee_t *marquee, char *args);
static void marqueeSetFont (marquee_t *marquee, int sz);
static void marqueeFind (marquee_t *marquee);
static void marqueeDisplayCompletion (marquee_t *marquee);

static int gKillReceived = 0;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  marquee_t       marquee;
  char            *mqfont;
  char            tbuff [MAXPATHLEN];
  int             flags;
  int             startflags;


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

  uiutilsUIWidgetInit (&marquee.window);
  uiutilsUIWidgetInit (&marquee.pbar);
  uiutilsUIWidgetInit (&marquee.sep);
  uiutilsUIWidgetInit (&marquee.countdownTimerLab);
  uiutilsUIWidgetInit (&marquee.infoBox);
  uiutilsUIWidgetInit (&marquee.infoArtistLab);
  uiutilsUIWidgetInit (&marquee.infoSepLab);
  uiutilsUIWidgetInit (&marquee.infoTitleLab);
  uiutilsUIWidgetInit (&marquee.danceLab);
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

  osSetStandardSignals (marqueeSigHandler);

  flags = BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  startflags = bdj4startup (argc, argv, NULL, "mq", ROUTE_MARQUEE, flags);
  if (bdjoptGetNum (OPT_P_HIDE_MARQUEE_ON_START) ||
     (startflags & BDJ4_INIT_HIDE_MARQUEE) == BDJ4_INIT_HIDE_MARQUEE) {
    marquee.hideonstart = true;
  }

  listenPort = bdjvarsGetNum (BDJVL_MARQUEE_PORT);
  marquee.mqLen = bdjoptGetNum (OPT_P_MQQLEN);
  marquee.mqShowInfo = bdjoptGetNum (OPT_P_MQ_SHOW_INFO);
  marquee.conn = connInit (ROUTE_MARQUEE);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "marquee", BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  marquee.optiondf = datafileAllocParse ("marquee-opt", DFTYPE_KEY_VAL, tbuff,
      mqdfkeys, MQ_KEY_MAX, DATAFILE_NO_LOOKUP);
  marquee.options = datafileGetList (marquee.optiondf);
  if (marquee.options == NULL) {
    marquee.options = nlistAlloc ("marquee-opt", LIST_ORDERED, free);

    nlistSetNum (marquee.options, MQ_WORKSPACE, -1);
    nlistSetNum (marquee.options, MQ_POSITION_X, -1);
    nlistSetNum (marquee.options, MQ_POSITION_Y, -1);
    nlistSetNum (marquee.options, MQ_SIZE_X, 600);
    nlistSetNum (marquee.options, MQ_SIZE_Y, 600);
    nlistSetNum (marquee.options, MQ_FONT_SZ, 36);
    nlistSetNum (marquee.options, MQ_FONT_SZ_FS, 60);
  }

  uiUIInitialize ();
  mqfont = bdjoptGetStr (OPT_MP_MQFONT);
  uiSetUIFont (mqfont);

  marqueeBuildUI (&marquee);
  osuiFinalize ();

  sockhMainLoop (listenPort, marqueeProcessMsg, marqueeMainLoop, &marquee);
  connFree (marquee.conn);
  progstateFree (marquee.progstate);
  logEnd ();
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

  if (uiWindowIsMaximized (&marquee->window)) {
    logProcEnd (LOG_PROC, "marqueeStoppingCallback", "is-maximized-b");
    return STATE_NOT_FINISH;
  }

  uiWindowGetSize (&marquee->window, &x, &y);
  nlistSetNum (marquee->options, MQ_SIZE_X, x);
  nlistSetNum (marquee->options, MQ_SIZE_Y, y);
  if (! marquee->isIconified) {
    marqueeSaveWindowPosition (marquee);
  }

  connDisconnect (marquee->conn, ROUTE_MAIN);
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
  char        fn [MAXPATHLEN];

  logProcBegin (LOG_PROC, "marqueeClosingCallback");

  /* these are moved here so that the window can be un-maximized and */
  /* the size/position saved */
  uiCloseWindow (&marquee->window);

  pathbldMakePath (fn, sizeof (fn),
      "marquee", BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("marquee", fn, mqdfkeys, MQ_KEY_MAX, marquee->options);

  bdj4shutdown (ROUTE_MARQUEE, NULL);

  connDisconnectAll (marquee->conn);

  if (marquee->marqueeLabs != NULL) {
    free (marquee->marqueeLabs);
  }
  if (marquee->options != NULL) {
    if (marquee->options != datafileGetList (marquee->optiondf)) {
      nlistFree (marquee->options);
    }
  }
  datafileFree (marquee->optiondf);

  logProcEnd (LOG_PROC, "marqueeClosingCallback", "");
  return STATE_FINISHED;
}

static void
marqueeBuildUI (marquee_t *marquee)
{
  char      imgbuff [MAXPATHLEN];
  UIWidget  uiwidget;
  UIWidget  mainvbox;
  UIWidget  hbox;
  UIWidget  vbox;
  int       x, y;

  logProcBegin (LOG_PROC, "marqueeBuildUI");
  uiutilsUIWidgetInit (&mainvbox);
  uiutilsUIWidgetInit (&vbox);
  uiutilsUIWidgetInit (&hbox);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_marquee", ".svg", PATHBLD_MP_IMGDIR);

  uiutilsUICallbackInit (&marquee->exitcb, marqueeCloseCallback, marquee);
  uiCreateMainWindow (&uiwidget, &marquee->exitcb,
      /* CONTEXT: marquee: marquee window title */
      _("Marquee"), imgbuff);
  uiWindowNoFocusOnStartup (&uiwidget);

  uiutilsUICallbackInit (&marquee->dclickcb,
      marqueeToggleFullscreen, marquee);
  uiWindowSetDoubleClickCallback (&uiwidget, &marquee->dclickcb);

  uiutilsUICallbackIntIntInit (&marquee->winstatecb,
      marqueeWinState, marquee);
  uiWindowSetWinStateCallback (&uiwidget, &marquee->winstatecb);

  uiutilsUICallbackInit (&marquee->winmapcb,
      marqueeWinMapped, marquee);
  uiWindowSetMappedCallback (&uiwidget, &marquee->winmapcb);

  uiWindowNoDim (&uiwidget);

  uiutilsUIWidgetCopy (&marquee->window, &uiwidget);

  x = nlistGetNum (marquee->options, MQ_SIZE_X);
  y = nlistGetNum (marquee->options, MQ_SIZE_Y);
  uiWindowSetDefaultSize (&marquee->window, x, y);

  uiCreateVertBox (&mainvbox);
  uiWidgetSetAllMargins (&mainvbox, 10);
  uiBoxPackInWindow (&marquee->window, &mainvbox);
  uiWidgetExpandHoriz (&mainvbox);
  uiWidgetExpandVert (&mainvbox);
  marquee->marginTotal = 20;

  uiCreateProgressBar (&marquee->pbar, bdjoptGetStr (OPT_P_MQ_ACCENT_COL));
  uiBoxPackStart (&mainvbox, &marquee->pbar);

  uiCreateVertBox (&vbox);
  uiWidgetExpandHoriz (&vbox);
  uiBoxPackStart (&mainvbox, &vbox);

  uiCreateHorizBox (&hbox);
  uiWidgetAlignHorizFill (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: marquee: displayed when nothing is set to be played */
  uiCreateLabel (&uiwidget, _("Not Playing"));
  uiWidgetAlignHorizStart (&uiwidget);
  uiWidgetDisableFocus (&uiwidget);
  uiLabelSetColor (&uiwidget, bdjoptGetStr (OPT_P_MQ_ACCENT_COL));
  uiBoxPackStart (&hbox, &uiwidget);
  uiutilsUIWidgetCopy (&marquee->danceLab, &uiwidget);

  uiCreateLabel (&uiwidget, "0:00");
  uiLabelSetMaxWidth (&uiwidget, 6);
  uiWidgetAlignHorizEnd (&uiwidget);
  uiWidgetDisableFocus (&uiwidget);
  uiLabelSetColor (&uiwidget, bdjoptGetStr (OPT_P_MQ_ACCENT_COL));
  uiBoxPackEnd (&hbox, &uiwidget);
  uiutilsUIWidgetCopy (&marquee->countdownTimerLab, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiWidgetAlignHorizFill (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);
  uiutilsUIWidgetCopy (&marquee->infoBox, &hbox);

  uiCreateLabel (&uiwidget, "");
  uiWidgetAlignHorizStart (&uiwidget);
  uiWidgetDisableFocus (&uiwidget);
  uiLabelEllipsizeOn (&uiwidget);
  uiBoxPackStart (&hbox, &uiwidget);
  uiutilsUIWidgetCopy (&marquee->infoArtistLab, &uiwidget);

  uiCreateLabel (&uiwidget, "");
  uiWidgetAlignHorizStart (&uiwidget);
  uiWidgetDisableFocus (&uiwidget);
  uiBoxPackStart (&hbox, &uiwidget);
  uiWidgetSetMarginStart (&uiwidget, uiBaseMarginSz * 2);
  uiWidgetSetMarginEnd (&uiwidget, uiBaseMarginSz * 2);
  uiutilsUIWidgetCopy (&marquee->infoSepLab, &uiwidget);

  uiCreateLabel (&uiwidget, "");
  uiWidgetAlignHorizStart (&uiwidget);
  uiWidgetDisableFocus (&uiwidget);
  uiLabelEllipsizeOn (&uiwidget);
  uiBoxPackStart (&hbox, &uiwidget);
  uiutilsUIWidgetCopy (&marquee->infoTitleLab, &uiwidget);

  uiCreateHorizSeparator (&marquee->sep);
  uiSeparatorSetColor (&marquee->sep, bdjoptGetStr (OPT_P_MQ_ACCENT_COL));
  uiWidgetExpandHoriz (&marquee->sep);
  uiWidgetSetMarginTop (&marquee->sep, uiBaseMarginSz * 2);
  uiWidgetSetMarginBottom (&marquee->sep, uiBaseMarginSz * 4);
  uiBoxPackEnd (&vbox, &marquee->sep);

  marquee->marqueeLabs = malloc (sizeof (UIWidget) * marquee->mqLen);

  for (int i = 0; i < marquee->mqLen; ++i) {
    uiutilsUIWidgetInit (&marquee->marqueeLabs [i]);
    uiCreateLabel (&marquee->marqueeLabs [i], "");
    uiWidgetAlignHorizStart (&marquee->marqueeLabs [i]);
    uiWidgetExpandHoriz (&marquee->marqueeLabs [i]);
    uiWidgetDisableFocus (&marquee->marqueeLabs [i]);
    uiWidgetSetMarginTop (&marquee->marqueeLabs [i], uiBaseMarginSz * 4);
    uiBoxPackStart (&mainvbox, &marquee->marqueeLabs [i]);
  }

  marqueeSetFont (marquee, nlistGetNum (marquee->options, MQ_FONT_SZ));

  if (marquee->hideonstart) {
    uiWindowIconify (&marquee->window);
    marquee->isIconified = true;
  }

  if (! marquee->mqShowInfo) {
    uiWidgetHide (&marquee->infoBox);
  }
  uiWidgetShowAll (&marquee->window);

  marqueeMoveWindow (marquee);

  progstateLogTime (marquee->progstate, "time-to-start-gui");

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_marquee", ".png", PATHBLD_MP_IMGDIR);
  osuiSetIcon (imgbuff);

  logProcEnd (LOG_PROC, "marqueeBuildUI", "");
}

static int
marqueeMainLoop (void *tmarquee)
{
  marquee_t   *marquee = tmarquee;
  int         stop = FALSE;

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
    char    tbuff [100];

    snprintf (tbuff, sizeof (tbuff), "%ld%c%ld",
        nlistGetNum (marquee->options, MQ_FONT_SZ),
        MSG_ARGS_RS,
        nlistGetNum (marquee->options, MQ_FONT_SZ_FS));
    connSendMessage (marquee->conn, ROUTE_PLAYERUI,
        MSG_MARQUEE_FONT_SIZES, tbuff);
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
  bool        dbgdisp = false;
  char        *targs = NULL;

  logProcBegin (LOG_PROC, "marqueeProcessMsg");
  if (args != NULL) {
    targs = strdup (args);
  }

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MARQUEE: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (marquee->conn, routefrom);
          dbgdisp = true;
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          progstateShutdownProcess (marquee->progstate);
          dbgdisp = true;
          break;
        }
        case MSG_MARQUEE_DATA: {
          marqueePopulate (marquee, targs);
          dbgdisp = true;
          break;
        }
        case MSG_MARQUEE_TIMER: {
          marqueeSetTimer (marquee, targs);
          break;
        }
        case MSG_MARQUEE_SET_FONT_SZ: {
          marqueeSetFont (marquee, atoi (targs));
          dbgdisp = true;
          break;
        }
        case MSG_MARQUEE_FIND: {
          marqueeFind (marquee);
          dbgdisp = true;
          break;
        }
        case MSG_FINISHED: {
          marqueeDisplayCompletion (marquee);
          dbgdisp = true;
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

  if (dbgdisp) {
    logMsg (LOG_DBG, LOG_MSGS, "rcvd: from:%d/%s route:%d/%s msg:%d/%s args:%s",
        routefrom, msgRouteDebugText (routefrom),
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
  }
  if (targs != NULL) {
    free (targs);
  }

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
    uiWindowIconify (&marquee->window);
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
    uiWindowDisableDecorations (&marquee->window);
  }
  uiWindowMaximize (&marquee->window);
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
  uiWindowUnMaximize (&marquee->window);
  if (! isWindows()) {
    /* does not work on windows platforms */
    uiWindowEnableDecorations (&marquee->window);
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

  uiWindowGetPosition (&marquee->window, &x, &y, &ws);
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
  uiWindowMove (&marquee->window, x, y, ws);
}

static void
marqueeSigHandler (int sig)
{
  gKillReceived = 1;
}

static void
marqueeSetFontSize (marquee_t *marquee, UIWidget *uilab, const char *font)
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
    uiWidgetHide (&marquee->infoBox);
  }

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokptr);
  if (uiWidgetIsValid (&marquee->infoArtistLab)) {
    if (p != NULL && *p == MSG_ARGS_EMPTY) {
      p = "";
    }
    if (p != NULL && *p != '\0') {
      ++showsep;
    }
    uiLabelSetText (&marquee->infoArtistLab, p);
  }

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  if (uiWidgetIsValid (&marquee->infoTitleLab)) {
    if (p != NULL && *p == MSG_ARGS_EMPTY) {
      p = "";
    }
    if (p != NULL && *p != '\0') {
      ++showsep;
    }
    uiLabelSetText (&marquee->infoTitleLab, p);
  }

  if (uiWidgetIsValid (&marquee->infoSepLab)) {
    if (showsep == 2) {
      uiLabelSetText (&marquee->infoSepLab, "/");
    } else {
      uiLabelSetText (&marquee->infoSepLab, "");
    }
  }

  /* first entry is the main dance */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  if (p != NULL && *p == MSG_ARGS_EMPTY) {
    p = "";
  }
  uiLabelSetText (&marquee->danceLab, p);

  for (int i = 0; i < marquee->mqLen; ++i) {
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
    if (p != NULL && *p != MSG_ARGS_EMPTY) {
      uiLabelSetText (&marquee->marqueeLabs [i], p);
    } else {
      uiLabelSetText (&marquee->marqueeLabs [i], "");
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
  uiLabelSetText (&marquee->countdownTimerLab, tbuff);

  ddur = (double) dur;
  dplayed = (double) played;
  if (dur != 0.0) {
    dratio = dplayed / ddur;
  } else {
    dratio = 0.0;
  }
  uiProgressBarSet (&marquee->pbar, dratio);
  logProcEnd (LOG_PROC, "marqueeSetTimer", "");
}

static void
marqueeSetFont (marquee_t *marquee, int sz)
{
  char    fontname [200];
  char    tbuff [200];
  size_t  i;

  logProcBegin (LOG_PROC, "marqueeSetFont");

  strlcpy (fontname, bdjoptGetStr (OPT_MP_MQFONT), sizeof (fontname));
  i = strlen (fontname) - 1;
  while (isdigit (fontname [i]) || isspace (fontname [i])) {
    fontname [i] = '\0';
    --i;
  }
  snprintf (tbuff, sizeof (tbuff), "%s bold %d", fontname, sz);

  if (marquee->isMaximized) {
    nlistSetNum (marquee->options, MQ_FONT_SZ_FS, sz);
  } else {
    nlistSetNum (marquee->options, MQ_FONT_SZ, sz);
  }

  marqueeSetFontSize (marquee, &marquee->danceLab, tbuff);
  marqueeSetFontSize (marquee, &marquee->countdownTimerLab, tbuff);

  /* not bold */
  snprintf (tbuff, sizeof (tbuff), "%s %d", fontname, sz);
  for (int i = 0; i < marquee->mqLen; ++i) {
    marqueeSetFontSize (marquee, &marquee->marqueeLabs [i], tbuff);
    uiLabelSetColor (&marquee->marqueeLabs [i], bdjoptGetStr (OPT_P_MQ_TEXT_COL));
  }

  sz = (int) round ((double) sz * 0.7);
  snprintf (tbuff, sizeof (tbuff), "%s %d", fontname, sz);
  if (uiWidgetIsValid (&marquee->infoArtistLab)) {
    marqueeSetFontSize (marquee, &marquee->infoArtistLab, tbuff);
    uiLabelSetColor (&marquee->infoArtistLab, bdjoptGetStr (OPT_P_MQ_INFO_COL));
    marqueeSetFontSize (marquee, &marquee->infoSepLab, tbuff);
    uiLabelSetColor (&marquee->infoSepLab, bdjoptGetStr (OPT_P_MQ_INFO_COL));
    marqueeSetFontSize (marquee, &marquee->infoTitleLab, tbuff);
    uiLabelSetColor (&marquee->infoTitleLab, bdjoptGetStr (OPT_P_MQ_INFO_COL));
  }

  logProcEnd (LOG_PROC, "marqueeSetFont", "");
}

static void
marqueeFind (marquee_t *marquee)
{
  /* on linux XFCE, this will position the window at this location in */
  /* its current workspace */
  nlistSetNum (marquee->options, MQ_POSITION_X, 200);
  nlistSetNum (marquee->options, MQ_POSITION_Y, 200);

  if (marquee->isIconified) {
    uiWindowDeIconify (&marquee->window);
  }
  marqueeSetNotMaximized (marquee);
  marqueeMoveWindow (marquee);
  uiWindowMoveToCurrentWorkspace (&marquee->window);
  uiWindowPresent (&marquee->window);
  marqueeSaveWindowPosition (marquee);
}

static void
marqueeDisplayCompletion (marquee_t *marquee)
{
  char  *disp;

  disp = bdjoptGetStr (OPT_P_COMPLETE_MSG);
  uiLabelSetText (&marquee->infoArtistLab, "");
  uiLabelSetText (&marquee->infoSepLab, "");
  uiLabelSetText (&marquee->infoTitleLab, disp);

  if (! marquee->mqShowInfo) {
    uiWidgetShowAll (&marquee->infoBox);
  }
}
