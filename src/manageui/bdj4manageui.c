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
#include "dance.h"
#include "datafile.h"
#include "dispsel.h"
#include "fileop.h"
#include "filemanip.h"
#include "localeutil.h"
#include "lock.h"
#include "log.h"
#include "manageui.h"
#include "m3u.h"
#include "musicq.h"
#include "osuiutils.h"
#include "osutils.h"
#include "pathbld.h"
#include "pathutil.h"
#include "playlist.h"
#include "procutil.h"
#include "progstate.h"
#include "sequence.h"
#include "slist.h"
#include "sock.h"
#include "sockh.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"
#include "ui.h"
#include "uimusicq.h"
#include "uinbutil.h"
#include "uiplayer.h"
#include "uiselectfile.h"
#include "uisongedit.h"
#include "uisongfilter.h"
#include "uisongsel.h"

enum {
  MANAGE_TAB_OTHER,
  MANAGE_TAB_MAIN_SL,
  MANAGE_TAB_MAIN_MM,
  MANAGE_TAB_MAIN_PL,
  MANAGE_TAB_MAIN_SEQ,
  MANAGE_TAB_MAIN_FILEMGR,
  MANAGE_TAB_SONGLIST,
  MANAGE_TAB_SONGEDIT,
  MANAGE_TAB_STATISTICS,
};

enum {
  MANAGE_NB_MAIN,
  MANAGE_NB_SONGLIST,
  MANAGE_NB_MM,
};

enum {
  MANAGE_MENU_CB_EZ_SL_EDIT,
  MANAGE_MENU_CB_SL_LOAD,
  MANAGE_MENU_CB_SL_COPY,
  MANAGE_MENU_CB_SL_NEW,
  MANAGE_MENU_CB_SL_MIX,
  MANAGE_MENU_CB_SL_TRUNCATE,
  MANAGE_MENU_CB_SL_EXP_M3U,
  MANAGE_MENU_CB_SL_EXP_BDJ,
  MANAGE_MENU_CB_SL_IMP_M3U,
  MANAGE_MENU_CB_SL_IMP_BDJ,
  MANAGE_MENU_CB_START_EDITALL,
  MANAGE_MENU_CB_APPLY_EDITALL,
  MANAGE_MENU_CB_CANCEL_EDITALL,
  MANAGE_MENU_CB_BPM,
  MANAGE_CB_EZ_SELECT,
  MANAGE_CB_NEW_SEL_SONGSEL,
  MANAGE_CB_NEW_SEL_SONGLIST,
  MANAGE_CB_QUEUE_SL,
  MANAGE_CB_QUEUE_SL_EZ,
  MANAGE_CB_PLAY_SL,
  MANAGE_CB_PLAY_SL_EZ,
  MANAGE_CB_PLAY_MM,
  MANAGE_CB_CLOSE,
  MANAGE_CB_MAIN_NB,
  MANAGE_CB_SL_NB,
  MANAGE_CB_MM_NB,
  MANAGE_CB_EDIT,
  MANAGE_CB_SEQ_LOAD,
  MANAGE_CB_PL_LOAD,
  MANAGE_CB_SAVE,
  MANAGE_CB_MAX,
};

/* track what the mm is currently displaying */
enum {
  MANAGE_DISP_SONG_SEL,
  MANAGE_DISP_SONG_LIST,
};

/* actions for the queue process */
enum {
  MANAGE_PLAY,
  MANAGE_QUEUE,
  MANAGE_QUEUE_LAST,
};

typedef struct manage manageui_t;

