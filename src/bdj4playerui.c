
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
#include "conn.h"
#include "lock.h"
#include "log.h"
#include "pathbld.h"
#include "process.h"
#include "progstart.h"
#include "sock.h"
#include "sockh.h"
#include "sysvars.h"
#include "tmutil.h"
#include "uiutils.h"

typedef struct {
  GtkApplication  *app;
  progstart_t     *progstart;
  conn_t          *conn;
  int             haveData;
  sockserver_t    *sockserver;
  mstime_t        tm;
  GtkBuilder      *gtkplayerui;
  GtkWidget       *window;
  /* song display */
  GtkWidget       *statusImg;
  GtkWidget       *repeatImg;
  GtkWidget       *danceLab;
  GtkWidget       *artistLab;
  GtkWidget       *titleLab;
  /* main controls */
  GtkWidget       *fadeButton;
  GtkWidget       *ppButton;
  GtkWidget       *repeatButton;
  GtkWidget       *beginButton;
  GtkWidget       *nextsongButton;
  GtkWidget       *pauseatendButton;
  GdkPixbuf       *playImg;
  GdkPixbuf       *stopImg;
  GdkPixbuf       *pauseImg;
  GtkWidget       *ledoffImg;
  GtkWidget       *ledonImg;
  /* position controls / display */
  GtkWidget       *speedScale;
  GtkWidget       *speedDisplayLab;
  GtkWidget       *seekScale;
  double          lastdur;
  GtkWidget       *countdownTimerLab;
  GtkWidget       *durationLab;
  /* volume controls / display */
  GtkWidget       *volumeScale;
  GtkWidget       *volumeDisplayLab;
  /* notebook */
  GtkWidget       *notebook;
  /* music queue tab */
  GtkWidget       *moveupButton;
  GtkWidget       *movedownButton;
  GtkWidget       *togglepauseButton;
  GtkWidget       *removeButton;
  GtkWidget       *reqexternalButton;
  /* song selection tab */
  GtkWidget       *queueButton;
  GtkWidget       *danceSelectLab;
  GtkWidget       *danceSelect;
  GtkWidget       *playlistSelectLab;
  GtkWidget       *playlistSelect;
  /* tree views */
  GtkWidget       *musicqTree;
  GtkWidget       *songselTree;
} playerui_t;

#define PLUI_EXIT_WAIT_COUNT   20

