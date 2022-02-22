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

#include <gtk/gtk.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
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
#include "tmutil.h"
#include "uiutils.h"

typedef struct {
  GtkApplication  *app;
  progstate_t     *progstate;
  char            *locknm;
  conn_t          *conn;
  sockserver_t    *sockserver;
  char            *mqfont;
  GtkWidget       *window;
  GtkWidget       *vbox;
  GtkWidget       *pbar;
  GtkWidget       *danceLab;
  GtkWidget       *countdownTimerLab;
  GtkWidget       *infoArtistLab;
  GtkWidget       *infoSepLab;
  GtkWidget       *infoTitleLab;
  GtkWidget       *sep;
  GtkWidget       **marqueeLabs;
  int             marginTotal;
  gulong          sizeSignal;
  gulong          unmaxSignal;
  int             newFontSize;
  int             setFontSize;
  double          fontAdjustment;
  int             mqLen;
  int             lastHeight;
  int             priorSize;
  bool            isMaximized : 1;
  bool            unMaximize : 1;
  bool            isIconified : 1;
  bool            inResize : 1;
  bool            userDoubleClicked : 1;
  bool            mqIconifyAction : 1;
  bool            inMax : 1;
  bool            setPrior : 1;
  bool            mqShowInfo : 1;
} marquee_t;

#define MARQUEE_EXIT_WAIT_COUNT   20
#define INFO_LAB_HEIGHT_ADJUST    0.85

static bool     marqueeConnectingCallback (void *udata, programstate_t programState);
static bool     marqueeHandshakeCallback (void *udata, programstate_t programState);
static bool     marqueeStoppingCallback (void *udata, programstate_t programState);
static bool     marqueeClosingCallback (void *udata, programstate_t programState);
static int      marqueeCreateGui (marquee_t *marquee, int argc, char *argv []);
static void     marqueeActivate (GApplication *app, gpointer userdata);
gboolean        marqueeMainLoop  (void *tmarquee);
static int      marqueeProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static gboolean marqueeCloseWin (GtkWidget *window, GdkEvent *event, gpointer userdata);
static gboolean marqueeToggleFullscreen (GtkWidget *window,
                    GdkEventButton *event, gpointer userdata);
static gboolean marqueeResized (GtkWidget *window, GdkEventConfigure *event,
                    gpointer userdata);
static gboolean marqueeWinState (GtkWidget *window, GdkEventWindowState *event,
                    gpointer userdata);
static void marqueeStateChg (GtkWidget *w, GtkStateType flags, gpointer userdata);
static void marqueeSigHandler (int sig);
static void marqueeSetFontSize (marquee_t *marquee, GtkWidget *lab, char *style, int sz);
static int  marqueeCalcFontSizes (marquee_t *marquee, int sz);
static void marqueeAdjustFontSizes (marquee_t *marquee, int sz);
static void marqueeAdjustFontCallback (GtkWidget *w, GtkAllocation *retAllocSize, gpointer userdata);
static void marqueePopulate (marquee_t *marquee, char *args);
static void marqueeSetTimer (marquee_t *marquee, char *args);
static void marqueeUnmaxCallback (GtkWidget *w, GtkAllocation *retAllocSize, gpointer userdata);

static int gKillReceived = 0;
static int gdone = 0;

