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
#include "bdjvarsdf.h"
#include "conn.h"
#include "datafile.h"
#include "dispsel.h"
#include "fileop.h"
#include "filemanip.h"
#include "localeutil.h"
#include "lock.h"
#include "log.h"
#include "musicq.h"
#include "osutils.h"
#include "osuiutils.h"
#include "pathbld.h"
#include "pathutil.h"
#include "playlist.h"
#include "procutil.h"
#include "progstate.h"
#include "sequence.h"
#include "slist.h"
#include "sock.h"
#include "sockh.h"
#include "songfilter.h"
#include "sysvars.h"
#include "tmutil.h"
#include "ui.h"
#include "uiduallist.h"
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
  MNG_SELFILE_COL_DISP,
  MNG_SELFILE_COL_SB_PAD,
  MNG_SELFILE_COL_MAX,
};

enum {
  MANAGE_DB_CHECK_NEW,
  MANAGE_DB_REORGANIZE,
  MANAGE_DB_UPD_FROM_TAGS,
  MANAGE_DB_WRITE_TAGS,
  MANAGE_DB_REBUILD,
};

enum {
  MANAGE_F_ALL,
  MANAGE_F_SONGLIST,
  MANAGE_F_PLAYLIST,
  MANAGE_F_SEQUENCE,
};

enum {
  MANAGE_CALLBACK_EZ_SELECT,
  MANAGE_CALLBACK_DB_START,
  MANAGE_CALLBACK_MAX,
};

typedef struct manage manageui_t;

typedef void (*manageselfilecb_t)(manageui_t *manage, const char *fname);

typedef struct manage {
  progstate_t     *progstate;
  procutil_t      *processes [ROUTE_MAX];
  char            *locknm;
  conn_t          *conn;
  UICallback      callbacks [MANAGE_CALLBACK_MAX];
  musicdb_t       *musicdb;
  musicqidx_t     musicqPlayIdx;
  musicqidx_t     musicqManageIdx;
  dispsel_t       *dispsel;
  int             stopwaitcount;
  UIWidget        statusMsg;
  /* update database */
  uispinbox_t       dbspinbox;
  uitextbox_t       *dbhelpdisp;
  uitextbox_t       *dbstatus;
  nlist_t           *dblist;
  nlist_t           *dbhelp;
  UIWidget          dbpbar;
  /* notebook tab handling */
  int               mainlasttab;
  int               sllasttab;
  int               mmlasttab;
  uimenu_t          *currmenu;
  uimenu_t          slmenu;
  uimenu_t          songeditmenu;
  uimenu_t          seqmenu;
  uiutilsnbtabid_t  *mainnbtabid;
  uiutilsnbtabid_t  *slnbtabid;
  uiutilsnbtabid_t  *mmnbtabid;
  /* file selection dialog */
  GtkWidget         *selfiledialog;
  GtkWidget         *selfiletree;
  manageselfilecb_t selfilecb;
  /* gtk stuff */
  GtkWidget       *menubar;
  GtkWidget       *window;
  GtkWidget       *mainnotebook;
  GtkWidget       *slnotebook;
  GtkWidget       *mmnotebook;
  GtkWidget       *vbox;
  /* song list ui major elements */
  uiplayer_t      *slplayer;
  uimusicq_t      *slmusicq;
  uisongsel_t     *slsongsel;
  uisongedit_t    *slsongedit;
  uimusicq_t      *slezmusicq;
  uisongsel_t     *slezsongsel;
  GtkWidget       *slezmusicqtabwidget;
  GtkWidget       *slmusicqtabwidget;
  GtkWidget       *slsongseltabwidget;
  GtkWidget       *ezvboxwidget;
  char            *sloldname;
  songfilter_t    *slsongfilter;
  songfilter_t    *mmsongfilter;
  /* music manager ui */
  uiplayer_t      *mmplayer;
  uimusicq_t      *mmmusicq;
  uisongsel_t     *mmsongsel;
  uisongedit_t    *mmsongedit;
  /* sequence editor */
  uiduallist_t    *seqduallist;
  uientry_t       seqname;
  char            *seqoldname;
  /* options */
  datafile_t      *optiondf;
  nlist_t         *options;
  /* various flags */
  bool            slbackupcreated : 1;
  bool            seqbackupcreated : 1;
} manageui_t;

/* re-use the plui enums so that the songsel filter enums can also be used */
static datafilekey_t manageuidfkeys [] = {
  { "EASY_SONGLIST",    MANAGE_EASY_SONGLIST,       VALUE_NUM, convBoolean, -1 },
  { "FILTER_POS_X",     SONGSEL_FILTER_POSITION_X,  VALUE_NUM, NULL, -1 },
  { "FILTER_POS_Y",     SONGSEL_FILTER_POSITION_Y,  VALUE_NUM, NULL, -1 },
  { "MNG_SELFILE_POS_X",MANAGE_SELFILE_POSITION_X,  VALUE_NUM, NULL, -1 },
  { "MNG_SELFILE_POS_Y",MANAGE_SELFILE_POSITION_Y,  VALUE_NUM, NULL, -1 },
  { "PLUI_POS_X",       PLUI_POSITION_X,            VALUE_NUM, NULL, -1 },
  { "PLUI_POS_Y",       PLUI_POSITION_Y,            VALUE_NUM, NULL, -1 },
  { "PLUI_SIZE_X",      PLUI_SIZE_X,                VALUE_NUM, NULL, -1 },
  { "PLUI_SIZE_Y",      PLUI_SIZE_Y,                VALUE_NUM, NULL, -1 },
  { "SORT_BY",          SONGSEL_SORT_BY,            VALUE_STR, NULL, -1 },
};
#define MANAGEUI_DFKEY_COUNT (sizeof (manageuidfkeys) / sizeof (datafilekey_t))

static bool     manageConnectingCallback (void *udata, programstate_t programState);
static bool     manageHandshakeCallback (void *udata, programstate_t programState);
static bool     manageStoppingCallback (void *udata, programstate_t programState);
static bool     manageStopWaitCallback (void *udata, programstate_t programState);
static bool     manageClosingCallback (void *udata, programstate_t programState);
static void     manageBuildUI (manageui_t *manage);
static void     manageBuildUISongListEditor (manageui_t *manage);
static void     manageBuildUIMusicManager (manageui_t *manage);
static void     manageBuildUIUpdateDatabase (manageui_t *manage);
static void     manageBuildUISequence (manageui_t *manage);
static int      manageMainLoop  (void *tmanage);
static int      manageProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static gboolean manageCloseWin (GtkWidget *window, GdkEvent *event, gpointer userdata);
static void     manageSigHandler (int sig);
/* update database */
static void     manageDbChg (GtkSpinButton *sb, gpointer udata);
static bool     manageDbStart (void *udata);
static void     manageDbProgressMsg (manageui_t *manage, char *args);
static void     manageDbStatusMsg (manageui_t *manage, char *args);
/* song editor */
static void     manageSongEditMenu (manageui_t *manage);
/* song list */
static void     manageSonglistMenu (manageui_t *manage);
static void     manageSonglistLoad (GtkMenuItem *mi, gpointer udata);
static void     manageSonglistCopy (GtkMenuItem *mi, gpointer udata);
static void     manageSonglistNew (GtkMenuItem *mi, gpointer udata);
static void     manageSonglistTruncate (GtkMenuItem *mi, gpointer udata);
static void     manageSonglistLoadFile (manageui_t *manage, const char *fn);
static void     manageToggleEasySonglist (GtkWidget *mi, gpointer udata);
static void     manageSetEasySonglist (manageui_t *manage);
static void     manageSonglistSave (manageui_t *manage);
static void     manageSetSonglistName (manageui_t *manage, const char *nm);
/* general */
static void     manageSwitchPage (GtkNotebook *nb, GtkWidget *page,
    guint pagenum, gpointer udata);
static void     manageSelectFileDialog (manageui_t *manage, int flags);
static GtkWidget * manageCreateSelectFileDialog (manageui_t *manage,
    slist_t *filelist, const char *filetype, manageselfilecb_t cb);
