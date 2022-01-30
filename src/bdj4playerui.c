
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

enum {
  DANCE_COL_IDX,
  DANCE_COL_NAME,
};

enum {
  PLAYLIST_COL_FNAME,
  PLAYLIST_COL_NAME,
};

typedef struct {
  GtkApplication  *app;
  progstart_t     *progstart;
  playerstate_t   playerState;
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
  bool            repeatLock;
  GtkWidget       *songBeginButton;
  GtkWidget       *nextsongButton;
  GtkWidget       *pauseatendButton;
  bool            pauseatendLock;
  GdkPixbuf       *playImg;
  GdkPixbuf       *stopImg;
  GdkPixbuf       *pauseImg;
  GtkWidget       *ledoffImg;
  GtkWidget       *ledonImg;
  /* position controls / display */
  GtkWidget       *speedScale;
  GtkWidget       *speedDisplayLab;
  bool            speedLock;
  mstime_t        speedLockTimeout;
  mstime_t        speedLockSend;
  GtkWidget       *seekScale;
  double          lastdur;
  GtkWidget       *countdownTimerLab;
  GtkWidget       *durationLab;
  /* volume controls / display */
  GtkWidget       *volumeScale;
  bool            volumeLock;
  mstime_t        volumeLockTimeout;
  mstime_t        volumeLockSend;
  GtkWidget       *volumeDisplayLab;
  /* notebook */
  GtkWidget       *notebook;
  /* music queue tab */
  GtkWidget       *moveupButton;
  GtkWidget       *movedownButton;
  GtkWidget       *togglepauseButton;
  GtkWidget       *removeButton;
  GtkWidget       *clearQueueButton;
  GtkWidget       *danceSelectLab;
  GtkWidget       *danceSelect;
  GtkWidget       *queueDanceButton;
  GtkWidget       *playlistSelectLab;
  GtkWidget       *playlistSelect;
  GtkWidget       *queuePlaylistButton;
  GtkWidget       *reqexternalButton;
  /* song selection tab */
  GtkWidget       *queueButton;
  /* tree views */
  GtkWidget       *musicqTree;
  GtkWidget       *songselTree;
} playerui_t;

#define PLUI_EXIT_WAIT_COUNT      20

