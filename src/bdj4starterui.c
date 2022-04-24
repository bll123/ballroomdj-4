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
#include <glib.h>
#include <zlib.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjvars.h"
#include "conn.h"
#include "fileop.h"
#include "filemanip.h"
#include "localeutil.h"
#include "lock.h"
#include "log.h"
#include "nlist.h"
#include "osutils.h"
#include "pathbld.h"
#include "procutil.h"
#include "progstate.h"
#include "sockh.h"
#include "sysvars.h"
#include "templateutil.h"
#include "uiutils.h"
#include "webclient.h"

typedef enum {
  START_STATE_NONE,
  START_STATE_DELAY,
  START_STATE_SUPPORT_INIT,
  START_STATE_SUPPORT_SEND_MSG,
  START_STATE_SUPPORT_SEND_INFO,
  START_STATE_SUPPORT_SEND_FILES_A,
  START_STATE_SUPPORT_SEND_FILES_B,
  START_STATE_SUPPORT_SEND_FILES_C,
  START_STATE_SUPPORT_SEND_FILES_D,
  START_STATE_SUPPORT_SEND_DB_PRE,
  START_STATE_SUPPORT_SEND_DB,
  START_STATE_SUPPORT_FINISH,
  START_STATE_SUPPORT_SEND_FILE,
} startstate_t;

typedef struct {
  progstate_t     *progstate;
  char            *locknm;
  procutil_t      *processes [ROUTE_MAX];
  conn_t          *conn;
  sockserver_t    *sockserver;
  nlist_t         *dispProfileList;
  nlist_t         *profileIdxMap;
  ssize_t         currprofile;
  ssize_t         newprofile;
  int             maxProfileWidth;
  startstate_t    startState;
  startstate_t    nextState;
  startstate_t    delayState;
  int             delayCount;
  char            *supportDir;
  slist_t         *supportFileList;
  slistidx_t      supportFileIterIdx;
  char            *supportInFname;
  char            *supportOutFname;
  webclient_t     *webclient;
  char            ident [80];
  /* gtk stuff */
  uiutilsspinbox_t  profilesel;
  GtkApplication    *app;
  GtkWidget         *window;
  GtkWidget         *supportDialog;
  GtkWidget         *supportSendFiles;
  GtkWidget         *supportSendDB;
  GtkWidget         *supportStatus;
  uiutilstextbox_t  *supporttb;
  uiutilsentry_t    supportsubject;
  uiutilsentry_t    supportemail;
  /* options */
  datafile_t      *optiondf;
  nlist_t         *options;
} startui_t;

enum {
  STARTERUI_POSITION_X,
  STARTERUI_POSITION_Y,
  STARTERUI_SIZE_X,
  STARTERUI_SIZE_Y,
  STARTERUI_KEY_MAX,
};

static datafilekey_t starteruidfkeys [STARTERUI_KEY_MAX] = {
  { "STARTERUI_POS_X",     STARTERUI_POSITION_X,    VALUE_NUM, NULL, -1 },
  { "STARTERUI_POS_Y",     STARTERUI_POSITION_Y,    VALUE_NUM, NULL, -1 },
  { "STARTERUI_SIZE_X",    STARTERUI_SIZE_X,        VALUE_NUM, NULL, -1 },
  { "STARTERUI_SIZE_Y",    STARTERUI_SIZE_Y,        VALUE_NUM, NULL, -1 },
};

#define STARTER_EXIT_WAIT_COUNT 20
#define SUPPORT_BUFF_SZ         (10*1024*1024)
#define LOOP_DELAY              5

static bool     starterConnectingCallback (void *udata, programstate_t programState);
static bool     starterStoppingCallback (void *udata, programstate_t programState);
static bool     starterClosingCallback (void *udata, programstate_t programState);
static void     starterActivate (GApplication *app, gpointer userdata);
gboolean        starterMainLoop  (void *tstarter);
static int      starterProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static gboolean starterCloseWin (GtkWidget *window, GdkEvent *event, gpointer userdata);
static void     starterSigHandler (int sig);

static void     starterStartPlayer (GtkButton *b, gpointer udata);
static void     starterStartManage (GtkButton *b, gpointer udata);
static void     starterStartConfig (GtkButton *b, gpointer udata);
static void     starterStartRaffleGames (GtkButton *b, gpointer udata);
static void     starterProcessSupport (GtkButton *b, gpointer udata);
static void     starterProcessExit (GtkButton *b, gpointer udata);

static void     starterGetProfiles (startui_t *starter);
static char     * starterSetProfile (void *udata, int idx);
static void     starterCheckProfile (startui_t *starter);

static void     starterProcessSupport (GtkButton *b, gpointer udata);
static void     starterSupportResponseHandler (GtkDialog *d, gint responseid, gpointer udata);
static void     starterCreateSupportDialog (GtkButton *b, gpointer udata);
static void     starterSupportMsgHandler (GtkDialog *d, gint responseid, gpointer udata);
static void     starterSendFilesInit (startui_t *starter, char *dir);
static void     starterSendFiles (startui_t *starter);
static void     starterSendFile (startui_t *starter, char *origfn, char *fn);
static void     starterSendFileCallback (void *userdata, char *resp, size_t len);

static void     starterCompressFile (char *infn, char *outfn);
static z_stream * starterGzipInit (char *out, int outsz);
static void     starterGzip (z_stream *zs, const char* in, int insz);
static size_t   starterGzipEnd (z_stream *zs);

static void     starterStopAllProcesses (GtkMenuItem *mi, gpointer udata);
static int      starterCountProcesses (startui_t *starter);