int
main (int argc, char *argv[])
{
  int             status = 0;
  int             c = 0;
  int             rc = 0;
  int             option_index = 0;
  uint16_t        listenPort;
  loglevel_t      loglevel = LOG_IMPORTANT | LOG_MAIN;
  marquee_t       marquee;
  char            *tval;
  char            *uifont;

  static struct option bdj_options [] = {
    { "debug",      required_argument,  NULL,   'd' },
    { "profile",    required_argument,  NULL,   'p' },
    { "bdj4marquee",no_argument,        NULL,   0 },
    { "marquee",    no_argument,        NULL,   0 },
    { NULL,         0,                  NULL,   0 }
  };

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
  marquee.unMaximize = false;
  marquee.isIconified = false;
  marquee.inResize = false;
  marquee.userDoubleClicked = false;
  marquee.mqIconifyAction = false;
  marquee.inMax = false;
  marquee.setPrior = false;
  marquee.mqfont = "";
  marquee.marginTotal = 0;
  marquee.sizeSignal = 0;
  marquee.unmaxSignal = 0;
  marquee.newFontSize = 0;
  marquee.setFontSize = 0;
  marquee.fontAdjustment = 0.0;

#if _define_SIGHUP
  procutilCatchSignal (marqueeSigHandler, SIGHUP);
#endif
  procutilCatchSignal (marqueeSigHandler, SIGINT);
  procutilDefaultSignal (SIGTERM);
#if _define_SIGCHLD
  procutilDefaultSignal (SIGCHLD);
#endif

  sysvarsInit (argv[0]);
  localeInit ();

  while ((c = getopt_long_only (argc, argv, "p:d:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'd': {
        if (optarg) {
          loglevel = (loglevel_t) atoi (optarg);
        }
        break;
      }
      case 'p': {
        if (optarg) {
          sysvarsSetNum (SVL_BDJIDX, atol (optarg));
        }
        break;
      }
      default: {
        break;
      }
    }
  }

  logStartAppend ("bdj4marquee", "mq", loglevel);
  logMsg (LOG_SESS, LOG_IMPORTANT, "Using profile %ld", sysvarsGetNum (SVL_BDJIDX));

  marquee.locknm = lockName (ROUTE_MARQUEE);
  rc = lockAcquire (marquee.locknm, PATHBLD_MP_USEIDX);
  if (rc < 0) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: marquee: unable to acquire lock: profile: %zd", sysvarsGetNum (SVL_BDJIDX));
    logMsg (LOG_SESS, LOG_IMPORTANT, "ERR: marquee: unable to acquire lock: profile: %zd", sysvarsGetNum (SVL_BDJIDX));
    logEnd ();
    exit (0);
  }

  bdjvarsInit ();
  bdjoptInit ();

  listenPort = bdjvarsGetNum (BDJVL_MARQUEE_PORT);
  marquee.mqLen = bdjoptGetNum (OPT_P_MQQLEN);
  marquee.mqShowInfo = bdjoptGetNum (OPT_P_MQ_SHOW_INFO);
  marquee.conn = connInit (ROUTE_MARQUEE);

  tval = bdjoptGetData (OPT_MP_MQFONT);
  if (tval != NULL) {
    marquee.mqfont = strdup (tval);
  }
  if (marquee.mqfont == NULL) {
    marquee.mqfont = "";
  }

  marquee.sockserver = sockhStartServer (listenPort);
  g_timeout_add (UI_MAIN_LOOP_TIMER, marqueeMainLoop, &marquee);

  uiutilsInitGtkLog ();
  gtk_init (&argc, NULL);
  uifont = bdjoptGetData (OPT_MP_UIFONT);
  uiutilsSetUIFont (uifont);

  osuiSetWindowAsAccessory ();

  status = marqueeCreateGui (&marquee, 0, NULL);

  while (progstateShutdownProcess (marquee.progstate) != STATE_CLOSED) {
    ;
  }
  progstateFree (marquee.progstate);
  logEnd ();
  return status;
}

/* internal routines */

static bool
marqueeStoppingCallback (void *udata, programstate_t programState)
{
  marquee_t   *marquee = udata;

  connDisconnectAll (marquee->conn);
  return true;
}

static bool
marqueeClosingCallback (void *udata, programstate_t programState)
{
  marquee_t   *marquee = udata;

  connFree (marquee->conn);

  sockhCloseServer (marquee->sockserver);
  bdjoptFree ();
  bdjvarsCleanup ();

  lockRelease (marquee->locknm, PATHBLD_MP_USEIDX);

  if (marquee->mqfont != NULL && *marquee->mqfont != '\0') {
    free (marquee->mqfont);
  }
  if (marquee->marqueeLabs != NULL) {
    free (marquee->marqueeLabs);
  }

  return true;
}

static int
marqueeCreateGui (marquee_t *marquee, int argc, char *argv [])
{
  int             status;

  marquee->app = gtk_application_new (
      "org.bdj4.BDJ4.marquee",
      G_APPLICATION_NON_UNIQUE
  );
  g_signal_connect (marquee->app, "activate", G_CALLBACK (marqueeActivate), marquee);

  status = g_application_run (G_APPLICATION (marquee->app), argc, argv);
  gtk_widget_destroy (marquee->window);
  g_object_unref (marquee->app);
  return status;
}