typedef struct manage {
  progstate_t     *progstate;
  char            *locknm;
  conn_t          *conn;
  procutil_t      *processes [ROUTE_MAX];
  UICallback      callbacks [MANAGE_CB_MAX];
  musicdb_t       *musicdb;
  musicqidx_t     musicqPlayIdx;
  musicqidx_t     musicqManageIdx;
  dispsel_t       *dispsel;
  int             stopwaitcount;
  UIWidget        statusMsg;
  int             dbchangecount;
  int             currbpmsel;
  int             currtimesig;
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
  UIWidget        menubar;
  UIWidget        mainnotebook;
  UIWidget        slnotebook;
  UIWidget        mmnotebook;
  dbidx_t         songlistdbidx;
  dbidx_t         seldbidx;
  int             selbypass;
  /* song list ui major elements */
  uiplayer_t      *slplayer;
  uimusicq_t      *slmusicq;
  managestats_t   *slstats;
  uisongsel_t     *slsongsel;
  uimusicq_t      *slezmusicq;
  uisongsel_t     *slezsongsel;
  UIWidget        slezmusicqtabwidget;
  UIWidget        *slmusicqtabwidget;
  UIWidget        *slsongseltabwidget;
  char            *sloldname;
  uisongfilter_t  *uisongfilter;
  /* music manager ui */
  uiplayer_t      *mmplayer;
  uimusicq_t      *mmmusicq;
  uisongsel_t     *mmsongsel;
  uisongedit_t    *mmsongedit;
  int             lastdisp;
  /* sequence */
  manageseq_t     *manageseq;
  /* playlist management */
  managepl_t      *managepl;
  /* update database */
  managedb_t      *managedb;
  /* options */
  datafile_t      *optiondf;
  nlist_t         *options;
  /* various flags */
  bool            slbackupcreated : 1;
  bool            selusesonglist : 1;
  bool            inload : 1;
  bool            bpmcounterstarted : 1;
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
enum {
  MANAGEUI_DFKEY_COUNT = (sizeof (manageuidfkeys) / sizeof (datafilekey_t))
};

static bool     manageConnectingCallback (void *udata, programstate_t programState);
static bool     manageHandshakeCallback (void *udata, programstate_t programState);
static bool     manageStoppingCallback (void *udata, programstate_t programState);
static bool     manageStopWaitCallback (void *udata, programstate_t programState);
static bool     manageClosingCallback (void *udata, programstate_t programState);
static void     manageBuildUI (manageui_t *manage);
static void     manageInitializeUI (manageui_t *manage);
static int      manageMainLoop  (void *tmanage);
static int      manageProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static bool     manageCloseWin (void *udata);
static void     manageSigHandler (int sig);
/* song editor */
static void     manageSongEditMenu (manageui_t *manage);
/* song list */
static void     manageBuildUISongListEditor (manageui_t *manage);
static void     manageBuildUIMusicManager (manageui_t *manage);
static void     manageSonglistMenu (manageui_t *manage);
static bool     manageSonglistLoad (void *udata);
static bool     manageSonglistCopy (void *udata);
static bool     manageSonglistNew (void *udata);
static bool     manageSonglistTruncate (void *udata);
static void     manageSonglistLoadFile (void *udata, const char *fn);
static long     manageLoadPlaylist (void *udata, const char *fn);
static long     manageLoadSonglist (void *udata, const char *fn);
static bool     manageToggleEasySonglist (void *udata);
static void     manageSetEasySonglist (manageui_t *manage);
static void     manageSonglistSave (manageui_t *manage);
static void     manageSetSonglistName (manageui_t *manage, const char *nm);
static bool     managePlayProcessSonglist (void *udata, long dbidx, int mqidx);
static bool     managePlayProcessEasySonglist (void *udata, long dbidx, int mqidx);
static bool     managePlayProcessMusicManager (void *udata, long dbidx, int mqidx);
static bool     manageQueueProcessSonglist (void *udata, long dbidx, int mqidx);
static bool     manageQueueProcessEasySonglist (void *udata, long dbidx, int mqidx);
static void     manageQueueProcess (void *udata, long dbidx, int mqidx, int dispsel, int action);
/* general */
static bool     manageSwitchPageMain (void *udata, long pagenum);
static bool     manageSwitchPageSonglist (void *udata, long pagenum);
static bool     manageSwitchPageMM (void *udata, long pagenum);
static void     manageSwitchPage (manageui_t *manage, long pagenum, int which);
static void     manageSetDisplayPerSelection (manageui_t *manage, int id);
static void     manageSetMenuCallback (manageui_t *manage, int midx, UICallbackFunc cb);
static void     manageSonglistLoadCheck (manageui_t *manage);
/* song editor */
static bool     manageNewSelectionSonglist (void *udata, long dbidx);
static bool     manageNewSelectionSongSel (void *udata, long dbidx);
static bool     manageSwitchToSongEditor (void *udata);
static bool     manageSongEditSaveCallback (void *udata);
static bool     manageStartBPMCounter (void *udata);
static void     manageSetBPMCounter (manageui_t *manage, song_t *song);
static void     manageSendBPMCounter (manageui_t *manage);
static bool     manageSonglistExportM3U (void *udata);
static bool     manageSonglistImportM3U (void *udata);

static int gKillReceived = false;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  manageui_t      manage;
  char            *uifont;
  char            tbuff [MAXPATHLEN];


  manage.progstate = progstateInit ("manageui");
  progstateSetCallback (manage.progstate, STATE_CONNECTING,
      manageConnectingCallback, &manage);
  progstateSetCallback (manage.progstate, STATE_WAIT_HANDSHAKE,
      manageHandshakeCallback, &manage);

  uiutilsUIWidgetInit (&manage.mainnotebook);
  uiutilsUIWidgetInit (&manage.window);
  manage.slplayer = NULL;
  manage.slmusicq = NULL;
  manage.slstats = NULL;
  manage.slsongsel = NULL;
  manage.slezmusicq = NULL;
  manage.slezsongsel = NULL;
  manage.mmplayer = NULL;
  manage.mmmusicq = NULL;
  manage.mmsongsel = NULL;
  manage.mmsongedit = NULL;
  manage.musicqPlayIdx = MUSICQ_MNG_PB;
  manage.musicqManageIdx = MUSICQ_SL;
  manage.stopwaitcount = 0;
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
  manage.selusesonglist = false;
  manage.inload = false;
  manage.lastdisp = MANAGE_DISP_SONG_SEL;
  manage.selbypass = 0;
  manage.seldbidx = -1;
  manage.songlistdbidx = -1;
  manage.uisongfilter = NULL;
  manage.dbchangecount = 0;
  manage.manageseq = NULL;   /* allocated within buildui */
  manage.managepl = NULL;   /* allocated within buildui */
  manage.managedb = NULL;   /* allocated within buildui */
  manage.bpmcounterstarted = false;
  manage.currbpmsel = BPM_BPM;
  manage.currtimesig = DANCE_TIMESIG_44;

  procutilInitProcesses (manage.processes);

  osSetStandardSignals (manageSigHandler);

  bdj4startup (argc, argv, &manage.musicdb,
      "mu", ROUTE_MANAGEUI, BDJ4_INIT_NONE);
  logProcBegin (LOG_PROC, "manageui");

  manage.dispsel = dispselAlloc ();

  listenPort = bdjvarsGetNum (BDJVL_MANAGEUI_PORT);
  manage.conn = connInit (ROUTE_MANAGEUI);

  pathbldMakePath (tbuff, sizeof (tbuff),
      MANAGEUI_OPT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
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

  uiUIInitialize ();
  uifont = bdjoptGetStr (OPT_MP_UIFONT);
  uiSetUIFont (uifont);

  manageBuildUI (&manage);

  /* will be propagated */
  uisongselSetDefaultSelection (manage.slsongsel);

  /* register these after calling the sub-window initialization */
  /* then these will be run last, after the other closing callbacks */
  progstateSetCallback (manage.progstate, STATE_STOPPING,
      manageStoppingCallback, &manage);
  progstateSetCallback (manage.progstate, STATE_STOP_WAIT,
      manageStopWaitCallback, &manage);
  progstateSetCallback (manage.progstate, STATE_CLOSING,
      manageClosingCallback, &manage);

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

  if (manage->bpmcounterstarted > 0) {
    procutilStopProcess (manage->processes [ROUTE_BPM_COUNTER],
        manage->conn, ROUTE_BPM_COUNTER, false);
    manage->bpmcounterstarted = false;
  }

  manageSonglistSave (manage);
  manageSequenceSave (manage->manageseq);
  managePlaylistSave (manage->managepl);

  uiWindowGetSize (&manage->window, &x, &y);
  nlistSetNum (manage->options, PLUI_SIZE_X, x);
  nlistSetNum (manage->options, PLUI_SIZE_Y, y);
  uiWindowGetPosition (&manage->window, &x, &y);
  nlistSetNum (manage->options, PLUI_POSITION_X, x);
  nlistSetNum (manage->options, PLUI_POSITION_Y, y);

  connDisconnect (manage->conn, ROUTE_STARTERUI);

  procutilStopAllProcess (manage->processes, manage->conn, false);

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

  manageDbClose (manage->managedb);
  uiCloseWindow (&manage->window);

  procutilStopAllProcess (manage->processes, manage->conn, true);
  procutilFreeAll (manage->processes);

  pathbldMakePath (fn, sizeof (fn),
      MANAGEUI_OPT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("manageui", fn, manageuidfkeys, MANAGEUI_DFKEY_COUNT, manage->options);

  bdj4shutdown (ROUTE_MANAGEUI, manage->musicdb);
  manageSequenceFree (manage->manageseq);
  managePlaylistFree (manage->managepl);
  manageDbFree (manage->managedb);

  if (manage->uisongfilter != NULL) {
    uisfFree (manage->uisongfilter);
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

  uiplayerFree (manage->slplayer);
  uimusicqFree (manage->slmusicq);
  manageStatsFree (manage->slstats);
  uisongselFree (manage->slsongsel);

  uimusicqFree (manage->slezmusicq);
  uisongselFree (manage->slezsongsel);

  uiplayerFree (manage->mmplayer);
  uimusicqFree (manage->mmmusicq);
  uisongselFree (manage->mmsongsel);
  uisongeditFree (manage->mmsongedit);

  dispselFree (manage->dispsel);
  datafileFree (manage->optiondf);

  uiCleanup ();

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
  /* CONTEXT: managementui: management user interface window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Management"),
      bdjoptGetStr (OPT_P_PROFILENAME));

  uiutilsUICallbackInit (&manage->callbacks [MANAGE_CB_CLOSE],
      manageCloseWin, manage);
  uiCreateMainWindow (&manage->window,
      &manage->callbacks [MANAGE_CB_CLOSE], tbuff, imgbuff);

  manageInitializeUI (manage);

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, 4);
  uiBoxPackInWindow (&manage->window, &vbox);

  uiCreateHorizBox (&hbox);
  uiWidgetSetMarginTop (&hbox, uiBaseMarginSz * 4);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateLabel (&uiwidget, "");
  uiWidgetSetSizeRequest (&uiwidget, 25, 25);
  uiWidgetSetMarginStart (&uiwidget, uiBaseMarginSz * 3);
  uiLabelSetBackgroundColor (&uiwidget, bdjoptGetStr (OPT_P_UI_PROFILE_COL));
  uiBoxPackEnd (&hbox, &uiwidget);

  uiCreateLabel (&uiwidget, "");
  uiLabelSetColor (&uiwidget, bdjoptGetStr (OPT_P_UI_ERROR_COL));
  uiBoxPackEnd (&hbox, &uiwidget);
  uiutilsUIWidgetCopy (&manage->statusMsg, &uiwidget);

  uiCreateMenubar (&manage->menubar);
  uiBoxPackStart (&hbox, &manage->menubar);

  uiCreateNotebook (&manage->mainnotebook);
  uiNotebookTabPositionLeft (&manage->mainnotebook);
  uiBoxPackStartExpand (&vbox, &manage->mainnotebook);

  manageBuildUISongListEditor (manage);
  manageBuildUIMusicManager (manage);

  /* update database */
  manage->managedb = manageDbAlloc (&manage->window,
      manage->options, &manage->statusMsg, manage->conn);

  uiCreateVertBox (&vbox);
  manageBuildUIUpdateDatabase (manage->managedb, &vbox);
  /* CONTEXT: managementui: notebook tab title: update database */
  uiCreateLabel (&uiwidget, _("Update Database"));
  uiNotebookAppendPage (&manage->mainnotebook, &vbox, &uiwidget);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_OTHER);

  /* playlist management */
  manage->managepl = managePlaylistAlloc (&manage->window,
      manage->options, &manage->statusMsg);

  uiCreateVertBox (&vbox);
  manageBuildUIPlaylist (manage->managepl, &vbox);
  /* CONTEXT: managementui: notebook tab title: playlist management */
  uiCreateLabel (&uiwidget, _("Playlist Management"));
  uiNotebookAppendPage (&manage->mainnotebook, &vbox, &uiwidget);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_MAIN_PL);

  /* sequence editor */
  manage->manageseq = manageSequenceAlloc (&manage->window,
      manage->options, &manage->statusMsg);

  uiCreateVertBox (&vbox);
  manageBuildUISequence (manage->manageseq, &vbox);
  /* CONTEXT: managementui: notebook tab title: edit sequences */
  uiCreateLabel (&uiwidget, _("Edit Sequences"));
  uiNotebookAppendPage (&manage->mainnotebook, &vbox, &uiwidget);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_MAIN_SEQ);

  /* file manager */
  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, uiBaseMarginSz * 2);
  /* CONTEXT: managementui: notebook tab title: file manager */
  uiCreateLabel (&uiwidget, _("File Manager"));
  uiNotebookAppendPage (&manage->mainnotebook, &vbox, &uiwidget);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_MAIN_FILEMGR);

  x = nlistGetNum (manage->options, PLUI_SIZE_X);
  y = nlistGetNum (manage->options, PLUI_SIZE_Y);
  uiWindowSetDefaultSize (&manage->window, x, y);

  uiutilsUICallbackLongInit (&manage->callbacks [MANAGE_CB_MAIN_NB],
      manageSwitchPageMain, manage);
  uiNotebookSetCallback (&manage->mainnotebook,
      &manage->callbacks [MANAGE_CB_MAIN_NB]);

  uiWidgetShowAll (&manage->window);

  x = nlistGetNum (manage->options, PLUI_POSITION_X);
  y = nlistGetNum (manage->options, PLUI_POSITION_Y);
  uiWindowMove (&manage->window, x, y);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", ".png", PATHBLD_MP_IMGDIR);
  osuiSetIcon (imgbuff);

  uiutilsUICallbackLongInit (&manage->callbacks [MANAGE_CB_NEW_SEL_SONGSEL],
      manageNewSelectionSongSel, manage);
  uisongselSetSelectionCallback (manage->slezsongsel,
      &manage->callbacks [MANAGE_CB_NEW_SEL_SONGSEL]);
  uisongselSetSelectionCallback (manage->slsongsel,
      &manage->callbacks [MANAGE_CB_NEW_SEL_SONGSEL]);
  uisongselSetSelectionCallback (manage->mmsongsel,
      &manage->callbacks [MANAGE_CB_NEW_SEL_SONGSEL]);

  uiutilsUICallbackLongInit (&manage->callbacks [MANAGE_CB_NEW_SEL_SONGLIST],
      manageNewSelectionSonglist, manage);
  uimusicqSetSelectionCallback (manage->slmusicq,
      &manage->callbacks [MANAGE_CB_NEW_SEL_SONGLIST]);
  uimusicqSetSelectionCallback (manage->slezmusicq,
      &manage->callbacks [MANAGE_CB_NEW_SEL_SONGLIST]);

  uiutilsUICallbackStrInit (&manage->callbacks [MANAGE_CB_SEQ_LOAD],
      manageLoadPlaylist, manage);
  manageSequenceSetLoadCallback (manage->manageseq,
      &manage->callbacks [MANAGE_CB_SEQ_LOAD]);

  uiutilsUICallbackStrInit (&manage->callbacks [MANAGE_CB_PL_LOAD],
      manageLoadSonglist, manage);
  managePlaylistSetLoadCallback (manage->managepl,
      &manage->callbacks [MANAGE_CB_PL_LOAD]);

  logProcEnd (LOG_PROC, "manageBuildUI", "");
}