/* there are all sorts of latency issues making the sliders work nicely */
/* it will take at least 100ms and at most 200ms for the message to get */
/* back.  Then there are the latency issues on this end. */
#define PLUI_LOCK_TIME_WAIT   300
#define PLUI_LOCK_TIME_SEND   30

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
static void     pluiProcessPlayerState (playerui_t *plui, int playerState);
static void     pluiProcessStatusData (playerui_t *plui, char *args);
static void     pluiProcessDanceList (playerui_t *pluiData, char *danceList);
static void     pluiProcessPlaylistList (playerui_t *pluiData, char *playlistList);
static void     pluiFadeProcess (GtkButton *b, gpointer udata);
static void     pluiPlayPauseProcess (GtkButton *b, gpointer udata);
static void     pluiRepeatProcess (GtkButton *b, gpointer udata);
static void     pluiSongBeginProcess (GtkButton *b, gpointer udata);
static void     pluiNextSongProcess (GtkButton *b, gpointer udata);
static void     pluiPauseatendProcess (GtkButton *b, gpointer udata);
static gboolean pluiSpeedProcess (GtkRange *range, GtkScrollType *scroll, gdouble value, gpointer udata);
static gboolean pluiVolumeProcess (GtkRange *range, GtkScrollType *scroll, gdouble value, gpointer udata);
static void     pluiClearQueueProcess (GtkButton *b, gpointer udata);
static void     pluiQueueDanceProcess (GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *column, gpointer udata);
static void     pluiQueuePlaylistProcess (GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *column, gpointer udata);
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
  plui.repeatLock = false;
  plui.songBeginButton = NULL;
  plui.nextsongButton = NULL;
  plui.pauseatendButton = NULL;
  plui.pauseatendLock = false;
  plui.stopImg = NULL;
  plui.playImg = NULL;
  plui.pauseImg = NULL;
  plui.ledoffImg = NULL;
  plui.ledonImg = NULL;
  plui.seekScale = NULL;
  plui.lastdur = 180000.0;
  plui.speedScale = NULL;
  plui.speedLock = false;
  mstimeset (&plui.speedLockTimeout, 3600000);
  mstimeset (&plui.speedLockSend, 3600000);
  plui.speedDisplayLab = NULL;
  plui.countdownTimerLab = NULL;
  plui.durationLab = NULL;
  plui.volumeScale = NULL;
  plui.volumeLock = false;
  mstimeset (&plui.volumeLockTimeout, 3600000);
  mstimeset (&plui.volumeLockSend, 3600000);
  plui.volumeDisplayLab = NULL;
  plui.notebook = NULL;
  plui.moveupButton = NULL;
  plui.movedownButton = NULL;
  plui.togglepauseButton = NULL;
  plui.removeButton = NULL;
  plui.clearQueueButton = NULL;
  plui.reqexternalButton = NULL;
  plui.queueButton = NULL;
  plui.danceSelectLab = NULL;
  plui.danceSelect = NULL;
  plui.queueDanceButton = NULL;
  plui.playlistSelectLab = NULL;
  plui.playlistSelect = NULL;
  plui.queuePlaylistButton = NULL;
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

  mstimestart (&plui->tm);
  gdone = 1;
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
  gtk_window_set_title (GTK_WINDOW (plui->window), bdjoptGetData (OPT_P_PROFILENAME));

  /* song display */
  plui->statusImg = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "playbackstatusimg"));
  assert (plui->statusImg != NULL);
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_stop", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  plui->stopImg = gtk_image_get_pixbuf (GTK_IMAGE (image));
  g_object_ref (plui->stopImg);
  gtk_image_set_from_pixbuf (GTK_IMAGE (plui->statusImg), plui->stopImg);

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
  g_signal_connect (plui->fadeButton, "clicked", G_CALLBACK (pluiFadeProcess), plui);

  plui->ppButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "playpausebutton"));
  assert (plui->ppButton != NULL);
  gtk_button_set_label (GTK_BUTTON (plui->ppButton), "");
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_playpause", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (plui->ppButton), image);
  g_signal_connect (plui->ppButton, "clicked", G_CALLBACK (pluiPlayPauseProcess), plui);

  plui->repeatButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "repeatbutton"));
  assert (plui->repeatButton != NULL);
  gtk_button_set_label (GTK_BUTTON (plui->repeatButton), "");
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_repeat", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (plui->repeatButton), image);
  g_signal_connect (plui->repeatButton, "toggled", G_CALLBACK (pluiRepeatProcess), plui);

  plui->songBeginButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "beginbutton"));
  assert (plui->songBeginButton != NULL);
  gtk_button_set_label (GTK_BUTTON (plui->songBeginButton), "");
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_begin", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (plui->songBeginButton), image);
  g_signal_connect (plui->songBeginButton, "clicked", G_CALLBACK (pluiSongBeginProcess), plui);

  plui->nextsongButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "nextsongbutton"));
  assert (plui->nextsongButton != NULL);
  gtk_button_set_label (GTK_BUTTON (plui->nextsongButton), "");
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_nextsong", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (plui->nextsongButton), image);
  g_signal_connect (plui->nextsongButton, "clicked", G_CALLBACK (pluiNextSongProcess), plui);

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
  gtk_button_set_image (GTK_BUTTON (plui->pauseatendButton), plui->ledoffImg);
  g_signal_connect (plui->pauseatendButton, "toggled", G_CALLBACK (pluiPauseatendProcess), plui);

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
  g_signal_connect (plui->speedScale, "change-value", G_CALLBACK (pluiSpeedProcess), plui);

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
  g_signal_connect (plui->volumeScale, "change-value", G_CALLBACK (pluiVolumeProcess), plui);

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
  gtk_button_set_image (GTK_BUTTON (plui->moveupButton), image);

  plui->movedownButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "movedownbutton"));
  assert (plui->movedownButton != NULL);
  gtk_button_set_label (GTK_BUTTON (plui->movedownButton), "");
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_down", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (plui->movedownButton), image);

  plui->togglepauseButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "togglepausebutton"));
  assert (plui->togglepauseButton != NULL);
  gtk_button_set_label (GTK_BUTTON (plui->togglepauseButton), "");
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_pause", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (plui->togglepauseButton), image);

  plui->removeButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "removebutton"));
  assert (plui->removeButton != NULL);
  gtk_button_set_label (GTK_BUTTON (plui->removeButton), "");
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_audioremove", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (plui->removeButton), image);

  plui->clearQueueButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "clearqueuebutton"));
  assert (plui->clearQueueButton != NULL);
  g_signal_connect (plui->clearQueueButton, "clicked", G_CALLBACK (pluiClearQueueProcess), plui);

  plui->reqexternalButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "requestexternalbutton"));
  assert (plui->reqexternalButton != NULL);

  plui->danceSelect = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "danceselect"));
  assert (plui->danceSelect != NULL);
  plui->queueDanceButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "queuedancebutton"));
  assert (plui->queueDanceButton != NULL);
  g_signal_connect (plui->danceSelect, "row-activated", G_CALLBACK (pluiQueueDanceProcess), plui);

  plui->playlistSelect = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "playlistselect"));
  assert (plui->playlistSelect != NULL);
  plui->queuePlaylistButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "queueplaylistbutton"));
  assert (plui->queuePlaylistButton != NULL);
  g_signal_connect (plui->playlistSelect, "row-activated", G_CALLBACK (pluiQueuePlaylistProcess), plui);

  /* song selection tab */
  plui->queueButton = GTK_WIDGET (gtk_builder_get_object (
      plui->gtkplayerui, "queuebutton"));
  assert (plui->queueButton != NULL);

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
      progstartShutdownProcess (plui->progstart, plui);
    }
    return cont;
  }

  if (mstimeCheck (&plui->volumeLockTimeout)) {
    mstimeset (&plui->volumeLockTimeout, 3600000);
    plui->volumeLock = false;
  }

  if (mstimeCheck (&plui->volumeLockSend)) {
    double        value;
    char          tbuff [40];

    value = gtk_range_get_value (GTK_RANGE (plui->volumeScale));
    snprintf (tbuff, sizeof (tbuff), "%.0f", value);
    connSendMessage (plui->conn, ROUTE_PLAYER, MSG_PLAYER_VOLUME, tbuff);
    if (plui->volumeLock) {
      mstimeset (&plui->volumeLockSend, PLUI_LOCK_TIME_SEND);
    } else {
      mstimeset (&plui->volumeLockSend, 3600000);
    }
  }

  if (mstimeCheck (&plui->speedLockTimeout)) {
    mstimeset (&plui->speedLockTimeout, 3600000);
    plui->speedLock = false;
  }

  if (mstimeCheck (&plui->speedLockSend)) {
    double        value;
    char          tbuff [40];

    value = gtk_range_get_value (GTK_RANGE (plui->speedScale));
    snprintf (tbuff, sizeof (tbuff), "%.0f", value);
    connSendMessage (plui->conn, ROUTE_PLAYER, MSG_PLAY_SPEED, tbuff);
    if (plui->speedLock) {
      mstimeset (&plui->speedLockSend, PLUI_LOCK_TIME_SEND);
    } else {
      mstimeset (&plui->speedLockSend, 3600000);
    }
  }

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstartShutdownProcess (plui->progstart, plui);
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
        case MSG_PLAYER_STATE: {
          pluiProcessPlayerState (plui, atol (args));
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
  plui->pauseatendLock = true;

  if (on) {
    gtk_button_set_image (GTK_BUTTON (plui->pauseatendButton), plui->ledonImg);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plui->pauseatendButton), TRUE);
  } else {
    gtk_button_set_image (GTK_BUTTON (plui->pauseatendButton), plui->ledoffImg);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plui->pauseatendButton), FALSE);
  }
  plui->pauseatendLock = false;
}

