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
#include "dispsel.h"
#include "localeutil.h"
#include "lock.h"
#include "log.h"
#include "musicq.h"
#include "osutils.h"
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
#include "uisongedit.h"
#include "uisongsel.h"
#include "uiutils.h"

enum {
  MANAGE_TAB_OTHER,
  MANAGE_TAB_MAIN_SL,
  MANAGE_TAB_MAIN_MM,
  MANAGE_TAB_PLMGMT,
  MANAGE_TAB_EDITSEQ,
  MANAGE_TAB_FILEMGR,
  MANAGE_TAB_SONGLIST,
  MANAGE_TAB_SONGEDIT,
};

enum {
  MANAGE_DB_CHECK_NEW,
  MANAGE_DB_REORGANIZE,
  MANAGE_DB_UPD_FROM_TAGS,
  MANAGE_DB_WRITE_TAGS,
  MANAGE_DB_REBUILD,
};

typedef struct manage manageui_t;

typedef struct manage {
  progstate_t     *progstate;
  procutil_t      *processes [ROUTE_MAX];
  char            *locknm;
  conn_t          *conn;
  musicdb_t       *musicdb;
  musicqidx_t     musicqPlayIdx;
  musicqidx_t     musicqManageIdx;
  dispsel_t       *dispsel;
  int             stopwaitcount;
  /* update database */
  uiutilsspinbox_t  dbspinbox;
  uiutilstextbox_t  *dbhelpdisp;
  uiutilstextbox_t  *dbstatus;
  nlist_t           *dblist;
  nlist_t           *dbhelp;
  /* notebook tab handling */
  int               mainlasttab;
  int               sllasttab;
  int               mmlasttab;
  uiutilsmenu_t     *currmenu;
  uiutilsmenu_t     slmenu;
  uiutilsmenu_t     songeditmenu;
  uiutilsnbtabid_t  *mainnbtabid;
  uiutilsnbtabid_t  *slnbtabid;
  uiutilsnbtabid_t  *mmnbtabid;
  /* gtk stuff */
  GtkWidget       *menubar;
  GtkWidget       *window;
  GtkWidget       *mainnotebook;
  GtkWidget       *slnotebook;
  GtkWidget       *mmnotebook;
  GtkWidget       *vbox;
  GtkWidget       *dbpbar;
  GtkWidget       *statusMsg;
  /* song list ui major elements */
  uiplayer_t      *slplayer;
  uimusicq_t      *slmusicq;
  uisongsel_t     *slsongsel;
  uisongedit_t    *slsongedit;
  /* music manager ui */
  uiplayer_t      *mmplayer;
  uimusicq_t      *mmmusicq;
  uisongsel_t     *mmsongsel;
  uisongedit_t    *mmsongedit;
  /* options */
  datafile_t      *optiondf;
  nlist_t         *options;
} manageui_t;

/* re-use the plui enums so that the songsel filter enums can also be used */
static datafilekey_t manageuidfkeys [] = {
  { "FILTER_POS_X",     SONGSEL_FILTER_POSITION_X,  VALUE_NUM, NULL, -1 },
  { "FILTER_POS_Y",     SONGSEL_FILTER_POSITION_Y,  VALUE_NUM, NULL, -1 },
  { "PLUI_POS_X",       PLUI_POSITION_X,            VALUE_NUM, NULL, -1 },
  { "PLUI_POS_Y",       PLUI_POSITION_Y,            VALUE_NUM, NULL, -1 },
  { "PLUI_SIZE_X",      PLUI_SIZE_X,                VALUE_NUM, NULL, -1 },
  { "PLUI_SIZE_Y",      PLUI_SIZE_Y,                VALUE_NUM, NULL, -1 },
  { "SORT_BY",          SONGSEL_SORT_BY,            VALUE_STR, NULL, -1 },
};
#define MANAGEUI_DFKEY_COUNT (sizeof (manageuidfkeys) / sizeof (datafilekey_t))

#define MANAGE_EXIT_WAIT_COUNT      20

static bool     manageConnectingCallback (void *udata, programstate_t programState);
static bool     manageHandshakeCallback (void *udata, programstate_t programState);
static bool     manageStoppingCallback (void *udata, programstate_t programState);
static bool     manageStopWaitCallback (void *udata, programstate_t programState);
static bool     manageClosingCallback (void *udata, programstate_t programState);
static void     manageBuildUI (manageui_t *manage);
static int      manageMainLoop  (void *tmanage);
static int      manageProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static gboolean manageCloseWin (GtkWidget *window, GdkEvent *event, gpointer userdata);
static void     manageSigHandler (int sig);
/* update database */
static void     manageDbChg (GtkSpinButton *sb, gpointer udata);
static void     manageDbStart (GtkButton *b, gpointer udata);
static void     manageDbProgressMsg (manageui_t *manage, char *args);
static void     manageDbStatusMsg (manageui_t *manage, char *args);
static void     manageSwitchPage (GtkNotebook *nb, GtkWidget *page,
    guint pagenum, gpointer udata);
static void     manageSonglistMenu (manageui_t *manage);
static void     manageSongEditMenu (manageui_t *manage);


