/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>

#include "audioadjust.h"
#include "audiosrc.h"
#include "audiotag.h"
#include "bdj4.h"
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdj4ui.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "bdjvarsdf.h"
#include "callback.h"
#include "conn.h"
#include "dance.h"
#include "datafile.h"
#include "dispsel.h"
#include "expimpbdj4.h"
#include "fileop.h"
#include "filemanip.h"
#include "itunes.h"
#include "localeutil.h"
#include "lock.h"
#include "log.h"
#include "m3u.h"
#include "manageui.h"
#include "mdebug.h"
#include "msgparse.h"
#include "musicq.h"
#include "ossignal.h"
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
#include "song.h"
#include "songdb.h"
#include "songutil.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"
#include "ui.h"
#include "uiapplyadj.h"
#include "uicopytags.h"
#include "uiexpimpbdj4.h"
#include "uimusicq.h"
#include "uinbutil.h"
#include "uiplayer.h"
#include "uiplaylist.h"
#include "uiselectfile.h"
#include "uisongedit.h"
#include "uisongfilter.h"
#include "uisongsel.h"
#include "uiutils.h"

enum {
  MANAGE_TAB_MAIN_OTHER,
  MANAGE_TAB_MAIN_SL,
  MANAGE_TAB_MAIN_MM,
  MANAGE_TAB_MAIN_PL,
  MANAGE_TAB_MAIN_SEQ,
  MANAGE_TAB_MM,
  MANAGE_TAB_OTHER,
  MANAGE_TAB_SONGLIST,
  MANAGE_TAB_SONGEDIT,
  MANAGE_TAB_AUDIOID,
  MANAGE_TAB_STATISTICS,
};

enum {
  MANAGE_NB_MAIN,
  MANAGE_NB_SONGLIST,
  MANAGE_NB_MM,
};

enum {
  /* music manager */
  MANAGE_MENU_CB_MM_CLEAR_MARK,
  MANAGE_MENU_CB_MM_SET_MARK,
  MANAGE_MENU_CB_MM_REMOVE_SONG,
  MANAGE_MENU_CB_MM_UNDO_REMOVE,
  /* song editor */
  MANAGE_MENU_CB_SE_BPM,
  MANAGE_MENU_CB_SE_WRITE_TAGS,
  MANAGE_MENU_CB_SE_COPY_TAGS,
  MANAGE_MENU_CB_SE_APPLY_EDIT_ALL,
  MANAGE_MENU_CB_SE_CANCEL_EDIT_ALL,
  MANAGE_MENU_CB_SE_START_EDIT_ALL,
  MANAGE_MENU_CB_SE_APPLY_ADJ,
  MANAGE_MENU_CB_SE_RESTORE_ORIG,
  /* sl options menu */
  MANAGE_MENU_CB_SL_SBS_EDIT,
  /* sl edit menu */
  MANAGE_MENU_CB_SL_LOAD,
  MANAGE_MENU_CB_SL_NEW,
  MANAGE_MENU_CB_SL_COPY,
  MANAGE_MENU_CB_SL_DELETE,
  /* sl actions menu */
  MANAGE_MENU_CB_SL_MIX,
  MANAGE_MENU_CB_SL_SWAP,
  MANAGE_MENU_CB_SL_TRUNCATE,
  MANAGE_MENU_CB_SL_MK_FROM_PL,
  /* sl export menu */
  MANAGE_MENU_CB_SL_M3U_EXP,
  MANAGE_MENU_CB_SL_MP3_EXP,
  MANAGE_MENU_CB_SL_BDJ4_EXP,
  MANAGE_MENU_CB_SL_BDJ4_IMP,
  /* sl import menu */
  MANAGE_MENU_CB_SL_M3U_IMP,
  MANAGE_MENU_CB_SL_ITUNES_IMP,
  /* other callbacks */
  MANAGE_CB_SBS_SELECT,
  MANAGE_CB_NEW_SEL_SONGSEL,
  MANAGE_CB_NEW_SEL_SONGLIST,
  MANAGE_CB_QUEUE_SL,
  MANAGE_CB_QUEUE_SL_SBS,
  MANAGE_CB_PLAY_SL,
  MANAGE_CB_PLAY_SL_SBS,
  MANAGE_CB_PLAY_MM,
  MANAGE_CB_CLOSE,
  MANAGE_CB_MAIN_NB,
  MANAGE_CB_SL_NB,
  MANAGE_CB_MM_NB,
  MANAGE_CB_EDIT,
  MANAGE_CB_SEQ_LOAD,
  MANAGE_CB_SEQ_NEW,
  MANAGE_CB_PL_LOAD,
  MANAGE_CB_SAVE,
  MANAGE_CB_CFPL_DIALOG,
  MANAGE_CB_ITUNES_DIALOG,
  MANAGE_CB_ITUNES_SEL,
  MANAGE_CB_APPLY_ADJ,
  MANAGE_CB_SL_SEL_FILE,
  MANAGE_CB_BDJ4_EXP,
  MANAGE_CB_BDJ4_IMP,
  MANAGE_CB_MAX,
};

enum {
  /* track what the mm is currently displaying */
  MANAGE_DISP_SONG_SEL,
  MANAGE_DISP_SONG_LIST,
  /* same song */
  MANAGE_SET_MARK,
  MANAGE_CLEAR_MARK,
};

/* actions for the queue process */
enum {
  MANAGE_PLAY,
  MANAGE_QUEUE,
  MANAGE_QUEUE_LAST,
};

enum {
  MANAGE_W_WINDOW,
  MANAGE_W_MENUBAR,
  MANAGE_W_MAIN_NB,
  MANAGE_W_SONGLIST_NB,
  MANAGE_W_MM_NB,
  MANAGE_W_SL_SBS_MUSICQ_TAB,
  MANAGE_W_SL_MUSICQ_TAB,         // not owner
  MANAGE_W_SONGSEL_TAB,           // not owner
  MANAGE_W_STATUS_MSG,
  MANAGE_W_ERROR_MSG,
  MANAGE_W_MENU_RESTORE_ORIG,
  MANAGE_W_ITUNES_SEL_DIALOG,
  MANAGE_W_CFPL_DIALOG,
  MANAGE_W_MENU_UNDO_REMOVE,
  MANAGE_W_MAX,
};

typedef struct {
  progstate_t       *progstate;
  char              *locknm;
  conn_t            *conn;
  procutil_t        *processes [ROUTE_MAX];
  callback_t        *callbacks [MANAGE_CB_MAX];
  musicdb_t         *musicdb;
  samesong_t        *samesong;
  musicqidx_t       musicqPlayIdx;
  musicqidx_t       musicqManageIdx;
  dispsel_t         *dispsel;
  int               stopwaitcount;
  uiwcont_t         *wcont [MANAGE_W_MAX];
  const char        *pleasewaitmsg;
  /* notebook tab handling */
  int               mainlasttab;
  int               sllasttab;
  int               mmlasttab;
  uimenu_t          *currmenu;
  uimenu_t          *slmenu;
  uimenu_t          *songeditmenu;
  uimenu_t          *mmmenu;
  uinbtabid_t       *mainnbtabid;
  uinbtabid_t       *slnbtabid;
  uinbtabid_t       *mmnbtabid;
  uibutton_t        *selectButton;
  dbidx_t           songlistdbidx;
  dbidx_t           seldbidx;
  dbidx_t           songeditdbidx;
  /* song list ui major elements */
  uiplayer_t        *slplayer;
  uimusicq_t        *slmusicq;
  managestats_t     *slstats;
  uisongsel_t       *slsongsel;
  uimusicq_t        *slsbsmusicq;
  uisongsel_t       *slsbssongsel;
  char              *sloldname;
  itunes_t          *itunes;
  uidropdown_t      *itunessel;
  /* prior name is used by create-from-playlist */
  uisongfilter_t    *uisongfilter;
  uiplaylist_t      *cfpl;
  uispinbox_t       *cfpltmlimit;
  char              *slpriorname;
  char              *cfplfn;
  /* music manager ui */
  uiplayer_t        *mmplayer;
  uimusicq_t        *mmmusicq;
  uisongsel_t       *mmsongsel;
  uisongedit_t      *mmsongedit;
  int               lastdisp;
  int               dbchangecount;
  int               editmode;
  int               lastinsertlocation;
  /* sequence */
  manageseq_t       *manageseq;
  /* playlist management */
  managepl_t        *managepl;
  /* update database */
  managedb_t        *managedb;
  /* audio id */
  manageaudioid_t   *manageaudioid;
  /* bpm counter */
  int               currtimesig;
  /* song editor */
  uict_t            *uict;
  int               ctstate;
  uiaa_t            *uiaa;
  int               aaflags;
  int               applyadjstate;
  int               impitunesstate;
  /* export/import bdj4 */
  uieibdj4_t        *uieibdj4;
  eibdj4_t          *eibdj4;
  mstime_t          eibdj4ChkTime;
  int               expimpbdj4state;
  /* options */
  datafile_t        *optiondf;
  nlist_t           *options;
  /* remove song */
  nlist_t           *removelist;
  /* various flags */
  bool              slbackupcreated : 1;
  bool              selusesonglist : 1;
  bool              inload : 1;
  bool              bpmcounterstarted : 1;
  bool              pluiActive : 1;
  bool              selbypass : 1;
  bool              cfplactive : 1;
  bool              cfplpostprocess : 1;
  bool              musicqupdated : 1;
  bool              importitunesactive : 1;
  bool              exportm3uactive : 1;
  bool              importm3uactive : 1;
  bool              exportbdj4active : 1;
  bool              importbdj4active : 1;
  bool              enablerestoreorig : 1;
  bool              ineditall : 1;
  bool              optionsalloc : 1;
  bool              musicqueueprocessflag : 1;
} manageui_t;