static void
pluiProcessPlayerState (playerui_t *plui, int playerState)
{
  if (plui->statusImg == NULL) {
    return;
  }

  plui->playerState = playerState;

  if (playerState == PL_STATE_IN_FADEOUT) {
    gtk_widget_set_sensitive (GTK_WIDGET (plui->volumeScale), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (plui->seekScale), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (plui->speedScale), FALSE);
  } else {
    gtk_widget_set_sensitive (GTK_WIDGET (plui->volumeScale), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (plui->seekScale), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (plui->speedScale), TRUE);
  }

  switch (playerState) {
    case PL_STATE_UNKNOWN:
    case PL_STATE_STOPPED: {
      gtk_image_clear (GTK_IMAGE (plui->statusImg));
      gtk_image_set_from_pixbuf (GTK_IMAGE (plui->statusImg), plui->stopImg);
      break;
    }
    case PL_STATE_LOADING:
    case PL_STATE_IN_FADEOUT:
    case PL_STATE_IN_GAP:
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
  char          tbuff [100];
  double        dval;
  double        ddur;
  GtkAdjustment *adjustment;

  /* player state */
  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  /* this is handled by the MSG_PLAYER_STATE message */

  /* repeat */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (plui->repeatImg != NULL) {
    plui->repeatLock = true;
    if (atol (p)) {
      pathbldMakePath (tbuff, sizeof (tbuff), "", "button_repeat", ".svg",
          PATHBLD_MP_IMGDIR);
      gtk_image_clear (GTK_IMAGE (plui->repeatImg));
      gtk_image_set_from_file (GTK_IMAGE (plui->repeatImg), tbuff);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plui->repeatButton), TRUE);
    } else {
      gtk_image_clear (GTK_IMAGE (plui->repeatImg));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plui->repeatButton), FALSE);
    }
    plui->repeatLock = false;
  }

  /* pauseatend */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  pluiProcessPauseatend (plui, atol (p));

  /* vol */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (! plui->volumeLock) {
    snprintf (tbuff, sizeof (tbuff), "%3s", p);
    gtk_label_set_label (GTK_LABEL (plui->volumeDisplayLab), p);
    dval = atof (p);
    gtk_range_set_value (GTK_RANGE (plui->volumeScale), dval);
  }

  /* speed */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (! plui->speedLock) {
    snprintf (tbuff, sizeof (tbuff), "%3s", p);
    gtk_label_set_label (GTK_LABEL (plui->speedDisplayLab), p);
    dval = atof (p);
    gtk_range_set_value (GTK_RANGE (plui->speedScale), dval);
  }

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
    gtk_list_store_set (store, &iter,
        DANCE_COL_IDX, atol (didx),
        DANCE_COL_NAME, tbuff, -1);
    didx = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    dstr = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  }

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
    renderer, "text", DANCE_COL_NAME, NULL);
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
    gtk_list_store_set (store, &iter,
        PLAYLIST_COL_FNAME, fnm,
        PLAYLIST_COL_NAME, tbuff, -1);
    fnm = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    plnm = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  }

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
    renderer, "text", PLAYLIST_COL_NAME, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
