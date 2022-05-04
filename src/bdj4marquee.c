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

#include <gtk/gtk.h>

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
#include "uiutils.h"

enum {
  MQ_POSITION_X,
  MQ_POSITION_Y,
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
};

typedef struct {
  progstate_t     *progstate;
  char            *locknm;
  conn_t          *conn;
  datafile_t      *optiondf;
  nlist_t         *options;
  GtkWidget       *window;
  GtkWidget       *vbox;
  GtkWidget       *pbar;
  GtkWidget       *danceLab;
  GtkWidget       *countdownTimerLab;
  GtkWidget       *infoBox;
  GtkWidget       *infoArtistLab;
  GtkWidget       *infoSepLab;
  GtkWidget       *infoTitleLab;
  GtkWidget       *sep;
  GtkWidget       **marqueeLabs;
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
#define MARQUEE_UNMAX_WAIT_COUNT  3

static bool     marqueeConnectingCallback (void *udata, programstate_t programState);
static bool     marqueeHandshakeCallback (void *udata, programstate_t programState);
static bool     marqueeStoppingCallback (void *udata, programstate_t programState);
static bool     marqueeClosingCallback (void *udata, programstate_t programState);
static void     marqueeBuildUI (marquee_t *marquee);
static int      marqueeMainLoop  (void *tmarquee);
static int      marqueeProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static gboolean marqueeCloseWin (GtkWidget *window, GdkEvent *event, gpointer userdata);
static gboolean marqueeToggleFullscreen (GtkWidget *window,
                    GdkEventButton *event, gpointer userdata);
static void     marqueeSetMaximized (marquee_t *marquee);
static void     marqueeSetNotMaximized (marquee_t *marquee);
static void     marqueeSetNotMaximizeFinish (marquee_t *marquee);
static void     marqueeSendMaximizeState (marquee_t *marquee);
static gboolean marqueeWinState (GtkWidget *window, GdkEventWindowState *event,
                    gpointer userdata);
static gboolean marqueeWinMapped (GtkWidget *window, GdkEventAny *event,
                    gpointer userdata);
static void marqueeSaveWindowPosition (marquee_t *);
static void marqueeMoveWindow (marquee_t *);
static void marqueeStateChg (GtkWidget *w, GtkStateType flags, gpointer userdata);
static void marqueeSigHandler (int sig);
static void marqueeSetFontSize (marquee_t *marquee, GtkWidget *lab, char *font);
static void marqueePopulate (marquee_t *marquee, char *args);
static void marqueeSetTimer (marquee_t *marquee, char *args);
static void marqueeSetFont (marquee_t *marquee, int sz);
static void marqueeDisplayCompletion (marquee_t *marquee);

static int gKillReceived = 0;
static int gdone = 0;

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
  progstateSetCallback (marquee.progstate, STATE_CLOSING,
      marqueeClosingCallback, &marquee);

  marquee.window = NULL;
  marquee.pbar = NULL;
  marquee.danceLab = NULL;
  marquee.countdownTimerLab = NULL;
  marquee.infoArtistLab = NULL;
  marquee.infoSepLab = NULL;
  marquee.infoTitleLab = NULL;
  marquee.sep = NULL;
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
      "marquee", ".txt", PATHBLD_MP_USEIDX);
  marquee.optiondf = datafileAllocParse ("marquee-opt", DFTYPE_KEY_VAL, tbuff,
      mqdfkeys, MQ_KEY_MAX, DATAFILE_NO_LOOKUP);
  marquee.options = datafileGetList (marquee.optiondf);
  if (marquee.options == NULL) {
    marquee.options = nlistAlloc ("marquee-opt", LIST_ORDERED, free);

    nlistSetNum (marquee.options, MQ_POSITION_X, -1);
    nlistSetNum (marquee.options, MQ_POSITION_Y, -1);
    nlistSetNum (marquee.options, MQ_SIZE_X, 600);
    nlistSetNum (marquee.options, MQ_SIZE_Y, 600);
    nlistSetNum (marquee.options, MQ_FONT_SZ, 36);
    nlistSetNum (marquee.options, MQ_FONT_SZ_FS, 60);
  }

  uiutilsUIInitialize ();
  mqfont = bdjoptGetStr (OPT_MP_MQFONT);
  uiutilsSetUIFont (mqfont);

  marqueeBuildUI (&marquee);
  sockhMainLoop (listenPort, marqueeProcessMsg, marqueeMainLoop, &marquee);

  progstateFree (marquee.progstate);
  logEnd ();
  return status;
}

