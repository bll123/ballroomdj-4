/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
#include <time.h>

#include "bdj4.h"
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjvars.h"
#include "callback.h"
#include "conn.h"
#include "dance.h"
#include "log.h"
#include "mdebug.h"
#include "ossignal.h"
#include "osuiutils.h"
#include "pathbld.h"
#include "procutil.h"
#include "progstate.h"
#include "sockh.h"
#include "sysvars.h"
#include "tmutil.h"
#include "ui.h"
#include "uiutils.h"

enum {
  BPMCOUNT_CB_EXIT,
  BPMCOUNT_CB_SAVE,
  BPMCOUNT_CB_RESET,
  BPMCOUNT_CB_CLICK,
  BPMCOUNT_CB_MAX,
};

enum {
  BPMCOUNT_DISP_BEATS,
  BPMCOUNT_DISP_SECONDS,
  BPMCOUNT_DISP_BPM,
  BPMCOUNT_DISP_MPM,
  BPMCOUNT_DISP_MAX,
};

enum {
  BPM_W_BUTTON_CLOSE,
  BPM_W_BUTTON_RESET,
  BPM_W_BUTTON_SAVE,
  BPM_W_BUTTON_BLUEBOX,
  BPM_W_WINDOW,
  BPM_W_MAX,
};

typedef struct {
  progstate_t     *progstate;
  const char      *disptxt [BPMCOUNT_DISP_MAX];
  procutil_t      *processes [ROUTE_MAX];
  conn_t          *conn;
  uiwcont_t       *dispvalue [BPMCOUNT_DISP_MAX];
  uiwcont_t       *dispwidget [BPMCOUNT_DISP_MAX];
  uiwcont_t       *wcont [BPM_W_MAX];
  int             values [BPMCOUNT_DISP_MAX];
  callback_t      *callbacks [BPMCOUNT_CB_MAX];
  int             stopwaitcount;
  int             count;
  time_t          begtime;
  int             timesig;
  /* options */
  datafile_t      *optiondf;
  nlist_t         *options;
  bool            optionsalloc;
} bpmcounter_t;

enum {
  BPMCOUNTER_POSITION_X,
  BPMCOUNTER_POSITION_Y,
  BPMCOUNTER_SIZE_X,
  BPMCOUNTER_SIZE_Y,
  BPMCOUNTER_KEY_MAX,
};