static bool     pluiConnectingCallback (void *udata, programstate_t programState);
static bool     pluiHandshakeCallback (void *udata, programstate_t programState);
static bool     pluiInitDataCallback (void *udata, programstate_t programState);
static bool     pluiStoppingCallback (void *udata, programstate_t programState);
static bool     pluiClosingCallback (void *udata, programstate_t programState);
static int      pluiCreateGui (playerui_t *plui, int argc, char *argv []);
static void     pluiActivate (GApplication *app, gpointer userdata);
gboolean        pluiMainLoop  (void *tplui);
static int      pluiProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static gboolean pluiCloseWin (GtkWidget *window, GdkEvent *event, gpointer userdata);
static void     pluiProcessPauseatend (playerui_t *plui, int on);
static void     pluiProcessStatusData (playerui_t *plui, char *args);
static void     pluiProcessDanceList (playerui_t *pluiData, char *danceList);
static void     pluiProcessPlaylistList (playerui_t *pluiData, char *playlistList);
static void     pluiSigHandler (int sig);

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

  static struct option bdj_options [] = {
    { "debug",      required_argument,  NULL,   'd' },
    { "profile",    required_argument,  NULL,   'p' },
    { "playerui",   no_argument,        NULL,   0 },
    { NULL,         0,                  NULL,   0 }
  };

  mstimestart (&plui.tm);
  plui.progstart = progstartInit ();
  progstartSetCallback (plui.progstart, STATE_CONNECTING, pluiConnectingCallback);
  progstartSetCallback (plui.progstart, STATE_WAIT_HANDSHAKE, pluiHandshakeCallback);
  progstartSetCallback (plui.progstart, STATE_INITIALIZE_DATA, pluiInitDataCallback);
  progstartSetCallback (plui.progstart, STATE_STOPPING, pluiStoppingCallback);
  progstartSetCallback (plui.progstart, STATE_CLOSING, pluiClosingCallback);
  plui.sockserver = NULL;
  plui.window = NULL;
  plui.haveData = 0;
  plui.statusImg = NULL;
  plui.danceLab = NULL;
  plui.artistLab = NULL;
  plui.titleLab = NULL;
  plui.fadeButton = NULL;
  plui.ppButton = NULL;
  plui.repeatButton = NULL;
  plui.beginButton = NULL;
  plui.nextsongButton = NULL;
  plui.pauseatendButton = NULL;
  plui.stopImg = NULL;
  plui.playImg = NULL;
  plui.pauseImg = NULL;
  plui.ledoffImg = NULL;
  plui.ledonImg = NULL;
  plui.seekScale = NULL;
  plui.lastdur = 180000.0;
  plui.speedScale = NULL;
  plui.speedDisplayLab = NULL;
  plui.countdownTimerLab = NULL;
  plui.durationLab = NULL;
  plui.volumeScale = NULL;
  plui.volumeDisplayLab = NULL;
  plui.notebook = NULL;
  plui.moveupButton = NULL;
  plui.movedownButton = NULL;
  plui.togglepauseButton = NULL;
  plui.removeButton = NULL;
  plui.reqexternalButton = NULL;
  plui.queueButton = NULL;
  plui.danceSelectLab = NULL;
  plui.danceSelect = NULL;
  plui.playlistSelectLab = NULL;
  plui.playlistSelect = NULL;
  plui.musicqTree = NULL;
  plui.songselTree = NULL;

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

  listenPort = bdjvarsl [BDJVL_PLAYERUI_PORT];
  plui.conn = connInit (ROUTE_PLAYERUI);

  plui.sockserver = sockhStartServer (listenPort);
  /* using a timeout of 5 seems to interfere with gtk */
  g_timeout_add (10, pluiMainLoop, &plui);

  status = pluiCreateGui (&plui, 0, NULL);

  while (progstartShutdownProcess (plui.progstart, &plui) != STATE_CLOSED) {
    ;
  }
  progstartFree (plui.progstart);

  return status;
}

/* internal routines */

static bool
pluiStoppingCallback (void *udata, programstate_t programState)
{
  playerui_t   *plui = udata;

  connDisconnectAll (plui->conn);
  return true;
}

static bool
pluiClosingCallback (void *udata, programstate_t programState)
{
  playerui_t   *plui = udata;

  connFree (plui->conn);

  sockhCloseServer (plui->sockserver);
  bdjoptFree ();
  bdjvarsCleanup ();

  lockRelease (PLAYERUI_LOCK_FN, PATHBLD_MP_USEIDX);
  logMsg (LOG_SESS, LOG_IMPORTANT, "time-to-end: %ld ms", mstimeend (&plui->tm));
  logEnd ();

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
  g_object_unref (plui->stopImg);
  g_object_unref (plui->playImg);
  g_object_unref (plui->pauseImg);
  g_object_unref (plui->ledonImg);
  g_object_unref (plui->ledoffImg);
  g_object_unref (plui->app);
  return status;
}