/* internal routines */

static bool
marqueeStoppingCallback (void *udata, programstate_t programState)
{
  marquee_t     *marquee = udata;
  gint          x, y;

  logProcBegin (LOG_PROC, "marqueeStoppingCallback");

  if (marquee->isMaximized) {
    marqueeSetNotMaximized (marquee);
    logProcEnd (LOG_PROC, "marqueeStoppingCallback", "is-maximized-a");
    return false;
  }

  if (uiutilsWindowIsMaximized (marquee->window)) {
    logProcEnd (LOG_PROC, "marqueeStoppingCallback", "is-maximized-b");
    return false;
  }

  uiutilsWindowGetSize (marquee->window, &x, &y);
  nlistSetNum (marquee->options, MQ_SIZE_X, x);
  nlistSetNum (marquee->options, MQ_SIZE_Y, y);
  if (! marquee->isIconified) {
    marqueeSaveWindowPosition (marquee);
  }

  connDisconnectAll (marquee->conn);
  gdone = 1;
  logProcEnd (LOG_PROC, "marqueeStoppingCallback", "");
  return true;
}

static bool
marqueeClosingCallback (void *udata, programstate_t programState)
{
  marquee_t   *marquee = udata;
  char        fn [MAXPATHLEN];

  logProcBegin (LOG_PROC, "marqueeClosingCallback");

  /* these are moved here so that the window can be un-maximized and */
  /* the size/position saved */
  uiutilsCloseMainWindow (marquee->window);

  pathbldMakePath (fn, sizeof (fn),
      "marquee", ".txt", PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("marquee", fn, mqdfkeys, MQ_KEY_MAX, marquee->options);

  bdj4shutdown (ROUTE_MARQUEE, NULL);

  connDisconnectAll (marquee->conn);
  connFree (marquee->conn);

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
  return true;
}

static void
marqueeBuildUI (marquee_t *marquee)
{
  char      imgbuff [MAXPATHLEN];
  char      tbuff [MAXPATHLEN];
  GtkWidget *window;
  GtkWidget *hbox;
  GtkWidget *vbox;
  gint      x, y;

  logProcBegin (LOG_PROC, "marqueeBuildUI");

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_marquee", ".svg", PATHBLD_MP_IMGDIR);

  window = uiutilsCreateMainWindow (_("Marquee"), imgbuff,
      marqueeCloseWin, marquee);
  g_signal_connect (window, "button-press-event", G_CALLBACK (marqueeToggleFullscreen), marquee);
  g_signal_connect (window, "window-state-event", G_CALLBACK (marqueeWinState), marquee);
  g_signal_connect (window, "map-event", G_CALLBACK (marqueeWinMapped), marquee);
  /* the backdrop window state must be intercepted */
  g_signal_connect (window, "state-flags-changed", G_CALLBACK (marqueeStateChg), marquee);
  uiutilsWindowNoFocusOnStartup (window);
  marquee->window = window;

  x = nlistGetNum (marquee->options, MQ_SIZE_X);
  y = nlistGetNum (marquee->options, MQ_SIZE_Y);
  uiutilsWindowSetDefaultSize (window, x, y);

  marquee->window = window;

  marquee->vbox = uiutilsCreateVertBox ();
  uiutilsWidgetSetAllMargins (marquee->vbox, 10);
  uiutilsBoxPackInWindow (window, marquee->vbox);
  uiutilsWidgetExpandHoriz (marquee->vbox);
  uiutilsWidgetExpandVert (marquee->vbox);
  marquee->marginTotal = 20;

  marquee->pbar = uiutilsCreateProgressBar (bdjoptGetStr (OPT_P_MQ_ACCENT_COL));
  uiutilsBoxPackStart (marquee->vbox, marquee->pbar);

  vbox = uiutilsCreateVertBox ();
  uiutilsWidgetSetAllMargins (marquee->vbox, 10);
  uiutilsWidgetExpandHoriz (marquee->vbox);
  uiutilsBoxPackStart (marquee->vbox, vbox);

  hbox = uiutilsCreateHorizBox ();
  uiutilsWidgetAlignHorizFill (hbox);
  uiutilsWidgetExpandHoriz (hbox);
  uiutilsBoxPackStart (vbox, hbox);

  /* CONTEXT: marquee: displayed when nothing is set to be played */
  marquee->danceLab = uiutilsCreateLabel (_("Not Playing"));
  uiutilsWidgetAlignHorizStart (marquee->danceLab);
  uiutilsWidgetDisableFocus (marquee->danceLab);
  snprintf (tbuff, sizeof (tbuff),
      "label { color: %s; }",
      bdjoptGetStr (OPT_P_MQ_ACCENT_COL));
  uiutilsSetCss (marquee->danceLab, tbuff);
  uiutilsBoxPackStart (hbox, marquee->danceLab);

  marquee->countdownTimerLab = uiutilsCreateLabel ("0:00");
  uiutilsLabelSetMaxWidth (marquee->countdownTimerLab, 6);
  uiutilsWidgetAlignHorizEnd (marquee->countdownTimerLab);
  uiutilsWidgetDisableFocus (marquee->countdownTimerLab);
  snprintf (tbuff, sizeof (tbuff),
      "label { color: %s; }",
      bdjoptGetStr (OPT_P_MQ_ACCENT_COL));
  uiutilsSetCss (marquee->countdownTimerLab, tbuff);
  uiutilsBoxPackEnd (hbox, marquee->countdownTimerLab);

  hbox = uiutilsCreateHorizBox ();
  uiutilsWidgetAlignHorizFill (hbox);
  uiutilsWidgetExpandHoriz (hbox);
  uiutilsBoxPackStart (vbox, hbox);
  marquee->infoBox = hbox;

  marquee->infoArtistLab = uiutilsCreateLabel ("");
  uiutilsWidgetAlignHorizStart (marquee->infoArtistLab);
  uiutilsWidgetExpandHoriz (hbox);
  uiutilsWidgetDisableFocus (marquee->infoArtistLab);
  uiutilsLabelEllipsizeOn (marquee->infoArtistLab);
  uiutilsBoxPackStart (hbox, marquee->infoArtistLab);

  marquee->infoSepLab = uiutilsCreateLabel ("");
  uiutilsWidgetAlignHorizStart (marquee->infoSepLab);
  uiutilsWidgetDisableFocus (marquee->infoSepLab);
  uiutilsBoxPackStart (hbox, marquee->infoSepLab);

  marquee->infoTitleLab = uiutilsCreateLabel ("");
  uiutilsWidgetAlignHorizStart (marquee->infoTitleLab);
  uiutilsWidgetExpandHoriz (hbox);
  uiutilsWidgetDisableFocus (marquee->infoTitleLab);
  uiutilsLabelEllipsizeOn (marquee->infoTitleLab);
  uiutilsBoxPackStart (hbox, marquee->infoTitleLab);

  marquee->sep = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  uiutilsWidgetExpandHoriz (marquee->sep);
  uiutilsWidgetSetMarginTop (marquee->sep, uiutilsBaseMarginSz);
  snprintf (tbuff, sizeof (tbuff),
      "separator { min-height: 4px; background-color: %s; }",
      bdjoptGetStr (OPT_P_MQ_ACCENT_COL));
  uiutilsSetCss (marquee->sep, tbuff);
  uiutilsBoxPackEnd (vbox, marquee->sep);

  marquee->marqueeLabs = malloc (sizeof (GtkWidget *) * marquee->mqLen);

  for (int i = 0; i < marquee->mqLen; ++i) {
    marquee->marqueeLabs [i] = uiutilsCreateLabel ("");
    uiutilsWidgetAlignHorizStart (marquee->marqueeLabs [i]);
    uiutilsWidgetExpandHoriz (marquee->marqueeLabs [i]);
    gtk_widget_set_margin_end (marquee->marqueeLabs [i], 10);
    uiutilsWidgetDisableFocus (marquee->marqueeLabs [i]);
    uiutilsBoxPackStart (marquee->vbox, marquee->marqueeLabs [i]);
  }

  marqueeSetFont (marquee, nlistGetNum (marquee->options, MQ_FONT_SZ));

  if (marquee->hideonstart) {
    uiutilsWindowIconify (window);
    marquee->isIconified = true;
  }

  if (! marquee->mqShowInfo) {
    uiutilsWidgetHide (marquee->infoBox);
  }
  uiutilsWidgetShowAll (window);

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

  if (gdone > EXIT_WAIT_COUNT) {
    stop = TRUE;
  }

  if (! stop) {
    uiutilsUIProcessEvents ();
  }

  if (gdone) {
    ++gdone;
  }

  connProcessUnconnected (marquee->conn);

  if (! progstateIsRunning (marquee->progstate)) {
    progstateProcess (marquee->progstate);
    if (! gdone && gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      progstateShutdownProcess (marquee->progstate);
    }
    return stop;
  }

  if (marquee->unMaximize == 1) {
    marqueeSetNotMaximizeFinish (marquee);
    marquee->unMaximize = 0;
  }
  if (marquee->unMaximize > 0) {
    --marquee->unMaximize;
  }

  if (! gdone && gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (marquee->progstate);
  }
  return stop;
}

static bool
marqueeConnectingCallback (void *udata, programstate_t programState)
{
  marquee_t   *marquee = udata;
  bool        rc = false;

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
    rc = true;
  }

  logProcEnd (LOG_PROC, "marqueeConnectingCallback", "");
  return rc;
}