static datafilekey_t bpmcounteruidfkeys [BPMCOUNTER_KEY_MAX] = {
  { "BPMCOUNTER_POS_X",     BPMCOUNTER_POSITION_X,    VALUE_NUM, NULL, DF_NORM },
  { "BPMCOUNTER_POS_Y",     BPMCOUNTER_POSITION_Y,    VALUE_NUM, NULL, DF_NORM },
  { "BPMCOUNTER_SIZE_X",    BPMCOUNTER_SIZE_X,        VALUE_NUM, NULL, DF_NORM },
  { "BPMCOUNTER_SIZE_Y",    BPMCOUNTER_SIZE_Y,        VALUE_NUM, NULL, DF_NORM },
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

static int gKillReceived = 0;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  bpmcounter_t    bpmcounter;
  char            tbuff [MAXPATHLEN];
  uint32_t        flags;
  uisetup_t       uisetup;

#if BDJ4_MEM_DEBUG
  mdebugInit ("bpmc");
#endif

  /* CONTEXT: bpm counter: number of beats */
  bpmcounter.disptxt [BPMCOUNT_DISP_BEATS] = _("Beats");
  /* CONTEXT: bpm counter: number of seconds */
  bpmcounter.disptxt [BPMCOUNT_DISP_SECONDS] = _("Seconds");
  /* CONTEXT: bpm counter: BPM */
  bpmcounter.disptxt [BPMCOUNT_DISP_BPM] = _("BPM");
  /* CONTEXT: bpm counter: MPM  */
  bpmcounter.disptxt [BPMCOUNT_DISP_MPM] = _("MPM");

  bpmcounter.count = 0;
  bpmcounter.begtime = 0;
  bpmcounter.timesig = DANCE_TIMESIG_44;
  for (int i = 0; i < BPMCOUNT_DISP_MAX; ++i) {
    bpmcounter.values [i] = 0;
    bpmcounter.dispvalue [i] = NULL;
    bpmcounter.dispwidget [i] = NULL;
  }
  for (int i = 0; i < BPMCOUNT_CB_MAX; ++i) {
    bpmcounter.callbacks [i] = NULL;
  }
  for (int i = 0; i < BPM_W_MAX; ++i) {
    bpmcounter.wcont [i] = NULL;
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

  procutilInitProcesses (bpmcounter.processes);

  osSetStandardSignals (bpmcounterSigHandler);

  flags = BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  bdj4startup (argc, argv, NULL, "bpmc", ROUTE_BPM_COUNTER, &flags);
  logProcBegin ();

  listenPort = bdjvarsGetNum (BDJVL_PORT_BPM_COUNTER);
  bpmcounter.conn = connInit (ROUTE_BPM_COUNTER);

  pathbldMakePath (tbuff, sizeof (tbuff),
      BPMCOUNTER_OPT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  bpmcounter.optiondf = datafileAllocParse ("ui-bpmcounter", DFTYPE_KEY_VAL, tbuff,
      bpmcounteruidfkeys, BPMCOUNTER_KEY_MAX, DF_NO_OFFSET, NULL);
  bpmcounter.options = datafileGetList (bpmcounter.optiondf);
  bpmcounter.optionsalloc = false;
  if (bpmcounter.options == NULL || nlistGetCount (bpmcounter.options) == 0) {
    bpmcounter.optionsalloc = true;
    bpmcounter.options = nlistAlloc ("ui-bpmcounter", LIST_ORDERED, NULL);

    nlistSetNum (bpmcounter.options, BPMCOUNTER_POSITION_X, -1);
    nlistSetNum (bpmcounter.options, BPMCOUNTER_POSITION_Y, -1);
    nlistSetNum (bpmcounter.options, BPMCOUNTER_SIZE_X, 1200);
    nlistSetNum (bpmcounter.options, BPMCOUNTER_SIZE_Y, 800);
  }

  uiUIInitialize (sysvarsGetNum (SVL_LOCALE_DIR));
  uiutilsInitSetup (&uisetup);
  uiSetUICSS (&uisetup);

  bpmcounterBuildUI (&bpmcounter);
  osuiFinalize ();

  sockhMainLoop (listenPort, bpmcounterProcessMsg, bpmcounterMainLoop, &bpmcounter, SOCKH_LOCAL);
  connFree (bpmcounter.conn);
  progstateFree (bpmcounter.progstate);
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

  logProcBegin ();

  uiWindowGetSize (bpmcounter->wcont [BPM_W_WINDOW], &x, &y);
  nlistSetNum (bpmcounter->options, BPMCOUNTER_SIZE_X, x);
  nlistSetNum (bpmcounter->options, BPMCOUNTER_SIZE_Y, y);
  uiWindowGetPosition (bpmcounter->wcont [BPM_W_WINDOW], &x, &y, &ws);
  nlistSetNum (bpmcounter->options, BPMCOUNTER_POSITION_X, x);
  nlistSetNum (bpmcounter->options, BPMCOUNTER_POSITION_Y, y);

  connDisconnectAll (bpmcounter->conn);

  logProcEnd ("");
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

  logProcBegin ();
  uiCloseWindow (bpmcounter->wcont [BPM_W_WINDOW]);
  uiCleanup ();
  for (int i = 0; i < BPM_W_MAX; ++i) {
    uiwcontFree (bpmcounter->wcont [i]);
  }
  for (int i = 0; i < BPMCOUNT_CB_MAX; ++i) {
    callbackFree (bpmcounter->callbacks [i]);
  }
  for (int i = 0; i < BPMCOUNT_DISP_MAX; ++i) {
    uiwcontFree (bpmcounter->dispvalue [i]);
    uiwcontFree (bpmcounter->dispwidget [i]);
  }

  procutilFreeAll (bpmcounter->processes);

  datafileSave (bpmcounter->optiondf, NULL, bpmcounter->options, DF_NO_OFFSET, 1);

  bdj4shutdown (ROUTE_BPM_COUNTER, NULL);

  if (bpmcounter->optionsalloc) {
    nlistFree (bpmcounter->options);
  }
  datafileFree (bpmcounter->optiondf);

  logProcEnd ("");
  return STATE_FINISHED;
}

static void
bpmcounterBuildUI (bpmcounter_t  *bpmcounter)
{
  uiwcont_t   *uiwidgetp = NULL;
  uiwcont_t   *vboxmain;
  uiwcont_t   *vbox;
  uiwcont_t   *hboxbpm;
  uiwcont_t   *hbox;
  uiwcont_t   *szgrp;
  uiwcont_t   *szgrpDisp;
  char        imgbuff [MAXPATHLEN];
  int         x, y;
  uiutilsaccent_t accent;

  logProcBegin ();

  szgrp = uiCreateSizeGroupHoriz ();      // labels
  szgrpDisp = uiCreateSizeGroupHoriz ();     // display

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", BDJ4_IMG_SVG_EXT, PATHBLD_MP_DIR_IMG);
  bpmcounter->callbacks [BPMCOUNT_CB_EXIT] = callbackInit (
      bpmcounterCloseCallback, bpmcounter, NULL);
  bpmcounter->wcont [BPM_W_WINDOW] = uiCreateMainWindow (
      bpmcounter->callbacks [BPMCOUNT_CB_EXIT],
      /* CONTEXT: bpm counter: title of window*/
      _("BPM Counter"), imgbuff);

  vboxmain = uiCreateVertBox ();
  uiWindowPackInWindow (bpmcounter->wcont [BPM_W_WINDOW], vboxmain);
  uiWidgetSetAllMargins (vboxmain, 4);

  uiutilsAddProfileColorDisplay (vboxmain, &accent);
  hbox = accent.hbox;
  uiwcontFree (accent.cbox);

  /* instructions */
  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vboxmain, hbox);

  /* CONTEXT: bpm counter: instructions, line 1 */
  uiwidgetp = uiCreateLabel (_("Click the mouse in the blue box in time with the beat."));
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vboxmain, hbox);

  /* CONTEXT: bpm counter: instructions, line 2 */
  uiwidgetp = uiCreateLabel (_("When the BPM value is stable, select the Save button."));
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  /* secondary box */
  hboxbpm = uiCreateHorizBox ();
  uiBoxPackStartExpand (vboxmain, hboxbpm);

  /* left side */
  vbox = uiCreateVertBox ();
  uiBoxPackStartExpand (hboxbpm, vbox);

  /* some spacing */
  uiwidgetp = uiCreateLabel ("");
  uiBoxPackStart (vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  for (int i = 0; i < BPMCOUNT_DISP_MAX; ++i) {
    uiwcontFree (hbox);
    hbox = uiCreateHorizBox ();
    uiBoxPackStart (vbox, hbox);

    uiwidgetp = uiCreateColonLabel (bpmcounter->disptxt [i]);
    uiBoxPackStart (hbox, uiwidgetp);
    uiSizeGroupAdd (szgrp, uiwidgetp);
    bpmcounter->dispwidget [i] = uiwidgetp;

    bpmcounter->dispvalue [i] = uiCreateLabel ("");
    uiLabelAlignEnd (bpmcounter->dispvalue [i]);
    uiBoxPackStart (hbox, bpmcounter->dispvalue [i]);
    uiSizeGroupAdd (szgrpDisp, bpmcounter->dispvalue [i]);
  }

  /* right side */
  uiwcontFree (vbox);
  vbox = uiCreateVertBox ();
  uiBoxPackStartExpand (hboxbpm, vbox);
  uiWidgetAlignHorizCenter (vbox);
  uiWidgetAlignVertCenter (vbox);

  /* blue box */
  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  bpmcounter->callbacks [BPMCOUNT_CB_CLICK] = callbackInit (
      bpmcounterProcessClick, bpmcounter, NULL);
  uiwidgetp = uiCreateButton ("bpmc-box",
      bpmcounter->callbacks [BPMCOUNT_CB_CLICK],
      NULL, "bluebox");
  uiButtonSetReliefNone (uiwidgetp);
  uiButtonSetFlat (uiwidgetp);
  uiWidgetDisableFocus (uiwidgetp);
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetSetAllMargins (uiwidgetp, 0);
  bpmcounter->wcont [BPM_W_BUTTON_BLUEBOX] = uiwidgetp;

  /* buttons */
  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vboxmain, hbox);

  bpmcounter->callbacks [BPMCOUNT_CB_SAVE] = callbackInit (
      bpmcounterProcessSave, bpmcounter, NULL);
  uiwidgetp = uiCreateButton ("bpmc-save",
      bpmcounter->callbacks [BPMCOUNT_CB_SAVE],
      /* CONTEXT: bpm counter: save button */
      _("Save"), NULL);
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetSetMarginTop (uiwidgetp, 2);
  bpmcounter->wcont [BPM_W_BUTTON_SAVE] = uiwidgetp;

  bpmcounter->callbacks [BPMCOUNT_CB_RESET] = callbackInit (
      bpmcounterProcessReset, bpmcounter, NULL);
  uiwidgetp = uiCreateButton ("bpmc-reset",
      bpmcounter->callbacks [BPMCOUNT_CB_RESET],
      /* CONTEXT: bpm counter: reset button */
      _("Reset"), NULL);
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetSetMarginTop (uiwidgetp, 2);
  bpmcounter->wcont [BPM_W_BUTTON_RESET] = uiwidgetp;

  uiwidgetp = uiCreateButton ("bpmc-close",
      bpmcounter->callbacks [BPMCOUNT_CB_EXIT],
      /* CONTEXT: bpm counter: close button */
      _("Close"), NULL);
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetSetMarginTop (uiwidgetp, 2);
  bpmcounter->wcont [BPM_W_BUTTON_CLOSE] = uiwidgetp;

  x = nlistGetNum (bpmcounter->options, BPMCOUNTER_POSITION_X);
  y = nlistGetNum (bpmcounter->options, BPMCOUNTER_POSITION_Y);
  uiWindowMove (bpmcounter->wcont [BPM_W_WINDOW], x, y, -1);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", BDJ4_IMG_PNG_EXT, PATHBLD_MP_DIR_IMG);
  osuiSetIcon (imgbuff);

  uiWidgetShowAll (bpmcounter->wcont [BPM_W_WINDOW]);

  uiwcontFree (vboxmain);
  uiwcontFree (vbox);
  uiwcontFree (hboxbpm);
  uiwcontFree (hbox);
  uiwcontFree (szgrp);
  uiwcontFree (szgrpDisp);

  logProcEnd ("");
}

