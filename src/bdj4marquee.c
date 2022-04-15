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
#include "pathbld.h"
#include "procutil.h"
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
  GtkApplication  *app;
  progstate_t     *progstate;
  char            *locknm;
  conn_t          *conn;
  sockserver_t    *sockserver;
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
} marquee_t;

#define MARQUEE_EXIT_WAIT_COUNT   20
#define INFO_LAB_HEIGHT_ADJUST    0.85
#define MARQUEE_UNMAX_WAIT_COUNT  3

static bool     marqueeConnectingCallback (void *udata, programstate_t programState);
static bool     marqueeHandshakeCallback (void *udata, programstate_t programState);
static bool     marqueeStoppingCallback (void *udata, programstate_t programState);
static bool     marqueeClosingCallback (void *udata, programstate_t programState);
static void     marqueeActivate (GApplication *app, gpointer userdata);
gboolean        marqueeMainLoop  (void *tmarquee);
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


  marquee.progstate = progstateInit ("marquee");
  progstateSetCallback (marquee.progstate, STATE_CONNECTING,
      marqueeConnectingCallback, &marquee);
  progstateSetCallback (marquee.progstate, STATE_WAIT_HANDSHAKE,
      marqueeHandshakeCallback, &marquee);
  progstateSetCallback (marquee.progstate, STATE_STOPPING,
      marqueeStoppingCallback, &marquee);
  progstateSetCallback (marquee.progstate, STATE_CLOSING,
      marqueeClosingCallback, &marquee);

  marquee.sockserver = NULL;
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

#if _define_SIGHUP
  procutilCatchSignal (marqueeSigHandler, SIGHUP);
#endif
  procutilCatchSignal (marqueeSigHandler, SIGINT);
  procutilDefaultSignal (SIGTERM);
#if _define_SIGCHLD
  procutilDefaultSignal (SIGCHLD);