static void manageSelectFileSelect (GtkTreeView* tv, GtkTreePath* path,
    GtkTreeViewColumn* column, gpointer udata);
static void manageSelectFileResponseHandler (GtkDialog *d, gint responseid,
    gpointer udata);
static void manageInitializeSongFilter (manageui_t *manage, nlist_t *options);
static void manageRenamePlaylistFiles (const char *oldname, const char *newname);
static void manageCopyPlaylistFiles (const char *oldname, const char *newname);
static void manageSetStatusMsg (manageui_t *manage, const char *msg);
static void manageCheckAndCreatePlaylist (manageui_t *manage, const char *name,
    const char *suppfname, pltype_t pltype);
static bool manageCreatePlaylistCopy (manageui_t *manage, const char *oname, const char *newname);
/* sequence */
static void     manageSequenceMenu (manageui_t *manage);
static void     manageSequenceLoad (GtkMenuItem *mi, gpointer udata);
static void     manageSequenceLoadFile (manageui_t *manage, const char *fn);
static void     manageSequenceCopy (GtkMenuItem *mi, gpointer udata);
static void     manageSequenceNew (GtkMenuItem *mi, gpointer udata);
static void     manageSequenceSave (manageui_t *manage);
static void     manageSetSequenceName (manageui_t *manage, const char *nm);


static int gKillReceived = 0;

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
  manage.slezmusicq = NULL;
  manage.slezsongsel = NULL;
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
  uiMenuInit (&manage.slmenu);
  uiMenuInit (&manage.songeditmenu);
  uiMenuInit (&manage.seqmenu);
  manage.mainnbtabid = uiutilsNotebookIDInit ();
  manage.slnbtabid = uiutilsNotebookIDInit ();
  manage.mmnbtabid = uiutilsNotebookIDInit ();
  manage.selfilecb = NULL;
  manage.sloldname = NULL;
  manage.slbackupcreated = false;
  manage.seqbackupcreated = false;
  manage.slsongfilter = NULL;
  manage.mmsongfilter = NULL;
  uiutilsUIWidgetInit (&manage.dbpbar);
  manage.seqduallist = NULL;
  uiEntryInit (&manage.seqname, 20, 50);
  manage.seqoldname = NULL;

  procutilInitProcesses (manage.processes);

  osSetStandardSignals (manageSigHandler);

  bdj4startup (argc, argv, &manage.musicdb,
      "mu", ROUTE_MANAGEUI, BDJ4_INIT_NONE);
  logProcBegin (LOG_PROC, "manageui");

  manage.dispsel = dispselAlloc ();
  uiSpinboxTextInit (&manage.dbspinbox);
  tlist = nlistAlloc ("db-action", LIST_ORDERED, free);
  hlist = nlistAlloc ("db-action-help", LIST_ORDERED, free);
  /* CONTEXT: database update: check for new audio files */
  nlistSetStr (tlist, MANAGE_DB_CHECK_NEW, _("Check For New"));
  nlistSetStr (hlist, MANAGE_DB_CHECK_NEW,
      /* CONTEXT: database update: check for new: help text */
      _("Checks for new audio files."));
  /* CONTEXT: database update: reorganize : renames audio files based on organization settings */
  nlistSetStr (tlist, MANAGE_DB_REORGANIZE, _("Reorganize"));
  nlistSetStr (hlist, MANAGE_DB_REORGANIZE,
      /* CONTEXT: database update: reorganize : help text */
      _("Renames the audio files based on the organization settings."));
  /* CONTEXT: database update: updates the database using the tags from the audio files */
  nlistSetStr (tlist, MANAGE_DB_UPD_FROM_TAGS, _("Update from Audio File Tags"));
  nlistSetStr (hlist, MANAGE_DB_UPD_FROM_TAGS,
      /* CONTEXT: database update: update from audio file tags: help text */
      _("Replaces the information in the BallroomDJ database with the audio file tag information."));
  /* CONTEXT: database update: writes the tags in the database to the audio files */
  nlistSetStr (tlist, MANAGE_DB_WRITE_TAGS, _("Write Tags to Audio Files"));
  nlistSetStr (hlist, MANAGE_DB_WRITE_TAGS,
      /* CONTEXT: database update: write tags to audio files: help text */
      _("Updates the audio file tags with the information from the BallroomDJ database."));
  /* CONTEXT: database update: rebuilds the database */
  nlistSetStr (tlist, MANAGE_DB_REBUILD, _("Rebuild Database"));
  nlistSetStr (hlist, MANAGE_DB_REBUILD,
      /* CONTEXT: database update: rebuild: help text */
      _("Replaces the BallroomDJ database in its entirety. All changes to the database will be lost."));
  manage.dblist = tlist;
  manage.dbhelp = hlist;

  listenPort = bdjvarsGetNum (BDJVL_MANAGEUI_PORT);
  manage.conn = connInit (ROUTE_MANAGEUI);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "manageui", BDJ4_CONFIG_EXT, PATHBLD_MP_USEIDX);
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
    nlistSetNum (manage.options, MANAGE_SELFILE_POSITION_X, -1);
    nlistSetNum (manage.options, MANAGE_SELFILE_POSITION_Y, -1);
    nlistSetNum (manage.options, MANAGE_EASY_SONGLIST, true);
  }

  manage.slsongfilter = songfilterAlloc ();
  manage.mmsongfilter = songfilterAlloc ();
  manageInitializeSongFilter (&manage, manage.options);

  manage.slplayer = uiplayerInit (manage.progstate, manage.conn,
      manage.musicdb);
  manage.slmusicq = uimusicqInit (manage.conn,
      manage.musicdb, manage.dispsel,
      UIMUSICQ_FLAGS_NO_QUEUE | UIMUSICQ_FLAGS_NO_TOGGLE_PAUSE,
      DISP_SEL_SONGLIST);
  manage.slsongsel = uisongselInit ("m-ss", manage.conn,
      manage.musicdb, manage.dispsel, manage.options,
      SONG_FILTER_FOR_SELECTION, DISP_SEL_SONGSEL);
  uisongselInitializeSongFilter (manage.slsongsel, manage.slsongfilter);
  manage.slsongedit = uisongeditInit (manage.conn,
      manage.musicdb, manage.dispsel, manage.options);
  manage.slezmusicq = uimusicqInit (manage.conn,
      manage.musicdb, manage.dispsel,
      UIMUSICQ_FLAGS_NO_QUEUE | UIMUSICQ_FLAGS_NO_TOGGLE_PAUSE,
      DISP_SEL_EZSONGLIST);
  manage.slezsongsel = uisongselInit ("m-ez", manage.conn,
      manage.musicdb, manage.dispsel, manage.options,
      SONG_FILTER_FOR_SELECTION, DISP_SEL_EZSONGSEL);
  uisongselInitializeSongFilter (manage.slezsongsel, manage.slsongfilter);

  uimusicqSetPlayIdx (manage.slmusicq, manage.musicqPlayIdx);
  uimusicqSetPlayIdx (manage.slezmusicq, manage.musicqPlayIdx);
  uimusicqSetManageIdx (manage.slmusicq, manage.musicqManageIdx);
  uimusicqSetManageIdx (manage.slezmusicq, manage.musicqManageIdx);

  manage.mmplayer = uiplayerInit (manage.progstate, manage.conn,
      manage.musicdb);
  manage.mmmusicq = uimusicqInit (manage.conn,
      manage.musicdb, manage.dispsel,
      UIMUSICQ_FLAGS_NO_QUEUE | UIMUSICQ_FLAGS_NO_TOGGLE_PAUSE,
      DISP_SEL_SONGLIST);
  manage.mmsongsel = uisongselInit ("m-mm", manage.conn,
      manage.musicdb, manage.dispsel, manage.options,
      SONG_FILTER_FOR_SELECTION, DISP_SEL_MM);
  uisongselInitializeSongFilter (manage.mmsongsel, manage.mmsongfilter);
  manage.mmsongedit = uisongeditInit (manage.conn,
      manage.musicdb, manage.dispsel, manage.options);

  uimusicqSetPlayIdx (manage.mmmusicq, manage.musicqPlayIdx);
  uimusicqSetManageIdx (manage.mmmusicq, manage.musicqManageIdx);

  /* register these after calling the sub-window initialization */
  /* then these will be run last, after the other closing callbacks */
  progstateSetCallback (manage.progstate, STATE_STOPPING,
      manageStoppingCallback, &manage);
  progstateSetCallback (manage.progstate, STATE_STOP_WAIT,
      manageStopWaitCallback, &manage);
  progstateSetCallback (manage.progstate, STATE_CLOSING,
      manageClosingCallback, &manage);

  uiUIInitialize ();
  uifont = bdjoptGetStr (OPT_MP_UIFONT);
  uiSetUIFont (uifont);

  manageBuildUI (&manage);
  sockhMainLoop (listenPort, manageProcessMsg, manageMainLoop, &manage);
  connFree (manage.conn);
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

  manageSonglistSave (manage);
  manageSequenceSave (manage);

  uiWindowGetSizeW (manage->window, &x, &y);
  nlistSetNum (manage->options, PLUI_SIZE_X, x);
  nlistSetNum (manage->options, PLUI_SIZE_Y, y);
  uiWindowGetPositionW (manage->window, &x, &y);
  nlistSetNum (manage->options, PLUI_POSITION_X, x);
  nlistSetNum (manage->options, PLUI_POSITION_Y, y);

  procutilStopAllProcess (manage->processes, manage->conn, false);
  connDisconnect (manage->conn, ROUTE_STARTERUI);
  logProcEnd (LOG_PROC, "manageStoppingCallback", "");
  return STATE_FINISHED;
}

