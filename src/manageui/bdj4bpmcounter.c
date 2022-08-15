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
#include <time.h>

#include "bdj4.h"
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjvars.h"
#include "conn.h"
#include "log.h"
#include "ossignal.h"
#include "osuiutils.h"
#include "pathbld.h"
#include "procutil.h"
#include "progstate.h"
#include "sockh.h"
#include "tmutil.h"
#include "ui.h"

enum {
  BPMCOUNT_CB_EXIT,
  BPMCOUNT_CB_SAVE,
  BPMCOUNT_CB_RESET,
  BPMCOUNT_CB_CLICK,
  BPMCOUNT_CB_RADIO,
  BPMCOUNT_CB_MAX,
};

enum {
  BPMCOUNT_DISP_BEATS,
  BPMCOUNT_DISP_SECONDS,
  BPMCOUNT_DISP_BPM,
  BPMCOUNT_DISP_MPM_24,
  BPMCOUNT_DISP_MPM_34,
  BPMCOUNT_DISP_MPM_44,
  BPMCOUNT_DISP_MAX,
};

static char *disptxt [BPMCOUNT_DISP_MAX];

typedef struct {
  progstate_t     *progstate;
  char            *locknm;
  procutil_t      *processes [ROUTE_MAX];
  conn_t          *conn;
  UIWidget        window;
  UIWidget        timesigsel [BPMCOUNT_DISP_MAX];
  UIWidget        dispvalue [BPMCOUNT_DISP_MAX];
  int             values [BPMCOUNT_DISP_MAX];
  UICallback      callbacks [BPMCOUNT_CB_MAX];
  int             stopwaitcount;
  int             count;
  time_t          begtime;
  int             timesigidx;
  /* options */
  datafile_t      *optiondf;
  nlist_t         *options;
} bpmcounter_t;

enum {
  BPMCOUNTER_POSITION_X,
  BPMCOUNTER_POSITION_Y,
  BPMCOUNTER_SIZE_X,
  BPMCOUNTER_SIZE_Y,
  BPMCOUNTER_KEY_MAX,
};

static datafilekey_t bpmcounteruidfkeys [BPMCOUNTER_KEY_MAX] = {
  { "BPMCOUNTER_POS_X",     BPMCOUNTER_POSITION_X,    VALUE_NUM, NULL, -1 },
  { "BPMCOUNTER_POS_Y",     BPMCOUNTER_POSITION_Y,    VALUE_NUM, NULL, -1 },
  { "BPMCOUNTER_SIZE_X",    BPMCOUNTER_SIZE_X,        VALUE_NUM, NULL, -1 },
  { "BPMCOUNTER_SIZE_Y",    BPMCOUNTER_SIZE_Y,        VALUE_NUM, NULL, -1 },
};

static bool     bpmcounterConnectingCallback (void *udata, programstate_t programState);
static bool     bpmcounterHandshakeCallback (void *udata, programstate_t programState);
static bool     bpmcounterStoppingCallback (void *udata, programstate_t programState);
static bool     bpmcounterStopWaitCallback (void *udata, programstate_t programState);
static bool     bpmcounterClosingCallback (void *udata, programstate_t programState);
static void     bpmcounterBuildUI (bpmcounter_t *bpmcounter);
static int      bpmcounterMainLoop  (void *tbpmcounter);
static int      bpmcounterProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static bool     bpmcounterCloseCallback (void *udata);
static void     bpmcounterSigHandler (int sig);
static bool     bpmcounterProcessSave (void *udata);
static bool     bpmcounterProcessReset (void *udata);
static bool     bpmcounterProcessClick (void *udata);
static void     bpmcounterProcessTimesig (bpmcounter_t *bpmcounter, char *args);
static bool     bpmcounterRadioChanged (void *udata);