//  gtk_tree_view_column_set_expand (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (plui->playlistSelect), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (plui->playlistSelect),
      GTK_TREE_MODEL (store));
  g_object_unref (store);
}

static void
pluiFadeProcess (GtkButton *b, gpointer udata)
{
  playerui_t      *plui = udata;

  connSendMessage (plui->conn, ROUTE_PLAYER, MSG_PLAY_FADE, NULL);
}

static void
pluiPlayPauseProcess (GtkButton *b, gpointer udata)
{
  playerui_t      *plui = udata;

  connSendMessage (plui->conn, ROUTE_MAIN, MSG_PLAY_PLAYPAUSE, NULL);
}

static void
pluiRepeatProcess (GtkButton *b, gpointer udata)
{
  playerui_t      *plui = udata;

  if (plui->repeatLock) {
    return;
  }

  connSendMessage (plui->conn, ROUTE_PLAYER, MSG_PLAY_REPEAT, NULL);
}

static void
pluiSongBeginProcess (GtkButton *b, gpointer udata)
{
  playerui_t      *plui = udata;

  connSendMessage (plui->conn, ROUTE_PLAYER, MSG_PLAY_SONG_BEGIN, NULL);
}

static void
pluiNextSongProcess (GtkButton *b, gpointer udata)
{
  playerui_t      *plui = udata;

  connSendMessage (plui->conn, ROUTE_PLAYER, MSG_PLAY_NEXTSONG, NULL);
}