/* re-use the plui enums so that the songsel filter enums can also be used */
static datafilekey_t manageuidfkeys [] = {
  { "APPLY_ADJ_POS_X",  APPLY_ADJ_POSITION_X,       VALUE_NUM, NULL, DF_NORM },
  { "APPLY_ADJ_POS_Y",  APPLY_ADJ_POSITION_Y,       VALUE_NUM, NULL, DF_NORM },
  { "COPY_TAGS_POS_X",  COPY_TAGS_POSITION_X,       VALUE_NUM, NULL, DF_NORM },
  { "COPY_TAGS_POS_Y",  COPY_TAGS_POSITION_Y,       VALUE_NUM, NULL, DF_NORM },
  { "EASY_SONGLIST",    MANAGE_SBS_SONGLIST,        VALUE_NUM, convBoolean, DF_NO_WRITE },
  { "FILTER_POS_X",     SONGSEL_FILTER_POSITION_X,  VALUE_NUM, NULL, DF_NORM },
  { "FILTER_POS_Y",     SONGSEL_FILTER_POSITION_Y,  VALUE_NUM, NULL, DF_NORM },
  { "MNG_AU_PANE_POS",  MANAGE_AUDIOID_PANE_POSITION, VALUE_NUM, NULL, DF_NORM },
  { "MNG_CFPL_POS_X",   MANAGE_CFPL_POSITION_X,     VALUE_NUM, NULL, DF_NORM },
  { "MNG_CFPL_POS_Y",   MANAGE_CFPL_POSITION_Y,     VALUE_NUM, NULL, DF_NORM },
  { "MNG_EXPIMP_POS_X", EXP_IMP_BDJ4_POSITION_X,    VALUE_NUM, NULL, DF_NORM },
  { "MNG_EXPIMP_POS_Y", EXP_IMP_BDJ4_POSITION_Y,    VALUE_NUM, NULL, DF_NORM },
  { "MNG_EXP_BDJ4_DIR", MANAGE_EXP_BDJ4_DIR,        VALUE_STR, NULL, DF_NORM },
  { "MNG_IMP_BDJ4_DIR", MANAGE_IMP_BDJ4_DIR,        VALUE_STR, NULL, DF_NORM },
  { "MNG_POS_X",        MANAGE_POSITION_X,          VALUE_NUM, NULL, DF_NORM },
  { "MNG_POS_Y",        MANAGE_POSITION_Y,          VALUE_NUM, NULL, DF_NORM },
  { "MNG_SELFILE_POS_X",MANAGE_SELFILE_POSITION_X,  VALUE_NUM, NULL, DF_NORM },
  { "MNG_SELFILE_POS_Y",MANAGE_SELFILE_POSITION_Y,  VALUE_NUM, NULL, DF_NORM },
  { "MNG_SIZE_X",       MANAGE_SIZE_X,              VALUE_NUM, NULL, DF_NORM },
  { "MNG_SIZE_Y",       MANAGE_SIZE_Y,              VALUE_NUM, NULL, DF_NORM },
  { "QE_POS_X",         QE_POSITION_X,              VALUE_NUM, NULL, DF_NORM },
  { "QE_POS_Y",         QE_POSITION_Y,              VALUE_NUM, NULL, DF_NORM },
  { "SBS_SONGLIST",     MANAGE_SBS_SONGLIST,        VALUE_NUM, convBoolean, DF_NORM },
  { "SORT_BY",          SONGSEL_SORT_BY,            VALUE_STR, NULL, DF_NORM },
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
static bool     manageNewSelectionSongSel (void *udata, long dbidx);
static bool     manageNewSelectionSonglist (void *udata, long dbidx);
static bool     manageSwitchToSongEditor (void *udata);
static bool     manageSongEditSaveCallback (void *udata, long dbidx);
static void     manageRePopulateData (manageui_t *manage);
static void     manageSetEditMenuItems (manageui_t *manage);
static bool     manageApplyAdjDialog (void *udata);
static bool     manageApplyAdjCallback (void *udata, long aaflags);
static bool     manageRestoreOrigCallback (void *udata);
static bool     manageCopyTagsStart (void *udata);
static bool     manageEditAllStart (void *udata);
static bool     manageEditAllApply (void *udata);
static bool     manageEditAllCancel (void *udata);
static void     manageReloadSongEdit (manageui_t *manage);
/* bpm counter */
static bool     manageStartBPMCounter (void *udata);
static void     manageSetBPMCounter (manageui_t *manage, song_t *song);
static void     manageSendBPMCounter (manageui_t *manage);
/* itunes */
static bool     manageSonglistImportiTunes (void *udata);
static void     manageiTunesCreateDialog (manageui_t *manage);
static void     manageiTunesDialogCreateList (manageui_t *manage);
static bool     manageiTunesDialogSelectHandler (void *udata, long idx);
static bool     manageiTunesDialogResponseHandler (void *udata, long responseid);
/* music manager */
static void     manageBuildUIMusicManager (manageui_t *manage);
static void     manageMusicManagerMenu (manageui_t *manage);
/* song list */
static void     manageBuildUISongListEditor (manageui_t *manage);
static void     manageSonglistMenu (manageui_t *manage);
static bool     manageSonglistLoad (void *udata);
static long     manageSonglistLoadCB (void *udata, const char *fn);
static bool     manageSonglistCopy (void *udata);
static bool     manageSonglistNew (void *udata);
static bool     manageSonglistDelete (void *udata);
static bool     manageSonglistTruncate (void *udata);
static bool     manageSonglistCreateFromPlaylist (void *udata);
static void     manageSongListCFPLCreateDialog (manageui_t *manage);
static bool     manageCFPLResponseHandler (void *udata, long responseid);
static void     manageCFPLCreate (manageui_t *manage);
static void     manageCFPLPostProcess (manageui_t *manage);
static bool     manageSonglistMix (void *udata);
static bool     manageSonglistSwap (void *udata);
static void     manageSonglistLoadFile (void *udata, const char *fn, int preloadflag);
static long     manageLoadPlaylistCB (void *udata, const char *fn);
static bool     manageNewPlaylistCB (void *udata);
static long     manageLoadSonglistSeqCB (void *udata, const char *fn);
static bool     manageToggleEasySonglist (void *udata);
static void     manageSetEasySonglist (manageui_t *manage);
static void     manageSonglistSave (manageui_t *manage);
static void     manageSetSonglistName (manageui_t *manage, const char *nm);
static bool     managePlayProcessSonglist (void *udata, long dbidx, int mqidx);
static bool     managePlayProcessEasySonglist (void *udata, long dbidx, int mqidx);
static bool     managePlayProcessMusicManager (void *udata, long dbidx, int mqidx);
static bool     manageQueueProcessSonglist (void *udata, long dbidx);
static bool     manageQueueProcessEasySonglist (void *udata, long dbidx);
static void     manageQueueProcess (void *udata, long dbidx, int mqidx, int dispsel, int action);
/* m3u */
static bool     manageSonglistExportM3U (void *udata);
static bool     manageSonglistImportM3U (void *udata);
/* export/import bdj4 */
static bool     manageSonglistExportBDJ4 (void *udata);
static bool     manageSonglistImportBDJ4 (void *udata);
static bool     manageExportBDJ4ResponseHandler (void *udata);
static bool     manageImportBDJ4ResponseHandler (void *udata);
/* general */
static bool     manageSwitchPageMain (void *udata, long pagenum);
static bool     manageSwitchPageSonglist (void *udata, long pagenum);
static bool     manageSwitchPageMM (void *udata, long pagenum);
static void     manageSwitchPage (manageui_t *manage, long pagenum, int which);
static void     manageSetDisplayPerSelection (manageui_t *manage, int id);
static void     manageSetMenuCallback (manageui_t *manage, int midx, callbackFunc cb);
static void     manageSonglistLoadCheck (manageui_t *manage);
static void     manageProcessDatabaseUpdate (manageui_t *manage);
/* same song */
static bool     manageSameSongSetMark (void *udata);
static bool     manageSameSongClearMark (void *udata);
static void     manageSameSongChangeMark (manageui_t *manage, int flag);
/* remove song */
static bool     manageMarkSongRemoved (void *udata);
static bool     manageUndoRemove (void *udata);
static void     manageRemoveSongs (manageui_t *manage);

static int gKillReceived = false;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  manageui_t      manage;
  char            tbuff [MAXPATHLEN];
  long            flags;

#if BDJ4_MEM_DEBUG
  mdebugInit ("mui");
#endif

  manage.progstate = progstateInit ("manageui");
  progstateSetCallback (manage.progstate, STATE_CONNECTING,
      manageConnectingCallback, &manage);
  progstateSetCallback (manage.progstate, STATE_WAIT_HANDSHAKE,
      manageHandshakeCallback, &manage);

  for (int i = 0; i < MANAGE_W_MAX; ++i) {
    manage.wcont [i] = NULL;
  }
  manage.slplayer = NULL;
  manage.slmusicq = NULL;
  manage.slstats = NULL;
  manage.slsongsel = NULL;
  manage.slsbsmusicq = NULL;
  manage.slsbssongsel = NULL;
  manage.mmplayer = NULL;
  manage.mmmusicq = NULL;
  manage.mmsongsel = NULL;
  manage.mmsongedit = NULL;
  manage.uieibdj4 = NULL;
  manage.eibdj4 = NULL;
  manage.expimpbdj4state = BDJ4_STATE_OFF;
  manage.musicqPlayIdx = MUSICQ_MNG_PB;
  manage.musicqManageIdx = MUSICQ_SL;
  manage.stopwaitcount = 0;
  manage.currmenu = NULL;
  manage.mainlasttab = MANAGE_TAB_MAIN_SL;
  manage.sllasttab = MANAGE_TAB_SONGLIST;
  manage.mmlasttab = MANAGE_TAB_MM;
  manage.slmenu = uiMenuAlloc ();
  manage.songeditmenu = uiMenuAlloc ();
  manage.mmmenu = uiMenuAlloc ();
  manage.mainnbtabid = uinbutilIDInit ();
  manage.slnbtabid = uinbutilIDInit ();
  manage.mmnbtabid = uinbutilIDInit ();
  manage.sloldname = NULL;
  manage.slpriorname = NULL;
  manage.cfplfn = NULL;
  manage.itunes = NULL;
  manage.itunessel = uiDropDownInit ();
  manage.slbackupcreated = false;
  manage.selusesonglist = false;
  manage.inload = false;
  manage.lastdisp = MANAGE_DISP_SONG_SEL;
  manage.selbypass = true;
  manage.seldbidx = -1;
  manage.songlistdbidx = -1;
  manage.songeditdbidx = -1;
  manage.uisongfilter = NULL;
  manage.dbchangecount = 0;
  manage.editmode = EDIT_TRUE;
  manage.manageseq = NULL;   /* allocated within buildui */
  manage.managepl = NULL;   /* allocated within buildui */
  manage.managedb = NULL;   /* allocated within buildui */
  manage.manageaudioid = NULL;
  manage.bpmcounterstarted = false;
  manage.currtimesig = DANCE_TIMESIG_44;
  manage.cfpl = NULL;
  manage.cfpltmlimit = uiSpinboxTimeInit (SB_TIME_BASIC);
  manage.pluiActive = false;
  manage.selectButton = NULL;
  manage.lastinsertlocation = QUEUE_LOC_LAST;
  manage.cfplactive = false;
  manage.cfplpostprocess = false;
  manage.musicqupdated = false;
  manage.importitunesactive = false;
  manage.exportm3uactive = false;
  manage.importm3uactive = false;
  manage.exportbdj4active = false;
  manage.importbdj4active = false;
  manage.enablerestoreorig = false;
  manage.ineditall = false;
  manage.musicqueueprocessflag = false;
  manage.applyadjstate = BDJ4_STATE_OFF;
  manage.impitunesstate = BDJ4_STATE_OFF;
  manage.uict = NULL;
  manage.ctstate = BDJ4_STATE_OFF;
  manage.uiaa = NULL;
  for (int i = 0; i < MANAGE_CB_MAX; ++i) {
    manage.callbacks [i] = NULL;
  }
  manage.removelist = nlistAlloc ("remove-list", LIST_ORDERED, NULL);

  /* CONTEXT: management ui: please wait... status message */
  manage.pleasewaitmsg = _("Please wait\xe2\x80\xa6");

  procutilInitProcesses (manage.processes);

  osSetStandardSignals (manageSigHandler);

  flags = BDJ4_INIT_ALL;
  bdj4startup (argc, argv, &manage.musicdb, "mui", ROUTE_MANAGEUI, &flags);
  logProcBegin (LOG_PROC, "manageui");

  manage.dispsel = dispselAlloc (DISP_SEL_LOAD_MANAGE);

  listenPort = bdjvarsGetNum (BDJVL_MANAGEUI_PORT);
  manage.conn = connInit (ROUTE_MANAGEUI);

  pathbldMakePath (tbuff, sizeof (tbuff),
      MANAGEUI_OPT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  manage.optiondf = datafileAllocParse ("ui-manage", DFTYPE_KEY_VAL, tbuff,
      manageuidfkeys, MANAGEUI_DFKEY_COUNT, DF_NO_OFFSET, NULL);
  manage.options = datafileGetList (manage.optiondf);
  manage.optionsalloc = false;
  if (manage.options == NULL || nlistGetCount (manage.options) == 0) {
    manage.optionsalloc = true;
    manage.options = nlistAlloc ("ui-manage", LIST_ORDERED, NULL);

    nlistSetNum (manage.options, SONGSEL_FILTER_POSITION_X, -1);
    nlistSetNum (manage.options, SONGSEL_FILTER_POSITION_Y, -1);
    nlistSetNum (manage.options, MANAGE_POSITION_X, -1);
    nlistSetNum (manage.options, MANAGE_POSITION_Y, -1);
    nlistSetNum (manage.options, MANAGE_SIZE_X, 1000);
    nlistSetNum (manage.options, MANAGE_SIZE_Y, 600);
    nlistSetStr (manage.options, SONGSEL_SORT_BY, "TITLE");
    nlistSetNum (manage.options, MANAGE_SELFILE_POSITION_X, -1);
    nlistSetNum (manage.options, MANAGE_SELFILE_POSITION_Y, -1);
    nlistSetNum (manage.options, MANAGE_SBS_SONGLIST, true);
    nlistSetNum (manage.options, MANAGE_CFPL_POSITION_X, -1);
    nlistSetNum (manage.options, MANAGE_CFPL_POSITION_Y, -1);
    nlistSetNum (manage.options, APPLY_ADJ_POSITION_X, -1);
    nlistSetNum (manage.options, APPLY_ADJ_POSITION_Y, -1);
    nlistSetNum (manage.options, COPY_TAGS_POSITION_X, -1);
    nlistSetNum (manage.options, COPY_TAGS_POSITION_Y, -1);
    nlistSetNum (manage.options, EXP_IMP_BDJ4_POSITION_X, -1);
    nlistSetNum (manage.options, EXP_IMP_BDJ4_POSITION_Y, -1);
    nlistSetStr (manage.options, MANAGE_EXP_BDJ4_DIR, "");
    nlistSetStr (manage.options, MANAGE_IMP_BDJ4_DIR, "");
    nlistSetNum (manage.options, MANAGE_AUDIOID_PANE_POSITION, -1);
    nlistSetNum (manage.options, QE_POSITION_X, -1);
    nlistSetNum (manage.options, QE_POSITION_Y, -1);
  }

  uiUIInitialize ();
  uiSetUICSS (uiutilsGetCurrentFont (),
      bdjoptGetStr (OPT_P_UI_ACCENT_COL),
      bdjoptGetStr (OPT_P_UI_ERROR_COL));

  manageBuildUI (&manage);
  osuiFinalize ();

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
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return status;
}

/* internal routines */

static bool
manageStoppingCallback (void *udata, programstate_t programState)
{
  manageui_t    * manage = udata;
  int           x, y, ws;

  logProcBegin (LOG_PROC, "manageStoppingCallback");

  /* remove any songs in the removal list */
  manageRemoveSongs (manage);

  connSendMessage (manage->conn, ROUTE_STARTERUI, MSG_STOP_MAIN, NULL);

  procutilStopAllProcess (manage->processes, manage->conn, PROCUTIL_NORM_TERM);

  if (manage->bpmcounterstarted > 0) {
    procutilStopProcess (manage->processes [ROUTE_BPM_COUNTER],
        manage->conn, ROUTE_BPM_COUNTER, PROCUTIL_NORM_TERM);
    manage->bpmcounterstarted = false;
  }

  connDisconnectAll (manage->conn);

  manageSonglistSave (manage);
  manageSequenceSave (manage->manageseq);
  managePlaylistSave (manage->managepl);

  manageAudioIdSavePosition (manage->manageaudioid);

  uiWindowGetSize (manage->wcont [MANAGE_W_WINDOW], &x, &y);
  nlistSetNum (manage->options, MANAGE_SIZE_X, x);
  nlistSetNum (manage->options, MANAGE_SIZE_Y, y);
  uiWindowGetPosition (manage->wcont [MANAGE_W_WINDOW], &x, &y, &ws);
  nlistSetNum (manage->options, MANAGE_POSITION_X, x);
  nlistSetNum (manage->options, MANAGE_POSITION_Y, y);

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

  logProcBegin (LOG_PROC, "manageClosingCallback");

  uiCloseWindow (manage->wcont [MANAGE_W_WINDOW]);
  uiCleanup ();

  manageDbClose (manage->managedb);

  for (int i = 0; i < MANAGE_W_MAX; ++i) {
    if (i == MANAGE_W_SL_MUSICQ_TAB ||
        i == MANAGE_W_SONGSEL_TAB) {
      continue;
    }
    uiwcontFree (manage->wcont [i]);
  }
  uiMenuFree (manage->slmenu);
  uiMenuFree (manage->songeditmenu);
  uiMenuFree (manage->mmmenu);
  itunesFree (manage->itunes);
  samesongFree (manage->samesong);
  uiButtonFree (manage->selectButton);
  uicopytagsFree (manage->uict);
  uiaaFree (manage->uiaa);
  uieibdj4Free (manage->uieibdj4);
  eibdj4Free (manage->eibdj4);
  nlistFree (manage->removelist);

  procutilStopAllProcess (manage->processes, manage->conn, PROCUTIL_FORCE_TERM);
  procutilFreeAll (manage->processes);

  datafileSave (manage->optiondf, NULL, manage->options, DF_NO_OFFSET, 1);

  bdj4shutdown (ROUTE_MANAGEUI, manage->musicdb);
  manageSequenceFree (manage->manageseq);
  managePlaylistFree (manage->managepl);
  manageDbFree (manage->managedb);
  manageAudioIdFree (manage->manageaudioid);

  uisfFree (manage->uisongfilter);
  dataFree (manage->sloldname);
  dataFree (manage->slpriorname);
  dataFree (manage->cfplfn);
  uinbutilIDFree (manage->mainnbtabid);
  uinbutilIDFree (manage->slnbtabid);
  uinbutilIDFree (manage->mmnbtabid);

  uiplayerFree (manage->slplayer);
  uimusicqFree (manage->slmusicq);
  manageStatsFree (manage->slstats);
  uisongselFree (manage->slsongsel);
  uiDropDownFree (manage->itunessel);

  uimusicqFree (manage->slsbsmusicq);
  uisongselFree (manage->slsbssongsel);

  uiplayerFree (manage->mmplayer);
  uimusicqFree (manage->mmmusicq);
  uisongselFree (manage->mmsongsel);
  uisongeditFree (manage->mmsongedit);

  uiplaylistFree (manage->cfpl);
  uiSpinboxFree (manage->cfpltmlimit);
  dispselFree (manage->dispsel);

  for (int i = 0; i < MANAGE_CB_MAX; ++i) {
    callbackFree (manage->callbacks [i]);
  }

  if (manage->optionsalloc) {
    nlistFree (manage->options);
  }
  datafileFree (manage->optiondf);

  logProcEnd (LOG_PROC, "manageClosingCallback", "");
  return STATE_FINISHED;
}

static void
manageBuildUI (manageui_t *manage)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *uiwidgetp;
  char        imgbuff [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];
  int         x, y;
  uiutilsaccent_t accent;

  logProcBegin (LOG_PROC, "manageBuildUI");
  *imgbuff = '\0';

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_manage", BDJ4_IMG_SVG_EXT, PATHBLD_MP_DIR_IMG);
  /* CONTEXT: managementui: management user interface window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Management"),
      bdjoptGetStr (OPT_P_PROFILENAME));

  manage->callbacks [MANAGE_CB_CLOSE] = callbackInit (
      manageCloseWin, manage, NULL);
  manage->wcont [MANAGE_W_WINDOW] = uiCreateMainWindow (
      manage->callbacks [MANAGE_CB_CLOSE], tbuff, imgbuff);

  manageInitializeUI (manage);

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);
  uiBoxPackInWindow (manage->wcont [MANAGE_W_WINDOW], vbox);

  uiutilsAddAccentColorDisplay (vbox, &accent);
  hbox = accent.hbox;
  uiwcontFree (accent.label);

  uiwidgetp = uiCreateLabel ("");
  uiWidgetSetClass (uiwidgetp, ERROR_CLASS);
  uiBoxPackEnd (hbox, uiwidgetp);
  manage->wcont [MANAGE_W_ERROR_MSG] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("");
  uiWidgetSetClass (uiwidgetp, ACCENT_CLASS);
  uiBoxPackEnd (hbox, uiwidgetp);
  manage->wcont [MANAGE_W_STATUS_MSG] = uiwidgetp;

  manage->wcont [MANAGE_W_MENUBAR] = uiCreateMenubar ();
  uiBoxPackStart (hbox, manage->wcont [MANAGE_W_MENUBAR]);

  manage->wcont [MANAGE_W_MAIN_NB] = uiCreateNotebook ();
  uiWidgetSetClass (manage->wcont [MANAGE_W_MAIN_NB], "leftnotebook");
  uiNotebookTabPositionLeft (manage->wcont [MANAGE_W_MAIN_NB]);
  uiBoxPackStartExpand (vbox, manage->wcont [MANAGE_W_MAIN_NB]);

  uiwcontFree (vbox);

  /* edit song lists */
  manageBuildUISongListEditor (manage);

  /* sequence editor */
  manage->manageseq = manageSequenceAlloc (manage->wcont [MANAGE_W_WINDOW],
      manage->options, manage->wcont [MANAGE_W_ERROR_MSG]);

  vbox = uiCreateVertBox ();
  manageBuildUISequence (manage->manageseq, vbox);
  /* CONTEXT: managementui: notebook tab title: edit sequences */
  uiwidgetp = uiCreateLabel (_("Edit Sequences"));
  uiNotebookAppendPage (manage->wcont [MANAGE_W_MAIN_NB], vbox, uiwidgetp);
  uinbutilIDAdd (manage->mainnbtabid, MANAGE_TAB_MAIN_SEQ);
  uiwcontFree (uiwidgetp);

  /* playlist management */
  manage->managepl = managePlaylistAlloc (manage->wcont [MANAGE_W_WINDOW],
      manage->options, manage->wcont [MANAGE_W_ERROR_MSG]);

  uiwcontFree (vbox);
  vbox = uiCreateVertBox ();
  manageBuildUIPlaylist (manage->managepl, vbox);

  /* CONTEXT: managementui: notebook tab title: playlist management */
  uiwidgetp = uiCreateLabel (_("Playlist Management"));
  uiNotebookAppendPage (manage->wcont [MANAGE_W_MAIN_NB], vbox, uiwidgetp);
  uinbutilIDAdd (manage->mainnbtabid, MANAGE_TAB_MAIN_PL);
  uiwcontFree (uiwidgetp);

  /* music manager */
  manageBuildUIMusicManager (manage);

  /* update database */
  manage->managedb = manageDbAlloc (manage->wcont [MANAGE_W_WINDOW],
      manage->options, manage->wcont [MANAGE_W_STATUS_MSG],
      manage->wcont [MANAGE_W_ERROR_MSG], manage->pleasewaitmsg,
      manage->conn, manage->processes);

  uiwcontFree (vbox);
  vbox = uiCreateVertBox ();
  manageBuildUIUpdateDatabase (manage->managedb, vbox);
  /* CONTEXT: managementui: notebook tab title: update database */
  uiwidgetp = uiCreateLabel (_("Update Database"));
  uiNotebookAppendPage (manage->wcont [MANAGE_W_MAIN_NB], vbox, uiwidgetp);
  uinbutilIDAdd (manage->mainnbtabid, MANAGE_TAB_MAIN_OTHER);
  uiwcontFree (uiwidgetp);

  x = nlistGetNum (manage->options, MANAGE_SIZE_X);
  y = nlistGetNum (manage->options, MANAGE_SIZE_Y);
  uiWindowSetDefaultSize (manage->wcont [MANAGE_W_WINDOW], x, y);

  manage->callbacks [MANAGE_CB_MAIN_NB] = callbackInitLong (
      manageSwitchPageMain, manage);
  uiNotebookSetCallback (manage->wcont [MANAGE_W_MAIN_NB],
      manage->callbacks [MANAGE_CB_MAIN_NB]);

  uiWidgetShowAll (manage->wcont [MANAGE_W_WINDOW]);

  x = nlistGetNum (manage->options, MANAGE_POSITION_X);
  y = nlistGetNum (manage->options, MANAGE_POSITION_Y);
  uiWindowMove (manage->wcont [MANAGE_W_WINDOW], x, y, -1);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_manage", BDJ4_IMG_PNG_EXT, PATHBLD_MP_DIR_IMG);
  osuiSetIcon (imgbuff);

  manage->callbacks [MANAGE_CB_NEW_SEL_SONGSEL] = callbackInitLong (
      manageNewSelectionSongSel, manage);
  uisongselSetSelectionCallback (manage->slsbssongsel,
      manage->callbacks [MANAGE_CB_NEW_SEL_SONGSEL]);
  uisongselSetSelectionCallback (manage->slsongsel,
      manage->callbacks [MANAGE_CB_NEW_SEL_SONGSEL]);
  uisongselSetSelectionCallback (manage->mmsongsel,
      manage->callbacks [MANAGE_CB_NEW_SEL_SONGSEL]);

  manage->callbacks [MANAGE_CB_NEW_SEL_SONGLIST] = callbackInitLong (
      manageNewSelectionSonglist, manage);
  uimusicqSetSelectionCallback (manage->slmusicq,
      manage->callbacks [MANAGE_CB_NEW_SEL_SONGLIST]);
  uimusicqSetSelectionCallback (manage->slsbsmusicq,
      manage->callbacks [MANAGE_CB_NEW_SEL_SONGLIST]);

  manage->callbacks [MANAGE_CB_SEQ_LOAD] = callbackInitStr (
      manageLoadPlaylistCB, manage);
  manageSequenceSetLoadCallback (manage->manageseq,
      manage->callbacks [MANAGE_CB_SEQ_LOAD]);

  manage->callbacks [MANAGE_CB_SEQ_NEW] = callbackInit (
      manageNewPlaylistCB, manage, NULL);
  manageSequenceSetNewCallback (manage->manageseq,
      manage->callbacks [MANAGE_CB_SEQ_NEW]);

  manage->callbacks [MANAGE_CB_PL_LOAD] = callbackInitStr (
      manageLoadSonglistSeqCB, manage);
  managePlaylistSetLoadCallback (manage->managepl,
      manage->callbacks [MANAGE_CB_PL_LOAD]);

  manage->selbypass = false;

  /* set up the initial menu */
  manageSwitchPage (manage, 0, MANAGE_NB_SONGLIST);

  uiwcontFree (hbox);
  uiwcontFree (vbox);

  logProcEnd (LOG_PROC, "manageBuildUI", "");
}

