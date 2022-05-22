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

#include "bdj4.h"
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdj4ui.h"
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
#include "manageui.h"
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
#include "uimusicq.h"
#include "uiplayer.h"
#include "uiselectfile.h"
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
  MANAGE_NB_MAIN,
  MANAGE_NB_SONGLIST,
  MANAGE_NB_MM,
};

enum {
  MANAGE_DB_CHECK_NEW,
  MANAGE_DB_REORGANIZE,
  MANAGE_DB_UPD_FROM_TAGS,
  MANAGE_DB_WRITE_TAGS,
  MANAGE_DB_REBUILD,
};

enum {
  MANAGE_CALLBACK_EZ_SELECT,
  MANAGE_CALLBACK_DB_START,
  MANAGE_CALLBACK_MAX,
};

enum {
  MANAGE_MENU_CB_EZ_SL_EDIT,
  MANAGE_MENU_CB_SL_LOAD,
  MANAGE_MENU_CB_SL_COPY,
  MANAGE_MENU_CB_SL_NEW,
  MANAGE_MENU_CB_SL_MIX,
  MANAGE_MENU_CB_SL_TRUNCATE,
  MANAGE_MENU_CB_SL_EXP_M3U,
  MANAGE_MENU_CB_SL_EXP_M3U8,
  MANAGE_MENU_CB_SL_EXP_BDJ,
  MANAGE_MENU_CB_SL_IMP_M3U,
  MANAGE_MENU_CB_SL_IMP_BDJ,
  MANAGE_MENU_CB_START_EDITALL,
  MANAGE_MENU_CB_APPLY_EDITALL,
  MANAGE_MENU_CB_CANCEL_EDITALL,
  MANAGE_MENU_CB_MAX,
};