static void
manageInitializeUI (manageui_t *manage)
{
  manage->uisongfilter = uisfInit (&manage->window, manage->options,
      SONG_FILTER_FOR_SELECTION);

  manage->slplayer = uiplayerInit (manage->progstate, manage->conn,
      manage->musicdb);
  manage->slmusicq = uimusicqInit ("m-songlist", manage->conn,
      manage->musicdb, manage->dispsel, DISP_SEL_SONGLIST);
  uimusicqSetPlayIdx (manage->slmusicq, manage->musicqPlayIdx);
  uimusicqSetManageIdx (manage->slmusicq, manage->musicqManageIdx);
  manage->slstats = manageStatsInit (manage->conn, manage->musicdb);
  manage->slsongsel = uisongselInit ("m-sl-songsel", manage->conn,
      manage->musicdb, manage->dispsel, manage->options,
      manage->uisongfilter, DISP_SEL_SONGSEL);
  uiutilsUICallbackLongIntInit (&manage->callbacks [MANAGE_CB_PLAY_SL],
      managePlayProcessSonglist, manage);
  uisongselSetPlayCallback (manage->slsongsel,
      &manage->callbacks [MANAGE_CB_PLAY_SL]);
  uiutilsUICallbackLongIntInit (&manage->callbacks [MANAGE_CB_QUEUE_SL],
      manageQueueProcessSonglist, manage);
  uisongselSetQueueCallback (manage->slsongsel,
      &manage->callbacks [MANAGE_CB_QUEUE_SL]);

  manage->slezmusicq = uimusicqInit ("m-ez-songlist", manage->conn,
      manage->musicdb, manage->dispsel, DISP_SEL_EZSONGLIST);
  manage->slezsongsel = uisongselInit ("m-ez-songsel", manage->conn,
      manage->musicdb, manage->dispsel, manage->options,
      manage->uisongfilter, DISP_SEL_EZSONGSEL);
  uimusicqSetPlayIdx (manage->slezmusicq, manage->musicqPlayIdx);
  uimusicqSetManageIdx (manage->slezmusicq, manage->musicqManageIdx);
  uiutilsUICallbackLongIntInit (&manage->callbacks [MANAGE_CB_PLAY_SL_EZ],
      managePlayProcessEasySonglist, manage);
  uisongselSetPlayCallback (manage->slezsongsel,
      &manage->callbacks [MANAGE_CB_PLAY_SL_EZ]);
  uiutilsUICallbackLongIntInit (&manage->callbacks [MANAGE_CB_QUEUE_SL_EZ],
      manageQueueProcessEasySonglist, manage);
  uisongselSetQueueCallback (manage->slezsongsel,
      &manage->callbacks [MANAGE_CB_QUEUE_SL_EZ]);

  manage->mmplayer = uiplayerInit (manage->progstate, manage->conn,
      manage->musicdb);
  manage->mmmusicq = uimusicqInit ("m-mm-songlist", manage->conn,
      manage->musicdb, manage->dispsel, DISP_SEL_SONGLIST);
  manage->mmsongsel = uisongselInit ("m-mm-songsel", manage->conn,
      manage->musicdb, manage->dispsel, manage->options,
      manage->uisongfilter, DISP_SEL_MM);
  uimusicqSetPlayIdx (manage->mmmusicq, manage->musicqPlayIdx);
  uimusicqSetManageIdx (manage->mmmusicq, manage->musicqManageIdx);
  uiutilsUICallbackLongIntInit (&manage->callbacks [MANAGE_CB_PLAY_MM],
      managePlayProcessMusicManager, manage);
  uisongselSetPlayCallback (manage->mmsongsel,
      &manage->callbacks [MANAGE_CB_PLAY_MM]);

  manage->mmsongedit = uisongeditInit (manage->conn,
      manage->musicdb, manage->dispsel, manage->options);

  uisongselSetPeer (manage->mmsongsel, manage->slezsongsel);
  uisongselSetPeer (manage->mmsongsel, manage->slsongsel);

  uisongselSetPeer (manage->slsongsel, manage->slezsongsel);
  uisongselSetPeer (manage->slsongsel, manage->mmsongsel);

  uisongselSetPeer (manage->slezsongsel, manage->mmsongsel);
  uisongselSetPeer (manage->slezsongsel, manage->slsongsel);

  uimusicqSetPeer (manage->slmusicq, manage->slezmusicq);
  uimusicqSetPeer (manage->slezmusicq, manage->slmusicq);

  uiutilsUICallbackInit (&manage->callbacks [MANAGE_CB_EDIT],
      manageSwitchToSongEditor, manage);
  uisongselSetEditCallback (manage->slsongsel, &manage->callbacks [MANAGE_CB_EDIT]);
  uisongselSetEditCallback (manage->slezsongsel, &manage->callbacks [MANAGE_CB_EDIT]);
  uisongselSetEditCallback (manage->mmsongsel, &manage->callbacks [MANAGE_CB_EDIT]);
  uimusicqSetEditCallback (manage->slmusicq, &manage->callbacks [MANAGE_CB_EDIT]);
  uimusicqSetEditCallback (manage->slezmusicq, &manage->callbacks [MANAGE_CB_EDIT]);

  uiutilsUICallbackInit (&manage->callbacks [MANAGE_CB_SAVE],
      manageSongEditSaveCallback, manage);
  uisongeditSetSaveCallback (manage->mmsongedit, &manage->callbacks [MANAGE_CB_SAVE]);
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

  /* CONTEXT: managementui: notebook tab title: edit song lists */
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

  /* CONTEXT: managementui: name of easy song list song selection tab */
  uiCreateLabel (&uiwidget, _("Song List"));
  uiNotebookAppendPage (&notebook, &mainhbox, &uiwidget);
  uiutilsNotebookIDAdd (manage->slnbtabid, MANAGE_TAB_SONGLIST);
  uiutilsUIWidgetCopy (&manage->slezmusicqtabwidget, &mainhbox);

  uiCreateHorizBox (&hbox);
  uiBoxPackStartExpand (&mainhbox, &hbox);

  uiwidgetp = uimusicqBuildUI (manage->slezmusicq, &manage->window, MUSICQ_SL,
      &manage->statusMsg, manageValidateName);
  uiBoxPackStartExpand (&hbox, uiwidgetp);

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, uiBaseMarginSz * 4);
  uiWidgetSetMarginTop (&vbox, uiBaseMarginSz * 64);
  uiBoxPackStart (&hbox, &vbox);

  uiutilsUICallbackInit (&manage->callbacks [MANAGE_CB_EZ_SELECT],
      uisongselSelectCallback, manage->slezsongsel);
  uiCreateButton (&uiwidget,
      &manage->callbacks [MANAGE_CB_EZ_SELECT],
      /* CONTEXT: managementui: config: button: add the selected songs to the song list */
      _("Select"), "button_left");
  uiBoxPackStart (&vbox, &uiwidget);

  uiwidgetp = uisongselBuildUI (manage->slezsongsel, &manage->window);
  uiBoxPackStartExpand (&hbox, uiwidgetp);

  /* song list editor: music queue tab */
  uiwidgetp = uimusicqBuildUI (manage->slmusicq, &manage->window, MUSICQ_SL,
      &manage->statusMsg, manageValidateName);
  /* CONTEXT: managementui: name of song list notebook tab */
  uiCreateLabel (&uiwidget, _("Song List"));
  uiNotebookAppendPage (&notebook, uiwidgetp, &uiwidget);
  uiutilsNotebookIDAdd (manage->slnbtabid, MANAGE_TAB_SONGLIST);
  manage->slmusicqtabwidget = uiwidgetp;

  /* song list editor: song selection tab*/
  uiwidgetp = uisongselBuildUI (manage->slsongsel, &manage->window);
  /* CONTEXT: managementui: name of song selection notebook tab */
  uiCreateLabel (&uiwidget, _("Song Selection"));
  uiNotebookAppendPage (&notebook, uiwidgetp, &uiwidget);
  uiutilsNotebookIDAdd (manage->slnbtabid, MANAGE_TAB_OTHER);
  manage->slsongseltabwidget = uiwidgetp;

  uimusicqPeerSonglistName (manage->slmusicq, manage->slezmusicq);

  /* song list editor: statistics tab */
  uiwidgetp = manageBuildUIStats (manage->slstats);
  /* CONTEXT: managementui: name of statistics tab */
  uiCreateLabel (&uiwidget, _("Statistics"));
  uiNotebookAppendPage (&notebook, uiwidgetp, &uiwidget);
  uiutilsNotebookIDAdd (manage->slnbtabid, MANAGE_TAB_STATISTICS);

  uiutilsUICallbackLongInit (&manage->callbacks [MANAGE_CB_SL_NB],
      manageSwitchPageSonglist, manage);
  uiNotebookSetCallback (&notebook, &manage->callbacks [MANAGE_CB_SL_NB]);
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
  /* CONTEXT: managementui: name of music manager notebook tab */
  uiCreateLabel (&uiwidget, _("Music Manager"));
  uiNotebookAppendPage (&manage->mainnotebook, &vbox, &uiwidget);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_MAIN_MM);

  /* music manager: player */
  uiwidgetp = uiplayerBuildUI (manage->mmplayer);
  uiBoxPackStart (&vbox, uiwidgetp);

  uiCreateNotebook (&notebook);
  uiBoxPackStartExpand (&vbox, &notebook);
  uiutilsUIWidgetCopy (&manage->mmnotebook, &notebook);

  /* music manager: song selection tab*/
  uiwidgetp = uisongselBuildUI (manage->mmsongsel, &manage->window);
  uiWidgetExpandHoriz (uiwidgetp);
  /* CONTEXT: managementui: name of song selection notebook tab */
  uiCreateLabel (&uiwidget, _("Music Manager"));
  uiNotebookAppendPage (&notebook, uiwidgetp, &uiwidget);
  uiutilsNotebookIDAdd (manage->mmnbtabid, MANAGE_TAB_OTHER);

  /* music manager: song editor tab */
  uiwidgetp = uisongeditBuildUI (manage->mmsongsel, manage->mmsongedit, &manage->window, &manage->statusMsg);
  /* CONTEXT: managementui: name of song editor notebook tab */
  uiCreateLabel (&uiwidget, _("Song Editor"));
  uiNotebookAppendPage (&notebook, uiwidgetp, &uiwidget);
  uiutilsNotebookIDAdd (manage->mmnbtabid, MANAGE_TAB_SONGEDIT);

  uiutilsUICallbackLongInit (&manage->callbacks [MANAGE_CB_MM_NB],
      manageSwitchPageMM, manage);
  uiNotebookSetCallback (&notebook, &manage->callbacks [MANAGE_CB_MM_NB]);
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
      gKillReceived = false;
    }
    return stop;
  }

  connProcessUnconnected (manage->conn);

  if (connHaveHandshake (manage->conn, ROUTE_BPM_COUNTER)) {
    if (! manage->bpmcounterstarted) {
      manage->bpmcounterstarted = true;
      manageSendBPMCounter (manage);
    }
  }

  uiplayerMainLoop (manage->slplayer);
  uiplayerMainLoop (manage->mmplayer);
  uimusicqMainLoop (manage->slmusicq);
  uisongselMainLoop (manage->slsongsel);
  uimusicqMainLoop (manage->slezmusicq);
  uisongselMainLoop (manage->slezsongsel);
  uimusicqMainLoop (manage->mmmusicq);
  uisongselMainLoop (manage->mmsongsel);

  if (manage->mainlasttab == MANAGE_TAB_MAIN_MM) {
    if (manage->mmlasttab == MANAGE_TAB_SONGEDIT) {
      /* the song edit main loop does not need to run all the time */
      uisongeditMainLoop (manage->mmsongedit);
    }
  }

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (manage->progstate);
    gKillReceived = false;
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
    char  tmp [40];

    connSendMessage (manage->conn, ROUTE_MAIN, MSG_QUEUE_PLAY_ON_ADD, "1");
    snprintf (tmp, sizeof (tmp), "%d", MUSICQ_MNG_PB);
    connSendMessage (manage->conn, ROUTE_MAIN, MSG_MUSICQ_SET_PLAYBACK, tmp);
    connSendMessage (manage->conn, ROUTE_MAIN, MSG_QUEUE_SWITCH_EMPTY, "0");
    connSendMessage (manage->conn, ROUTE_MAIN, MSG_MUSICQ_SET_LEN, "999");
    progstateLogTime (manage->progstate, "time-to-start-gui");
    manageDbChg (manage->managedb);
    manageSetEasySonglist (manage);
    /* CONTEXT: managementui: song list: default name for a new song list */
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

  targs = strdup (args);

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
          if (routefrom == ROUTE_BPM_COUNTER) {
            manage->bpmcounterstarted = false;
          }
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = false;
          progstateShutdownProcess (manage->progstate);
          break;
        }
        case MSG_DB_PROGRESS: {
          manageDbProgressMsg (manage->managedb, targs);
          break;
        }
        case MSG_DB_STATUS_MSG: {
          manageDbStatusMsg (manage->managedb, targs);
          break;
        }
        case MSG_DB_FINISH: {
          manageDbFinish (manage->managedb, routefrom);

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

  free (targs);

  /* due to the db update message, these must be applied afterwards */
  uiplayerProcessMsg (routefrom, route, msg, args, manage->slplayer);
  uimusicqProcessMsg (routefrom, route, msg, args, manage->slmusicq);
  manageStatsProcessMsg (routefrom, route, msg, args, manage->slstats);
  uisongselProcessMsg (routefrom, route, msg, args, manage->slsongsel);
  uisongselProcessMsg (routefrom, route, msg, args, manage->slezsongsel);
  uimusicqProcessMsg (routefrom, route, msg, args, manage->slezmusicq);
  uiplayerProcessMsg (routefrom, route, msg, args, manage->mmplayer);
  uisongselProcessMsg (routefrom, route, msg, args, manage->mmsongsel);
  uimusicqProcessMsg (routefrom, route, msg, args, manage->mmmusicq);
  uisongeditProcessMsg (routefrom, route, msg, args, manage->mmsongedit);

  /* reset the bypass flag after a music queue update has been received */
  if (msg == MSG_MUSIC_QUEUE_DATA) {
    if (manage->selbypass) {
      --manage->selbypass;
    }
  }

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

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: close window");
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
  gKillReceived = true;
}