static int gKillReceived = 0;
static int gdone = 0;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  manageui_t      manage;
  char            *uifont;
  char            tbuff [MAXPATHLEN];
  nlist_t         *tlist;
  nlist_t         *hlist;


  manage.mainnotebook = NULL;
  manage.progstate = progstateInit ("manageui");
  progstateSetCallback (manage.progstate, STATE_CONNECTING,
      manageConnectingCallback, &manage);
  progstateSetCallback (manage.progstate, STATE_WAIT_HANDSHAKE,
      manageHandshakeCallback, &manage);
  manage.window = NULL;
  manage.slplayer = NULL;
  manage.slmusicq = NULL;
  manage.slsongsel = NULL;
  manage.slsongedit = NULL;
  manage.mmplayer = NULL;
  manage.mmmusicq = NULL;
  manage.mmsongsel = NULL;
  manage.mmsongedit = NULL;
  manage.musicqPlayIdx = MUSICQ_B;
  manage.musicqManageIdx = MUSICQ_A;
  manage.stopwaitcount = 0;
  manage.dblist = NULL;
  manage.dbhelp = NULL;
  manage.currmenu = NULL;
  manage.mainlasttab = MANAGE_TAB_MAIN_SL;
  manage.sllasttab = MANAGE_TAB_SONGLIST;
  manage.mmlasttab = MANAGE_TAB_OTHER;
  uiutilsMenuInit (&manage.slmenu);
  uiutilsMenuInit (&manage.songeditmenu);
  manage.mainnbtabid = uiutilsNotebookIDInit ();
  manage.slnbtabid = uiutilsNotebookIDInit ();
  manage.mmnbtabid = uiutilsNotebookIDInit ();

  procutilInitProcesses (manage.processes);

  osSetStandardSignals (manageSigHandler);

  bdj4startup (argc, argv, &manage.musicdb,
      "mu", ROUTE_MANAGEUI, BDJ4_INIT_NONE);
  logProcBegin (LOG_PROC, "manageui");

  manage.dispsel = dispselAlloc ();
  uiutilsSpinboxTextInit (&manage.dbspinbox);
  tlist = nlistAlloc ("db-action", LIST_UNORDERED, free);
  hlist = nlistAlloc ("db-action-help", LIST_ORDERED, free);
  nlistSetStr (tlist, MANAGE_DB_CHECK_NEW, _("Check For New"));
  nlistSetStr (hlist, MANAGE_DB_CHECK_NEW,
      _("Checks for new files."));
  nlistSetStr (tlist, MANAGE_DB_REORGANIZE, _("Reorganize"));
  nlistSetStr (hlist, MANAGE_DB_REORGANIZE,
      _("Renames the audio files based on the organization settings."));
  nlistSetStr (tlist, MANAGE_DB_UPD_FROM_TAGS, _("Update from Audio File Tags"));
  nlistSetStr (hlist, MANAGE_DB_UPD_FROM_TAGS,
      _("Replaces the information in the BallroomDJ database with the audio file tag information."));
  nlistSetStr (tlist, MANAGE_DB_WRITE_TAGS, _("Write Tags to Audio Files"));
  nlistSetStr (hlist, MANAGE_DB_WRITE_TAGS,
      _("Updates the audio file tags with the information from the BallroomDJ database."));
  nlistSetStr (tlist, MANAGE_DB_REBUILD, _("Rebuild Database"));
  nlistSetStr (hlist, MANAGE_DB_REBUILD,
      _("Replaces the BallroomDJ database in its entirety. All changes to the database will be lost."));
  manage.dblist = tlist;
  manage.dbhelp = hlist;

  listenPort = bdjvarsGetNum (BDJVL_MANAGEUI_PORT);
  manage.conn = connInit (ROUTE_MANAGEUI);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "manageui", ".txt", PATHBLD_MP_USEIDX);
  manage.optiondf = datafileAllocParse ("manageui-opt", DFTYPE_KEY_VAL, tbuff,
      manageuidfkeys, MANAGEUI_DFKEY_COUNT, DATAFILE_NO_LOOKUP);
  manage.options = datafileGetList (manage.optiondf);
  if (manage.options == NULL) {
    manage.options = nlistAlloc ("manageui-opt", LIST_ORDERED, free);

    nlistSetNum (manage.options, SONGSEL_FILTER_POSITION_X, -1);
    nlistSetNum (manage.options, SONGSEL_FILTER_POSITION_Y, -1);
    nlistSetNum (manage.options, PLUI_POSITION_X, -1);
    nlistSetNum (manage.options, PLUI_POSITION_Y, -1);
    nlistSetNum (manage.options, PLUI_SIZE_X, 1000);
    nlistSetNum (manage.options, PLUI_SIZE_Y, 600);
    nlistSetStr (manage.options, SONGSEL_SORT_BY, "TITLE");
  }

  manage.slplayer = uiplayerInit (manage.progstate, manage.conn,
      manage.musicdb);
  manage.slmusicq = uimusicqInit (manage.conn,
      manage.musicdb, manage.dispsel,
      UIMUSICQ_FLAGS_NO_QUEUE | UIMUSICQ_FLAGS_NO_TOGGLE_PAUSE,
      DISP_SEL_SONGLIST);
  manage.slsongsel = uisongselInit (manage.conn,
      manage.musicdb, manage.dispsel, manage.options,
      SONG_FILTER_FOR_SELECTION, DISP_SEL_SONGSEL);
  manage.slsongedit = uisongeditInit (manage.conn,
      manage.musicdb, manage.dispsel, manage.options);

  manage.mmplayer = uiplayerInit (manage.progstate, manage.conn,
      manage.musicdb);
  manage.mmmusicq = uimusicqInit (manage.conn,
      manage.musicdb, manage.dispsel,
      UIMUSICQ_FLAGS_NO_QUEUE | UIMUSICQ_FLAGS_NO_TOGGLE_PAUSE,
      DISP_SEL_SONGLIST);
  manage.mmsongsel = uisongselInit (manage.conn,
      manage.musicdb, manage.dispsel, manage.options,
      SONG_FILTER_FOR_SELECTION, DISP_SEL_MM);
  manage.mmsongedit = uisongeditInit (manage.conn,
      manage.musicdb, manage.dispsel, manage.options);

  /* register these after calling the sub-window initialization */
  /* then these will be run last, after the other closing callbacks */
  progstateSetCallback (manage.progstate, STATE_STOPPING,
      manageStoppingCallback, &manage);
  progstateSetCallback (manage.progstate, STATE_STOP_WAIT,
      manageStopWaitCallback, &manage);
  progstateSetCallback (manage.progstate, STATE_CLOSING,
      manageClosingCallback, &manage);

  uiutilsInitUILog ();
  gtk_init (&argc, NULL);
  uifont = bdjoptGetStr (OPT_MP_UIFONT);
  uiutilsSetUIFont (uifont);

  manageBuildUI (&manage);
  sockhMainLoop (listenPort, manageProcessMsg, manageMainLoop, &manage);

  progstateFree (manage.progstate);
  logProcEnd (LOG_PROC, "manageui", "");
  logEnd ();
  return status;
}

