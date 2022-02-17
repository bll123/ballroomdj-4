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
#include "bdjvars.h"
#include "conn.h"
#include "localeutil.h"
#include "log.h"
#include "pathbld.h"
#include "procutil.h"
#include "progstate.h"
#include "sockh.h"
#include "uiutils.h"

typedef struct {
  progstate_t     *progstate;
  char            *locknm;
  procutil_t      *processes [ROUTE_MAX];
  conn_t          *conn;
  sockserver_t    *sockserver;
  /* gtk stuff */
  GtkApplication  *app;
  GtkWidget       *window;
} startui_t;

#define STARTER_EXIT_WAIT_COUNT      20

static bool     starterStoppingCallback (void *udata, programstate_t programState);
static bool     starterClosingCallback (void *udata, programstate_t programState);
static int      starterCreateGui (startui_t *starter, int argc, char *argv []);
static void     starterActivate (GApplication *app, gpointer userdata);
gboolean        starterMainLoop  (void *tstarter);
static int      starterProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static gboolean starterCloseWin (GtkWidget *window, GdkEvent *event, gpointer userdata);
static void     starterSigHandler (int sig);

static void     starterStartPlayer (GtkButton *b, gpointer udata);
static void     starterStartManage (GtkButton *b, gpointer udata);
static void     starterStartConfig (GtkButton *b, gpointer udata);
static void     starterProcessExit (GtkButton *b, gpointer udata);


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
  startui_t      starter;
  char            tbuff [MAXPATHLEN];
  char            *uifont;


  static struct option bdj_options [] = {
    { "debug",      required_argument,  NULL,   'd' },
    { "profile",    required_argument,  NULL,   'p' },
    { "bdj4playerui", no_argument,      NULL,   0 },
    { "playerui",   no_argument,        NULL,   0 },
    { NULL,         0,                  NULL,   0 }
  };


  starter.progstate = progstateInit ("starterui");
  progstateSetCallback (starter.progstate, STATE_STOPPING,
      starterStoppingCallback, &starter);
  progstateSetCallback (starter.progstate, STATE_CLOSING,
      starterClosingCallback, &starter);
  starter.sockserver = NULL;
  starter.window = NULL;

  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    starter.processes [i] = NULL;
  }

#if _define_SIGHUP
  procutilCatchSignal (starterSigHandler, SIGHUP);
#endif
  procutilCatchSignal (starterSigHandler, SIGINT);
  procutilDefaultSignal (SIGTERM);
#if _define_SIGCHLD
  procutilDefaultSignal (SIGCHLD);
#endif

  bdj4startup (argc, argv, "st", ROUTE_STARTERUI);
  localeInit ();
  logProcBegin (LOG_PROC, "starterui");

  listenPort = bdjvarsGetNum (BDJVL_STARTERUI_PORT);
  starter.conn = connInit (ROUTE_STARTERUI);

  starter.sockserver = sockhStartServer (listenPort);

  uiutilsInitGtkLog ();
  gtk_init (&argc, NULL);
  uifont = bdjoptGetData (OPT_MP_UIFONT);
  uiutilsSetUIFont (uifont);

  g_timeout_add (UI_MAIN_LOOP_TIMER, starterMainLoop, &starter);

  status = starterCreateGui (&starter, 0, NULL);

  while (progstateShutdownProcess (starter.progstate) != STATE_CLOSED) {
    ;
  }
  progstateFree (starter.progstate);
  logEnd ();
  return status;
}

/* internal routines */

static bool
starterStoppingCallback (void *udata, programstate_t programState)
{
  startui_t   *starter = udata;

  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    if (starter->processes [i] != NULL) {
      procutilStopProcess (starter->processes [i], starter->conn, i, false);
    }
  }

  gdone = 1;
  connDisconnectAll (starter->conn);
  return true;
}

static bool
starterClosingCallback (void *udata, programstate_t programState)
{
  startui_t   *starter = udata;

  connFree (starter->conn);
  sockhCloseServer (starter->sockserver);

  bdj4shutdown (ROUTE_STARTERUI);

  /* give the other processes some time to shut down */
  mssleep (200);
  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    if (starter->processes [i] != NULL) {
      procutilStopProcess (starter->processes [i], starter->conn, i, true);
      procutilFree (starter->processes [i]);
    }
  }
  return true;
}

static int
starterCreateGui (startui_t *starter, int argc, char *argv [])
{
  int           status;

  starter->app = gtk_application_new (
      "org.bdj4.BDJ4.starterui",
      G_APPLICATION_NON_UNIQUE
  );

  g_signal_connect (starter->app, "activate", G_CALLBACK (starterActivate), starter);

  status = g_application_run (G_APPLICATION (starter->app), argc, argv);
  gtk_widget_destroy (starter->window);
  g_object_unref (starter->app);
  return status;
}