static bool
manageStopWaitCallback (void *udata, programstate_t programState)
{
  manageui_t  * manage = udata;
  bool        rc;

  rc = connWaitClosed (manage->conn, &manage->stopwaitcount);
  return rc;
}

static bool
manageClosingCallback (void *udata, programstate_t programState)
{
  manageui_t    *manage = udata;
  char          fn [MAXPATHLEN];

  logProcBegin (LOG_PROC, "manageClosingCallback");

  uiCloseWindowW (manage->window);

  procutilStopAllProcess (manage->processes, manage->conn, true);
  procutilFreeAll (manage->processes);

  pathbldMakePath (fn, sizeof (fn),
      "manageui", BDJ4_CONFIG_EXT, PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("manageui", fn, manageuidfkeys, MANAGEUI_DFKEY_COUNT, manage->options);

  bdj4shutdown (ROUTE_MANAGEUI, manage->musicdb);
  dispselFree (manage->dispsel);

  uiEntryFree (&manage->seqname);
  if (manage->slsongfilter != NULL) {
    songfilterFree (manage->slsongfilter);
  }
  if (manage->mmsongfilter != NULL) {
    songfilterFree (manage->mmsongfilter);
  }
  if (manage->sloldname != NULL) {
    free (manage->sloldname);
  }
  if (manage->seqoldname != NULL) {
    free (manage->seqoldname);
  }
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

  uiTextBoxFree (manage->dbhelpdisp);
  uiTextBoxFree (manage->dbstatus);
  uiplayerFree (manage->slplayer);
  uimusicqFree (manage->slmusicq);
  uisongselFree (manage->slsongsel);
  uimusicqFree (manage->slezmusicq);
  uisongselFree (manage->slezsongsel);
  uisongeditFree (manage->slsongedit);
  uiplayerFree (manage->mmplayer);
  uimusicqFree (manage->mmmusicq);
  uisongselFree (manage->mmsongsel);
  uisongeditFree (manage->mmsongedit);
  uiCleanup ();
  if (manage->dblist != NULL) {
    nlistFree (manage->dblist);
  }
  if (manage->dbhelp != NULL) {
    nlistFree (manage->dbhelp);
  }

  logProcEnd (LOG_PROC, "manageClosingCallback", "");
  return STATE_FINISHED;
}

static void
manageBuildUI (manageui_t *manage)
{
  GtkWidget           *menubar;
  GtkWidget           *tabLabel;
  GtkWidget           *vbox;
  UIWidget            hbox;
  UIWidget            uiwidget;
  UIWidget            uivbox;
  char                imgbuff [MAXPATHLEN];
  char                tbuff [MAXPATHLEN];
  gint                x, y;

  logProcBegin (LOG_PROC, "manageBuildUI");
  *imgbuff = '\0';
  uiutilsUIWidgetInit (&uivbox);
  uiutilsUIWidgetInit (&hbox);
  uiutilsUIWidgetInit (&uiwidget);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", ".svg", PATHBLD_MP_IMGDIR);
  /* CONTEXT: management user interface window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Management"), BDJ4_NAME);
  manage->window = uiCreateMainWindowW (tbuff, imgbuff,
      manageCloseWin, manage);

  uiCreateVertBox (&uivbox);
  uiWidgetSetAllMargins (&uivbox, 4);
  uiBoxPackInWindowWU (manage->window, &uivbox);

  uiCreateHorizBox (&hbox);
  uiWidgetSetMarginTop (&hbox, uiBaseMarginSz * 4);
  uiBoxPackStart (&uivbox, &hbox);

  uiCreateLabel (&uiwidget, "");
  uiLabelSetColor (&uiwidget, bdjoptGetStr (OPT_P_UI_ACCENT_COL));
  uiBoxPackEnd (&hbox, &uiwidget);
  uiutilsUIWidgetCopy (&manage->statusMsg, &uiwidget);

  menubar = uiCreateMenubar ();
  uiBoxPackStartUW (&hbox, menubar);
  manage->menubar = menubar;

  manage->mainnotebook = uiCreateNotebookW ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (manage->mainnotebook), GTK_POS_LEFT);
  uiBoxPackStartExpandUW (&uivbox, manage->mainnotebook);

  manageBuildUISongListEditor (manage);
  manageBuildUIMusicManager (manage);
  manageBuildUIUpdateDatabase (manage);

  /* playlist management */
  vbox = uiCreateVertBoxWW ();
  uiWidgetSetAllMarginsW (vbox, 4);
  /* CONTEXT: notebook tab title: playlist management */
  tabLabel = uiCreateLabelW (_("Playlist Management"));
  uiNotebookAppendPageW (manage->mainnotebook, vbox, tabLabel);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_PLMGMT);

  manageBuildUISequence (manage);

  /* file manager */
  vbox = uiCreateVertBoxWW ();
  uiWidgetSetAllMarginsW (vbox, 4);
  /* CONTEXT: notebook tab title: file manager */
  tabLabel = uiCreateLabelW (_("File Manager"));
  uiNotebookAppendPageW (manage->mainnotebook, vbox, tabLabel);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_FILEMGR);

  x = nlistGetNum (manage->options, PLUI_SIZE_X);
  y = nlistGetNum (manage->options, PLUI_SIZE_Y);
  uiWindowSetDefaultSizeW (manage->window, x, y);

  g_signal_connect (manage->mainnotebook, "switch-page",
      G_CALLBACK (manageSwitchPage), manage);

  uiWidgetShowAllW (manage->window);

  x = nlistGetNum (manage->options, PLUI_POSITION_X);
  y = nlistGetNum (manage->options, PLUI_POSITION_Y);
  uiWindowMoveW (manage->window, x, y);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", ".png", PATHBLD_MP_IMGDIR);
  osuiSetIcon (imgbuff);

  logProcEnd (LOG_PROC, "manageBuildUI", "");
}

