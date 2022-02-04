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
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "bdjvarsdfload.h"
#include "conn.h"
#include "lock.h"
#include "log.h"
#include "musicdb.h"
#include "musicq.h"
#include "pathbld.h"
#include "process.h"
#include "progstart.h"
#include "slist.h"
#include "sock.h"
#include "sockh.h"
#include "sysvars.h"
#include "tmutil.h"
#include "uimusicq.h"
#include "uiplayer.h"
#include "uisongsel.h"
#include "uiutils.h"

typedef struct {
  progstart_t     *progstart;
  conn_t          *conn;
  sockserver_t    *sockserver;
  musicqidx_t     musicqPlayIdx;
  musicqidx_t     musicqManageIdx;
  /* gtk stuff */
  GtkApplication  *app;
  GtkWidget       *window;
  GtkWidget       *vbox;
  GtkWidget       *notebook;
  GtkWidget       *musicqImage [MUSICQ_MAX];
  GtkWidget       *setPlaybackButton;
  GdkPixbuf       *ledoffImg;
  GdkPixbuf       *ledonImg;
  /* ui major elements */
  uiplayer_t      *uiplayer;
  uimusicq_t      *uimusicq;
  uisongselect_t  *uisongselect;
} playerui_t;

#define PLUI_EXIT_WAIT_COUNT      20

static bool     pluiConnectingCallback (void *udata, programstate_t programState);
static bool     pluiHandshakeCallback (void *udata, programstate_t programState);
static bool     pluiStoppingCallback (void *udata, programstate_t programState);
static bool     pluiClosingCallback (void *udata, programstate_t programState);
static int      pluiCreateGui (playerui_t *plui, int argc, char *argv []);
static void     pluiActivate (GApplication *app, gpointer userdata);
gboolean        pluiMainLoop  (void *tplui);
static int      pluiProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static gboolean pluiCloseWin (GtkWidget *window, GdkEvent *event, gpointer userdata);
static void     pluiSigHandler (int sig);
static void     pluiSwitchPage (GtkNotebook *nb, GtkWidget *page, guint pagenum, gpointer udata);
static void     pluiSetPlaybackQueue (GtkButton *b, gpointer udata);


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
  playerui_t      plui;
  char            tbuff [MAXPATHLEN];


  static struct option bdj_options [] = {
    { "debug",      required_argument,  NULL,   'd' },
    { "profile",    required_argument,  NULL,   'p' },
    { "playerui",   no_argument,        NULL,   0 },
    { NULL,         0,                  NULL,   0 }
  };

  plui.notebook = NULL;
  plui.progstart = progstartInit ("playerui");
  progstartSetCallback (plui.progstart, STATE_CONNECTING,
      pluiConnectingCallback, &plui);
  progstartSetCallback (plui.progstart, STATE_WAIT_HANDSHAKE,
      pluiHandshakeCallback, &plui);
  progstartSetCallback (plui.progstart, STATE_STOPPING,
      pluiStoppingCallback, &plui);
  progstartSetCallback (plui.progstart, STATE_CLOSING,
      pluiClosingCallback, &plui);
  plui.sockserver = NULL;
  plui.window = NULL;
  plui.uiplayer = NULL;
  plui.uimusicq = NULL;
  plui.uisongselect = NULL;
  plui.musicqPlayIdx = MUSICQ_A;
  plui.musicqManageIdx = MUSICQ_A;

#if _define_SIGHUP
  processCatchSignal (pluiSigHandler, SIGHUP);
#endif
  processCatchSignal (pluiSigHandler, SIGINT);
  processDefaultSignal (SIGTERM);
#if _define_SIGCHLD
  processDefaultSignal (SIGCHLD);