typedef struct manage manageui_t;

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
  uiutilsnbtabid_t  *mainnbtabid;
  uiutilsnbtabid_t  *slnbtabid;
  uiutilsnbtabid_t  *mmnbtabid;
  UIWidget        window;
  UICallback      closecb;
  UIWidget        menubar;
  UICallback      menucb [MANAGE_MENU_CB_MAX];
  UIWidget        mainnotebook;
  UICallback      mainnbcb;
  UIWidget        slnotebook;
  UICallback      slnbcb;
  UICallback      mmnbcb;
  /* song list ui major elements */
  uiplayer_t      *slplayer;
  uimusicq_t      *slmusicq;
  uisongsel_t     *slsongsel;
  uisongedit_t    *slsongedit;
  uimusicq_t      *slezmusicq;
  uisongsel_t     *slezsongsel;
  UIWidget        slezmusicqtabwidget;
  UIWidget        *slmusicqtabwidget;
  UIWidget        *slsongseltabwidget;
  char            *sloldname;
  songfilter_t    *slsongfilter;
  songfilter_t    *mmsongfilter;
  /* music manager ui */
  uiplayer_t      *mmplayer;
  uimusicq_t      *mmmusicq;
  uisongsel_t     *mmsongsel;
  uisongedit_t    *mmsongedit;
  /* sequence */
  manageseq_t     *manageseq;
  /* playlist management */
  managepl_t      *managepl;
  /* options */
  datafile_t      *optiondf;
  nlist_t         *options;
  /* various flags */
  bool            slbackupcreated : 1;
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
static int      manageMainLoop  (void *tmanage);
static int      manageProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static bool     manageCloseWin (void *udata);
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
static bool     manageSonglistLoad (void *udata);
static bool     manageSonglistCopy (void *udata);
static bool     manageSonglistNew (void *udata);
static bool     manageSonglistTruncate (void *udata);
static void     manageSonglistLoadFile (void *udata, const char *fn);
static bool     manageToggleEasySonglist (void *udata);
static void     manageSetEasySonglist (manageui_t *manage);
static void     manageSonglistSave (manageui_t *manage);
static void     manageSetSonglistName (manageui_t *manage, const char *nm);
/* general */
static bool     manageSwitchPageMain (void *udata, int pagenum);
static bool     manageSwitchPageSonglist (void *udata, int pagenum);
static bool     manageSwitchPageMM (void *udata, int pagenum);
static void     manageSwitchPage (void *udata, int pagenum, int which);
static void manageInitializeSongFilter (manageui_t *manage, nlist_t *options);
static void manageSetMenuCallback (manageui_t *manage, int midx, UICallbackFunc cb);

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


  manage.progstate = progstateInit ("manageui");
  progstateSetCallback (manage.progstate, STATE_CONNECTING,
      manageConnectingCallback, &manage);
  progstateSetCallback (manage.progstate, STATE_WAIT_HANDSHAKE,
      manageHandshakeCallback, &manage);

  uiutilsUIWidgetInit (&manage.mainnotebook);
  uiutilsUIWidgetInit (&manage.window);
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
  manage.mainnbtabid = uiutilsNotebookIDInit ();
  manage.slnbtabid = uiutilsNotebookIDInit ();
  manage.mmnbtabid = uiutilsNotebookIDInit ();
  manage.sloldname = NULL;
  manage.slbackupcreated = false;
  manage.slsongfilter = NULL;
  manage.mmsongfilter = NULL;
  uiutilsUIWidgetInit (&manage.dbpbar);
  manage.manageseq = NULL;   /* allocated within buildui */
  manage.managepl = NULL;   /* allocated within buildui */

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
  int           x, y;

  logProcBegin (LOG_PROC, "manageStoppingCallback");
  connSendMessage (manage->conn, ROUTE_STARTERUI, MSG_STOP_MAIN, NULL);

  manageSonglistSave (manage);
  manageSequenceSave (manage->manageseq);
  managePlaylistSave (manage->managepl);

  uiWindowGetSize (&manage->window, &x, &y);
  nlistSetNum (manage->options, PLUI_SIZE_X, x);
  nlistSetNum (manage->options, PLUI_SIZE_Y, y);
  uiWindowGetPosition (&manage->window, &x, &y);
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

  uiCloseWindow (&manage->window);

  procutilStopAllProcess (manage->processes, manage->conn, true);
  procutilFreeAll (manage->processes);

  pathbldMakePath (fn, sizeof (fn),
      "manageui", BDJ4_CONFIG_EXT, PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("manageui", fn, manageuidfkeys, MANAGEUI_DFKEY_COUNT, manage->options);

  bdj4shutdown (ROUTE_MANAGEUI, manage->musicdb);
  dispselFree (manage->dispsel);
  manageSequenceFree (manage->manageseq);
  managePlaylistFree (manage->managepl);

  if (manage->slsongfilter != NULL) {
    songfilterFree (manage->slsongfilter);
  }
  if (manage->mmsongfilter != NULL) {
    songfilterFree (manage->mmsongfilter);
  }
  if (manage->sloldname != NULL) {
    free (manage->sloldname);
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
  uisongeditFree (manage->slsongedit);

  uimusicqFree (manage->slezmusicq);
  uisongselFree (manage->slezsongsel);

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
  UIWidget            vbox;
  UIWidget            hbox;
  UIWidget            uiwidget;
  char                imgbuff [MAXPATHLEN];
  char                tbuff [MAXPATHLEN];
  int                 x, y;

  logProcBegin (LOG_PROC, "manageBuildUI");
  *imgbuff = '\0';
  uiutilsUIWidgetInit (&vbox);
  uiutilsUIWidgetInit (&hbox);
  uiutilsUIWidgetInit (&uiwidget);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", ".svg", PATHBLD_MP_IMGDIR);
  /* CONTEXT: management user interface window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Management"), BDJ4_NAME);

  uiutilsUICallbackInit (&manage->closecb, manageCloseWin, manage);
  uiCreateMainWindow (&manage->window, &manage->closecb, tbuff, imgbuff);

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, 4);
  uiBoxPackInWindow (&manage->window, &vbox);

  uiCreateHorizBox (&hbox);
  uiWidgetSetMarginTop (&hbox, uiBaseMarginSz * 4);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateLabel (&uiwidget, "");
  uiLabelSetColor (&uiwidget, bdjoptGetStr (OPT_P_UI_ACCENT_COL));
  uiBoxPackEnd (&hbox, &uiwidget);
  uiutilsUIWidgetCopy (&manage->statusMsg, &uiwidget);

  uiCreateMenubar (&manage->menubar);
  uiBoxPackStart (&hbox, &manage->menubar);

  uiCreateNotebook (&manage->mainnotebook);
  uiNotebookTabPositionLeft (&manage->mainnotebook);
  uiBoxPackStartExpand (&vbox, &manage->mainnotebook);

  manageBuildUISongListEditor (manage);
  manageBuildUIMusicManager (manage);
  manageBuildUIUpdateDatabase (manage);

  manage->managepl = managePlaylistAlloc (&manage->window,
      manage->options, &manage->statusMsg);

  /* playlist management */
  uiCreateVertBox (&vbox);
  manageBuildUIPlaylist (manage->managepl, &vbox);
  /* CONTEXT: notebook tab title: playlist management */
  uiCreateLabel (&uiwidget, _("Playlist Management"));
  uiNotebookAppendPage (&manage->mainnotebook, &vbox, &uiwidget);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_PLMGMT);

  manage->manageseq = manageSequenceAlloc (&manage->window,
      manage->options, &manage->statusMsg);

  uiCreateVertBox (&vbox);
  manageBuildUISequence (manage->manageseq, &vbox);
  /* CONTEXT: notebook tab title: edit sequences */
  uiCreateLabel (&uiwidget, _("Edit Sequences"));
  uiNotebookAppendPage (&manage->mainnotebook, &vbox, &uiwidget);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_EDITSEQ);

  /* file manager */
  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, uiBaseMarginSz * 2);
  /* CONTEXT: notebook tab title: file manager */
  uiCreateLabel (&uiwidget, _("File Manager"));
  uiNotebookAppendPage (&manage->mainnotebook, &vbox, &uiwidget);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_FILEMGR);

  x = nlistGetNum (manage->options, PLUI_SIZE_X);
  y = nlistGetNum (manage->options, PLUI_SIZE_Y);
  uiWindowSetDefaultSize (&manage->window, x, y);

  uiutilsUICallbackIntInit (&manage->mainnbcb, manageSwitchPageMain, manage);
  uiNotebookSetCallback (&manage->mainnotebook, &manage->mainnbcb);

  uiWidgetShowAll (&manage->window);

  x = nlistGetNum (manage->options, PLUI_POSITION_X);
  y = nlistGetNum (manage->options, PLUI_POSITION_Y);
  uiWindowMove (&manage->window, x, y);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", ".png", PATHBLD_MP_IMGDIR);
  osuiSetIcon (imgbuff);

  logProcEnd (LOG_PROC, "manageBuildUI", "");
}