static void
starterActivate (GApplication *app, gpointer userdata)
{
  startui_t           *starter = userdata;
  GError              *gerr = NULL;
  GtkWidget           *widget;
  GtkWidget           *vbox;
  char                *str;
  char                imgbuff [MAXPATHLEN];
  char                tbuff [MAXPATHLEN];

  starter->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  assert (starter->window != NULL);
  gtk_window_set_application (GTK_WINDOW (starter->window), GTK_APPLICATION (app));
  gtk_window_set_application (GTK_WINDOW (starter->window), starter->app);
  gtk_window_set_default_icon_from_file (imgbuff, &gerr);
  g_signal_connect (starter->window, "delete-event", G_CALLBACK (starterCloseWin), starter);
  gtk_window_set_title (GTK_WINDOW (starter->window), bdjoptGetData (OPT_P_PROFILENAME));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (starter->window), vbox);
  gtk_widget_set_margin_top (GTK_WIDGET (vbox), 4);
  gtk_widget_set_margin_bottom (GTK_WIDGET (vbox), 4);
  gtk_widget_set_margin_start (GTK_WIDGET (vbox), 4);
  gtk_widget_set_margin_end (GTK_WIDGET (vbox), 4);

  pathbldMakePath (tbuff, sizeof (tbuff), "", "ballroomdj4", ".svg",
      PATHBLD_MP_IMGDIR);
  widget = gtk_image_new_from_file (tbuff);
  assert (widget != NULL);
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_button_set_label (GTK_BUTTON (widget), _("Player"));
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);
  g_signal_connect (widget, "clicked",
      G_CALLBACK (starterStartPlayer), starter);

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_button_set_label (GTK_BUTTON (widget), _("Manage"));
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);
  g_signal_connect (widget, "clicked",
      G_CALLBACK (starterStartManage), starter);

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_button_set_label (GTK_BUTTON (widget), _("Configure"));
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);
  g_signal_connect (widget, "clicked",
      G_CALLBACK (starterStartConfig), starter);

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_button_set_label (GTK_BUTTON (widget), _("Exit"));
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);
  g_signal_connect (widget, "clicked",
      G_CALLBACK (starterProcessExit), starter);

  gtk_widget_show_all (GTK_WIDGET (starter->window));
}

gboolean
starterMainLoop (void *tstarter)
{
  startui_t   *starter = tstarter;
  int         tdone = 0;
  gboolean    cont = TRUE;

  tdone = sockhProcessMain (starter->sockserver, starterProcessMsg, starter);
  if (tdone || gdone) {
    ++gdone;
  }
  if (gdone > STARTER_EXIT_WAIT_COUNT) {
    g_application_quit (G_APPLICATION (starter->app));
    cont = FALSE;
  }

  if (! progstateIsRunning (starter->progstate)) {
    progstateProcess (starter->progstate);
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      progstateShutdownProcess (starter->progstate);
    }
    return cont;
  }

  if (! connIsConnected (starter->conn, ROUTE_PLAYERUI)) {
    connConnect (starter->conn, ROUTE_PLAYERUI);
  }
  if (! connIsConnected (starter->conn, ROUTE_MANAGEUI)) {
    connConnect (starter->conn, ROUTE_MANAGEUI);
  }
  if (! connIsConnected (starter->conn, ROUTE_CONFIGUI)) {
    connConnect (starter->conn, ROUTE_CONFIGUI);
  }

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (starter->progstate);
  }
  return cont;
}

static int
starterProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  startui_t       *starter = udata;

  logProcBegin (LOG_PROC, "starterProcessMsg");

  logMsg (LOG_DBG, LOG_MSGS, "got: from %d route: %d msg:%d args:%s",
      routefrom, route, msg, args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_STARTERUI: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (starter->conn, routefrom);
          connConnectResponse (starter->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          logMsg (LOG_DBG, LOG_MSGS, "got: req-exit");
          progstateShutdownProcess (starter->progstate);
          logProcEnd (LOG_PROC, "starterProcessMsg", "req-exit");
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

  logProcEnd (LOG_PROC, "starterProcessMsg", "");

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  return gKillReceived;
}


static gboolean
starterCloseWin (GtkWidget *window, GdkEvent *event, gpointer userdata)
{
  startui_t   *starter = userdata;

  if (! gdone) {
    progstateShutdownProcess (starter->progstate);
    logMsg (LOG_DBG, LOG_MSGS, "got: close win request");
    return TRUE;
  }

  return FALSE;
}

static void
starterSigHandler (int sig)
{
  gKillReceived = 1;
}

static void
starterStartPlayer (GtkButton *b, gpointer udata)
{
  startui_t      *starter = udata;

  starter->processes [ROUTE_PLAYERUI] = procutilStartProcess (
      ROUTE_PLAYERUI, "bdj4playerui");
}

static void
starterStartManage (GtkButton *b, gpointer udata)
{
  startui_t      *starter = udata;

//  starter->processes [ROUTE_MANAGEUI] = procutilStartProcess (
//      ROUTE_MANAGEUI, "bdj4manageui");
}

static void
starterStartConfig (GtkButton *b, gpointer udata)
{
  startui_t      *starter = udata;

//  starter->processes [ROUTE_CONFIGUI] = procutilStartProcess (
//      ROUTE_CONFIGUI, "bdj4configui");
}


static void
starterProcessExit (GtkButton *b, gpointer udata)
{
  startui_t      *starter = udata;

  gdone = 1;
}