static void
manageInitializeUI (manageui_t *manage)
{
  manage->callbacks [MANAGE_CB_SL_SEL_FILE] =
      callbackInitStr (manageSonglistLoadCB, manage);
  manage->callbacks [MANAGE_CB_CFPL_DIALOG] = callbackInitLong (
      manageCFPLResponseHandler, manage);
  manage->callbacks [MANAGE_CB_ITUNES_DIALOG] = callbackInitLong (
      manageiTunesDialogResponseHandler, manage);
  manage->callbacks [MANAGE_CB_ITUNES_SEL] = callbackInitLong (
      manageiTunesDialogSelectHandler, manage);
  manage->callbacks [MANAGE_CB_BDJ4_EXP] = callbackInit (
      manageExportBDJ4ResponseHandler, manage, NULL);
  manage->callbacks [MANAGE_CB_BDJ4_IMP] = callbackInit (
      manageImportBDJ4ResponseHandler, manage, NULL);

  manage->samesong = samesongAlloc (manage->musicdb);
  manage->uisongfilter = uisfInit (manage->wcont [MANAGE_W_WINDOW], manage->options,
      SONG_FILTER_FOR_SELECTION);

  manage->slplayer = uiplayerInit (manage->progstate, manage->conn,
      manage->musicdb);
  manage->slmusicq = uimusicqInit ("m-songlist", manage->conn,
      manage->musicdb, manage->dispsel, DISP_SEL_SONGLIST);
  uimusicqSetPlayIdx (manage->slmusicq, manage->musicqPlayIdx);
  uimusicqSetManageIdx (manage->slmusicq, manage->musicqManageIdx);
  manage->slstats = manageStatsInit (manage->conn, manage->musicdb);
  manage->slsongsel = uisongselInit ("m-sl-songsel", manage->conn,
      manage->musicdb, manage->dispsel, manage->samesong, manage->options,
      manage->uisongfilter, DISP_SEL_SONGSEL);
  manage->callbacks [MANAGE_CB_PLAY_SL] = callbackInitLongInt (
      managePlayProcessSonglist, manage);
  uisongselSetPlayCallback (manage->slsongsel,
      manage->callbacks [MANAGE_CB_PLAY_SL]);
  manage->callbacks [MANAGE_CB_QUEUE_SL] = callbackInitLong (
      manageQueueProcessSonglist, manage);
  uisongselSetQueueCallback (manage->slsongsel,
      manage->callbacks [MANAGE_CB_QUEUE_SL]);

  manage->slsbsmusicq = uimusicqInit ("m-sbs-songlist", manage->conn,
      manage->musicdb, manage->dispsel, DISP_SEL_SBS_SONGLIST);
  manage->slsbssongsel = uisongselInit ("m-sbs-songsel", manage->conn,
      manage->musicdb, manage->dispsel, manage->samesong, manage->options,
      manage->uisongfilter, DISP_SEL_SBS_SONGSEL);
  uimusicqSetPlayIdx (manage->slsbsmusicq, manage->musicqPlayIdx);
  uimusicqSetManageIdx (manage->slsbsmusicq, manage->musicqManageIdx);
  manage->callbacks [MANAGE_CB_PLAY_SL_SBS] = callbackInitLongInt (
      managePlayProcessEasySonglist, manage);
  uisongselSetPlayCallback (manage->slsbssongsel,
      manage->callbacks [MANAGE_CB_PLAY_SL_SBS]);
  manage->callbacks [MANAGE_CB_QUEUE_SL_SBS] = callbackInitLong (
      manageQueueProcessEasySonglist, manage);
  uisongselSetQueueCallback (manage->slsbssongsel,
      manage->callbacks [MANAGE_CB_QUEUE_SL_SBS]);

  manage->mmplayer = uiplayerInit (manage->progstate, manage->conn,
      manage->musicdb);
  manage->mmmusicq = uimusicqInit ("m-mm-songlist", manage->conn,
      manage->musicdb, manage->dispsel, DISP_SEL_SONGLIST);
  manage->mmsongsel = uisongselInit ("m-mm-songsel", manage->conn,
      manage->musicdb, manage->dispsel, manage->samesong, manage->options,
      manage->uisongfilter, DISP_SEL_MM);
  uimusicqSetPlayIdx (manage->mmmusicq, manage->musicqPlayIdx);
  uimusicqSetManageIdx (manage->mmmusicq, manage->musicqManageIdx);
  manage->callbacks [MANAGE_CB_PLAY_MM] = callbackInitLongInt (
      managePlayProcessMusicManager, manage);
  uisongselSetPlayCallback (manage->mmsongsel,
      manage->callbacks [MANAGE_CB_PLAY_MM]);

  manage->mmsongedit = uisongeditInit (manage->conn,
      manage->musicdb, manage->dispsel, manage->options);

  uisongselSetPeer (manage->mmsongsel, manage->slsbssongsel);
  uisongselSetPeer (manage->mmsongsel, manage->slsongsel);

  uisongselSetPeer (manage->slsongsel, manage->slsbssongsel);
  uisongselSetPeer (manage->slsongsel, manage->mmsongsel);

  uisongselSetPeer (manage->slsbssongsel, manage->mmsongsel);
  uisongselSetPeer (manage->slsbssongsel, manage->slsongsel);

  uimusicqSetPeer (manage->slmusicq, manage->slsbsmusicq);
  uimusicqSetPeer (manage->slsbsmusicq, manage->slmusicq);

  manage->callbacks [MANAGE_CB_EDIT] = callbackInit (
      manageSwitchToSongEditor, manage, NULL);
  uisongselSetEditCallback (manage->slsongsel, manage->callbacks [MANAGE_CB_EDIT]);
  uisongselSetEditCallback (manage->slsbssongsel, manage->callbacks [MANAGE_CB_EDIT]);
  uisongselSetEditCallback (manage->mmsongsel, manage->callbacks [MANAGE_CB_EDIT]);
  uimusicqSetEditCallback (manage->slmusicq, manage->callbacks [MANAGE_CB_EDIT]);
  uimusicqSetEditCallback (manage->slsbsmusicq, manage->callbacks [MANAGE_CB_EDIT]);

  manage->callbacks [MANAGE_CB_SAVE] = callbackInitLong (
      manageSongEditSaveCallback, manage);
  uisongeditSetSaveCallback (manage->mmsongedit, manage->callbacks [MANAGE_CB_SAVE]);
  uisongselSetSongSaveCallback (manage->slsongsel, manage->callbacks [MANAGE_CB_SAVE]);
  uisongselSetSongSaveCallback (manage->slsbssongsel, manage->callbacks [MANAGE_CB_SAVE]);
  uisongselSetSongSaveCallback (manage->mmsongsel, manage->callbacks [MANAGE_CB_SAVE]);
  uimusicqSetSongSaveCallback (manage->slmusicq, manage->callbacks [MANAGE_CB_SAVE]);
  uimusicqSetSongSaveCallback (manage->slsbsmusicq, manage->callbacks [MANAGE_CB_SAVE]);

  manage->uieibdj4 = uieibdj4Init (manage->wcont [MANAGE_W_WINDOW],
      manage->options);
  uieibdj4SetResponseCallback (manage->uieibdj4,
      manage->callbacks [MANAGE_CB_BDJ4_EXP], UIEIBDJ4_EXPORT);
  uieibdj4SetResponseCallback (manage->uieibdj4,
      manage->callbacks [MANAGE_CB_BDJ4_IMP], UIEIBDJ4_IMPORT);
}

static void
manageBuildUISongListEditor (manageui_t *manage)
{
  uibutton_t  *uibutton;
  uiwcont_t   *uip;
  uiwcont_t   *uiwidgetp;
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *mainhbox;

  /* song list editor */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 2);

  /* CONTEXT: managementui: notebook tab title: edit song lists */
  uiwidgetp = uiCreateLabel (_("Edit Song Lists"));
  uiNotebookAppendPage (manage->wcont [MANAGE_W_MAIN_NB], vbox, uiwidgetp);
  uinbutilIDAdd (manage->mainnbtabid, MANAGE_TAB_MAIN_SL);
  uiwcontFree (uiwidgetp);

  /* song list editor: player */
  uiwidgetp = uiplayerBuildUI (manage->slplayer);
  uiBoxPackStart (vbox, uiwidgetp);

  manage->wcont [MANAGE_W_SONGLIST_NB] = uiCreateNotebook ();
  uiBoxPackStartExpand (vbox, manage->wcont [MANAGE_W_SONGLIST_NB]);

  /* song list editor: side-by-side view tab */
  mainhbox = uiCreateHorizBox ();

  /* CONTEXT: managementui: name of side-by-side view tab */
  uiwidgetp = uiCreateLabel (_("Song List"));
  uiNotebookAppendPage (manage->wcont [MANAGE_W_SONGLIST_NB], mainhbox, uiwidgetp);
  uinbutilIDAdd (manage->slnbtabid, MANAGE_TAB_SONGLIST);
  manage->wcont [MANAGE_W_SL_SBS_MUSICQ_TAB] = mainhbox;
  uiwcontFree (uiwidgetp);

  hbox = uiCreateHorizBox ();
  uiBoxPackStartExpand (mainhbox, hbox);

  uiwidgetp = uimusicqBuildUI (manage->slsbsmusicq, manage->wcont [MANAGE_W_WINDOW], MUSICQ_SL,
      manage->wcont [MANAGE_W_STATUS_MSG], uiutilsValidatePlaylistName);
  uiBoxPackStartExpand (hbox, uiwidgetp);

  uiwcontFree (vbox);
  vbox = uiCreateVertBox ();

  uiWidgetSetAllMargins (vbox, 4);
  uiWidgetSetMarginTop (vbox, 64);
  uiBoxPackStart (hbox, vbox);

  manage->callbacks [MANAGE_CB_SBS_SELECT] = callbackInit (
      uisongselSelectCallback, manage->slsbssongsel, NULL);
  uibutton = uiCreateButton (
      manage->callbacks [MANAGE_CB_SBS_SELECT],
      /* CONTEXT: managementui: config: button: add the selected songs to the song list */
      _("Select"), "button_left");
  manage->selectButton = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackStart (vbox, uiwidgetp);

  uiwidgetp = uisongselBuildUI (manage->slsbssongsel, manage->wcont [MANAGE_W_WINDOW]);
  uiBoxPackStartExpand (hbox, uiwidgetp);

  /* song list editor: music queue tab */
  uip = uimusicqBuildUI (manage->slmusicq, manage->wcont [MANAGE_W_WINDOW], MUSICQ_SL,
      manage->wcont [MANAGE_W_STATUS_MSG], uiutilsValidatePlaylistName);
  /* CONTEXT: managementui: name of song list notebook tab */
  uiwidgetp = uiCreateLabel (_("Song List"));
  uiNotebookAppendPage (manage->wcont [MANAGE_W_SONGLIST_NB], uip, uiwidgetp);
  uinbutilIDAdd (manage->slnbtabid, MANAGE_TAB_SONGLIST);
  manage->wcont [MANAGE_W_SL_MUSICQ_TAB] = uip;
  uiwcontFree (uiwidgetp);

  /* song list editor: song selection tab */
  uip = uisongselBuildUI (manage->slsongsel, manage->wcont [MANAGE_W_WINDOW]);
  /* CONTEXT: managementui: name of song selection notebook tab */
  uiwidgetp = uiCreateLabel (_("Song Selection"));
  uiNotebookAppendPage (manage->wcont [MANAGE_W_SONGLIST_NB], uip, uiwidgetp);
  uinbutilIDAdd (manage->slnbtabid, MANAGE_TAB_OTHER);
  manage->wcont [MANAGE_W_SONGSEL_TAB] = uip;
  uiwcontFree (uiwidgetp);

  uimusicqPeerSonglistName (manage->slmusicq, manage->slsbsmusicq);

  /* song list editor: statistics tab */
  uip = manageBuildUIStats (manage->slstats);
  /* CONTEXT: managementui: name of statistics tab */
  uiwidgetp = uiCreateLabel (_("Statistics"));
  uiNotebookAppendPage (manage->wcont [MANAGE_W_SONGLIST_NB], uip, uiwidgetp);
  uinbutilIDAdd (manage->slnbtabid, MANAGE_TAB_STATISTICS);
  uiwcontFree (uiwidgetp);

  manage->callbacks [MANAGE_CB_SL_NB] = callbackInitLong (
      manageSwitchPageSonglist, manage);
  uiNotebookSetCallback (manage->wcont [MANAGE_W_SONGLIST_NB], manage->callbacks [MANAGE_CB_SL_NB]);

  uiwcontFree (vbox);
  uiwcontFree (hbox);
}