static void
manageBuildUISongListEditor (manageui_t *manage)
{
  UIWidget            uiwidget;
  GtkWidget           *vbox;
  UIWidget            hbox;
  UIWidget            *uiwidgetp;
  GtkWidget           *tabLabel;
  GtkWidget           *widget;
  GtkWidget           *notebook;

  /* song list editor */
  uiutilsUIWidgetInit (&hbox);

  vbox = uiCreateVertBoxWW ();
  uiWidgetSetAllMarginsW (vbox, 4);

  /* CONTEXT: notebook tab title: edit song lists (manual playlists) */
  tabLabel = uiCreateLabelW (_("Edit Song Lists"));
  uiNotebookAppendPageW (manage->mainnotebook, vbox, tabLabel);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_MAIN_SL);

  /* song list editor: player */
  uiwidgetp = uiplayerBuildUI (manage->slplayer);
  uiBoxPackStartWU (vbox, uiwidgetp);

  notebook = uiCreateNotebookW ();
  uiBoxPackStartExpandWW (vbox, notebook);
  manage->slnotebook = notebook;

  /* song list editor: easy song list tab */
  widget = uiCreateHorizBoxWW ();

  /* CONTEXT: name of easy song list song selection tab */
  tabLabel = uiCreateLabelW (_("Song List"));
  uiNotebookAppendPageW (notebook, widget, tabLabel);
  uiutilsNotebookIDAdd (manage->slnbtabid, MANAGE_TAB_SONGLIST);
  manage->slezmusicqtabwidget = widget;

  uiCreateHorizBox (&hbox);
  uiBoxPackStartExpandWU (widget, &hbox);

  uiwidgetp = uimusicqBuildUI (manage->slezmusicq, manage->window, MUSICQ_A);
  uiBoxPackStartExpand (&hbox, uiwidgetp);

  vbox = uiCreateVertBoxWW ();
  uiWidgetSetAllMarginsW (vbox, uiBaseMarginSz * 4);
  uiWidgetSetMarginTopW (vbox, uiBaseMarginSz * 64);
  uiBoxPackStartUW (&hbox, vbox);
  manage->ezvboxwidget = vbox;

  uiutilsUICallbackInit (&manage->callbacks [MANAGE_CALLBACK_EZ_SELECT],
      uisongselQueueProcessSelectCallback, manage->slezsongsel);
  widget = uiCreateButton (&uiwidget,
      &manage->callbacks [MANAGE_CALLBACK_EZ_SELECT],
      /* CONTEXT: config: button: add the selected songs to the song list */
      _("Select"), "button_left", NULL, NULL);
  uiBoxPackStartWW (vbox, widget);

  uiwidgetp = uisongselBuildUI (manage->slezsongsel, manage->window);
  uiBoxPackStartExpand (&hbox, uiwidgetp);

  /* song list editor: music queue tab */
  uiwidgetp = uimusicqBuildUI (manage->slmusicq, manage->window, MUSICQ_A);
  /* CONTEXT: name of easy song list notebook tab */
  tabLabel = uiCreateLabelW (_("Song List"));
  uiNotebookAppendPageW (notebook, uiwidgetp->widget, tabLabel);
  uiutilsNotebookIDAdd (manage->slnbtabid, MANAGE_TAB_SONGLIST);
  manage->slmusicqtabwidget = uiwidgetp->widget;

  /* song list editor: song selection tab*/
  uiwidgetp = uisongselBuildUI (manage->slsongsel, manage->window);
  /* CONTEXT: name of song selection notebook tab */
  tabLabel = uiCreateLabelW (_("Song Selection"));
  uiNotebookAppendPageW (notebook, uiwidgetp->widget, tabLabel);
  uiutilsNotebookIDAdd (manage->slnbtabid, MANAGE_TAB_OTHER);
  manage->slsongseltabwidget = uiwidgetp->widget;

  /* song list editor song editor tab */
  widget = uisongeditBuildUI (manage->slsongedit, manage->window);
  /* CONTEXT: name of song editor notebook tab */
  tabLabel = uiCreateLabelW (_("Song Editor"));
  uiNotebookAppendPageW (notebook, widget, tabLabel);
  uiutilsNotebookIDAdd (manage->slnbtabid, MANAGE_TAB_SONGEDIT);

  uimusicqPeerSonglistName (manage->slmusicq, manage->slezmusicq);
}

static void
manageBuildUIMusicManager (manageui_t *manage)
{
  UIWidget            *uiwidgetp;
  GtkWidget           *vbox;
  GtkWidget           *tabLabel;
  GtkWidget           *widget;
  GtkWidget           *notebook;

  /* music manager */
  vbox = uiCreateVertBoxWW ();
  uiWidgetSetAllMarginsW (vbox, 4);
  /* CONTEXT: name of music manager notebook tab */
  tabLabel = uiCreateLabelW (_("Music Manager"));
  uiNotebookAppendPageW (manage->mainnotebook, vbox, tabLabel);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_MAIN_MM);

  /* music manager: player */
  uiwidgetp = uiplayerBuildUI (manage->mmplayer);
  uiBoxPackStartWU (vbox, uiwidgetp);

  notebook = uiCreateNotebookW ();
  uiBoxPackStartExpandWW (vbox, notebook);
  manage->mmnotebook = notebook;

  /* music manager: song selection tab*/
  uiwidgetp = uisongselBuildUI (manage->mmsongsel, manage->window);
  uiWidgetExpandHoriz (uiwidgetp);
  /* CONTEXT: name of song selection notebook tab */
  tabLabel = uiCreateLabelW (_("Music Manager"));
  uiNotebookAppendPageW (notebook, uiwidgetp->widget, tabLabel);
  uiutilsNotebookIDAdd (manage->mmnbtabid, MANAGE_TAB_OTHER);

  /* music manager: song editor tab */
  widget = uisongeditBuildUI (manage->mmsongedit, manage->window);
  /* CONTEXT: name of song editor notebook tab */
  tabLabel = uiCreateLabelW (_("Song Editor"));
  uiNotebookAppendPageW (notebook, widget, tabLabel);
  uiutilsNotebookIDAdd (manage->mmnbtabid, MANAGE_TAB_SONGEDIT);

  g_signal_connect (notebook, "switch-page",
      G_CALLBACK (manageSwitchPage), manage);
}

static void
manageBuildUIUpdateDatabase (manageui_t *manage)
{
  UIWidget            uiwidget;
  GtkWidget           *vbox;
  UIWidget            hbox;
  GtkWidget           *tabLabel;
  GtkWidget           *widget;
  uitextbox_t    *tb;

  /* update database */
  vbox = uiCreateVertBoxWW ();
  uiWidgetSetAllMarginsW (vbox, 4);
  /* CONTEXT: notebook tab title: update database */
  tabLabel = uiCreateLabelW (_("Update Database"));
  uiNotebookAppendPageW (manage->mainnotebook, vbox, tabLabel);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_OTHER);

  /* help display */
  tb = uiTextBoxCreate (60);
  uiTextBoxSetReadonly (tb);
  uiTextBoxSetHeight (tb, 70);
  uiBoxPackStartWU (vbox, uiTextBoxGetScrolledWindow (tb));
  manage->dbhelpdisp = tb;

  uiCreateHorizBox (&hbox);
  uiBoxPackStartWU (vbox, &hbox);

  widget = uiSpinboxTextCreate (&manage->dbspinbox, manage);
  /* currently hard-coded at 30 chars */
  uiSpinboxTextSet (&manage->dbspinbox, 0,
      nlistGetCount (manage->dblist), 30,
      manage->dblist, NULL, NULL);
  uiSpinboxTextSetValue (&manage->dbspinbox, MANAGE_DB_CHECK_NEW);
  g_signal_connect (widget, "value-changed", G_CALLBACK (manageDbChg), manage);
  uiBoxPackStartUW (&hbox, widget);

  uiutilsUICallbackInit (&manage->callbacks [MANAGE_CALLBACK_DB_START],
      manageDbStart, manage);
  widget = uiCreateButton (&uiwidget,
      &manage->callbacks [MANAGE_CALLBACK_DB_START],
      /* CONTEXT: update database: button to start the database update process */
      _("Start"), NULL, NULL, NULL);
  uiBoxPackStartUW (&hbox, widget);

  uiCreateProgressBar (&manage->dbpbar, bdjoptGetStr (OPT_P_UI_ACCENT_COL));
  uiBoxPackStartWU (vbox, &manage->dbpbar);

  tb = uiTextBoxCreate (200);
  uiTextBoxSetReadonly (tb);
  uiTextBoxDarken (tb);
  uiTextBoxSetHeight (tb, 300);
  uiBoxPackStartExpandWU (vbox, uiTextBoxGetScrolledWindow (tb));
  manage->dbstatus = tb;
}