/* song editor */

static void
manageSongEditMenu (manageui_t *manage)
{
  UIWidget  menu;
  UIWidget  menuitem;

  if (! manage->songeditmenu.initialized) {
    uiMenuAddMainItem (&manage->menubar, &menuitem,
        /* CONTEXT: managementui: menu selection: actions for song editor */
        &manage->songeditmenu, _("Actions"));

    uiCreateSubMenu (&menuitem, &menu);

    /* I would prefer to have BPM as a stand-alone menu item, but */
    /* gtk does not appear to have a way to create a top-level */
    /* menu item in the menu bar. */
    uiutilsUICallbackInit (&manage->callbacks [MANAGE_MENU_CB_BPM],
        manageStartBPMCounter, manage);
    uiMenuCreateItem (&menu, &menuitem, tagdefs [TAG_BPM].displayname,
        &manage->callbacks [MANAGE_MENU_CB_BPM]);

    /* CONTEXT: managementui: menu selection: song editor: edit all */
    uiMenuCreateItem (&menu, &menuitem, _("Edit All"), NULL);
    uiWidgetDisable (&menuitem);

    /* CONTEXT: managementui: menu selection: song editor: apply edit all */
    uiMenuCreateItem (&menu, &menuitem, _("Apply Edit All"), NULL);
    uiWidgetDisable (&menuitem);

    /* CONTEXT: managementui: menu selection: song editor: cancel edit all */
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
        /* CONTEXT: managementui: menu selection: song list: options menu */
        &manage->slmenu, _("Options"));

    uiCreateSubMenu (&menuitem, &menu);

    manageSetMenuCallback (manage, MANAGE_MENU_CB_EZ_SL_EDIT,
        manageToggleEasySonglist);
    /* CONTEXT: managementui: menu checkbox: easy song list editor */
    uiMenuCreateCheckbox (&menu, &menuitem, _("Easy Song List Editor"),
        nlistGetNum (manage->options, MANAGE_EASY_SONGLIST),
        &manage->callbacks [MANAGE_MENU_CB_EZ_SL_EDIT]);

    uiMenuAddMainItem (&manage->menubar, &menuitem,
        /* CONTEXT: managementui: menu selection: song list: edit menu */
        &manage->slmenu, _("Edit"));

    uiCreateSubMenu (&menuitem, &menu);

    manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_LOAD, manageSonglistLoad);
    /* CONTEXT: managementui: menu selection: song list: edit menu: load */
    uiMenuCreateItem (&menu, &menuitem, _("Load"),
        &manage->callbacks [MANAGE_MENU_CB_SL_LOAD]);

    manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_COPY, manageSonglistCopy);
    /* CONTEXT: managementui: menu selection: song list: edit menu: create copy */
    uiMenuCreateItem (&menu, &menuitem, _("Create Copy"),
        &manage->callbacks [MANAGE_MENU_CB_SL_COPY]);

    manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_NEW, manageSonglistNew);
    /* CONTEXT: managementui: menu selection: song list: edit menu: start new song list */
    uiMenuCreateItem (&menu, &menuitem, _("Start New Song List"),
        &manage->callbacks [MANAGE_MENU_CB_SL_NEW]);

    uiMenuAddMainItem (&manage->menubar, &menuitem,
        /* CONTEXT: managementui: menu selection: actions for song list */
        &manage->slmenu, _("Actions"));

    uiCreateSubMenu (&menuitem, &menu);

    /* CONTEXT: managementui: menu selection: song list: actions menu: rearrange the songs and create a new mix */
    uiMenuCreateItem (&menu, &menuitem, _("Mix"), NULL);
    uiWidgetDisable (&menuitem);

    manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_TRUNCATE,
        manageSonglistTruncate);
    /* CONTEXT: managementui: menu selection: song list: actions menu: truncate the song list */
    uiMenuCreateItem (&menu, &menuitem, _("Truncate"),
        &manage->callbacks [MANAGE_MENU_CB_SL_TRUNCATE]);

    uiMenuAddMainItem (&manage->menubar, &menuitem,
        /* CONTEXT: managementui: menu selection: export actions for song list */
        &manage->slmenu, _("Export"));

    uiCreateSubMenu (&menuitem, &menu);

    manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_EXP_M3U,
        manageSonglistExportM3U);
    /* CONTEXT: managementui: menu selection: song list: export: export as m3u */
    uiMenuCreateItem (&menu, &menuitem, _("Export as M3U Playlist"),
        &manage->callbacks [MANAGE_MENU_CB_SL_EXP_M3U]);

    /* CONTEXT: managementui: menu selection: song list: export: export for ballroomdj */
    snprintf (tbuff, sizeof (tbuff), _("Export for %s"), BDJ4_NAME);
    uiMenuCreateItem (&menu, &menuitem, tbuff, NULL);
    uiWidgetDisable (&menuitem);

    uiMenuAddMainItem (&manage->menubar, &menuitem,
        /* CONTEXT: managementui: menu selection: import actions for song list */
        &manage->slmenu, _("Import"));

    uiCreateSubMenu (&menuitem, &menu);

    /* CONTEXT: managementui: menu selection: song list: import: import m3u */
    manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_IMP_M3U,
        manageSonglistImportM3U);
    uiMenuCreateItem (&menu, &menuitem, _("Import M3U"),
        &manage->callbacks [MANAGE_MENU_CB_SL_IMP_M3U]);

    /* CONTEXT: managementui: menu selection: song list: import: import from ballroomdj */
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

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: load songlist");
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

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: copy songlist");
  manageSonglistSave (manage);

  oname = uimusicqGetSonglistName (manage->slmusicq);
  /* CONTEXT: managementui: the new name after 'create copy' (e.g. "Copy of DJ-2022-04") */
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

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: new songlist");
  manageSonglistSave (manage);

  /* CONTEXT: managementui: song list: default name for a new song list */
  snprintf (tbuff, sizeof (tbuff), _("New Song List"));
  manageSetSonglistName (manage, tbuff);
  manage->slbackupcreated = false;
  uimusicqSetSelectionFirst (manage->slmusicq, manage->musicqManageIdx);
  uimusicqClearQueueCallback (manage->slmusicq);
  return UICB_CONT;
}