static int
manageMainLoop (void *tmanage)
{
  manageui_t   *manage = tmanage;
  int         stop = false;

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

  if (manage->expimpbdj4state == BDJ4_STATE_PROCESS) {
    if (mstimeCheck (&manage->eibdj4ChkTime)) {
      int   count, tot;

      eibdj4GetCount (manage->eibdj4, &count, &tot);
      uiutilsProgressStatus (manage->wcont [MANAGE_W_STATUS_MSG], count, tot);
      uieibdj4UpdateStatus (manage->uieibdj4, count, tot);
      mstimeset (&manage->eibdj4ChkTime, 200);
    }

    if (eibdj4Process (manage->eibdj4)) {
      if (eibdj4DatabaseChanged (manage->eibdj4)) {
        manageProcessDatabaseUpdate (manage);
      }
      uiutilsProgressStatus (manage->wcont [MANAGE_W_STATUS_MSG], -1, -1);
      uieibdj4UpdateStatus (manage->uieibdj4, -1, -1);
      manageSonglistLoadFile (manage,
          uieibdj4GetNewName (manage->uieibdj4), MANAGE_STD);
      uieibdj4DialogClear (manage->uieibdj4);
      manage->expimpbdj4state = BDJ4_STATE_OFF;
    }
  }

  if (manage->expimpbdj4state == BDJ4_STATE_START) {
    uiLabelSetText (manage->wcont [MANAGE_W_STATUS_MSG], manage->pleasewaitmsg);
    manage->expimpbdj4state = BDJ4_STATE_PROCESS;
  }

  uieibdj4Process (manage->uieibdj4);
  manageDbProcess (manage->managedb);
  uicopytagsProcess (manage->uict);

  /* copy tags processing */

  if (manage->ctstate == BDJ4_STATE_PROCESS) {
    int   state;

    state = uicopytagsState (manage->uict);
    if (state == BDJ4_STATE_FINISH) {
      const char  *outfn;
      const char  *tfn;
      song_t      *song;


      outfn = uicopytagsGetFilename (manage->uict);
      tfn = songutilGetRelativePath (outfn);
      song = dbGetByName (manage->musicdb, tfn);
      if (song != NULL) {
        void        *data;
        dbidx_t     rrn;
        dbidx_t     dbidx;
        char        tmp [40];

        rrn = songGetNum (song, TAG_RRN);
        dbidx = songGetNum (song, TAG_DBIDX);
        data = audiotagReadTags (outfn);
        if (data != NULL) {
          slist_t     *tagdata;
          int         rewrite;

          tagdata = audiotagParseData (outfn, data, &rewrite);
          if (slistGetCount (tagdata) > 0) {
            dbWrite (manage->musicdb, tfn, tagdata, rrn);
          }

          dbLoadEntry (manage->musicdb, dbidx);
          manageRePopulateData (manage);
          if (manage->songeditdbidx == dbidx) {
            manageReloadSongEdit (manage);
          }
          snprintf (tmp, sizeof (tmp), "%d", dbidx);
          connSendMessage (manage->conn, ROUTE_STARTERUI, MSG_DB_ENTRY_UPDATE, tmp);
        }
      }
      // send the db entry update message
      //   (see apply adj processing below)
    }
    if (state != BDJ4_STATE_WAIT) {
      manage->ctstate = BDJ4_STATE_OFF;
      uicopytagsReset (manage->uict);
    }
  }

  if (manage->ctstate == BDJ4_STATE_START) {
    manage->ctstate = BDJ4_STATE_PROCESS;
  }

  /* apply adjustments processing */

  if (manage->applyadjstate == BDJ4_STATE_PROCESS) {
    bool    changed = false;

    changed = aaApplyAdjustments (manage->musicdb, manage->songeditdbidx, manage->aaflags);

    if (changed) {
      char    tmp [40];

      manageRePopulateData (manage);
      manageReloadSongEdit (manage);

      snprintf (tmp, sizeof (tmp), "%d", manage->songeditdbidx);
      connSendMessage (manage->conn, ROUTE_STARTERUI, MSG_DB_ENTRY_UPDATE, tmp);
    }
    uiLabelSetText (manage->wcont [MANAGE_W_STATUS_MSG], "");
    uiaaDialogClear (manage->uiaa);
    manage->applyadjstate = BDJ4_STATE_OFF;
  }

  if (manage->applyadjstate == BDJ4_STATE_START) {
    uiLabelSetText (manage->wcont [MANAGE_W_STATUS_MSG], manage->pleasewaitmsg);
    manage->applyadjstate = BDJ4_STATE_PROCESS;
  }

  /* itunes processing */

  if (manage->impitunesstate == BDJ4_STATE_PROCESS) {
    manageSonglistSave (manage);

    if (manage->itunes == NULL) {
      manage->itunes = itunesAlloc ();
    }

    itunesParse (manage->itunes);

    /* CONTEXT: managementui: song list: default name for a new song list */
    manageSetSonglistName (manage, _("New Song List"));

    manageiTunesCreateDialog (manage);
    uiDialogShow (manage->wcont [MANAGE_W_ITUNES_SEL_DIALOG]);

    uiLabelSetText (manage->wcont [MANAGE_W_STATUS_MSG], "");
    manage->impitunesstate = BDJ4_STATE_OFF;
  }

  if (manage->impitunesstate == BDJ4_STATE_START) {
    uiLabelSetText (manage->wcont [MANAGE_W_STATUS_MSG], manage->pleasewaitmsg);
    manage->impitunesstate = BDJ4_STATE_PROCESS;
  }

  /* create from playlist post process */
  if (manage->cfplpostprocess && manage->musicqupdated) {
    manageCFPLPostProcess (manage);
  }

  uiplayerMainLoop (manage->slplayer);
  uiplayerMainLoop (manage->mmplayer);
  uimusicqMainLoop (manage->slmusicq);
  uisongselMainLoop (manage->slsongsel);
  uimusicqMainLoop (manage->slsbsmusicq);
  uisongselMainLoop (manage->slsbssongsel);
  uimusicqMainLoop (manage->mmmusicq);
  uisongselMainLoop (manage->mmsongsel);

  if (manage->mainlasttab == MANAGE_TAB_MAIN_MM) {
    if (manage->mmlasttab == MANAGE_TAB_SONGEDIT) {
      /* the song edit main loop does not need to run all the time */
      uisongeditMainLoop (manage->mmsongedit);
    }
    if (manage->mmlasttab == MANAGE_TAB_AUDIOID) {
      /* the audio id main loop does not need to run all the time */
      manageAudioIdMainLoop (manage->manageaudioid);
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
    char    tmp [40];

    snprintf (tmp, sizeof (tmp), "%d", ROUTE_PLAYERUI);
    connSendMessage (manage->conn, ROUTE_STARTERUI, MSG_REQ_PROCESS_ACTIVE, tmp);
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

  if (msg != MSG_PLAYER_STATUS_DATA) {
    logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
        routefrom, msgRouteDebugText (routefrom),
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
  }

  targs = mdstrdup (args);

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
            procutilFree (manage->processes [ROUTE_BPM_COUNTER]);
            manage->processes [ROUTE_BPM_COUNTER] = NULL;
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
        case MSG_WINDOW_FIND: {
          uiWindowFind (manage->wcont [MANAGE_W_WINDOW]);
          break;
        }
        case MSG_DB_WAIT: {
          uiLabelSetText (manage->wcont [MANAGE_W_STATUS_MSG], manage->pleasewaitmsg);
          break;
        }
        case MSG_DB_WAIT_FINISH: {
          uiLabelSetText (manage->wcont [MANAGE_W_STATUS_MSG], "");
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
          /* the currently open database pointer is no longer valid */
          /* if a rebuild or a compact was run */

          /* the database has been updated, tell the other processes to */
          /* reload it, and reload it ourselves */
          manageProcessDatabaseUpdate (manage);
          manageDbResetButtons (manage->managedb);
          break;
        }
        case MSG_MUSIC_QUEUE_DATA: {
          mp_musicqupdate_t  *musicqupdate;

          musicqupdate = msgparseMusicQueueData (targs);
          if (musicqupdate->mqidx == manage->musicqManageIdx) {
            uimusicqProcessMusicQueueData (manage->slmusicq, musicqupdate);
            uimusicqProcessMusicQueueData (manage->slsbsmusicq, musicqupdate);
            uimusicqProcessMusicQueueData (manage->mmmusicq, musicqupdate);
            manageStatsProcessData (manage->slstats, musicqupdate);
          }
          /* the music queue data is used to display the mark */
          /* indicating that the song is already in the song list */
          if (musicqupdate->mqidx == manage->musicqManageIdx) {
            uisongselProcessMusicQueueData (manage->slsongsel, musicqupdate);
            uisongselProcessMusicQueueData (manage->slsbssongsel, musicqupdate);
          }
          msgparseMusicQueueDataFree (musicqupdate);
          if (uimusicqGetCount (manage->slmusicq) > 0) {
            manage->musicqupdated = true;
          }
          manage->musicqueueprocessflag = true;
          break;
        }
        case MSG_SONG_SELECT: {
          mp_songselect_t   *songselect;

          songselect = msgparseSongSelect (targs);
          /* the display is offset by 1, as the 0 index is the current song */
          --songselect->loc;
          uimusicqProcessSongSelect (manage->slmusicq, songselect);
          uimusicqProcessSongSelect (manage->slsbsmusicq, songselect);
          uimusicqProcessSongSelect (manage->mmmusicq, songselect);
          msgparseSongSelectFree (songselect);
          break;
        }
        case MSG_PROCESS_ACTIVE: {
          int   val;
          char  tmp [40];


          val = atoi (targs);
          manage->pluiActive = val;
          uimusicqSetPlayButtonState (manage->slmusicq, val);
          uisongselSetPlayButtonState (manage->slsongsel, val);
          uimusicqSetPlayButtonState (manage->slsbsmusicq, val);
          uisongselSetPlayButtonState (manage->slsbssongsel, val);
          uisongselSetPlayButtonState (manage->mmsongsel, val);
          uisongeditSetPlayButtonState (manage->mmsongedit, val);
          if (! manage->pluiActive) {
            snprintf (tmp, sizeof (tmp), "%d", MUSICQ_MNG_PB);
            connSendMessage (manage->conn, ROUTE_MAIN, MSG_MUSICQ_SET_PLAYBACK, tmp);
            connSendMessage (manage->conn, ROUTE_MAIN, MSG_QUEUE_SWITCH_EMPTY, "0");
            snprintf (tmp, sizeof (tmp), "%d", SONG_LIST_LIMIT);
            connSendMessage (manage->conn, ROUTE_MAIN, MSG_MUSICQ_SET_LEN, tmp);
          }
          break;
        }
        case MSG_DB_ENTRY_UPDATE: {
          dbLoadEntry (manage->musicdb, atol (targs));
          manageRePopulateData (manage);
          manageReloadSongEdit (manage);
          break;
        }
        case MSG_PROCESSING_FAIL: {
          /* CONTEXT: the 'Mix' action failed */
          uiLabelSetText (manage->wcont [MANAGE_W_STATUS_MSG], _("Action failed"));
          break;
        }
        case MSG_PROCESSING_FINISH: {
          uiLabelSetText (manage->wcont [MANAGE_W_STATUS_MSG], "");
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

  mdfree (targs);

  uiplayerProcessMsg (routefrom, route, msg, args, manage->slplayer);
  uiplayerProcessMsg (routefrom, route, msg, args, manage->mmplayer);
  uisongeditProcessMsg (routefrom, route, msg, args, manage->mmsongedit);

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
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
  uiwcont_t   *menu = NULL;
  uiwcont_t   *menuitem = NULL;
  const char  *p;
  void        *tempp;

  logProcBegin (LOG_PROC, "manageSongEditMenu");
  if (! uiMenuInitialized (manage->songeditmenu)) {
    manage->uict = uicopytagsInit (manage->wcont [MANAGE_W_WINDOW], manage->options);
    manage->uiaa = uiaaInit (manage->wcont [MANAGE_W_WINDOW], manage->options);
    manage->callbacks [MANAGE_CB_APPLY_ADJ] = callbackInitLong (
        manageApplyAdjCallback, manage);
    uiaaSetResponseCallback (manage->uiaa, manage->callbacks [MANAGE_CB_APPLY_ADJ]);

    menuitem = uiMenuAddMainItem (manage->wcont [MANAGE_W_MENUBAR],
        /* CONTEXT: managementui: menu selection: actions for song editor */
        manage->songeditmenu, _("Actions"));
    menu = uiCreateSubMenu (menuitem);
    uiwcontFree (menuitem);

    manage->callbacks [MANAGE_MENU_CB_SE_BPM] = callbackInit (
        manageStartBPMCounter, manage, NULL);
    menuitem = uiMenuCreateItem (menu, tagdefs [TAG_BPM].displayname,
        manage->callbacks [MANAGE_MENU_CB_SE_BPM]);
    uiwcontFree (menuitem);

    manage->callbacks [MANAGE_MENU_CB_SE_COPY_TAGS] = callbackInit (
        manageCopyTagsStart, manage, NULL);
    /* CONTEXT: managementui: song editor menu: copy audio tags */
    menuitem = uiMenuCreateItem (menu, _("Copy Audio Tags"),
        manage->callbacks [MANAGE_MENU_CB_SE_COPY_TAGS]);
    uiwcontFree (menuitem);

    uiMenuAddSeparator (menu);

    manage->callbacks [MANAGE_MENU_CB_SE_START_EDIT_ALL] = callbackInit (
        manageEditAllStart, manage, NULL);
    /* CONTEXT: managementui: song editor menu: edit all */
    menuitem = uiMenuCreateItem (menu, _("Edit All"),
        manage->callbacks [MANAGE_MENU_CB_SE_START_EDIT_ALL]);
    uiwcontFree (menuitem);

    manage->callbacks [MANAGE_MENU_CB_SE_APPLY_EDIT_ALL] = callbackInit (
        manageEditAllApply, manage, NULL);
    /* CONTEXT: managementui: song editor menu: apply edit all */
    menuitem = uiMenuCreateItem (menu, _("Apply Edit All"),
        manage->callbacks [MANAGE_MENU_CB_SE_APPLY_EDIT_ALL]);
    uiwcontFree (menuitem);

    manage->callbacks [MANAGE_MENU_CB_SE_CANCEL_EDIT_ALL] = callbackInit (
        manageEditAllCancel, manage, NULL);
    /* CONTEXT: managementui: song editor menu: cancel edit all */
    menuitem = uiMenuCreateItem (menu, _("Cancel Edit All"),
        manage->callbacks [MANAGE_MENU_CB_SE_CANCEL_EDIT_ALL]);
    uiwcontFree (menuitem);

    uiMenuAddSeparator (menu);

    manage->callbacks [MANAGE_MENU_CB_SE_APPLY_ADJ] = callbackInit (
        manageApplyAdjDialog, manage, NULL);
    uisongeditSetApplyAdjCallback (manage->mmsongedit, manage->callbacks [MANAGE_MENU_CB_SE_APPLY_ADJ]);
    /* CONTEXT: managementui: song editor menu: apply adjustments */
    menuitem = uiMenuCreateItem (menu, _("Apply Adjustments"),
        manage->callbacks [MANAGE_MENU_CB_SE_APPLY_ADJ]);
    p = sysvarsGetStr (SV_PATH_FFMPEG);
    if (p == NULL || ! *p) {
      uiWidgetSetState (menuitem, UIWIDGET_DISABLE);
    }
    uiwcontFree (menuitem);

    manage->callbacks [MANAGE_MENU_CB_SE_RESTORE_ORIG] = callbackInit (
        manageRestoreOrigCallback, manage, NULL);
    /* CONTEXT: managementui: song editor menu: restore original */
    menuitem = uiMenuCreateItem (menu, _("Restore Original"),
        manage->callbacks [MANAGE_MENU_CB_SE_RESTORE_ORIG]);
    manage->wcont [MANAGE_W_MENU_RESTORE_ORIG] = menuitem;
    if (manage->enablerestoreorig) {
      uiWidgetSetState (menuitem, UIWIDGET_ENABLE);
    } else {
      uiWidgetSetState (menuitem, UIWIDGET_DISABLE);
    }
    /* a missing audio adjust file will not stop startup */
    tempp = bdjvarsdfGet (BDJVDF_AUDIO_ADJUST);
    if (tempp == NULL) {
      uiWidgetSetState (menuitem, UIWIDGET_DISABLE);
    }
    /* do not free this menu item here */

    uiMenuSetInitialized (manage->songeditmenu);
    uiwcontFree (menu);
  }

  uiMenuDisplay (manage->songeditmenu);
  manage->currmenu = manage->songeditmenu;

  logProcEnd (LOG_PROC, "manageSongEditMenu", "");
}

static bool
manageNewSelectionSongSel (void *udata, long dbidx)
{
  manageui_t  *manage = udata;
  song_t      *song = NULL;

  logProcBegin (LOG_PROC, "manageNewSelectionSongSel");
  if (manage->selbypass) {
    logProcEnd (LOG_PROC, "manageNewSelectionSongSel", "sel-bypass");
    return UICB_CONT;
  }

  if (dbidx < 0) {
    logProcEnd (LOG_PROC, "manageNewSelectionSongSel", "bad-dbidx");
    return UICB_CONT;
  }

  if (manage->mainlasttab != MANAGE_TAB_MAIN_MM) {
    manage->selusesonglist = false;
  }
  manage->seldbidx = dbidx;

  manage->songeditdbidx = dbidx;
  manageReloadSongEdit (manage);
  song = dbGetByIdx (manage->musicdb, dbidx);
  manageSetBPMCounter (manage, song);

  logProcEnd (LOG_PROC, "manageNewSelectionSongSel", "");
  return UICB_CONT;
}

static bool
manageNewSelectionSonglist (void *udata, long dbidx)
{
  manageui_t  *manage = udata;

  logProcBegin (LOG_PROC, "manageNewSelectionSonglist");
  if (manage->selbypass) {
    logProcEnd (LOG_PROC, "manageNewSelectionSonglist", "sel-bypass");
    return UICB_CONT;
  }

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: select within song list");
  manage->selusesonglist = true;
  manage->songlistdbidx = dbidx;

  logProcEnd (LOG_PROC, "manageNewSelectionSonglist", "");
  return UICB_CONT;
}

static bool
manageSwitchToSongEditor (void *udata)
{
  manageui_t  *manage = udata;
  int         pagenum;

  logProcBegin (LOG_PROC, "manageSwitchToSongEditor");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: switch to song editor");
  if (manage->mainlasttab != MANAGE_TAB_MAIN_MM) {
    /* switching to the music manager tab will also apply the appropriate */
    /* song filter and load the editor */
    pagenum = uinbutilIDGetPage (manage->mainnbtabid, MANAGE_TAB_MAIN_MM);
    uiNotebookSetPage (manage->wcont [MANAGE_W_MAIN_NB], pagenum);
  }
  if (manage->mmlasttab != MANAGE_TAB_SONGEDIT) {
    pagenum = uinbutilIDGetPage (manage->mmnbtabid, MANAGE_TAB_SONGEDIT);
    uiNotebookSetPage (manage->wcont [MANAGE_W_MM_NB], pagenum);
  }

  /* the notebook-switch-page callback will load the song editor data */

  logProcEnd (LOG_PROC, "manageSwitchToSongEditor", "");
  return UICB_CONT;
}

static bool
manageSongEditSaveCallback (void *udata, long dbidx)
{
  manageui_t  *manage = udata;
  char        tmp [40];

  logProcBegin (LOG_PROC, "manageSongEditSaveCallback");

  if (manage->dbchangecount > MANAGE_DB_COUNT_SAVE) {
    dbBackup ();
    manage->dbchangecount = 0;
  }

  /* this fetches the song from in-memory, which has already been updated */
  songWriteDB (manage->musicdb, dbidx);

  /* the database has been updated, tell the other processes to reload  */
  /* this particular entry */
  snprintf (tmp, sizeof (tmp), "%ld", dbidx);
  connSendMessage (manage->conn, ROUTE_STARTERUI, MSG_DB_ENTRY_UPDATE, tmp);

  manageRePopulateData (manage);

  /* re-load the song into the song editor */
  /* it is unknown if called from saving a favorite or from the song editor */
  /* the overhead is minor */
  manage->songeditdbidx = dbidx;
  manageReloadSongEdit (manage);

  ++manage->dbchangecount;

  logProcEnd (LOG_PROC, "manageSongEditSaveCallback", "");
  return UICB_CONT;
}

static void
manageRePopulateData (manageui_t *manage)
{
  /* re-populate the song selection displays to display the updated info */
  manage->selbypass = true;
  uisongselPopulateData (manage->slsongsel);
  uisongselPopulateData (manage->slsbssongsel);
  uisongselPopulateData (manage->mmsongsel);
  manage->selbypass = false;
}


static void
manageSetEditMenuItems (manageui_t *manage)
{
  song_t    *song;
  bool      hasorig;

  song = dbGetByIdx (manage->musicdb, manage->songeditdbidx);
  hasorig = songutilHasOriginal (songGetStr (song, TAG_URI));
  if (hasorig) {
    manage->enablerestoreorig = true;
  } else {
    manage->enablerestoreorig = false;
  }
  if (manage->wcont [MANAGE_W_MENU_RESTORE_ORIG] == NULL) {
    return;
  }
  if (hasorig) {
    uiWidgetSetState (manage->wcont [MANAGE_W_MENU_RESTORE_ORIG], UIWIDGET_ENABLE);
  } else {
    uiWidgetSetState (manage->wcont [MANAGE_W_MENU_RESTORE_ORIG], UIWIDGET_DISABLE);
  }
}

static bool
manageApplyAdjDialog (void *udata)
{
  manageui_t    *manage = udata;
  bool          rc;
  bool          hasorig;
  song_t        *song;

  if (manage->songeditdbidx < 0) {
    return UICB_STOP;
  }

  song = dbGetByIdx (manage->musicdb, manage->songeditdbidx);
  hasorig = songutilHasOriginal (songGetStr (song, TAG_URI));
  rc = uiaaDialog (manage->uiaa, songGetNum (song, TAG_ADJUSTFLAGS), hasorig);
  return rc;
}

static bool
manageApplyAdjCallback (void *udata, long aaflags)
{
  manageui_t  *manage = udata;

  if (manage->songeditdbidx < 0) {
    return UICB_STOP;
  }

  manage->aaflags = aaflags;
  manage->applyadjstate = BDJ4_STATE_START;

  return UICB_CONT;
}

static bool
manageRestoreOrigCallback (void *udata)
{
  manageui_t    *manage = udata;

  if (manage->songeditdbidx < 0) {
    return UICB_STOP;
  }

  manage->aaflags = SONG_ADJUST_RESTORE;
  manage->applyadjstate = BDJ4_STATE_START;

  return UICB_CONT;
}

static bool
manageCopyTagsStart (void *udata)
{
  manageui_t  *manage = udata;
  int         rc;

  rc = uicopytagsDialog (manage->uict);
  manage->ctstate = BDJ4_STATE_START;
  return rc;
}

/* edit all */

static bool
manageEditAllStart (void *udata)
{
  manageui_t  *manage = udata;

  /* do this to make sure any changes to non edit-all fields are reverted */
  manageReloadSongEdit (manage);
  uisongeditClearChanged (manage->mmsongedit, UISONGEDIT_EDITALL);
  uisongeditEditAllSetFields (manage->mmsongedit, UISONGEDIT_EDITALL_ON);
  manage->ineditall = true;

  return UICB_CONT;
}

static bool
manageEditAllApply (void *udata)
{
  manageui_t  *manage = udata;

  if (! manage->ineditall) {
    return UICB_STOP;
  }
  uisongeditEditAllApply (manage->mmsongedit);
  uisongeditEditAllSetFields (manage->mmsongedit, UISONGEDIT_EDITALL_OFF);
  manage->ineditall = false;

  return UICB_CONT;
}

static bool
manageEditAllCancel (void *udata)
{
  manageui_t  *manage = udata;

  if (! manage->ineditall) {
    return UICB_STOP;
  }

  /* revert any changes */
  manageReloadSongEdit (manage);
  uisongeditClearChanged (manage->mmsongedit, UISONGEDIT_ALL);
  uisongeditEditAllSetFields (manage->mmsongedit, UISONGEDIT_EDITALL_OFF);
  manage->ineditall = false;

  return UICB_CONT;
}

static void
manageReloadSongEdit (manageui_t *manage)
{
  song_t    *song;

  song = dbGetByIdx (manage->musicdb, manage->songeditdbidx);
  if (manage->mmlasttab == MANAGE_TAB_SONGEDIT) {
    uisongeditLoadData (manage->mmsongedit, song,
        manage->songeditdbidx, UISONGEDIT_ALL);
  }
  if (manage->mmlasttab == MANAGE_TAB_AUDIOID) {
    manageAudioIdLoad (manage->manageaudioid, song, manage->songeditdbidx);
  }
  manageSetEditMenuItems (manage);
}

/* bpm counter */

static bool
manageStartBPMCounter (void *udata)
{
  manageui_t  *manage = udata;
  const char  *targv [2];
  int         targc = 0;

  logProcBegin (LOG_PROC, "manageStartBPMCounter");

  if (manage->bpmcounterstarted) {
    connSendMessage (manage->conn, ROUTE_BPM_COUNTER, MSG_WINDOW_FIND, NULL);
    return UICB_CONT;
  }

  if (! manage->bpmcounterstarted) {
    int     flags;

    flags = PROCUTIL_DETACH;
    targv [targc++] = NULL;

    manage->processes [ROUTE_BPM_COUNTER] = procutilStartProcess (
        ROUTE_MAIN, "bdj4bpmcounter", flags, targv);
  }

  manageSendBPMCounter (manage);
  logProcEnd (LOG_PROC, "manageStartBPMCounter", "");
  return UICB_CONT;
}

static void
manageSetBPMCounter (manageui_t *manage, song_t *song)
{
  ilistidx_t  danceIdx;

  logProcBegin (LOG_PROC, "manageSetBPMCounter");

  danceIdx = songGetNum (song, TAG_DANCE);
  if (danceIdx < 0) {
    return;
  }
  manage->currtimesig = danceGetTimeSignature (danceIdx);

  manageSendBPMCounter (manage);
  logProcEnd (LOG_PROC, "manageSetBPMCounter", "");
}

static void
manageSendBPMCounter (manageui_t *manage)
{
  char        tbuff [60];

  logProcBegin (LOG_PROC, "manageSendBPMCounter");
  if (! manage->bpmcounterstarted) {
    logProcEnd (LOG_PROC, "manageSendBPMCounter", "not-started");
    return;
  }

  snprintf (tbuff, sizeof (tbuff), "%d", manage->currtimesig);
  connSendMessage (manage->conn, ROUTE_BPM_COUNTER, MSG_BPM_TIMESIG, tbuff);
  logProcEnd (LOG_PROC, "manageSendBPMCounter", "");
}

/* itunes */

static bool
manageSonglistImportiTunes (void *udata)
{
  manageui_t  *manage = udata;
  char        tbuff [MAXPATHLEN];

  if (manage->importitunesactive) {
    return UICB_STOP;
  }

  manage->importitunesactive = true;
  uiLabelSetText (manage->wcont [MANAGE_W_ERROR_MSG], "");
  uiLabelSetText (manage->wcont [MANAGE_W_STATUS_MSG], "");
  if (! itunesConfigured ()) {
    /* CONTEXT: manage ui: status message: itunes is not configured */
    snprintf (tbuff, sizeof (tbuff), _("%s is not configured."), ITUNES_NAME);
    uiLabelSetText (manage->wcont [MANAGE_W_ERROR_MSG], tbuff);
    return UICB_CONT;
  }

  logProcBegin (LOG_PROC, "manageSonglistImportiTunes");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: import itunes");
  manage->impitunesstate = BDJ4_STATE_START;

  logProcEnd (LOG_PROC, "manageSonglistImportiTunes", "");
  return UICB_CONT;
}

static void
manageiTunesCreateDialog (manageui_t *manage)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *uiwidgetp;
  char        tbuff [50];

  logProcBegin (LOG_PROC, "manageiTunesCreateDialog");
  if (manage->wcont [MANAGE_W_ITUNES_SEL_DIALOG] != NULL) {
    logProcEnd (LOG_PROC, "manageiTunesCreateDialog", "already");
    return;
  }

  /* CONTEXT: import from itunes: title for the dialog */
  snprintf (tbuff, sizeof (tbuff), _("Import from %s"), ITUNES_NAME);
  manage->wcont [MANAGE_W_ITUNES_SEL_DIALOG] = uiCreateDialog (manage->wcont [MANAGE_W_WINDOW],
      manage->callbacks [MANAGE_CB_ITUNES_DIALOG],
      tbuff,
      /* CONTEXT: import from itunes: closes the dialog */
      _("Close"),
      RESPONSE_CLOSE,
      /* CONTEXT: import from itunes: import the selected itunes playlist */
      _("Import"),
      RESPONSE_APPLY,
      NULL
      );

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);
  uiDialogPackInDialog (manage->wcont [MANAGE_W_ITUNES_SEL_DIALOG], vbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: import from itunes: select the itunes playlist to use (iTunes Playlist) */
  snprintf (tbuff, sizeof (tbuff), _("%s Playlist"), ITUNES_NAME);
  uiwidgetp = uiCreateColonLabel (tbuff);
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiComboboxCreate (manage->itunessel,
      manage->wcont [MANAGE_W_ITUNES_SEL_DIALOG], "",
      manage->callbacks [MANAGE_CB_ITUNES_SEL], manage);
  manageiTunesDialogCreateList (manage);
  uiBoxPackStart (hbox, uiwidgetp);

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  uiwcontFree (vbox);
  uiwcontFree (hbox);

  logProcEnd (LOG_PROC, "manageiTunesCreateDialog", "");
}

static void
manageiTunesDialogCreateList (manageui_t *manage)
{
  slist_t     *pllist;
  const char  *skey;

  logProcBegin (LOG_PROC, "manageiTunesCreateList");

  pllist = slistAlloc ("itunes-pl-sel", LIST_ORDERED, NULL);
  itunesStartIteratePlaylists (manage->itunes);
  while ((skey = itunesIteratePlaylists (manage->itunes)) != NULL) {
    slistSetStr (pllist, skey, skey);
  }
  slistCalcMaxWidth (pllist);
  /* what text is best to use for 'no selection'? */
  uiDropDownSetList (manage->itunessel, pllist, "");
  slistFree (pllist);
  logProcEnd (LOG_PROC, "manageiTunesCreateList", "");
}

static bool
manageiTunesDialogSelectHandler (void *udata, long idx)
{
  return UICB_CONT;
}

static bool
manageiTunesDialogResponseHandler (void *udata, long responseid)
{
  manageui_t  *manage = udata;

  logProcBegin (LOG_PROC, "manageiTunesResponseHandler");

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: itunes: del window");
      uiwcontFree (manage->wcont [MANAGE_W_ITUNES_SEL_DIALOG]);
      manage->wcont [MANAGE_W_ITUNES_SEL_DIALOG] = NULL;
      manage->importitunesactive = false;
      break;
    }
    case RESPONSE_CLOSE: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: itunes: close window");
      uiWidgetHide (manage->wcont [MANAGE_W_ITUNES_SEL_DIALOG]);
      manage->importitunesactive = false;
      break;
    }
    case RESPONSE_APPLY: {
      const char  *plname;
      nlist_t     *ids;
      nlistidx_t  iteridx;
      nlistidx_t  ituneskey;
      nlist_t     *idata;
      const char  *songfn;
      song_t      *song;
      dbidx_t     dbidx;
      char        tbuff [200];

      logMsg (LOG_DBG, LOG_ACTIONS, "= action: itunes: import");
      plname = uiDropDownGetString (manage->itunessel);
      ids = itunesGetPlaylistData (manage->itunes, plname);
      snprintf (tbuff, sizeof (tbuff), "%d", manage->musicqManageIdx);
      connSendMessage (manage->conn, ROUTE_MAIN, MSG_QUEUE_CLEAR, tbuff);

      nlistStartIterator (ids, &iteridx);
      while ((ituneskey = nlistIterateKey (ids, &iteridx)) >= 0) {
        idata = itunesGetSongData (manage->itunes, ituneskey);
        songfn = nlistGetStr (idata, TAG_URI);
        song = dbGetByName (manage->musicdb, songfn);
        if (song != NULL) {
          dbidx = songGetNum (song, TAG_DBIDX);
          snprintf (tbuff, sizeof (tbuff), "%d%c%d%c%d",
              manage->musicqManageIdx, MSG_ARGS_RS, QUEUE_LOC_LAST, MSG_ARGS_RS, dbidx);
          connSendMessage (manage->conn, ROUTE_MAIN, MSG_MUSICQ_INSERT, tbuff);
        } else {
          logMsg (LOG_DBG, LOG_INFO, "itunes import: song not found %s", songfn);
        }
      }

      manageSetSonglistName (manage, plname);
      uiWidgetHide (manage->wcont [MANAGE_W_ITUNES_SEL_DIALOG]);
      manage->importitunesactive = false;
      break;
    }
  }

  logProcEnd (LOG_PROC, "manageiTunesResponseHandler", "");
  return UICB_CONT;
}