static void
manageBuildUISongListEditor (manageui_t *manage)
{
  UIWidget            uiwidget;
  UIWidget            vbox;
  UIWidget            hbox;
  UIWidget            mainhbox;
  UIWidget            *uiwidgetp;
  UIWidget            notebook;

  /* song list editor */
  uiutilsUIWidgetInit (&hbox);

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, uiBaseMarginSz * 2);

  /* CONTEXT: notebook tab title: edit song lists */
  uiCreateLabel (&uiwidget, _("Edit Song Lists"));
  uiNotebookAppendPage (&manage->mainnotebook, &vbox, &uiwidget);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_MAIN_SL);

  /* song list editor: player */
  uiwidgetp = uiplayerBuildUI (manage->slplayer);
  uiBoxPackStart (&vbox, uiwidgetp);

  uiCreateNotebook (&notebook);
  uiBoxPackStartExpand (&vbox, &notebook);
  uiutilsUIWidgetCopy (&manage->slnotebook, &notebook);

  /* song list editor: easy song list tab */
  uiCreateHorizBox (&mainhbox);

  /* CONTEXT: name of easy song list song selection tab */
  uiCreateLabel (&uiwidget, _("Song List"));
  uiNotebookAppendPage (&notebook, &mainhbox, &uiwidget);
  uiutilsNotebookIDAdd (manage->slnbtabid, MANAGE_TAB_SONGLIST);
  uiutilsUIWidgetCopy (&manage->slezmusicqtabwidget, &mainhbox);

  uiCreateHorizBox (&hbox);
  uiBoxPackStartExpand (&mainhbox, &hbox);

  uiwidgetp = uimusicqBuildUI (manage->slezmusicq, manage->window.widget, MUSICQ_A);
  uiBoxPackStartExpand (&hbox, uiwidgetp);

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, uiBaseMarginSz * 4);
  uiWidgetSetMarginTop (&vbox, uiBaseMarginSz * 64);
  uiBoxPackStart (&hbox, &vbox);

  uiutilsUICallbackInit (&manage->callbacks [MANAGE_CALLBACK_EZ_SELECT],
      uisongselQueueProcessSelectCallback, manage->slezsongsel);
  uiCreateButton (&uiwidget,
      &manage->callbacks [MANAGE_CALLBACK_EZ_SELECT],
      /* CONTEXT: config: button: add the selected songs to the song list */
      _("Select"), "button_left", NULL, NULL);
  uiBoxPackStart (&vbox, &uiwidget);

  uiwidgetp = uisongselBuildUI (manage->slezsongsel, &manage->window);
  uiBoxPackStartExpand (&hbox, uiwidgetp);

  /* song list editor: music queue tab */
  uiwidgetp = uimusicqBuildUI (manage->slmusicq, manage->window.widget, MUSICQ_A);
  /* CONTEXT: name of easy song list notebook tab */
  uiCreateLabel (&uiwidget, _("Song List"));
  uiNotebookAppendPage (&notebook, uiwidgetp, &uiwidget);
  uiutilsNotebookIDAdd (manage->slnbtabid, MANAGE_TAB_SONGLIST);
  manage->slmusicqtabwidget = uiwidgetp;

  /* song list editor: song selection tab*/
  uiwidgetp = uisongselBuildUI (manage->slsongsel, &manage->window);
  /* CONTEXT: name of song selection notebook tab */
  uiCreateLabel (&uiwidget, _("Song Selection"));
  uiNotebookAppendPage (&notebook, uiwidgetp, &uiwidget);
  uiutilsNotebookIDAdd (manage->slnbtabid, MANAGE_TAB_OTHER);
  manage->slsongseltabwidget = uiwidgetp;

  /* song list editor song editor tab */
  uiwidgetp = uisongeditBuildUI (manage->slsongedit, &manage->window);
  /* CONTEXT: name of song editor notebook tab */
  uiCreateLabel (&uiwidget, _("Song Editor"));
  uiNotebookAppendPage (&notebook, uiwidgetp, &uiwidget);
  uiutilsNotebookIDAdd (manage->slnbtabid, MANAGE_TAB_SONGEDIT);

  uimusicqPeerSonglistName (manage->slmusicq, manage->slezmusicq);

  uiutilsUICallbackIntInit (&manage->slnbcb, manageSwitchPageSonglist, manage);
  uiNotebookSetCallback (&notebook, &manage->slnbcb);
}

