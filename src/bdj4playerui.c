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
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "bdjvarsdfload.h"
#include "conn.h"
#include "localeutil.h"
#include "lock.h"
#include "log.h"
#include "musicq.h"
#include "pathbld.h"
#include "procutil.h"
#include "progstate.h"
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
  progstate_t     *progstate;
  char            *locknm;
  procutil_t      *processes [ROUTE_MAX];
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
  uisongsel_t     *uisongsel;
  /* options */
  bool            playWhenQueued : 1;
  bool            showExtraQueues : 1;
  bool            switchQueueWhenEmpty : 1;
} playerui_t;

#define PLUI_EXIT_WAIT_COUNT      20

static bool     pluiListeningCallback (void *udata, programstate_t programState);
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
/* queue selection handlers */
static void     pluiSwitchPage (GtkNotebook *nb, GtkWidget *page, guint pagenum, gpointer udata);
static void     pluiSetSwitchPage (playerui_t *plui, int pagenum);
static void     pluiProcessSetPlaybackQueue (GtkButton *b, gpointer udata);
static void     pluiSetPlaybackQueue (playerui_t *plui, musicqidx_t newqueue);
/* option handlers */
static void     pluiTogglePlayWhenQueued (GtkWidget *mi, gpointer udata);
static void     pluiSetPlayWhenQueued (playerui_t *plui);
static void     pluiToggleExtraQueues (GtkWidget *mi, gpointer udata);
static void     pluiSetExtraQueues (playerui_t *plui);
static void     pluiToggleSwitchQueue (GtkWidget *mi, gpointer udata);
static void     pluiSetSwitchQueue (playerui_t *plui);


static int gKillReceived = 0;
static int gdone = 0;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  playerui_t      plui;
  char            *uifont;


  plui.notebook = NULL;
  plui.progstate = progstateInit ("playerui");
  progstateSetCallback (plui.progstate, STATE_LISTENING,
      pluiListeningCallback, &plui);
  progstateSetCallback (plui.progstate, STATE_CONNECTING,
      pluiConnectingCallback, &plui);
  progstateSetCallback (plui.progstate, STATE_WAIT_HANDSHAKE,
      pluiHandshakeCallback, &plui);
  progstateSetCallback (plui.progstate, STATE_STOPPING,
      pluiStoppingCallback, &plui);
  progstateSetCallback (plui.progstate, STATE_CLOSING,
      pluiClosingCallback, &plui);
  plui.sockserver = NULL;
  plui.window = NULL;
  plui.uiplayer = NULL;
  plui.uimusicq = NULL;
  plui.uisongsel = NULL;
  plui.musicqPlayIdx = MUSICQ_A;
  plui.musicqManageIdx = MUSICQ_A;
  plui.playWhenQueued = true;
  plui.showExtraQueues = false;
  plui.switchQueueWhenEmpty = false;

  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    plui.processes [i] = NULL;
  }

#if _define_SIGHUP
  procutilCatchSignal (pluiSigHandler, SIGHUP);
#endif
  procutilCatchSignal (pluiSigHandler, SIGINT);
  procutilDefaultSignal (SIGTERM);
#if _define_SIGCHLD
  procutilDefaultSignal (SIGCHLD);
#endif

  bdj4startup (argc, argv, "pu", ROUTE_PLAYERUI);
  localeInit ();
  logProcBegin (LOG_PROC, "playerui");

  listenPort = bdjvarsGetNum (BDJVL_PLAYERUI_PORT);
  plui.conn = connInit (ROUTE_PLAYERUI);

  plui.uiplayer = uiplayerInit (plui.progstate, plui.conn);
  plui.uimusicq = uimusicqInit (plui.progstate, plui.conn);
  plui.uisongsel = uisongselInit (plui.progstate, plui.conn);

  plui.sockserver = sockhStartServer (listenPort);

  uiutilsInitGtkLog ();
  gtk_init (&argc, NULL);
  uifont = bdjoptGetData (OPT_MP_UIFONT);
  uiutilsSetUIFont (uifont);

  /* read in the playerui options */
  /* setextraqueues is called in the gui activation */
  // ### TODO read in playerui options
  pluiSetPlayWhenQueued (&plui);
  pluiSetSwitchQueue (&plui);

  g_timeout_add (UI_MAIN_LOOP_TIMER, pluiMainLoop, &plui);

  status = pluiCreateGui (&plui, 0, NULL);

  while (progstateShutdownProcess (plui.progstate) != STATE_CLOSED) {
    ;
  }
  progstateFree (plui.progstate);
  logEnd ();
  return status;
}

/* internal routines */

static bool
pluiStoppingCallback (void *udata, programstate_t programState)
{
  playerui_t   *plui = udata;

  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    if (plui->processes [i] != NULL) {
      procutilStopProcess (plui->processes [i], plui->conn, i, false);
    }
  }

  gdone = 1;
  connDisconnectAll (plui->conn);
  return true;
}