/* internal routines */

static bool
manageStoppingCallback (void *udata, programstate_t programState)
{
  manageui_t    * manage = udata;
  gint          x, y;

  logProcBegin (LOG_PROC, "manageStoppingCallback");
  connSendMessage (manage->conn, ROUTE_STARTERUI, MSG_STOP_MAIN, NULL);

  gtk_window_get_size (GTK_WINDOW (manage->window), &x, &y);
  nlistSetNum (manage->options, PLUI_SIZE_X, x);
  nlistSetNum (manage->options, PLUI_SIZE_Y, y);
  gtk_window_get_position (GTK_WINDOW (manage->window), &x, &y);
  nlistSetNum (manage->options, PLUI_POSITION_X, x);
  nlistSetNum (manage->options, PLUI_POSITION_Y, y);

  procutilStopAllProcess (manage->processes, manage->conn, false);
  connDisconnectAll (manage->conn);
  gdone = 1;
  logProcEnd (LOG_PROC, "manageStoppingCallback", "");
  return true;
}

static bool
manageStopWaitCallback (void *udata, programstate_t programState)
{
  manageui_t  * manage = udata;
  bool        rc = true;

  logProcBegin (LOG_PROC, "manageStopWaitCallback");
  rc = connCheckAll (manage->conn);
  if (rc == false) {
    ++manage->stopwaitcount;
    if (manage->stopwaitcount > STOP_WAIT_COUNT_MAX) {
      rc = true;
    }
  }
  logProcEnd (LOG_PROC, "manageStopWaitCallback", "");
  return rc;
}

