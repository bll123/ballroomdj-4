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
#include "bdj4playerui.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "bdjvarsdfload.h"
#include "conn.h"
#include "datafile.h"
#include "localeutil.h"
#include "lock.h"
#include "log.h"
#include "musicq.h"
#include "osuiutils.h"
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
  int             dbgflags;
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
  datafile_t      *optiondf;
  nlist_t         *options;
} playerui_t;

static datafilekey_t playeruidfkeys [] = {
  { "FILTER_POS_X",             SONGSEL_FILTER_POSITION_X,    VALUE_NUM, NULL, -1 },
  { "FILTER_POS_Y",             SONGSEL_FILTER_POSITION_Y,    VALUE_NUM, NULL, -1 },
  { "PLAY_WHEN_QUEUED",         PLUI_PLAY_WHEN_QUEUED,        VALUE_NUM, NULL, -1 },
  { "PLUI_POS_X",               PLUI_POSITION_X,              VALUE_NUM, NULL, -1 },
  { "PLUI_POS_Y",               PLUI_POSITION_Y,              VALUE_NUM, NULL, -1 },
  { "PLUI_SIZE_X",              PLUI_SIZE_X,                  VALUE_NUM, NULL, -1 },
  { "PLUI_SIZE_Y",              PLUI_SIZE_Y,                  VALUE_NUM, NULL, -1 },
  { "SHOW_EXTRA_QUEUES",        PLUI_SHOW_EXTRA_QUEUES,       VALUE_NUM, NULL, -1 },
  { "SORT_BY",                  SONGSEL_SORT_BY,              VALUE_STR, NULL, -1 },
  { "SWITCH_QUEUE_WHEN_EMPTY",  PLUI_SWITCH_QUEUE_WHEN_EMPTY, VALUE_NUM, NULL, -1 },
};
#define PLAYERUI_DFKEY_COUNT (sizeof (playeruidfkeys) / sizeof (datafilekey_t))

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
  char            tbuff [MAXPATHLEN];


  plui.notebook = NULL;
  plui.progstate = progstateInit ("playerui");
  progstateSetCallback (plui.progstate, STATE_LISTENING,
      pluiListeningCallback, &plui);
  progstateSetCallback (plui.progstate, STATE_CONNECTING,
      pluiConnectingCallback, &plui);
  progstateSetCallback (plui.progstate, STATE_WAIT_HANDSHAKE,
      pluiHandshakeCallback, &plui);
  plui.sockserver = NULL;
  plui.window = NULL;
  plui.uiplayer = NULL;
  plui.uimusicq = NULL;
  plui.uisongsel = NULL;
  plui.musicqPlayIdx = MUSICQ_A;
  plui.musicqManageIdx = MUSICQ_A;

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

  plui.dbgflags = bdj4startup (argc, argv, "pu", ROUTE_PLAYERUI, BDJ4_INIT_NONE);
  logProcBegin (LOG_PROC, "playerui");

  listenPort = bdjvarsGetNum (BDJVL_PLAYERUI_PORT);
  plui.conn = connInit (ROUTE_PLAYERUI);

  pathbldMakePath (tbuff, sizeof (tbuff), "",
      "playerui", ".txt", PATHBLD_MP_USEIDX);
  plui.optiondf = datafileAllocParse ("playerui-opt", DFTYPE_KEY_VAL, tbuff,
      playeruidfkeys, PLAYERUI_DFKEY_COUNT, DATAFILE_NO_LOOKUP);
  plui.options = datafileGetList (plui.optiondf);
  if (plui.options == NULL) {
    plui.options = nlistAlloc ("playerui-opt", LIST_ORDERED, free);

    nlistSetNum (plui.options, PLUI_PLAY_WHEN_QUEUED, true);
    nlistSetNum (plui.options, PLUI_SHOW_EXTRA_QUEUES, false);
    nlistSetNum (plui.options, PLUI_SWITCH_QUEUE_WHEN_EMPTY, false);
    nlistSetNum (plui.options, SONGSEL_FILTER_POSITION_X, -1);
    nlistSetNum (plui.options, SONGSEL_FILTER_POSITION_Y, -1);
    nlistSetNum (plui.options, PLUI_POSITION_X, -1);
    nlistSetNum (plui.options, PLUI_POSITION_Y, -1);
    nlistSetNum (plui.options, PLUI_SIZE_X, 1000);
    nlistSetNum (plui.options, PLUI_SIZE_Y, 600);
    nlistSetStr (plui.options, SONGSEL_SORT_BY, "TITLE");
  }

  plui.uiplayer = uiplayerInit (plui.progstate, plui.conn);
  plui.uimusicq = uimusicqInit (plui.progstate, plui.conn);
  plui.uisongsel = uisongselInit (plui.progstate, plui.conn, plui.options,
      SONG_FILTER_FOR_PLAYBACK);

  /* register these after calling the sub-window initialization */
  /* then these will be run last, after the other closing callbacks */
  progstateSetCallback (plui.progstate, STATE_STOPPING,
      pluiStoppingCallback, &plui);
  progstateSetCallback (plui.progstate, STATE_CLOSING,
      pluiClosingCallback, &plui);

  plui.sockserver = sockhStartServer (listenPort);

  uiutilsInitGtkLog ();
  gtk_init (&argc, NULL);
  uifont = bdjoptGetStr (OPT_MP_UIFONT);
  uiutilsSetUIFont (uifont);

  pluiSetPlayWhenQueued (&plui);
  pluiSetSwitchQueue (&plui);

  g_timeout_add (UI_MAIN_LOOP_TIMER, pluiMainLoop, &plui);

  status = pluiCreateGui (&plui, 0, NULL);

  while (progstateShutdownProcess (plui.progstate) != STATE_CLOSED) {
    ;
  }
  progstateFree (plui.progstate);

  logProcEnd (LOG_PROC, "playerui", "");
  logEnd ();
  return status;
}