static int gKillReceived = 0;
static int gdone = 0;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  startui_t       starter;
  char            *uifont;
  char            tbuff [MAXPATHLEN];
  int             flags;


  starter.progstate = progstateInit ("starterui");
  progstateSetCallback (starter.progstate, STATE_CONNECTING,
      starterConnectingCallback, &starter);
  progstateSetCallback (starter.progstate, STATE_STOPPING,
      starterStoppingCallback, &starter);
  progstateSetCallback (starter.progstate, STATE_CLOSING,
      starterClosingCallback, &starter);
  starter.sockserver = NULL;
  starter.window = NULL;
  starter.maxProfileWidth = 0;
  starter.dispProfileList = NULL;
  starter.profileIdxMap = NULL;
  starter.startState = START_STATE_NONE;
  starter.nextState = START_STATE_NONE;
  starter.delayState = START_STATE_NONE;
  starter.delayCount = 0;
  starter.supportDir = NULL;
  starter.supportFileList = NULL;
  starter.supportInFname = NULL;
  starter.supportOutFname = NULL;
  starter.webclient = NULL;

  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    starter.processes [i] = NULL;
  }

#if _define_SIGHUP
  procutilCatchSignal (starterSigHandler, SIGHUP);
#endif
  procutilCatchSignal (starterSigHandler, SIGINT);
  procutilDefaultSignal (SIGTERM);
#if _define_SIGCHLD
  procutilIgnoreSignal (SIGCHLD);
#endif

  flags = BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  bdj4startup (argc, argv, NULL, "st", ROUTE_STARTERUI, flags);
  logProcBegin (LOG_PROC, "starterui");

  /* get the profile list after bdjopt has been initialized */
  starterGetProfiles (&starter);
  uiutilsSpinboxTextInit (&starter.profilesel);

  listenPort = bdjvarsGetNum (BDJVL_STARTERUI_PORT);
  starter.conn = connInit (ROUTE_STARTERUI);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "starterui", ".txt", PATHBLD_MP_USEIDX);
  starter.optiondf = datafileAllocParse ("starterui-opt", DFTYPE_KEY_VAL, tbuff,
      starteruidfkeys, STARTERUI_KEY_MAX, DATAFILE_NO_LOOKUP);
  starter.options = datafileGetList (starter.optiondf);
  if (starter.options == NULL) {
    starter.options = nlistAlloc ("starterui-opt", LIST_ORDERED, free);

    nlistSetNum (starter.options, STARTERUI_POSITION_X, -1);
    nlistSetNum (starter.options, STARTERUI_POSITION_Y, -1);
    nlistSetNum (starter.options, STARTERUI_SIZE_X, 1200);
    nlistSetNum (starter.options, STARTERUI_SIZE_Y, 800);
  }

  starter.sockserver = sockhStartServer (listenPort);

  uiutilsInitUILog ();
  gtk_init (&argc, NULL);
  uifont = bdjoptGetStr (OPT_MP_UIFONT);
  uiutilsSetUIFont (uifont);

  g_timeout_add (UI_MAIN_LOOP_TIMER, starterMainLoop, &starter);

  status = uiutilsCreateApplication (0, NULL, "starterui",
      &starter.app, starterActivate, &starter);

  while (progstateShutdownProcess (starter.progstate) != STATE_CLOSED) {
    ;
  }

  progstateFree (starter.progstate);
  logProcEnd (LOG_PROC, "starterui", "");
  logEnd ();
  return status;
}

/* internal routines */

static bool
starterConnectingCallback (void *udata, programstate_t programState)
{
  startui_t *starter = udata;

  /* the starter process only need to re-try a connection if it */
  /* had crashed.  these connections are not possible during normal */
  /* startup */

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

  return true;
}