/* music manager */

void
manageBuildUIMusicManager (manageui_t *manage)
{
  uiwcont_t  *uip;
  uiwcont_t  *uiwidgetp;
  uiwcont_t  *vbox;

  logProcBegin (LOG_PROC, "manageBuildUIMusicManager");
  /* music manager */
  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 2);
  /* CONTEXT: managementui: name of music manager notebook tab */
  uiwidgetp = uiCreateLabel (_("Music Manager"));
  uiNotebookAppendPage (manage->wcont [MANAGE_W_MAIN_NB], vbox, uiwidgetp);
  uinbutilIDAdd (manage->mainnbtabid, MANAGE_TAB_MAIN_MM);
  uiwcontFree (uiwidgetp);

  /* music manager: player */
  uiwidgetp = uiplayerBuildUI (manage->mmplayer);
  uiBoxPackStart (vbox, uiwidgetp);

  manage->wcont [MANAGE_W_MM_NB] = uiCreateNotebook ();
  uiBoxPackStartExpand (vbox, manage->wcont [MANAGE_W_MM_NB]);

  /* music manager: song selection tab*/
  uip = uisongselBuildUI (manage->mmsongsel, manage->wcont [MANAGE_W_WINDOW]);
  uiWidgetExpandHoriz (uiwidgetp);
  /* CONTEXT: managementui: name of song selection notebook tab */
  uiwidgetp = uiCreateLabel (_("Music Manager"));
  uiNotebookAppendPage (manage->wcont [MANAGE_W_MM_NB], uip, uiwidgetp);
  uinbutilIDAdd (manage->mmnbtabid, MANAGE_TAB_MM);
  uiwcontFree (uiwidgetp);

  /* music manager: song editor tab */
  uip = uisongeditBuildUI (manage->mmsongsel, manage->mmsongedit, manage->wcont [MANAGE_W_WINDOW], manage->wcont [MANAGE_W_ERROR_MSG]);
  /* CONTEXT: managementui: name of song editor notebook tab */
  uiwidgetp = uiCreateLabel (_("Song Editor"));
  uiNotebookAppendPage (manage->wcont [MANAGE_W_MM_NB], uip, uiwidgetp);
  uinbutilIDAdd (manage->mmnbtabid, MANAGE_TAB_SONGEDIT);
  uiwcontFree (uiwidgetp);

  /* music manager: audio identification tab */

  manage->manageaudioid = manageAudioIdAlloc (manage->dispsel, manage->options,
      manage->wcont [MANAGE_W_WINDOW], manage->wcont [MANAGE_W_ERROR_MSG],
      manage->wcont [MANAGE_W_STATUS_MSG], manage->pleasewaitmsg);
  manageAudioIdSetSaveCallback (manage->manageaudioid, manage->callbacks [MANAGE_CB_SAVE]);

  uip = manageAudioIdBuildUI (manage->manageaudioid, manage->mmsongsel);
  /* CONTEXT: managementui: name of audio identification notebook tab */
  uiwidgetp = uiCreateLabel (_("Audio ID"));
  uiNotebookAppendPage (manage->wcont [MANAGE_W_MM_NB], uip, uiwidgetp);
  uinbutilIDAdd (manage->mmnbtabid, MANAGE_TAB_AUDIOID);
  uiwcontFree (uiwidgetp);

  manage->callbacks [MANAGE_CB_MM_NB] = callbackInitLong (
      manageSwitchPageMM, manage);
  uiNotebookSetCallback (manage->wcont [MANAGE_W_MM_NB], manage->callbacks [MANAGE_CB_MM_NB]);

  uiwcontFree (vbox);

  logProcEnd (LOG_PROC, "manageBuildUIMusicManager", "");
}

void
manageMusicManagerMenu (manageui_t *manage)
{
  uiwcont_t  *menu = NULL;
  uiwcont_t  *menuitem = NULL;

  logProcBegin (LOG_PROC, "manageMusicManagerMenu");
  if (! uiMenuInitialized (manage->mmmenu)) {
    menuitem = uiMenuAddMainItem (manage->wcont [MANAGE_W_MENUBAR],
        /* CONTEXT: managementui: menu selection: actions for music manager */
        manage->mmmenu, _("Actions"));
    menu = uiCreateSubMenu (menuitem);
    uiwcontFree (menuitem);

    manageSetMenuCallback (manage, MANAGE_MENU_CB_MM_SET_MARK,
        manageSameSongSetMark);
    /* CONTEXT: managementui: menu selection: music manager: set same-song mark */
    menuitem = uiMenuCreateItem (menu, _("Mark as Same Song"),
        manage->callbacks [MANAGE_MENU_CB_MM_SET_MARK]);
    uiwcontFree (menuitem);

    manageSetMenuCallback (manage, MANAGE_MENU_CB_MM_CLEAR_MARK,
        manageSameSongClearMark);
    /* CONTEXT: managementui: menu selection: music manager: clear same-song mark */
    menuitem = uiMenuCreateItem (menu, _("Clear Same Song Mark"),
        manage->callbacks [MANAGE_MENU_CB_MM_CLEAR_MARK]);
    uiwcontFree (menuitem);

    uiMenuAddSeparator (menu);

    manageSetMenuCallback (manage, MANAGE_MENU_CB_MM_REMOVE_SONG,
        manageMarkSongRemoved);
    /* CONTEXT: managementui: menu selection: music manager: remove song */
    menuitem = uiMenuCreateItem (menu, _("Remove Song"),
        manage->callbacks [MANAGE_MENU_CB_MM_REMOVE_SONG]);
    uiwcontFree (menuitem);

    manageSetMenuCallback (manage, MANAGE_MENU_CB_MM_UNDO_REMOVE,
        manageUndoRemove);
    /* CONTEXT: managementui: menu selection: music manager: undo song removal */
    menuitem = uiMenuCreateItem (menu, _("Undo Song Removal"),
        manage->callbacks [MANAGE_MENU_CB_MM_UNDO_REMOVE]);
    uiWidgetSetState (menuitem, UIWIDGET_DISABLE);
    manage->wcont [MANAGE_W_MENU_UNDO_REMOVE] = menuitem;

    uiMenuSetInitialized (manage->mmmenu);
    uiwcontFree (menu);
  }

  uiMenuDisplay (manage->mmmenu);
  manage->currmenu = manage->mmmenu;

  logProcEnd (LOG_PROC, "manageMusicManagerMenu", "");
}