/* internal routines */

static bool
pluiStoppingCallback (void *udata, programstate_t programState)
{
  playerui_t    * plui = udata;
  gint          x, y;

  logProcBegin (LOG_PROC, "pluiStoppingCallback");
  gtk_window_get_size (GTK_WINDOW (plui->window), &x, &y);
  nlistSetNum (plui->options, PLUI_SIZE_X, x);
  nlistSetNum (plui->options, PLUI_SIZE_Y, y);
  gtk_window_get_position (GTK_WINDOW (plui->window), &x, &y);
  nlistSetNum (plui->options, PLUI_POSITION_X, x);
  nlistSetNum (plui->options, PLUI_POSITION_Y, y);

  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    if (plui->processes [i] != NULL) {
      procutilStopProcess (plui->processes [i], plui->conn, i, false);
    }
  }

  gdone = 1;
  connDisconnectAll (plui->conn);
  logProcEnd (LOG_PROC, "pluiStoppingCallback", "");
  return true;
}

static bool
pluiClosingCallback (void *udata, programstate_t programState)
{
  playerui_t    *plui = udata;
  char          fn [MAXPATHLEN];

  logProcBegin (LOG_PROC, "pluiClosingCallback");
  pathbldMakePath (fn, sizeof (fn), "",
      "playerui", ".txt", PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("playerui", fn, playeruidfkeys, PLAYERUI_DFKEY_COUNT, plui->options);

  g_object_unref (plui->ledonImg);
  g_object_unref (plui->ledoffImg);

  sockhCloseServer (plui->sockserver);

  bdj4shutdown (ROUTE_PLAYERUI);

  if (plui->options != datafileGetList (plui->optiondf)) {
    nlistFree (plui->options);
  }
  datafileFree (plui->optiondf);

  /* give the other processes some time to shut down */
  mssleep (200);
  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    if (plui->processes [i] != NULL) {
      procutilStopProcess (plui->processes [i], plui->conn, i, true);
      procutilFree (plui->processes [i]);
    }
  }

  connFree (plui->conn);

  uiplayerFree (plui->uiplayer);
  uimusicqFree (plui->uimusicq);
  uisongselFree (plui->uisongsel);
  uiutilsCleanup ();

  logProcEnd (LOG_PROC, "pluiClosingCallback", "");
  return true;
}