static void
manageBuildUIMusicManager (manageui_t *manage)
{
  UIWidget            *uiwidgetp;
  UIWidget            vbox;
  UIWidget            uiwidget;
  UIWidget            notebook;

  /* music manager */
  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, uiBaseMarginSz * 2);
  /* CONTEXT: name of music manager notebook tab */
  uiCreateLabel (&uiwidget, _("Music Manager"));
  uiNotebookAppendPage (&manage->mainnotebook, &vbox, &uiwidget);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_MAIN_MM);

  /* music manager: player */
  uiwidgetp = uiplayerBuildUI (manage->mmplayer);
  uiBoxPackStart (&vbox, uiwidgetp);

  uiCreateNotebook (&notebook);
  uiBoxPackStartExpand (&vbox, &notebook);

  /* music manager: song selection tab*/
  uiwidgetp = uisongselBuildUI (manage->mmsongsel, &manage->window);
  uiWidgetExpandHoriz (uiwidgetp);
  /* CONTEXT: name of song selection notebook tab */
  uiCreateLabel (&uiwidget, _("Music Manager"));
  uiNotebookAppendPage (&notebook, uiwidgetp, &uiwidget);
  uiutilsNotebookIDAdd (manage->mmnbtabid, MANAGE_TAB_OTHER);

  /* music manager: song editor tab */
  uiwidgetp = uisongeditBuildUI (manage->mmsongedit, &manage->window);
  /* CONTEXT: name of song editor notebook tab */
  uiCreateLabel (&uiwidget, _("Song Editor"));
  uiNotebookAppendPage (&notebook, uiwidgetp, &uiwidget);
  uiutilsNotebookIDAdd (manage->mmnbtabid, MANAGE_TAB_SONGEDIT);

  uiutilsUICallbackIntInit (&manage->mmnbcb, manageSwitchPageMM, manage);
  uiNotebookSetCallback (&notebook, &manage->mmnbcb);
}