static bool
manageSonglistTruncate (void *udata)
{
  manageui_t  *manage = udata;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: truncate songlist");
  uimusicqClearQueueCallback (manage->slmusicq);
  return UICB_CONT;
}

static void
manageSonglistLoadFile (void *udata, const char *fn)
{
  manageui_t  *manage = udata;
  char  tbuff [200];

  if (manage->inload) {
    return;
  }
  manage->inload = true;

  /* any selection made by the load process should not trigger */
  /* a change in the song editor */
  /* the next two updates received from main will decrement the bypass flag */
  manage->selusesonglist = false;
  /* two messages are received in response to the changes below */
  /* (clear queue and queue playlist) */
  /* this is gross */
  manage->selbypass = 2;

  manageSonglistSave (manage);

  /* truncate from the first selection */
  uimusicqSetSelectionFirst (manage->slmusicq, manage->musicqManageIdx);
  uimusicqClearQueueCallback (manage->slmusicq);

  snprintf (tbuff, sizeof (tbuff), "%d%c%s",
      manage->musicqManageIdx, MSG_ARGS_RS, fn);
  connSendMessage (manage->conn, ROUTE_MAIN, MSG_QUEUE_PLAYLIST, tbuff);

  manageSetSonglistName (manage, fn);
  manageLoadPlaylist (manage, fn);
  manage->slbackupcreated = false;
  manage->inload = false;
}