static int gKillReceived = 0;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  bpmcounter_t    bpmcounter;
  char            *uifont;
  char            tbuff [MAXPATHLEN];
  int             flags;


  /* CONTEXT: bpm counter: number of beats */
  disptxt [BPMCOUNT_DISP_BEATS] = _("Beats");
  /* CONTEXT: bpm counter: number of seconds */
  disptxt [BPMCOUNT_DISP_SECONDS] = _("Seconds");
  /* CONTEXT: bpm counter: BPM */
  disptxt [BPMCOUNT_DISP_BPM] = _("BPM");
  /* CONTEXT: bpm counter: MPM in 2/4 time */
  disptxt [BPMCOUNT_DISP_MPM_24] = _("MPM (2/4 time)");
  /* CONTEXT: bpm counter: MPM in 3/4 time */
  disptxt [BPMCOUNT_DISP_MPM_34] = _("MPM (3/4 time)");
  /* CONTEXT: bpm counter: MPM in 4/4 or 4/8 time */
  disptxt [BPMCOUNT_DISP_MPM_44] = _("MPM (4/4 or 4/8 time)");

  bpmcounter.count = 0;
  bpmcounter.begtime = 0;
  bpmcounter.timesigidx = BPMCOUNT_DISP_BPM;
  for (int i = 0; i < BPMCOUNT_DISP_MAX; ++i) {
    bpmcounter.values [i] = 0;
    uiutilsUIWidgetInit (&bpmcounter.timesigsel [i]);
    uiutilsUIWidgetInit (&bpmcounter.dispvalue [i]);
  }
  for (int i = 0; i < BPMCOUNT_CB_MAX; ++i) {
    uiutilsUICallbackInit (&bpmcounter.callbacks [i], NULL, NULL);
  }

  bpmcounter.stopwaitcount = 0;
  bpmcounter.progstate = progstateInit ("bpmcounter");

  progstateSetCallback (bpmcounter.progstate, STATE_CONNECTING,
      bpmcounterConnectingCallback, &bpmcounter);
  progstateSetCallback (bpmcounter.progstate, STATE_WAIT_HANDSHAKE,
      bpmcounterHandshakeCallback, &bpmcounter);
  progstateSetCallback (bpmcounter.progstate, STATE_STOPPING,
      bpmcounterStoppingCallback, &bpmcounter);
  progstateSetCallback (bpmcounter.progstate, STATE_STOP_WAIT,
      bpmcounterStopWaitCallback, &bpmcounter);
  progstateSetCallback (bpmcounter.progstate, STATE_CLOSING,
      bpmcounterClosingCallback, &bpmcounter);

  uiutilsUIWidgetInit (&bpmcounter.window);

  procutilInitProcesses (bpmcounter.processes);

  osSetStandardSignals (bpmcounterSigHandler);

  flags = BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  bdj4startup (argc, argv, NULL, "bc", ROUTE_BPM_COUNTER, flags);
  logProcBegin (LOG_PROC, "bpmcounter");

  listenPort = bdjvarsGetNum (BDJVL_BPM_COUNTER_PORT);
  bpmcounter.conn = connInit (ROUTE_BPM_COUNTER);

  pathbldMakePath (tbuff, sizeof (tbuff),
      BPMCOUNTER_OPT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  bpmcounter.optiondf = datafileAllocParse ("bpmcounter-opt", DFTYPE_KEY_VAL, tbuff,
      bpmcounteruidfkeys, BPMCOUNTER_KEY_MAX);
  bpmcounter.options = datafileGetList (bpmcounter.optiondf);
  if (bpmcounter.options == NULL) {
    bpmcounter.options = nlistAlloc ("bpmcounterui-opt", LIST_ORDERED, free);

    nlistSetNum (bpmcounter.options, BPMCOUNTER_POSITION_X, -1);
    nlistSetNum (bpmcounter.options, BPMCOUNTER_POSITION_Y, -1);
    nlistSetNum (bpmcounter.options, BPMCOUNTER_SIZE_X, 1200);
    nlistSetNum (bpmcounter.options, BPMCOUNTER_SIZE_Y, 800);
  }

  uiUIInitialize ();
  uifont = bdjoptGetStr (OPT_MP_UIFONT);
  uiSetUIFont (uifont);

  bpmcounterBuildUI (&bpmcounter);
  osuiFinalize ();

  sockhMainLoop (listenPort, bpmcounterProcessMsg, bpmcounterMainLoop, &bpmcounter);
  connFree (bpmcounter.conn);
  progstateFree (bpmcounter.progstate);
  logProcEnd (LOG_PROC, "bpmcounter", "");
  logEnd ();
  return status;
}

/* internal routines */