static void
pluiActivate (GApplication *app, gpointer userdata)
{
  playerui_t          *plui = userdata;
  char                tbuff [MAXPATHLEN];
  GError              *gerr;
  GtkWidget           *image;
  GtkAdjustment       *adjustment;


  pathbldMakePath (tbuff, sizeof (tbuff), "", "bdj4playerui", ".glade",
      PATHBLD_MP_RESOURCEDIR);
  plui->gtkplayerui = gtk_builder_new_from_file (tbuff);

  plui->window = GTK_WIDGET (gtk_builder_get_object (plui->gtkplayerui, "mainwindow"));
  assert (plui->window != NULL);
  gtk_window_set_application (GTK_WINDOW (plui->window), plui->app);
  gtk_window_set_default_icon_from_file ("img/bdj4_icon.svg", &gerr);
  g_signal_connect (plui->window, "delete-event", G_CALLBACK (pluiCloseWin), plui);
  // ### FIX: get title from profile
  gtk_window_set_title (GTK_WINDOW (plui->window), _("BDJ4"));

  /* song display */
  plui->statusImg = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "playbackstatusimg"));
  assert (plui->statusImg != NULL);
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_stop", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  plui->stopImg = gtk_image_get_pixbuf (GTK_IMAGE (image));
  g_object_ref (plui->stopImg);
  if (plui->statusImg != NULL) {
    gtk_image_set_from_pixbuf (GTK_IMAGE (plui->statusImg), plui->stopImg);
  }

  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_play", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  plui->playImg = gtk_image_get_pixbuf (GTK_IMAGE (image));
  g_object_ref (plui->playImg);

  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_pause", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  plui->pauseImg = gtk_image_get_pixbuf (GTK_IMAGE (image));
  g_object_ref (plui->pauseImg);


  plui->repeatImg = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "repeatstatusimg"));
  assert (plui->repeatImg != NULL);
  gtk_image_clear (GTK_IMAGE (plui->repeatImg));

  plui->danceLab = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "dancelabel"));
  assert (plui->danceLab != NULL);
  plui->artistLab = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "artistlabel"));
  assert (plui->artistLab != NULL);
  plui->titleLab = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "titlelabel"));
  assert (plui->titleLab != NULL);
  /* main controls */
  plui->fadeButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "fadebutton"));
  assert (plui->fadeButton != NULL);

  plui->ppButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "playpausebutton"));
  assert (plui->ppButton != NULL);
  gtk_button_set_label (GTK_BUTTON (plui->ppButton), "");
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_playpause", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  if (plui->ppButton != NULL) {
    gtk_button_set_image (GTK_BUTTON (plui->ppButton), image);
  }

  plui->repeatButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "repeatbutton"));
  assert (plui->repeatButton != NULL);
  gtk_button_set_label (GTK_BUTTON (plui->repeatButton), "");
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_repeat", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  if (plui->repeatButton != NULL) {
    gtk_button_set_image (GTK_BUTTON (plui->repeatButton), image);
  }

  plui->beginButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "beginbutton"));
  assert (plui->beginButton != NULL);
  gtk_button_set_label (GTK_BUTTON (plui->beginButton), "");
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_begin", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  if (plui->beginButton != NULL) {
    gtk_button_set_image (GTK_BUTTON (plui->beginButton), image);
  }

  plui->nextsongButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "nextsongbutton"));
  assert (plui->nextsongButton != NULL);
  gtk_button_set_label (GTK_BUTTON (plui->nextsongButton), "");
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_nextsong", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  if (plui->nextsongButton != NULL) {
    gtk_button_set_image (GTK_BUTTON (plui->nextsongButton), image);
  }

  plui->pauseatendButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "pauseatend"));
  assert (plui->pauseatendButton != NULL);
  pathbldMakePath (tbuff, sizeof (tbuff), "", "led_off", ".svg",
      PATHBLD_MP_IMGDIR);
  plui->ledoffImg = gtk_image_new_from_file (tbuff);
  g_object_ref (plui->ledoffImg);
  pathbldMakePath (tbuff, sizeof (tbuff), "", "led_on", ".svg",
      PATHBLD_MP_IMGDIR);
  plui->ledonImg = gtk_image_new_from_file (tbuff);
  g_object_ref (plui->ledonImg);
  if (plui->pauseatendButton != NULL) {
    gtk_button_set_image (GTK_BUTTON (plui->pauseatendButton), plui->ledoffImg);
  }

  /* position controls / display */
  plui->speedScale = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "speedscale"));
  assert (plui->speedScale != NULL);
  adjustment = gtk_adjustment_new (0.0, 70.0, 130.0, 0.1, 1.0, 0.0);
  gtk_range_set_adjustment (GTK_RANGE (plui->speedScale), adjustment);
  gtk_range_set_value (GTK_RANGE (plui->speedScale), 100.0);
  uiutilsSetCss (GTK_WIDGET (plui->speedScale),
      "scale, trough { min-height: 5px; }");
  plui->speedDisplayLab = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "speeddisplaylabel"));
  assert (plui->speedDisplayLab != NULL);

  plui->seekScale = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "seekscale"));
  assert (plui->seekScale != NULL);
  adjustment = gtk_adjustment_new (0.0, 0.0, 180000.0, 100.0, 1000.0, 0.0);
  gtk_range_set_adjustment (GTK_RANGE (plui->seekScale), adjustment);
  gtk_range_set_value (GTK_RANGE (plui->seekScale), 0.0);
  uiutilsSetCss (GTK_WIDGET (plui->seekScale),
      "scale, trough { min-height: 5px; }");
  plui->countdownTimerLab = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "countdowntimer"));
  assert (plui->countdownTimerLab != NULL);
  plui->durationLab = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "duration"));
  assert (plui->durationLab != NULL);

  /* volume controls / display */
  plui->volumeScale = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "volumescale"));
  assert (plui->volumeScale != NULL);
  adjustment = gtk_adjustment_new (0.0, 0.0, 100.0, 0.1, 1.0, 0.0);
  gtk_range_set_adjustment (GTK_RANGE (plui->volumeScale), adjustment);
  gtk_range_set_value (GTK_RANGE (plui->volumeScale), 0.0);
  uiutilsSetCss (GTK_WIDGET (plui->volumeScale),
      "scale, trough { min-height: 5px; }");
  plui->volumeDisplayLab = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "volumedisplaylabel"));
  assert (plui->volumeDisplayLab != NULL);

  /* notebook */
  plui->notebook = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "notebook"));
  assert (plui->notebook != NULL);

  /* music queue tab */
  plui->moveupButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "moveupbutton"));
  assert (plui->moveupButton != NULL);
  gtk_button_set_label (GTK_BUTTON (plui->moveupButton), "");
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_up", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  if (plui->moveupButton != NULL) {
    gtk_button_set_image (GTK_BUTTON (plui->moveupButton), image);
  }

  plui->movedownButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "movedownbutton"));
  assert (plui->movedownButton != NULL);
  gtk_button_set_label (GTK_BUTTON (plui->movedownButton), "");
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_down", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  if (plui->movedownButton != NULL) {
    gtk_button_set_image (GTK_BUTTON (plui->movedownButton), image);
  }

  plui->togglepauseButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "togglepausebutton"));
  assert (plui->togglepauseButton != NULL);
  gtk_button_set_label (GTK_BUTTON (plui->togglepauseButton), "");
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_pause", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  if (plui->togglepauseButton != NULL) {
    gtk_button_set_image (GTK_BUTTON (plui->togglepauseButton), image);
  }

  plui->removeButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "removebutton"));
  assert (plui->removeButton != NULL);
  gtk_button_set_label (GTK_BUTTON (plui->removeButton), "");
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_audioremove", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  if (plui->removeButton != NULL) {
    gtk_button_set_image (GTK_BUTTON (plui->removeButton), image);
  }

  plui->reqexternalButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "requestexternalbutton"));
  assert (plui->reqexternalButton != NULL);

  /* song selection tab */
  plui->queueButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "queuebutton"));
  assert (plui->queueButton != NULL);
  plui->danceSelect = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "danceselect"));
  assert (plui->danceSelect != NULL);
  plui->playlistSelect = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "playlistselect"));
  assert (plui->playlistSelect != NULL);

  /* tree views */
  plui->musicqTree = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "musicqueue"));
  assert (plui->musicqTree != NULL);
  plui->songselTree = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "songselection"));
  assert (plui->songselTree != NULL);

  gtk_window_set_default_size (GTK_WINDOW (plui->window), 1000, 600);

  logMsg (LOG_SESS, LOG_IMPORTANT, "running: time-to-init-gui: %ld ms", mstimeend (&plui->tm));
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
    progstartProcess (plui->progstart, plui);
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      cont = FALSE;
    }
    return cont;
  }

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    cont = FALSE;
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
  bool        rc = false;


  if (connHaveHandshake (plui->conn, ROUTE_MAIN) &&
      connHaveHandshake (plui->conn, ROUTE_PLAYER)) {
    connSendMessage (plui->conn, ROUTE_MAIN,
        MSG_GET_DANCE_LIST, NULL);
    connSendMessage (plui->conn, ROUTE_MAIN,
        MSG_GET_PLAYLIST_LIST, NULL);
    rc = true;
  }

  return rc;
}