static bool
starterStoppingCallback (void *udata, programstate_t programState)
{
  startui_t   *starter = udata;
  gint        x, y;

  gtk_window_get_size (GTK_WINDOW (starter->window), &x, &y);
  nlistSetNum (starter->options, STARTERUI_SIZE_X, x);
  nlistSetNum (starter->options, STARTERUI_SIZE_Y, y);
  gtk_window_get_position (GTK_WINDOW (starter->window), &x, &y);
  nlistSetNum (starter->options, STARTERUI_POSITION_X, x);
  nlistSetNum (starter->options, STARTERUI_POSITION_Y, y);

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
  char        fn [MAXPATHLEN];

  if (GTK_IS_WIDGET (starter->window)) {
    gtk_widget_destroy (starter->window);
  }

  uiutilsSpinboxTextFree (&starter->profilesel);

  pathbldMakePath (fn, sizeof (fn),
      "starterui", ".txt", PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("starterui", fn, starteruidfkeys, STARTERUI_KEY_MAX, starter->options);

  connFree (starter->conn);
  sockhCloseServer (starter->sockserver);
  nlistFree (starter->dispProfileList);
  nlistFree (starter->profileIdxMap);
  if (starter->optiondf != NULL) {
    datafileFree (starter->optiondf);
  }
  if (starter->supportDir != NULL) {
    free (starter->supportDir);
  }
  if (starter->supportFileList != NULL) {
    slistFree (starter->supportFileList);
  }

  bdj4shutdown (ROUTE_STARTERUI, NULL);

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

static void
starterActivate (GApplication *app, gpointer userdata)
{
  startui_t           *starter = userdata;
  GtkWidget           *widget;
  GtkWidget           *menubar;
  GtkWidget           *menu;
  GtkWidget           *menuitem;
  GtkWidget           *vbox;
  GtkWidget           *bvbox;
  GtkWidget           *hbox;
  GdkPixbuf           *image;
  GtkSizeGroup        *sg;
  GError              *gerr = NULL;
  char                imgbuff [MAXPATHLEN];
  char                tbuff [MAXPATHLEN];
  int                 x, y;

  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", ".svg", PATHBLD_MP_IMGDIR);
  starter->window = uiutilsCreateMainWindow (app, BDJ4_LONG_NAME, imgbuff,
      starterCloseWin, starter);

  vbox = uiutilsCreateVertBox ();
  uiutilsWidgetSetAllMargins (vbox, uiutilsBaseMarginSz * 2);
  gtk_container_add (GTK_CONTAINER (starter->window), vbox);

  hbox = uiutilsCreateHorizBox ();
  gtk_widget_set_margin_top (hbox, 8);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  menubar = gtk_menu_bar_new ();
  gtk_box_pack_start (GTK_BOX (hbox), menubar, FALSE, FALSE, 0);

  menuitem = gtk_menu_item_new_with_label (_("Actions"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), menuitem);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

  snprintf (tbuff, sizeof (tbuff), _("Stop All %s Processes"), BDJ4_NAME);
  menuitem = gtk_menu_item_new_with_label (tbuff);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  g_signal_connect (menuitem, "activate",
      G_CALLBACK (starterStopAllProcesses), starter);

  /* main display */
  hbox = uiutilsCreateHorizBox ();
  gtk_widget_set_margin_top (hbox, 8);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* CONTEXT: starter: profile to be used when starting BDJ4 */
  widget = uiutilsCreateColonLabel (_("Profile"));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  widget = uiutilsSpinboxTextCreate (&starter->profilesel, starter);
  uiutilsSpinboxTextSet (&starter->profilesel, starter->currprofile,
      nlistGetCount (starter->dispProfileList),
      starter->maxProfileWidth, starter->dispProfileList, starterSetProfile);
  gtk_widget_set_margin_start (widget, 8);
  gtk_widget_set_halign (widget, GTK_ALIGN_FILL);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  hbox = uiutilsCreateHorizBox ();
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  bvbox = uiutilsCreateVertBox ();
  gtk_box_pack_start (GTK_BOX (hbox), bvbox, FALSE, FALSE, 0);

  pathbldMakePath (tbuff, sizeof (tbuff),
     "bdj4_icon", ".svg", PATHBLD_MP_IMGDIR);
  image = gdk_pixbuf_new_from_file_at_scale (tbuff, 128, -1, TRUE, &gerr);
  assert (image != NULL);
  widget = gtk_image_new_from_pixbuf (image);
  assert (widget != NULL);
  gtk_widget_set_hexpand (widget, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  /* CONTEXT: button: starts the player user interface */
  widget = uiutilsCreateButton (_("Player"), NULL, starterStartPlayer, starter);
  gtk_widget_set_margin_top (widget, 4);
  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_size_group_add_widget (sg, widget);
  gtk_box_pack_start (GTK_BOX (bvbox), widget, FALSE, FALSE, 0);
  widget = gtk_bin_get_child (GTK_BIN (widget));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);

  /* CONTEXT: button: starts the management user interface */
  widget = uiutilsCreateButton (_("Manage"), NULL, starterStartManage, starter);
  gtk_widget_set_margin_top (widget, 4);
  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_size_group_add_widget (sg, widget);
  gtk_box_pack_start (GTK_BOX (bvbox), widget, FALSE, FALSE, 0);
  widget = gtk_bin_get_child (GTK_BIN (widget));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);

  /* CONTEXT: button: starts the configuration user interface */
  widget = uiutilsCreateButton (_("Configure"), NULL, starterStartConfig, starter);
  gtk_widget_set_margin_top (widget, 4);
  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_size_group_add_widget (sg, widget);
  gtk_box_pack_start (GTK_BOX (bvbox), widget, FALSE, FALSE, 0);
  widget = gtk_bin_get_child (GTK_BIN (widget));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);

  /* CONTEXT: button: support : starts raffle games  */
  widget = uiutilsCreateButton (_("Raffle Games"), NULL, starterStartRaffleGames, starter);
  gtk_widget_set_margin_top (widget, 4);
  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_size_group_add_widget (sg, widget);
  gtk_box_pack_start (GTK_BOX (bvbox), widget, FALSE, FALSE, 0);
  widget = gtk_bin_get_child (GTK_BIN (widget));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);

  /* CONTEXT: button: support : support information */
  widget = uiutilsCreateButton (_("Support"), NULL, starterProcessSupport, starter);
  gtk_widget_set_margin_top (widget, 4);
  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_size_group_add_widget (sg, widget);
  gtk_box_pack_start (GTK_BOX (bvbox), widget, FALSE, FALSE, 0);
  widget = gtk_bin_get_child (GTK_BIN (widget));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);

  /* CONTEXT: button: exits BDJ4 */
  widget = uiutilsCreateButton (_("Exit"), NULL, starterProcessExit, starter);
  gtk_widget_set_margin_top (widget, 4);
  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_size_group_add_widget (sg, widget);
  gtk_box_pack_start (GTK_BOX (bvbox), widget, FALSE, FALSE, 0);
  widget = gtk_bin_get_child (GTK_BIN (widget));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);

  x = nlistGetNum (starter->options, STARTERUI_POSITION_X);
  y = nlistGetNum (starter->options, STARTERUI_POSITION_Y);
  if (x != -1 && y != -1) {
    gtk_window_move (GTK_WINDOW (starter->window), x, y);
  }

  gtk_widget_show_all (starter->window);
}

