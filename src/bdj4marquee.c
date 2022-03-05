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
#include "tmutil.h"
#include "uiutils.h"

enum {
  MQ_POSITION_X,
  MQ_POSITION_Y,
  MQ_SIZE_X,
  MQ_SIZE_Y,
};

static datafilekey_t mqdfkeys[] = {
  { "MQ_POS_X",     MQ_POSITION_X,    VALUE_NUM, NULL, -1 },
  { "MQ_POS_Y",     MQ_POSITION_Y,    VALUE_NUM, NULL, -1 },
  { "MQ_SIZE_X",    MQ_SIZE_X,        VALUE_NUM, NULL, -1 },
  { "MQ_SIZE_Y",    MQ_SIZE_Y,        VALUE_NUM, NULL, -1 },
};
#define MARQUEE_DFKEY_COUNT (sizeof (mqdfkeys) / sizeof (datafilekey_t))

typedef struct {
  GtkApplication  *app;
  progstate_t     *progstate;
  char            *locknm;
  conn_t          *conn;
  sockserver_t    *sockserver;
  char            *mqfont;
  datafile_t      *optiondf;
  nlist_t         *options;
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
static void     marqueeSetNotMaximized (marquee_t *marquee);
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
  char            tbuff [MAXPATHLEN];
  bool            ignorelock = false;


  static struct option bdj_options [] = {
    { "debug",      required_argument,  NULL,   'd' },
    { "profile",    required_argument,  NULL,   'p' },
    { "bdj4marquee",no_argument,        NULL,   0 },
    { "marquee",    no_argument,        NULL,   0 },
    { "nodetach",   no_argument,        NULL,   0 },
    { "ignorelock", no_argument,        NULL,   'i' },
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
      case 'i': {
        ignorelock = true;
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
    if (ignorelock == false) {
      logEnd ();
      exit (0);
    } else {
      logMsg (LOG_DBG, LOG_IMPORTANT, "marquee: lock ignored: profile: %zd", sysvarsGetNum (SVL_BDJIDX));
      logMsg (LOG_SESS, LOG_IMPORTANT, "marquee: lock ignored: profile: %zd", sysvarsGetNum (SVL_BDJIDX));
    }
  }

  bdjvarsInit ();
  bdjoptInit ();

  listenPort = bdjvarsGetNum (BDJVL_MARQUEE_PORT);
  marquee.mqLen = bdjoptGetNum (OPT_P_MQQLEN);
  marquee.mqShowInfo = bdjoptGetNum (OPT_P_MQ_SHOW_INFO);
  marquee.conn = connInit (ROUTE_MARQUEE);

  pathbldMakePath (tbuff, sizeof (tbuff), "",
      "marquee", ".txt", PATHBLD_MP_USEIDX);
  marquee.optiondf = datafileAllocParse ("marquee-opt", DFTYPE_KEY_VAL, tbuff,
      mqdfkeys, MARQUEE_DFKEY_COUNT, DATAFILE_NO_LOOKUP);
  marquee.options = datafileGetList (marquee.optiondf);
  if (marquee.options == NULL) {
    marquee.options = nlistAlloc ("marquee-opt", LIST_ORDERED, free);

    nlistSetNum (marquee.options, MQ_POSITION_X, -1);
    nlistSetNum (marquee.options, MQ_POSITION_Y, -1);
    nlistSetNum (marquee.options, MQ_SIZE_X, 600);
    nlistSetNum (marquee.options, MQ_SIZE_Y, 600);
  }

  tval = bdjoptGetStr (OPT_MP_MQFONT);
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
  uifont = bdjoptGetStr (OPT_MP_UIFONT);
  uiutilsSetUIFont (uifont);

  osuiSetWindowAsAccessory ();

  status = marqueeCreateGui (&marquee, 0, NULL);

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

  if (marquee->isMaximized) {
    marqueeSetNotMaximized (marquee);
    return false;
  }

  if (gtk_window_is_maximized (GTK_WINDOW (marquee->window))) {
    return false;
  }

  gtk_window_get_size (GTK_WINDOW (marquee->window), &x, &y);
  nlistSetNum (marquee->options, MQ_SIZE_X, x);
  nlistSetNum (marquee->options, MQ_SIZE_Y, y);
  if (! marquee->isIconified) {
    gtk_window_get_position (GTK_WINDOW (marquee->window), &x, &y);
    nlistSetNum (marquee->options, MQ_POSITION_X, x);
    nlistSetNum (marquee->options, MQ_POSITION_Y, y);
  }

  connDisconnectAll (marquee->conn);
  return true;
}

static bool
marqueeClosingCallback (void *udata, programstate_t programState)
{
  marquee_t   *marquee = udata;
  char        fn [MAXPATHLEN];

  /* these are moved here so that the window can be un-maximized and */
  /* the size/position saved */
  gtk_widget_destroy (marquee->window);
  g_object_unref (marquee->app);

  pathbldMakePath (fn, sizeof (fn), "",
      "marquee", ".txt", PATHBLD_MP_USEIDX);
  datafileSaveKeyVal (fn, mqdfkeys, MARQUEE_DFKEY_COUNT, marquee->options);

  sockhCloseServer (marquee->sockserver);

  connFree (marquee->conn);

  bdjoptFree ();
  bdjvarsCleanup ();

  lockRelease (marquee->locknm, PATHBLD_MP_USEIDX);

  if (marquee->mqfont != NULL && *marquee->mqfont != '\0') {
    free (marquee->mqfont);
  }
  if (marquee->marqueeLabs != NULL) {
    free (marquee->marqueeLabs);
  }
  if (marquee->options != NULL) {
    if (marquee->options != datafileGetList (marquee->optiondf)) {
      nlistFree (marquee->options);
    }
  }
  datafileFree (marquee->optiondf);

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

  return status;
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
  GError    *gerr;
  gint      x, y;

  marquee->inResize = true;

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
      (char *) bdjoptGetStr (OPT_MP_MQ_ACCENT_COL));
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

  marquee->danceLab = gtk_label_new (_("Not Playing"));
  gtk_widget_set_margin_top (marquee->danceLab, 2);
  gtk_widget_set_halign (marquee->danceLab, GTK_ALIGN_START);
  gtk_widget_set_hexpand (marquee->danceLab, TRUE);
  gtk_widget_set_can_focus (marquee->danceLab, FALSE);
  snprintf (tbuff, sizeof (tbuff),
      "label { color: %s; }",
      (char *) bdjoptGetStr (OPT_MP_MQ_ACCENT_COL));
  uiutilsSetCss (marquee->danceLab, tbuff);
  gtk_box_pack_start (GTK_BOX (hbox), marquee->danceLab,
      TRUE, TRUE, 0);

  marquee->countdownTimerLab = gtk_label_new ("0:00");
  gtk_widget_set_margin_top (marquee->countdownTimerLab, 2);
  gtk_label_set_max_width_chars (GTK_LABEL (marquee->countdownTimerLab), 6);
  gtk_widget_set_halign (marquee->countdownTimerLab, GTK_ALIGN_END);
  gtk_widget_set_can_focus (marquee->countdownTimerLab, FALSE);
  snprintf (tbuff, sizeof (tbuff),
      "label { color: %s; }",
      (char *) bdjoptGetStr (OPT_MP_MQ_ACCENT_COL));
  uiutilsSetCss (marquee->countdownTimerLab, tbuff);
  gtk_box_pack_end (GTK_BOX (hbox), marquee->countdownTimerLab,
      FALSE, FALSE, 0);

  if (marquee->mqShowInfo) {
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_halign (hbox, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand (hbox, TRUE);
    gtk_widget_set_margin_end (hbox, 0);
    gtk_widget_set_margin_start (hbox, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox,
        FALSE, FALSE, 0);

    marquee->infoArtistLab = gtk_label_new ("");
    gtk_widget_set_margin_top (marquee->infoArtistLab, 2);
    gtk_widget_set_halign (marquee->infoArtistLab, GTK_ALIGN_START);
    gtk_widget_set_hexpand (hbox, TRUE);
    gtk_widget_set_can_focus (marquee->infoArtistLab, FALSE);
    gtk_label_set_ellipsize (GTK_LABEL (marquee->infoArtistLab), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start (GTK_BOX (hbox), marquee->infoArtistLab,
        FALSE, FALSE, 0);

    marquee->infoSepLab = gtk_label_new ("");
    gtk_widget_set_margin_top (marquee->infoSepLab, 2);
    gtk_widget_set_halign (marquee->infoSepLab, GTK_ALIGN_START);
    gtk_widget_set_hexpand (hbox, FALSE);
    gtk_widget_set_can_focus (marquee->infoSepLab, FALSE);
    gtk_box_pack_start (GTK_BOX (hbox), marquee->infoSepLab,
        FALSE, FALSE, 0);

    marquee->infoTitleLab = gtk_label_new ("");
    gtk_widget_set_margin_top (marquee->infoTitleLab, 2);
    gtk_widget_set_halign (marquee->infoTitleLab, GTK_ALIGN_START);
    gtk_widget_set_hexpand (hbox, TRUE);
    gtk_widget_set_can_focus (marquee->infoTitleLab, FALSE);
    gtk_label_set_ellipsize (GTK_LABEL (marquee->infoArtistLab), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start (GTK_BOX (hbox), marquee->infoTitleLab,
        FALSE, FALSE, 0);
  }

  marquee->sep = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_margin_top (marquee->sep, 2);
  snprintf (tbuff, sizeof (tbuff),
      "separator { min-height: 4px; background-color: %s; }",
      (char *) bdjoptGetStr (OPT_MP_MQ_ACCENT_COL));
  uiutilsSetCss (marquee->sep, tbuff);
  gtk_box_pack_end (GTK_BOX (vbox), marquee->sep,
      TRUE, TRUE, 0);

  marquee->marqueeLabs = malloc (sizeof (GtkWidget *) * marquee->mqLen);

  for (int i = 0; i < marquee->mqLen; ++i) {
    marquee->marqueeLabs [i] = gtk_label_new ("");
    gtk_widget_set_halign (marquee->marqueeLabs [i], GTK_ALIGN_START);
    gtk_widget_set_hexpand (marquee->marqueeLabs [i], TRUE);
    gtk_widget_set_margin_top (marquee->marqueeLabs [i], 2);
    gtk_widget_set_margin_start (marquee->marqueeLabs [i], 10);
    gtk_widget_set_margin_end (marquee->marqueeLabs [i], 10);
    gtk_widget_set_can_focus (marquee->marqueeLabs [i], FALSE);
    gtk_box_pack_start (GTK_BOX (marquee->vbox),
        marquee->marqueeLabs [i], FALSE, FALSE, 0);
  }

  gtk_widget_show_all (window);

  x = nlistGetNum (marquee->options, MQ_POSITION_X);
  y = nlistGetNum (marquee->options, MQ_POSITION_Y);
  if (x != -1 && y != -1) {
    gtk_window_move (GTK_WINDOW (window), x, y);
  }

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
        case MSG_MARQUEE_SHOW: {
          if (marquee->isIconified) {
            gtk_widget_show (marquee->window);
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
    gint        x, y;

    gtk_window_get_position (GTK_WINDOW (window), &x, &y);
    nlistSetNum (marquee->options, MQ_POSITION_X, x);
    nlistSetNum (marquee->options, MQ_POSITION_Y, y);

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
    marqueeSetNotMaximized (marquee);
  } else {
    marquee->inMax = true;
    marquee->isMaximized = true;
    gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
    marquee->inMax = false;
    gtk_window_maximize (GTK_WINDOW (window));
  }

  return FALSE;
}

static void
marqueeSetNotMaximized (marquee_t *marquee)
{
  if (marquee->isMaximized) {
    marquee->inMax = true;
    marquee->isMaximized = false;
    marqueeAdjustFontSizes (marquee, marquee->priorSize);
    marquee->setPrior = true;
    gtk_window_unmaximize (GTK_WINDOW (marquee->window));
    marquee->unMaximize = true;
    marquee->inMax = false;
    gtk_window_set_decorated (GTK_WINDOW (marquee->window), TRUE);
  }
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
  gtk_widget_unset_state_flags (window, GTK_STATE_FLAG_BACKDROP);
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

  gtk_widget_get_allocation (marquee->pbar, &allocSize);
  pbarHeight = allocSize.height;
  gtk_widget_get_allocation (marquee->sep, &allocSize);
  sepHeight = allocSize.height;
  if (marquee->mqShowInfo) {
    gtk_widget_get_allocation (marquee->infoArtistLab, &allocSize);
    infoHeight = allocSize.height;
  }
  margin = gtk_widget_get_margin_top (marquee->danceLab);
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
  g_signal_handler_disconnect (w, marquee->sizeSignal);
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
  g_signal_handler_disconnect (w, marquee->unmaxSignal);
  marquee->unmaxSignal = 0;
  gtk_window_unmaximize (GTK_WINDOW (marquee->window));
}