static void
manageBuildUISequence (manageui_t *manage)
{
  GtkWidget           *tabLabel;
  GtkWidget           *widget;
  UIWidget            vbox;
  UIWidget            hbox;
  UIWidget            uiwidget;
  dance_t             *dances;
  slist_t             *dancelist;

  uiutilsUIWidgetInit (&hbox);
  uiutilsUIWidgetInit (&vbox);
  uiutilsUIWidgetInit (&uiwidget);

  /* edit sequences */
  uiCreateVertBox (&vbox);
  uiWidgetSetAllMarginsW (vbox.widget, 4);
  /* CONTEXT: notebook tab title: edit sequences */
  tabLabel = uiCreateLabelW (_("Edit Sequences"));
  uiNotebookAppendPageW (manage->mainnotebook, vbox.widget, tabLabel);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_EDITSEQ);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: sequence editor: label for sequence name */
  uiCreateColonLabel (&uiwidget, _("Sequence"));
  uiBoxPackStart (&hbox, &uiwidget);

  widget = uiEntryCreate (&manage->seqname);
  uiEntrySetColor (&manage->seqname, bdjoptGetStr (OPT_P_UI_ACCENT_COL));
  /* CONTEXT: sequence: default name for a new sequence */
  manageSetSequenceName (manage, _("New Sequence"));
  uiBoxPackStartUW (&hbox, widget);

  manage->seqduallist = uiCreateDualList (&vbox,
      DUALLIST_FLAGS_MULTIPLE | DUALLIST_FLAGS_PERSISTENT,
      /* CONTEXT: sequence editor: titles for the selection list and the sequence list  */
      _("Dance"), _("Sequence"));

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dancelist = danceGetDanceList (dances);
  uiduallistSet (manage->seqduallist, dancelist, DUALLIST_TREE_SOURCE);
}

static int
manageMainLoop (void *tmanage)
{
  manageui_t   *manage = tmanage;
  int         stop = 0;

  if (! stop) {
    uiUIProcessEvents ();
  }

  if (! progstateIsRunning (manage->progstate)) {
    progstateProcess (manage->progstate);
    if (progstateCurrState (manage->progstate) == STATE_CLOSED) {
      stop = true;
    }
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      progstateShutdownProcess (manage->progstate);
      gKillReceived = 0;
    }
    return stop;
  }

  connProcessUnconnected (manage->conn);

  uiplayerMainLoop (manage->slplayer);
  uimusicqMainLoop (manage->slmusicq);
  uisongselMainLoop (manage->slsongsel);
  uimusicqMainLoop (manage->slezmusicq);
  uisongselMainLoop (manage->slezsongsel);
  uiplayerMainLoop (manage->mmplayer);
  uimusicqMainLoop (manage->mmmusicq);
  uisongselMainLoop (manage->mmsongsel);

  if (gKillReceived) {
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
  bool        rc = STATE_NOT_FINISH;

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
    rc = STATE_FINISHED;
  }

  logProcEnd (LOG_PROC, "manageConnectingCallback", "");
  return rc;
}