gboolean
starterMainLoop (void *tstarter)
{
  startui_t   *starter = tstarter;
  int         tdone = 0;
  gboolean    cont = TRUE;
  /* support message handling */
  char        tbuff [MAXPATHLEN];
  char        ofn [MAXPATHLEN];


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

  switch (starter->startState) {
    case START_STATE_NONE: {
      break;
    }
    case START_STATE_DELAY: {
      ++starter->delayCount;
      if (starter->delayCount > LOOP_DELAY) {
        starter->delayCount = 0;
        starter->startState = starter->delayState;
      }
      break;
    }
    case START_STATE_SUPPORT_INIT: {
      char        datestr [40];
      char        tmstr [40];

      tmutilDstamp (datestr, sizeof (datestr));
      tmutilTstamp (tmstr, sizeof (tmstr));
      snprintf (starter->ident, sizeof (starter->ident), "%s-%s-%s",
          sysvarsGetStr (SV_USER_MUNGE), datestr, tmstr);

      starter->webclient = webclientAlloc (starter, starterSendFileCallback);
      /* CONTEXT: starterui: support: status message */
      snprintf (tbuff, sizeof (tbuff), _("Sending Support Message"));
      uiutilsLabelSetText (starter->supportStatus, tbuff);
      starter->delayCount = 0;
      starter->delayState = START_STATE_SUPPORT_SEND_MSG;
      starter->startState = START_STATE_DELAY;
      break;
    }
    case START_STATE_SUPPORT_SEND_MSG: {
      const char  *email;
      const char  *subj;
      char        *msg;
      FILE        *fh;

      email = uiutilsEntryGetValue (&starter->supportemail);
      subj = uiutilsEntryGetValue (&starter->supportsubject);
      msg = uiutilsTextBoxGetValue (starter->supporttb);

      strlcpy (tbuff, "support.txt", sizeof (tbuff));
      fh = fopen (tbuff, "w");
      if (fh != NULL) {
        fprintf (fh, " Ident: %s\n", starter->ident);
        fprintf (fh, " E-Mail: %s\n", email);
        fprintf (fh, " Subject: %s\n", subj);
        fprintf (fh, " Message:\n%s\n", msg);
        fclose (fh);
      }
      free (msg);

      pathbldMakePath (ofn, sizeof (ofn),
          tbuff, ".gz.b64", PATHBLD_MP_TMPDIR);
      starterSendFile (starter, tbuff, ofn);
      fileopDelete (tbuff);

      /* CONTEXT: starterui: support: status message */
      snprintf (tbuff, sizeof (tbuff), _("Sending %s Information"), BDJ4_NAME);
      uiutilsLabelSetText (starter->supportStatus, tbuff);
      starter->delayCount = 0;
      starter->delayState = START_STATE_SUPPORT_SEND_INFO;
      starter->startState = START_STATE_DELAY;
      break;
    }
    case START_STATE_SUPPORT_SEND_INFO: {
      char        prog [MAXPATHLEN];
      char        arg [40];
      char        *targv [4];
      int         targc = 0;

      pathbldMakePath (prog, sizeof (prog),
          "bdj4info", sysvarsGetStr (SV_OS_EXEC_EXT), PATHBLD_MP_EXECDIR);
      strlcpy (arg, "--bdj4", MAXPATHLEN);
      strlcpy (tbuff, "bdj4info.txt", sizeof (tbuff));
      targv [targc++] = prog;
      targv [targc++] = arg;
      targv [targc++] = NULL;
      osProcessStart (targv, OS_PROC_WAIT, NULL, tbuff);
      pathbldMakePath (ofn, sizeof (ofn),
          tbuff, ".gz.b64", PATHBLD_MP_TMPDIR);
      starterSendFile (starter, tbuff, ofn);
      fileopDelete (tbuff);

      starter->startState = START_STATE_SUPPORT_SEND_FILES_A;
      break;
    }
    case START_STATE_SUPPORT_SEND_FILES_A: {
      bool        sendfiles;

      sendfiles = gtk_toggle_button_get_active (
            GTK_TOGGLE_BUTTON (starter->supportSendFiles));
      if (! sendfiles) {
        starter->startState = START_STATE_SUPPORT_SEND_DB_PRE;
        break;
      }

      pathbldMakePath (tbuff, sizeof (tbuff),
          "", "", PATHBLD_MP_NONE);
      starterSendFilesInit (starter, tbuff);
      starter->startState = START_STATE_SUPPORT_SEND_FILE;
      starter->nextState = START_STATE_SUPPORT_SEND_FILES_B;
      break;
    }
    case START_STATE_SUPPORT_SEND_FILES_B: {
      pathbldMakePath (tbuff, sizeof (tbuff),
          "", "", PATHBLD_MP_USEIDX);
      starterSendFilesInit (starter, tbuff);
      starter->startState = START_STATE_SUPPORT_SEND_FILE;
      starter->nextState = START_STATE_SUPPORT_SEND_FILES_C;
      break;
    }
    case START_STATE_SUPPORT_SEND_FILES_C: {
      pathbldMakePath (tbuff, sizeof (tbuff),
          "", "", PATHBLD_MP_HOSTNAME);
      starterSendFilesInit (starter, tbuff);
      starter->startState = START_STATE_SUPPORT_SEND_FILE;
      starter->nextState = START_STATE_SUPPORT_SEND_FILES_D;
      break;
    }
    case START_STATE_SUPPORT_SEND_FILES_D: {
      pathbldMakePath (tbuff, sizeof (tbuff),
          "", "", PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
      starterSendFilesInit (starter, tbuff);
      starter->startState = START_STATE_SUPPORT_SEND_FILE;
      starter->nextState = START_STATE_SUPPORT_SEND_DB_PRE;
      break;
    }
    case START_STATE_SUPPORT_SEND_DB_PRE: {
      bool        senddb;

      senddb = gtk_toggle_button_get_active (
            GTK_TOGGLE_BUTTON (starter->supportSendDB));
      if (! senddb) {
        starter->startState = START_STATE_SUPPORT_FINISH;
        break;
      }
      /* CONTEXT: starterui: support: status message */
      snprintf (tbuff, sizeof (tbuff), _("Sending %s"), "data/musicdb.dat");
      uiutilsLabelSetText (starter->supportStatus, tbuff);
      starter->delayCount = 0;
      starter->delayState = START_STATE_SUPPORT_SEND_DB;
      starter->startState = START_STATE_DELAY;
      break;
    }
    case START_STATE_SUPPORT_SEND_DB: {
      bool        senddb;

      senddb = gtk_toggle_button_get_active (
            GTK_TOGGLE_BUTTON (starter->supportSendDB));
      if (senddb) {
        strlcpy (tbuff, "data/musicdb.dat", sizeof (tbuff));
        pathbldMakePath (ofn, sizeof (ofn),
            "musicdb.dat", ".gz.b64", PATHBLD_MP_TMPDIR);
        starterSendFile (starter, tbuff, ofn);
      }
      starter->startState = START_STATE_SUPPORT_FINISH;
      break;
    }
    case START_STATE_SUPPORT_FINISH: {
      webclientClose (starter->webclient);
      gtk_widget_destroy (GTK_WIDGET (starter->supportDialog));
      uiutilsEntryFree (&starter->supportsubject);
      uiutilsEntryFree (&starter->supportemail);
      starter->startState = START_STATE_NONE;
      break;
    }
    case START_STATE_SUPPORT_SEND_FILE: {
      starterSendFiles (starter);
      starter->delayCount = 0;
      starter->delayState = starter->startState;
      starter->startState = START_STATE_DELAY;
      break;
    }
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

  starterCheckProfile (starter);
  starter->processes [ROUTE_PLAYERUI] = procutilStartProcess (
      ROUTE_PLAYERUI, "bdj4playerui", PROCUTIL_DETACH);
}

static void
starterStartManage (GtkButton *b, gpointer udata)
{
  startui_t      *starter = udata;

  starterCheckProfile (starter);
  starter->processes [ROUTE_MANAGEUI] = procutilStartProcess (
      ROUTE_MANAGEUI, "bdj4manageui", PROCUTIL_DETACH);
}

static void
starterStartRaffleGames (GtkButton *b, gpointer udata)
{
//  startui_t      *starter = udata;

//  starterCheckProfile (starter);
//  starter->processes [ROUTE_RAFFLE] = procutilStartProcess (
//      ROUTE_RAFFLE, "bdj4raffle");
}

static void
starterStartConfig (GtkButton *b, gpointer udata)
{
  startui_t      *starter = udata;

  starterCheckProfile (starter);
  starter->processes [ROUTE_CONFIGUI] = procutilStartProcess (
      ROUTE_CONFIGUI, "bdj4configui", PROCUTIL_DETACH);
}

static void
starterProcessSupport (GtkButton *b, gpointer udata)
{
  startui_t     *starter = udata;
  GtkWidget     *content;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *widget;
  GtkWidget     *dialog;
  char          tbuff [MAXPATHLEN];
  char          uri [MAXPATHLEN];

  dialog = gtk_dialog_new_with_buttons (
      /* CONTEXT: title for the support dialog */
      _("Support"),
      GTK_WINDOW (starter->window),
      GTK_DIALOG_DESTROY_WITH_PARENT,
      /* CONTEXT: action button for the support dialog */
      _("Close"),
      GTK_RESPONSE_CLOSE,
      NULL
      );

  content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  uiutilsWidgetSetAllMargins (content, uiutilsBaseMarginSz * 2);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  assert (vbox != NULL);
  gtk_widget_set_hexpand (vbox, FALSE);
  gtk_widget_set_vexpand (vbox, FALSE);
  gtk_container_add (GTK_CONTAINER (content), vbox);

  /* line 1 */
  /* CONTEXT: starterui: basic support dialog, listing support options */
  widget = uiutilsCreateColonLabel (_("Support options"));
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  /* line 2 */
  snprintf (uri, sizeof (uri), "%s%s",
      sysvarsGetStr (SV_FORUM_HOST), sysvarsGetStr (SV_FORUM_URI));
  /* CONTEXT: starterui: basic support dialog: support option */
  snprintf (tbuff, sizeof (tbuff), _("%s Forums"), BDJ4_NAME);
  widget = uiutilsCreateLink (tbuff, uri);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  /* line 3 */
  snprintf (uri, sizeof (uri), "%s%s",
      sysvarsGetStr (SV_SUPPORT_HOST), sysvarsGetStr (SV_SUPPORT_URI));
  /* CONTEXT: starterui: basic support dialog: support option */
  snprintf (tbuff, sizeof (tbuff), _("%s Support Tickets"), BDJ4_NAME);
  widget = uiutilsCreateLink (tbuff, uri);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  /* line 4 */
  /* CONTEXT: starterui: basic support dialog: button: support option */
  widget = uiutilsCreateButton (_("Send Support Message"), NULL,
      starterCreateSupportDialog, starter);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  /* the dialog doesn't have any space above the buttons */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  widget = uiutilsCreateLabel (" ");
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  g_signal_connect (dialog, "response",
      G_CALLBACK (starterSupportResponseHandler), starter);
  gtk_widget_show_all (dialog);
}


static void
starterSupportResponseHandler (GtkDialog *d, gint responseid, gpointer udata)
{
  switch (responseid) {
    case GTK_RESPONSE_DELETE_EVENT: {
      break;
    }
    case GTK_RESPONSE_CLOSE: {
      gtk_widget_destroy (GTK_WIDGET (d));
      break;
    }
  }
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
  int         availidx = -1;

  starter->newprofile = -1;
  starter->currprofile = sysvarsGetNum (SVL_BDJIDX);

  starter->dispProfileList = nlistAlloc ("profile-list", LIST_ORDERED, free);
  starter->profileIdxMap = nlistAlloc ("profile-map", LIST_ORDERED, NULL);
  max = 0;

  count = 0;
  for (int i = 0; i < BDJOPT_MAX_PROFILES; ++i) {
    sysvarsSetNum (SVL_BDJIDX, i);
    pathbldMakePath (tbuff, sizeof (tbuff),
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
    } else if (availidx == -1) {
      availidx = i;
    }
  }

  /* CONTEXT: starter: creating a new profile */
  nlistSetStr (starter->dispProfileList, count, _("New Profile"));
  nlistSetNum (starter->profileIdxMap, count, availidx);
  starter->newprofile = availidx;
  len = strlen (nlistGetStr (starter->dispProfileList, count));
  max = len > max ? len : max;
  starter->maxProfileWidth = (int) max;
  sysvarsSetNum (SVL_BDJIDX, starter->currprofile);
}

static char *
starterSetProfile (void *udata, int idx)
{
  startui_t *starter = udata;
  char      *disp;
  int       profidx;

  profidx = nlistGetNum (starter->profileIdxMap, idx);
  sysvarsSetNum (SVL_BDJIDX, profidx);
  disp = nlistGetStr (starter->dispProfileList, idx);
  return disp;
}

static void
starterCheckProfile (startui_t *starter)
{
  if (sysvarsGetNum (SVL_BDJIDX) == starter->newprofile) {
    char  tbuff [100];
    int   profidx;

    bdjoptInit ();
    profidx = sysvarsGetNum (SVL_BDJIDX);

    /* CONTEXT: starter: name of the new profile (New profile 9) */
    snprintf (tbuff, sizeof (tbuff), "%s %d", _("New Profile"), profidx);
    bdjoptSetStr (OPT_P_PROFILENAME, tbuff);
    bdjoptSave ();
    templateDisplaySettingsCopy ();
    /* do not re-run the new profile init */
    starter->newprofile = -1;
  }
}


static void
starterCreateSupportDialog (GtkButton *b, gpointer udata)
{
  startui_t     *starter = udata;
  GtkWidget     *content;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *widget;
  GtkWidget     *dialog;
  GtkSizeGroup  *sg;
  uiutilstextbox_t *tb;
  char          tbuff [200];

  dialog = gtk_dialog_new_with_buttons (
      /* CONTEXT: title for the support message dialog */
      _("Support Message"),
      GTK_WINDOW (starter->window),
      GTK_DIALOG_DESTROY_WITH_PARENT,
      /* CONTEXT: action button for the support message dialog */
      _("Close"),
      GTK_RESPONSE_CLOSE,
      /* CONTEXT: action button for the support message dialog */
      _("Send Support Message"),
      GTK_RESPONSE_APPLY,
      NULL
      );
  gtk_window_set_default_size (GTK_WINDOW (dialog), -1, 400);

  content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  uiutilsWidgetSetAllMargins (content, uiutilsBaseMarginSz * 2);

  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  assert (vbox != NULL);
  gtk_widget_set_hexpand (vbox, FALSE);
  gtk_widget_set_vexpand (vbox, FALSE);
  gtk_container_add (GTK_CONTAINER (content), vbox);

  /* line 1 */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* CONTEXT: sending support message: user's e-mail address */
  widget = uiutilsCreateColonLabel (_("E-Mail Address"));
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sg, widget);

  uiutilsEntryInit (&starter->supportemail, 50, 100);
  widget = uiutilsEntryCreate (&starter->supportemail);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  /* line 2 */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* CONTEXT: sending support message: subject of message */
  widget = uiutilsCreateColonLabel (_("Subject"));
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sg, widget);

  uiutilsEntryInit (&starter->supportsubject, 50, 100);
  widget = uiutilsEntryCreate (&starter->supportsubject);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  /* line 3 */
  /* CONTEXT: sending support message: message */
  widget = uiutilsCreateColonLabel (_("Message"));
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  /* line 4 */
  tb = uiutilsTextBoxCreate ();
  uiutilsTextBoxSetHeight (tb, 200);
  gtk_box_pack_start (GTK_BOX (vbox), tb->scw, FALSE, FALSE, 0);
  starter->supporttb = tb;

  /* line 5 */
  /* CONTEXT: sending support message: checkbox: option to send data files */
  widget = uiutilsCreateCheckButton (_("Attach Data Files"), 0);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  starter->supportSendFiles = widget;

  /* line 6 */
  /* CONTEXT: sending support message: checkbox: option to send database */
  widget = uiutilsCreateCheckButton (_("Attach Database"), 0);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  starter->supportSendDB = widget;

  /* line 7 */
  widget = uiutilsCreateLabel ("");
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  snprintf (tbuff, sizeof (tbuff),
      "label { color: %s; }", bdjoptGetStr (OPT_P_UI_ACCENT_COL));
  uiutilsSetCss (widget, tbuff);
  starter->supportStatus = widget;

  g_signal_connect (dialog, "response",
      G_CALLBACK (starterSupportMsgHandler), starter);
  gtk_widget_show_all (dialog);
  starter->supportDialog = dialog;
}