#endif

  flags = BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  bdj4startup (argc, argv, "mq", ROUTE_MARQUEE, flags);

  listenPort = bdjvarsGetNum (BDJVL_MARQUEE_PORT);
  marquee.mqLen = bdjoptGetNum (OPT_P_MQQLEN);
  marquee.mqShowInfo = bdjoptGetNum (OPT_P_MQ_SHOW_INFO);
  marquee.conn = connInit (ROUTE_MARQUEE);

  pathbldMakePath (tbuff, sizeof (tbuff), "",
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

  marquee.sockserver = sockhStartServer (listenPort);
  g_timeout_add (UI_MAIN_LOOP_TIMER, marqueeMainLoop, &marquee);

  uiutilsInitGtkLog ();
  gtk_init (&argc, NULL);
  mqfont = bdjoptGetStr (OPT_MP_MQFONT);
  uiutilsSetUIFont (mqfont);

  status = uiutilsCreateApplication (0, NULL, "marquee",
      &marquee.app, marqueeActivate, &marquee);

  while (progstateShutdownProcess (marquee.progstate) != STATE_CLOSED) {
    mssleep (10);
  }
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

  if (gtk_window_is_maximized (GTK_WINDOW (marquee->window))) {
    logProcEnd (LOG_PROC, "marqueeStoppingCallback", "is-maximized-b");
    return false;
  }

  gtk_window_get_size (GTK_WINDOW (marquee->window), &x, &y);
  nlistSetNum (marquee->options, MQ_SIZE_X, x);
  nlistSetNum (marquee->options, MQ_SIZE_Y, y);
  if (! marquee->isIconified) {
    marqueeSaveWindowPosition (marquee);
  }

  connDisconnectAll (marquee->conn);
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
  if (GTK_IS_WIDGET (marquee->window)) {
    gtk_widget_destroy (marquee->window);
  }

  pathbldMakePath (fn, sizeof (fn), "",
      "marquee", ".txt", PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("marquee", fn, mqdfkeys, MQ_KEY_MAX, marquee->options);

  bdj4shutdown (ROUTE_MARQUEE);
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
marqueeActivate (GApplication *app, gpointer userdata)
{
  char      imgbuff [MAXPATHLEN];
  char      tbuff [MAXPATHLEN];
  marquee_t *marquee = userdata;
  GtkWidget *window;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GError    *gerr = NULL;
  gint      x, y;

  logProcBegin (LOG_PROC, "marqueeActivate");

  pathbldMakePath (imgbuff, sizeof (imgbuff), "",
      "bdj4_icon_marquee", ".svg", PATHBLD_MP_IMGDIR);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  assert (window != NULL);
  gtk_window_set_application (GTK_WINDOW (window), GTK_APPLICATION (app));
  g_signal_connect (window, "delete-event", G_CALLBACK (marqueeCloseWin), marquee);
  g_signal_connect (window, "button-press-event", G_CALLBACK (marqueeToggleFullscreen), marquee);
  g_signal_connect (window, "window-state-event", G_CALLBACK (marqueeWinState), marquee);
  g_signal_connect (window, "map-event", G_CALLBACK (marqueeWinMapped), marquee);
  /* the backdrop window state must be intercepted */
  g_signal_connect (window, "state-flags-changed", G_CALLBACK (marqueeStateChg), marquee);
  marquee->window = window;

  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_NORMAL);
  gtk_window_set_focus_on_map (GTK_WINDOW (window), FALSE);
  gtk_window_set_title (GTK_WINDOW (window), _("Marquee"));
  gtk_window_set_default_icon_from_file (imgbuff, &gerr);

  x = nlistGetNum (marquee->options, MQ_SIZE_X);
  y = nlistGetNum (marquee->options, MQ_SIZE_Y);
  gtk_window_set_default_size (GTK_WINDOW (window), x, y);

  marquee->window = window;

  marquee->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  gtk_widget_set_margin_top (marquee->vbox, 10);
  gtk_widget_set_margin_bottom (marquee->vbox, 10);
  gtk_container_add (GTK_CONTAINER (window), marquee->vbox);
  gtk_widget_set_hexpand (marquee->vbox, TRUE);
  gtk_widget_set_vexpand (marquee->vbox, TRUE);
  marquee->marginTotal = 20;

  marquee->pbar = gtk_progress_bar_new ();
  gtk_widget_set_halign (marquee->pbar, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (marquee->pbar, TRUE);
  snprintf (tbuff, sizeof (tbuff),
      "progress, trough { min-height: 25px; } progressbar > trough > progress { background-color: %s; }",
      (char *) bdjoptGetStr (OPT_P_MQ_ACCENT_COL));
  uiutilsSetCss (marquee->pbar, tbuff);
  gtk_box_pack_start (GTK_BOX (marquee->vbox), marquee->pbar,
      FALSE, FALSE, 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  gtk_widget_set_margin_start (marquee->vbox, 10);
  gtk_widget_set_margin_end (marquee->vbox, 10);
  gtk_widget_set_hexpand (marquee->vbox, TRUE);
  gtk_box_pack_start (GTK_BOX (marquee->vbox), vbox,
      FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_widget_set_halign (hbox, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_widget_set_margin_end (hbox, 0);
  gtk_widget_set_margin_start (hbox, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox,
      FALSE, FALSE, 0);

  marquee->danceLab = uiutilsCreateLabel (_("Not Playing"));
  gtk_widget_set_halign (marquee->danceLab, GTK_ALIGN_START);
  gtk_widget_set_hexpand (marquee->danceLab, TRUE);
  gtk_widget_set_can_focus (marquee->danceLab, FALSE);
  snprintf (tbuff, sizeof (tbuff),
      "label { color: %s; }",
      (char *) bdjoptGetStr (OPT_P_MQ_ACCENT_COL));
  uiutilsSetCss (marquee->danceLab, tbuff);
  gtk_box_pack_start (GTK_BOX (hbox), marquee->danceLab,
      TRUE, TRUE, 0);

  marquee->countdownTimerLab = uiutilsCreateLabel ("0:00");
  gtk_label_set_max_width_chars (GTK_LABEL (marquee->countdownTimerLab), 6);
  gtk_widget_set_halign (marquee->countdownTimerLab, GTK_ALIGN_END);
  gtk_widget_set_can_focus (marquee->countdownTimerLab, FALSE);
  snprintf (tbuff, sizeof (tbuff),
      "label { color: %s; }",
      (char *) bdjoptGetStr (OPT_P_MQ_ACCENT_COL));
  uiutilsSetCss (marquee->countdownTimerLab, tbuff);
  gtk_box_pack_end (GTK_BOX (hbox), marquee->countdownTimerLab,
      FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_widget_set_halign (hbox, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_widget_set_margin_end (hbox, 0);
  gtk_widget_set_margin_start (hbox, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox,
      FALSE, FALSE, 0);
  marquee->infoBox = hbox;

  marquee->infoArtistLab = uiutilsCreateLabel ("");
  gtk_widget_set_halign (marquee->infoArtistLab, GTK_ALIGN_START);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_widget_set_can_focus (marquee->infoArtistLab, FALSE);
  gtk_label_set_ellipsize (GTK_LABEL (marquee->infoArtistLab), PANGO_ELLIPSIZE_END);
  gtk_box_pack_start (GTK_BOX (hbox), marquee->infoArtistLab,
      FALSE, FALSE, 0);

  marquee->infoSepLab = uiutilsCreateLabel ("");
  gtk_widget_set_halign (marquee->infoSepLab, GTK_ALIGN_START);
  gtk_widget_set_hexpand (hbox, FALSE);
  gtk_widget_set_can_focus (marquee->infoSepLab, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), marquee->infoSepLab,
      FALSE, FALSE, 0);

  marquee->infoTitleLab = uiutilsCreateLabel ("");
  gtk_widget_set_halign (marquee->infoTitleLab, GTK_ALIGN_START);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_widget_set_can_focus (marquee->infoTitleLab, FALSE);
  gtk_label_set_ellipsize (GTK_LABEL (marquee->infoTitleLab), PANGO_ELLIPSIZE_END);
  gtk_box_pack_start (GTK_BOX (hbox), marquee->infoTitleLab, FALSE, FALSE, 0);

  marquee->sep = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_margin_top (marquee->sep, 2);
  snprintf (tbuff, sizeof (tbuff),
      "separator { min-height: 4px; background-color: %s; }",
      (char *) bdjoptGetStr (OPT_P_MQ_ACCENT_COL));
  uiutilsSetCss (marquee->sep, tbuff);
  gtk_box_pack_end (GTK_BOX (vbox), marquee->sep, TRUE, TRUE, 0);

  marquee->marqueeLabs = malloc (sizeof (GtkWidget *) * marquee->mqLen);

  for (int i = 0; i < marquee->mqLen; ++i) {
    marquee->marqueeLabs [i] = uiutilsCreateLabel ("");
    gtk_widget_set_halign (marquee->marqueeLabs [i], GTK_ALIGN_START);
    gtk_widget_set_hexpand (marquee->marqueeLabs [i], TRUE);
    gtk_widget_set_margin_end (marquee->marqueeLabs [i], 10);
    gtk_widget_set_can_focus (marquee->marqueeLabs [i], FALSE);
    gtk_box_pack_start (GTK_BOX (marquee->vbox),
        marquee->marqueeLabs [i], FALSE, FALSE, 0);
  }

  marqueeSetFont (marquee, nlistGetNum (marquee->options, MQ_FONT_SZ));

  if (bdjoptGetNum (OPT_P_HIDE_MARQUEE_ON_START)) {
    gtk_window_iconify (GTK_WINDOW (window));
    marquee->isIconified = true;
  }

  if (! marquee->mqShowInfo) {
    gtk_widget_hide (marquee->infoBox);
  }
  gtk_widget_show_all (window);

  marqueeMoveWindow (marquee);

  progstateLogTime (marquee->progstate, "time-to-start-gui");

  pathbldMakePath (imgbuff, sizeof (imgbuff), "",
      "bdj4_icon_marquee", ".png", PATHBLD_MP_IMGDIR);
  osuiSetIcon (imgbuff);

  logProcEnd (LOG_PROC, "marqueeActivate", "");
}

gboolean
marqueeMainLoop (void *tmarquee)
{
  marquee_t   *marquee = tmarquee;
  int         tdone = 0;
  gboolean    cont = TRUE;

  tdone = sockhProcessMain (marquee->sockserver, marqueeProcessMsg, marquee);
  if (tdone || gdone) {
    ++gdone;
  }
  if (gdone > MARQUEE_EXIT_WAIT_COUNT) {
    g_application_quit (G_APPLICATION (marquee->app));
    cont = FALSE;
  }

  if (! progstateIsRunning (marquee->progstate)) {
    progstateProcess (marquee->progstate);
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      cont = FALSE;
    }
    return cont;
  }

  if (marquee->unMaximize == 1) {
    marqueeSetNotMaximizeFinish (marquee);
    marquee->unMaximize = 0;
  }
  if (marquee->unMaximize > 0) {
    --marquee->unMaximize;
  }

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    cont = FALSE;
  }
  return cont;
}

static bool
marqueeConnectingCallback (void *udata, programstate_t programState)
{
  marquee_t   *marquee = udata;
  bool        rc = false;

  logProcBegin (LOG_PROC, "marqueeConnectingCallback");

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

  logMsg (LOG_DBG, LOG_MSGS, "got: from:%ld/%s route:%ld/%s msg:%ld/%s args:%s",
      routefrom, msgRouteDebugText (routefrom),
      route, msgRouteDebugText (route), msg, msgDebugText (msg), args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MARQUEE: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (marquee->conn, routefrom);
          connConnectResponse (marquee->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          logMsg (LOG_DBG, LOG_MSGS, "got: req-exit");
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

    gtk_window_iconify (GTK_WINDOW (window));
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
    gtk_window_set_decorated (GTK_WINDOW (marquee->window), FALSE);
  }
  gtk_window_maximize (GTK_WINDOW (marquee->window));
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
  gtk_window_unmaximize (GTK_WINDOW (marquee->window));
  if (! isWindows()) {
    /* does not work on windows platforms */
    gtk_window_set_decorated (GTK_WINDOW (marquee->window), TRUE);
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

  gtk_window_get_position (GTK_WINDOW (marquee->window), &x, &y);
  nlistSetNum (marquee->options, MQ_POSITION_X, x);
  nlistSetNum (marquee->options, MQ_POSITION_Y, y);
}

static void
marqueeMoveWindow (marquee_t *marquee)
{
  gint  x, y;

  x = nlistGetNum (marquee->options, MQ_POSITION_X);
  y = nlistGetNum (marquee->options, MQ_POSITION_Y);
  if (x != -1 && y != -1) {
    gtk_window_move (GTK_WINDOW (marquee->window), x, y);
  }
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
    gtk_widget_hide (marquee->infoBox);
  }

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokptr);
  if (marquee->infoArtistLab != NULL) {
    if (p != NULL && *p == MSG_ARGS_EMPTY) {
      p = "";
    }
    if (p != NULL && *p != '\0') {
      ++showsep;
    }
    gtk_label_set_label (GTK_LABEL (marquee->infoArtistLab), p);
  }

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  if (marquee->infoTitleLab != NULL) {
    if (p != NULL && *p == MSG_ARGS_EMPTY) {
      p = "";
    }
    if (p != NULL && *p != '\0') {
      ++showsep;
    }
    gtk_label_set_label (GTK_LABEL (marquee->infoTitleLab), p);
  }

  if (marquee->infoSepLab != NULL) {
    if (showsep == 2) {
      gtk_label_set_label (GTK_LABEL (marquee->infoSepLab), " / ");
    } else {
      gtk_label_set_label (GTK_LABEL (marquee->infoSepLab), "");
    }
  }

  /* first entry is the main dance */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  if (p != NULL && *p == MSG_ARGS_EMPTY) {
    p = "";
  }
  gtk_label_set_label (GTK_LABEL (marquee->danceLab), p);

  for (int i = 0; i < marquee->mqLen; ++i) {
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
    if (p != NULL && *p != MSG_ARGS_EMPTY) {
      gtk_label_set_label (GTK_LABEL (marquee->marqueeLabs [i]), p);
    } else {
      gtk_label_set_label (GTK_LABEL (marquee->marqueeLabs [i]), "");
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
  gtk_label_set_label (GTK_LABEL (marquee->countdownTimerLab), tbuff);

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
  gtk_label_set_label (GTK_LABEL (marquee->infoArtistLab), "");
  gtk_label_set_label (GTK_LABEL (marquee->infoSepLab), "");
  gtk_label_set_label (GTK_LABEL (marquee->infoTitleLab), disp);

  if (! marquee->mqShowInfo) {
    gtk_widget_show_all (marquee->infoBox);
  }
}
