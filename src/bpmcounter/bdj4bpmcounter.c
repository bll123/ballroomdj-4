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

#include "bdj4.h"
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjvars.h"
#include "conn.h"
#include "log.h"
#include "osutils.h"
#include "osuiutils.h"
#include "pathbld.h"
#include "procutil.h"
#include "progstate.h"
#include "sockh.h"
#include "ui.h"

enum {
  BPMCOUNT_CB_EXIT,
  BPMCOUNT_CB_SAVE,
  BPMCOUNT_CB_RESET,
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

static char *disp [BPMCOUNT_DISP_MAX];

typedef struct {
  progstate_t     *progstate;
  char            *locknm;
  procutil_t      *processes [ROUTE_MAX];
  conn_t          *conn;
  UIWidget        window;
  uitextbox_t     *inst;
  UIWidget        dispvalue [BPMCOUNT_DISP_MAX];
  UICallback      callbacks [BPMCOUNT_CB_MAX];
  int             stopwaitcount;
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

#define SUPPORT_BUFF_SZ         (10*1024*1024)
#define LOOP_DELAY              5

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


  disp [BPMCOUNT_DISP_BEATS] = _("Beats");
  disp [BPMCOUNT_DISP_SECONDS] = _("Seconds");
  disp [BPMCOUNT_DISP_BPM] = _("BPM");
  disp [BPMCOUNT_DISP_MPM_24] = _("MPM (2/4 time)");
  disp [BPMCOUNT_DISP_MPM_34] = _("MPM (3/4 time)");
  disp [BPMCOUNT_DISP_MPM_44] = _("MPM (4/4 or 4/8 time)");

  for (int i = 0; i < BPMCOUNT_DISP_MAX; ++i) {
    uiutilsUIWidgetInit (&bpmcounter.dispvalue [i]);
  }

  bpmcounter.inst = NULL;
  bpmcounter.stopwaitcount = 0;
  bpmcounter.progstate = progstateInit ("bpmcounter");

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
      BPMCOUNTER_OPT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_USEIDX);
  bpmcounter.optiondf = datafileAllocParse ("bpmcounter-opt", DFTYPE_KEY_VAL, tbuff,
      bpmcounteruidfkeys, BPMCOUNTER_KEY_MAX, DATAFILE_NO_LOOKUP);
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
  sockhMainLoop (listenPort, bpmcounterProcessMsg, bpmcounterMainLoop, &bpmcounter);
  connFree (bpmcounter.conn);
  progstateFree (bpmcounter.progstate);
  logProcEnd (LOG_PROC, "bpmcounter", "");
  logEnd ();
  return status;
}

/* internal routines */