static void
marqueeActivate (GApplication *app, gpointer userdata)
{
  char      imgbuff [MAXPATHLEN];
  marquee_t *marquee = userdata;
  GtkWidget *window;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GError    *gerr;

  pathbldMakePath (imgbuff, sizeof (imgbuff), "",
      "bdj4_icon_marquee", ".svg", PATHBLD_MP_IMGDIR);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  assert (window != NULL);
  gtk_window_set_application (GTK_WINDOW (window), GTK_APPLICATION (app));
  g_signal_connect (window, "delete-event", G_CALLBACK (marqueeCloseWin), marquee);
  g_signal_connect (window, "button-press-event", G_CALLBACK (marqueeToggleFullscreen), marquee);
  g_signal_connect (window, "configure-event", G_CALLBACK (marqueeResized), marquee);
  g_signal_connect (window, "window-state-event", G_CALLBACK (marqueeWinState), marquee);
  /* the backdrop window state must be intercepted */
  g_signal_connect (window, "state-flags-changed", G_CALLBACK (marqueeStateChg), marquee);

  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_NORMAL);
  gtk_window_set_focus_on_map (GTK_WINDOW (window), FALSE);
  gtk_window_set_title (GTK_WINDOW (window), _("Marquee"));
  gtk_window_set_default_icon_from_file (imgbuff, &gerr);
  gtk_window_set_default_size (GTK_WINDOW (window), 600, 600);
  marquee->window = window;

  marquee->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  gtk_widget_set_margin_top (GTK_WIDGET (marquee->vbox), 10);
  gtk_widget_set_margin_bottom (GTK_WIDGET (marquee->vbox), 10);
  gtk_container_add (GTK_CONTAINER (window), marquee->vbox);
  gtk_widget_set_hexpand (GTK_WIDGET (marquee->vbox), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (marquee->vbox), TRUE);
  marquee->marginTotal = 20;

  marquee->pbar = gtk_progress_bar_new ();
  gtk_widget_set_halign (GTK_WIDGET (marquee->pbar), GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (GTK_WIDGET (marquee->pbar), TRUE);
  uiutilsSetCss (GTK_WIDGET (marquee->pbar),
      "progress, trough { min-height: 25px; } progressbar > trough > progress { background-color: #ffa600; }");
  gtk_box_pack_start (GTK_BOX (marquee->vbox), GTK_WIDGET (marquee->pbar),
      FALSE, FALSE, 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  gtk_widget_set_margin_start (GTK_WIDGET (marquee->vbox), 10);
  gtk_widget_set_margin_end (GTK_WIDGET (marquee->vbox), 10);
  gtk_widget_set_hexpand (GTK_WIDGET (marquee->vbox), TRUE);
  gtk_box_pack_start (GTK_BOX (marquee->vbox), GTK_WIDGET (vbox),
      FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_widget_set_halign (GTK_WIDGET (hbox), GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
  gtk_widget_set_margin_end (GTK_WIDGET (hbox), 0);
  gtk_widget_set_margin_start (GTK_WIDGET (hbox), 0);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox),
      FALSE, FALSE, 0);

  marquee->danceLab = gtk_label_new (_("Not Playing"));
  gtk_widget_set_margin_top (GTK_WIDGET (marquee->danceLab), 2);
  gtk_widget_set_halign (GTK_WIDGET (marquee->danceLab), GTK_ALIGN_START);
  gtk_widget_set_hexpand (GTK_WIDGET (marquee->danceLab), TRUE);
  gtk_widget_set_can_focus (GTK_WIDGET (marquee->danceLab), FALSE);
  uiutilsSetCss (GTK_WIDGET (marquee->danceLab),
      "label { color: #ffa600; }");
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (marquee->danceLab),
      TRUE, TRUE, 0);

  marquee->countdownTimerLab = gtk_label_new ("0:00");
  gtk_widget_set_margin_top (GTK_WIDGET (marquee->countdownTimerLab), 2);
  gtk_label_set_max_width_chars (GTK_LABEL (marquee->countdownTimerLab), 6);
  gtk_widget_set_halign (GTK_WIDGET (marquee->countdownTimerLab), GTK_ALIGN_END);
  gtk_widget_set_can_focus (GTK_WIDGET (marquee->countdownTimerLab), FALSE);
  uiutilsSetCss (GTK_WIDGET (marquee->countdownTimerLab),
      "label { color: #ffa600; }");
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (marquee->countdownTimerLab),
      FALSE, FALSE, 0);

  if (marquee->mqShowInfo) {
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_halign (GTK_WIDGET (hbox), GTK_ALIGN_FILL);
    gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
    gtk_widget_set_margin_end (GTK_WIDGET (hbox), 0);
    gtk_widget_set_margin_start (GTK_WIDGET (hbox), 0);
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox),
        FALSE, FALSE, 0);

    marquee->infoArtistLab = gtk_label_new ("");
    gtk_widget_set_margin_top (GTK_WIDGET (marquee->infoArtistLab), 2);
    gtk_widget_set_halign (GTK_WIDGET (marquee->infoArtistLab), GTK_ALIGN_START);
    gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
    gtk_widget_set_can_focus (GTK_WIDGET (marquee->infoArtistLab), FALSE);
    gtk_label_set_ellipsize (GTK_LABEL (marquee->infoArtistLab), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (marquee->infoArtistLab),
        FALSE, FALSE, 0);

    marquee->infoSepLab = gtk_label_new ("");
    gtk_widget_set_margin_top (GTK_WIDGET (marquee->infoSepLab), 2);
    gtk_widget_set_halign (GTK_WIDGET (marquee->infoSepLab), GTK_ALIGN_START);
    gtk_widget_set_hexpand (GTK_WIDGET (hbox), FALSE);
    gtk_widget_set_can_focus (GTK_WIDGET (marquee->infoSepLab), FALSE);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (marquee->infoSepLab),
        FALSE, FALSE, 0);

    marquee->infoTitleLab = gtk_label_new ("");
    gtk_widget_set_margin_top (GTK_WIDGET (marquee->infoTitleLab), 2);
    gtk_widget_set_halign (GTK_WIDGET (marquee->infoTitleLab), GTK_ALIGN_START);
    gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
    gtk_widget_set_can_focus (GTK_WIDGET (marquee->infoTitleLab), FALSE);
    gtk_label_set_ellipsize (GTK_LABEL (marquee->infoArtistLab), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (marquee->infoTitleLab),
        FALSE, FALSE, 0);
  }

  marquee->sep = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_margin_top (GTK_WIDGET (marquee->sep), 2);
  uiutilsSetCss (GTK_WIDGET (marquee->sep),
      "separator { min-height: 4px; background-color: #ffa600; }");
  gtk_box_pack_end (GTK_BOX (vbox), GTK_WIDGET (marquee->sep),
      TRUE, TRUE, 0);

  marquee->marqueeLabs = malloc (sizeof (GtkWidget *) * marquee->mqLen);

  for (int i = 0; i < marquee->mqLen; ++i) {
    marquee->marqueeLabs [i] = gtk_label_new ("");
    gtk_widget_set_halign (GTK_WIDGET (marquee->marqueeLabs [i]), GTK_ALIGN_START);
    gtk_widget_set_hexpand (GTK_WIDGET (marquee->marqueeLabs [i]), TRUE);
    gtk_widget_set_margin_top (GTK_WIDGET (marquee->marqueeLabs [i]), 2);
    gtk_widget_set_margin_start (GTK_WIDGET (marquee->marqueeLabs [i]), 10);
    gtk_widget_set_margin_end (GTK_WIDGET (marquee->marqueeLabs [i]), 10);
    gtk_widget_set_can_focus (GTK_WIDGET (marquee->marqueeLabs [i]), FALSE);
    gtk_box_pack_start (GTK_BOX (marquee->vbox),
        GTK_WIDGET (marquee->marqueeLabs [i]), FALSE, FALSE, 0);
  }

  marquee->inResize = true;
  gtk_widget_show_all (GTK_WIDGET (window));
  marquee->inResize = false;

  marqueeAdjustFontSizes (marquee, 0);
  progstateLogTime (marquee->progstate, "time-to-start-gui");

  pathbldMakePath (imgbuff, sizeof (imgbuff), "",
      "bdj4_icon_marquee", ".png", PATHBLD_MP_IMGDIR);
  osuiSetIcon (imgbuff);
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

  if (marquee->setFontSize != 0) {
    marquee->inResize = true;
    marqueeSetFontSize (marquee, marquee->danceLab, "bold", marquee->setFontSize);
    marqueeSetFontSize (marquee, marquee->countdownTimerLab, "bold", marquee->setFontSize);
    if (marquee->infoArtistLab != NULL) {
      marqueeSetFontSize (marquee, marquee->infoArtistLab, "", (int) (marquee->setFontSize * INFO_LAB_HEIGHT_ADJUST));
      marqueeSetFontSize (marquee, marquee->infoSepLab, "", (int) (marquee->setFontSize * INFO_LAB_HEIGHT_ADJUST));
      marqueeSetFontSize (marquee, marquee->infoTitleLab, "", (int) (marquee->setFontSize * INFO_LAB_HEIGHT_ADJUST));
    }

    for (int i = 0; i < marquee->mqLen; ++i) {
      marqueeSetFontSize (marquee, marquee->marqueeLabs [i], "", marquee->setFontSize);
    }

    marquee->setFontSize = 0;
    if (marquee->unMaximize) {
      marquee->unmaxSignal = g_signal_connect (marquee->marqueeLabs [marquee->mqLen - 1],
          "size-allocate", G_CALLBACK (marqueeUnmaxCallback), marquee);
    }
    marquee->inResize = false;
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

  if (! connIsConnected (marquee->conn, ROUTE_MAIN)) {
    connConnect (marquee->conn, ROUTE_MAIN);
  }

  if (connIsConnected (marquee->conn, ROUTE_MAIN)) {
    rc = true;
  }

  return rc;
}

