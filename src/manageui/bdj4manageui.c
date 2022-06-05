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
#include "sysvars.h"
#include "tmutil.h"
#include "ui.h"
#include "uimusicq.h"
#include "uiplayer.h"
#include "uiselectfile.h"
#include "uisongedit.h"
#include "uisongfilter.h"
#include "uisongsel.h"
#include "uiutils.h"

enum {
  MANAGE_TAB_OTHER,
  MANAGE_TAB_MAIN_SL,
  MANAGE_TAB_MAIN_MM,
  MANAGE_TAB_MAIN_PL,
  MANAGE_TAB_MAIN_SEQ,
  MANAGE_TAB_MAIN_FILEMGR,
  MANAGE_TAB_SONGLIST,
  MANAGE_TAB_SONGEDIT,
};

enum {
  MANAGE_NB_MAIN,
  MANAGE_NB_SONGLIST,
  MANAGE_NB_MM,
};

enum {
  MANAGE_CB_EZ_SELECT,
  MANAGE_CB_NEW_SEL_DBIDX,
  MANAGE_CB_QUEUE_SL,
  MANAGE_CB_QUEUE_SL_EZ,
  MANAGE_CB_MAX,
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
  char            *locknm;
  conn_t          *conn;
  UICallback      callbacks [MANAGE_CB_MAX];
  musicdb_t       *musicdb;
  musicqidx_t     musicqPlayIdx;
  musicqidx_t     musicqManageIdx;
  dispsel_t       *dispsel;
  int             stopwaitcount;
  UIWidget        statusMsg;
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
  UIWidget        mmnotebook;
  UICallback      mmnbcb;
  /* song list ui major elements */
  uiplayer_t      *slplayer;
  uimusicq_t      *slmusicq;
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
  bool            innewselection : 1;
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
static bool     manageToggleEasySonglist (void *udata);
static void     manageSetEasySonglist (manageui_t *manage);
static void     manageSonglistSave (manageui_t *manage);
static void     manageSetSonglistName (manageui_t *manage, const char *nm);
static bool     manageQueueProcessSonglist (void *udata, long dbidx, int mqidx);
static bool     manageQueueProcessEasySonglist (void *udata, long dbidx, int mqidx);
static void     manageQueueProcess (void *udata, long dbidx, int mqidx, int dispsel);
/* general */
static bool     manageSwitchPageMain (void *udata, long pagenum);
static bool     manageSwitchPageSonglist (void *udata, long pagenum);
static bool     manageSwitchPageMM (void *udata, long pagenum);
static void     manageSwitchPage (manageui_t *manage, long pagenum, int which);
static void     manageSetMenuCallback (manageui_t *manage, int midx, UICallbackFunc cb);
static void     manageSonglistLoadCheck (manageui_t *manage);
/* song editor */
static bool     manageNewSelectionDbidx (void *udata, long dbidx);
static bool     manageSwitchToSongEditor (void *udata);

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
  manage.slsongsel = NULL;
  manage.slezmusicq = NULL;
  manage.slezsongsel = NULL;
  manage.mmplayer = NULL;
  manage.mmmusicq = NULL;
  manage.mmsongsel = NULL;
  manage.mmsongedit = NULL;
  manage.musicqPlayIdx = MUSICQ_B;
  manage.musicqManageIdx = MUSICQ_A;
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
  manage.innewselection = false;
  manage.uisongfilter = NULL;
  manage.manageseq = NULL;   /* allocated within buildui */
  manage.managepl = NULL;   /* allocated within buildui */
  manage.managedb = NULL;   /* allocated within buildui */

  osSetStandardSignals (manageSigHandler);

  bdj4startup (argc, argv, &manage.musicdb,
      "mu", ROUTE_MANAGEUI, BDJ4_INIT_NONE);
  logProcBegin (LOG_PROC, "manageui");

  manage.dispsel = dispselAlloc ();

  listenPort = bdjvarsGetNum (BDJVL_MANAGEUI_PORT);
  manage.conn = connInit (ROUTE_MANAGEUI);

  pathbldMakePath (tbuff, sizeof (tbuff),
      MANAGEUI_OPT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_USEIDX);
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

  /* this callback loads the selected song into the song editor */
  uiutilsUICallbackLongInit (&manage.callbacks [MANAGE_CB_NEW_SEL_DBIDX],
      manageNewSelectionDbidx, &manage);
  uisongselSetSelectionCallback (manage.slezsongsel,
      &manage.callbacks [MANAGE_CB_NEW_SEL_DBIDX]);
  uisongselSetSelectionCallback (manage.slsongsel,
      &manage.callbacks [MANAGE_CB_NEW_SEL_DBIDX]);
  uisongselSetSelectionCallback (manage.mmsongsel,
      &manage.callbacks [MANAGE_CB_NEW_SEL_DBIDX]);

