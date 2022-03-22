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
#include "fileop.h"
#include "localeutil.h"
#include "log.h"
#include "nlist.h"
#include "pathbld.h"
#include "procutil.h"
#include "progstate.h"
#include "sockh.h"
#include "sysvars.h"
#include "uiutils.h"

typedef struct {
  progstate_t     *progstate;
  char            *locknm;
  procutil_t      *processes [ROUTE_MAX];
  conn_t          *conn;
  sockserver_t    *sockserver;
  nlist_t         *dispProfileList;
  nlist_t         *profileIdxMap;
  ssize_t         currprofile;
  int             maxProfileWidth;
  /* gtk stuff */
  uiutilsspinbox_t  profilesel;
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

static void     starterGetProfiles (startui_t *starter);


static int gKillReceived = 0;
static int gdone = 0;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  startui_t       starter;
  char            *uifont;


  starter.progstate = progstateInit ("starterui");
  progstateSetCallback (starter.progstate, STATE_STOPPING,
      starterStoppingCallback, &starter);
  progstateSetCallback (starter.progstate, STATE_CLOSING,
      starterClosingCallback, &starter);
  starter.sockserver = NULL;
  starter.window = NULL;
  starter.maxProfileWidth = 0;
  starter.dispProfileList = NULL;
  starter.profileIdxMap = NULL;

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

  bdj4startup (argc, argv, "st", ROUTE_STARTERUI, BDJ4_INIT_NO_DB_LOAD);
  localeInit ();
  logProcBegin (LOG_PROC, "starterui");

  /* get the profile list after bdjopt has been initialized */
  starterGetProfiles (&starter);
  uiutilsSpinboxTextInit (&starter.profilesel);

  listenPort = bdjvarsGetNum (BDJVL_STARTERUI_PORT);
  starter.conn = connInit (ROUTE_STARTERUI);

  starter.sockserver = sockhStartServer (listenPort);

  uiutilsInitGtkLog ();
  gtk_init (&argc, NULL);
  uifont = bdjoptGetStr (OPT_MP_UIFONT);
  uiutilsSetUIFont (uifont);

  g_timeout_add (UI_MAIN_LOOP_TIMER, starterMainLoop, &starter);

  status = starterCreateGui (&starter, 0, NULL);

  while (progstateShutdownProcess (starter.progstate) != STATE_CLOSED) {
    ;
  }
  uiutilsSpinboxTextFree (&starter.profilesel);
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
  nlistFree (starter->dispProfileList);
  nlistFree (starter->profileIdxMap);

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
  GtkWidget           *hbox;
  char                imgbuff [MAXPATHLEN];
  char                tbuff [MAXPATHLEN];

  starter->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  assert (starter->window != NULL);
  gtk_window_set_application (GTK_WINDOW (starter->window), GTK_APPLICATION (app));
  gtk_window_set_application (GTK_WINDOW (starter->window), starter->app);
  gtk_window_set_default_icon_from_file (imgbuff, &gerr);
  g_signal_connect (starter->window, "delete-event", G_CALLBACK (starterCloseWin), starter);
  gtk_window_set_title (GTK_WINDOW (starter->window), bdjoptGetStr (OPT_P_PROFILENAME));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (starter->window), vbox);
  gtk_widget_set_margin_top (vbox, 4);
  gtk_widget_set_margin_bottom (vbox, 4);
  gtk_widget_set_margin_start (vbox, 4);
  gtk_widget_set_margin_end (vbox, 4);

  pathbldMakePath (tbuff, sizeof (tbuff), "", "ballroomdj4", ".svg",
      PATHBLD_MP_IMGDIR);
  widget = gtk_image_new_from_file (tbuff);
  assert (widget != NULL);
  gtk_widget_set_margin_top (widget, 2);
  gtk_widget_set_margin_start (widget, 2);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  widget = uiutilsCreateColonLabel (_("Profile"));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  widget = uiutilsSpinboxTextCreate (&starter->profilesel, starter);
  uiutilsSpinboxTextSet (&starter->profilesel, starter->currprofile,
      nlistGetCount (starter->dispProfileList),
      starter->maxProfileWidth, starter->dispProfileList, NULL);
  gtk_widget_set_margin_start (widget, 8);
  gtk_widget_set_halign (widget, GTK_ALIGN_FILL);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  widget = uiutilsCreateButton (_("Player"), NULL, starterStartPlayer, starter);
  gtk_widget_set_margin_top (widget, 4);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  widget = uiutilsCreateButton (_("Manage"), NULL, starterStartManage, starter);
  gtk_widget_set_margin_top (widget, 4);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  widget = uiutilsCreateButton (_("Configure"), NULL, starterStartConfig, starter);
  gtk_widget_set_margin_top (widget, 4);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  widget = uiutilsCreateButton (_("Exit"), NULL, starterProcessExit, starter);
  gtk_widget_set_margin_top (widget, 4);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  gtk_widget_show_all (starter->window);
}