static bool
marqueeHandshakeCallback (void *udata, programstate_t programState)
{
  marquee_t   *marquee = udata;
  bool        rc = false;


  if (connHaveHandshake (marquee->conn, ROUTE_MAIN)) {
    rc = true;
  }

  return rc;
}

static int
marqueeProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  marquee_t       *marquee = udata;

  logProcBegin (LOG_PROC, "marqueeProcessMsg");

  logMsg (LOG_DBG, LOG_MSGS, "got: from %d route: %d msg:%d args:%s",
      routefrom, route, msg, args);

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
        case MSG_MARQUEE_SHOW: {
          if (marquee->isIconified) {
            gtk_widget_show (GTK_WIDGET (marquee->window));
            marquee->mqIconifyAction = true;
            marquee->isIconified = false;
          }
          break;
        }
        case MSG_MARQUEE_DATA: {
          marqueePopulate (marquee, args);
          break;
        }
        case MSG_MARQUEE_TIMER: {
          marqueeSetTimer (marquee, args);
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
  return gKillReceived;
}


static gboolean
marqueeCloseWin (GtkWidget *window, GdkEvent *event, gpointer userdata)
{
  marquee_t   *marquee = userdata;

  if (! gdone) {
    gtk_window_iconify (GTK_WINDOW (window));
    marquee->mqIconifyAction = true;
    marquee->isIconified = true;
    return TRUE;
  }

  return FALSE;
}