/* song list */

static void
manageSonglistMenu (manageui_t *manage)
{
  char        tbuff [200];
  uiwcont_t   *menu;
  uiwcont_t   *menuitem;

  logProcBegin (LOG_PROC, "manageSonglistMenu");
  if (uiMenuInitialized (manage->slmenu)) {
    uiMenuDisplay (manage->slmenu);
    manage->currmenu = manage->slmenu;
    logProcEnd (LOG_PROC, "manageSonglistMenu", "already");
    return;
  }

  /* edit */
  menuitem = uiMenuAddMainItem (manage->wcont [MANAGE_W_MENUBAR],
      /* CONTEXT: managementui: menu selection: song list: edit menu */
      manage->slmenu, _("Edit"));
  menu = uiCreateSubMenu (menuitem);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_LOAD, manageSonglistLoad);
  /* CONTEXT: managementui: menu selection: song list: edit menu: load */
  menuitem = uiMenuCreateItem (menu, _("Load"),
      manage->callbacks [MANAGE_MENU_CB_SL_LOAD]);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_NEW, manageSonglistNew);
  /* CONTEXT: managementui: menu selection: song list: edit menu: start new song list */
  menuitem = uiMenuCreateItem (menu, _("Start New Song List"),
      manage->callbacks [MANAGE_MENU_CB_SL_NEW]);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_COPY, manageSonglistCopy);
  /* CONTEXT: managementui: menu selection: song list: edit menu: create copy */
  menuitem = uiMenuCreateItem (menu, _("Create Copy"),
      manage->callbacks [MANAGE_MENU_CB_SL_COPY]);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_DELETE, manageSonglistDelete);
  /* CONTEXT: managementui: menu selection: song list: edit menu: delete song list */
  menuitem = uiMenuCreateItem (menu, _("Delete"),
      manage->callbacks [MANAGE_MENU_CB_SL_DELETE]);
  uiwcontFree (menuitem);

  /* actions */
  uiwcontFree (menu);
  menuitem = uiMenuAddMainItem (manage->wcont [MANAGE_W_MENUBAR],
      /* CONTEXT: managementui: menu selection: actions for song list */
      manage->slmenu, _("Actions"));
  menu = uiCreateSubMenu (menuitem);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_MIX, manageSonglistMix);
  /* CONTEXT: managementui: menu selection: song list: actions menu: rearrange the songs and create a new mix */
  menuitem = uiMenuCreateItem (menu, _("Mix"),
      manage->callbacks [MANAGE_MENU_CB_SL_MIX]);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_SWAP, manageSonglistSwap);
  /* CONTEXT: managementui: menu selection: song list: actions menu: swap two songs */
  menuitem = uiMenuCreateItem (menu, _("Swap"),
      manage->callbacks [MANAGE_MENU_CB_SL_SWAP]);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_TRUNCATE,
      manageSonglistTruncate);
  /* CONTEXT: managementui: menu selection: song list: actions menu: truncate the song list */
  menuitem = uiMenuCreateItem (menu, _("Truncate"),
      manage->callbacks [MANAGE_MENU_CB_SL_TRUNCATE]);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_MK_FROM_PL,
      manageSonglistCreateFromPlaylist);
  /* CONTEXT: managementui: menu selection: song list: actions menu: create a song list from a playlist */
  menuitem = uiMenuCreateItem (menu, _("Create from Playlist"),
      manage->callbacks [MANAGE_MENU_CB_SL_MK_FROM_PL]);
  uiwcontFree (menuitem);

  /* export */
  uiwcontFree (menu);
  menuitem = uiMenuAddMainItem (manage->wcont [MANAGE_W_MENUBAR],
      /* CONTEXT: managementui: menu selection: export actions for song list */
      manage->slmenu, _("Export"));
  menu = uiCreateSubMenu (menuitem);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_M3U_EXP,
      manageSonglistExportM3U);
  /* CONTEXT: managementui: menu selection: song list: export: export as m3u */
  menuitem = uiMenuCreateItem (menu, _("Export as M3U Playlist"),
      manage->callbacks [MANAGE_MENU_CB_SL_M3U_EXP]);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_BDJ4_EXP,
      manageSonglistExportBDJ4);
  /* CONTEXT: managementui: menu selection: song list: export: export for ballroomdj */
  snprintf (tbuff, sizeof (tbuff), _("Export for %s"), BDJ4_NAME);
  menuitem = uiMenuCreateItem (menu, tbuff,
      manage->callbacks [MANAGE_MENU_CB_SL_BDJ4_EXP]);
  uiwcontFree (menuitem);

  /* import */
  uiwcontFree (menu);
  menuitem = uiMenuAddMainItem (manage->wcont [MANAGE_W_MENUBAR],
      /* CONTEXT: managementui: menu selection: import actions for song list */
      manage->slmenu, _("Import"));
  menu = uiCreateSubMenu (menuitem);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_M3U_IMP,
      manageSonglistImportM3U);
  /* CONTEXT: managementui: menu selection: song list: import: import m3u */
  menuitem = uiMenuCreateItem (menu, _("Import M3U"),
      manage->callbacks [MANAGE_MENU_CB_SL_M3U_IMP]);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_BDJ4_IMP,
      manageSonglistImportBDJ4);
  /* CONTEXT: managementui: menu selection: song list: import: import from ballroomdj */
  snprintf (tbuff, sizeof (tbuff), _("Import from %s"), BDJ4_NAME);
  menuitem = uiMenuCreateItem (menu, tbuff,
      manage->callbacks [MANAGE_MENU_CB_SL_BDJ4_IMP]);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_ITUNES_IMP,
      manageSonglistImportiTunes);
  /* CONTEXT: managementui: menu selection: song list: import: import from itunes */
  snprintf (tbuff, sizeof (tbuff), _("Import from %s"), ITUNES_NAME);
  menuitem = uiMenuCreateItem (menu, tbuff,
      manage->callbacks [MANAGE_MENU_CB_SL_ITUNES_IMP]);
  uiwcontFree (menuitem);

  /* options */
  uiwcontFree (menu);
  menuitem = uiMenuAddMainItem (manage->wcont [MANAGE_W_MENUBAR],
      /* CONTEXT: managementui: menu selection: song list: options menu */
      manage->slmenu, _("Options"));
  menu = uiCreateSubMenu (menuitem);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_SBS_EDIT,
      manageToggleEasySonglist);
  /* CONTEXT: managementui: menu checkbox: side-by-side view (suggestion: combined view) */
  menuitem = uiMenuCreateCheckbox (menu, _("Side-by-Side View"),
      nlistGetNum (manage->options, MANAGE_SBS_SONGLIST),
      manage->callbacks [MANAGE_MENU_CB_SL_SBS_EDIT]);
  uiwcontFree (menuitem);

  uiMenuSetInitialized (manage->slmenu);

  uiMenuDisplay (manage->slmenu);
  manage->currmenu = manage->slmenu;

  uiwcontFree (menu);

  logProcEnd (LOG_PROC, "manageSonglistMenu", "");
}

static bool
manageSonglistLoad (void *udata)
{
  manageui_t  *manage = udata;

  logProcBegin (LOG_PROC, "manageSonglistLoad");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: load songlist");
  manageSonglistSave (manage);
  selectFileDialog (SELFILE_SONGLIST, manage->wcont [MANAGE_W_WINDOW], manage->options,
      manage->callbacks [MANAGE_CB_SL_SEL_FILE]);
  logProcEnd (LOG_PROC, "manageSonglistLoad", "");
  return UICB_CONT;
}

static long
manageSonglistLoadCB (void *udata, const char *fn)
{
  manageui_t  *manage = udata;

  manageSonglistLoadFile (manage, fn, MANAGE_STD);
  return 0;
}

static bool
manageSonglistCopy (void *udata)
{
  manageui_t  *manage = udata;
  char        *oname;
  char        newname [200];

  logProcBegin (LOG_PROC, "manageSonglistCopy");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: copy songlist");
  manageSonglistSave (manage);

  oname = uimusicqGetSonglistName (manage->slmusicq);

  /* CONTEXT: managementui: the new name after 'create copy' (e.g. "Copy of DJ-2022-04") */
  snprintf (newname, sizeof (newname), _("Copy of %s"), oname);
  if (manageCreatePlaylistCopy (manage->wcont [MANAGE_W_ERROR_MSG], oname, newname)) {
    manageSetSonglistName (manage, newname);
    manageLoadPlaylistCB (manage, newname);
    manage->slbackupcreated = false;
  }
  mdfree (oname);
  logProcEnd (LOG_PROC, "manageSonglistCopy", "");
  return UICB_CONT;
}

static bool
manageSonglistNew (void *udata)
{
  manageui_t  *manage = udata;

  logProcBegin (LOG_PROC, "manageSonglistNew");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: new songlist");
  manageSonglistSave (manage);

  /* CONTEXT: managementui: song list: default name for a new song list */
  manageSetSonglistName (manage, _("New Song List"));
  manage->slbackupcreated = false;
  uimusicqSetSelectionFirst (manage->slmusicq, manage->musicqManageIdx);
  uimusicqTruncateQueueCallback (manage->slmusicq);
  manageNewPlaylistCB (manage);
  /* the music manager must be reset to use the song selection as */
  /* there are no songs selected */
  manageNewSelectionSongSel (manage, manage->seldbidx);
  logProcEnd (LOG_PROC, "manageSonglistNew", "");
  return UICB_CONT;
}

static bool
manageSonglistDelete (void *udata)
{
  manageui_t  *manage = udata;
  char        *oname;

  logProcBegin (LOG_PROC, "manageSonglistDelete");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: new songlist");
  oname = uimusicqGetSonglistName (manage->slmusicq);

  manageDeletePlaylist (manage->wcont [MANAGE_W_ERROR_MSG], oname);
  /* no save */
  dataFree (manage->sloldname);
  manage->sloldname = NULL;
  manageSonglistNew (manage);
  mdfree (oname);
  logProcEnd (LOG_PROC, "manageSonglistDelete", "");
  return UICB_CONT;
}

static bool
manageSonglistTruncate (void *udata)
{
  manageui_t  *manage = udata;

  logProcBegin (LOG_PROC, "manageSonglistTruncate");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: truncate songlist");
  uimusicqTruncateQueueCallback (manage->slmusicq);
  logProcEnd (LOG_PROC, "manageSonglistTruncate", "");
  return UICB_CONT;
}

static bool
manageSonglistCreateFromPlaylist (void *udata)
{
  manageui_t  *manage = udata;
  int         x, y;

  if (manage->cfplactive) {
    return UICB_STOP;
  }

  manage->cfplactive = true;
  logProcBegin (LOG_PROC, "manageSonglistCreateFromPlaylist");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: create from playlist");
  manageSonglistSave (manage);

  /* if there are no songs, preserve the song list name, as */
  /* the user may have typed in a new name before running create-from-pl */
  dataFree (manage->slpriorname);
  manage->slpriorname = NULL;
  if (uimusicqGetCount (manage->slmusicq) <= 0) {
    manage->slpriorname = uimusicqGetSonglistName (manage->slmusicq);
  }
  manageSongListCFPLCreateDialog (manage);

  uiDialogShow (manage->wcont [MANAGE_W_CFPL_DIALOG]);

  x = nlistGetNum (manage->options, MANAGE_CFPL_POSITION_X);
  y = nlistGetNum (manage->options, MANAGE_CFPL_POSITION_Y);
  uiWindowMove (manage->wcont [MANAGE_W_CFPL_DIALOG], x, y, -1);

  logProcEnd (LOG_PROC, "manageSonglistCreateFromPlaylist", "");
  return UICB_CONT;
}