  uisongselSetDefaultSelection (manage.slezsongsel);
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

  pathbldMakePath (fn, sizeof (fn),
      MANAGEUI_OPT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("manageui", fn, manageuidfkeys, MANAGEUI_DFKEY_COUNT, manage->options);

  bdj4shutdown (ROUTE_MANAGEUI, manage->musicdb);
  dispselFree (manage->dispsel);
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
  datafileFree (manage->optiondf);

  uiplayerFree (manage->slplayer);
  uimusicqFree (manage->slmusicq);
  uisongselFree (manage->slsongsel);

  uimusicqFree (manage->slezmusicq);
  uisongselFree (manage->slezsongsel);

  uiplayerFree (manage->mmplayer);
  uimusicqFree (manage->mmmusicq);
  uisongselFree (manage->mmsongsel);
  uisongeditFree (manage->mmsongedit);

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
  /* CONTEXT: management user interface window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Management"), BDJ4_NAME);

  uiutilsUICallbackInit (&manage->closecb, manageCloseWin, manage);
  uiCreateMainWindow (&manage->window, &manage->closecb, tbuff, imgbuff);

  manageInitializeUI (manage);

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

  /* update database */
  manage->managedb = manageDbAlloc (&manage->window,
      manage->options, &manage->statusMsg, manage->conn);

  uiCreateVertBox (&vbox);
  manageBuildUIUpdateDatabase (manage->managedb, &vbox);
  /* CONTEXT: notebook tab title: update database */
  uiCreateLabel (&uiwidget, _("Update Database"));
  uiNotebookAppendPage (&manage->mainnotebook, &vbox, &uiwidget);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_OTHER);

  /* playlist management */
  manage->managepl = managePlaylistAlloc (&manage->window,
      manage->options, &manage->statusMsg);