static bool
pluiClosingCallback (void *udata, programstate_t programState)
{
  playerui_t   *plui = udata;

  // ### TODO save playerui options

  g_object_unref (plui->ledonImg);
  g_object_unref (plui->ledoffImg);

  connFree (plui->conn);
  sockhCloseServer (plui->sockserver);

  bdj4shutdown (ROUTE_PLAYERUI);

  uiplayerFree (plui->uiplayer);
  uimusicqFree (plui->uimusicq);
  uisongselFree (plui->uisongsel);

  /* give the other processes some time to shut down */
  mssleep (200);
  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    if (plui->processes [i] != NULL) {
      procutilStopProcess (plui->processes [i], plui->conn, i, true);
      procutilFree (plui->processes [i]);
    }
  }
  return true;
}

static int
pluiCreateGui (playerui_t *plui, int argc, char *argv [])
{
  int           status;

  plui->app = gtk_application_new (
      "org.bdj4.BDJ4.playerui",
      G_APPLICATION_NON_UNIQUE
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
  GtkWidget           *menubar;
  GtkWidget           *menu;
  GtkWidget           *menuitem;
  char                *str;
  char                imgbuff [MAXPATHLEN];
  char                tbuff [MAXPATHLEN];

  pathbldMakePath (imgbuff, sizeof (imgbuff), "",
      "bdj4_icon", ".svg", PATHBLD_MP_IMGDIR);
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

  plui->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  assert (plui->window != NULL);
  gtk_window_set_application (GTK_WINDOW (plui->window), GTK_APPLICATION (app));
  gtk_window_set_application (GTK_WINDOW (plui->window), plui->app);
  gtk_window_set_default_icon_from_file (imgbuff, &gerr);
  g_signal_connect (plui->window, "delete-event", G_CALLBACK (pluiCloseWin), plui);
  gtk_window_set_title (GTK_WINDOW (plui->window), bdjoptGetData (OPT_P_PROFILENAME));

  plui->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (plui->window), plui->vbox);
  gtk_widget_set_margin_top (GTK_WIDGET (plui->vbox), 4);
  gtk_widget_set_margin_bottom (GTK_WIDGET (plui->vbox), 4);
  gtk_widget_set_margin_start (GTK_WIDGET (plui->vbox), 4);
  gtk_widget_set_margin_end (GTK_WIDGET (plui->vbox), 4);

  /* menu */
  menubar = gtk_menu_bar_new ();
  gtk_widget_set_hexpand (GTK_WIDGET (menubar), TRUE);
  gtk_box_pack_start (GTK_BOX (plui->vbox), GTK_WIDGET (menubar),
      FALSE, FALSE, 0);

  menuitem = gtk_menu_item_new_with_label (_("Options"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), menuitem);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

  menuitem = gtk_check_menu_item_new_with_label (_("Play When Queued"));
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
      plui->playWhenQueued);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  g_signal_connect (menuitem, "toggled",
      G_CALLBACK (pluiTogglePlayWhenQueued), plui);

  menuitem = gtk_check_menu_item_new_with_label (_("Show Extra Queues"));
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
      plui->showExtraQueues);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  g_signal_connect (menuitem, "toggled",
      G_CALLBACK (pluiToggleExtraQueues), plui);

  menuitem = gtk_check_menu_item_new_with_label (_("Switch Queue When Empty"));
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
      plui->switchQueueWhenEmpty);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  g_signal_connect (menuitem, "toggled",
      G_CALLBACK (pluiToggleSwitchQueue), plui);

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
  g_signal_connect (widget, "clicked", G_CALLBACK (pluiProcessSetPlaybackQueue), plui);
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
  widget = uisongselActivate (plui->uisongsel, plui->window);
  tabLabel = gtk_label_new ("Song Selection");
  gtk_notebook_append_page (GTK_NOTEBOOK (plui->notebook), widget, tabLabel);

  gtk_window_set_default_size (GTK_WINDOW (plui->window), 1000, 600);

  gtk_widget_show_all (GTK_WIDGET (plui->window));
  pluiSetExtraQueues (plui);
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

  if (! progstateIsRunning (plui->progstate)) {
    progstateProcess (plui->progstate);
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      progstateShutdownProcess (plui->progstate);
    }
    return cont;
  }

  uiplayerMainLoop (plui->uiplayer);
  uimusicqMainLoop (plui->uimusicq);
  uisongselMainLoop (plui->uisongsel);

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (plui->progstate);
  }
  return cont;
}