static int
bpmcounterMainLoop (void *tbpmcounter)
{
  bpmcounter_t  *bpmcounter = tbpmcounter;
  int           stop = SOCKH_CONTINUE;

  uiUIProcessEvents ();

  if (! progstateIsRunning (bpmcounter->progstate)) {
    progstateProcess (bpmcounter->progstate);
    if (progstateCurrState (bpmcounter->progstate) == STATE_CLOSED) {
      stop = SOCKH_STOP;
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

  logProcBegin ();

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
        case MSG_WINDOW_FIND: {
          uiWindowFind (bpmcounter->wcont [BPM_W_WINDOW]);
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

  logProcEnd ("");

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

  /* always return the MPM */
  snprintf (tbuff, sizeof (tbuff), "%d",
      bpmcounter->values [BPMCOUNT_DISP_MPM]);
  connSendMessage (bpmcounter->conn, ROUTE_MANAGEUI, MSG_BPM_SET, tbuff);
  bpmcounterCloseCallback (bpmcounter);
  return UICB_CONT;
}

static bool
bpmcounterProcessReset (void *udata)
{
  bpmcounter_t *bpmcounter = udata;

  for (int i = 0; i < BPMCOUNT_DISP_MAX; ++i) {
    uiLabelSetText (bpmcounter->dispvalue [i], "");
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
    uiLabelSetText (bpmcounter->dispvalue [BPMCOUNT_DISP_BPM], tbuff);

    bpmcounter->values [BPMCOUNT_DISP_MPM] =
        (int) (dval / (double) danceTimesigValues [bpmcounter->timesig]);
    snprintf (tbuff, sizeof (tbuff), "%d",
        bpmcounter->values [BPMCOUNT_DISP_MPM]);
    uiLabelSetText (bpmcounter->dispvalue [BPMCOUNT_DISP_MPM], tbuff);
  }

  /* these are always displayed */
  snprintf (tbuff, sizeof (tbuff), "%d", bpmcounter->count);
  uiLabelSetText (bpmcounter->dispvalue [BPMCOUNT_DISP_BEATS], tbuff);
  snprintf (tbuff, sizeof (tbuff), "%.1f", elapsed);
  uiLabelSetText (bpmcounter->dispvalue [BPMCOUNT_DISP_SECONDS], tbuff);

  return UICB_CONT;
}

static void
bpmcounterProcessTimesig (bpmcounter_t *bpmcounter, char *args)
{
  int     timesig = DANCE_TIMESIG_44;

  if (args != NULL && *args) {
    timesig = atoi (args);
  }
  if (timesig < 0) {
    timesig = DANCE_TIMESIG_44;
  }

  bpmcounter->timesig = timesig;
}