static bool
pluiInitDataCallback (void *udata, programstate_t programState)
{
  playerui_t   *plui = udata;
  bool          rc = false;


  if (plui->haveData == 2) {
    gtk_widget_show_all (GTK_WIDGET (plui->window));
    logMsg (LOG_SESS, LOG_IMPORTANT, "running: time-to-start: %ld ms", mstimeend (&plui->tm));
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
          mstimestart (&plui->tm);
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          logMsg (LOG_DBG, LOG_MSGS, "got: req-exit");
          progstartShutdownProcess (plui->progstart, plui);
          logProcEnd (LOG_PROC, "pluiProcessMsg", "req-exit");
          return 1;
        }
        case MSG_DANCE_LIST_DATA: {
          ++plui->haveData;
          pluiProcessDanceList (plui, args);
          break;
        }
        case MSG_PLAYLIST_LIST_DATA: {
          ++plui->haveData;
          pluiProcessPlaylistList (plui, args);
          break;
        }
        case MSG_PLAY_PAUSEATEND_STATE: {
          pluiProcessPauseatend (plui, atol (args));
          break;
        }
        case MSG_STATUS_DATA: {
          pluiProcessStatusData (plui, args);
          break;
        }
        case MSG_MARQUEE_DATA: {
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
    mstimestart (&plui->tm);
    gdone = 1;
    progstartShutdownProcess (plui->progstart, plui);
    logMsg (LOG_DBG, LOG_MSGS, "got: close win request");
    return TRUE;
  }

  return FALSE;
}