/* callback to load playlist upon songlist/sequence load */
static long
manageLoadPlaylist (void *udata, const char *fn)
{
  manageui_t    *manage = udata;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: seq: load playlist");
  managePlaylistLoadFile (manage->managepl, fn);
  return UICB_CONT;
}

/* callback to load upon playlist load */
static long
manageLoadSonglist (void *udata, const char *fn)
{
  manageui_t    *manage = udata;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: pl: load songlist");
  /* the load will save any current song list */
  manageSonglistLoadFile (manage, fn);
  return UICB_CONT;
}

static bool
manageToggleEasySonglist (void *udata)
{
  manageui_t  *manage = udata;
  int         val;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: toggle ez songlist");
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
  if (manage->sloldname != NULL) {
    free (manage->sloldname);
  }
  manage->sloldname = strdup (nm);
  uimusicqSetSonglistName (manage->slmusicq, nm);
}

static void
manageSonglistSave (manageui_t *manage)
{
  char  *name;
  char  nnm [MAXPATHLEN];

  if (manage == NULL) {
    return;
  }
  if (manage->sloldname == NULL) {
    return;
  }
  if (manage->slmusicq == NULL) {
    return;
  }

  if (uimusicqGetCount (manage->slmusicq) <= 0) {
    return;
  }

  name = strdup (uimusicqGetSonglistName (manage->slmusicq));

  /* the song list has been renamed */
  if (strcmp (manage->sloldname, name) != 0) {
    manageRenamePlaylistFiles (manage->sloldname, name);
  }

  /* need the full name for backups */
  pathbldMakePath (nnm, sizeof (nnm),
      name, BDJ4_SONGLIST_EXT, PATHBLD_MP_DATA);
  if (! manage->slbackupcreated) {
    filemanipBackup (nnm, 1);
    manage->slbackupcreated = true;
  }

  manageSetSonglistName (manage, name);
  uimusicqSave (manage->slmusicq, name);
  manageCheckAndCreatePlaylist (name, PLTYPE_SONGLIST);
  free (name);
}

static bool
managePlayProcessSonglist (void *udata, long dbidx, int mqidx)
{
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: play from songlist");
  manageQueueProcess (udata, dbidx, mqidx, DISP_SEL_SONGLIST, MANAGE_PLAY);
  return UICB_CONT;
}

static bool
managePlayProcessEasySonglist (void *udata, long dbidx, int mqidx)
{
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: play from ez songlist");
  manageQueueProcess (udata, dbidx, mqidx, DISP_SEL_EZSONGLIST, MANAGE_PLAY);
  return UICB_CONT;
}

static bool
managePlayProcessMusicManager (void *udata, long dbidx, int mqidx)
{
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: play from mm");
  manageQueueProcess (udata, dbidx, mqidx, DISP_SEL_MM, MANAGE_PLAY);
  return UICB_CONT;
}

static bool
manageQueueProcessSonglist (void *udata, long dbidx, int mqidx)
{
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: queue to songlist");
  manageQueueProcess (udata, dbidx, mqidx, DISP_SEL_SONGLIST, MANAGE_QUEUE);
  return UICB_CONT;
}

static bool
manageQueueProcessEasySonglist (void *udata, long dbidx, int mqidx)
{
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: queue to ez songlist");
  manageQueueProcess (udata, dbidx, mqidx, DISP_SEL_EZSONGLIST, MANAGE_QUEUE);
  return UICB_CONT;
}