static gboolean
marqueeToggleFullscreen (GtkWidget *window, GdkEventButton *event, gpointer userdata)
{
  marquee_t   *marquee = userdata;

  if (gdk_event_get_event_type ((GdkEvent *) event) != GDK_DOUBLE_BUTTON_PRESS) {
    return FALSE;
  }

  marquee->userDoubleClicked = true;
  if (marquee->isMaximized) {
    marquee->inMax = true;
    marquee->isMaximized = false;
    marqueeAdjustFontSizes (marquee, marquee->priorSize);
    marquee->setPrior = true;
    gtk_window_unmaximize (GTK_WINDOW (window));
    marquee->unMaximize = true;
    marquee->inMax = false;
    gtk_window_set_decorated (GTK_WINDOW (window), TRUE);
  } else {
    marquee->inMax = true;
    marquee->isMaximized = true;
    gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
    marquee->inMax = false;
    gtk_window_maximize (GTK_WINDOW (window));
  }

  return FALSE;
}

/* resize gets called multiple times; it's difficult to get the font      */
/* size reset back to the normal.  there are all sorts of little glitches */
/* and race conditions that are possible.  this code will very fragile    */
static gboolean
marqueeResized (GtkWidget *window, GdkEventConfigure *event, gpointer userdata)
{
  marquee_t   *marquee = userdata;
  int         sz = 0;
  gint        eheight;

  if (marquee->inMax) {
    return FALSE;
  }
  if (marquee->inResize) {
    return FALSE;
  }

  eheight = event->height;
  if (eheight == marquee->lastHeight) {
    return FALSE;
  }

  if (marquee->setPrior) {
    sz = marquee->priorSize;
    marquee->setPrior = false;
  }
  marqueeAdjustFontSizes (marquee, sz);
  marquee->lastHeight = eheight;

  return FALSE;
}