static bool
manageHandshakeCallback (void *udata, programstate_t programState)
{
  manageui_t   *manage = udata;
  bool        rc = STATE_NOT_FINISH;

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
    connSendMessage (manage->conn, ROUTE_MAIN, MSG_QUEUE_SWITCH_EMPTY, "0");
    progstateLogTime (manage->progstate, "time-to-start-gui");
    manageDbChg (NULL, manage);
    manageSetEasySonglist (manage);
    /* CONTEXT: song list: default name for a new song list */
    manageSetSonglistName (manage, _("New Song List"));
    rc = STATE_FINISHED;
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

  if (msg != MSG_PLAYER_STATUS_DATA) {
    logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
        routefrom, msgRouteDebugText (routefrom),
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
  }

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
          progstateShutdownProcess (manage->progstate);
          break;
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

          manage->musicdb = bdj4ReloadDatabase (manage->musicdb);

          uiplayerSetDatabase (manage->slplayer, manage->musicdb);
          uiplayerSetDatabase (manage->mmplayer, manage->musicdb);
          uisongselSetDatabase (manage->slsongsel, manage->musicdb);
          uisongselSetDatabase (manage->slezsongsel, manage->musicdb);
          uisongselSetDatabase (manage->mmsongsel, manage->musicdb);
          uimusicqSetDatabase (manage->slmusicq, manage->musicdb);
          uimusicqSetDatabase (manage->slezmusicq, manage->musicdb);
          uimusicqSetDatabase (manage->mmmusicq, manage->musicdb);

          /* the database has been updated, tell the other processes to */
          /* reload it, and reload it ourselves */
          connSendMessage (manage->conn, ROUTE_STARTERUI, MSG_DATABASE_UPDATE, NULL);

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
  uimusicqProcessMsg (routefrom, route, msg, args, manage->slmusicq);
  uisongselProcessMsg (routefrom, route, msg, args, manage->slsongsel);
  uisongselProcessMsg (routefrom, route, msg, args, manage->slezsongsel);
  uimusicqProcessMsg (routefrom, route, msg, args, manage->slezmusicq);
  uiplayerProcessMsg (routefrom, route, msg, targs, manage->mmplayer);
  uisongselProcessMsg (routefrom, route, msg, targs, manage->mmsongsel);
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
  if (progstateCurrState (manage->progstate) <= STATE_RUNNING) {
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
  uiTextBoxSetValue (manage->dbhelpdisp, sval);
}

static bool
manageDbStart (void *udata)
{
  manageui_t  *manage = udata;
  int         nval;
  char        *sval = NULL;
  char        *targv [10];
  int         targc = 0;
  char        tbuff [MAXPATHLEN];

  pathbldMakePath (tbuff, sizeof (tbuff),
      "bdj4dbupdate", sysvarsGetStr (SV_OS_EXEC_EXT), PATHBLD_MP_EXECDIR);

  nval = uiSpinboxTextGetValue (&manage->dbspinbox);

  sval = nlistGetStr (manage->dblist, nval);
  uiTextBoxAppendStr (manage->dbstatus, "-- ");
  uiTextBoxAppendStr (manage->dbstatus, sval);
  uiTextBoxAppendStr (manage->dbstatus, "\n");

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

  uiProgressBarSet (&manage->dbpbar, 0.0);
  manage->processes [ROUTE_DBUPDATE] = procutilStartProcess (
      ROUTE_DBUPDATE, "bdj4dbupdate", OS_PROC_DETACH, targv);
  return UICB_CONT;
}

static void
manageDbProgressMsg (manageui_t *manage, char *args)
{
  double    progval;

  if (strncmp ("END", args, 3) == 0) {
    uiProgressBarSet (&manage->dbpbar, 100.0);
  } else {
    if (sscanf (args, "PROG %lf", &progval) == 1) {
      uiProgressBarSet (&manage->dbpbar, progval);
    }
  }
}

static void
manageDbStatusMsg (manageui_t *manage, char *args)
{
  uiTextBoxAppendStr (manage->dbstatus, args);
  uiTextBoxAppendStr (manage->dbstatus, "\n");
  uiTextBoxScrollToEnd (manage->dbstatus);
}

/* song editor */

static void
manageSongEditMenu (manageui_t *manage)
{
  GtkWidget *menu;
  GtkWidget *menuitem;

  if (! manage->songeditmenu.initialized) {
    menuitem = uiMenuAddMainItem (manage->menubar,
        /* CONTEXT: menu selection: actions for song editor */
        &manage->songeditmenu,_("Actions"));

    menu = uiCreateSubMenu (menuitem);

    /* CONTEXT: menu selection: song editor: edit all */
    menuitem = uiMenuCreateItem (menu, _("Edit All"), NULL, NULL);
    uiWidgetDisableW (menuitem);

    /* CONTEXT: menu selection: song editor: apply edit all */
    menuitem = uiMenuCreateItem (menu, _("Apply Edit All"), NULL, NULL);
    uiWidgetDisableW (menuitem);

    /* CONTEXT: menu selection: song editor: cancel edit all */
    menuitem = uiMenuCreateItem (menu, _("Cancel Edit All"), NULL, NULL);
    uiWidgetDisableW (menuitem);

    manage->songeditmenu.initialized = true;
  }

  uiMenuDisplay (&manage->songeditmenu);
  manage->currmenu = &manage->songeditmenu;
}

/* song list */

static void
manageSonglistMenu (manageui_t *manage)
{
  char      tbuff [200];
  GtkWidget *menu;
  GtkWidget *menuitem;

  if (! manage->slmenu.initialized) {
    menuitem = uiMenuAddMainItem (manage->menubar,
        /* CONTEXT: menu selection: song list: options menu */
        &manage->slmenu, _("Options"));

    menu = uiCreateSubMenu (menuitem);

    /* CONTEXT: menu checkbox: easy song list editor */
    menuitem = uiMenuCreateCheckbox (menu, _("Easy Song List Editor"),
        nlistGetNum (manage->options, MANAGE_EASY_SONGLIST),
        manageToggleEasySonglist, manage);

    menuitem = uiMenuAddMainItem (manage->menubar,
        /* CONTEXT: menu selection: song list: edit menu */
        &manage->slmenu, _("Edit"));

    menu = uiCreateSubMenu (menuitem);

    /* CONTEXT: menu selection: song list: edit menu: load */
    menuitem = uiMenuCreateItem (menu, _("Load"),
        manageSonglistLoad, manage);

    /* CONTEXT: menu selection: song list: edit menu: create copy */
    menuitem = uiMenuCreateItem (menu, _("Create Copy"),
        manageSonglistCopy, manage);

    /* CONTEXT: menu selection: song list: edit menu: start new song list */
    menuitem = uiMenuCreateItem (menu, _("Start New Song List"),
        manageSonglistNew, manage);

    menuitem = uiMenuAddMainItem (manage->menubar,
        /* CONTEXT: menu selection: actions for song list */
        &manage->slmenu,_("Actions"));

    menu = uiCreateSubMenu (menuitem);

    /* CONTEXT: menu selection: song list: actions menu: rearrange the songs and create a new mix */
    menuitem = uiMenuCreateItem (menu, _("Mix"), NULL, NULL);
    uiWidgetDisableW (menuitem);

    /* CONTEXT: menu selection: song list: actions menu: truncate the song list */
    menuitem = uiMenuCreateItem (menu, _("Truncate"),
        manageSonglistTruncate, manage);

    menuitem = uiMenuAddMainItem (manage->menubar,
        /* CONTEXT: menu selection: export actions for song list */
        &manage->slmenu, _("Export"));

    menu = uiCreateSubMenu (menuitem);

    /* CONTEXT: menu selection: song list: export: export as m3u */
    menuitem = uiMenuCreateItem (menu, _("Export as M3U Playlist"), NULL, NULL);
    uiWidgetDisableW (menuitem);

    /* CONTEXT: menu selection: song list: export: export as m3u8 */
    menuitem = uiMenuCreateItem (menu, _("Export as M3U8 Playlist"), NULL, NULL);
    uiWidgetDisableW (menuitem);

    /* CONTEXT: menu selection: song list: export: export for ballroomdj */
    snprintf (tbuff, sizeof (tbuff), _("Export for %s"), BDJ4_NAME);
    menuitem = uiMenuCreateItem (menu, tbuff, NULL, NULL);
    uiWidgetDisableW (menuitem);

    menuitem = uiMenuAddMainItem (manage->menubar,
        /* CONTEXT: menu selection: import actions for song list */
        &manage->slmenu, _("Import"));

    menu = uiCreateSubMenu (menuitem);

    /* CONTEXT: menu selection: song list: import: import m3u */
    menuitem = uiMenuCreateItem (menu, _("Import M3U"), NULL, NULL);
    uiWidgetDisableW (menuitem);

    /* CONTEXT: menu selection: song list: import: import from ballroomdj */
    snprintf (tbuff, sizeof (tbuff), _("Import from %s"), BDJ4_NAME);
    menuitem = uiMenuCreateItem (menu, tbuff, NULL, NULL);
    uiWidgetDisableW (menuitem);

    manage->slmenu.initialized = true;
  }

  uiMenuDisplay (&manage->slmenu);
  manage->currmenu = &manage->slmenu;
}

static void
manageSonglistLoad (GtkMenuItem *mi, gpointer udata)
{
  manageui_t  *manage = udata;

  manageSelectFileDialog (manage, MANAGE_F_SONGLIST);
}

static void
manageSonglistCopy (GtkMenuItem *mi, gpointer udata)
{
  manageui_t  *manage = udata;
  const char  *oname;
  char        newname [200];

  manageSonglistSave (manage);

  oname = uimusicqGetSonglistName (manage->slmusicq);
  /* CONTEXT: the new name after 'create copy' (e.g. "Copy of DJ-2022-04") */
  snprintf (newname, sizeof (newname), _("Copy of %s"), oname);
  if (manageCreatePlaylistCopy (manage, oname, newname)) {
    manageSetSonglistName (manage, newname);
    manage->slbackupcreated = false;
  }
}

static void
manageSonglistNew (GtkMenuItem *mi, gpointer udata)
{
  manageui_t  *manage = udata;
  char        tbuff [200];

  manageSonglistSave (manage);

  /* CONTEXT: song list: default name for a new song list */
  snprintf (tbuff, sizeof (tbuff), _("New Song List"));
  manageSetSonglistName (manage, tbuff);
  manage->slbackupcreated = false;
  uimusicqSetSelection (manage->slmusicq, "0");
  uimusicqClearQueueProcess (manage->slmusicq);
}

static void
manageSonglistTruncate (GtkMenuItem *mi, gpointer udata)
{
  manageui_t  *manage = udata;

  uimusicqClearQueueProcess (manage->slmusicq);
}

static void
manageSonglistLoadFile (manageui_t *manage, const char *fn)
{
  char  tbuff [200];

  manageSonglistSave (manage);

  /* truncate from the first selection */
  uimusicqSetSelection (manage->slmusicq, "0");
  uimusicqClearQueueProcess (manage->slmusicq);

  snprintf (tbuff, sizeof (tbuff), "%d%c%s",
      manage->musicqManageIdx, MSG_ARGS_RS, fn);
  connSendMessage (manage->conn, ROUTE_MAIN, MSG_QUEUE_PLAYLIST, tbuff);

  manageSetSonglistName (manage, fn);
  manage->slbackupcreated = false;
}

static void
manageToggleEasySonglist (GtkWidget *mi, gpointer udata)
{
  manageui_t  *manage = udata;
  int         val;

  val = nlistGetNum (manage->options, MANAGE_EASY_SONGLIST);
  val = ! val;
  nlistSetNum (manage->options, MANAGE_EASY_SONGLIST, val);
  manageSetEasySonglist (manage);
}

static void
manageSetEasySonglist (manageui_t *manage)
{
  int     val;

  val = nlistGetNum (manage->options, MANAGE_EASY_SONGLIST);
  if (val) {
    uiWidgetHideW (manage->slmusicqtabwidget);
    uiWidgetHideW (manage->slsongseltabwidget);
    uiWidgetShowAllW (manage->slezmusicqtabwidget);
    uiNotebookSetPageW (manage->slnotebook, 0);
  } else {
    uiWidgetHideW (manage->slezmusicqtabwidget);
    uiWidgetShowAllW (manage->slmusicqtabwidget);
    uiWidgetShowAllW (manage->slsongseltabwidget);
    uiNotebookSetPageW (manage->slnotebook, 1);
  }
}

static void
manageSetSonglistName (manageui_t *manage, const char *nm)
{
  uimusicqSetSonglistName (manage->slmusicq, nm);
  if (manage->sloldname != NULL) {
    free (manage->sloldname);
  }
  manage->sloldname = strdup (nm);
}

static void
manageSonglistSave (manageui_t *manage)
{
  const char  *name;
  char        onm [MAXPATHLEN];
  char        nnm [MAXPATHLEN];

  if (manage->sloldname == NULL) {
    return;
  }

  if (uimusicqGetCount (manage->slmusicq) <= 0) {
    return;
  }

  name = uimusicqGetSonglistName (manage->slmusicq);

  /* the song list has been renamed */
  if (strcmp (manage->sloldname, name) != 0) {
    manageRenamePlaylistFiles (manage->sloldname, name);
  }

  pathbldMakePath (onm, sizeof (onm),
      name, BDJ4_SONGLIST_EXT, PATHBLD_MP_DATA);
  strlcat (onm, ".n", sizeof (onm));

  uimusicqSave (manage->slmusicq, onm);

  pathbldMakePath (nnm, sizeof (nnm),
      name, BDJ4_SONGLIST_EXT, PATHBLD_MP_DATA);
  if (! manage->slbackupcreated) {
    filemanipBackup (nnm, 1);
    manage->slbackupcreated = true;
  }
  filemanipMove (onm, nnm);

  manageCheckAndCreatePlaylist (manage, name, nnm, PLTYPE_MANUAL);
}

/* general */

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

  if (manage->mainlasttab == MANAGE_TAB_MAIN_SL && mmnb) {
    manageSonglistSave (manage);
  }
  if (manage->mainlasttab == MANAGE_TAB_EDITSEQ) {
    manageSequenceSave (manage);
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
      uiMenuClear (manage->currmenu);
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
  }
  if (mainnb && id == MANAGE_TAB_MAIN_MM) {
    /* force menu selection */
    mmnb = true;
    id = manage->mmlasttab;
  }

  switch (id) {
    case MANAGE_TAB_MAIN_SL: {
      break;
    }
    case MANAGE_TAB_MAIN_MM: {
      break;
    }
    case MANAGE_TAB_SONGLIST: {
      manageSonglistMenu (manage);
      break;
    }
    case MANAGE_TAB_SONGEDIT: {
      manageSongEditMenu (manage);
      break;
    }
    case MANAGE_TAB_PLMGMT: {
      break;
    }
    case MANAGE_TAB_EDITSEQ: {
      manageSequenceMenu (manage);
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
manageSelectFileDialog (manageui_t *manage, int flags)
{
  slist_t     *filelist;
  int         x, y;
  GtkWidget   *dialog;
  manageselfilecb_t cb = NULL;
  int         pltype;

  logProcBegin (LOG_PROC, "manageSelectFileDialog");
  pltype = PL_LIST_NORMAL;
  if ((flags & MANAGE_F_SONGLIST) == MANAGE_F_SONGLIST) {
    cb = manageSonglistLoadFile;
    pltype = PL_LIST_MANUAL;
  }
  if ((flags & MANAGE_F_PLAYLIST) == MANAGE_F_PLAYLIST) {
//      cb = managePlaylistLoadFile;
    pltype = PL_LIST_ALL;
  }
  if ((flags & MANAGE_F_SEQUENCE) == MANAGE_F_SEQUENCE) {
    cb = manageSequenceLoadFile;
    pltype = PL_LIST_SEQUENCE;
  }
  filelist = playlistGetPlaylistList (pltype);

  /* CONTEXT: what type of file to load */
  if (cb != NULL) {
    /* CONTEXT: file type for the file selection dialog (song list) */
    dialog = manageCreateSelectFileDialog (manage, filelist, _("Song List"), cb);
    uiWidgetShowAllW (dialog);

    x = nlistGetNum (manage->options, MANAGE_SELFILE_POSITION_X);
    y = nlistGetNum (manage->options, MANAGE_SELFILE_POSITION_Y);
    uiWindowMoveW (dialog, x, y);
  }
  logProcEnd (LOG_PROC, "manageSelectFileDialog", "");
}

static GtkWidget *
manageCreateSelectFileDialog (manageui_t *manage,
    slist_t *filelist, const char *filetype, manageselfilecb_t cb)
{
  GtkWidget     *dialog;
  GtkWidget     *content;
  GtkWidget     *vbox;
  UIWidget      hbox;
  GtkWidget     *scwin;
  GtkWidget     *widget;
  char          tbuff [200];
  GtkListStore  *store;
  GtkTreeIter   iter;
  slistidx_t    fliteridx;
  char          *disp;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;


  logProcBegin (LOG_PROC, "manageCreateSelectFileDialog");

  manage->selfilecb = cb;

  /* CONTEXT: file select dialog, title of window: select <file-type> */
  snprintf (tbuff, sizeof (tbuff), _("Select %s"), filetype);
  dialog = gtk_dialog_new_with_buttons (
      tbuff,
      GTK_WINDOW (manage->window),
      GTK_DIALOG_DESTROY_WITH_PARENT,
      /* CONTEXT: file select dialog: closes the dialog */
      _("Close"),
      GTK_RESPONSE_CLOSE,
      /* CONTEXT: file select dialog: selects the file */
      _("Select"),
      GTK_RESPONSE_APPLY,
      NULL
      );
  manage->selfiledialog = dialog;

  content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  uiWidgetSetAllMarginsW (content, uiBaseMarginSz * 2);

  vbox = uiCreateVertBoxWW ();
  uiBoxPackInWindowWW (content, vbox);

  scwin = uiCreateScrolledWindowW (250);
  uiWidgetExpandVertW (scwin);
  uiBoxPackStartExpandWW (vbox, scwin);

  widget = uiCreateTreeView ();
  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (widget), FALSE);
  uiWidgetAlignHorizFillW (widget);
  uiWidgetExpandHorizW (widget);
  manage->selfiletree = widget;

  store = gtk_list_store_new (MNG_SELFILE_COL_MAX,
      G_TYPE_STRING, G_TYPE_STRING);
  assert (store != NULL);

  slistStartIterator (filelist, &fliteridx);
  while ((disp = slistIterateKey (filelist, &fliteridx)) != NULL) {
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        MNG_SELFILE_COL_DISP, disp,
        MNG_SELFILE_COL_SB_PAD, "    ",
        -1);
  }

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", MNG_SELFILE_COL_DISP,
      NULL);
  gtk_tree_view_column_set_title (column, "");
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);

  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", MNG_SELFILE_COL_SB_PAD,
      NULL);
  gtk_tree_view_column_set_title (column, "");
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (widget), GTK_TREE_MODEL (store));
  g_object_unref (store);

  g_signal_connect (widget, "row-activated",
      G_CALLBACK (manageSelectFileSelect), manage);

  uiBoxPackInWindowWW (scwin, widget);

  /* the dialog doesn't have any space above the buttons */
  uiCreateHorizBox (&hbox);
  uiBoxPackStartWU (vbox, &hbox);

  widget = uiCreateLabelW (" ");
  uiBoxPackStartUW (&hbox, widget);

  g_signal_connect (dialog, "response",
      G_CALLBACK (manageSelectFileResponseHandler), manage);
  logProcEnd (LOG_PROC, "manageCreateSelectFileDialog", "");

  return dialog;
}