#endif

  sysvarsInit (argv[0]);

  while ((c = getopt_long (argc, argv, "", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'd': {
        if (optarg) {
          loglevel = (loglevel_t) atoi (optarg);
        }
        break;
      }
      case 'p': {
        if (optarg) {
          sysvarSetNum (SVL_BDJIDX, atol (optarg));
        }
        break;
      }
      default: {
        break;
      }
    }
  }

  logStartAppend ("bdj4plui", "pu", loglevel);
  logMsg (LOG_SESS, LOG_IMPORTANT, "Using profile %ld", lsysvars [SVL_BDJIDX]);

  rc = lockAcquire (PLAYERUI_LOCK_FN, PATHBLD_MP_USEIDX);
  if (rc < 0) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: plui: unable to acquire lock: profile: %zd", lsysvars [SVL_BDJIDX]);
    logMsg (LOG_SESS, LOG_IMPORTANT, "ERR: plui: unable to acquire lock: profile: %zd", lsysvars [SVL_BDJIDX]);
    logEnd ();
    exit (0);
  }

  bdjvarsInit ();
  bdjoptInit ();
  bdjvarsdfloadInit ();

  pathbldMakePath (tbuff, MAXPATHLEN, "",
      MUSICDB_FNAME, MUSICDB_EXT, PATHBLD_MP_NONE);
  dbOpen (tbuff);

  listenPort = bdjvarsl [BDJVL_PLAYERUI_PORT];
  plui.conn = connInit (ROUTE_PLAYERUI);

  plui.uiplayer = uiplayerInit (plui.progstart, plui.conn);
  plui.uimusicq = uimusicqInit (plui.progstart, plui.conn);
  plui.uisongselect = uisongselInit (plui.progstart, plui.conn);

  plui.sockserver = sockhStartServer (listenPort);
  /* using a timeout of 5 seems to interfere with gtk */
  g_timeout_add (10, pluiMainLoop, &plui);

  status = pluiCreateGui (&plui, 0, NULL);

  while (progstartShutdownProcess (plui.progstart) != STATE_CLOSED) {
    ;
  }
  progstartFree (plui.progstart);
  logEnd ();
  return status;
}

/* internal routines */

static bool
pluiStoppingCallback (void *udata, programstate_t programState)
{
  playerui_t   *plui = udata;

  gdone = 1;
  connDisconnectAll (plui->conn);
  return true;
}

static bool
pluiClosingCallback (void *udata, programstate_t programState)
{
  playerui_t   *plui = udata;

  g_object_unref (plui->ledonImg);
  g_object_unref (plui->ledoffImg);

  connFree (plui->conn);

  sockhCloseServer (plui->sockserver);
  dbClose ();
  bdjoptFree ();
  bdjvarsCleanup ();
  bdjvarsdfloadCleanup ();

  uiplayerFree (plui->uiplayer);
  uimusicqFree (plui->uimusicq);
  uisongselFree (plui->uisongselect);

  lockRelease (PLAYERUI_LOCK_FN, PATHBLD_MP_USEIDX);
  return true;
}

static int
pluiCreateGui (playerui_t *plui, int argc, char *argv [])
{
  int             status;

  plui->app = gtk_application_new (
      "org.ballroomdj.BallroomDJ.playerui",
      G_APPLICATION_FLAGS_NONE
  );

  g_signal_connect (plui->app, "activate", G_CALLBACK (pluiActivate), plui);

  status = g_application_run (G_APPLICATION (plui->app), argc, argv);
  gtk_widget_destroy (plui->window);
  g_object_unref (plui->app);
  return status;
}