static gboolean
marqueeWinState (GtkWidget *window, GdkEventWindowState *event, gpointer userdata)
{
  marquee_t         *marquee = userdata;


  if (event->changed_mask == GDK_WINDOW_STATE_ICONIFIED) {
    if (marquee->mqIconifyAction) {
      marquee->mqIconifyAction = false;
      return FALSE;
    }

    if ((event->new_window_state & GDK_WINDOW_STATE_ICONIFIED) !=
        GDK_WINDOW_STATE_ICONIFIED) {
      marquee->isIconified = false;
      return FALSE;
    }
  }
  if (event->changed_mask == GDK_WINDOW_STATE_MAXIMIZED) {
    /* if the user double-clicked, this is a know maximize change and */
    /* no processing needs to be done here */
    if (marquee->userDoubleClicked) {
      marquee->userDoubleClicked = false;
      return FALSE;
    }

    /* user selected the maximize button */
    if ((event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) ==
        GDK_WINDOW_STATE_MAXIMIZED) {
      marquee->isMaximized = true;
      marquee->inMax = true;
      gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
      marquee->inMax = false;
      /* I don't know why this is necessary; did the event propogation stop? */
      gtk_window_maximize (GTK_WINDOW (window));
    }
  }

  return FALSE;
}

static void
marqueeStateChg (GtkWidget *window, GtkStateType flags, gpointer userdata)
{
  /* the marquee is never in a backdrop state */
  gtk_widget_unset_state_flags (GTK_WIDGET (window), GTK_STATE_FLAG_BACKDROP);
}

static void
marqueeSigHandler (int sig)
{
  gKillReceived = 1;
}

static void
marqueeSetFontSize (marquee_t *marquee, GtkWidget *lab, char *style, int sz)
{
  PangoFontDescription  *font_desc;
  PangoAttribute        *attr;
  PangoAttrList         *attrlist;
  char                  tbuff [200];


  if (lab == NULL) {
    return;
  }

  attrlist = pango_attr_list_new ();
  snprintf (tbuff, sizeof (tbuff), "%s %s", marquee->mqfont, style);
  font_desc = pango_font_description_from_string (tbuff);
  pango_font_description_set_absolute_size (font_desc, sz * PANGO_SCALE);
  attr = pango_attr_font_desc_new (font_desc);
  pango_attr_list_insert (attrlist, attr);
  gtk_label_set_attributes (GTK_LABEL (lab), attrlist);
}