static bool
bpmcounterConnectingCallback (void *udata, programstate_t programState)
{
  bpmcounter_t  *bpmcounter = udata;
  bool          rc = STATE_NOT_FINISH;

  connProcessUnconnected (bpmcounter->conn);

  if (! connIsConnected (bpmcounter->conn, ROUTE_MANAGEUI)) {
    connConnect (bpmcounter->conn, ROUTE_MANAGEUI);
  }

  if (connIsConnected (bpmcounter->conn, ROUTE_MANAGEUI)) {
    rc = STATE_FINISHED;
  }

  return rc;
}

static bool
bpmcounterHandshakeCallback (void *udata, programstate_t programState)
{
  bpmcounter_t  *bpmcounter = udata;
  bool          rc = STATE_NOT_FINISH;

  connProcessUnconnected (bpmcounter->conn);

  if (! connIsConnected (bpmcounter->conn, ROUTE_MANAGEUI)) {
    connConnect (bpmcounter->conn, ROUTE_MANAGEUI);
  }

  if (connHaveHandshake (bpmcounter->conn, ROUTE_MANAGEUI)) {
    rc = STATE_FINISHED;
  }

  return rc;
}

static bool
bpmcounterStoppingCallback (void *udata, programstate_t programState)
{
  bpmcounter_t   *bpmcounter = udata;
  int             x, y, ws;

  logProcBegin (LOG_PROC, "bpmcounterStoppingCallback");

  uiWindowGetSize (&bpmcounter->window, &x, &y);
  nlistSetNum (bpmcounter->options, BPMCOUNTER_SIZE_X, x);
  nlistSetNum (bpmcounter->options, BPMCOUNTER_SIZE_Y, y);
  uiWindowGetPosition (&bpmcounter->window, &x, &y, &ws);
  nlistSetNum (bpmcounter->options, BPMCOUNTER_POSITION_X, x);
  nlistSetNum (bpmcounter->options, BPMCOUNTER_POSITION_Y, y);

  logProcEnd (LOG_PROC, "bpmcounterStoppingCallback", "");
  return STATE_FINISHED;
}

static bool
bpmcounterStopWaitCallback (void *udata, programstate_t programState)
{
  bpmcounter_t   *bpmcounter = udata;
  bool        rc;

  rc = connWaitClosed (bpmcounter->conn, &bpmcounter->stopwaitcount);
  return rc;
}