static void
manageBuildUIUpdateDatabase (manageui_t *manage)
{
  UIWidget       uiwidget;
  UIWidget       vbox;
  UIWidget       hbox;
  GtkWidget      *widget;
  uitextbox_t    *tb;

  /* update database */
  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, uiBaseMarginSz * 2);
  /* CONTEXT: notebook tab title: update database */
  uiCreateLabel (&uiwidget, _("Update Database"));
  uiNotebookAppendPage (&manage->mainnotebook, &vbox, &uiwidget);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_OTHER);

  /* help display */
  tb = uiTextBoxCreate (80);
  uiTextBoxSetReadonly (tb);
  uiTextBoxSetHeight (tb, 70);
  uiBoxPackStart (&vbox, uiTextBoxGetScrolledWindow (tb));
  manage->dbhelpdisp = tb;

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  widget = uiSpinboxTextCreateW (&manage->dbspinbox, manage);
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
  uiBoxPackStart (&vbox, &manage->dbpbar);

  tb = uiTextBoxCreate (200);
  uiTextBoxSetReadonly (tb);
  uiTextBoxDarken (tb);
  uiTextBoxSetHeight (tb, 300);
  uiBoxPackStartExpand (&vbox, uiTextBoxGetScrolledWindow (tb));
  manage->dbstatus = tb;
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