static void
starterSupportMsgHandler (GtkDialog *d, gint responseid, gpointer udata)
{
  startui_t   *starter = udata;

  switch (responseid) {
    case GTK_RESPONSE_DELETE_EVENT: {
      break;
    }
    case GTK_RESPONSE_CLOSE: {
      gtk_widget_destroy (GTK_WIDGET (d));
      break;
    }
    case GTK_RESPONSE_APPLY: {
      starter->startState = START_STATE_SUPPORT_INIT;
      break;
    }
  }
}

static void
starterSendFilesInit (startui_t *starter, char *dir)
{
  slist_t     *list;

  list = filemanipBasicDirList (dir, ".txt");
  starter->supportFileList = list;
  slistStartIterator (list, &starter->supportFileIterIdx);
  if (starter->supportDir != NULL) {
    free (starter->supportDir);
  }
  starter->supportDir = strdup (dir);
}

static void
starterSendFiles (startui_t *starter)
{
  char        *fn;
  char        ifn [MAXPATHLEN];
  char        ofn [MAXPATHLEN];
  char        tbuff [100];

  if (starter->supportInFname != NULL) {
    starterSendFile (starter, starter->supportInFname, starter->supportOutFname);
    free (starter->supportInFname);
    free (starter->supportOutFname);
    starter->supportInFname = NULL;
    starter->supportOutFname = NULL;
    return;
  }

  if ((fn = slistIterateKey (starter->supportFileList,
      &starter->supportFileIterIdx)) == NULL) {
    slistFree (starter->supportFileList);
    starter->supportFileList = NULL;
    if (starter->supportDir != NULL) {
      free (starter->supportDir);
    }
    starter->supportDir = NULL;
    starter->startState = starter->nextState;
    return;
  }

  strlcpy (ifn, starter->supportDir, sizeof (ifn));
  strlcat (ifn, "/", sizeof (ifn));
  strlcat (ifn, fn, sizeof (ifn));
  pathbldMakePath (ofn, sizeof (ofn),
      fn, ".gz.b64", PATHBLD_MP_TMPDIR);
  starter->supportInFname = strdup (ifn);
  starter->supportOutFname = strdup (ofn);
  /* CONTEXT: starterui: support: status message */
  snprintf (tbuff, sizeof (tbuff), _("Sending %s"), ifn);
  uiutilsLabelSetText (starter->supportStatus, tbuff);
}