static int
pluiCreateGui (playerui_t *plui, int argc, char *argv [])
{
  int           status;

  logProcBegin (LOG_PROC, "pluiCreateGui");

  plui->app = gtk_application_new (
      "org.bdj4.BDJ4.playerui",
      G_APPLICATION_NON_UNIQUE
  );

  g_signal_connect (plui->app, "activate", G_CALLBACK (pluiActivate), plui);

  /* gtk messes up the locale setting somehow; a re-bind is necessary */
  localeInit ();

  status = g_application_run (G_APPLICATION (plui->app), argc, argv);
  if (GTK_IS_WIDGET (plui->window)) {
    gtk_widget_destroy (plui->window);
  }
  g_object_unref (plui->app);

  logProcEnd (LOG_PROC, "pluiCreateGui", "");
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
  gint                x, y;

  logProcBegin (LOG_PROC, "pluiActivate");

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
  gtk_window_set_title (GTK_WINDOW (plui->window), bdjoptGetStr (OPT_P_PROFILENAME));

  plui->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (plui->window), plui->vbox);
  gtk_widget_set_margin_top (plui->vbox, 4);
  gtk_widget_set_margin_bottom (plui->vbox, 4);
  gtk_widget_set_margin_start (plui->vbox, 4);
  gtk_widget_set_margin_end (plui->vbox, 4);

  /* menu */
  menubar = gtk_menu_bar_new ();
  gtk_widget_set_hexpand (menubar, TRUE);
  gtk_box_pack_start (GTK_BOX (plui->vbox), menubar,
      FALSE, FALSE, 0);

  menuitem = gtk_menu_item_new_with_label (_("Options"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), menuitem);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

  menuitem = gtk_check_menu_item_new_with_label (_("Play When Queued"));
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
      nlistGetNum (plui->options, PLUI_PLAY_WHEN_QUEUED));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  g_signal_connect (menuitem, "toggled",
      G_CALLBACK (pluiTogglePlayWhenQueued), plui);

  menuitem = gtk_check_menu_item_new_with_label (_("Show Extra Queues"));
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
      nlistGetNum (plui->options, PLUI_SHOW_EXTRA_QUEUES));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  g_signal_connect (menuitem, "toggled",
      G_CALLBACK (pluiToggleExtraQueues), plui);

  menuitem = gtk_check_menu_item_new_with_label (_("Switch Queue When Empty"));
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
      nlistGetNum (plui->options, PLUI_SWITCH_QUEUE_WHEN_EMPTY));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  g_signal_connect (menuitem, "toggled",
      G_CALLBACK (pluiToggleSwitchQueue), plui);

  /* player */
  widget = uiplayerActivate (plui->uiplayer);
  gtk_widget_set_hexpand (widget, TRUE);
  gtk_box_pack_start (GTK_BOX (plui->vbox), widget,
      FALSE, FALSE, 0);

  /* there doesn't seem to be any other good method to identify which */
  /* notebook page is which.  The code is dependent on musicq_a being */
  /* in tab 0, */
  plui->notebook = gtk_notebook_new ();
  assert (plui->notebook != NULL);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (plui->notebook), TRUE);
  gtk_widget_set_margin_top (plui->notebook, 4);
  gtk_widget_set_hexpand (plui->notebook, TRUE);
  gtk_widget_set_vexpand (plui->notebook, FALSE);
  uiutilsSetCss (plui->notebook,
      "notebook tab:checked { background-color: #111111; }");
  gtk_box_pack_start (GTK_BOX (plui->vbox), plui->notebook,
      TRUE, TRUE, 0);

  g_signal_connect (plui->notebook, "switch-page", G_CALLBACK (pluiSwitchPage), plui);

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_button_set_label (GTK_BUTTON (widget), "Set Queue for Playback");
  gtk_widget_set_margin_start (widget, 2);
  gtk_notebook_set_action_widget (GTK_NOTEBOOK (plui->notebook), widget, GTK_PACK_END);
  gtk_widget_show_all (widget);
  g_signal_connect (widget, "clicked", G_CALLBACK (pluiProcessSetPlaybackQueue), plui);
  plui->setPlaybackButton = widget;

  /* music queue tab */
  widget = uimusicqActivate (plui->uimusicq, plui->window, 0);
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  str = bdjoptGetStr (OPT_P_QUEUE_NAME_A);
  tabLabel = gtk_label_new (str);
  gtk_box_pack_start (GTK_BOX (hbox), tabLabel, FALSE, FALSE, 1);
  plui->musicqImage [MUSICQ_A] = gtk_image_new ();
  gtk_image_set_from_pixbuf (GTK_IMAGE (plui->musicqImage [MUSICQ_A]), plui->ledonImg);
  gtk_widget_set_margin_start (plui->musicqImage [MUSICQ_A], 2);
  gtk_box_pack_start (GTK_BOX (hbox), plui->musicqImage [MUSICQ_A], FALSE, FALSE, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (plui->notebook), widget, hbox);
  gtk_widget_show_all (hbox);

  /* queue B tab */
  widget = uimusicqActivate (plui->uimusicq, plui->window, 1);
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  str = bdjoptGetStr (OPT_P_QUEUE_NAME_B);
  tabLabel = gtk_label_new (str);
  gtk_box_pack_start (GTK_BOX (hbox), tabLabel, FALSE, FALSE, 1);
  plui->musicqImage [MUSICQ_B] = gtk_image_new ();
  gtk_image_set_from_pixbuf (GTK_IMAGE (plui->musicqImage [MUSICQ_B]), plui->ledoffImg);
  gtk_widget_set_margin_start (plui->musicqImage [MUSICQ_B], 2);
  gtk_box_pack_start (GTK_BOX (hbox), plui->musicqImage [MUSICQ_B], FALSE, FALSE, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (plui->notebook), widget, hbox);
  gtk_widget_show_all (hbox);

  /* song selection tab */
  widget = uisongselActivate (plui->uisongsel, plui->window);
  tabLabel = gtk_label_new ("Song Selection");
  gtk_notebook_append_page (GTK_NOTEBOOK (plui->notebook), widget, tabLabel);

  x = nlistGetNum (plui->options, PLUI_SIZE_X);
  y = nlistGetNum (plui->options, PLUI_SIZE_Y);
  gtk_window_set_default_size (GTK_WINDOW (plui->window), x, y);

  gtk_widget_show_all (plui->window);

  x = nlistGetNum (plui->options, PLUI_POSITION_X);
  y = nlistGetNum (plui->options, PLUI_POSITION_Y);
  if (x != -1 && y != -1) {
    gtk_window_move (GTK_WINDOW (plui->window), x, y);
  }

  pathbldMakePath (imgbuff, sizeof (imgbuff), "",
      "bdj4_icon", ".png", PATHBLD_MP_IMGDIR);
  osuiSetIcon (imgbuff);

  pluiSetExtraQueues (plui);

  logProcEnd (LOG_PROC, "pluiActivate", "");
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
  playerui_t    *plui = udata;
  int           flags;

  logProcBegin (LOG_PROC, "pluiListeningCallback");
  flags = PROCUTIL_DETACH;
  if ((plui->dbgflags & BDJ4_INIT_NO_DETACH) == BDJ4_INIT_NO_DETACH) {
    flags = PROCUTIL_NO_DETACH;
  }
  plui->processes [ROUTE_MAIN] = procutilStartProcess (
      ROUTE_MAIN, "bdj4main", flags);
  logProcEnd (LOG_PROC, "pluiListeningCallback", "");
  return true;
}