static bool
manageCloseWin (void *udata)
{
  manageui_t   *manage = udata;

  logProcBegin (LOG_PROC, "manageCloseWin");
  if (progstateCurrState (manage->progstate) <= STATE_RUNNING) {
    progstateShutdownProcess (manage->progstate);
    logMsg (LOG_DBG, LOG_MSGS, "got: close win request");
    logProcEnd (LOG_PROC, "manageCloseWin", "not-done");
    return UICB_STOP;
  }

  logProcEnd (LOG_PROC, "manageCloseWin", "");
  return UICB_STOP;
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
  double          value;
  ssize_t         nval;
  char            *sval;

  nval = MANAGE_DB_CHECK_NEW;
  if (sb != NULL) {
    value = uiSpinboxTextGetValue (&manage->dbspinbox);
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
  UIWidget  menu;
  UIWidget  menuitem;

  if (! manage->songeditmenu.initialized) {
    uiMenuAddMainItem (&manage->menubar, &menuitem,
        /* CONTEXT: menu selection: actions for song editor */
        &manage->songeditmenu, _("Actions"));

    uiCreateSubMenu (&menuitem, &menu);

    /* CONTEXT: menu selection: song editor: edit all */
    uiMenuCreateItem (&menu, &menuitem, _("Edit All"), NULL);
    uiWidgetDisable (&menuitem);

    /* CONTEXT: menu selection: song editor: apply edit all */
    uiMenuCreateItem (&menu, &menuitem, _("Apply Edit All"), NULL);
    uiWidgetDisable (&menuitem);

    /* CONTEXT: menu selection: song editor: cancel edit all */
    uiMenuCreateItem (&menu, &menuitem, _("Cancel Edit All"), NULL);
    uiWidgetDisable (&menuitem);

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
  UIWidget  menu;
  UIWidget  menuitem;

  if (! manage->slmenu.initialized) {
    uiMenuAddMainItem (&manage->menubar, &menuitem,
        /* CONTEXT: menu selection: song list: options menu */
        &manage->slmenu, _("Options"));

    uiCreateSubMenu (&menuitem, &menu);

    /* CONTEXT: menu checkbox: easy song list editor */
    manageSetMenuCallback (manage, MANAGE_MENU_CB_EZ_SL_EDIT,
        manageToggleEasySonglist);
    uiMenuCreateCheckbox (&menu, &menuitem, _("Easy Song List Editor"),
        nlistGetNum (manage->options, MANAGE_EASY_SONGLIST),
        &manage->menucb [MANAGE_MENU_CB_EZ_SL_EDIT]);

    uiMenuAddMainItem (&manage->menubar, &menuitem,
        /* CONTEXT: menu selection: song list: edit menu */
        &manage->slmenu, _("Edit"));

    uiCreateSubMenu (&menuitem, &menu);

    /* CONTEXT: menu selection: song list: edit menu: load */
    manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_LOAD, manageSonglistLoad);
    uiMenuCreateItem (&menu, &menuitem, _("Load"),
        &manage->menucb [MANAGE_MENU_CB_SL_LOAD]);

    /* CONTEXT: menu selection: song list: edit menu: create copy */
    manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_COPY, manageSonglistCopy);
    uiMenuCreateItem (&menu, &menuitem, _("Create Copy"),
        &manage->menucb [MANAGE_MENU_CB_SL_COPY]);

    /* CONTEXT: menu selection: song list: edit menu: start new song list */
    manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_NEW, manageSonglistNew);
    uiMenuCreateItem (&menu, &menuitem, _("Start New Song List"),
        &manage->menucb [MANAGE_MENU_CB_SL_NEW]);

    uiMenuAddMainItem (&manage->menubar, &menuitem,
        /* CONTEXT: menu selection: actions for song list */
        &manage->slmenu, _("Actions"));

    uiCreateSubMenu (&menuitem, &menu);

    /* CONTEXT: menu selection: song list: actions menu: rearrange the songs and create a new mix */
    uiMenuCreateItem (&menu, &menuitem, _("Mix"), NULL);
    uiWidgetDisable (&menuitem);

    /* CONTEXT: menu selection: song list: actions menu: truncate the song list */
    manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_TRUNCATE,
        manageSonglistTruncate);
    uiMenuCreateItem (&menu, &menuitem, _("Truncate"),
        &manage->menucb [MANAGE_MENU_CB_SL_TRUNCATE]);

    uiMenuAddMainItem (&manage->menubar, &menuitem,
        /* CONTEXT: menu selection: export actions for song list */
        &manage->slmenu, _("Export"));

    uiCreateSubMenu (&menuitem, &menu);

    /* CONTEXT: menu selection: song list: export: export as m3u */
    uiMenuCreateItem (&menu, &menuitem, _("Export as M3U Playlist"), NULL);
    uiWidgetDisable (&menuitem);

    /* CONTEXT: menu selection: song list: export: export as m3u8 */
    uiMenuCreateItem (&menu, &menuitem, _("Export as M3U8 Playlist"), NULL);
    uiWidgetDisable (&menuitem);

    /* CONTEXT: menu selection: song list: export: export for ballroomdj */
    snprintf (tbuff, sizeof (tbuff), _("Export for %s"), BDJ4_NAME);
    uiMenuCreateItem (&menu, &menuitem, tbuff, NULL);
    uiWidgetDisable (&menuitem);

    uiMenuAddMainItem (&manage->menubar, &menuitem,
        /* CONTEXT: menu selection: import actions for song list */
        &manage->slmenu, _("Import"));

    uiCreateSubMenu (&menuitem, &menu);

    /* CONTEXT: menu selection: song list: import: import m3u */
    uiMenuCreateItem (&menu, &menuitem, _("Import M3U"), NULL);
    uiWidgetDisable (&menuitem);

    /* CONTEXT: menu selection: song list: import: import from ballroomdj */
    snprintf (tbuff, sizeof (tbuff), _("Import from %s"), BDJ4_NAME);
    uiMenuCreateItem (&menu, &menuitem, tbuff, NULL);
    uiWidgetDisable (&menuitem);

    manage->slmenu.initialized = true;
  }

  uiMenuDisplay (&manage->slmenu);
  manage->currmenu = &manage->slmenu;
}

static bool
manageSonglistLoad (void *udata)
{
  manageui_t  *manage = udata;

  selectFileDialog (SELFILE_SONGLIST, &manage->window, manage->options,
      manage, manageSonglistLoadFile);
  return UICB_CONT;
}