static void
starterSendFile (startui_t *starter, char *origfn, char *fn)
{
  char      uri [1024];
  char      *query [7];

  starterCompressFile (origfn, fn);
  snprintf (uri, sizeof (uri), "%s%s",
      sysvarsGetStr (SV_SUPPORTMSG_HOST), sysvarsGetStr (SV_SUPPORTMSG_URI));
  query [0] = "key";
  query [1] = "9034545";
  query [2] = "ident";
  query [3] = starter->ident;
  query [4] = "origfn";
  query [5] = origfn;
  query [6] = NULL;
  webclientUploadFile (starter->webclient, uri, query, fn);
  fileopDelete (fn);
}

static void
starterSendFileCallback (void *userdata, char *resp, size_t len)
{
  return;
}

static void
starterCompressFile (char *infn, char *outfn)
{
  FILE      *infh = NULL;
  FILE      *outfh = NULL;
  char      *buff;
  char      *obuff;
  char      *data;
  size_t    r;
  size_t    olen;
  z_stream  *zs;

  infh = fopen (infn, "rb");
  if (infh == NULL) {
    return;
  }
  outfh = fopen (outfn, "wb");
  if (outfh == NULL) {
    return;
  }

  buff = malloc (SUPPORT_BUFF_SZ);
  assert (buff != NULL);
  /* if the database becomes so large that 10 megs compressed can't hold it */
  /* then there will be a problem */
  obuff = malloc (SUPPORT_BUFF_SZ);
  assert (obuff != NULL);

  zs = starterGzipInit (obuff, SUPPORT_BUFF_SZ);
  while ((r = fread (buff, 1, SUPPORT_BUFF_SZ, infh)) > 0) {
    starterGzip (zs, buff, r);
  }
  olen = starterGzipEnd (zs);
  data = g_base64_encode ((const guchar *) obuff, olen);
  fwrite (data, strlen (data), 1, outfh);
  free (data);

  fclose (infh);
  fclose (outfh);
  free (buff);
  free (obuff);
}