static void
manageQueueProcess (void *udata, long dbidx, int mqidx, int dispsel, int action)
{
  manageui_t  *manage = udata;
  char        tbuff [100];
  long        loc = 999;
  uimusicq_t  *uimusicq = NULL;

  if (dispsel == DISP_SEL_SONGLIST) {
    uimusicq = manage->slmusicq;
  }
  if (dispsel == DISP_SEL_EZSONGLIST) {
    uimusicq = manage->slezmusicq;
  }
  if (dispsel == DISP_SEL_MM) {
    uimusicq = manage->mmmusicq;
  }

  if (action == MANAGE_QUEUE) {
    /* on a queue action, queue after the current selection */
    loc = uimusicqGetSelectLocation (uimusicq, mqidx);
    if (loc < 0) {
      loc = 999;
    }
  }

  if (action == MANAGE_QUEUE_LAST) {
    action = MANAGE_QUEUE;
    loc = 999;
  }

  if (action == MANAGE_QUEUE) {
    snprintf (tbuff, sizeof (tbuff), "%d%c%ld%c%ld", mqidx,
        MSG_ARGS_RS, loc, MSG_ARGS_RS, dbidx);
    connSendMessage (manage->conn, ROUTE_MAIN, MSG_MUSICQ_INSERT, tbuff);
  }

  if (action == MANAGE_PLAY) {
    snprintf (tbuff, sizeof (tbuff), "%d%c%d%c%ld", mqidx,
        MSG_ARGS_RS, 999, MSG_ARGS_RS, dbidx);
    connSendMessage (manage->conn, ROUTE_MAIN, MSG_QUEUE_CLEAR_PLAY, tbuff);
  }
}

/* general */

static bool
manageSwitchPageMain (void *udata, long pagenum)
{
  manageui_t  *manage = udata;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: switch main nb page");
  manageSwitchPage (manage, pagenum, MANAGE_NB_MAIN);
  return UICB_CONT;
}

static bool
manageSwitchPageSonglist (void *udata, long pagenum)
{
  manageui_t  *manage = udata;

  manageSwitchPage (manage, pagenum, MANAGE_NB_SONGLIST);
  return UICB_CONT;
}

static bool
manageSwitchPageMM (void *udata, long pagenum)
{
  manageui_t  *manage = udata;

  manageSwitchPage (manage, pagenum, MANAGE_NB_MM);
  return UICB_CONT;
}