static bool
pluiConnectingCallback (void *udata, programstate_t programState)
{
  playerui_t   *plui = udata;
  bool        rc = false;

  logProcBegin (LOG_PROC, "pluiConnectingCallback");

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

  logProcEnd (LOG_PROC, "pluiConnectingCallback", "");
  return rc;
}

static bool
pluiHandshakeCallback (void *udata, programstate_t programState)
{
  playerui_t   *plui = udata;
  bool          rc = false;

  logProcBegin (LOG_PROC, "pluiHandshakeCallback");

  if (connHaveHandshake (plui->conn, ROUTE_MAIN) &&
      connHaveHandshake (plui->conn, ROUTE_PLAYER)) {
    progstateLogTime (plui->progstate, "time-to-start-gui");
    rc = true;
  }

  logProcEnd (LOG_PROC, "pluiHandshakeCallback", "");
  return rc;
}

static int
pluiProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  playerui_t       *plui = udata;

  logProcBegin (LOG_PROC, "pluiProcessMsg");

  logMsg (LOG_DBG, LOG_MSGS, "got: from:%ld/%s route:%ld/%s msg:%ld/%s args:%s",
      routefrom, msgRouteDebugText (routefrom),
      route, msgRouteDebugText (route), msg, msgDebugText (msg), args);

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

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  logProcEnd (LOG_PROC, "pluiProcessMsg", "");
  return gKillReceived;
}