static z_stream *
starterGzipInit (char *out, int outsz)
{
  z_stream *zs;

  zs = malloc (sizeof (z_stream));
  zs->zalloc = Z_NULL;
  zs->zfree = Z_NULL;
  zs->opaque = Z_NULL;
  zs->avail_in = (uInt) 0;
  zs->next_in = (Bytef *) NULL;
  zs->avail_out = (uInt) outsz;
  zs->next_out = (Bytef *) out;

  // hard to believe they don't have a macro for gzip encoding, "Add 16" is the best thing zlib can do:
  // "Add 16 to windowBits to write a simple gzip header and trailer around the compressed data instead of a zlib wrapper"
  deflateInit2 (zs, 6, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);
  return zs;
}

static void
starterGzip (z_stream *zs, const char* in, int insz)
{
  zs->avail_in = (uInt) insz;
  zs->next_in = (Bytef *) in;

  deflate (zs, Z_NO_FLUSH);
}

static size_t
starterGzipEnd (z_stream *zs)
{
  size_t    olen;

  zs->avail_in = (uInt) 0;
  zs->next_in = (Bytef *) NULL;

  deflate (zs, Z_FINISH);
  olen = zs->total_out;
  deflateEnd (zs);
  free (zs);
  return olen;
}

static void
starterStopAllProcesses (GtkMenuItem *mi, gpointer udata)
{
  startui_t     *starter = udata;
  bdjmsgroute_t route;
  char          *locknm;
  pid_t         pid;
  int           count;

  logProcBegin (LOG_PROC, "starterStopAllProcesses");
  fprintf (stderr, "stop-all-processes\n");

  count = starterCountProcesses (starter);
  if (count <= 1) {
    logProcEnd (LOG_PROC, "starterStopAllProcesses", "begin-only-one");
    fprintf (stderr, "done\n");
    return;
  }

  /* send the standard exit request to the main controlling processes first */
  logMsg (LOG_DBG, LOG_IMPORTANT, "send exit request to ui");
  fprintf (stderr, "send exit request to ui\n");
  if (! connIsConnected (starter->conn, ROUTE_PLAYERUI)) {
    connConnect (starter->conn, ROUTE_PLAYERUI);
  }
  connSendMessage (starter->conn, ROUTE_PLAYERUI, MSG_EXIT_REQUEST, NULL);
  if (! connIsConnected (starter->conn, ROUTE_MANAGEUI)) {
    connConnect (starter->conn, ROUTE_MANAGEUI);
  }
  connSendMessage (starter->conn, ROUTE_MANAGEUI, MSG_EXIT_REQUEST, NULL);
  if (! connIsConnected (starter->conn, ROUTE_CONFIGUI)) {
    connConnect (starter->conn, ROUTE_CONFIGUI);
  }
  connSendMessage (starter->conn, ROUTE_CONFIGUI, MSG_EXIT_REQUEST, NULL);
  logMsg (LOG_DBG, LOG_IMPORTANT, "sleeping");
  fprintf (stderr, "sleeping\n");
  mssleep (1000);

  count = starterCountProcesses (starter);
  if (count <= 1) {
    logProcEnd (LOG_PROC, "starterStopAllProcesses", "after-ui");
    fprintf (stderr, "done\n");
    return;
  }

  if (isWindows ()) {
    /* windows is slow */
    mssleep (1500);
  }

  /* send the exit request to main */
  logMsg (LOG_DBG, LOG_IMPORTANT, "send exit request to main");
  fprintf (stderr, "send exit request to main\n");
  if (! connIsConnected (starter->conn, ROUTE_MAIN)) {
    connConnect (starter->conn, ROUTE_MAIN);
  }
  connSendMessage (starter->conn, ROUTE_MAIN, MSG_EXIT_REQUEST, NULL);
  logMsg (LOG_DBG, LOG_IMPORTANT, "sleeping");
  fprintf (stderr, "sleeping\n");
  mssleep (1000);

  count = starterCountProcesses (starter);
  if (count <= 1) {
    logProcEnd (LOG_PROC, "starterStopAllProcesses", "after-main");
    fprintf (stderr, "done\n");
    return;
  }

  /* see which lock files exist, and send exit requests to them */
  count = 0;
  for (route = ROUTE_NONE + 1; route < ROUTE_MAX; ++route) {
    if (route == ROUTE_STARTERUI) {
      continue;
    }

    locknm = lockName (route);
    pid = lockExists (locknm, PATHBLD_MP_USEIDX);
    if (pid > 0) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "route %d %s exists; send exit request",
          route, msgRouteDebugText (route));
      fprintf (stderr, "route %d %s exists; send exit request\n",
          route, msgRouteDebugText (route));
      if (! connIsConnected (starter->conn, route)) {
        connConnect (starter->conn, route);
      }
      connSendMessage (starter->conn, route, MSG_EXIT_REQUEST, NULL);
      ++count;
    }
  }

  if (count <= 0) {
    logProcEnd (LOG_PROC, "starterStopAllProcesses", "after-exit-all");
    fprintf (stderr, "done\n");
    return;
  }

  logMsg (LOG_DBG, LOG_IMPORTANT, "sleeping");
  fprintf (stderr, "sleeping\n");
  mssleep (1000);

  /* see which lock files still exist and kill the processes */
  count = 0;
  for (route = ROUTE_NONE + 1; route < ROUTE_MAX; ++route) {
    if (route == ROUTE_STARTERUI) {
      continue;
    }

    locknm = lockName (route);
    pid = lockExists (locknm, PATHBLD_MP_USEIDX);
    if (pid > 0) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "lock %d %s exists; send terminate",
          route, msgRouteDebugText (route));
      fprintf (stderr, "lock %d %s exists; send terminate\n",
          route, msgRouteDebugText (route));
      procutilTerminate (pid, false);
      ++count;
    }
  }

  if (count <= 0) {
    logProcEnd (LOG_PROC, "starterStopAllProcesses", "after-term");
    fprintf (stderr, "done\n");
    return;
  }

  logMsg (LOG_DBG, LOG_IMPORTANT, "sleeping");
  fprintf (stderr, "sleeping\n");
  mssleep (1000);

  /* see which lock files still exist and kill the processes with */
  /* a signal that is not caught; remove the lock file */
  for (route = ROUTE_NONE + 1; route < ROUTE_MAX; ++route) {
    if (route == ROUTE_STARTERUI) {
      continue;
    }

    locknm = lockName (route);
    pid = lockExists (locknm, PATHBLD_MP_USEIDX);
    if (pid > 0) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "lock %d %s exists; force terminate",
          route, msgRouteDebugText (route));
      fprintf (stderr, "lock %d %s exists; force terminate\n",
          route, msgRouteDebugText (route));
      procutilTerminate (pid, true);
    }
    mssleep (100);
    pid = lockExists (locknm, PATHBLD_MP_USEIDX);
    if (pid > 0) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "removing lock %d %s",
          route, msgRouteDebugText (route));
      fprintf (stderr, "removing lock %d %s\n",
          route, msgRouteDebugText (route));
      fileopDelete (locknm);
    }
  }
  fprintf (stderr, "done\n");
  logProcEnd (LOG_PROC, "starterStopAllProcesses", "");
}

static int
starterCountProcesses (startui_t *starter)
{
  bdjmsgroute_t route;
  char          *locknm;
  pid_t         pid;
  int           count;

  count = 0;
  for (route = ROUTE_NONE + 1; route < ROUTE_MAX; ++route) {
    locknm = lockName (route);
    pid = lockExists (locknm, PATHBLD_MP_USEIDX);
    if (pid > 0) {
      ++count;
    }
  }

  return count;
}