  uiCreateVertBox (&vbox);
  manageBuildUIPlaylist (manage->managepl, &vbox);
  /* CONTEXT: notebook tab title: playlist management */
  uiCreateLabel (&uiwidget, _("Playlist Management"));
  uiNotebookAppendPage (&manage->mainnotebook, &vbox, &uiwidget);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_MAIN_PL);

  /* sequence editor */
  manage->manageseq = manageSequenceAlloc (&manage->window,
      manage->options, &manage->statusMsg);

  uiCreateVertBox (&vbox);
  manageBuildUISequence (manage->manageseq, &vbox);
  /* CONTEXT: notebook tab title: edit sequences */
  uiCreateLabel (&uiwidget, _("Edit Sequences"));
  uiNotebookAppendPage (&manage->mainnotebook, &vbox, &uiwidget);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_MAIN_SEQ);

  /* file manager */
  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, uiBaseMarginSz * 2);
  /* CONTEXT: notebook tab title: file manager */
  uiCreateLabel (&uiwidget, _("File Manager"));
  uiNotebookAppendPage (&manage->mainnotebook, &vbox, &uiwidget);
  uiutilsNotebookIDAdd (manage->mainnbtabid, MANAGE_TAB_MAIN_FILEMGR);

  x = nlistGetNum (manage->options, PLUI_SIZE_X);
  y = nlistGetNum (manage->options, PLUI_SIZE_Y);
  uiWindowSetDefaultSize (&manage->window, x, y);

  uiutilsUICallbackLongInit (&manage->mainnbcb, manageSwitchPageMain, manage);
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
manageInitializeUI (manageui_t *manage)
{
  UICallback      uicb;

  manage->uisongfilter = uisfInit (&manage->window, manage->options,
      SONG_FILTER_FOR_SELECTION);

  manage->slplayer = uiplayerInit (manage->progstate, manage->conn,
      manage->musicdb);
  manage->slmusicq = uimusicqInit ("m-sl", manage->conn,
      manage->musicdb, manage->dispsel, DISP_SEL_SONGLIST);
  manage->slsongsel = uisongselInit ("m-sl", manage->conn,
      manage->musicdb, manage->dispsel, manage->options,
      manage->uisongfilter, DISP_SEL_SONGSEL);
  uiutilsUICallbackLongIntInit (&manage->callbacks [MANAGE_CB_QUEUE_SL],
      manageQueueProcessSonglist, manage);
  uisongselSetQueueCallback (manage->slsongsel,
      &manage->callbacks [MANAGE_CB_QUEUE_SL]);

  manage->slezmusicq = uimusicqInit ("m-slez", manage->conn,
      manage->musicdb, manage->dispsel, DISP_SEL_EZSONGLIST);
  manage->slezsongsel = uisongselInit ("m-slez", manage->conn,
      manage->musicdb, manage->dispsel, manage->options,
      manage->uisongfilter, DISP_SEL_EZSONGSEL);
  uiutilsUICallbackLongIntInit (&manage->callbacks [MANAGE_CB_QUEUE_SL_EZ],
      manageQueueProcessEasySonglist, manage);
  uisongselSetQueueCallback (manage->slezsongsel,
      &manage->callbacks [MANAGE_CB_QUEUE_SL_EZ]);

  uimusicqSetPlayIdx (manage->slmusicq, manage->musicqPlayIdx);
  uimusicqSetPlayIdx (manage->slezmusicq, manage->musicqPlayIdx);
  uimusicqSetManageIdx (manage->slmusicq, manage->musicqManageIdx);
  uimusicqSetManageIdx (manage->slezmusicq, manage->musicqManageIdx);

  manage->mmplayer = uiplayerInit (manage->progstate, manage->conn,
      manage->musicdb);
  manage->mmmusicq = uimusicqInit ("m-mm", manage->conn,
      manage->musicdb, manage->dispsel, DISP_SEL_SONGLIST);
  manage->mmsongsel = uisongselInit ("m-mm", manage->conn,
      manage->musicdb, manage->dispsel, manage->options,
      manage->uisongfilter, DISP_SEL_MM);

  manage->mmsongedit = uisongeditInit (manage->conn,
      manage->musicdb, manage->dispsel, manage->options);

  uisongselSetPeer (manage->mmsongsel, manage->slezsongsel);
  uisongselSetPeer (manage->mmsongsel, manage->slsongsel);

  uisongselSetPeer (manage->slsongsel, manage->slezsongsel);
  uisongselSetPeer (manage->slsongsel, manage->mmsongsel);

  uisongselSetPeer (manage->slezsongsel, manage->mmsongsel);
  uisongselSetPeer (manage->slezsongsel, manage->slsongsel);

  uimusicqSetPlayIdx (manage->mmmusicq, manage->musicqPlayIdx);
  uimusicqSetManageIdx (manage->mmmusicq, manage->musicqManageIdx);

  uiutilsUICallbackInit (&uicb, manageSwitchToSongEditor, manage);
  uisongselSetEditCallback (manage->slsongsel, &uicb);
  uisongselSetEditCallback (manage->slezsongsel, &uicb);
  uisongselSetEditCallback (manage->mmsongsel, &uicb);
  uimusicqSetEditCallback (manage->slmusicq, &uicb);
  uimusicqSetEditCallback (manage->slezmusicq, &uicb);
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

  uiutilsUICallbackInit (&manage->callbacks [MANAGE_CB_EZ_SELECT],
      uisongselQueueProcessSelectCallback, manage->slezsongsel);
  uiCreateButton (&uiwidget,
      &manage->callbacks [MANAGE_CB_EZ_SELECT],
      /* CONTEXT: config: button: add the selected songs to the song list */
      _("Select"), "button_left");
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

  uimusicqPeerSonglistName (manage->slmusicq, manage->slezmusicq);

  uiutilsUICallbackLongInit (&manage->slnbcb, manageSwitchPageSonglist, manage);
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
  uiutilsUIWidgetCopy (&manage->mmnotebook, &notebook);

  /* music manager: song selection tab*/
  uiwidgetp = uisongselBuildUI (manage->mmsongsel, &manage->window);
  uiWidgetExpandHoriz (uiwidgetp);
  /* CONTEXT: name of song selection notebook tab */
  uiCreateLabel (&uiwidget, _("Music Manager"));
  uiNotebookAppendPage (&notebook, uiwidgetp, &uiwidget);
  uiutilsNotebookIDAdd (manage->mmnbtabid, MANAGE_TAB_OTHER);

  /* music manager: song editor tab */
  uiwidgetp = uisongeditBuildUI (manage->mmsongsel, manage->mmsongedit, &manage->window);
  /* CONTEXT: name of song editor notebook tab */
  uiCreateLabel (&uiwidget, _("Song Editor"));
  uiNotebookAppendPage (&notebook, uiwidgetp, &uiwidget);
  uiutilsNotebookIDAdd (manage->mmnbtabid, MANAGE_TAB_SONGEDIT);

  uiutilsUICallbackLongInit (&manage->mmnbcb, manageSwitchPageMM, manage);
  uiNotebookSetCallback (&notebook, &manage->mmnbcb);
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
    connSendMessage (manage->conn, ROUTE_MAIN, MSG_QUEUE_PLAY_ON_ADD, "1");
    connSendMessage (manage->conn, ROUTE_MAIN, MSG_MUSICQ_SET_PLAYBACK, "1");
    connSendMessage (manage->conn, ROUTE_MAIN, MSG_QUEUE_SWITCH_EMPTY, "0");
    progstateLogTime (manage->progstate, "time-to-start-gui");
    manageDbChg (manage->managedb);
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
          gKillReceived = false;
          progstateShutdownProcess (manage->progstate);
          break;
        }
        case MSG_DB_PROGRESS: {
          manageDbProgressMsg (manage->managedb, args);
          break;
        }
        case MSG_DB_STATUS_MSG: {
          manageDbStatusMsg (manage->managedb, args);
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

  /* due to the db update message, these must be applied afterwards */
  targs = strdup (args);
  uiplayerProcessMsg (routefrom, route, msg, targs, manage->slplayer);
  free (targs);
  uimusicqProcessMsg (routefrom, route, msg, args, manage->slmusicq);
  uisongselProcessMsg (routefrom, route, msg, args, manage->slsongsel);
  uisongselProcessMsg (routefrom, route, msg, args, manage->slezsongsel);
  uimusicqProcessMsg (routefrom, route, msg, args, manage->slezmusicq);
  targs = strdup (args);
  uiplayerProcessMsg (routefrom, route, msg, targs, manage->mmplayer);
  free (targs);
  uisongselProcessMsg (routefrom, route, msg, args, manage->mmsongsel);
  uimusicqProcessMsg (routefrom, route, msg, args, manage->mmmusicq);

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
  uimusicqSetSelectionFirst (manage->slmusicq, manage->musicqManageIdx);
  uimusicqClearQueue (manage->slmusicq);
  return UICB_CONT;
}