static bool
pluiListeningCallback (void *udata, programstate_t programState)
{
  playerui_t   *plui = udata;

  plui->processes [ROUTE_MAIN] = procutilStartProcess (
      ROUTE_MAIN, "bdj4main");
  return true;
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
    progstateLogTime (plui->progstate, "time-to-start-gui");
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
          progstateShutdownProcess (plui->progstate);
          logProcEnd (LOG_PROC, "pluiProcessMsg", "req-exit");
          return 1;
        }
        case MSG_QUEUE_SWITCH: {
          pluiSetPlaybackQueue (plui, atoi (args));
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
    progstateShutdownProcess (plui->progstate);
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

  pluiSetSwitchPage (plui, pagenum);
}

static void
pluiSetSwitchPage (playerui_t *plui, int pagenum)
{
  /* note that the design requires that the music queues be the first */
  /* tabs in the notebook */
  if (pagenum < MUSICQ_MAX && plui->showExtraQueues) {
    plui->musicqManageIdx = pagenum;
    uimusicqSetManageIdx (plui->uimusicq, pagenum);
    gtk_widget_show (GTK_WIDGET (plui->setPlaybackButton));
  } else {
    gtk_widget_hide (GTK_WIDGET (plui->setPlaybackButton));
  }
  return;
}

static void
pluiProcessSetPlaybackQueue (GtkButton *b, gpointer udata)
{
  playerui_t      *plui = udata;

  pluiSetPlaybackQueue (plui, plui->musicqManageIdx);
}

static void
pluiSetPlaybackQueue (playerui_t  *plui, musicqidx_t newQueue)
{
  char            tbuff [40];

  if (plui->showExtraQueues) {
    plui->musicqPlayIdx = newQueue;
    if (plui->musicqPlayIdx == MUSICQ_A) {
      gtk_image_set_from_pixbuf (GTK_IMAGE (plui->musicqImage [MUSICQ_A]), plui->ledonImg);
      gtk_image_set_from_pixbuf (GTK_IMAGE (plui->musicqImage [MUSICQ_B]), plui->ledoffImg);
    } else {
      gtk_image_set_from_pixbuf (GTK_IMAGE (plui->musicqImage [MUSICQ_A]), plui->ledoffImg);
      gtk_image_set_from_pixbuf (GTK_IMAGE (plui->musicqImage [MUSICQ_B]), plui->ledonImg);
    }
  }
  /* if showextraqueues is off, reject any attempt to switch playback. */
  /* let main know what queue is being used */
  uimusicqSetPlayIdx (plui->uimusicq, plui->musicqPlayIdx);
  snprintf (tbuff, sizeof (tbuff), "%d", plui->musicqPlayIdx);
  connSendMessage (plui->conn, ROUTE_MAIN, MSG_MUSICQ_SET_PLAYBACK, tbuff);
}




static void
pluiTogglePlayWhenQueued (GtkWidget *mi, gpointer udata)
{
  playerui_t      *plui = udata;

  plui->playWhenQueued = ! plui->playWhenQueued;
  pluiSetPlayWhenQueued (plui);
}

static void
pluiSetPlayWhenQueued (playerui_t *plui)
{
  char  tbuff [40];

  snprintf (tbuff, sizeof (tbuff), "%d", plui->playWhenQueued);
  connSendMessage (plui->conn, ROUTE_MAIN, MSG_QUEUE_PLAY_ON_ADD, tbuff);
}


static void
pluiToggleExtraQueues (GtkWidget *mi, gpointer udata)
{
  playerui_t      *plui = udata;

  plui->showExtraQueues = ! plui->showExtraQueues;

  pluiSetExtraQueues (plui);
}

static void
pluiSetExtraQueues (playerui_t *plui)
{
  GtkWidget       *page;
  int             pagenum;

  if (plui->notebook == NULL ||
      plui->setPlaybackButton == NULL) {
    return;
  }

  page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (plui->notebook), MUSICQ_B);
  gtk_widget_set_visible (page, plui->showExtraQueues);
  if (plui->showExtraQueues) {
    pagenum = gtk_notebook_get_current_page (GTK_NOTEBOOK (plui->notebook));
    /* I wish there was a way to identify the notebook tab */
    if (pagenum == 0) {
      gtk_widget_show (plui->setPlaybackButton);
    }
  } else {
    gtk_widget_hide (plui->setPlaybackButton);
  }
}

static void
pluiToggleSwitchQueue (GtkWidget *mi, gpointer udata)
{
  playerui_t      *plui = udata;

  plui->switchQueueWhenEmpty = ! plui->switchQueueWhenEmpty;
  pluiSetSwitchQueue (plui);
}

static void
pluiSetSwitchQueue (playerui_t *plui)
{
  char  tbuff [40];

  snprintf (tbuff, sizeof (tbuff), "%d", plui->switchQueueWhenEmpty);
  connSendMessage (plui->conn, ROUTE_MAIN, MSG_QUEUE_SWITCH_EMPTY, tbuff);
}