static void
pluiActivate (GApplication *app, gpointer userdata)
{
  playerui_t          *plui = userdata;
  GError              *gerr = NULL;
  GtkWidget           *tabLabel;
  GtkWidget           *widget;
  GtkWidget           *image;
  GtkWidget           *hbox;
  char                *str;
  char                tbuff [MAXPATHLEN];


  pathbldMakePath (tbuff, sizeof (tbuff), "", "led_off", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  plui->ledoffImg = gtk_image_get_pixbuf (GTK_IMAGE (image));
  g_object_ref (G_OBJECT (plui->ledoffImg));
  pathbldMakePath (tbuff, sizeof (tbuff), "", "led_on", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  plui->ledonImg = gtk_image_get_pixbuf (GTK_IMAGE (image));
  g_object_ref (G_OBJECT (plui->ledonImg));

  plui->window = gtk_application_window_new (GTK_APPLICATION (app));
  assert (plui->window != NULL);
  gtk_window_set_application (GTK_WINDOW (plui->window), plui->app);
  gtk_window_set_default_icon_from_file ("img/bdj4_icon.svg", &gerr);
  g_signal_connect (plui->window, "delete-event", G_CALLBACK (pluiCloseWin), plui);
  gtk_window_set_title (GTK_WINDOW (plui->window), bdjoptGetData (OPT_P_PROFILENAME));

  plui->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (plui->window), plui->vbox);

  /* player */
  widget = uiplayerActivate (plui->uiplayer);
  gtk_widget_set_hexpand (GTK_WIDGET (widget), TRUE);
  gtk_box_pack_start (GTK_BOX (plui->vbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);

  /* there doesn't seem to be any other good method to identify which */
  /* notebook page is which.  The code is dependent on musicq_a being */
  /* in tab 0, */
  plui->notebook = gtk_notebook_new ();
  assert (plui->notebook != NULL);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (plui->notebook), TRUE);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (plui->notebook), TRUE);
  gtk_widget_set_margin_top (GTK_WIDGET (plui->notebook), 4);
  gtk_widget_set_hexpand (GTK_WIDGET (plui->notebook), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (plui->notebook), FALSE);
  gtk_box_pack_start (GTK_BOX (plui->vbox), plui->notebook,
      TRUE, TRUE, 0);
  g_signal_connect (plui->notebook, "switch-page", G_CALLBACK (pluiSwitchPage), plui);

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_button_set_label (GTK_BUTTON (widget), "Set Queue for Playback");
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  gtk_notebook_set_action_widget (GTK_NOTEBOOK (plui->notebook), widget, GTK_PACK_END);
  gtk_widget_show_all (GTK_WIDGET (widget));
  g_signal_connect (widget, "clicked", G_CALLBACK (pluiSetPlaybackQueue), plui);
  plui->setPlaybackButton = widget;

  /* music queue tab */
  widget = uimusicqActivate (plui->uimusicq, plui->window, 0);
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  str = bdjoptGetData (OPT_P_QUEUE_NAME_A);
  tabLabel = gtk_label_new (str);
  gtk_box_pack_start (GTK_BOX (hbox), tabLabel, FALSE, FALSE, 1);
  plui->musicqImage [MUSICQ_A] = gtk_image_new ();
  gtk_image_set_from_pixbuf (GTK_IMAGE (plui->musicqImage [MUSICQ_A]), plui->ledonImg);
  gtk_widget_set_margin_start (GTK_WIDGET (plui->musicqImage [MUSICQ_A]), 2);
  gtk_box_pack_start (GTK_BOX (hbox), plui->musicqImage [MUSICQ_A], FALSE, FALSE, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (plui->notebook), widget, hbox);
  gtk_widget_show_all (GTK_WIDGET (hbox));

  /* queue B tab */
  widget = uimusicqActivate (plui->uimusicq, plui->window, 1);
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  str = bdjoptGetData (OPT_P_QUEUE_NAME_B);
  tabLabel = gtk_label_new (str);
  gtk_box_pack_start (GTK_BOX (hbox), tabLabel, FALSE, FALSE, 1);
  plui->musicqImage [MUSICQ_B] = gtk_image_new ();
  gtk_image_set_from_pixbuf (GTK_IMAGE (plui->musicqImage [MUSICQ_B]), plui->ledoffImg);
  gtk_widget_set_margin_start (GTK_WIDGET (plui->musicqImage [MUSICQ_B]), 2);
  gtk_box_pack_start (GTK_BOX (hbox), plui->musicqImage [MUSICQ_B], FALSE, FALSE, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (plui->notebook), widget, hbox);
  gtk_widget_show_all (GTK_WIDGET (hbox));

  /* song selection tab */
  widget = uisongselActivate (plui->uisongselect);
  tabLabel = gtk_label_new ("Song Selection");
  gtk_notebook_append_page (GTK_NOTEBOOK (plui->notebook), widget, tabLabel);

  gtk_window_set_default_size (GTK_WINDOW (plui->window), 1000, 600);
}

gboolean
pluiMainLoop (void *tplui)
{
  playerui_t   *plui = tplui;
  int         tdone = 0;
  gboolean    cont = TRUE;

  tdone = sockhProcessMain (plui->sockserver, pluiProcessMsg, plui);
  if (tdone || gdone) {
    ++gdone;
  }
  if (gdone > PLUI_EXIT_WAIT_COUNT) {
    g_application_quit (G_APPLICATION (plui->app));
    cont = FALSE;
  }

  if (! progstartIsRunning (plui->progstart)) {
    progstartProcess (plui->progstart);
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      progstartShutdownProcess (plui->progstart);
    }
    return cont;
  }

  uiplayerMainLoop (plui->uiplayer);

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstartShutdownProcess (plui->progstart);
  }
  return cont;
}