static bool
manageSonglistTruncate (void *udata)
{
  manageui_t  *manage = udata;

  uimusicqClearQueue (manage->slmusicq);
  return UICB_CONT;
}

static void
manageSonglistLoadFile (void *udata, const char *fn)
{
  manageui_t  *manage = udata;
  char  tbuff [200];

  manageSonglistSave (manage);

  /* truncate from the first selection */
  uimusicqSetSelectionFirst (manage->slmusicq, manage->musicqManageIdx);
  uimusicqClearQueue (manage->slmusicq);

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

  if (manage->sloldname == NULL) {
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
manageQueueProcessSonglist (void *udata, long dbidx, int mqidx)
{
  manageQueueProcess (udata, dbidx, mqidx, DISP_SEL_SONGLIST);
  return UICB_CONT;
}

static bool
manageQueueProcessEasySonglist (void *udata, long dbidx, int mqidx)
{
  manageQueueProcess (udata, dbidx, mqidx, DISP_SEL_EZSONGLIST);
  return UICB_CONT;
}

static void
manageQueueProcess (void *udata, long dbidx, int mqidx, int dispsel)
{
  manageui_t  *manage = udata;
  long        loc;
  char        tbuff [100];
  uimusicq_t  *uimusicq = NULL;

  if (dispsel == DISP_SEL_SONGLIST) {
    uimusicq = manage->slmusicq;
  }
  if (dispsel == DISP_SEL_EZSONGLIST) {
    uimusicq = manage->slezmusicq;
  }

  loc = uimusicqGetSelectLocation (uimusicq, mqidx);

  snprintf (tbuff, sizeof (tbuff), "%d%c%ld%c%ld", mqidx,
      MSG_ARGS_RS, loc, MSG_ARGS_RS, dbidx);
  connSendMessage (manage->conn, ROUTE_MAIN, MSG_MUSICQ_INSERT, tbuff);
}

/* general */

static bool
manageSwitchPageMain (void *udata, long pagenum)
{
  manageui_t  *manage = udata;

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
      manageSonglistLoadCheck (manage);
      break;
    }
    case MANAGE_TAB_MAIN_MM: {
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

static void
manageSetMenuCallback (manageui_t *manage, int midx, UICallbackFunc cb)
{
  uiutilsUICallbackInit (&manage->menucb [midx], cb, manage);
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
manageNewSelectionDbidx (void *udata, long dbidx)
{
  manageui_t    *manage = udata;
  song_t        *song;

  if (manage->innewselection) {
    return UICB_CONT;
  }

  song = dbGetByIdx (manage->musicdb, dbidx);
  uisongeditLoadData (manage->mmsongedit, song);
  return UICB_CONT;
}

static bool
manageSwitchToSongEditor (void *udata)
{
  manageui_t  *manage = udata;
  int         pagenum;

  if (manage->mainlasttab != MANAGE_TAB_MAIN_MM) {
    pagenum = uiutilsNotebookIDGetPage (manage->mainnbtabid, MANAGE_TAB_MAIN_MM);
    uiNotebookSetPage (&manage->mainnotebook, pagenum);
  }
  if (manage->mmlasttab != MANAGE_TAB_SONGEDIT) {
    pagenum = uiutilsNotebookIDGetPage (manage->mmnbtabid, MANAGE_TAB_SONGEDIT);
    uiNotebookSetPage (&manage->mmnotebook, pagenum);
  }

  return UICB_CONT;
}