static bool
bpmcounterStoppingCallback (void *udata, programstate_t programState)
{
  bpmcounter_t   *bpmcounter = udata;
  int         x, y;

  logProcBegin (LOG_PROC, "bpmcounterStoppingCallback");

  uiWindowGetSize (&bpmcounter->window, &x, &y);
  nlistSetNum (bpmcounter->options, BPMCOUNTER_SIZE_X, x);
  nlistSetNum (bpmcounter->options, BPMCOUNTER_SIZE_Y, y);
  uiWindowGetPosition (&bpmcounter->window, &x, &y);
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
      BPMCOUNTER_OPT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("bpmcounter", fn, bpmcounteruidfkeys, BPMCOUNTER_KEY_MAX, bpmcounter->options);

  uiTextBoxFree (bpmcounter->inst);
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
  uitextbox_t *tb;
  char        imgbuff [MAXPATHLEN];
  int         x, y;

  logProcBegin (LOG_PROC, "bpmcounterBuildUI");
  uiCreateSizeGroupHoriz (&sg);
  uiCreateSizeGroupHoriz (&sgb);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", ".svg", PATHBLD_MP_IMGDIR);
  uiutilsUICallbackInit (&bpmcounter->callbacks [BPMCOUNT_CB_EXIT],
      bpmcounterCloseCallback, bpmcounter);
  uiCreateMainWindow (&bpmcounter->window,
      &bpmcounter->callbacks [BPMCOUNT_CB_EXIT],
      BDJ4_LONG_NAME, imgbuff);

  uiCreateVertBox (&vboxmain);
  uiWidgetSetAllMargins (&vboxmain, uiBaseMarginSz * 2);
  uiBoxPackInWindow (&bpmcounter->window, &vboxmain);

  /* main display */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vboxmain, &hbox);

  /* instructions */
  tb = uiTextBoxCreate (80);
  uiTextBoxSetReadonly (tb);
  uiTextBoxSetHeight (tb, 70);
  uiBoxPackStartExpand (&hbox, uiTextBoxGetScrolledWindow (tb));
  bpmcounter->inst = tb;

  /* secondary box */
  uiCreateHorizBox (&hboxbpm);
  uiBoxPackStartExpand (&vboxmain, &hboxbpm);

  /* left side */
  uiCreateVertBox (&vbox);
  uiBoxPackStartExpand (&hboxbpm, &vbox);

  for (int i = 0; i < BPMCOUNT_DISP_MAX; ++i) {
    uiCreateHorizBox (&hbox);
    uiBoxPackStart (&vbox, &hbox);

    if (i < BPMCOUNT_DISP_BPM) {
      uiCreateColonLabel (&uiwidget, disp [i]);
    } else if (i == BPMCOUNT_DISP_BPM) {
      uiCreateRadioButton (&uiwidget, NULL, disp [i], 1);
      uiutilsUIWidgetCopy (&grpuiwidget, &uiwidget);
    } else {
      uiCreateRadioButton (&uiwidget, &grpuiwidget, disp [i], 0);
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
  uiBoxPackStartExpand (&hboxbpm, &vbox);

  /* blue box */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* buttons */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vboxmain, &hbox);

  uiutilsUICallbackInit (&bpmcounter->callbacks [BPMCOUNT_CB_SAVE],
      bpmcounterProcessSave, bpmcounter);
  uiCreateButton (&uiwidget,
      &bpmcounter->callbacks [BPMCOUNT_CB_SAVE],
      _("Save"), NULL);
  uiWidgetSetMarginTop (&uiwidget, uiBaseMarginSz * 2);
  uiBoxPackEnd (&hbox, &uiwidget);

  uiutilsUICallbackInit (&bpmcounter->callbacks [BPMCOUNT_CB_RESET],
      bpmcounterProcessReset, bpmcounter);
  uiCreateButton (&uiwidget,
      &bpmcounter->callbacks [BPMCOUNT_CB_RESET],
      _("Reset"), NULL);
  uiWidgetSetMarginTop (&uiwidget, uiBaseMarginSz * 2);
  uiBoxPackEnd (&hbox, &uiwidget);

  uiCreateButton (&uiwidget,
      &bpmcounter->callbacks [BPMCOUNT_CB_EXIT],
      _("Close"), NULL);
  uiWidgetSetMarginTop (&uiwidget, uiBaseMarginSz * 2);
  uiBoxPackEnd (&hbox, &uiwidget);

  x = nlistGetNum (bpmcounter->options, BPMCOUNTER_POSITION_X);
  y = nlistGetNum (bpmcounter->options, BPMCOUNTER_POSITION_Y);
  uiWindowMove (&bpmcounter->window, x, y);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", ".png", PATHBLD_MP_IMGDIR);
  osuiSetIcon (imgbuff);

  uiWidgetShowAll (&bpmcounter->window);

  uiTextBoxSetValue (bpmcounter->inst,
      _("Click the mouse in the blue box in time with the beat.  "
        "When the BPM value is stable, select the Save button."));

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
//  bpmcounter_t *bpmcounter = udata;

  return UICB_CONT;
}

static bool
bpmcounterProcessReset (void *udata)
{
//  bpmcounter_t *bpmcounter = udata;

  return UICB_CONT;
}