static void
manageSwitchPage (manageui_t *manage, long pagenum, int which)
{
  int         id;
  bool        mainnb = false;
  bool        slnb = false;
  bool        mmnb = false;
  uiutilsnbtabid_t  *nbtabid = NULL;

  /* need to know which notebook is selected so that the correct id value */
  /* can be retrieved */
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

  if (mainnb) {
    if (manage->mainlasttab == MANAGE_TAB_MAIN_SL) {
      manageSonglistSave (manage);
    }
    if (manage->mainlasttab == MANAGE_TAB_MAIN_SEQ) {
      manageSequenceSave (manage->manageseq);
    }
    if (manage->mainlasttab == MANAGE_TAB_MAIN_PL) {
      managePlaylistSave (manage->managepl);
    }
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

  switch (id) {
    case MANAGE_TAB_MAIN_SL: {
      manageSetDisplayPerSelection (manage, id);
      manageSonglistLoadCheck (manage);
      break;
    }
    case MANAGE_TAB_MAIN_MM: {
      manageSetDisplayPerSelection (manage, id);
      break;
    }
    case MANAGE_TAB_MAIN_PL: {
      managePlaylistLoadCheck (manage->managepl);
      manage->currmenu = managePlaylistMenu (manage->managepl, &manage->menubar);
      break;
    }
    case MANAGE_TAB_MAIN_SEQ: {
      manageSequenceLoadCheck (manage->manageseq);
      manage->currmenu = manageSequenceMenu (manage->manageseq, &manage->menubar);
      break;
    }
    case MANAGE_TAB_MAIN_FILEMGR: {
      break;
    }
    default: {
      /* do nothing (other), (songlist and songedit handled below) */
      break;
    }
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
    case MANAGE_TAB_SONGLIST: {
      manageSonglistMenu (manage);
      break;
    }
    case MANAGE_TAB_SONGEDIT: {
      manageSongEditMenu (manage);
      break;
    }
    default: {
      break;
    }
  }
}

/* when switching pages, the music manager must be adjusted to */
/* display the song selection list or the playlist depending on */
/* what was last selected. the song selection lists never display */
/* the playlist.  This is all very messy, but makes the song editor */
/* easier to use and more intuitive. */
static void
manageSetDisplayPerSelection (manageui_t *manage, int id)
{
  dbidx_t     dbidx;

  if (id == MANAGE_TAB_MAIN_SL) {
    if (uisfPlaylistInUse (manage->uisongfilter) ||
        manage->lastdisp == MANAGE_DISP_SONG_LIST) {
      long        loc;

      loc = uisongselGetSelectLocation (manage->mmsongsel);
      uisfClearPlaylist (manage->uisongfilter);
      manage->selbypass = 1;
      uisongselApplySongFilter (manage->mmsongsel);
      manage->selbypass = 0;
      uisongselRestoreSelections (manage->mmsongsel);
      if (manage->selusesonglist) {
        uimusicqSetSelectLocation (manage->slmusicq, manage->musicqManageIdx, loc);
      }
      manage->selusesonglist = false;
      manage->lastdisp = MANAGE_DISP_SONG_SEL;
    }

    uisfHidePlaylistDisplay (manage->uisongfilter);
  }

  if (id == MANAGE_TAB_MAIN_MM) {
    char    *slname;
    song_t  *song;
    int     redisp = false;

    uisfShowPlaylistDisplay (manage->uisongfilter);

    if (manage->selusesonglist &&
        manage->lastdisp == MANAGE_DISP_SONG_SEL) {
      uisongselSaveSelections (manage->mmsongsel);
      /* the song list must be saved, otherwise the song filter */
      /* can't load it */
      manageSonglistSave (manage);
      slname = strdup (uimusicqGetSonglistName (manage->slmusicq));
      uisfSetPlaylist (manage->uisongfilter, slname);
      free (slname);
      manage->lastdisp = MANAGE_DISP_SONG_LIST;
      redisp = true;
    }

    if (redisp) {
      manage->selbypass = 1;
      uisongselApplySongFilter (manage->mmsongsel);

      if (manage->selusesonglist) {
        long idx;

        /* these match because they are displaying the same list */
        idx = uimusicqGetSelectLocation (manage->slmusicq, manage->musicqManageIdx);
        uisongselSetSelection (manage->mmsongsel, idx);
      }
      manage->selbypass = 0;
    }

    if (manage->selusesonglist) {
      dbidx = manage->songlistdbidx;
    } else {
      dbidx = manage->seldbidx;
    }

    song = dbGetByIdx (manage->musicdb, dbidx);
    uisongeditLoadData (manage->mmsongedit, song);
    manageSetBPMCounter (manage, song);
  }
}

static void
manageSetMenuCallback (manageui_t *manage, int midx, UICallbackFunc cb)
{
  uiutilsUICallbackInit (&manage->callbacks [midx], cb, manage);
}

/* the current songlist may be renamed or deleted. */
/* check for this and if the songlist has */
/* disappeared, reset */
static void
manageSonglistLoadCheck (manageui_t *manage)
{
  const char  *name;

  if (manage->sloldname == NULL) {
    return;
  }

  name = uimusicqGetSonglistName (manage->slmusicq);

  if (! managePlaylistExists (name)) {
    /* make sure no save happens */
    manage->sloldname = NULL;
    manageSonglistNew (manage);
  }
}

static bool
manageNewSelectionSongSel (void *udata, long dbidx)
{
  manageui_t  *manage = udata;
  song_t      *song = NULL;

  if (manage->selbypass) {
    return UICB_CONT;
  }

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: edit new selection");
  if (manage->mainlasttab != MANAGE_TAB_MAIN_MM) {
    manage->selusesonglist = false;
  }
  manage->seldbidx = dbidx;

  song = dbGetByIdx (manage->musicdb, dbidx);
  uisongeditLoadData (manage->mmsongedit, song);
  manageSetBPMCounter (manage, song);

  return UICB_CONT;
}

static bool
manageNewSelectionSonglist (void *udata, long dbidx)
{
  manageui_t  *manage = udata;

  if (manage->selbypass) {
    return UICB_CONT;
  }

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: select within song list");
  manage->selusesonglist = true;
  manage->songlistdbidx = dbidx;

  return UICB_CONT;
}

static bool
manageSwitchToSongEditor (void *udata)
{
  manageui_t  *manage = udata;
  int         pagenum;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: switch to song editor");
  if (manage->mainlasttab != MANAGE_TAB_MAIN_MM) {
    /* switching to the music manager tab will also apply the appropriate */
    /* song filter and load the editor */
    pagenum = uiutilsNotebookIDGetPage (manage->mainnbtabid, MANAGE_TAB_MAIN_MM);
    uiNotebookSetPage (&manage->mainnotebook, pagenum);
  }
  if (manage->mmlasttab != MANAGE_TAB_SONGEDIT) {
    pagenum = uiutilsNotebookIDGetPage (manage->mmnbtabid, MANAGE_TAB_SONGEDIT);
    uiNotebookSetPage (&manage->mmnotebook, pagenum);
  }

  return UICB_CONT;
}

static bool
manageSongEditSaveCallback (void *udata)
{
  manageui_t  *manage = udata;
  dbidx_t     dbidx;
  song_t      *song = NULL;
  char        tmp [40];

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: song edit save");
  if (manage->dbchangecount > MANAGE_DB_COUNT_SAVE) {
    dbBackup ();
    manage->dbchangecount = 0;
  }

  if (manage->selusesonglist) {
    dbidx = manage->songlistdbidx;
  } else {
    dbidx = manage->seldbidx;
  }
  song = dbGetByIdx (manage->musicdb, dbidx);
  dbWriteSong (manage->musicdb, song);

  /* the database has been updated, tell the other processes to reload  */
  /* this particular entry */
  snprintf (tmp, sizeof (tmp), "%d", dbidx);
  connSendMessage (manage->conn, ROUTE_STARTERUI, MSG_DB_ENTRY_UPDATE, tmp);

  /* re-populate the song selection displays to display the updated info */
  manage->selbypass = 1;
  uisongselPopulateData (manage->slsongsel);
  uisongselPopulateData (manage->slezsongsel);
  uisongselPopulateData (manage->mmsongsel);
  manage->selbypass = 0;

  /* if write-tags is not 'none', write the tags to the audio file */
  // ### TODO

  ++manage->dbchangecount;

  return UICB_CONT;
}

static bool
manageStartBPMCounter (void *udata)
{
  manageui_t  *manage = udata;
  const char  *targv [2];
  int         targc = 0;

  if (! manage->bpmcounterstarted) {
    int     flags;

    flags = PROCUTIL_DETACH;
    targv [targc++] = NULL;

    manage->processes [ROUTE_MAIN] = procutilStartProcess (
        ROUTE_MAIN, "bdj4bpmcounter", flags, targv);
  }

  return UICB_CONT;
}

static void
manageSetBPMCounter (manageui_t *manage, song_t *song)
{
  int         bpmsel;
  int         timesig = 0;

  bpmsel = bdjoptGetNum (OPT_G_BPM);
  if (bpmsel == BPM_MPM) {
    dance_t     *dances;
    ilistidx_t  danceIdx;

    dances = bdjvarsdfGet (BDJVDF_DANCES);
    danceIdx = songGetNum (song, TAG_DANCE);
    if (danceIdx < 0) {
      /* unknown / not-set dance */
      timesig = DANCE_TIMESIG_44;
    } else {
      timesig = danceGetNum (dances, danceIdx, DANCE_TIMESIG);
    }
    /* 4/8 is handled the same as 4/4 in the bpm counter */
    if (timesig == DANCE_TIMESIG_48) {
      timesig = DANCE_TIMESIG_44;
    }
  }


  manage->currbpmsel = bpmsel;
  manage->currtimesig = timesig;
  manageSendBPMCounter (manage);
}

static void
manageSendBPMCounter (manageui_t *manage)
{
  char        tbuff [60];

  if (! manage->bpmcounterstarted) {
    return;
  }

  snprintf (tbuff, sizeof (tbuff), "%d%c%d",
      manage->currbpmsel, MSG_ARGS_RS, manage->currtimesig);
  connSendMessage (manage->conn, ROUTE_BPM_COUNTER, MSG_BPM_TIMESIG, tbuff);
}

static bool
manageSonglistExportM3U (void *udata)
{
  manageui_t  *manage = udata;
  char        tbuff [200];
  char        tname [200];
  uiselect_t  *selectdata;
  char        *fn;
  const char  *name;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: export m3u");
  name = uimusicqGetSonglistName (manage->slmusicq);
  /* CONTEXT: managementui: song list export: title of save dialog */
  snprintf (tbuff, sizeof (tbuff), _("Export as M3U Playlist"));
  snprintf (tname, sizeof (tname), "%s.m3u", name);
  selectdata = uiDialogCreateSelect (&manage->window,
      tbuff, sysvarsGetStr (SV_BDJ4DATATOPDIR), tname,
      /* CONTEXT: managementui: song list export: name of file save type */
      _("M3U Files"), "audio/x-mpegurl");
  fn = uiSaveFileDialog (selectdata);
  if (fn != NULL) {
    uimusicqExportM3U (manage->slmusicq, fn, name);
    free (fn);
  }
  free (selectdata);
  return UICB_CONT;
}

static bool
manageSonglistImportM3U (void *udata)
{
  manageui_t  *manage = udata;
  char        nplname [200];
  char        tbuff [MAXPATHLEN];
  uiselect_t  *selectdata;
  char        *fn;
  pathinfo_t  *pi;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: import m3u");
  manageSonglistSave (manage);

  /* CONTEXT: managementui: song list: default name for a new song list */
  manageSetSonglistName (manage, _("New Song List"));
  strlcpy (nplname, manage->sloldname, sizeof (nplname));

  selectdata = uiDialogCreateSelect (&manage->window,
      /* CONTEXT: managementui: song list import: title of dialog */
      _("Import M3U"), sysvarsGetStr (SV_BDJ4DATATOPDIR), NULL,
      /* CONTEXT: managementui: song list import: name of file type */
      _("M3U Files"), "audio/x-mpegurl");

  fn = uiSelectFileDialog (selectdata);

  if (fn != NULL) {
    nlist_t     *list;
    int         mqidx;
    dbidx_t     dbidx;
    nlistidx_t  iteridx;
    int         len;

    pi = pathInfo (fn);
    len = pi->blen + 1 > sizeof (nplname) ? sizeof (nplname) : pi->blen + 1;
    strlcpy (nplname, pi->basename, len);

    list = m3uImport (manage->musicdb, fn, nplname, sizeof (nplname));
    pathbldMakePath (tbuff, sizeof (tbuff),
        nplname, BDJ4_SONGLIST_EXT, PATHBLD_MP_DATA);
    if (! fileopFileExists (tbuff)) {
      manageSetSonglistName (manage, nplname);
    }

    if (nlistGetCount (list) > 0) {
      mqidx = manage->musicqManageIdx;

      /* clear the entire queue */
      uimusicqClearQueue (manage->slmusicq, mqidx, 1);

      nlistStartIterator (list, &iteridx);
      while ((dbidx = nlistIterateKey (list, &iteridx)) >= 0) {
        manageQueueProcess (manage, dbidx, mqidx,
            DISP_SEL_SONGLIST, MANAGE_QUEUE_LAST);
      }
    }

    nlistFree (list);
    free (fn);
  }
  free (selectdata);
  return UICB_CONT;
}