static void
pluiProcessPauseatend (playerui_t *plui, int on)
{
  if (plui->pauseatendButton == NULL) {
    return;
  }

  if (on) {
    gtk_button_set_image (GTK_BUTTON (plui->pauseatendButton), plui->ledonImg);
  } else {
    gtk_button_set_image (GTK_BUTTON (plui->pauseatendButton), plui->ledoffImg);
  }
}

static void
pluiProcessPlayerState (playerui_t *plui, int playerState)
{
  if (plui->statusImg == NULL) {
    return;
  }

  switch (playerState) {
    case PL_STATE_STOPPED: {
      gtk_image_clear (GTK_IMAGE (plui->statusImg));
      gtk_image_set_from_pixbuf (GTK_IMAGE (plui->statusImg), plui->stopImg);
      break;
    }
    case PL_STATE_PLAYING: {
      gtk_image_clear (GTK_IMAGE (plui->statusImg));
      gtk_image_set_from_pixbuf (GTK_IMAGE (plui->statusImg), plui->playImg);
      break;
    }
    case PL_STATE_PAUSED: {
      gtk_image_clear (GTK_IMAGE (plui->statusImg));
      gtk_image_set_from_pixbuf (GTK_IMAGE (plui->statusImg), plui->pauseImg);
      break;
    }
    default: {
      gtk_image_clear (GTK_IMAGE (plui->statusImg));
      gtk_image_set_from_pixbuf (GTK_IMAGE (plui->statusImg), plui->stopImg);
      break;
    }
  }
}