static bool
marqueeHandshakeCallback (void *udata, programstate_t programState)
{
  marquee_t   *marquee = udata;
  bool        rc = false;

  logProcBegin (LOG_PROC, "marqueeHandshakeCallback");

  connProcessUnconnected (marquee->conn);

  if (connHaveHandshake (marquee->conn, ROUTE_MAIN) &&
      connHaveHandshake (marquee->conn, ROUTE_PLAYERUI)) {
    char    tbuff [100];

    rc = true;
    snprintf (tbuff, sizeof (tbuff), "%ld%c%ld",
        nlistGetNum (marquee->options, MQ_FONT_SZ),
        MSG_ARGS_RS,
        nlistGetNum (marquee->options, MQ_FONT_SZ_FS));
    connSendMessage (marquee->conn, ROUTE_PLAYERUI,
        MSG_MARQUEE_FONT_SIZES, tbuff);
  }

  logProcEnd (LOG_PROC, "marqueeHandshakeCallback", "");
  return rc;
}

static int
marqueeProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  marquee_t       *marquee = udata;

  logProcBegin (LOG_PROC, "marqueeProcessMsg");

  logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
      routefrom, msgRouteDebugText (routefrom),
      route, msgRouteDebugText (route), msg, msgDebugText (msg), args);

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
          logProcEnd (LOG_PROC, "marqueeProcessMsg", "req-exit");
          return 1;
        }
        case MSG_MARQUEE_DATA: {
          marqueePopulate (marquee, args);
          break;
        }
        case MSG_MARQUEE_TIMER: {
          marqueeSetTimer (marquee, args);
          break;
        }
        case MSG_MARQUEE_SET_FONT_SZ: {
          marqueeSetFont (marquee, atoi (args));
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

  logProcEnd (LOG_PROC, "marqueeProcessMsg", "");

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  logProcEnd (LOG_PROC, "marqueeProcessMsg", "");
  return gKillReceived;
}