static void
manageSongListCFPLCreateDialog (manageui_t *manage)
{
  uiwcont_t  *vbox;
  uiwcont_t  *hbox;
  uiwcont_t  *uiwidgetp;
  uiwcont_t  *szgrp;  // labels

  logProcBegin (LOG_PROC, "manageSongListCFPLCreateDialog");
  if (manage->wcont [MANAGE_W_CFPL_DIALOG] != NULL) {
    logProcEnd (LOG_PROC, "manageSongListCFPLCreateDialog", "already");
    return;
  }

  szgrp = uiCreateSizeGroupHoriz ();

  manage->wcont [MANAGE_W_CFPL_DIALOG] = uiCreateDialog (manage->wcont [MANAGE_W_WINDOW],
      manage->callbacks [MANAGE_CB_CFPL_DIALOG],
      /* CONTEXT: create from playlist: title for the dialog */
      _("Create from Playlist"),
      /* CONTEXT: create from playlist: closes the dialog */
      _("Close"),
      RESPONSE_CLOSE,
      /* CONTEXT: create from playlist: creates the new songlist */
      _("Create"),
      RESPONSE_APPLY,
      NULL
      );

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);
  uiDialogPackInDialog (manage->wcont [MANAGE_W_CFPL_DIALOG], vbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: create from playlist: select the playlist to use */
  uiwidgetp = uiCreateColonLabel (_("Playlist"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  manage->cfpl = uiplaylistCreate (manage->wcont [MANAGE_W_CFPL_DIALOG],
      hbox, PL_LIST_AUTO_SEQ);

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: create from playlist: set the maximum time for the song list */
  uiwidgetp = uiCreateColonLabel (_("Time Limit"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiSpinboxTimeCreate (manage->cfpltmlimit, manage, NULL);
  uiSpinboxTimeSetValue (manage->cfpltmlimit, 3 * 60 * 1000);
  uiSpinboxSetRange (manage->cfpltmlimit, 0.0, 600000.0);
  uiwidgetp = uiSpinboxGetWidgetContainer (manage->cfpltmlimit);
  uiBoxPackStart (hbox, uiwidgetp);

  uiwcontFree (vbox);
  uiwcontFree (hbox);
  uiwcontFree (szgrp);

  logProcEnd (LOG_PROC, "manageSongListCFPLCreateDialog", "");
}

static bool
manageCFPLResponseHandler (void *udata, long responseid)
{
  manageui_t  *manage = udata;
  int         x, y, ws;

  logProcBegin (LOG_PROC, "manageCFPLResponseHandler");
  uiWindowGetPosition (manage->wcont [MANAGE_W_CFPL_DIALOG], &x, &y, &ws);
  nlistSetNum (manage->options, MANAGE_CFPL_POSITION_X, x);
  nlistSetNum (manage->options, MANAGE_CFPL_POSITION_Y, y);

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: cfpl: del window");
      uiwcontFree (manage->wcont [MANAGE_W_CFPL_DIALOG]);
      manage->wcont [MANAGE_W_CFPL_DIALOG] = NULL;
      manage->cfplactive = false;
      break;
    }
    case RESPONSE_CLOSE: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: cfpl: close window");
      uiWidgetHide (manage->wcont [MANAGE_W_CFPL_DIALOG]);
      manage->cfplactive = false;
      break;
    }
    case RESPONSE_APPLY: {
      manageCFPLCreate (manage);
      break;
    }
  }

  logProcEnd (LOG_PROC, "manageCFPLResponseHandler", "");
  return UICB_CONT;
}

static void
manageCFPLCreate (manageui_t *manage)
{
  const char  *fn;
  long        stoptime;
  char        tbuff [40];

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: cfpl: create");

  manage->musicqupdated = false;

  fn = uiplaylistGetValue (manage->cfpl);

  if (fn == NULL || ! *fn) {
    manage->cfplactive = false;
    return;
  }

  dataFree (manage->cfplfn);
  manage->cfplfn = mdstrdup (fn);
  stoptime = uiSpinboxTimeGetValue (manage->cfpltmlimit);
  /* convert from mm:ss to hh:mm */
  stoptime *= 60;
  /* adjust : add in the current hh:mm */
  stoptime += mstime () - mstimestartofday ();

  snprintf (tbuff, sizeof (tbuff), "%d", manage->musicqManageIdx);
  connSendMessage (manage->conn, ROUTE_MAIN, MSG_QUEUE_CLEAR, tbuff);

  /* overriding the stop time will set the stop time for the next */
  /* playlist that is loaded */
  snprintf (tbuff, sizeof (tbuff), "%ld", stoptime);
  connSendMessage (manage->conn, ROUTE_MAIN,
      MSG_PL_OVERRIDE_STOP_TIME, tbuff);

  /* the edit mode must be false to allow the stop time to be applied */
  manage->editmode = EDIT_FALSE;
  /* re-use songlist-load-file to load the auto/seq playlist */
  manageSonglistLoadFile (manage, fn, MANAGE_CREATE);

  /* make sure no save happens with the playlist being used */
  dataFree (manage->sloldname);
  manage->sloldname = NULL;
  manage->editmode = EDIT_TRUE;
  if (manage->slpriorname != NULL) {
    manageSetSonglistName (manage, manage->slpriorname);
  } else {
    /* CONTEXT: managementui: song list: default name for a new song list */
    manageSetSonglistName (manage, _("New Song List"));
  }

  /* now tell main to clear the playlist queue so that the */
  /* automatic/sequenced playlist is no longer present */
  snprintf (tbuff, sizeof (tbuff), "%d", manage->musicqManageIdx);
  connSendMessage (manage->conn, ROUTE_MAIN, MSG_PL_CLEAR_QUEUE, tbuff);
  manage->slbackupcreated = false;

  uiWidgetHide (manage->wcont [MANAGE_W_CFPL_DIALOG]);

  manage->cfplpostprocess = true;
  manage->cfplactive = false;
}

static void
manageCFPLPostProcess (manageui_t *manage)
{
  playlist_t  *autopl;
  playlist_t  *pl;
  dance_t     *dances;
  ilistidx_t  diteridx;
  ilistidx_t  dkey;
  char        *tnm;

  tnm = uimusicqGetSonglistName (manage->slmusicq);
  manageSonglistSave (manage);

  /* copy the settings from the base playlist to the new song list */
  pl = playlistLoad (tnm, NULL);
  autopl = playlistLoad (manage->cfplfn, NULL);

  if (pl != NULL && autopl != NULL) {
    playlistSetConfigNum (pl, PLAYLIST_MAX_PLAY_TIME,
        playlistGetConfigNum (autopl, PLAYLIST_MAX_PLAY_TIME));
    playlistSetConfigNum (pl, PLAYLIST_STOP_TIME,
        playlistGetConfigNum (autopl, PLAYLIST_STOP_TIME));
    playlistSetConfigNum (pl, PLAYLIST_GAP,
        playlistGetConfigNum (autopl, PLAYLIST_GAP));
    playlistSetConfigNum (pl, PLAYLIST_ANNOUNCE,
        playlistGetConfigNum (autopl, PLAYLIST_ANNOUNCE));

    dances = bdjvarsdfGet (BDJVDF_DANCES);
    danceStartIterator (dances, &diteridx);
    while ((dkey = danceIterate (dances, &diteridx)) >= 0) {
      playlistSetDanceNum (pl, dkey, PLDANCE_MAXPLAYTIME,
          playlistGetDanceNum (autopl, dkey, PLDANCE_MAXPLAYTIME));
    }
    playlistSave (pl, tnm);
  }

  playlistFree (autopl);
  playlistFree (pl);

  /* update the playlist tab */
  managePlaylistLoadFile (manage->managepl, tnm, MANAGE_PRELOAD_FORCE);
  mdfree (tnm);

  manage->cfplpostprocess = false;
}

static bool
manageSonglistMix (void *udata)
{
  manageui_t  *manage = udata;
  char        tbuff [40];

  logProcBegin (LOG_PROC, "manageSonglistMix");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: mix songlist");
  uiLabelSetText (manage->wcont [MANAGE_W_STATUS_MSG], manage->pleasewaitmsg);
  snprintf (tbuff, sizeof (tbuff), "%d", manage->musicqManageIdx);
  connSendMessage (manage->conn, ROUTE_MAIN, MSG_QUEUE_MIX, tbuff);
  logProcEnd (LOG_PROC, "manageSonglistMix", "");
  return UICB_CONT;
}

static bool
manageSonglistSwap (void *udata)
{
  manageui_t  *manage = udata;

  logProcBegin (LOG_PROC, "manageSonglistSwap");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: songlist swap");
  uimusicqSwap (manage->slmusicq, manage->musicqManageIdx);
  logProcEnd (LOG_PROC, "manageSonglistSwap", "");
  return UICB_CONT;
}

static void
manageSonglistLoadFile (void *udata, const char *fn, int preloadflag)
{
  manageui_t  *manage = udata;
  char  tbuff [200];

  logProcBegin (LOG_PROC, "manageSonglistLoadFile");
  if (manage->inload) {
    logProcEnd (LOG_PROC, "manageSonglistLoadFile", "in-load");
    return;
  }

  if (preloadflag == MANAGE_CREATE) {
    if (! playlistExists (fn)) {
      logProcEnd (LOG_PROC, "manageSonglistLoadFile", "no-playlist");
      return;
    }
  }
  if (preloadflag == MANAGE_STD || preloadflag == MANAGE_PRELOAD) {
    if (! songlistExists (fn)) {
      logProcEnd (LOG_PROC, "manageSonglistLoadFile", "no-songlist");
      return;
    }
  }

  manage->inload = true;

  /* create-from-playlist already did a save */
  if (preloadflag == MANAGE_STD) {
    manageSonglistSave (manage);
  }

  manage->selusesonglist = true;
  /* ask the main player process to not send music queue updates */
  /* the selbypass flag cannot be used due to timing issues */
  snprintf (tbuff, sizeof (tbuff), "%d", manage->musicqManageIdx);
  connSendMessage (manage->conn, ROUTE_MAIN, MSG_MUSICQ_DATA_SUSPEND, tbuff);

  /* truncate from the first selection */
  uimusicqSetSelectionFirst (manage->slmusicq, manage->musicqManageIdx);
  uimusicqTruncateQueueCallback (manage->slmusicq);

  snprintf (tbuff, sizeof (tbuff), "%d%c%s%c%d",
      manage->musicqManageIdx, MSG_ARGS_RS, fn, MSG_ARGS_RS, manage->editmode);
  connSendMessage (manage->conn, ROUTE_MAIN, MSG_QUEUE_PLAYLIST, tbuff);

  snprintf (tbuff, sizeof (tbuff), "%d", manage->musicqManageIdx);
  connSendMessage (manage->conn, ROUTE_MAIN, MSG_MUSICQ_DATA_RESUME, tbuff);

  manageSetSonglistName (manage, fn);
  if (preloadflag != MANAGE_PRELOAD) {
    manageLoadPlaylistCB (manage, fn);
  }
  manage->slbackupcreated = false;
  manage->inload = false;
  logProcEnd (LOG_PROC, "manageSonglistLoadFile", "");
}

/* callback to load playlist upon songlist/sequence load */
static long
manageLoadPlaylistCB (void *udata, const char *fn)
{
  manageui_t    *manage = udata;

  logMsg (LOG_DBG, LOG_INFO, "load playlist cb: %s", fn);
  managePlaylistLoadFile (manage->managepl, fn, MANAGE_PRELOAD);
  return UICB_CONT;
}

/* callback to reset playlist upon songlist/sequence new */
static bool
manageNewPlaylistCB (void *udata)
{
  manageui_t    *manage = udata;

  logMsg (LOG_DBG, LOG_INFO, "new playlist cb");
  managePlaylistNew (manage->managepl, MANAGE_PRELOAD);
  return UICB_CONT;
}

/* callback to load upon playlist load */
static long
manageLoadSonglistSeqCB (void *udata, const char *fn)
{
  manageui_t    *manage = udata;

  logProcBegin (LOG_PROC, "manageLoadSonglistSeqCB");
  /* the load will save any current song list */
  manageSonglistLoadFile (manage, fn, MANAGE_PRELOAD);
  manageSequenceLoadFile (manage->manageseq, fn, MANAGE_PRELOAD);
  logProcEnd (LOG_PROC, "manageLoadSonglistSeqCB", "");
  return UICB_CONT;
}

static bool
manageToggleEasySonglist (void *udata)
{
  manageui_t  *manage = udata;
  int         val;

  logProcBegin (LOG_PROC, "manageToggleEasySonglist");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: toggle side-by-side songlist");
  val = nlistGetNum (manage->options, MANAGE_SBS_SONGLIST);
  val = ! val;
  nlistSetNum (manage->options, MANAGE_SBS_SONGLIST, val);
  manageSetEasySonglist (manage);
  logProcEnd (LOG_PROC, "manageToggleEasySonglist", "");
  return UICB_CONT;
}

static void
manageSetEasySonglist (manageui_t *manage)
{
  int     val;

  logProcBegin (LOG_PROC, "manageSetEasySonglist");
  val = nlistGetNum (manage->options, MANAGE_SBS_SONGLIST);
  if (val) {
    uiWidgetShowAll (manage->wcont [MANAGE_W_SL_SBS_MUSICQ_TAB]);
    uiWidgetHide (manage->wcont [MANAGE_W_SL_MUSICQ_TAB]);
    uiWidgetHide (manage->wcont [MANAGE_W_SONGSEL_TAB]);
    uiNotebookSetPage (manage->wcont [MANAGE_W_SONGLIST_NB], 0);
  } else {
    uiWidgetHide (manage->wcont [MANAGE_W_SL_SBS_MUSICQ_TAB]);
    uiWidgetShowAll (manage->wcont [MANAGE_W_SL_MUSICQ_TAB]);
    uiWidgetShowAll (manage->wcont [MANAGE_W_SONGSEL_TAB]);
    uiNotebookSetPage (manage->wcont [MANAGE_W_SONGLIST_NB], 1);
  }
  logProcEnd (LOG_PROC, "manageSetEasySonglist", "");
}

static void
manageSetSonglistName (manageui_t *manage, const char *nm)
{
  dataFree (manage->sloldname);
  manage->sloldname = mdstrdup (nm);
  uimusicqSetSonglistName (manage->slmusicq, nm);
  logMsg (LOG_DBG, LOG_INFO, "song list name set: %s", nm);
}

static void
manageSonglistSave (manageui_t *manage)
{
  char  *name;
  char  nnm [MAXPATHLEN];

  logProcBegin (LOG_PROC, "manageSonglistSave");
  if (manage == NULL) {
    logProcEnd (LOG_PROC, "manageSonglistSave", "null-manage");
    return;
  }
  if (manage->sloldname == NULL) {
    logProcEnd (LOG_PROC, "manageSonglistSave", "no-sl-old-name");
    return;
  }
  if (manage->slmusicq == NULL) {
    logProcEnd (LOG_PROC, "manageSonglistSave", "no-slmusicq");
    return;
  }

  if (uimusicqGetCount (manage->slmusicq) <= 0) {
    logProcEnd (LOG_PROC, "manageSonglistSave", "count <= 0");
    return;
  }

  name = uimusicqGetSonglistName (manage->slmusicq);

  /* the song list has been renamed */
  if (strcmp (manage->sloldname, name) != 0) {
    playlistRename (manage->sloldname, name);
    /* continue on so that renames are handled properly */
  }

  /* need the full name for backups */
  pathbldMakePath (nnm, sizeof (nnm),
      name, BDJ4_SONGLIST_EXT, PATHBLD_MP_DREL_DATA);
  if (! manage->slbackupcreated) {
    filemanipBackup (nnm, 1);
    manage->slbackupcreated = true;
  }

  manageSetSonglistName (manage, name);
  uimusicqSave (manage->slmusicq, name);
  playlistCheckAndCreate (name, PLTYPE_SONGLIST);
  manageLoadPlaylistCB (manage, name);
  mdfree (name);
  logProcEnd (LOG_PROC, "manageSonglistSave", "");
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
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: play from side-by-side songlist");
  manageQueueProcess (udata, dbidx, mqidx, DISP_SEL_SBS_SONGLIST, MANAGE_PLAY);
  return UICB_CONT;
}

static bool
managePlayProcessMusicManager (void *udata, long dbidx, int mqidx)
{
  manageui_t  *manage = udata;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: play from mm");

  /* if using the song editor, the play button should play the song */
  /* being currently edited.  */
  /* if there is a multi-selection, uisongselui */
  /* will play the incorrect selection */
  if (manage->mmlasttab == MANAGE_TAB_SONGEDIT) {
    dbidx = manage->songeditdbidx;
  }

  manageQueueProcess (manage, dbidx, mqidx, DISP_SEL_MM, MANAGE_PLAY);
  return UICB_CONT;
}

static bool
manageQueueProcessSonglist (void *udata, long dbidx)
{
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: queue to songlist");
  manageQueueProcess (udata, dbidx, MUSICQ_SL, DISP_SEL_SONGLIST, MANAGE_QUEUE);
  return UICB_CONT;
}

static bool
manageQueueProcessEasySonglist (void *udata, long dbidx)
{
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: queue to side-by-side songlist");
  manageQueueProcess (udata, dbidx, MUSICQ_SL, DISP_SEL_SBS_SONGLIST, MANAGE_QUEUE);
  return UICB_CONT;
}

static void
manageQueueProcess (void *udata, long dbidx, int mqidx, int dispsel, int action)
{
  manageui_t  *manage = udata;
  char        tbuff [100];
  long        loc = QUEUE_LOC_LAST;
  uimusicq_t  *uimusicq = NULL;

  logProcBegin (LOG_PROC, "manageQueueProcess");
  if (dispsel == DISP_SEL_SONGLIST) {
    uimusicq = manage->slmusicq;
  }
  if (dispsel == DISP_SEL_SBS_SONGLIST) {
    uimusicq = manage->slsbsmusicq;
  }
  if (dispsel == DISP_SEL_MM) {
    uimusicq = manage->mmmusicq;
  }

  if (action == MANAGE_QUEUE) {
    /* on a queue action, queue after the current selection */
    loc = uimusicqGetSelectLocation (uimusicq, mqidx);
    if (loc < 0) {
      loc = QUEUE_LOC_LAST;
    }
    if (! manage->musicqueueprocessflag) {
      /* the queue has not yet been processed, add 1 to the last location */
      loc = manage->lastinsertlocation + 1;
    }
  }

  if (action == MANAGE_QUEUE_LAST) {
    action = MANAGE_QUEUE;
    loc = QUEUE_LOC_LAST;
  }

  if (action == MANAGE_QUEUE) {
    /* add 1 to the index, as the tree-view index is one less than */
    /* the music queue index */
    manage->musicqueueprocessflag = false;
    manage->lastinsertlocation = loc;
    snprintf (tbuff, sizeof (tbuff), "%d%c%ld%c%ld", mqidx,
        MSG_ARGS_RS, loc + 1, MSG_ARGS_RS, dbidx);
    connSendMessage (manage->conn, ROUTE_MAIN, MSG_MUSICQ_INSERT, tbuff);
  }

  if (action == MANAGE_PLAY) {
    snprintf (tbuff, sizeof (tbuff), "%d%c%d%c%ld", mqidx,
        MSG_ARGS_RS, QUEUE_LOC_LAST, MSG_ARGS_RS, dbidx);
    connSendMessage (manage->conn, ROUTE_MAIN, MSG_QUEUE_CLEAR_PLAY, tbuff);
  }
  logProcEnd (LOG_PROC, "manageQueueProcess", "");
}

/* m3u */

static bool
manageSonglistExportM3U (void *udata)
{
  manageui_t  *manage = udata;
  char        tbuff [200];
  char        tname [200];
  uiselect_t  *selectdata;
  char        *fn;
  char        *slname;

  if (manage->exportm3uactive) {
    return UICB_STOP;
  }

  logProcBegin (LOG_PROC, "manageSonglistExportM3U");
  manage->exportm3uactive = true;
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: export m3u");

  manageSonglistSave (manage);

  slname = uimusicqGetSonglistName (manage->slmusicq);

  /* CONTEXT: managementui: song list export: title of save dialog */
  snprintf (tbuff, sizeof (tbuff), _("Export as M3U Playlist"));
  snprintf (tname, sizeof (tname), "%s.m3u", slname);
  selectdata = uiDialogCreateSelect (manage->wcont [MANAGE_W_WINDOW],
      tbuff, sysvarsGetStr (SV_BDJ4_DIR_DATATOP), tname,
      /* CONTEXT: managementui: song list export: name of file save type */
      _("M3U Files"), "audio/x-mpegurl");
  fn = uiSaveFileDialog (selectdata);
  if (fn != NULL) {
    uimusicqExportM3U (manage->slmusicq, fn, slname);
    mdfree (fn);
  }
  mdfree (selectdata);
  mdfree (slname);
  manage->exportm3uactive = false;
  logProcEnd (LOG_PROC, "manageSonglistExportM3U", "");
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

  if (manage->importm3uactive) {
    return UICB_STOP;
  }

  manage->importm3uactive = true;
  logProcBegin (LOG_PROC, "manageSonglistImportM3U");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: import m3u");

  manageSonglistSave (manage);
  manageSonglistNew (manage);

  /* CONTEXT: managementui: song list: default name for a new song list */
  manageSetSonglistName (manage, _("New Song List"));
  strlcpy (nplname, manage->sloldname, sizeof (nplname));

  selectdata = uiDialogCreateSelect (manage->wcont [MANAGE_W_WINDOW],
      /* CONTEXT: managementui: song list import: title of dialog */
      _("Import M3U"), sysvarsGetStr (SV_BDJ4_DIR_DATATOP), NULL,
      /* CONTEXT: managementui: song list import: name of file type */
      _("M3U Files"), "audio/x-mpegurl");

  fn = uiSelectFileDialog (selectdata);

  if (fn != NULL) {
    nlist_t     *list;
    int         mqidx;
    dbidx_t     dbidx;
    nlistidx_t  iteridx;
    int         len;
    pathinfo_t  *pi;

    pi = pathInfo (fn);
    len = pi->blen + 1 > sizeof (nplname) ? sizeof (nplname) : pi->blen + 1;
    strlcpy (nplname, pi->basename, len);
    pathInfoFree (pi);

    list = m3uImport (manage->musicdb, fn, nplname, sizeof (nplname));
    pathbldMakePath (tbuff, sizeof (tbuff),
        nplname, BDJ4_SONGLIST_EXT, PATHBLD_MP_DREL_DATA);
    if (! fileopFileExists (tbuff)) {
      manageSetSonglistName (manage, nplname);
    }

    if (nlistGetCount (list) > 0) {
      mqidx = manage->musicqManageIdx;

      /* clear the entire queue */
      uimusicqTruncateQueueCallback (manage->slmusicq);

      nlistStartIterator (list, &iteridx);
      while ((dbidx = nlistIterateKey (list, &iteridx)) >= 0) {
        manageQueueProcess (manage, dbidx, mqidx,
            DISP_SEL_SONGLIST, MANAGE_QUEUE_LAST);
      }
    }

    nlistFree (list);
    mdfree (fn);
  }
  mdfree (selectdata);

  manageLoadPlaylistCB (manage, nplname);
  manage->importm3uactive = false;
  logProcEnd (LOG_PROC, "manageSonglistImportM3U", "");
  return UICB_CONT;
}


/* export/import bdj4 */

static bool
manageSonglistExportBDJ4 (void *udata)
{
  manageui_t  *manage = udata;

  if (manage->exportbdj4active) {
    return UICB_STOP;
  }

  manage->exportbdj4active = true;
  logProcBegin (LOG_PROC, "manageSonglistExportBDJ4");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: export bdj4");

  manageSonglistSave (manage);

  uieibdj4Dialog (manage->uieibdj4, UIEIBDJ4_EXPORT);

  manage->exportbdj4active = false;
  logProcEnd (LOG_PROC, "manageSonglistExportBDJ4", "");
  return UICB_CONT;
}

static bool
manageSonglistImportBDJ4 (void *udata)
{
  manageui_t  *manage = udata;

  if (manage->importbdj4active) {
    return UICB_STOP;
  }

  manage->importbdj4active = true;
  logProcBegin (LOG_PROC, "manageSonglistImportBDJ4");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: import bdj4");

  manageSonglistSave (manage);

  uieibdj4Dialog (manage->uieibdj4, UIEIBDJ4_IMPORT);

  manage->importbdj4active = false;
  logProcEnd (LOG_PROC, "manageSonglistImportBDJ4", "");
  return UICB_CONT;
}

static bool
manageExportBDJ4ResponseHandler (void *udata)
{
  manageui_t  *manage = udata;
  char        *slname;
  char        *dir = NULL;
  nlist_t     *dbidxlist;

  slname = uimusicqGetSonglistName (manage->slmusicq);
  dbidxlist = uimusicqGetDBIdxList (manage->slmusicq, MUSICQ_SL);

  dir = uieibdj4GetDir (manage->uieibdj4);
  nlistSetStr (manage->options, MANAGE_EXP_BDJ4_DIR, dir);

  pathNormalizePath (dir, strlen (dir));
  eibdj4Free (manage->eibdj4);
  manage->eibdj4 = eibdj4Init (manage->musicdb, dir, EIBDJ4_EXPORT);
  dataFree (dir);

  eibdj4SetPlaylist (manage->eibdj4, slname);
  eibdj4SetDBIdxList (manage->eibdj4, dbidxlist);

  manage->expimpbdj4state = BDJ4_STATE_PROCESS;
  mstimeset (&manage->eibdj4ChkTime, 200);

  mdfree (slname);
  return UICB_CONT;
}

static bool
manageImportBDJ4ResponseHandler (void *udata)
{
  manageui_t  *manage = udata;
  char        *dir = NULL;
  const char  *plname = NULL;
  const char  *newname = NULL;

  dir = uieibdj4GetDir (manage->uieibdj4);
  nlistSetStr (manage->options, MANAGE_IMP_BDJ4_DIR, dir);
  plname = uieibdj4GetPlaylist (manage->uieibdj4);
  newname = uieibdj4GetNewName (manage->uieibdj4);

  pathNormalizePath (dir, strlen (dir));
  eibdj4Free (manage->eibdj4);
  manage->eibdj4 = eibdj4Init (manage->musicdb, dir, EIBDJ4_IMPORT);
  dataFree (dir);

  eibdj4SetPlaylist (manage->eibdj4, plname);
  eibdj4SetNewName (manage->eibdj4, newname);

  manage->expimpbdj4state = BDJ4_STATE_PROCESS;
  mstimeset (&manage->eibdj4ChkTime, 500);

  return UICB_CONT;
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
  uinbtabid_t  *nbtabid = NULL;

  logProcBegin (LOG_PROC, "manageSwitchPage");
  uiLabelSetText (manage->wcont [MANAGE_W_ERROR_MSG], "");
  uiLabelSetText (manage->wcont [MANAGE_W_STATUS_MSG], "");

  /* need to know which notebook is selected so that the correct id value */
  /* can be retrieved */
  if (which == MANAGE_NB_MAIN) {
    logMsg (LOG_DBG, LOG_INFO, "switch page to main");
    nbtabid = manage->mainnbtabid;
    mainnb = true;
  }
  if (which == MANAGE_NB_SONGLIST) {
    logMsg (LOG_DBG, LOG_INFO, "switch page to songlist");
    nbtabid = manage->slnbtabid;
    slnb = true;
  }
  if (which == MANAGE_NB_MM) {
    logMsg (LOG_DBG, LOG_INFO, "switch page to mm");
    nbtabid = manage->mmnbtabid;
    mmnb = true;
  }
  if (nbtabid == NULL) {
    logProcEnd (LOG_PROC, "manageSwitchPage", "no-tab-id");
    return;
  }

  if (mainnb) {
    if (manage->mainlasttab == MANAGE_TAB_MAIN_SL) {
      logMsg (LOG_DBG, LOG_INFO, "last tab: songlist");
      manageSonglistSave (manage);
    }
    if (manage->mainlasttab == MANAGE_TAB_MAIN_SEQ) {
      logMsg (LOG_DBG, LOG_INFO, "last tab: sequence");
      manageSequenceSave (manage->manageseq);
    }
    if (manage->mainlasttab == MANAGE_TAB_MAIN_PL) {
      logMsg (LOG_DBG, LOG_INFO, "last tab: playlist");
      managePlaylistSave (manage->managepl);
    }
  }

  id = uinbutilIDGet (nbtabid, pagenum);

  if (manage->currmenu != NULL) {
    uiMenuClear (manage->currmenu);
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
      logMsg (LOG_DBG, LOG_INFO, "new tab: main-sl");
      manageSetDisplayPerSelection (manage, id);
      manageSonglistLoadCheck (manage);
      break;
    }
    case MANAGE_TAB_MAIN_MM: {
      logMsg (LOG_DBG, LOG_INFO, "new tab: main-mm");
      manageSetDisplayPerSelection (manage, id);
      break;
    }
    case MANAGE_TAB_MAIN_PL: {
      logMsg (LOG_DBG, LOG_INFO, "new tab: main-pl");
      managePlaylistLoadCheck (manage->managepl);
      manage->currmenu = managePlaylistMenu (manage->managepl, manage->wcont [MANAGE_W_MENUBAR]);
      break;
    }
    case MANAGE_TAB_MAIN_SEQ: {
      logMsg (LOG_DBG, LOG_INFO, "new tab: main-seq");
      manageSequenceLoadCheck (manage->manageseq);
      manage->currmenu = manageSequenceMenu (manage->manageseq,
          manage->wcont [MANAGE_W_MENUBAR]);
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
    case MANAGE_TAB_MM: {
      manageMusicManagerMenu (manage);
      break;
    }
    case MANAGE_TAB_SONGLIST: {
      manageSonglistMenu (manage);
      break;
    }
    case MANAGE_TAB_SONGEDIT: {
      song_t  *song;

      manageSongEditMenu (manage);
      song = dbGetByIdx (manage->musicdb, manage->songeditdbidx);
      uisongeditLoadData (manage->mmsongedit, song,
          manage->songeditdbidx, UISONGEDIT_ALL);
      break;
    }
    case MANAGE_TAB_AUDIOID: {
      song_t  *song;

      manage->currmenu = manageAudioIDMenu (manage->manageaudioid,
          manage->wcont [MANAGE_W_MENUBAR]);
      song = dbGetByIdx (manage->musicdb, manage->songeditdbidx);
      manageAudioIdLoad (manage->manageaudioid, song, manage->songeditdbidx);
      break;
    }
    default: {
      break;
    }
  }
  logProcEnd (LOG_PROC, "manageSwitchPage", "");
}

/* when switching pages, the music manager must be adjusted to */
/* display the song selection list or the playlist depending on */
/* what was last selected. the song selection lists never display */
/* the playlist.  This is all very messy, especially for the filter */
/* display, but makes the song editor easier to use and more intuitive. */
static void
manageSetDisplayPerSelection (manageui_t *manage, int id)
{
  dbidx_t     dbidx;

  logProcBegin (LOG_PROC, "manageSetDisplayPerSelection");
  if (id == MANAGE_TAB_MAIN_SL) {
    if (uisfPlaylistInUse (manage->uisongfilter) ||
        manage->lastdisp == MANAGE_DISP_SONG_LIST) {
      nlistidx_t    nidx;

      nidx = uisongselGetSelectLocation (manage->mmsongsel);
      uisfClearPlaylist (manage->uisongfilter);
      manage->selbypass = true;
      uisongselApplySongFilter (manage->mmsongsel);
      manage->selbypass = false;
      uisongselRestoreSelections (manage->mmsongsel);
      if (manage->selusesonglist) {
        uimusicqSetSelectLocation (manage->slmusicq, manage->musicqManageIdx, nidx);
      }
      manage->lastdisp = MANAGE_DISP_SONG_SEL;
    }

    uisfHidePlaylistDisplay (manage->uisongfilter);
  }

  if (id == MANAGE_TAB_MAIN_MM) {
    char    *slname;
    song_t  *song;
    int     redisp = false;

    /* need this check to handle a queue that has been completely truncated */
    if (uimusicqGetCount (manage->slmusicq) <= 0 &&
        manage->selusesonglist) {
      manage->selusesonglist = false;
      redisp = true;
    }

    uisfShowPlaylistDisplay (manage->uisongfilter);

    if (manage->selusesonglist &&
        manage->lastdisp == MANAGE_DISP_SONG_SEL) {
      uisongselSaveSelections (manage->mmsongsel);
      /* the song list must be saved, otherwise the song filter */
      /* can't load it */
      manageSonglistSave (manage);
      slname = uimusicqGetSonglistName (manage->slmusicq);
      uisfSetPlaylist (manage->uisongfilter, slname);
      mdfree (slname);
      manage->lastdisp = MANAGE_DISP_SONG_LIST;
      redisp = true;
    }

    if (redisp) {
      manage->selbypass = true;
      uisongselApplySongFilter (manage->mmsongsel);

      if (manage->selusesonglist) {
        long idx;

        /* these match because they are displaying the same list */
        idx = uimusicqGetSelectLocation (manage->slmusicq, manage->musicqManageIdx);
        uisongselClearAllUISelections (manage->mmsongsel);
        /* must set the selection offset by the idx-start */
        uisongselSetSelectionOffset (manage->mmsongsel, idx);
      }
      manage->selbypass = false;
    }

    if (manage->selusesonglist) {
      dbidx = manage->songlistdbidx;
    } else {
      dbidx = manage->seldbidx;
    }

    manage->songeditdbidx = dbidx;
    manageReloadSongEdit (manage);
    song = dbGetByIdx (manage->musicdb, dbidx);
    manageSetBPMCounter (manage, song);
  }
  logProcEnd (LOG_PROC, "manageSetDisplayPerSelection", "");
}

static void
manageSetMenuCallback (manageui_t *manage, int midx, callbackFunc cb)
{
  logProcBegin (LOG_PROC, "manageSetMenuCallback");
  manage->callbacks [midx] = callbackInit (cb, manage, NULL);
  logProcEnd (LOG_PROC, "manageSetMenuCallback", "");
}

/* the current songlist may be renamed or deleted. */
/* check for this and if the songlist has */
/* disappeared, reset */
static void
manageSonglistLoadCheck (manageui_t *manage)
{
  char  *name;

  logProcBegin (LOG_PROC, "manageSonglistLoadCheck");
  if (manage->sloldname == NULL) {
    logProcEnd (LOG_PROC, "manageSonglistLoadCheck", "no-old-name");
    return;
  }

  name = uimusicqGetSonglistName (manage->slmusicq);

  if (! songlistExists (name)) {
    logMsg (LOG_DBG, LOG_INFO, "no songlist %s", name);
    /* make sure no save happens */
    dataFree (manage->sloldname);
    manage->sloldname = NULL;
    manageSonglistNew (manage);
  } else {
    manageLoadPlaylistCB (manage, name);
  }
  mdfree (name);
  logProcEnd (LOG_PROC, "manageSonglistLoadCheck", "");
}

static void
manageProcessDatabaseUpdate (manageui_t *manage)
{
  samesongFree (manage->samesong);
  manage->musicdb = bdj4ReloadDatabase (manage->musicdb);
  manage->samesong = samesongAlloc (manage->musicdb);

  manageStatsSetDatabase (manage->slstats, manage->musicdb);
  uiplayerSetDatabase (manage->slplayer, manage->musicdb);
  uiplayerSetDatabase (manage->mmplayer, manage->musicdb);
  uisongselSetDatabase (manage->slsongsel, manage->musicdb);
  uisongselSetSamesong (manage->slsongsel, manage->samesong);
  uisongselSetDatabase (manage->slsbssongsel, manage->musicdb);
  uisongselSetSamesong (manage->slsbssongsel, manage->samesong);
  uisongselSetDatabase (manage->mmsongsel, manage->musicdb);
  uisongselSetSamesong (manage->mmsongsel, manage->samesong);
  uimusicqSetDatabase (manage->slmusicq, manage->musicdb);
  uimusicqSetDatabase (manage->slsbsmusicq, manage->musicdb);
  uimusicqSetDatabase (manage->mmmusicq, manage->musicdb);

  connSendMessage (manage->conn, ROUTE_STARTERUI, MSG_DATABASE_UPDATE, NULL);

  uisongselApplySongFilter (manage->slsongsel);
}

/* same song */

static bool
manageSameSongSetMark (void *udata)
{
  manageui_t  *manage = udata;

  logProcBegin (LOG_PROC, "manageSameSongSetMark");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: set mark");
  manageSameSongChangeMark (manage, MANAGE_SET_MARK);
  logProcEnd (LOG_PROC, "manageSameSongSetMark", "");
  return UICB_CONT;
}

static bool
manageSameSongClearMark (void *udata)
{
  manageui_t  *manage = udata;

  logProcBegin (LOG_PROC, "manageSameSongClearMark");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: clear mark");
  manageSameSongChangeMark (manage, MANAGE_CLEAR_MARK);
  logProcEnd (LOG_PROC, "manageSameSongClearMark", "");
  return UICB_CONT;
}

static void
manageSameSongChangeMark (manageui_t *manage, int flag)
{
  nlist_t     *sellist;
  nlistidx_t  iteridx;
  dbidx_t     dbidx;

  logProcBegin (LOG_PROC, "manageSameSongChangeMark");
  sellist = uisongselGetSelectedList (manage->mmsongsel);

  if (flag == MANAGE_SET_MARK) {
    samesongSet (manage->samesong, sellist);
  }
  if (flag == MANAGE_CLEAR_MARK) {
    samesongClear (manage->samesong, sellist);
  }

  nlistStartIterator (sellist, &iteridx);
  while ((dbidx = nlistIterateKey (sellist, &iteridx)) >= 0) {
    songWriteDB (manage->musicdb, dbidx);
  }

  nlistFree (sellist);
  /* the same song marks only appear in the music manager */
  uisongselPopulateData (manage->mmsongsel);
  logProcEnd (LOG_PROC, "manageSameSongChangeMark", "");
}

/* remove song */

static bool
manageMarkSongRemoved (void *udata)
{
  manageui_t  *manage = udata;
  nlist_t     *sellist;
  nlistidx_t  iteridx;
  dbidx_t     dbidx;

  logProcBegin (LOG_PROC, "manageMarkSongRemoved");

  sellist = uisongselGetSelectedList (manage->mmsongsel);

  nlistStartIterator (sellist, &iteridx);
  while ((dbidx = nlistIterateKey (sellist, &iteridx)) >= 0) {
    char    tmp [40];
    song_t  *song;

    song = dbGetByIdx (manage->musicdb, dbidx);
    if (song == NULL) {
      continue;
    }
    nlistSetStr (manage->removelist, dbidx, songGetStr (song, TAG_URI));
    dbMarkEntryRemoved (manage->musicdb, dbidx);
    snprintf (tmp, sizeof (tmp), "%d", dbidx);
    connSendMessage (manage->conn, ROUTE_STARTERUI, MSG_DB_ENTRY_REMOVE, tmp);
    manage->songeditdbidx = -1;
  }

  /* need to re-filter the songs */
  uisongselApplySongFilter (manage->mmsongsel);
  manageRePopulateData (manage);
  uisongselSetDefaultSelection (manage->mmsongsel);

  uiWidgetSetState (manage->wcont [MANAGE_W_MENU_UNDO_REMOVE], UIWIDGET_ENABLE);

  nlistFree (sellist);
  logProcEnd (LOG_PROC, "manageMarkSongRemoved", "");
  return UICB_CONT;
}

static bool
manageUndoRemove (void *udata)
{
  manageui_t  *manage = udata;
  nlistidx_t  iteridx;
  dbidx_t     dbidx;

  nlistStartIterator (manage->removelist, &iteridx);
  while ((dbidx = nlistIterateKey (manage->removelist, &iteridx)) >= 0) {
    char    tmp [40];

    dbClearEntryRemoved (manage->musicdb, dbidx);
    snprintf (tmp, sizeof (tmp), "%d", dbidx);
    connSendMessage (manage->conn, ROUTE_STARTERUI, MSG_DB_ENTRY_UNREMOVE, tmp);
  }

  nlistFree (manage->removelist);
  manage->removelist = nlistAlloc ("remove-list", LIST_ORDERED, NULL);
  uiWidgetSetState (manage->wcont [MANAGE_W_MENU_UNDO_REMOVE], UIWIDGET_DISABLE);

  /* need to re-filter the songs */
  uisongselApplySongFilter (manage->mmsongsel);
  manageRePopulateData (manage);

  return UICB_CONT;
}

static void
manageRemoveSongs (manageui_t *manage)
{
  nlistidx_t  iteridx;
  dbidx_t     dbidx;

  if (nlistGetCount (manage->removelist) == 0) {
    return;
  }

  nlistStartIterator (manage->removelist, &iteridx);
  while ((dbidx = nlistIterateKey (manage->removelist, &iteridx)) >= 0) {
    const char  *songfn;

    songfn = nlistGetStr (manage->removelist, dbidx);
    audiosrcRemove (songfn);
  }
}