static gboolean
pluiCloseWin (GtkWidget *window, GdkEvent *event, gpointer userdata)
{
  playerui_t   *plui = userdata;

  logProcBegin (LOG_PROC, "pluiCloseWin");
  if (! gdone) {
    progstateShutdownProcess (plui->progstate);
    logMsg (LOG_DBG, LOG_MSGS, "got: close win request");
    logProcEnd (LOG_PROC, "pluiCloseWin", "not-done");
    return TRUE;
  }

  logProcEnd (LOG_PROC, "pluiCloseWin", "");
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

  logProcBegin (LOG_PROC, "pluiSwitchPage");
  pluiSetSwitchPage (plui, pagenum);
  logProcEnd (LOG_PROC, "pluiSwitchPage", "");
}

static void
pluiSetSwitchPage (playerui_t *plui, int pagenum)
{
  logProcBegin (LOG_PROC, "pluiSetSwitchPage");
  /* note that the design requires that the music queues be the first */
  /* tabs in the notebook */
  if (pagenum < MUSICQ_MAX &&
      nlistGetNum (plui->options, PLUI_SHOW_EXTRA_QUEUES)) {
    plui->musicqManageIdx = pagenum;
    uimusicqSetManageIdx (plui->uimusicq, pagenum);
    gtk_widget_show (plui->setPlaybackButton);
  } else {
    gtk_widget_hide (plui->setPlaybackButton);
  }
  logProcEnd (LOG_PROC, "pluiSetSwitchPage", "");
  return;
}

static void
pluiProcessSetPlaybackQueue (GtkButton *b, gpointer udata)
{
  playerui_t      *plui = udata;

  logProcBegin (LOG_PROC, "pluiProcessSetPlaybackQueue");
  pluiSetPlaybackQueue (plui, plui->musicqManageIdx);
  logProcEnd (LOG_PROC, "pluiProcessSetPlaybackQueue", "");
}