static void
pluiProcessStatusData (playerui_t *plui, char *args)
{
  char          *p;
  char          *tokstr;
  int           ps;
  char          tbuff [100];
  double        dval;
  double        ddur;
  GtkAdjustment *adjustment;

  /* player state */
  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  if (strcmp (p, "play") == 0) { ps = PL_STATE_PLAYING; }
  if (strcmp (p, "stop") == 0) { ps = PL_STATE_STOPPED; }
  if (strcmp (p, "pause") == 0) { ps = PL_STATE_PAUSED; }
  pluiProcessPlayerState (plui, ps);

  /* repeat */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (plui->repeatImg != NULL) {
    if (atol (p)) {
      pathbldMakePath (tbuff, sizeof (tbuff), "", "button_repeat", ".svg",
          PATHBLD_MP_IMGDIR);
      gtk_image_clear (GTK_IMAGE (plui->repeatImg));
      gtk_image_set_from_file (GTK_IMAGE (plui->repeatImg), tbuff);
    } else {
      gtk_image_clear (GTK_IMAGE (plui->repeatImg));
    }
  }

  /* pauseatend */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  pluiProcessPauseatend (plui, atol (p));

  /* vol */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff), "%3s", p);
  gtk_label_set_label (GTK_LABEL (plui->volumeDisplayLab), p);
  dval = atof (p);
  gtk_range_set_value (GTK_RANGE (plui->volumeScale), dval);

  /* speed */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff), "%3s", p);
  gtk_label_set_label (GTK_LABEL (plui->speedDisplayLab), p);
  dval = atof (p);
  gtk_range_set_value (GTK_RANGE (plui->speedScale), dval);

  /* playedtime */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  tmutilToMSD (atol (p), tbuff, sizeof (tbuff));
  gtk_label_set_label (GTK_LABEL (plui->countdownTimerLab), tbuff);
  dval = atof (p);

  /* duration */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  ddur = atof (p);
  if (ddur > 0.0 && ddur != plui->lastdur) {
    tmutilToMS (atol (p), tbuff, sizeof (tbuff));
    gtk_label_set_label (GTK_LABEL (plui->durationLab), tbuff);
    adjustment = gtk_adjustment_new (0.0, 0.0, ddur, 100.0, 1000.0, 0.0);
    gtk_range_set_adjustment (GTK_RANGE (plui->seekScale), adjustment);
    plui->lastdur = ddur;
  }
  if (ddur == 0.0) {
    gtk_range_set_value (GTK_RANGE (plui->seekScale), 0.0);
  } else {
    gtk_range_set_value (GTK_RANGE (plui->seekScale), dval);
  }

  /* dance */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (plui->danceLab != NULL) {
    gtk_label_set_label (GTK_LABEL (plui->danceLab), p);
  }

  /* artist */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (plui->artistLab != NULL) {
    gtk_label_set_label (GTK_LABEL (plui->artistLab), p);
  }

  /* title */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (plui->titleLab != NULL) {
    gtk_label_set_label (GTK_LABEL (plui->titleLab), p);
  }
}

static void
pluiProcessDanceList (playerui_t *plui, char *danceList)
{
  char              *didx = NULL;
  char              *dstr = NULL;
  char              *tokstr = NULL;
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  char              tbuff [200];

  store = gtk_list_store_new (2, G_TYPE_ULONG, G_TYPE_STRING);

  didx = strtok_r (danceList, MSG_ARGS_RS_STR, &tokstr);
  dstr = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  while (didx != NULL) {
    gtk_list_store_append (store, &iter);
    snprintf (tbuff, sizeof (tbuff), "%s    ", dstr);
    gtk_list_store_set (store, &iter, 0, didx, 1, tbuff, -1);
    didx = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    dstr = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  }

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
    renderer, "text", 1, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
//  gtk_tree_view_column_set_expand (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (plui->danceSelect), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (plui->danceSelect),
      GTK_TREE_MODEL (store));
  g_object_unref (store);
}


static void
pluiProcessPlaylistList (playerui_t *plui, char *playlistList)
{
  char              *fnm;
  char              *plnm;
  char              *tokstr;
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  char              tbuff [200];

  store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

  fnm = strtok_r (playlistList, MSG_ARGS_RS_STR, &tokstr);
  plnm = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  while (fnm != NULL) {
    gtk_list_store_append (store, &iter);
    snprintf (tbuff, sizeof (tbuff), "%s    ", plnm);
    gtk_list_store_set (store, &iter, 0, fnm, 1, tbuff, -1);
    fnm = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    plnm = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  }

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
    renderer, "text", 1, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
//  gtk_tree_view_column_set_expand (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (plui->playlistSelect), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (plui->playlistSelect),
      GTK_TREE_MODEL (store));
  g_object_unref (store);
}

static void
pluiSigHandler (int sig)
{
  gKillReceived = 1;
}