static void
manageSelectFileSelect (GtkTreeView* tv, GtkTreePath* path,
    GtkTreeViewColumn* column, gpointer udata)
{
  manageui_t  *manage = udata;

  manageSelectFileResponseHandler (GTK_DIALOG (manage->selfiledialog),
      GTK_RESPONSE_APPLY, manage);
}

static void
manageSelectFileResponseHandler (GtkDialog *d, gint responseid,
    gpointer udata)
{
  manageui_t    *manage = udata;
  gint          x, y;
  char          *str;
  GtkTreeModel  *model;
  GtkTreeIter   iter;
  int           count;

  uiWindowGetPositionW (GTK_WIDGET (d), &x, &y);
  nlistSetNum (manage->options, MANAGE_SELFILE_POSITION_X, x);
  nlistSetNum (manage->options, MANAGE_SELFILE_POSITION_Y, y);

  switch (responseid) {
    case GTK_RESPONSE_DELETE_EVENT: {
      manage->selfilecb = NULL;
      break;
    }
    case GTK_RESPONSE_CLOSE: {
      uiCloseWindowW (GTK_WIDGET (d));
      manage->selfilecb = NULL;
      break;
    }
    case GTK_RESPONSE_APPLY: {
      count = uiTreeViewGetSelection (manage->selfiletree, &model, &iter);
      if (count != 1) {
        break;
      }

      gtk_tree_model_get (model, &iter, MNG_SELFILE_COL_DISP, &str, -1);
      uiCloseWindowW (GTK_WIDGET (d));
      if (manage->selfilecb != NULL) {
        manage->selfilecb (manage, str);
      }
      free (str);
      manage->selfilecb = NULL;
      break;
    }
  }
}