static bool
bpmcounterClosingCallback (void *udata, programstate_t programState)
{
  bpmcounter_t   *bpmcounter = udata;
  char        fn [MAXPATHLEN];

  logProcBegin (LOG_PROC, "bpmcounterClosingCallback");
  uiCloseWindow (&bpmcounter->window);

  procutilFreeAll (bpmcounter->processes);

  bdj4shutdown (ROUTE_BPM_COUNTER, NULL);

  pathbldMakePath (fn, sizeof (fn),
      BPMCOUNTER_OPT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("bpmcounter", fn, bpmcounteruidfkeys, BPMCOUNTER_KEY_MAX, bpmcounter->options);

  if (bpmcounter->optiondf != NULL) {
    datafileFree (bpmcounter->optiondf);
  } else if (bpmcounter->options != NULL) {
    nlistFree (bpmcounter->options);
  }

  logProcEnd (LOG_PROC, "bpmcounterClosingCallback", "");
  return STATE_FINISHED;
}

static void
bpmcounterBuildUI (bpmcounter_t  *bpmcounter)
{
  UIWidget    grpuiwidget;
  UIWidget    uiwidget;
  UIWidget    vboxmain;
  UIWidget    vbox;
  UIWidget    hboxbpm;
  UIWidget    hbox;
  UIWidget    sg;
  UIWidget    sgb;
  char        imgbuff [MAXPATHLEN];
  int         x, y;

  logProcBegin (LOG_PROC, "bpmcounterBuildUI");
  uiutilsUIWidgetInit (&grpuiwidget);
  uiCreateSizeGroupHoriz (&sg);
  uiCreateSizeGroupHoriz (&sgb);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", ".svg", PATHBLD_MP_IMGDIR);
  uiutilsUICallbackInit (&bpmcounter->callbacks [BPMCOUNT_CB_EXIT],
      bpmcounterCloseCallback, bpmcounter);
  uiCreateMainWindow (&bpmcounter->window,
      &bpmcounter->callbacks [BPMCOUNT_CB_EXIT],
      /* CONTEXT: bpm counter: title of window*/
      _("BPM Counter"), imgbuff);

  uiCreateVertBox (&vboxmain);
  uiWidgetSetAllMargins (&vboxmain, uiBaseMarginSz * 2);
  uiBoxPackInWindow (&bpmcounter->window, &vboxmain);

  /* instructions */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vboxmain, &hbox);

  /* CONTEXT: bpm counter: instructions, line 1 */
  uiCreateLabel (&uiwidget, _("Click the mouse in the blue box in time with the beat."));
  uiBoxPackStart (&hbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vboxmain, &hbox);

  /* CONTEXT: bpm counter: instructions, line 2 */
  uiCreateLabel (&uiwidget, _("When the BPM value is stable, select the Save button."));
  uiBoxPackStart (&hbox, &uiwidget);

  /* secondary box */
  uiCreateHorizBox (&hboxbpm);
  uiBoxPackStartExpand (&vboxmain, &hboxbpm);

  /* left side */
  uiCreateVertBox (&vbox);
  uiBoxPackStartExpand (&hboxbpm, &vbox);

  /* some spacing */
  uiCreateLabel (&uiwidget, "");
  uiBoxPackStart (&vbox, &uiwidget);

  uiutilsUICallbackInit (&bpmcounter->callbacks [BPMCOUNT_CB_RADIO],
      bpmcounterRadioChanged, bpmcounter);

  for (int i = 0; i < BPMCOUNT_DISP_MAX; ++i) {
    uiCreateHorizBox (&hbox);
    uiBoxPackStart (&vbox, &hbox);

    if (i < BPMCOUNT_DISP_BPM) {
      uiCreateColonLabel (&uiwidget, disptxt [i]);
    } else if (i == BPMCOUNT_DISP_BPM) {
      uiCreateRadioButton (&uiwidget, NULL, disptxt [i], 1);
      uiutilsUIWidgetCopy (&grpuiwidget, &uiwidget);
    } else {
      uiCreateRadioButton (&uiwidget, &grpuiwidget, disptxt [i], 0);
    }
    uiutilsUIWidgetCopy (&bpmcounter->timesigsel [i], &uiwidget);
    if (i >= BPMCOUNT_DISP_BPM) {
      uiToggleButtonSetCallback (&bpmcounter->timesigsel [i],
          &bpmcounter->callbacks [BPMCOUNT_CB_RADIO]);
    }
    uiSizeGroupAdd (&sg, &uiwidget);
    uiBoxPackStart (&hbox, &uiwidget);

    uiCreateLabel (&bpmcounter->dispvalue [i], "");
    uiLabelAlignEnd (&bpmcounter->dispvalue [i]);
    uiSizeGroupAdd (&sgb, &bpmcounter->dispvalue [i]);
    uiBoxPackStart (&hbox, &bpmcounter->dispvalue [i]);
  }

  /* right side */
  uiCreateVertBox (&vbox);
  uiWidgetAlignHorizCenter (&vbox);
  uiWidgetAlignVertCenter (&vbox);
  uiBoxPackStartExpand (&hboxbpm, &vbox);

  /* blue box */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiutilsUICallbackInit (&bpmcounter->callbacks [BPMCOUNT_CB_CLICK],
      bpmcounterProcessClick, bpmcounter);
  uiCreateButton (&uiwidget,
      &bpmcounter->callbacks [BPMCOUNT_CB_CLICK],
      NULL, "bluebox");
  uiButtonSetReliefNone (&uiwidget);
  uiButtonSetFlat (&uiwidget);
  uiWidgetDisableFocus (&uiwidget);
  uiWidgetSetAllMargins (&uiwidget, 0);
  uiBoxPackEnd (&hbox, &uiwidget);

  /* buttons */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vboxmain, &hbox);

  uiutilsUICallbackInit (&bpmcounter->callbacks [BPMCOUNT_CB_SAVE],
      bpmcounterProcessSave, bpmcounter);
  uiCreateButton (&uiwidget,
      &bpmcounter->callbacks [BPMCOUNT_CB_SAVE],
      /* CONTEXT: bpm counter: save button */
      _("Save"), NULL);
  uiWidgetSetMarginTop (&uiwidget, uiBaseMarginSz * 2);
  uiBoxPackEnd (&hbox, &uiwidget);

  uiutilsUICallbackInit (&bpmcounter->callbacks [BPMCOUNT_CB_RESET],
      bpmcounterProcessReset, bpmcounter);
  uiCreateButton (&uiwidget,
      &bpmcounter->callbacks [BPMCOUNT_CB_RESET],
      /* CONTEXT: bpm counter: reset button */
      _("Reset"), NULL);
  uiWidgetSetMarginTop (&uiwidget, uiBaseMarginSz * 2);
  uiBoxPackEnd (&hbox, &uiwidget);

  uiCreateButton (&uiwidget,
      &bpmcounter->callbacks [BPMCOUNT_CB_EXIT],
      /* CONTEXT: bpm counter: close button */
      _("Close"), NULL);
  uiWidgetSetMarginTop (&uiwidget, uiBaseMarginSz * 2);
  uiBoxPackEnd (&hbox, &uiwidget);

  x = nlistGetNum (bpmcounter->options, BPMCOUNTER_POSITION_X);
  y = nlistGetNum (bpmcounter->options, BPMCOUNTER_POSITION_Y);
  uiWindowMove (&bpmcounter->window, x, y, -1);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", ".png", PATHBLD_MP_IMGDIR);
  osuiSetIcon (imgbuff);

  uiWidgetShowAll (&bpmcounter->window);

  logProcEnd (LOG_PROC, "bpmcounterBuildUI", "");
}