static bool
pluiConnectingCallback (void *udata, programstate_t programState)
{
  playerui_t   *plui = udata;
  bool        rc = false;


  if (! connIsConnected (plui->conn, ROUTE_MAIN)) {
    connConnect (plui->conn, ROUTE_MAIN);
  }
  if (! connIsConnected (plui->conn, ROUTE_PLAYER)) {
    connConnect (plui->conn, ROUTE_PLAYER);
  }

  if (connIsConnected (plui->conn, ROUTE_MAIN) &&
      connIsConnected (plui->conn, ROUTE_PLAYER)) {
    rc = true;
  }

  return rc;
}

static bool
pluiHandshakeCallback (void *udata, programstate_t programState)
{
  playerui_t   *plui = udata;
  bool          rc = false;


  if (connHaveHandshake (plui->conn, ROUTE_MAIN) &&
      connHaveHandshake (plui->conn, ROUTE_PLAYER)) {
    gtk_widget_show_all (GTK_WIDGET (plui->window));
    progstartLogTime (plui->progstart, "time-to-start-gui");
    rc = true;
  }

  return rc;
}

static int
pluiProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  playerui_t       *plui = udata;

  logProcBegin (LOG_PROC, "pluiProcessMsg");

  logMsg (LOG_DBG, LOG_MSGS, "got: from %d route: %d msg:%d args:%s",
      routefrom, route, msg, args);

  uiplayerProcessMsg (routefrom, route, msg, args, plui->uiplayer);
  uimusicqProcessMsg (routefrom, route, msg, args, plui->uimusicq);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_PLAYERUI: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (plui->conn, routefrom);
          connConnectResponse (plui->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          logMsg (LOG_DBG, LOG_MSGS, "got: req-exit");
          progstartShutdownProcess (plui->progstart);
          logProcEnd (LOG_PROC, "pluiProcessMsg", "req-exit");
          return 1;
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

  logProcEnd (LOG_PROC, "pluiProcessMsg", "");

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  return gKillReceived;
}


static gboolean
pluiCloseWin (GtkWidget *window, GdkEvent *event, gpointer userdata)
{
  playerui_t   *plui = userdata;

  if (! gdone) {
    progstartShutdownProcess (plui->progstart);
    logMsg (LOG_DBG, LOG_MSGS, "got: close win request");
    return TRUE;
  }

  return FALSE;
}

static void
pluiSigHandler (int sig)
{
  gKillReceived = 1;
}

static void
pluiSwitchPage (GtkNotebook *nb, GtkWidget *page, guint pagenum, gpointer udata)
{
  playerui_t  *plui = udata;

  /* note that the design requires that the music queues be the first */
  /* tabs in the notebook */
  if (pagenum < MUSICQ_MAX) {
    plui->musicqManageIdx = pagenum;
    uimusicqSetManageIdx (plui->uimusicq, pagenum);
    gtk_widget_show_all (GTK_WIDGET (plui->setPlaybackButton));
  } else {
    gtk_widget_hide (GTK_WIDGET (plui->setPlaybackButton));
  }
  return;
}

static void
pluiSetPlaybackQueue (GtkButton *b, gpointer udata)
{
  playerui_t      *plui = udata;
  char            tbuff [40];

  plui->musicqPlayIdx = plui->musicqManageIdx;
  if (plui->musicqPlayIdx == 0) {
    gtk_image_set_from_pixbuf (GTK_IMAGE (plui->musicqImage [MUSICQ_A]), plui->ledonImg);
    gtk_image_set_from_pixbuf (GTK_IMAGE (plui->musicqImage [MUSICQ_B]), plui->ledoffImg);
  } else {
    gtk_image_set_from_pixbuf (GTK_IMAGE (plui->musicqImage [MUSICQ_A]), plui->ledoffImg);
    gtk_image_set_from_pixbuf (GTK_IMAGE (plui->musicqImage [MUSICQ_B]), plui->ledonImg);
  }
  snprintf (tbuff, sizeof (tbuff), "%d", plui->musicqPlayIdx);
  connSendMessage (plui->conn, ROUTE_MAIN, MSG_MUSICQ_SET_PLAYBACK, tbuff);
}