static void
pluiSetPlaybackQueue (playerui_t  *plui, musicqidx_t newQueue)
{
  char            tbuff [40];

  logProcBegin (LOG_PROC, "pluiSetPlaybackQueue");
  if (nlistGetNum (plui->options, PLUI_SHOW_EXTRA_QUEUES)) {
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
  logProcEnd (LOG_PROC, "pluiSetPlaybackQueue", "");
}

static void
pluiTogglePlayWhenQueued (GtkWidget *mi, gpointer udata)
{
  playerui_t      *plui = udata;
  ssize_t         val;

  logProcBegin (LOG_PROC, "pluiTogglePlayWhenQueued");
  val = nlistGetNum (plui->options, PLUI_PLAY_WHEN_QUEUED);
  val = ! val;
  nlistSetNum (plui->options, PLUI_PLAY_WHEN_QUEUED, val);
  pluiSetPlayWhenQueued (plui);
  logProcEnd (LOG_PROC, "pluiTogglePlayWhenQueued", "");
}

static void
pluiSetPlayWhenQueued (playerui_t *plui)
{
  char  tbuff [40];

  logProcBegin (LOG_PROC, "pluiSetPlayWhenQueued");
  snprintf (tbuff, sizeof (tbuff), "%zd",
      nlistGetNum (plui->options, PLUI_PLAY_WHEN_QUEUED));
  connSendMessage (plui->conn, ROUTE_MAIN, MSG_QUEUE_PLAY_ON_ADD, tbuff);
  logProcEnd (LOG_PROC, "pluiSetPlayWhenQueued", "");
}


static void
pluiToggleExtraQueues (GtkWidget *mi, gpointer udata)
{
  playerui_t      *plui = udata;
  ssize_t         val;

  logProcBegin (LOG_PROC, "pluiToggleExtraQueues");
  val = nlistGetNum (plui->options, PLUI_SHOW_EXTRA_QUEUES);
  val = ! val;
  nlistSetNum (plui->options, PLUI_SHOW_EXTRA_QUEUES, val);
  pluiSetExtraQueues (plui);
  logProcEnd (LOG_PROC, "pluiToggleExtraQueues", "");
}

static void
pluiSetExtraQueues (playerui_t *plui)
{
  GtkWidget       *page;
  int             pagenum;

  logProcBegin (LOG_PROC, "pluiSetExtraQueues");
  if (plui->notebook == NULL ||
      plui->setPlaybackButton == NULL) {
    logProcEnd (LOG_PROC, "pluiSetExtraQueues", "no-notebook");
    return;
  }

  page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (plui->notebook), MUSICQ_B);
  gtk_widget_set_visible (page,
      nlistGetNum (plui->options, PLUI_SHOW_EXTRA_QUEUES));
  if (nlistGetNum (plui->options, PLUI_SHOW_EXTRA_QUEUES)) {
    pagenum = gtk_notebook_get_current_page (GTK_NOTEBOOK (plui->notebook));
    /* I wish there was a way to identify the notebook tab */
    if (pagenum == 0) {
      gtk_widget_show (plui->setPlaybackButton);
    }
  } else {
    gtk_widget_hide (plui->setPlaybackButton);
  }
  logProcEnd (LOG_PROC, "pluiSetExtraQueues", "");
}

static void
pluiToggleSwitchQueue (GtkWidget *mi, gpointer udata)
{
  playerui_t      *plui = udata;
  ssize_t         val;

  logProcBegin (LOG_PROC, "pluiToggleSwitchQueue");
  val = nlistGetNum (plui->options, PLUI_SWITCH_QUEUE_WHEN_EMPTY);
  val = ! val;
  nlistSetNum (plui->options, PLUI_SWITCH_QUEUE_WHEN_EMPTY, val);
  pluiSetSwitchQueue (plui);
  logProcEnd (LOG_PROC, "pluiToggleSwitchQueue", "");
}

static void
pluiSetSwitchQueue (playerui_t *plui)
{
  char  tbuff [40];

  logProcBegin (LOG_PROC, "pluiSetSwitchQueue");
  snprintf (tbuff, sizeof (tbuff), "%zd",
      nlistGetNum (plui->options, PLUI_SWITCH_QUEUE_WHEN_EMPTY));
  connSendMessage (plui->conn, ROUTE_MAIN, MSG_QUEUE_SWITCH_EMPTY, tbuff);
  logProcEnd (LOG_PROC, "pluiSetSwitchQueue", "");
}