static gboolean
marqueeCloseWin (GtkWidget *window, GdkEvent *event, gpointer userdata)
{
  marquee_t   *marquee = userdata;

  logProcBegin (LOG_PROC, "marqueeCloseWin");

  if (! gdone) {
    marqueeSaveWindowPosition (marquee);

    uiutilsWindowIconify (window);
    marquee->mqIconifyAction = true;
    marquee->isIconified = true;
    logProcEnd (LOG_PROC, "marqueeCloseWin", "user-close-win");
    return TRUE;
  }

  logProcEnd (LOG_PROC, "marqueeCloseWin", "");
  return FALSE;
}

static gboolean
marqueeToggleFullscreen (GtkWidget *window, GdkEventButton *event, gpointer userdata)
{
  marquee_t   *marquee = userdata;

  logProcBegin (LOG_PROC, "marqueeToggleFullscreen");

  if (gdk_event_get_event_type ((GdkEvent *) event) != GDK_DOUBLE_BUTTON_PRESS) {
    logProcEnd (LOG_PROC, "marqueeToggleFullscreen", "no-button-press");
    return FALSE;
  }

  marquee->userDoubleClicked = true;
  if (marquee->isMaximized) {
    marqueeSetNotMaximized (marquee);
  } else {
    marqueeSetMaximized (marquee);
  }

  logProcEnd (LOG_PROC, "marqueeToggleFullscreen", "");
  return FALSE;
}