static bool
manageClosingCallback (void *udata, programstate_t programState)
{
  manageui_t    *manage = udata;
  char          fn [MAXPATHLEN];

  logProcBegin (LOG_PROC, "manageClosingCallback");

  if (GTK_IS_WIDGET (manage->window)) {
    gtk_widget_destroy (manage->window);
  }

  procutilStopAllProcess (manage->processes, manage->conn, true);
  procutilFreeAll (manage->processes);

  pathbldMakePath (fn, sizeof (fn),
      "manageui", ".txt", PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("manageui", fn, manageuidfkeys, MANAGEUI_DFKEY_COUNT, manage->options);

  bdj4shutdown (ROUTE_MANAGEUI, manage->musicdb);
  dispselFree (manage->dispsel);

  if (manage->mainnbtabid != NULL) {
    uiutilsNotebookIDFree (manage->mainnbtabid);
  }
  if (manage->slnbtabid != NULL) {
    uiutilsNotebookIDFree (manage->slnbtabid);
  }
  if (manage->mmnbtabid != NULL) {
    uiutilsNotebookIDFree (manage->mmnbtabid);
  }
  if (manage->options != datafileGetList (manage->optiondf)) {
    nlistFree (manage->options);
  }
  datafileFree (manage->optiondf);

  connFree (manage->conn);

  uiutilsTextBoxFree (manage->dbhelpdisp);
  uiutilsTextBoxFree (manage->dbstatus);
  uiplayerFree (manage->slplayer);
  uimusicqFree (manage->slmusicq);
  uisongselFree (manage->slsongsel);
  uisongeditFree (manage->slsongedit);
  uiplayerFree (manage->mmplayer);
  uimusicqFree (manage->mmmusicq);
  uisongselFree (manage->mmsongsel);
  uisongeditFree (manage->mmsongedit);
  uiutilsCleanup ();
  if (manage->dblist != NULL) {
    nlistFree (manage->dblist);
  }
  if (manage->dbhelp != NULL) {
    nlistFree (manage->dbhelp);
  }

  logProcEnd (LOG_PROC, "manageClosingCallback", "");
  return true;
}

static void
manageBuildUI (manageui_t *manage)
{
  GtkWidget           *menubar;
  GtkWidget           *notebook;
  GtkWidget           *tabLabel;
  GtkWidget           *widget;
  GtkWidget           *hbox;
  GtkWidget           *vbox;
  uiutilstextbox_t    *tb;
  char                imgbuff [MAXPATHLEN];
  char                tbuff [MAXPATHLEN];
  gint                x, y;

  logProcBegin (LOG_PROC, "manageBuildUI");
  *imgbuff = '\0';

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", ".svg", PATHBLD_MP_IMGDIR);
  /* CONTEXT: management ui window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Management"), BDJ4_NAME);
  manage->window = uiutilsCreateMainWindow (tbuff, imgbuff,
      manageCloseWin, manage);

  vbox = uiutilsCreateVertBox ();
  uiutilsWidgetSetAllMargins (vbox, 4);
  gtk_container_add (GTK_CONTAINER (manage->window), vbox);

  hbox = uiutilsCreateHorizBox ();
  gtk_widget_set_margin_top (hbox, 8);
  uiutilsBoxPackStart (vbox, hbox);

  widget = uiutilsCreateLabel ("");
  uiutilsBoxPackEnd (hbox, widget);
  snprintf (tbuff, sizeof (tbuff),
      "label { color: %s; }",
      bdjoptGetStr (OPT_P_UI_ACCENT_COL));
  uiutilsSetCss (widget, tbuff);
  manage->statusMsg = widget;

  menubar = gtk_menu_bar_new ();
  uiutilsBoxPackStart (hbox, menubar);
  manage->menubar = menubar;

  manage->mainnotebook = uiutilsCreateNotebook ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (manage->mainnotebook), GTK_POS_LEFT);
  uiutilsBoxPackStart (vbox, manage->mainnotebook);

  /* song list editor */
  vbox = uiutilsCreateVertBox ();
  uiutilsWidgetSetAllMargins (vbox, 4);

  tabLabel = uiutilsCreateLabel (_("Edit Song Lists"));
  uiutilsNotebookAppendPage (manage->mainnotebook, vbox, tabLabel);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_MAIN_SL);

  /* song list editor: player */
  widget = uiplayerBuildUI (manage->slplayer);
  gtk_widget_set_hexpand (widget, TRUE);
  uiutilsBoxPackStart (vbox, widget);

  notebook = uiutilsCreateNotebook ();
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
  manage->slnotebook = notebook;

  /* song list editor: music queue tab */
  widget = uimusicqBuildUI (manage->slmusicq, manage->window, MUSICQ_A);
  /* CONTEXT: name of song list editor tab */
  tabLabel = uiutilsCreateLabel (_("Song List"));
  uiutilsNotebookAppendPage (notebook, widget, tabLabel);
  uiutilsNotebookIDAdd (manage->slnbtabid, MANAGE_TAB_SONGLIST);

  /* song list editor: song selection tab*/
  widget = uisongselBuildUI (manage->slsongsel, manage->window);
  /* CONTEXT: name of song selection tab */
  tabLabel = uiutilsCreateLabel (_("Song Selection"));
  uiutilsNotebookAppendPage (notebook, widget, tabLabel);
  uiutilsNotebookIDAdd (manage->slnbtabid, MANAGE_TAB_OTHER);

  /* song list editor song editor tab */
  widget = uisongeditBuildUI (manage->slsongedit, manage->window);
  /* CONTEXT: name of song editor tab */
  tabLabel = uiutilsCreateLabel (_("Song Editor"));
  uiutilsNotebookAppendPage (notebook, widget, tabLabel);
  uiutilsNotebookIDAdd (manage->slnbtabid, MANAGE_TAB_SONGEDIT);

  /* music manager */
  vbox = uiutilsCreateVertBox ();
  uiutilsWidgetSetAllMargins (vbox, 4);
  tabLabel = uiutilsCreateLabel (_("Music Manager"));
  uiutilsNotebookAppendPage (manage->mainnotebook, vbox, tabLabel);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_MAIN_MM);

  /* music manager: player */
  widget = uiplayerBuildUI (manage->mmplayer);
  gtk_widget_set_hexpand (widget, TRUE);
  uiutilsBoxPackStart (vbox, widget);

  g_signal_connect (notebook, "switch-page",
      G_CALLBACK (manageSwitchPage), manage);

  notebook = uiutilsCreateNotebook ();
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
  manage->mmnotebook = notebook;

  /* music manager: song selection tab*/
  widget = uisongselBuildUI (manage->mmsongsel, manage->window);
  /* CONTEXT: name of song selection tab */
  tabLabel = uiutilsCreateLabel (_("Music Manager"));
  uiutilsNotebookAppendPage (notebook, widget, tabLabel);
  uiutilsNotebookIDAdd (manage->mmnbtabid, MANAGE_TAB_OTHER);

  /* music manager: song editor tab */
  widget = uisongeditBuildUI (manage->mmsongedit, manage->window);
  /* CONTEXT: name of song editor tab */
  tabLabel = uiutilsCreateLabel (_("Song Editor"));
  uiutilsNotebookAppendPage (notebook, widget, tabLabel);
  uiutilsNotebookIDAdd (manage->mmnbtabid, MANAGE_TAB_SONGEDIT);

  g_signal_connect (notebook, "switch-page",
      G_CALLBACK (manageSwitchPage), manage);

  /* update database */
  vbox = uiutilsCreateVertBox ();
  uiutilsWidgetSetAllMargins (vbox, 4);
  tabLabel = uiutilsCreateLabel (_("Update Database"));
  uiutilsNotebookAppendPage (manage->mainnotebook, vbox, tabLabel);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_OTHER);

  tb = uiutilsTextBoxCreate ();
  uiutilsTextBoxSetReadonly (tb);
  uiutilsTextBoxSetHeight (tb, 70);
  uiutilsBoxPackStart (vbox, tb->scw);
  manage->dbhelpdisp = tb;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  uiutilsBoxPackStart (vbox, hbox);

  widget = uiutilsSpinboxTextCreate (&manage->dbspinbox, manage);
  /* currently hard-coded at 30 chars */
  uiutilsSpinboxTextSet (&manage->dbspinbox, 0,
      nlistGetCount (manage->dblist), 30, manage->dblist, NULL);
  uiutilsSpinboxTextSetValue (&manage->dbspinbox, 0);
  g_signal_connect (widget, "value-changed", G_CALLBACK (manageDbChg), manage);
  uiutilsBoxPackStart (hbox, widget);

  widget = uiutilsCreateButton (_("Start"), NULL, manageDbStart, manage);
  uiutilsBoxPackStart (hbox, widget);

  widget = uiutilsCreateProgressBar (bdjoptGetStr (OPT_P_UI_ACCENT_COL));
  uiutilsBoxPackStart (vbox, widget);
  manage->dbpbar = widget;

  tb = uiutilsTextBoxCreate ();
  uiutilsTextBoxSetReadonly (tb);
  uiutilsTextBoxDarken (tb);
  uiutilsTextBoxSetHeight (tb, 300);
  uiutilsBoxPackStart (vbox, tb->scw);
  manage->dbstatus = tb;

  /* playlist management */
  vbox = uiutilsCreateVertBox ();
  uiutilsWidgetSetAllMargins (vbox, 4);
  tabLabel = uiutilsCreateLabel (_("Playlist Management"));
  uiutilsNotebookAppendPage (manage->mainnotebook, vbox, tabLabel);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_PLMGMT);

  /* edit sequences */
  vbox = uiutilsCreateVertBox ();
  uiutilsWidgetSetAllMargins (vbox, 4);
  tabLabel = uiutilsCreateLabel (_("Edit Sequences"));
  uiutilsNotebookAppendPage (manage->mainnotebook, vbox, tabLabel);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_EDITSEQ);

  /* file manager */
  vbox = uiutilsCreateVertBox ();
  uiutilsWidgetSetAllMargins (vbox, 4);
  tabLabel = uiutilsCreateLabel (_("File Manager"));
  uiutilsNotebookAppendPage (manage->mainnotebook, vbox, tabLabel);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_FILEMGR);

  x = nlistGetNum (manage->options, PLUI_SIZE_X);
  y = nlistGetNum (manage->options, PLUI_SIZE_Y);
  gtk_window_set_default_size (GTK_WINDOW (manage->window), x, y);

  g_signal_connect (manage->mainnotebook, "switch-page",
      G_CALLBACK (manageSwitchPage), manage);

  gtk_widget_show_all (manage->window);

  x = nlistGetNum (manage->options, PLUI_POSITION_X);
  y = nlistGetNum (manage->options, PLUI_POSITION_Y);
  if (x != -1 && y != -1) {
    gtk_window_move (GTK_WINDOW (manage->window), x, y);
  }

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", ".png", PATHBLD_MP_IMGDIR);
  osuiSetIcon (imgbuff);

  logProcEnd (LOG_PROC, "manageBuildUI", "");
}