/* getting this to work correctly is a pain */
static int
marqueeCalcFontSizes (marquee_t *marquee, int sz)
{
  int             newsza;
  gint            width;
  gint            height;
  int             pbarHeight;
  int             sepHeight;
  int             margin;
  int             infoHeight = 0;
  GtkAllocation   allocSize;
  int             newsz;


  gtk_window_get_size (GTK_WINDOW (marquee->window), &width, &height);

  if (marquee->pbar == NULL || marquee->danceLab == NULL) {
    return 0;
  }

  gtk_widget_get_allocation (GTK_WIDGET (marquee->pbar), &allocSize);
  pbarHeight = allocSize.height;
  gtk_widget_get_allocation (GTK_WIDGET (marquee->sep), &allocSize);
  sepHeight = allocSize.height;
  if (marquee->mqShowInfo) {
    gtk_widget_get_allocation (GTK_WIDGET (marquee->infoArtistLab), &allocSize);
    infoHeight = allocSize.height;
  }
  margin = gtk_widget_get_margin_top (GTK_WIDGET (marquee->danceLab));
  margin += gtk_box_get_spacing (GTK_BOX (marquee->vbox));

  marquee->newFontSize = sz;

  if (sz == 0) {
    int     numitems;
    int     numtextitems;

    /* pbar, dance, info, sep, mqlen */
    numitems = 3 + marquee->mqShowInfo + marquee->mqLen + 2;
    /* dance, mqlen */
    numtextitems = 1 + marquee->mqLen;

    /* 40 is extra space needed so the marquee can be shrunk */
    newsza = (height - 40 - marquee->marginTotal - pbarHeight - sepHeight -
        infoHeight - (margin * numitems)) / numtextitems;
    infoHeight = (int) (newsza * INFO_LAB_HEIGHT_ADJUST);
    newsz = (height - 40 - marquee->marginTotal - pbarHeight - sepHeight -
        infoHeight - (margin * numitems)) / numtextitems;
    if (newsz > newsza) {
      newsz = newsza;
    }
  } else {
    /* the old size to restore from before being maximized */
    newsz = sz;
  }

  return newsz;
}

static void
marqueeAdjustFontSizes (marquee_t *marquee, int sz)
{
  int   newsz = 0;

  if (marquee->inResize) {
    return;
  }

  marquee->inResize = true;

  newsz = marqueeCalcFontSizes (marquee, sz);

  /* only set the size of the dance label and the info label */
  /* the callback will determine the true size, and make adjustments */
  marqueeSetFontSize (marquee, marquee->danceLab, "bold", newsz);
  marqueeSetFontSize (marquee, marquee->infoArtistLab, "", (int) (newsz * INFO_LAB_HEIGHT_ADJUST));

  marquee->sizeSignal = g_signal_connect (marquee->danceLab, "size-allocate",
      G_CALLBACK (marqueeAdjustFontCallback), marquee);
  marquee->inResize = false;
}

static void
marqueeAdjustFontCallback (GtkWidget *w, GtkAllocation *retAllocSize, gpointer userdata)
{
  int             newsz;
  marquee_t       *marquee = userdata;
  double          dnewsz;
  double          dheight;

  if (marquee->inResize) {
    return;
  }
  if (marquee->sizeSignal == 0) {
    /* this trap is necessary */
    return;
  }

  marquee->inResize = true;
  g_signal_handler_disconnect (GTK_WIDGET (w), marquee->sizeSignal);
  marquee->sizeSignal = 0;

  newsz = marqueeCalcFontSizes (marquee, marquee->newFontSize);

  /* newsz is the maximum height available */
  /* given the allocation size, determine the actual size available */
  dnewsz = (double) newsz;
  if (marquee->fontAdjustment == 0.0) {
    dheight = (double) retAllocSize->height;
    marquee->fontAdjustment = (dnewsz / dheight);
  }
  dnewsz = round (dnewsz * marquee->fontAdjustment);
  newsz = (int) dnewsz;
  marquee->setFontSize = newsz;

  /* only save the prior size once */
  if (! marquee->isMaximized && marquee->priorSize == 0) {
    marquee->priorSize = newsz;
  }

  marquee->inResize = false;
}

static void
marqueePopulate (marquee_t *marquee, char *args)
{
  char      *p;
  char      *tokptr;
  int       showsep = 0;

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
}

static void
marqueeUnmaxCallback (GtkWidget *w, GtkAllocation *retAllocSize, gpointer userdata)
{
  marquee_t   *marquee = userdata;

  if (! marquee->unMaximize) {
    return;
  }
  if (marquee->unmaxSignal == 0) {
    return;
  }

  marquee->unMaximize = false;
  g_signal_handler_disconnect (GTK_WIDGET (w), marquee->unmaxSignal);
  marquee->unmaxSignal = 0;
  gtk_window_unmaximize (GTK_WINDOW (marquee->window));
}