gboolean
starterMainLoop (void *tstarter)
{
  startui_t   *starter = tstarter;
  int         tdone = 0;
  gboolean    cont = TRUE;
  int         idx;
  int         profidx;

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

  idx = uiutilsSpinboxTextGetValue (&starter->profilesel);
  profidx = nlistGetNum (starter->profileIdxMap, idx);
  sysvarsSetNum (SVL_BDJIDX, profidx);

  if (starter->processes [ROUTE_PLAYERUI] != NULL &&
      ! connIsConnected (starter->conn, ROUTE_PLAYERUI)) {
    connConnect (starter->conn, ROUTE_PLAYERUI);
  }
  if (starter->processes [ROUTE_MANAGEUI] != NULL &&
      ! connIsConnected (starter->conn, ROUTE_MANAGEUI)) {
    connConnect (starter->conn, ROUTE_MANAGEUI);
  }
  if (starter->processes [ROUTE_CONFIGUI] != NULL &&
      ! connIsConnected (starter->conn, ROUTE_CONFIGUI)) {
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
      ROUTE_PLAYERUI, "bdj4playerui", PROCUTIL_DETACH);
}

static void
starterStartManage (GtkButton *b, gpointer udata)
{
//  startui_t      *starter = udata;

//  starter->processes [ROUTE_MANAGEUI] = procutilStartProcess (
//      ROUTE_MANAGEUI, "bdj4manageui");
}

static void
starterStartConfig (GtkButton *b, gpointer udata)
{
  startui_t      *starter = udata;

  starter->processes [ROUTE_CONFIGUI] = procutilStartProcess (
      ROUTE_CONFIGUI, "bdj4configui", PROCUTIL_DETACH);
}


static void
starterProcessExit (GtkButton *b, gpointer udata)
{
  gdone = 1;
}

static void
starterGetProfiles (startui_t *starter)
{
  char        tbuff [MAXPATHLEN];
  datafile_t  *df;
  int         count;
  size_t      max;
  size_t      len;
  nlist_t     *dflist;
  char        *pname;

  starter->currprofile = sysvarsGetNum (SVL_BDJIDX);

  starter->dispProfileList = nlistAlloc ("profile-list", LIST_ORDERED, free);
  starter->profileIdxMap = nlistAlloc ("profile-map", LIST_ORDERED, NULL);
  max = 0;

  count = 0;
  for (int i = 0; i < 20; ++i) {
    sysvarsSetNum (SVL_BDJIDX, i);
    pathbldMakePath (tbuff, sizeof (tbuff), "profiles",
        BDJ_CONFIG_BASEFN, BDJ_CONFIG_EXT, PATHBLD_MP_USEIDX);
    if (fileopFileExists (tbuff)) {
      if (i == starter->currprofile) {
        pname = bdjoptGetStr (OPT_P_PROFILENAME);
      } else {
        df = datafileAllocParse ("bdjopt-prof", DFTYPE_KEY_VAL, tbuff,
            bdjoptprofiledfkeys, bdjoptprofiledfcount, DATAFILE_NO_LOOKUP);
        dflist = datafileGetList (df);
        pname = nlistGetStr (dflist, OPT_P_PROFILENAME);
      }
      len = strlen (pname);
      max = len > max ? len : max;
      nlistSetStr (starter->dispProfileList, count, pname);
      nlistSetNum (starter->profileIdxMap, count, i);
      if (i != starter->currprofile) {
        datafileFree (df);
      }
      ++count;
    }
  }

  starter->maxProfileWidth = (int) max;
  sysvarsSetNum (SVL_BDJIDX, starter->currprofile);
}