static bool
manageSonglistCopy (void *udata)
{
  manageui_t  *manage = udata;
  const char  *oname;
  char        newname [200];

  manageSonglistSave (manage);

  oname = uimusicqGetSonglistName (manage->slmusicq);
  /* CONTEXT: the new name after 'create copy' (e.g. "Copy of DJ-2022-04") */
  snprintf (newname, sizeof (newname), _("Copy of %s"), oname);
  if (manageCreatePlaylistCopy (&manage->statusMsg, oname, newname)) {
    manageSetSonglistName (manage, newname);
    manage->slbackupcreated = false;
  }
  return UICB_CONT;
}

static bool
manageSonglistNew (void *udata)
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
  return UICB_CONT;
}

static bool
manageSonglistTruncate (void *udata)
{
  manageui_t  *manage = udata;

  uimusicqClearQueueProcess (manage->slmusicq);
  return UICB_CONT;
}

static void
manageSonglistLoadFile (void *udata, const char *fn)
{
  manageui_t  *manage = udata;
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

static bool
manageToggleEasySonglist (void *udata)
{
  manageui_t  *manage = udata;
  int         val;

  val = nlistGetNum (manage->options, MANAGE_EASY_SONGLIST);
  val = ! val;
  nlistSetNum (manage->options, MANAGE_EASY_SONGLIST, val);
  manageSetEasySonglist (manage);
  return UICB_CONT;
}

static void
manageSetEasySonglist (manageui_t *manage)
{
  int     val;

  val = nlistGetNum (manage->options, MANAGE_EASY_SONGLIST);
  if (val) {
    uiWidgetShowAll (&manage->slezmusicqtabwidget);
    uiWidgetHide (manage->slmusicqtabwidget);
    uiWidgetHide (manage->slsongseltabwidget);
    uiNotebookSetPage (&manage->slnotebook, 0);
  } else {
    uiWidgetHide (&manage->slezmusicqtabwidget);
    uiWidgetShowAll (manage->slmusicqtabwidget);
    uiWidgetShowAll (manage->slsongseltabwidget);
    uiNotebookSetPage (&manage->slnotebook, 1);
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

  manageCheckAndCreatePlaylist (name, nnm, PLTYPE_SONGLIST);
}

/* general */

static bool
manageSwitchPageMain (void *udata, int pagenum)
{
  manageSwitchPage (udata, pagenum, MANAGE_NB_MAIN);
  return UICB_CONT;
}

static bool
manageSwitchPageSonglist (void *udata, int pagenum)
{
  manageSwitchPage (udata, pagenum, MANAGE_NB_SONGLIST);
  return UICB_CONT;
}

static bool
manageSwitchPageMM (void *udata, int pagenum)
{
  manageSwitchPage (udata, pagenum, MANAGE_NB_MM);
  return UICB_CONT;
}

static void
manageSwitchPage (void *udata, int pagenum, int which)
{
  manageui_t  *manage = udata;
  int         id;
  bool        mainnb = false;
  bool        slnb = false;
  bool        mmnb = false;
  uiutilsnbtabid_t  *nbtabid = NULL;

  /* need to know which notebook is selected so that the correct id value */
  /* can be retrieved */
  /* how to do this? */
  if (which == MANAGE_NB_MAIN) {
    nbtabid = manage->mainnbtabid;
    mainnb = true;
  }
  if (which == MANAGE_NB_SONGLIST) {
    nbtabid = manage->slnbtabid;
    slnb = true;
  }
  if (which == MANAGE_NB_MM) {
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
    manageSequenceSave (manage->manageseq);
  }
  if (manage->mainlasttab == MANAGE_TAB_PLMGMT) {
    managePlaylistSave (manage->managepl);
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
      manage->currmenu = managePlaylistMenu (manage->managepl, &manage->menubar);
      break;
    }
    case MANAGE_TAB_EDITSEQ: {
      manage->currmenu = manageSequenceMenu (manage->manageseq, &manage->menubar);
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
manageInitializeSongFilter (manageui_t *manage, nlist_t *options)
{
  songfilterSetSort (manage->slsongfilter,
      nlistGetStr (options, SONGSEL_SORT_BY));
  songfilterSetSort (manage->mmsongfilter,
      nlistGetStr (options, SONGSEL_SORT_BY));
}

static void
manageSetMenuCallback (manageui_t *manage, int midx, UICallbackFunc cb)
{
  uiutilsUICallbackInit (&manage->menucb [midx], cb, manage);
}