static int
manageMainLoop (void *tmanage)
{
  manageui_t   *manage = tmanage;
  int         stop = 0;

  if (gdone > MANAGE_EXIT_WAIT_COUNT) {
    stop = TRUE;
  }

  while (! stop && gtk_events_pending ()) {
    gtk_main_iteration_do (FALSE);
  }

  if (gdone) {
    ++gdone;
  }

  if (! progstateIsRunning (manage->progstate)) {
    progstateProcess (manage->progstate);
    if (! gdone && gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      progstateShutdownProcess (manage->progstate);
      gKillReceived = 0;
    }
    return stop;
  }

  connProcessUnconnected (manage->conn);

  uiplayerMainLoop (manage->slplayer);
  uiplayerMainLoop (manage->mmplayer);
  uimusicqMainLoop (manage->slmusicq);
  uimusicqMainLoop (manage->mmmusicq);
  uisongselMainLoop (manage->slsongsel);
  uisongselMainLoop (manage->mmsongsel);

  if (! gdone && gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (manage->progstate);
    gKillReceived = 0;
  }
  return stop;
}

static bool
manageConnectingCallback (void *udata, programstate_t programState)
{
  manageui_t   *manage = udata;
  bool        rc = false;

  logProcBegin (LOG_PROC, "manageConnectingCallback");

  connProcessUnconnected (manage->conn);

  if (! connIsConnected (manage->conn, ROUTE_STARTERUI)) {
    connConnect (manage->conn, ROUTE_STARTERUI);
  }
  if (! connIsConnected (manage->conn, ROUTE_MAIN)) {
    connConnect (manage->conn, ROUTE_MAIN);
  }
  if (! connIsConnected (manage->conn, ROUTE_PLAYER)) {
    connConnect (manage->conn, ROUTE_PLAYER);
  }

  if (connIsConnected (manage->conn, ROUTE_STARTERUI)) {
    connSendMessage (manage->conn, ROUTE_STARTERUI, MSG_START_MAIN, "1");
    rc = true;
  }

  logProcEnd (LOG_PROC, "manageConnectingCallback", "");
  return rc;
}