static int
bpmcounterMainLoop (void *tbpmcounter)
{
  bpmcounter_t  *bpmcounter = tbpmcounter;
  int           stop = FALSE;

  if (! stop) {
    uiUIProcessEvents ();
  }

  if (! progstateIsRunning (bpmcounter->progstate)) {
    progstateProcess (bpmcounter->progstate);
    if (progstateCurrState (bpmcounter->progstate) == STATE_CLOSED) {
      stop = true;
    }
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      progstateShutdownProcess (bpmcounter->progstate);
      gKillReceived = 0;
    }
    return stop;
  }

  connProcessUnconnected (bpmcounter->conn);

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (bpmcounter->progstate);
    gKillReceived = 0;
  }
  return stop;
}

static int
bpmcounterProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  bpmcounter_t       *bpmcounter = udata;

  logProcBegin (LOG_PROC, "bpmcounterProcessMsg");

  logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
      routefrom, msgRouteDebugText (routefrom),
      route, msgRouteDebugText (route), msg, msgDebugText (msg), args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_BPM_COUNTER: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (bpmcounter->conn, routefrom);
          break;
        }
        case MSG_SOCKET_CLOSE: {
          procutilCloseProcess (bpmcounter->processes [routefrom],
              bpmcounter->conn, routefrom);
          procutilFreeRoute (bpmcounter->processes, routefrom);
          bpmcounter->processes [routefrom] = NULL;
          connDisconnect (bpmcounter->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          progstateShutdownProcess (bpmcounter->progstate);
          gKillReceived = 0;
          break;
        }
        case MSG_BPM_TIMESIG: {
          bpmcounterProcessTimesig (bpmcounter, args);
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

  logProcEnd (LOG_PROC, "bpmcounterProcessMsg", "");

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  return gKillReceived;
}

static bool
bpmcounterCloseCallback (void *udata)
{
  bpmcounter_t   *bpmcounter = udata;

  if (progstateCurrState (bpmcounter->progstate) <= STATE_RUNNING) {
    progstateShutdownProcess (bpmcounter->progstate);
    logMsg (LOG_DBG, LOG_MSGS, "got: close win request");
  }

  return UICB_STOP;
}

static void
bpmcounterSigHandler (int sig)
{
  gKillReceived = 1;
}

static bool
bpmcounterProcessSave (void *udata)
{
  bpmcounter_t  *bpmcounter = udata;
  char          tbuff [40];

  snprintf (tbuff, sizeof (tbuff), "%d",
      bpmcounter->values [bpmcounter->timesigidx]);
  connSendMessage (bpmcounter->conn, ROUTE_MANAGEUI, MSG_BPM_SET, tbuff);
  return UICB_CONT;
}

static bool
bpmcounterProcessReset (void *udata)
{
  bpmcounter_t *bpmcounter = udata;

  for (int i = 0; i < BPMCOUNT_DISP_MAX; ++i) {
    uiLabelSetText (&bpmcounter->dispvalue [i], "");
    bpmcounter->values [i] = 0;
  }
  bpmcounter->count = 0;
  bpmcounter->begtime = 0;

  return UICB_CONT;
}

static bool
bpmcounterProcessClick (void *udata)
{
  bpmcounter_t  *bpmcounter = udata;
  double        dval;
  time_t        lasttime;
  time_t        endtime;
  time_t        tottime;
  double        elapsed;
  char          tbuff [40];

  ++bpmcounter->count;
  lasttime = mstime ();
  endtime = lasttime;
  if (bpmcounter->begtime == 0) {
    bpmcounter->begtime = lasttime;
  }

  tottime = endtime - bpmcounter->begtime;

  /* adjust for the time in the first beat */
  if (bpmcounter->count > 1) {
    time_t        extra;

    extra = (time_t) round ((double) tottime / (double) (bpmcounter->count - 1));
    tottime += extra;
  }

  elapsed = round ((double) tottime / 1000.0 * 10.0) / 10.0;

  if (bpmcounter->count > 1) {
    dval = (double) bpmcounter->count / (double) tottime;
    dval *= 1000.0;
    dval = round (dval * 60.0);

    bpmcounter->values [BPMCOUNT_DISP_BPM] = (int) dval;
    snprintf (tbuff, sizeof (tbuff), "%d", (int) dval);
    uiLabelSetText (&bpmcounter->dispvalue [BPMCOUNT_DISP_BPM], tbuff);

    bpmcounter->values [BPMCOUNT_DISP_MPM_24] = (int) (dval / 2.0);
    snprintf (tbuff, sizeof (tbuff), "%d",
        bpmcounter->values [BPMCOUNT_DISP_MPM_24]);
    uiLabelSetText (&bpmcounter->dispvalue [BPMCOUNT_DISP_MPM_24], tbuff);

    bpmcounter->values [BPMCOUNT_DISP_MPM_34] = (int) (dval / 3.0);
    snprintf (tbuff, sizeof (tbuff), "%d",
        bpmcounter->values [BPMCOUNT_DISP_MPM_34]);
    uiLabelSetText (&bpmcounter->dispvalue [BPMCOUNT_DISP_MPM_34], tbuff);

    bpmcounter->values [BPMCOUNT_DISP_MPM_44] = (int) (dval / 4.0);
    snprintf (tbuff, sizeof (tbuff), "%d",
        bpmcounter->values [BPMCOUNT_DISP_MPM_44]);
    uiLabelSetText (&bpmcounter->dispvalue [BPMCOUNT_DISP_MPM_44], tbuff);
  }

  /* these are always displayed */
  snprintf (tbuff, sizeof (tbuff), "%d", bpmcounter->count);
  uiLabelSetText (&bpmcounter->dispvalue [BPMCOUNT_DISP_BEATS], tbuff);
  snprintf (tbuff, sizeof (tbuff), "%.1f", elapsed);
  uiLabelSetText (&bpmcounter->dispvalue [BPMCOUNT_DISP_SECONDS], tbuff);

  return UICB_CONT;
}

static void
bpmcounterProcessTimesig (bpmcounter_t *bpmcounter, char *args)
{
  char    *p;
  char    *tokstr;
  int     bpmsel;
  int     timesig;
  int     idx;

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  bpmsel = atoi (p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  timesig = atoi (p);

  if (bpmsel == BPM_BPM) {
    idx = 0;
  } else {
    idx = timesig + 1;
  }
  idx += BPMCOUNT_DISP_BPM;
  bpmcounter->timesigidx = idx;
  uiToggleButtonSetState (&bpmcounter->timesigsel [idx], 1);
}

static bool
bpmcounterRadioChanged (void *udata)
{
  bpmcounter_t  *bpmcounter = udata;

  /* gtk radio buttons are not very friendly */
  for (int i = BPMCOUNT_DISP_BPM; i < BPMCOUNT_DISP_MAX; ++i) {
    if (uiToggleButtonIsActive (&bpmcounter->timesigsel [i])) {
      bpmcounter->timesigidx = i;
      break;
    }
  }
  return UICB_CONT;
}