static void
marqueeSetMaximized (marquee_t *marquee)
{
  if (marquee->isMaximized) {
    return;
  }

  marquee->isMaximized = true;
  if (! isWindows()) {
    /* does not work on windows platforms */
    uiutilsWindowDisableDecorations (marquee->window);
  }
  uiutilsWindowMaximize (marquee->window);
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
}

static void
marqueeSetNotMaximizeFinish (marquee_t *marquee)
{
  marquee->setPrior = true;
  uiutilsWindowUnMaximize (marquee->window);
  if (! isWindows()) {
    /* does not work on windows platforms */
    uiutilsWindowEnableDecorations (marquee->window);
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

static gboolean
marqueeWinState (GtkWidget *window, GdkEventWindowState *event, gpointer userdata)
{
  marquee_t         *marquee = userdata;

  logProcBegin (LOG_PROC, "marqueeWinState");


  if (event->changed_mask == GDK_WINDOW_STATE_ICONIFIED) {
    if (marquee->mqIconifyAction) {
      marquee->mqIconifyAction = false;
      logProcEnd (LOG_PROC, "marqueeWinState", "close-button");
      return FALSE;
    }

    if ((event->new_window_state & GDK_WINDOW_STATE_ICONIFIED) !=
        GDK_WINDOW_STATE_ICONIFIED) {
      marquee->isIconified = false;
      marqueeMoveWindow (marquee);
    } else {
      marqueeSaveWindowPosition (marquee);
      marquee->isIconified = true;
    }
    logProcEnd (LOG_PROC, "marqueeWinState", "iconified/deiconified");
    return FALSE;
  }
  if (event->changed_mask == GDK_WINDOW_STATE_MAXIMIZED) {
    /* if the user double-clicked, this is a known maximize change and */
    /* no processing needs to be done here */
    if (marquee->userDoubleClicked) {
      marquee->userDoubleClicked = false;
      logProcEnd (LOG_PROC, "marqueeWinState", "user-double-clicked");
      return FALSE;
    }

    /* user selected the maximize button */
    if ((event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) ==
        GDK_WINDOW_STATE_MAXIMIZED) {
      marqueeSetMaximized (marquee);
    }
  }

  logProcEnd (LOG_PROC, "marqueeWinState", "");
  return FALSE;
}

static gboolean
marqueeWinMapped (GtkWidget *window, GdkEventAny *event, gpointer userdata)
{
  marquee_t         *marquee = userdata;

  logProcBegin (LOG_PROC, "marqueeWinMapped");

  if (! marquee->isMaximized && ! marquee->isIconified) {
    marqueeMoveWindow (marquee);
  }

  logProcEnd (LOG_PROC, "marqueeWinMapped", "");
  return TRUE;
}

static void
marqueeSaveWindowPosition (marquee_t *marquee)
{
  gint  x, y;

  uiutilsWindowGetPosition (marquee->window, &x, &y);
  nlistSetNum (marquee->options, MQ_POSITION_X, x);
  nlistSetNum (marquee->options, MQ_POSITION_Y, y);
}

static void
marqueeMoveWindow (marquee_t *marquee)
{
  gint  x, y;

  x = nlistGetNum (marquee->options, MQ_POSITION_X);
  y = nlistGetNum (marquee->options, MQ_POSITION_Y);
  uiutilsWindowMove (marquee->window, x, y);
}

static void
marqueeStateChg (GtkWidget *window, GtkStateType flags, gpointer userdata)
{
  logProcBegin (LOG_PROC, "marqueeStateChg");
  /* the marquee is never in a backdrop state */
  gtk_widget_unset_state_flags (window, GTK_STATE_FLAG_BACKDROP);
  logProcEnd (LOG_PROC, "marqueeStateChg", "");
}

static void
marqueeSigHandler (int sig)
{
  gKillReceived = 1;
}

static void
marqueeSetFontSize (marquee_t *marquee, GtkWidget *lab, char *font)
{
  PangoFontDescription  *font_desc;
  PangoAttribute        *attr;
  PangoAttrList         *attrlist;

  logProcBegin (LOG_PROC, "marqueeSetFontSize");

  if (lab == NULL) {
    logProcEnd (LOG_PROC, "marqueeSetFontSize", "no-lab");
    return;
  }

  attrlist = pango_attr_list_new ();
  font_desc = pango_font_description_from_string (font);
  attr = pango_attr_font_desc_new (font_desc);
  pango_attr_list_insert (attrlist, attr);
  gtk_label_set_attributes (GTK_LABEL (lab), attrlist);
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
    uiutilsWidgetHide (marquee->infoBox);
  }

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokptr);
  if (marquee->infoArtistLab != NULL) {
    if (p != NULL && *p == MSG_ARGS_EMPTY) {
      p = "";
    }
    if (p != NULL && *p != '\0') {
      ++showsep;
    }
    uiutilsLabelSetText (marquee->infoArtistLab, p);
  }

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  if (marquee->infoTitleLab != NULL) {
    if (p != NULL && *p == MSG_ARGS_EMPTY) {
      p = "";
    }
    if (p != NULL && *p != '\0') {
      ++showsep;
    }
    uiutilsLabelSetText (marquee->infoTitleLab, p);
  }

  if (marquee->infoSepLab != NULL) {
    if (showsep == 2) {
      uiutilsLabelSetText (marquee->infoSepLab, "/ ");
    } else {
      uiutilsLabelSetText (marquee->infoSepLab, "");
    }
  }

  /* first entry is the main dance */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  if (p != NULL && *p == MSG_ARGS_EMPTY) {
    p = "";
  }
  uiutilsLabelSetText (marquee->danceLab, p);

  for (int i = 0; i < marquee->mqLen; ++i) {
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
    if (p != NULL && *p != MSG_ARGS_EMPTY) {
      uiutilsLabelSetText (marquee->marqueeLabs [i], p);
    } else {
      uiutilsLabelSetText (marquee->marqueeLabs [i], "");
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
  uiutilsLabelSetText (marquee->countdownTimerLab, tbuff);

  ddur = (double) dur;
  dplayed = (double) played;
  if (dur != 0.0) {
    dratio = dplayed / ddur;
  } else {
    dratio = 0.0;
  }
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (marquee->pbar), dratio);
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

  marqueeSetFontSize (marquee, marquee->danceLab, tbuff);
  marqueeSetFontSize (marquee, marquee->countdownTimerLab, tbuff);

  snprintf (tbuff, sizeof (tbuff), "%s %d", fontname, sz);
  if (marquee->infoArtistLab != NULL) {
    marqueeSetFontSize (marquee, marquee->infoArtistLab, tbuff);
    marqueeSetFontSize (marquee, marquee->infoSepLab, tbuff);
    marqueeSetFontSize (marquee, marquee->infoTitleLab, tbuff);
  }

  for (int i = 0; i < marquee->mqLen; ++i) {
    marqueeSetFontSize (marquee, marquee->marqueeLabs [i], tbuff);
  }

  logProcEnd (LOG_PROC, "marqueeSetFont", "");
}

static void
marqueeDisplayCompletion (marquee_t *marquee)
{
  char  *disp;

  disp = bdjoptGetStr (OPT_P_COMPLETE_MSG);
  uiutilsLabelSetText (marquee->infoArtistLab, "");
  uiutilsLabelSetText (marquee->infoSepLab, "");
  uiutilsLabelSetText (marquee->infoTitleLab, disp);

  if (! marquee->mqShowInfo) {
    uiutilsWidgetShowAll (marquee->infoBox);
  }
}