static bool
manageHandshakeCallback (void *udata, programstate_t programState)
{
  manageui_t   *manage = udata;
  bool          rc = false;

  logProcBegin (LOG_PROC, "manageHandshakeCallback");

  connProcessUnconnected (manage->conn);

  if (! connIsConnected (manage->conn, ROUTE_MAIN)) {
    connConnect (manage->conn, ROUTE_MAIN);
  }
  if (! connIsConnected (manage->conn, ROUTE_PLAYER)) {
    connConnect (manage->conn, ROUTE_PLAYER);
  }

  if (connHaveHandshake (manage->conn, ROUTE_STARTERUI) &&
      connHaveHandshake (manage->conn, ROUTE_MAIN) &&
      connHaveHandshake (manage->conn, ROUTE_PLAYER)) {
    connSendMessage (manage->conn, ROUTE_MAIN, MSG_QUEUE_PLAY_ON_ADD, "1");
    connSendMessage (manage->conn, ROUTE_MAIN, MSG_MUSICQ_SET_PLAYBACK, "1");
    progstateLogTime (manage->progstate, "time-to-start-gui");
    manageDbChg (NULL, manage);
    rc = true;
  }

  logProcEnd (LOG_PROC, "manageHandshakeCallback", "");
  return rc;
}

static int
manageProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  manageui_t  *manage = udata;
  char        *targs;

  logProcBegin (LOG_PROC, "manageProcessMsg");

  logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
      routefrom, msgRouteDebugText (routefrom),
      route, msgRouteDebugText (route), msg, msgDebugText (msg), args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MANAGEUI: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (manage->conn, routefrom);
          break;
        }
        case MSG_SOCKET_CLOSE: {
          connDisconnect (manage->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          logMsg (LOG_DBG, LOG_MSGS, "got: req-exit");
          progstateShutdownProcess (manage->progstate);
          logProcEnd (LOG_PROC, "manageProcessMsg", "req-exit");
          return 1;
        }
        case MSG_DB_PROGRESS: {
          manageDbProgressMsg (manage, args);
          break;
        }
        case MSG_DB_STATUS_MSG: {
          manageDbStatusMsg (manage, args);
          break;
        }
        case MSG_DB_FINISH: {
          procutilCloseProcess (manage->processes [routefrom],
              manage->conn, ROUTE_DBUPDATE);
          procutilFreeRoute (manage->processes, routefrom);
          connDisconnect (manage->conn, routefrom);
          /* the database has been updated, tell the other processes to */
          /* reload it, and reload it ourselves */
          connSendMessage (manage->conn, ROUTE_STARTERUI, MSG_DATABASE_UPDATE, NULL);
          manage->musicdb = bdj4ReloadDatabase (manage->musicdb);
          uiplayerSetDatabase (manage->slplayer, manage->musicdb);
          uiplayerSetDatabase (manage->mmplayer, manage->musicdb);
          uisongselSetDatabase (manage->slsongsel, manage->musicdb);
          uisongselSetDatabase (manage->mmsongsel, manage->musicdb);
          uimusicqSetDatabase (manage->slmusicq, manage->musicdb);
          uimusicqSetDatabase (manage->mmmusicq, manage->musicdb);
          /* force a database update message */
          routefrom = ROUTE_MANAGEUI;
          msg = MSG_DATABASE_UPDATE;
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

  /* due to the db update message, these must be applied afterwards */
  targs = strdup (args);
  uiplayerProcessMsg (routefrom, route, msg, args, manage->slplayer);
  uiplayerProcessMsg (routefrom, route, msg, targs, manage->mmplayer);
  uisongselProcessMsg (routefrom, route, msg, args, manage->slsongsel);
  uisongselProcessMsg (routefrom, route, msg, targs, manage->mmsongsel);
  uimusicqProcessMsg (routefrom, route, msg, args, manage->slmusicq);
  uimusicqProcessMsg (routefrom, route, msg, targs, manage->mmmusicq);
  free (targs);

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  logProcEnd (LOG_PROC, "manageProcessMsg", "");
  return gKillReceived;
}


static gboolean
manageCloseWin (GtkWidget *window, GdkEvent *event, gpointer userdata)
{
  manageui_t   *manage = userdata;

  logProcBegin (LOG_PROC, "manageCloseWin");
  if (! gdone) {
    progstateShutdownProcess (manage->progstate);
    logMsg (LOG_DBG, LOG_MSGS, "got: close win request");
    logProcEnd (LOG_PROC, "manageCloseWin", "not-done");
    return TRUE;
  }

  logProcEnd (LOG_PROC, "manageCloseWin", "");
  return FALSE;
}

static void
manageSigHandler (int sig)
{
  gKillReceived = 1;
}

/* update database */

static void
manageDbChg (GtkSpinButton *sb, gpointer udata)
{
  manageui_t      *manage = udata;
  GtkAdjustment   *adjustment;
  double          value;
  ssize_t         nval;
  char            *sval;

  nval = MANAGE_DB_CHECK_NEW;
  if (sb != NULL) {
    adjustment = gtk_spin_button_get_adjustment (sb);
    value = gtk_adjustment_get_value (adjustment);
    nval = (ssize_t) value;
  }

  sval = nlistGetStr (manage->dbhelp, nval);
  uiutilsTextBoxSetValue (manage->dbhelpdisp, sval);
}

static void
manageDbStart (GtkButton *b, gpointer udata)
{
  manageui_t  *manage = udata;
  int         nval;
  char        *targv [10];
  int         targc = 0;
  char        tbuff [MAXPATHLEN];

  pathbldMakePath (tbuff, sizeof (tbuff),
      "bdj4dbupdate", sysvarsGetStr (SV_OS_EXEC_EXT), PATHBLD_MP_EXECDIR);

  nval = uiutilsSpinboxTextGetValue (&manage->dbspinbox);
  switch (nval) {
    case MANAGE_DB_CHECK_NEW: {
      targv [targc++] = "--checknew";
      break;
    }
    case MANAGE_DB_REORGANIZE: {
      targv [targc++] = "--reorganize";
      break;
    }
    case MANAGE_DB_UPD_FROM_TAGS: {
      targv [targc++] = "--updfromtags";
      break;
    }
    case MANAGE_DB_WRITE_TAGS: {
      targv [targc++] = "--writetags";
      break;
    }
    case MANAGE_DB_REBUILD: {
      targv [targc++] = "--rebuild";
      break;
    }
  }

  targv [targc++] = "--progress";
  targv [targc++] = NULL;

  uiutilsProgressBarSet (manage->dbpbar, 0.0);
  manage->processes [ROUTE_DBUPDATE] = procutilStartProcess (
      ROUTE_DBUPDATE, "bdj4dbupdate", OS_PROC_DETACH, targv);
}

static void
manageDbProgressMsg (manageui_t *manage, char *args)
{
  double    progval;

  if (strncmp ("END", args, 3) == 0) {
    uiutilsProgressBarSet (manage->dbpbar, 100.0);
  } else {
    if (sscanf (args, "PROG %lf", &progval) == 1) {
      uiutilsProgressBarSet (manage->dbpbar, progval);
    }
  }
}

static void
manageDbStatusMsg (manageui_t *manage, char *args)
{
  uiutilsTextBoxAppendStr (manage->dbstatus, args);
  uiutilsTextBoxAppendStr (manage->dbstatus, "\n");
  uiutilsTextBoxScrollToEnd (manage->dbstatus);
}

static void
manageSwitchPage (GtkNotebook *nb, GtkWidget *page, guint pagenum,
    gpointer udata)
{
  manageui_t  *manage = udata;
  int         id;
  bool        mainnb = false;
  bool        slnb = false;
  bool        mmnb = false;
  uiutilsnbtabid_t  *nbtabid = NULL;

  /* need to know which notebook is selected so that the correct id value */
  /* can be retrieved */
  if (nb == GTK_NOTEBOOK (manage->mainnotebook)) {
    nbtabid = manage->mainnbtabid;
    mainnb = true;
  }
  if (nb == GTK_NOTEBOOK (manage->slnotebook)) {
    nbtabid = manage->slnbtabid;
    slnb = true;
  }
  if (nb == GTK_NOTEBOOK (manage->mmnotebook)) {
    nbtabid = manage->mmnbtabid;
    mmnb = true;
  }
  if (nbtabid == NULL) {
    return;
  }

  id = uiutilsNotebookIDGet (nbtabid, pagenum);

  if (manage->currmenu != NULL) {
    bool clear = true;

    /* handle startup issue */
    if (manage->mainlasttab == id && id == MANAGE_TAB_MAIN_SL) {
      clear = false;
    }
    if (manage->mainlasttab == MANAGE_TAB_MAIN_SL && mmnb) {
      clear = false;
    }
    if (clear) {
      uiutilsMenuClear (manage->currmenu);
    }
  }

  if (mainnb) {
    manage->mainlasttab = id;
  }
  if (slnb) {
    manage->sllasttab = id;
  }
  if (mmnb) {
    manage->mmlasttab = id;
  }

  if (mainnb && id == MANAGE_TAB_MAIN_SL) {
    /* force menu selection */
    slnb = true;
    id = manage->sllasttab;
fprintf (stderr, "force sl tab %d\n", id);
  }
  if (mainnb && id == MANAGE_TAB_MAIN_MM) {
    /* force menu selection */
    mmnb = true;
    id = manage->mmlasttab;
fprintf (stderr, "force mm tab %d\n", id);
  }

  switch (id) {
    case MANAGE_TAB_MAIN_SL: {
      break;
    }
    case MANAGE_TAB_MAIN_MM: {
      break;
    }
    case MANAGE_TAB_SONGLIST: {
fprintf (stderr, "set sl menu\n");
      manageSonglistMenu (manage);
      break;
    }
    case MANAGE_TAB_SONGEDIT: {
fprintf (stderr, "set songedit menu\n");
      manageSongEditMenu (manage);
      break;
    }
    case MANAGE_TAB_PLMGMT: {
      break;
    }
    case MANAGE_TAB_EDITSEQ: {
      break;
    }
    case MANAGE_TAB_FILEMGR: {
      break;
    }
    case MANAGE_TAB_OTHER: {
      /* do nothing */
      break;
    }
  }
}

static void
manageSonglistMenu (manageui_t *manage)
{
  char      tbuff [200];
  GtkWidget *menu;
  GtkWidget *menuitem;

  if (! manage->slmenu.initialized) {
    menuitem = uiutilsMenuAddMainItem (manage->menubar,
        /* CONTEXT: menu selection: song list: edit menu */
        &manage->slmenu, _("Edit"));

    menu = uiutilsCreateSubMenu (menuitem);

    /* CONTEXT: menu selection: song list: edit menu: load */
    menuitem = uiutilsCreateMenuItem (menu, _("Load"));

    /* CONTEXT: menu selection: song list: edit menu: create copy */
    menuitem = uiutilsCreateMenuItem (menu, _("Create Copy"));

    /* CONTEXT: menu selection: song list: edit menu: start new song list */
    menuitem = uiutilsCreateMenuItem (menu, _("Start New Song List"));

    menuitem = uiutilsMenuAddMainItem (manage->menubar,
        /* CONTEXT: menu selection: actions for song list */
        &manage->slmenu,_("Actions"));

    menu = uiutilsCreateSubMenu (menuitem);

    /* CONTEXT: menu selection: song list: actions menu: rearrange the songs and create a new mix */
    menuitem = uiutilsCreateMenuItem (menu, _("Mix"));

    /* CONTEXT: menu selection: song list: actions menu: truncate the song list */
    menuitem = uiutilsCreateMenuItem (menu, _("Truncate"));

    menuitem = uiutilsMenuAddMainItem (manage->menubar,
        /* CONTEXT: menu selection: export actions for song list */
        &manage->slmenu, _("Export"));

    menu = uiutilsCreateSubMenu (menuitem);

    /* CONTEXT: menu selection: song list: export: export as m3u */
    menuitem = uiutilsCreateMenuItem (menu, _("Export as M3U Playlist"));

    /* CONTEXT: menu selection: song list: export: export as m3u8 */
    menuitem = uiutilsCreateMenuItem (menu, _("Export as M3U8 Playlist"));

    /* CONTEXT: menu selection: song list: export: export for ballroomdj */
    snprintf (tbuff, sizeof (tbuff), _("Export for %s"), BDJ4_NAME);
    menuitem = uiutilsCreateMenuItem (menu, tbuff);

    menuitem = uiutilsMenuAddMainItem (manage->menubar,
        /* CONTEXT: menu selection: import actions for song list */
        &manage->slmenu, _("Import"));

    menu = uiutilsCreateSubMenu (menuitem);

    /* CONTEXT: menu selection: song list: import: import m3u */
    menuitem = uiutilsCreateMenuItem (menu, _("Import M3U"));

    /* CONTEXT: menu selection: song list: import: import from ballroomdj */
    snprintf (tbuff, sizeof (tbuff), _("Import from %s"), BDJ4_NAME);
    menuitem = uiutilsCreateMenuItem (menu, tbuff);

    manage->slmenu.initialized = true;
  }

  uiutilsMenuDisplay (&manage->slmenu);
  manage->currmenu = &manage->slmenu;
}

static void
manageSongEditMenu (manageui_t *manage)
{
  GtkWidget *menu;
  GtkWidget *menuitem;

  if (! manage->songeditmenu.initialized) {
    menuitem = uiutilsMenuAddMainItem (manage->menubar,
        /* CONTEXT: menu selection: actions for song editor */
        &manage->songeditmenu,_("Actions"));

    menu = uiutilsCreateSubMenu (menuitem);

    /* CONTEXT: menu selection: song edit: edit all */
    menuitem = uiutilsCreateMenuItem (menu, _("Edit All"));

    /* CONTEXT: menu selection: song edit: apply edit all */
    menuitem = uiutilsCreateMenuItem (menu, _("Apply Edit All"));

    /* CONTEXT: menu selection: song edit: cancel edit all */
    menuitem = uiutilsCreateMenuItem (menu, _("Cancel Edit All"));

    manage->songeditmenu.initialized = true;
  }

  uiutilsMenuDisplay (&manage->songeditmenu);
  manage->currmenu = &manage->songeditmenu;
}