static void
pluiPauseatendProcess (GtkButton *b, gpointer udata)
{
  playerui_t      *plui = udata;

  if (plui->pauseatendLock) {
    return;
  }
  connSendMessage (plui->conn, ROUTE_PLAYER, MSG_PLAY_PAUSEATEND, NULL);
}

static gboolean
pluiSpeedProcess (GtkRange *range, GtkScrollType *scroll, gdouble value, gpointer udata)
{
  playerui_t    *plui = udata;
  char          tbuff [40];

  if (! plui->speedLock) {
    mstimeset (&plui->speedLockSend, PLUI_LOCK_TIME_SEND);
  }
  plui->speedLock = true;
  mstimeset (&plui->speedLockTimeout, PLUI_LOCK_TIME_WAIT);
  snprintf (tbuff, sizeof (tbuff), "%3.0f", value);
  gtk_label_set_label (GTK_LABEL (plui->speedDisplayLab), tbuff);
  return FALSE;
}

static gboolean
pluiVolumeProcess (GtkRange *range, GtkScrollType *scroll, gdouble value, gpointer udata)
{
  playerui_t    *plui = udata;
  char          tbuff [40];

  if (! plui->volumeLock) {
    mstimeset (&plui->volumeLockSend, PLUI_LOCK_TIME_SEND);
  }
  plui->volumeLock = true;
  mstimeset (&plui->volumeLockTimeout, PLUI_LOCK_TIME_WAIT);
  snprintf (tbuff, sizeof (tbuff), "%3.0f", value);
  gtk_label_set_label (GTK_LABEL (plui->volumeDisplayLab), tbuff);
  return FALSE;
}

static void
pluiClearQueueProcess (GtkButton *b, gpointer udata)
{
  playerui_t      *plui = udata;

  connSendMessage (plui->conn, ROUTE_MAIN, MSG_QUEUE_CLEAR, NULL);
}

static void
pluiQueueDanceProcess (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  playerui_t    *plui = udata;
  GtkTreeIter   iter;
  GtkTreeModel  *model = gtk_tree_view_get_model (GTK_TREE_VIEW (plui->danceSelect));
  unsigned long idx;
  char          tbuff [20];

  if (gtk_tree_model_get_iter (model, &iter, path)) {
    gtk_tree_model_get (model, &iter, DANCE_COL_IDX, &idx, -1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plui->queueDanceButton), FALSE);
    snprintf (tbuff, sizeof (tbuff), "%lu", idx);
    connSendMessage (plui->conn, ROUTE_MAIN, MSG_QUEUE_DANCE, tbuff);
  }
}

static void
pluiQueuePlaylistProcess (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  playerui_t    *plui = udata;
  GtkTreeIter   iter;
  GtkTreeModel  *model = gtk_tree_view_get_model (GTK_TREE_VIEW (plui->playlistSelect));

  if (gtk_tree_model_get_iter (model, &iter, path)) {
    char    *plfname;

    gtk_tree_model_get (model, &iter, PLAYLIST_COL_FNAME, &plfname, -1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plui->queuePlaylistButton), FALSE);
    connSendMessage (plui->conn, ROUTE_MAIN, MSG_QUEUE_PLAYLIST, plfname);
    free (plfname);
  }
}


static void
pluiSigHandler (int sig)
{
  gKillReceived = 1;
}