static void
manageInitializeSongFilter (manageui_t *manage, nlist_t *options)
{
  songfilterSetSort (manage->slsongfilter,
      nlistGetStr (options, SONGSEL_SORT_BY));
  songfilterSetSort (manage->mmsongfilter,
      nlistGetStr (options, SONGSEL_SORT_BY));
}


static void
manageRenamePlaylistFiles (const char *oldname, const char *newname)
{
  char  onm [MAXPATHLEN];
  char  nnm [MAXPATHLEN];

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DATA);
  filemanipRenameAll (onm, nnm);

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_SONGLIST_EXT, PATHBLD_MP_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_SONGLIST_EXT, PATHBLD_MP_DATA);
  filemanipRenameAll (onm, nnm);

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DATA);
  filemanipRenameAll (onm, nnm);

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_PL_DANCE_EXT, PATHBLD_MP_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_PL_DANCE_EXT, PATHBLD_MP_DATA);
  filemanipRenameAll (onm, nnm);
}

static void
manageCopyPlaylistFiles (const char *oldname, const char *newname)
{
  char  onm [MAXPATHLEN];
  char  nnm [MAXPATHLEN];

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DATA);
  filemanipCopy (onm, nnm);

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_PL_DANCE_EXT, PATHBLD_MP_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_PL_DANCE_EXT, PATHBLD_MP_DATA);
  filemanipCopy (onm, nnm);
}

static void
manageSetStatusMsg (manageui_t *manage, const char *msg)
{
  uiLabelSetText (&manage->statusMsg, msg);
}

static void
manageCheckAndCreatePlaylist (manageui_t *manage, const char *name,
    const char *suppfname, pltype_t pltype)
{
  char  onm [MAXPATHLEN];

  pathbldMakePath (onm, sizeof (onm),
      name, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DATA);
  if (! fileopFileExists (onm) &&
      /* CONTEXT: name of the special song list for raffle songs */
      strcmp (name, _("Raffle Songs")) != 0) {
    playlist_t    *pl;

    pl = playlistAlloc (manage->musicdb);
    playlistCreate (pl, name, pltype, suppfname);
    playlistSave (pl);
    playlistFree (pl);
  }
}

static bool
manageCreatePlaylistCopy (manageui_t *manage,
    const char *oname, const char *newname)
{
  char  tbuff [MAXPATHLEN];
  bool  rc = true;

  pathbldMakePath (tbuff, sizeof (tbuff),
      newname, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DATA);
  if (fileopFileExists (tbuff)) {
    /* CONTEXT: failure status message */
    snprintf (tbuff, sizeof (tbuff), _("Copy already exists."));
    manageSetStatusMsg (manage, tbuff);
    rc = false;
  }
  if (rc) {
    manageCopyPlaylistFiles (oname, newname);
  }
  return rc;
}


/* sequence */

static void
manageSequenceMenu (manageui_t *manage)
{
  GtkWidget *menu;
  GtkWidget *menuitem;

  if (! manage->seqmenu.initialized) {
    menuitem = uiMenuAddMainItem (manage->menubar,
        /* CONTEXT: menu selection: sequence: edit menu */
        &manage->seqmenu, _("Edit"));

    menu = uiCreateSubMenu (menuitem);

    /* CONTEXT: menu selection: sequence: edit menu: load */
    menuitem = uiMenuCreateItem (menu, _("Load"),
        manageSequenceLoad, manage);

    /* CONTEXT: menu selection: sequence: edit menu: create copy */
    menuitem = uiMenuCreateItem (menu, _("Create Copy"),
        manageSequenceCopy, manage);

    /* CONTEXT: menu selection: sequence: edit menu: start new sequence */
    menuitem = uiMenuCreateItem (menu, _("Start New Sequence"),
        manageSequenceNew, manage);

    manage->seqmenu.initialized = true;
  }

  uiMenuDisplay (&manage->seqmenu);
  manage->currmenu = &manage->seqmenu;
}

static void
manageSequenceLoad (GtkMenuItem *mi, gpointer udata)
{
  manageui_t  *manage = udata;

  manageSelectFileDialog (manage, MANAGE_F_SEQUENCE);
}

static void
manageSequenceLoadFile (manageui_t *manage, const char *fn)
{
  sequence_t  *seq = NULL;
  char        *dstr = NULL;
  nlist_t     *dancelist = NULL;
  slist_t     *tlist = NULL;
  nlistidx_t  iteridx;
  nlistidx_t  didx;

  manageSequenceSave (manage);

  seq = sequenceAlloc (fn);
  if (seq == NULL) {
    return;
  }

  dancelist = sequenceGetDanceList (seq);
  tlist = slistAlloc ("temp-seq", LIST_UNORDERED, NULL);
  nlistStartIterator (dancelist, &iteridx);
  while ((didx = nlistIterateKey (dancelist, &iteridx)) >= 0) {
    dstr = nlistGetStr (dancelist, didx);
    slistSetNum (tlist, dstr, didx);
  }
  uiduallistSet (manage->seqduallist, tlist, DUALLIST_TREE_TARGET);
  slistFree (tlist);

  manageSetSequenceName (manage, fn);
  manage->seqbackupcreated = false;
}

static void
manageSequenceCopy (GtkMenuItem *mi, gpointer udata)
{
  manageui_t  *manage = udata;
  const char  *oname;
  char        newname [200];

  manageSequenceSave (manage);

  oname = uiEntryGetValue (&manage->seqname);
  /* CONTEXT: the new name after 'create copy' (e.g. "Copy of DJ-2022-04") */
  snprintf (newname, sizeof (newname), _("Copy of %s"), oname);
  if (manageCreatePlaylistCopy (manage, oname, newname)) {
    manageSetSequenceName (manage, newname);
    manage->seqbackupcreated = false;
  }
}

static void
manageSequenceNew (GtkMenuItem *mi, gpointer udata)
{
  manageui_t  *manage = udata;
  char        tbuff [200];
  slist_t     *tlist;

  manageSequenceSave (manage);

  /* CONTEXT: sequence: default name for a new sequence */
  snprintf (tbuff, sizeof (tbuff), _("New Sequence"));
  manageSetSequenceName (manage, tbuff);
  manage->seqbackupcreated = false;
  tlist = slistAlloc ("tmp-sequence", LIST_UNORDERED, NULL);
  uiduallistSet (manage->seqduallist, tlist, DUALLIST_TREE_TARGET);
  slistFree (tlist);
}

static void
manageSequenceSave (manageui_t *manage)
{
  sequence_t  *seq = NULL;
  slist_t     *slist;
  char        onm [MAXPATHLEN];
  char        nnm [MAXPATHLEN];
  const char  *name;

  if (manage->seqoldname == NULL) {
    return;
  }

  slist = uiduallistGetList (manage->seqduallist);
  if (slistGetCount (slist) <= 0) {
    slistFree (slist);
    return;
  }

  name = uiEntryGetValue (&manage->seqname);

  /* the song list has been renamed */
  if (strcmp (manage->seqoldname, name) != 0) {
    manageRenamePlaylistFiles (manage->seqoldname, name);
  }

  pathbldMakePath (onm, sizeof (onm),
      name, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DATA);
  strlcat (onm, ".n", sizeof (onm));

  seq = sequenceCreate (name);
  sequenceSave (seq, slist);
  sequenceFree (seq);

  pathbldMakePath (nnm, sizeof (nnm),
      name, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DATA);
  if (! manage->seqbackupcreated) {
    filemanipBackup (nnm, 1);
    manage->seqbackupcreated = true;
  }
  filemanipMove (onm, nnm);

  manageCheckAndCreatePlaylist (manage, name, nnm, PLTYPE_SEQ);
  slistFree (slist);
}

static void
manageSetSequenceName (manageui_t *manage, const char *name)
{
  uiEntrySetValue (&manage->seqname, name);
  if (manage->seqoldname != NULL) {
    free (manage->seqoldname);
  }
  manage->seqoldname = strdup (name);
}
