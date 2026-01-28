/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
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
#include "continst.h"
#include "dance.h"
#include "datafile.h"
#include "dispsel.h"
#include "expimp.h"
#include "expimpbdj4.h"
#include "fileop.h"
#include "filemanip.h"
#include "grouping.h"
#include "imppl.h"
#include "itunes.h"
#include "localeutil.h"
#include "lock.h"
#include "log.h"
#include "manageui.h"
#include "mdebug.h"
#include "msgparse.h"
#include "musicq.h"
#include "oslocale.h"
#include "ossignal.h"
#include "osuiutils.h"
#include "pathbld.h"
#include "pathinfo.h"
#include "pathutil.h"
#include "playlist.h"
#include "podcast.h"
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
#include "uidd.h"
#include "uiexppl.h"
#include "uiexpimpbdj4.h"
#include "uiimppl.h"
#include "uimusicq.h"
#include "uinbutil.h"
#include "uiplayer.h"
#include "uiplaylist.h"
#include "uiselectfile.h"
#include "uisongedit.h"
#include "uisongfilter.h"
#include "uisongsel.h"
#include "uiutils.h"
#include "uivnb.h"

enum {
  /* main tabs */
  MANAGE_TAB_MAIN_SL,
  MANAGE_TAB_MAIN_SEQ,
  MANAGE_TAB_MAIN_PLMGMT,
  MANAGE_TAB_MAIN_MM,
  MANAGE_TAB_MAIN_UPDDB,
  /* sl sub-tabs */
  MANAGE_TAB_SONGLIST,
  MANAGE_TAB_SL_SONGSEL,
  MANAGE_TAB_STATISTICS,
  /* mm sub-tabs */
  MANAGE_TAB_MM,
  MANAGE_TAB_SONGEDIT,
  MANAGE_TAB_AUDIOID,
};

enum {
  MANAGE_NB_MAIN,
  MANAGE_NB_SONGLIST,
  MANAGE_NB_MM,
  MANAGE_NB_MAX,
};

enum {
  /* music manager */
  MANAGE_MENU_CB_MM_CLEAR_MARK,
  MANAGE_MENU_CB_MM_SET_MARK,
  MANAGE_MENU_CB_MM_REMOVE_SONG,
  MANAGE_MENU_CB_MM_UNDO_REMOVE,
  MANAGE_MENU_CB_MM_UNDO_ALL_REMOVE,
  MANAGE_MENU_CB_MM_IMPORT,
  /* song editor */
  MANAGE_MENU_CB_SE_BPM,
  MANAGE_MENU_CB_SE_COPY_TAGS,
  MANAGE_MENU_CB_SE_APPLY_EDIT_ALL,
  MANAGE_MENU_CB_SE_CANCEL_EDIT_ALL,
  MANAGE_MENU_CB_SE_START_EDIT_ALL,
  MANAGE_MENU_CB_SE_TRIM_SILENCE,
  MANAGE_MENU_CB_SE_WRITE_AUDIO_TAGS,
  /* apply adjustment and restore original will go away someday */
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
  MANAGE_MENU_CB_SL_EXPORT,
  MANAGE_MENU_CB_SL_MP3_EXP,
  MANAGE_MENU_CB_SL_BDJ4_EXP,
  MANAGE_MENU_CB_SL_BDJ4_IMP,
  /* sl import menu */
  MANAGE_MENU_CB_SL_IMPORT,
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
  MANAGE_CB_EDIT_SS,
  MANAGE_CB_EDIT_SL,
  MANAGE_CB_SEQ_LOAD,
  MANAGE_CB_SEQ_NEW,
  MANAGE_CB_PL_LOAD,
  MANAGE_CB_SAVE,
  MANAGE_CB_CFPL_DIALOG,
  MANAGE_CB_ITUNES_DIALOG,
  MANAGE_CB_APPLY_ADJ,
  MANAGE_CB_SL_SEL_FILE,
  MANAGE_CB_BDJ4_EXP,
  MANAGE_CB_BDJ4_IMP,
  MANAGE_CB_IMP_PL,
  MANAGE_CB_EXP_PL,
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
  MANAGE_W_CFPL_DIALOG,
  MANAGE_W_ERROR_MSG,
  MANAGE_W_ITUNES_SEL_DIALOG,
  MANAGE_W_MENUBAR,
  MANAGE_W_MENUITEM_RESTORE_ORIG,
  MANAGE_W_MENUITEM_UNDO_REMOVE,
  MANAGE_W_MENUITEM_UNDO_ALL_REMOVE,
  MANAGE_W_MENU_MM,
  MANAGE_W_MENU_SL,
  MANAGE_W_MENU_SONGEDIT,
  MANAGE_W_MM_NB,
  MANAGE_W_SL_MUSICQ_TAB,         // not owner
  MANAGE_W_SL_SBS_MUSICQ_TAB,
  MANAGE_W_SONGLIST_NB,
  MANAGE_W_SONGSEL_TAB,           // not owner
  MANAGE_W_STATUS_MSG,
  MANAGE_W_WINDOW,
  MANAGE_W_SELECT_BUTTON,
  MANAGE_W_CFPL_TM_LIMIT,
  MANAGE_W_MAX,
};

typedef struct {
  progstate_t       *progstate;
  char              *locknm;
  conn_t            *conn;
  procutil_t        *processes [ROUTE_MAX];
  callback_t        *callbacks [MANAGE_CB_MAX];
  manageinfo_t      minfo;
  musicdb_t         *musicdb;
  grouping_t        *grouping;
  songdb_t          *songdb;
  samesong_t        *samesong;
  musicqidx_t       musicqPlayIdx;
  musicqidx_t       musicqManageIdx;
  uiwcont_t         *wcont [MANAGE_W_MAX];
  uivnb_t           *mainvnb;
  continst_t        *continst;
  int               stopwaitcount;
  /* notebook tab handling */
  int               maincurrtab;
  int               slcurrtab;
  int               mmcurrtab;
  uiwcont_t         *currmenu;
  uinbtabid_t       *nbtabid [MANAGE_NB_MAX];
  dbidx_t           songlistdbidx;
  dbidx_t           seldbidx;
  dbidx_t           songeditdbidx;
  /* song list ui major elements */
  uiplayer_t        *slplayer;
  uimusicq_t        *currmusicq;
  uimusicq_t        *slmusicq;
  managestats_t     *slstats;
  uisongsel_t       *slsongsel;
  uimusicq_t        *slsbsmusicq;
  uisongsel_t       *slsbssongsel;
  char              sloldname [MAX_PL_NM_LEN];
  itunes_t          *itunes;
  uidd_t            *itunesdd;
  ilist_t           *itunesddlist;
  /* prior name is used by create-from-playlist */
  uisongfilter_t    *uisongfilter;
  uiplaylist_t      *cfpl;
  char              slpriorname [MAX_PL_NM_LEN];
  char              *cfplfn;
  /* music manager ui */
  uiplayer_t        *mmplayer;
  uisongsel_t       *mmsongsel;
  uisongedit_t      *mmsongedit;
  /* lastmmdisp is the last type of display that was in the mm */
  /* it may have been a playlist filter or the entire song selection */
  int               lastmmdisp;
  /* lasttabsel: one of MANAGE_TAB_SONGLIST or MANAGE_TAB_SL_SONGSEL */
  /* this is needed so that the side-by-side view will work */
  int               lasttabsel;
  int               dbchangecount;
  int               editmode;
  int               lastinsertlocation;
  mp_musicqupdate_t *musicqupdate [MUSICQ_MAX];
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
  /* import/export playlist */
  uiexppl_t         *uiexppl;
  uiimppl_t         *uiimppl;
  int               pltype;
  slist_t           *songidxlist;
  const char        *impplname;
  imppl_t           *imppl;
  mstime_t          impplChkTime;
  int               impplstate;
  /* export/import bdj4 */
  uieibdj4_t        *uieibdj4;
  eibdj4_t          *eibdj4;
  mstime_t          eibdj4ChkTime;
  int               expimpbdj4state;
  /* options */
  datafile_t        *optiondf;
  /* remove song */
  nlist_t           *removelist;
  /* various flags */
  bool              bpmcounterstarted;
  bool              cfplactive;
  bool              cfplpostprocess;
  bool              dbloaded;
  bool              enablerestoreorig;
  bool              exportactive;
  bool              exportbdj4active;
  bool              importplactive;
  bool              importbdj4active;
  bool              importitunesactive;
  bool              ineditall;
  bool              inload;
  bool              musicqueueprocessflag;
  bool              musicqupdated;
  bool              optionsalloc;
  bool              pluiActive;
  bool              processactivercvd;
  bool              sbssonglist;
  bool              selbypass;
  bool              slbackupcreated;
  bool              seforcesave;
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
  { "MNG_EXP_PL_DIR",   MANAGE_EXP_PL_DIR,          VALUE_STR, NULL, DF_NORM },
  { "MNG_EXP_PL_POS_X", EXP_PL_POSITION_X,          VALUE_NUM, NULL, DF_NORM },
  { "MNG_EXP_PL_POS_Y", EXP_PL_POSITION_Y,          VALUE_NUM, NULL, DF_NORM },
  { "MNG_EXP_PL_TYPE",  MANAGE_EXP_PL_TYPE,         VALUE_NUM, NULL, DF_NORM },
  { "MNG_IMP_BDJ4_DIR", MANAGE_IMP_BDJ4_DIR,        VALUE_STR, NULL, DF_NORM },
  { "MNG_IMP_PL_ASKEY", MANAGE_IMP_PL_ASKEY,        VALUE_NUM, NULL, DF_NORM },
  { "MNG_IMP_PL_POS_X", IMP_PL_POSITION_X,          VALUE_NUM, NULL, DF_NORM },
  { "MNG_IMP_PL_POS_Y", IMP_PL_POSITION_Y,          VALUE_NUM, NULL, DF_NORM },
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
static bool     manageInitDataCallback (void *udata, programstate_t programState);
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
static bool     manageNewSelectionSongSel (void *udata, int32_t dbidx);
static bool     manageNewSelectionSonglist (void *udata, int32_t dbidx);
static bool     manageSwitchToSongEditorSS (void *udata);
static bool     manageSwitchToSongEditorSL (void *udata);
static bool     manageSwitchToSongEditor (manageui_t *manage);
static bool     manageSongEditSaveCallback (void *udata, int32_t dbidx);
static void     manageRePopulateData (manageui_t *manage);
static void     manageSetEditMenuItems (manageui_t *manage);
static bool     manageTrimSilence (void *udata);
static bool     manageWriteAudioTags (void *udata);
static bool     manageApplyAdjDialog (void *udata);
static bool     manageApplyAdjCallback (void *udata, int32_t aaflags);
static bool     manageRestoreOrigCallback (void *udata);
static bool     manageCopyTagsStart (void *udata);
static bool     manageEditAllStart (void *udata);
static bool     manageEditAllApply (void *udata);
static bool     manageEditAllCancel (void *udata);
static void     manageReloadSongData (manageui_t *manage);
static void manageNewSelectionMoveCheck (manageui_t *manage, dbidx_t dbidx);
static void manageSetSongEditDBIdx (manageui_t *manage, int mainlasttabsel, int mmlasttabsel);
/* itunes */
static bool     managePlaylistImportiTunes (void *udata);
static void     manageiTunesCreateDialog (manageui_t *manage);
static void     manageiTunesDialogCreateList (manageui_t *manage);
static bool     manageiTunesDialogResponseHandler (void *udata, int32_t responseid);
/* music manager */
static void     manageBuildUIMusicManager (manageui_t *manage);
static void     manageMusicManagerMenu (manageui_t *manage);
/* song list */
static void     manageBuildUISongListEditor (manageui_t *manage);
static void     manageSonglistMenu (manageui_t *manage);
static bool     manageSonglistLoad (void *udata);
static int32_t  manageSonglistLoadCB (void *udata, const char *fn);
static bool     manageSonglistCopy (void *udata);
static bool     manageSonglistNew (void *udata);
static void     manageResetCreateNew (manageui_t *manage, pltype_t pltype);
static bool     manageSonglistDelete (void *udata);
static bool     manageSonglistTruncate (void *udata);
static bool     manageSonglistCreateFromPlaylist (void *udata);
static void     manageSongListCFPLCreateDialog (manageui_t *manage);
static bool     manageCFPLResponseHandler (void *udata, int32_t responseid);
static void     manageCFPLCreate (manageui_t *manage);
static void     manageCFPLPostProcess (manageui_t *manage);
static bool     manageSonglistMix (void *udata);
static bool     manageSonglistSwap (void *udata);
static void     manageSonglistLoadFile (void *udata, const char *fn, int preloadflag);
static int32_t  manageLoadPlaylistCB (void *udata, const char *fn);
static bool     manageNewPlaylistCB (void *udata);
static int32_t  manageLoadSonglistCB (void *udata, const char *fn);
static bool     manageToggleSBSSonglist (void *udata);
static void     manageSetSBSSonglist (manageui_t *manage);
static void     manageSonglistSave (manageui_t *manage);
static void     manageSetSonglistName (manageui_t *manage, const char *nm);
static bool     managePlayProcessSonglist (void *udata, int32_t dbidx, int32_t mqidx);
static bool     managePlayProcessSBSSonglist (void *udata, int32_t dbidx, int32_t mqidx);
static bool     managePlayProcessMusicManager (void *udata, int32_t dbidx, int32_t mqidx);
static bool     manageQueueProcessSonglist (void *udata, int32_t dbidx);
static bool     manageQueueProcessSBSSongList (void *udata, int32_t dbidx);
static void     manageQueueProcess (void *udata, dbidx_t dbidx, int mqidx, int dispsel, int action);
static nlistidx_t manageLoadMusicQueue (manageui_t *manage, int mqidx);
/* export playlist */
static bool managePlaylistExport (void *udata);
static bool managePlaylistExportRespHandler (void *udata, const char *fname, int type);
/* import playlist */
static bool managePlaylistImport (void *udata);
static bool managePlaylistImportRespHandler (void *udata);
static void managePlaylistImportFinalize (manageui_t *manage);

//static void managePlaylistImportCreateSonglist (manageui_t *manage, slist_t *songlist);
//static bool managePlaylistImportCreateSongs (manageui_t *manage, const char *songnm, int imptype, slist_t *songlist, slist_t *tagdata, int retain);

/* export/import bdj4 */
static bool     managePlaylistExportBDJ4 (void *udata);
static bool     managePlaylistImportBDJ4 (void *udata);
static bool     manageExportBDJ4ResponseHandler (void *udata);
static bool     manageImportBDJ4ResponseHandler (void *udata);
/* general */
static bool     manageSwitchPageMain (void *udata, int32_t pagenum);
static bool     manageSwitchPageSonglist (void *udata, int32_t pagenum);
static bool     manageSwitchPageMM (void *udata, int32_t pagenum);
static void     manageSwitchPage (manageui_t *manage, int pagenum, int which);
static void     manageSetDisplayPerSelection (manageui_t *manage, int lastmaintab);
static void     manageSetMenuCallback (manageui_t *manage, int midx, callbackFunc cb);
static void     manageSonglistLoadCheck (manageui_t *manage);
static void     manageProcessDatabaseUpdate (manageui_t *manage);
static void     manageUpdateDBPointers (manageui_t *manage);
static uimusicq_t * manageGetCurrMusicQ (manageui_t *manage);
/* bpm counter */
static bool     manageStartBPMCounter (void *udata);
static void     manageSetBPMCounter (manageui_t *manage, song_t *song);
static void     manageSendBPMCounter (manageui_t *manage);
/* same song */
static bool     manageSameSongSetMark (void *udata);
static bool     manageSameSongClearMark (void *udata);
static void     manageSameSongChangeMark (manageui_t *manage, int flag);
/* remove song */
static bool     manageMarkSongRemoved (void *udata);
static bool     manageUndoRemove (void *udata);
static bool     manageUndoRemoveAll (void *udata);
static void     manageRemoveSongs (manageui_t *manage);

static int gKillReceived = false;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  manageui_t      manage;
  char            tbuff [BDJ4_PATH_MAX];
  uint32_t        flags;
  uisetup_t       uisetup;

#if BDJ4_MEM_DEBUG
  mdebugInit ("mui");
#endif

  manage.progstate = progstateInit ("manageui");
  progstateSetCallback (manage.progstate, PROGSTATE_CONNECTING,
      manageConnectingCallback, &manage);
  progstateSetCallback (manage.progstate, PROGSTATE_WAIT_HANDSHAKE,
      manageHandshakeCallback, &manage);
  progstateSetCallback (manage.progstate, PROGSTATE_INITIALIZE_DATA,
      manageInitDataCallback, &manage);

  for (int i = 0; i < MANAGE_W_MAX; ++i) {
    manage.wcont [i] = NULL;
  }
  manage.slplayer = NULL;
  manage.currmusicq = NULL;
  manage.slmusicq = NULL;
  manage.slstats = NULL;
  manage.slsongsel = NULL;
  manage.slsbsmusicq = NULL;
  manage.slsbssongsel = NULL;
  manage.mmplayer = NULL;
  manage.mmsongsel = NULL;
  manage.mmsongedit = NULL;
  manage.uieibdj4 = NULL;
  manage.eibdj4 = NULL;
  manage.expimpbdj4state = BDJ4_STATE_OFF;
  manage.uiimppl = NULL;
  manage.uiexppl = NULL;
  manage.pltype = PLTYPE_SONGLIST;
  manage.songidxlist = NULL;
  manage.imppl = NULL;
  manage.impplname = NULL;
  manage.impplstate = BDJ4_STATE_OFF;
  manage.musicqPlayIdx = MUSICQ_MNG_PB;
  manage.musicqManageIdx = MUSICQ_SL;
  manage.stopwaitcount = 0;
  manage.currmenu = NULL;
  manage.maincurrtab = MANAGE_TAB_MAIN_SL;
  manage.slcurrtab = MANAGE_TAB_SONGLIST;
  manage.mmcurrtab = MANAGE_TAB_MM;
  manage.wcont [MANAGE_W_MENU_MM] = uiMenuAlloc ();
  manage.wcont [MANAGE_W_MENU_SL] = uiMenuAlloc ();
  manage.wcont [MANAGE_W_MENU_SONGEDIT] = uiMenuAlloc ();
  for (int i = 0; i < MANAGE_NB_MAX; ++i) {
    manage.nbtabid [i] = uinbutilIDInit ();
  }
  *manage.sloldname = '\0';
  *manage.slpriorname = '\0';
  manage.cfplfn = NULL;
  manage.itunes = NULL;
  manage.itunesdd = NULL;
  manage.itunesddlist = NULL;
  manage.slbackupcreated = false;
  manage.seforcesave = false;
  manage.inload = false;
  manage.lastmmdisp = MANAGE_DISP_SONG_SEL;
  manage.lasttabsel = MANAGE_TAB_SL_SONGSEL;
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
  manage.currtimesig = DANCE_TIMESIG_44;
  manage.cfpl = NULL;
  manage.lastinsertlocation = QUEUE_LOC_LAST;
  manage.bpmcounterstarted = false;
  manage.cfplactive = false;
  manage.cfplpostprocess = false;
  manage.dbloaded = false;
  manage.enablerestoreorig = false;
  manage.exportactive = false;
  manage.exportbdj4active = false;
  manage.importplactive = false;
  manage.importbdj4active = false;
  manage.importitunesactive = false;
  manage.ineditall = false;
  manage.musicqueueprocessflag = false;
  manage.musicqupdated = false;
  manage.pluiActive = false;
  manage.processactivercvd = false;
  manage.sbssonglist = true;
  manage.applyadjstate = BDJ4_STATE_OFF;
  manage.impitunesstate = BDJ4_STATE_OFF;
  manage.uict = NULL;
  manage.ctstate = BDJ4_STATE_OFF;
  manage.uiaa = NULL;
  manage.continst = NULL;
  for (int i = 0; i < MANAGE_CB_MAX; ++i) {
    manage.callbacks [i] = NULL;
  }
  for (int i = 0; i < MUSICQ_MAX; ++i) {
    manage.musicqupdate [i] = NULL;
  }
  manage.removelist = nlistAlloc ("remove-list", LIST_ORDERED, NULL);

  /* CONTEXT: management ui: please wait... status message */
  manage.minfo.pleasewaitmsg = _("Please wait\xe2\x80\xa6");

  procutilInitProcesses (manage.processes);

  osSetStandardSignals (manageSigHandler);

  flags = BDJ4_INIT_ALL;
  bdj4startup (argc, argv, &manage.musicdb, "mui", ROUTE_MANAGEUI, &flags);
  logProcBegin ();
  manage.grouping = groupingAlloc (manage.musicdb);

  manage.songdb = songdbAlloc (manage.musicdb);
  manage.minfo.dispsel = dispselAlloc (DISP_SEL_LOAD_MANAGE);
  manage.minfo.musicdb = manage.musicdb;

  listenPort = bdjvarsGetNum (BDJVL_PORT_MANAGEUI);
  manage.conn = connInit (ROUTE_MANAGEUI);

  pathbldMakePath (tbuff, sizeof (tbuff),
      MANAGEUI_OPT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  manage.optiondf = datafileAllocParse ("ui-manage", DFTYPE_KEY_VAL, tbuff,
      manageuidfkeys, MANAGEUI_DFKEY_COUNT, DF_NO_OFFSET, NULL);
  manage.minfo.options = datafileGetList (manage.optiondf);
  manage.optionsalloc = false;
  if (manage.minfo.options == NULL ||
      nlistGetCount (manage.minfo.options) == 0) {
    manage.optionsalloc = true;
    manage.minfo.options = nlistAlloc ("ui-manage", LIST_ORDERED, NULL);

    nlistSetNum (manage.minfo.options, SONGSEL_FILTER_POSITION_X, -1);
    nlistSetNum (manage.minfo.options, SONGSEL_FILTER_POSITION_Y, -1);
    nlistSetNum (manage.minfo.options, MANAGE_POSITION_X, -1);
    nlistSetNum (manage.minfo.options, MANAGE_POSITION_Y, -1);
    nlistSetNum (manage.minfo.options, MANAGE_SIZE_X, 1000);
    nlistSetNum (manage.minfo.options, MANAGE_SIZE_Y, 600);
    nlistSetStr (manage.minfo.options, SONGSEL_SORT_BY, "TITLE");
    nlistSetNum (manage.minfo.options, MANAGE_SELFILE_POSITION_X, -1);
    nlistSetNum (manage.minfo.options, MANAGE_SELFILE_POSITION_Y, -1);
    nlistSetNum (manage.minfo.options, MANAGE_SBS_SONGLIST, true);
    nlistSetNum (manage.minfo.options, MANAGE_CFPL_POSITION_X, -1);
    nlistSetNum (manage.minfo.options, MANAGE_CFPL_POSITION_Y, -1);
    nlistSetNum (manage.minfo.options, APPLY_ADJ_POSITION_X, -1);
    nlistSetNum (manage.minfo.options, APPLY_ADJ_POSITION_Y, -1);
    nlistSetNum (manage.minfo.options, COPY_TAGS_POSITION_X, -1);
    nlistSetNum (manage.minfo.options, COPY_TAGS_POSITION_Y, -1);
    nlistSetNum (manage.minfo.options, EXP_IMP_BDJ4_POSITION_X, -1);
    nlistSetNum (manage.minfo.options, EXP_IMP_BDJ4_POSITION_Y, -1);
    nlistSetStr (manage.minfo.options, MANAGE_EXP_BDJ4_DIR, "");
    nlistSetStr (manage.minfo.options, MANAGE_IMP_BDJ4_DIR, "");
    nlistSetNum (manage.minfo.options, EXP_PL_POSITION_X, -1);
    nlistSetNum (manage.minfo.options, EXP_PL_POSITION_Y, -1);
    nlistSetNum (manage.minfo.options, MANAGE_EXP_PL_TYPE, EI_TYPE_M3U);
    nlistSetStr (manage.minfo.options, MANAGE_EXP_PL_DIR, "");
    nlistSetNum (manage.minfo.options, MANAGE_AUDIOID_PANE_POSITION, -1);
    nlistSetNum (manage.minfo.options, QE_POSITION_X, -1);
    nlistSetNum (manage.minfo.options, QE_POSITION_Y, -1);
    nlistSetNum (manage.minfo.options, IMP_PL_POSITION_X, -1);
    nlistSetNum (manage.minfo.options, IMP_PL_POSITION_Y, -1);
    nlistSetNum (manage.minfo.options, MANAGE_IMP_PL_ASKEY, -1);
  }
  manage.sbssonglist = nlistGetNum (manage.minfo.options, MANAGE_SBS_SONGLIST);

  uiUIInitialize (sysvarsGetNum (SVL_LOCALE_TEXT_DIR));
  uiutilsInitSetup (&uisetup);
  uiSetUICSS (&uisetup);

  manageBuildUI (&manage);
  osuiFinalize ();

  /* register these after calling the sub-window initialization */
  /* then these will be run last, after the other closing callbacks */
  progstateSetCallback (manage.progstate, PROGSTATE_STOPPING,
      manageStoppingCallback, &manage);
  progstateSetCallback (manage.progstate, PROGSTATE_STOP_WAIT,
      manageStopWaitCallback, &manage);
  progstateSetCallback (manage.progstate, PROGSTATE_CLOSING,
      manageClosingCallback, &manage);

  sockhMainLoop (listenPort, manageProcessMsg, manageMainLoop, &manage);
  connFree (manage.conn);
  progstateFree (manage.progstate);
  logProcEnd ("");
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

  logProcBegin ();

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

  uiWindowGetSize (manage->minfo.window, &x, &y);
  nlistSetNum (manage->minfo.options, MANAGE_SIZE_X, x);
  nlistSetNum (manage->minfo.options, MANAGE_SIZE_Y, y);
  uiWindowGetPosition (manage->minfo.window, &x, &y, &ws);
  nlistSetNum (manage->minfo.options, MANAGE_POSITION_X, x);
  nlistSetNum (manage->minfo.options, MANAGE_POSITION_Y, y);

  logProcEnd ("");
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

  logProcBegin ();

  uiCloseWindow (manage->minfo.window);
  uiCleanup ();

  contInstanceFree (manage->continst);
  manageDbClose (manage->managedb);

  for (int i = 0; i < MUSICQ_MAX; ++i) {
    msgparseMusicQueueDataFree (manage->musicqupdate [i]);
  }
  for (int i = 0; i < MANAGE_W_MAX; ++i) {
    if (i == MANAGE_W_SL_MUSICQ_TAB ||
        i == MANAGE_W_SONGSEL_TAB) {
      continue;
    }
    uiwcontFree (manage->wcont [i]);
  }
  itunesFree (manage->itunes);
  uiddFree (manage->itunesdd);
  ilistFree (manage->itunesddlist);
  samesongFree (manage->samesong);
  uicopytagsFree (manage->uict);
  uiaaFree (manage->uiaa);
  uieibdj4Free (manage->uieibdj4);
  eibdj4Free (manage->eibdj4);
  uiimpplFree (manage->uiimppl);
  uiexpplFree (manage->uiexppl);
  nlistFree (manage->removelist);
  slistFree (manage->songidxlist);
  uivnbFree (manage->mainvnb);

  procutilStopAllProcess (manage->processes, manage->conn, PROCUTIL_FORCE_TERM);
  procutilFreeAll (manage->processes);

  datafileSave (manage->optiondf, NULL, manage->minfo.options, DF_NO_OFFSET, 1);

  groupingFree (manage->grouping);
  bdj4shutdown (ROUTE_MANAGEUI, manage->musicdb);
  manageSequenceFree (manage->manageseq);
  managePlaylistFree (manage->managepl);
  manageDbFree (manage->managedb);
  manageAudioIdFree (manage->manageaudioid);

  songdbFree (manage->songdb);
  uisfFree (manage->uisongfilter);
  dataFree (manage->cfplfn);
  for (int i = 0; i < MANAGE_NB_MAX; ++i) {
    uinbutilIDFree (manage->nbtabid [i]);
  }

  uiplayerFree (manage->slplayer);
  uimusicqFree (manage->slmusicq);
  manageStatsFree (manage->slstats);
  uisongselFree (manage->slsongsel);

  uimusicqFree (manage->slsbsmusicq);
  uisongselFree (manage->slsbssongsel);

  uiplayerFree (manage->mmplayer);
  uisongselFree (manage->mmsongsel);
  uisongeditFree (manage->mmsongedit);

  uiplaylistFree (manage->cfpl);
  dispselFree (manage->minfo.dispsel);

  for (int i = 0; i < MANAGE_CB_MAX; ++i) {
    callbackFree (manage->callbacks [i]);
  }

  if (manage->optionsalloc) {
    nlistFree (manage->minfo.options);
  }
  datafileFree (manage->optiondf);

  logProcEnd ("");
  return STATE_FINISHED;
}

static void
manageBuildUI (manageui_t *manage)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *uiwidgetp;
  char        imgbuff [BDJ4_PATH_MAX];
  char        tbuff [BDJ4_PATH_MAX];
  int         x, y;
  uiutilsaccent_t accent;

  logProcBegin ();
  *imgbuff = '\0';

  /* CONTEXT: manage-ui: management user interface window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Management"),
      bdjoptGetStr (OPT_P_PROFILENAME));

  manage->callbacks [MANAGE_CB_CLOSE] = callbackInit (
      manageCloseWin, manage, NULL);
  manage->minfo.window = uiCreateMainWindow (
      manage->callbacks [MANAGE_CB_CLOSE], tbuff, "bdj4_icon_manage");
  manage->wcont [MANAGE_W_WINDOW] = manage->minfo.window;

  manageInitializeUI (manage);

  vbox = uiCreateVertBox (NULL);
  uiWindowPackInWindow (manage->minfo.window, vbox);
  uiWidgetSetAllMargins (vbox, 4);

  uiutilsAddProfileColorDisplay (vbox, &accent);
  hbox = accent.hbox;
  uiwcontFree (accent.cbox);

  uiwidgetp = uiCreateLabel ("");
  uiWidgetAddClass (uiwidgetp, ERROR_CLASS);
  uiBoxPackEnd (hbox, uiwidgetp);
  manage->minfo.errorMsg = uiwidgetp;
  manage->wcont [MANAGE_W_ERROR_MSG] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("");
  uiWidgetAddClass (uiwidgetp, ACCENT_CLASS);
  uiBoxPackEnd (hbox, uiwidgetp);
  manage->minfo.statusMsg = uiwidgetp;
  manage->wcont [MANAGE_W_STATUS_MSG] = uiwidgetp;

  manage->wcont [MANAGE_W_MENUBAR] = uiCreateMenubar ();
  uiBoxPackStartExpand (hbox, manage->wcont [MANAGE_W_MENUBAR]);

  manage->mainvnb = uivnbCreate ("vnb-manage", vbox);

  uiwcontFree (vbox);

  /* edit song lists */
  manageBuildUISongListEditor (manage);

  /* sequence editor */
  manage->manageseq = manageSequenceAlloc (&manage->minfo);

  vbox = uiCreateVertBox (NULL);
  manageBuildUISequence (manage->manageseq, vbox);
  uivnbAppendPage (manage->mainvnb, vbox,
      /* CONTEXT: manage-ui: notebook tab title: edit sequences */
      _("Edit Sequences"), MANAGE_TAB_MAIN_SEQ);
  uiWidgetSetAllMargins (vbox, 2);

  /* playlist management */
  manage->managepl = managePlaylistAlloc (&manage->minfo);

  uiwcontFree (vbox);
  vbox = uiCreateVertBox (NULL);
  manageBuildUIPlaylist (manage->managepl, vbox);

  uivnbAppendPage (manage->mainvnb, vbox,
      /* CONTEXT: manage-ui: notebook tab title: playlist management */
      _("Playlist Management"), MANAGE_TAB_MAIN_PLMGMT);
  uiWidgetSetAllMargins (vbox, 2);

  /* music manager */
  manageBuildUIMusicManager (manage);

  /* update database */
  manage->managedb = manageDbAlloc (&manage->minfo,
      manage->conn, manage->processes);

  uiwcontFree (vbox);
  vbox = uiCreateVertBox (NULL);
  manageBuildUIUpdateDatabase (manage->managedb, vbox);
  uivnbAppendPage (manage->mainvnb, vbox,
      /* CONTEXT: manage-ui: notebook tab title: update database */
      _("Update Database"), MANAGE_TAB_MAIN_UPDDB);

  x = nlistGetNum (manage->minfo.options, MANAGE_SIZE_X);
  y = nlistGetNum (manage->minfo.options, MANAGE_SIZE_Y);
  uiWindowSetDefaultSize (manage->minfo.window, x, y);

  manage->callbacks [MANAGE_CB_MAIN_NB] = callbackInitI (
      manageSwitchPageMain, manage);
  uivnbSetCallback (manage->mainvnb, manage->callbacks [MANAGE_CB_MAIN_NB]);

  uiWidgetShowAll (manage->minfo.window);

  x = nlistGetNum (manage->minfo.options, MANAGE_POSITION_X);
  y = nlistGetNum (manage->minfo.options, MANAGE_POSITION_Y);
  uiWindowMove (manage->minfo.window, x, y, -1);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_manage", BDJ4_IMG_PNG_EXT, PATHBLD_MP_DIR_IMG);
  osuiSetIcon (imgbuff);

  manage->callbacks [MANAGE_CB_NEW_SEL_SONGSEL] = callbackInitI (
      manageNewSelectionSongSel, manage);
  uisongselSetSelectionCallback (manage->slsbssongsel,
      manage->callbacks [MANAGE_CB_NEW_SEL_SONGSEL]);
  uisongselSetSelectionCallback (manage->slsongsel,
      manage->callbacks [MANAGE_CB_NEW_SEL_SONGSEL]);
  uisongselSetSelectionCallback (manage->mmsongsel,
      manage->callbacks [MANAGE_CB_NEW_SEL_SONGSEL]);

  manage->callbacks [MANAGE_CB_NEW_SEL_SONGLIST] = callbackInitI (
      manageNewSelectionSonglist, manage);
  uimusicqSetSelectionCallback (manage->slmusicq,
      manage->callbacks [MANAGE_CB_NEW_SEL_SONGLIST]);
  uimusicqSetSelectionCallback (manage->slsbsmusicq,
      manage->callbacks [MANAGE_CB_NEW_SEL_SONGLIST]);

  manage->callbacks [MANAGE_CB_SEQ_LOAD] = callbackInitS (
      manageLoadPlaylistCB, manage);
  manageSequenceSetLoadCallback (manage->manageseq,
      manage->callbacks [MANAGE_CB_SEQ_LOAD]);

  manage->callbacks [MANAGE_CB_SEQ_NEW] = callbackInit (
      manageNewPlaylistCB, manage, NULL);
  manageSequenceSetNewCallback (manage->manageseq,
      manage->callbacks [MANAGE_CB_SEQ_NEW]);

  manage->callbacks [MANAGE_CB_PL_LOAD] = callbackInitS (
      manageLoadSonglistCB, manage);
  managePlaylistSetLoadCallback (manage->managepl,
      manage->callbacks [MANAGE_CB_PL_LOAD]);

  manage->selbypass = false;

  /* make sure the curr-musicq is set */
  manage->currmusicq = manageGetCurrMusicQ (manage);
  /* set up the initial menu */
  manageSwitchPage (manage, 0, MANAGE_NB_SONGLIST);

  uiwcontFree (hbox);
  uiwcontFree (vbox);

  /* set a default selection.  this will also set the song editor dbidx */
  uisongselSetSelection (manage->mmsongsel, 0);

  logProcEnd ("");
}

static void
manageInitializeUI (manageui_t *manage)
{
  manage->callbacks [MANAGE_CB_SL_SEL_FILE] =
      callbackInitS (manageSonglistLoadCB, manage);
  manage->callbacks [MANAGE_CB_CFPL_DIALOG] = callbackInitI (
      manageCFPLResponseHandler, manage);
  manage->callbacks [MANAGE_CB_ITUNES_DIALOG] = callbackInitI (
      manageiTunesDialogResponseHandler, manage);

  manage->samesong = samesongAlloc (manage->musicdb);
  manage->uisongfilter = uisfInit (manage->minfo.window, manage->minfo.options,
      SONG_FILTER_FOR_SELECTION);

  manage->slplayer = uiplayerInit ("sl-player", manage->progstate, manage->conn,
      manage->musicdb, manage->minfo.dispsel);
  manage->slmusicq = uimusicqInit ("m-sl", manage->conn,
      manage->musicdb, manage->minfo.dispsel, DISP_SEL_SONGLIST);
  uimusicqSetPlayIdx (manage->slmusicq, manage->musicqPlayIdx);
  uimusicqSetManageIdx (manage->slmusicq, manage->musicqManageIdx);
  manage->slstats = manageStatsInit (manage->conn, manage->musicdb);
  manage->slsongsel = uisongselInit ("m-sl-songsel", manage->conn,
      manage->musicdb, manage->grouping,
      manage->minfo.dispsel, manage->samesong,
      manage->minfo.options, manage->uisongfilter, DISP_SEL_SONGSEL);
  manage->callbacks [MANAGE_CB_PLAY_SL] = callbackInitII (
      managePlayProcessSonglist, manage);
  uisongselSetPlayCallback (manage->slsongsel,
      manage->callbacks [MANAGE_CB_PLAY_SL]);
  manage->callbacks [MANAGE_CB_QUEUE_SL] = callbackInitI (
      manageQueueProcessSonglist, manage);
  uisongselSetQueueCallback (manage->slsongsel,
      manage->callbacks [MANAGE_CB_QUEUE_SL]);

  manage->slsbsmusicq = uimusicqInit ("m-sbs-sl", manage->conn,
      manage->musicdb, manage->minfo.dispsel, DISP_SEL_SBS_SONGLIST);
  manage->slsbssongsel = uisongselInit ("m-sbs-songsel", manage->conn,
      manage->musicdb, manage->grouping,
      manage->minfo.dispsel, manage->samesong,
      manage->minfo.options, manage->uisongfilter, DISP_SEL_SBS_SONGSEL);
  uimusicqSetPlayIdx (manage->slsbsmusicq, manage->musicqPlayIdx);
  uimusicqSetManageIdx (manage->slsbsmusicq, manage->musicqManageIdx);
  manage->callbacks [MANAGE_CB_PLAY_SL_SBS] = callbackInitII (
      managePlayProcessSBSSonglist, manage);
  uisongselSetPlayCallback (manage->slsbssongsel,
      manage->callbacks [MANAGE_CB_PLAY_SL_SBS]);
  manage->callbacks [MANAGE_CB_QUEUE_SL_SBS] = callbackInitI (
      manageQueueProcessSBSSongList, manage);
  uisongselSetQueueCallback (manage->slsbssongsel,
      manage->callbacks [MANAGE_CB_QUEUE_SL_SBS]);

  manage->mmplayer = uiplayerInit ("mm-player", manage->progstate, manage->conn,
      manage->musicdb, manage->minfo.dispsel);
  manage->mmsongsel = uisongselInit ("m-mm-songsel", manage->conn,
      manage->musicdb, manage->grouping,
      manage->minfo.dispsel, manage->samesong,
      manage->minfo.options, manage->uisongfilter, DISP_SEL_MM);
  manage->callbacks [MANAGE_CB_PLAY_MM] = callbackInitII (
      managePlayProcessMusicManager, manage);
  uisongselSetPlayCallback (manage->mmsongsel,
      manage->callbacks [MANAGE_CB_PLAY_MM]);

  manage->mmsongedit = uisongeditInit (manage->conn,
      manage->musicdb, manage->minfo.dispsel, manage->minfo.options);

  manage->callbacks [MANAGE_CB_EDIT_SL] = callbackInit (
      manageSwitchToSongEditorSL, manage, NULL);
  manage->callbacks [MANAGE_CB_EDIT_SS] = callbackInit (
      manageSwitchToSongEditorSS, manage, NULL);
  uisongselSetEditCallback (manage->slsongsel, manage->callbacks [MANAGE_CB_EDIT_SS]);
  uisongselSetEditCallback (manage->slsbssongsel, manage->callbacks [MANAGE_CB_EDIT_SS]);
  uisongselSetEditCallback (manage->mmsongsel, manage->callbacks [MANAGE_CB_EDIT_SS]);
  uimusicqSetEditCallback (manage->slmusicq, manage->callbacks [MANAGE_CB_EDIT_SL]);
  uimusicqSetEditCallback (manage->slsbsmusicq, manage->callbacks [MANAGE_CB_EDIT_SL]);

  manage->callbacks [MANAGE_CB_SAVE] = callbackInitI (
      manageSongEditSaveCallback, manage);
  uisongeditSetSaveCallback (manage->mmsongedit, manage->callbacks [MANAGE_CB_SAVE]);
  uisongselSetSongSaveCallback (manage->slsongsel, manage->callbacks [MANAGE_CB_SAVE]);
  uisongselSetSongSaveCallback (manage->slsbssongsel, manage->callbacks [MANAGE_CB_SAVE]);
  uisongselSetSongSaveCallback (manage->mmsongsel, manage->callbacks [MANAGE_CB_SAVE]);
  uimusicqSetSongSaveCallback (manage->slmusicq, manage->callbacks [MANAGE_CB_SAVE]);
  uimusicqSetSongSaveCallback (manage->slsbsmusicq, manage->callbacks [MANAGE_CB_SAVE]);

  manage->uieibdj4 = uieibdj4Init (manage->minfo.window, manage->minfo.options);
  manage->callbacks [MANAGE_CB_BDJ4_EXP] = callbackInit (
      manageExportBDJ4ResponseHandler, manage, NULL);
  uieibdj4SetResponseCallback (manage->uieibdj4,
      manage->callbacks [MANAGE_CB_BDJ4_EXP], UIEIBDJ4_EXPORT);
  manage->callbacks [MANAGE_CB_BDJ4_IMP] = callbackInit (
      manageImportBDJ4ResponseHandler, manage, NULL);
  uieibdj4SetResponseCallback (manage->uieibdj4,
      manage->callbacks [MANAGE_CB_BDJ4_IMP], UIEIBDJ4_IMPORT);

  manage->uiimppl = uiimpplInit (manage->minfo.window, manage->minfo.options,
      manage->minfo.pleasewaitmsg);
  manage->callbacks [MANAGE_CB_IMP_PL] = callbackInit (
      managePlaylistImportRespHandler, manage, NULL);
  uiimpplSetResponseCallback (manage->uiimppl,
      manage->callbacks [MANAGE_CB_IMP_PL]);

  manage->uiexppl = uiexpplInit (manage->minfo.window, manage->minfo.options);
  manage->callbacks [MANAGE_CB_EXP_PL] = callbackInitSI (
      managePlaylistExportRespHandler, manage);
  uiexpplSetResponseCallback (manage->uiexppl,
      manage->callbacks [MANAGE_CB_EXP_PL]);
}

static void
manageBuildUISongListEditor (manageui_t *manage)
{
  uiwcont_t   *uip;
  uiwcont_t   *uiwidgetp;
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *mainhbox;
  const char  *buttonnm;

  /* management */

  vbox = uiCreateVertBox (NULL);

  uivnbAppendPage (manage->mainvnb, vbox,
      /* CONTEXT: manage-ui: notebook tab title: edit song lists */
      _("Edit Song Lists"), MANAGE_TAB_MAIN_SL);
  uiWidgetSetAllMargins (vbox, 2);

  /* management: player */
  uiwidgetp = uiplayerBuildUI (manage->slplayer);
  uiBoxPackStart (vbox, uiwidgetp);

  manage->wcont [MANAGE_W_SONGLIST_NB] = uiCreateNotebook ("nb-songlist");
  uiBoxPackStartExpand (vbox, manage->wcont [MANAGE_W_SONGLIST_NB]);

  /* management: side-by-side view tab */
  mainhbox = uiCreateHorizBox (NULL);

  uiNotebookAppendPage (manage->wcont [MANAGE_W_SONGLIST_NB], mainhbox,
      /* CONTEXT: manage-ui: name of side-by-side view tab */
      _("Song List"), NULL);
  uinbutilIDAdd (manage->nbtabid [MANAGE_NB_SONGLIST], MANAGE_TAB_SONGLIST);
  manage->wcont [MANAGE_W_SL_SBS_MUSICQ_TAB] = mainhbox;

  hbox = uiCreateHorizBox (NULL);
  uiBoxPackStartExpand (mainhbox, hbox);

  uiwidgetp = uimusicqBuildUI (manage->slsbsmusicq, manage->minfo.window,
      MUSICQ_SL, manage->minfo.errorMsg,
      manage->minfo.statusMsg, uiutilsValidatePlaylistNameClr);
  uiBoxPackStartExpand (hbox, uiwidgetp);

  uiwcontFree (vbox);

  vbox = uiCreateVertBox (NULL);

  uiBoxPackStart (hbox, vbox);
  uiWidgetSetAllMargins (vbox, 4);
  uiWidgetSetMarginTop (vbox, 64);

  buttonnm = "button_left";
  if (sysvarsGetNum (SVL_LOCALE_TEXT_DIR) == TEXT_DIR_RTL) {
    buttonnm = "button_right";
  }

  manage->callbacks [MANAGE_CB_SBS_SELECT] = callbackInit (
      uisongselSelectCallback, manage->slsbssongsel, NULL);
  uiwidgetp = uiCreateButton ("b-mng-sbs-select",
      manage->callbacks [MANAGE_CB_SBS_SELECT],
      /* CONTEXT: manage-ui: config: button: add the selected songs to the song list */
      _("Select"), buttonnm);
  uiBoxPackStart (vbox, uiwidgetp);
  manage->wcont [MANAGE_W_SELECT_BUTTON] = uiwidgetp;

  uiwidgetp = uisongselBuildUI (manage->slsbssongsel, manage->minfo.window);
  uiBoxPackStartExpand (hbox, uiwidgetp);

  /* song list: music queue tab */
  uip = uimusicqBuildUI (manage->slmusicq, manage->minfo.window,
      MUSICQ_SL, manage->minfo.errorMsg,
      manage->minfo.statusMsg, uiutilsValidatePlaylistNameClr);
  uiNotebookAppendPage (manage->wcont [MANAGE_W_SONGLIST_NB], uip,
      /* CONTEXT: manage-ui: name of song list notebook tab */
      _("Song List"), NULL);
  uinbutilIDAdd (manage->nbtabid [MANAGE_NB_SONGLIST], MANAGE_TAB_SONGLIST);
  manage->wcont [MANAGE_W_SL_MUSICQ_TAB] = uip;

  /* song list: song selection tab */
  uip = uisongselBuildUI (manage->slsongsel, manage->minfo.window);
  uiNotebookAppendPage (manage->wcont [MANAGE_W_SONGLIST_NB], uip,
      /* CONTEXT: manage-ui: name of song selection notebook tab */
      _("Song Selection"), NULL);
  uinbutilIDAdd (manage->nbtabid [MANAGE_NB_SONGLIST], MANAGE_TAB_SL_SONGSEL);
  manage->wcont [MANAGE_W_SONGSEL_TAB] = uip;

  /* song list editor: statistics tab */
  uip = manageBuildUIStats (manage->slstats);
  uiNotebookAppendPage (manage->wcont [MANAGE_W_SONGLIST_NB], uip,
      /* CONTEXT: manage-ui: name of statistics tab */
      _("Statistics"), NULL);
  uinbutilIDAdd (manage->nbtabid [MANAGE_NB_SONGLIST], MANAGE_TAB_STATISTICS);

  manage->callbacks [MANAGE_CB_SL_NB] = callbackInitI (
      manageSwitchPageSonglist, manage);
  uiNotebookSetCallback (manage->wcont [MANAGE_W_SONGLIST_NB], manage->callbacks [MANAGE_CB_SL_NB]);

  uiwcontFree (vbox);
  uiwcontFree (hbox);
}

static int
manageMainLoop (void *tmanage)
{
  manageui_t   *manage = tmanage;
  int         stop = SOCKH_CONTINUE;

  uiUIProcessEvents ();

  if (! progstateIsRunning (manage->progstate)) {
    progstateProcess (manage->progstate);
    if (progstateCurrState (manage->progstate) == PROGSTATE_CLOSED) {
      stop = SOCKH_STOP;
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

  if (manage->impplstate == BDJ4_STATE_PROCESS) {
    if (mstimeCheck (&manage->impplChkTime)) {
      int   count, tot;

      impplGetCount (manage->imppl, &count, &tot);
      uiutilsProgressStatus (manage->minfo.statusMsg, count, tot);
      uiimpplUpdateStatus (manage->uiimppl, count, tot);
      mstimeset (&manage->impplChkTime, 200);
    }

    if (impplProcess (manage->imppl)) {
      uiutilsProgressStatus (manage->minfo.statusMsg, 0, 0);
      uiimpplUpdateStatus (manage->uiimppl, 0, 0);
      manage->impplstate = BDJ4_STATE_FINISH;
      if (impplIsDBChanged (manage->imppl)) {
        manageProcessDatabaseUpdate (manage);
        manage->impplstate = BDJ4_STATE_WAIT;
      }
    }
  }

  if (manage->impplstate == BDJ4_STATE_WAIT) {
    if (manage->dbloaded) {
      manage->dbloaded = false;
      manage->impplstate = BDJ4_STATE_FINISH;
    }
  }

  if (manage->impplstate == BDJ4_STATE_FINISH) {
    managePlaylistImportFinalize (manage);
    uiutilsProgressStatus (manage->minfo.statusMsg, -1, -1);
    uiimpplUpdateStatus (manage->uiimppl, -1, -1);
    manage->impplstate = BDJ4_STATE_OFF;
    uiimpplDialogClear (manage->uiimppl);
  }

  if (manage->impplstate == BDJ4_STATE_START) {
    manage->impplstate = BDJ4_STATE_PROCESS;
  }

  if (manage->expimpbdj4state == BDJ4_STATE_WAIT) {
    if (manage->dbloaded) {
      manage->dbloaded = false;
      /* expimpbdj4.c will have set its db-loaded flag to false */
      /* wait for the db-reload response, and start the processing up again */
      manage->expimpbdj4state = BDJ4_STATE_PROCESS;
    }
  }

  if (manage->expimpbdj4state == BDJ4_STATE_PROCESS) {
    if (mstimeCheck (&manage->eibdj4ChkTime)) {
      int   count, tot;

      eibdj4GetCount (manage->eibdj4, &count, &tot);
      uiutilsProgressStatus (manage->minfo.statusMsg, count, tot);
      uieibdj4UpdateStatus (manage->uieibdj4, count, tot);
      mstimeset (&manage->eibdj4ChkTime, 200);
    }

    if (eibdj4Process (manage->eibdj4)) {
      if (eibdj4DatabaseChanged (manage->eibdj4)) {
        manageProcessDatabaseUpdate (manage);
        manage->expimpbdj4state = BDJ4_STATE_WAIT;
        return stop;
      }
      uiutilsProgressStatus (manage->minfo.statusMsg, -1, -1);
      uieibdj4UpdateStatus (manage->uieibdj4, -1, -1);
      manageSonglistLoadFile (manage,
          uieibdj4GetNewName (manage->uieibdj4), MANAGE_STD);
      uieibdj4DialogClear (manage->uieibdj4);
      manage->expimpbdj4state = BDJ4_STATE_OFF;
    }
  }

  if (manage->expimpbdj4state == BDJ4_STATE_START) {
    uiutilsProgressStatus (manage->minfo.statusMsg, 0, 0);
    uieibdj4UpdateStatus (manage->uieibdj4, 0, 0);
    manage->expimpbdj4state = BDJ4_STATE_PROCESS;
  }

  uieibdj4Process (manage->uieibdj4);
  uiexpplProcess (manage->uiexppl);
  uiimpplProcess (manage->uiimppl);
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
      tfn = audiosrcRelativePath (outfn, 0);
      /* if there is a prefix length, want the full path, */
      /* and tfn will be correct */
      song = dbGetByName (manage->musicdb, tfn);
      if (song != NULL) {
        dbidx_t     rrn;
        dbidx_t     dbidx;
        char        tmp [40];
        slist_t     *tagdata;
        int         rewrite;

        rrn = songGetNum (song, TAG_RRN);
        dbidx = songGetNum (song, TAG_DBIDX);

        tagdata = audiotagParseData (outfn, &rewrite);
        if (slistGetCount (tagdata) > 0) {
          song_t    *song;
          int32_t   songdbflags;

          song = songAlloc ();
          songFromTagList (song, tagdata);
          songSetStr (song, TAG_URI, tfn);
          songSetChanged (song);
          songdbflags = SONGDB_NONE;
          songdbWriteDBSong (manage->songdb, song, &songdbflags, rrn);
          songFree (song);

          dbLoadEntry (manage->musicdb, dbidx);
          manageRePopulateData (manage);
          snprintf (tmp, sizeof (tmp), "%" PRId32 "%c%" PRId32, dbidx,
              MSG_ARGS_RS, rrn);
          connSendMessage (manage->conn, ROUTE_STARTERUI, MSG_DB_ENTRY_UPDATE, tmp);
        }
      }
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
      manageReloadSongData (manage);

      snprintf (tmp, sizeof (tmp), "%" PRId32 "%c%d",
          manage->songeditdbidx, MSG_ARGS_RS, MUSICDB_ENTRY_UNK);
      connSendMessage (manage->conn, ROUTE_STARTERUI, MSG_DB_ENTRY_UPDATE, tmp);
    }
    uiLabelSetText (manage->minfo.statusMsg, "");
    uiaaDialogClear (manage->uiaa);
    manage->applyadjstate = BDJ4_STATE_OFF;
  }

  if (manage->applyadjstate == BDJ4_STATE_START) {
    uiLabelSetText (manage->minfo.statusMsg, manage->minfo.pleasewaitmsg);
    manage->applyadjstate = BDJ4_STATE_PROCESS;
  }

  /* itunes processing */

  if (manage->impitunesstate == BDJ4_STATE_PROCESS) {
    manageSonglistSave (manage);

    if (manage->itunes == NULL) {
      manage->itunes = itunesAlloc ();
    }

    itunesParse (manage->itunes);

    /* CONTEXT: manage-ui: song list: default name for a new song list */
    manageSetSonglistName (manage, _("New Song List"));

    manageiTunesCreateDialog (manage);
    uiDialogShow (manage->wcont [MANAGE_W_ITUNES_SEL_DIALOG]);

    uiLabelSetText (manage->minfo.statusMsg, "");
    manage->impitunesstate = BDJ4_STATE_OFF;
  }

  if (manage->impitunesstate == BDJ4_STATE_START) {
    uiLabelSetText (manage->minfo.statusMsg, manage->minfo.pleasewaitmsg);
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
  uisongselMainLoop (manage->mmsongsel);

  if (manage->maincurrtab == MANAGE_TAB_MAIN_MM) {
    if (manage->mmcurrtab == MANAGE_TAB_SONGEDIT) {
      /* the song edit main loop does not need to run all the time */
      uisongeditMainLoop (manage->mmsongedit);
    }
    if (manage->mmcurrtab == MANAGE_TAB_AUDIOID) {
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

  logProcBegin ();

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

  logProcEnd ("");
  return rc;
}

static bool
manageHandshakeCallback (void *udata, programstate_t programState)
{
  manageui_t   *manage = udata;
  bool        rc = STATE_NOT_FINISH;

  logProcBegin ();

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
    manageSetSBSSonglist (manage);
    /* CONTEXT: manage-ui: song list: default name for a new song list */
    manageSetSonglistName (manage, _("New Song List"));
    manage->pltype = PLTYPE_SONGLIST;
    connSendMessage (manage->conn, ROUTE_PLAYER, MSG_PLAYER_SUPPORT, NULL);
    rc = STATE_FINISHED;
  }

  logProcEnd ("");
  return rc;
}

static bool
manageInitDataCallback (void *udata, programstate_t programState)
{
  manageui_t   *manage = udata;
  bool          rc = STATE_NOT_FINISH;

  logProcBegin ();

  if (manage->processactivercvd == false) {
    return rc;
  }

  if (manage->pluiActive) {
    rc = STATE_FINISHED;
    return rc;
  }

  if (manage->continst == NULL) {
    manage->continst = contInstanceInit (manage->conn, manage->slplayer);
  }

  if (manage->continst != NULL) {
    rc = contInstanceSetup (manage->continst);
  }

  logProcEnd ("");
  return rc;
}

static int
manageProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  manageui_t  *manage = udata;

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
          uiWindowFind (manage->minfo.window);
          break;
        }
        case MSG_DB_WAIT: {
          uiLabelSetText (manage->minfo.statusMsg, manage->minfo.pleasewaitmsg);
          break;
        }
        case MSG_DB_WAIT_FINISH: {
          uiLabelSetText (manage->minfo.statusMsg, "");
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
          /* the currently open database pointer is no longer valid */
          /* if a rebuild or a compact was run */

          /* the database has been updated, tell the other processes to */
          /* reload it, and reload it ourselves */
          manageProcessDatabaseUpdate (manage);
          manageDbResetButtons (manage->managedb);
          break;
        }
        case MSG_DB_LOADED: {
          manage->dbloaded = true;
          break;
        }
        case MSG_MUSIC_QUEUE_DATA: {
          mp_musicqupdate_t   *musicqupdate;
          nlistidx_t          newcount = 0;

          musicqupdate = msgparseMusicQueueData (args);
          if (musicqupdate == NULL) {
            break;
          }

          msgparseMusicQueueDataFree (manage->musicqupdate [musicqupdate->mqidx]);
          manage->musicqupdate [musicqupdate->mqidx] = musicqupdate;
          if (musicqupdate->mqidx == manage->musicqManageIdx) {
            uimusicqSetMusicQueueData (manage->slmusicq, musicqupdate);
            uimusicqSetMusicQueueData (manage->slsbsmusicq, musicqupdate);
            newcount = manageLoadMusicQueue (manage, musicqupdate->mqidx);
            manageStatsProcessData (manage->slstats, musicqupdate);

            /* the music queue data is used to display the mark */
            /* indicating that the song is already in the song list */
            uisongselProcessMusicQueueData (manage->slsbssongsel, musicqupdate);
            uisongselProcessMusicQueueData (manage->slsongsel, musicqupdate);
          }
          if (newcount > 0) {
            manage->musicqupdated = true;
          }
          manage->musicqueueprocessflag = true;
          break;
        }
        case MSG_SONG_SELECT: {
          mp_songselect_t   *songselect;

          songselect = msgparseSongSelect (args);
          uimusicqProcessSongSelect (manage->currmusicq, songselect);
          msgparseSongSelectFree (songselect);
          break;
        }
        case MSG_PROCESS_ACTIVE: {
          int   val;
          char  tmp [40];

          val = atoi (args);
          manage->pluiActive = val;
          manage->processactivercvd = true;
          uimusicqSetPlayButtonState (manage->slmusicq, val);
          uimusicqSetPlayButtonState (manage->slsbsmusicq, val);
          uisongselSetPlayButtonState (manage->slsongsel, val);
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
          dbidx_t   dbidx;

          msgparseDBEntryUpdate (args, &dbidx);
          dbLoadEntry (manage->musicdb, dbidx);
          manageRePopulateData (manage);
          if (manage->maincurrtab == MANAGE_TAB_MAIN_MM &&
              manage->mmcurrtab == MANAGE_TAB_SONGEDIT) {
            manageReloadSongData (manage);
          }
          break;
        }
        case MSG_PROCESSING_FAIL: {
          /* CONTEXT: the 'Mix' action failed */
          uiLabelSetText (manage->minfo.statusMsg, _("Action failed"));
          break;
        }
        case MSG_PROCESSING_FINISH: {
          uiLabelSetText (manage->minfo.statusMsg, "");
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
  logProcBegin ();
  if (progstateCurrState (manage->progstate) <= PROGSTATE_RUNNING) {
    progstateShutdownProcess (manage->progstate);
    logMsg (LOG_DBG, LOG_MSGS, "got: close win request");
    logProcEnd ("not-done");
    return UICB_STOP;
  }

  logProcEnd ("");
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

  logProcBegin ();

  if (uiMenuIsInitialized (manage->wcont [MANAGE_W_MENU_SONGEDIT])) {
    uiMenuDisplay (manage->wcont [MANAGE_W_MENU_SONGEDIT]);
    manage->currmenu = manage->wcont [MANAGE_W_MENU_SONGEDIT];
    return;
  }

  menuitem = uiMenuAddMainItem (manage->wcont [MANAGE_W_MENUBAR],
      /* CONTEXT: manage-ui: menu selection: actions for song editor */
      manage->wcont [MANAGE_W_MENU_SONGEDIT], _("Actions"));
  menu = uiCreateSubMenu (menuitem);
  uiwcontFree (menuitem);

  manage->callbacks [MANAGE_MENU_CB_SE_BPM] = callbackInit (
      manageStartBPMCounter, manage, NULL);
  menuitem = uiMenuCreateItem (menu, tagdefs [TAG_BPM].displayname,
      manage->callbacks [MANAGE_MENU_CB_SE_BPM]);
  uiwcontFree (menuitem);

  manage->callbacks [MANAGE_MENU_CB_SE_COPY_TAGS] = callbackInit (
      manageCopyTagsStart, manage, NULL);
  /* CONTEXT: manage-ui: song editor menu: copy audio tags */
  menuitem = uiMenuCreateItem (menu, _("Copy Audio Tags"),
      manage->callbacks [MANAGE_MENU_CB_SE_COPY_TAGS]);
  uiwcontFree (menuitem);

  manage->callbacks [MANAGE_MENU_CB_SE_WRITE_AUDIO_TAGS] = callbackInit (
      manageWriteAudioTags, manage, NULL);
  /* CONTEXT: manage-ui: song editor menu: write audio tags*/
  menuitem = uiMenuCreateItem (menu, _("Write Audio Tags"),
      manage->callbacks [MANAGE_MENU_CB_SE_WRITE_AUDIO_TAGS]);
  uiwcontFree (menuitem);

  uiMenuAddSeparator (menu);

  manage->callbacks [MANAGE_MENU_CB_SE_START_EDIT_ALL] = callbackInit (
      manageEditAllStart, manage, NULL);
  /* CONTEXT: manage-ui: song editor menu: edit all */
  menuitem = uiMenuCreateItem (menu, _("Edit All"),
      manage->callbacks [MANAGE_MENU_CB_SE_START_EDIT_ALL]);
  uiwcontFree (menuitem);

  manage->callbacks [MANAGE_MENU_CB_SE_APPLY_EDIT_ALL] = callbackInit (
      manageEditAllApply, manage, NULL);
  /* CONTEXT: manage-ui: song editor menu: apply edit all */
  menuitem = uiMenuCreateItem (menu, _("Apply Edit All"),
      manage->callbacks [MANAGE_MENU_CB_SE_APPLY_EDIT_ALL]);
  uiwcontFree (menuitem);

  manage->callbacks [MANAGE_MENU_CB_SE_CANCEL_EDIT_ALL] = callbackInit (
      manageEditAllCancel, manage, NULL);
  /* CONTEXT: manage-ui: song editor menu: cancel edit all */
  menuitem = uiMenuCreateItem (menu, _("Cancel Edit All"),
      manage->callbacks [MANAGE_MENU_CB_SE_CANCEL_EDIT_ALL]);
  uiwcontFree (menuitem);

  uiMenuAddSeparator (menu);

  manage->callbacks [MANAGE_MENU_CB_SE_TRIM_SILENCE] = callbackInit (
      manageTrimSilence, manage, NULL);
  /* CONTEXT: manage-ui: song editor menu: trim silence */
  menuitem = uiMenuCreateItem (menu, _("Trim Silence"),
      manage->callbacks [MANAGE_MENU_CB_SE_TRIM_SILENCE]);
  p = sysvarsGetStr (SV_PATH_FFMPEG);
  if (p == NULL || ! *p) {
    uiWidgetSetState (menuitem, UIWIDGET_DISABLE);
  }
  uiwcontFree (menuitem);

  if (bdjoptGetNum (OPT_G_AUD_ADJ_DISP)) {
    manage->callbacks [MANAGE_MENU_CB_SE_APPLY_ADJ] = callbackInit (
        manageApplyAdjDialog, manage, NULL);
    uisongeditSetApplyAdjCallback (manage->mmsongedit, manage->callbacks [MANAGE_MENU_CB_SE_APPLY_ADJ]);
    /* CONTEXT: manage-ui: song editor menu: apply adjustments */
    menuitem = uiMenuCreateItem (menu, _("Apply Adjustments"),
        manage->callbacks [MANAGE_MENU_CB_SE_APPLY_ADJ]);
    p = sysvarsGetStr (SV_PATH_FFMPEG);
    if (p == NULL || ! *p) {
      uiWidgetSetState (menuitem, UIWIDGET_DISABLE);
    }
    uiwcontFree (menuitem);

    manage->callbacks [MANAGE_MENU_CB_SE_RESTORE_ORIG] = callbackInit (
        manageRestoreOrigCallback, manage, NULL);
    /* CONTEXT: manage-ui: song editor menu: restore original */
    menuitem = uiMenuCreateItem (menu, _("Restore Original"),
        manage->callbacks [MANAGE_MENU_CB_SE_RESTORE_ORIG]);
    manage->wcont [MANAGE_W_MENUITEM_RESTORE_ORIG] = menuitem;
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
  }

  uiMenuSetInitialized (manage->wcont [MANAGE_W_MENU_SONGEDIT]);
  uiwcontFree (menu);

  uiMenuDisplay (manage->wcont [MANAGE_W_MENU_SONGEDIT]);
  manage->currmenu = manage->wcont [MANAGE_W_MENU_SONGEDIT];

  logProcEnd ("");
}

static bool
manageNewSelectionSongSel (void *udata, int32_t dbidx)
{
  manageui_t  *manage = udata;

  logProcBegin ();
  if (manage->selbypass) {
    logProcEnd ("sel-bypass");
    return UICB_CONT;
  }
  if (dbidx < 0) {
    logProcEnd ("bad-dbidx");
    return UICB_CONT;
  }

  if (manage->maincurrtab == MANAGE_TAB_MAIN_MM) {
    int       state = UIWIDGET_DISABLE;

    if (uisfPlaylistInUse (manage->uisongfilter)) {
      logMsg (LOG_DBG, LOG_INFO, "new-selection: mm/pl-in-use set sl-dbidx");
      manage->songlistdbidx = dbidx;
    } else {
      logMsg (LOG_DBG, LOG_INFO, "new-selection: mm set sel-dbidx");
      manage->seldbidx = dbidx;
    }

    if (nlistGetStr (manage->removelist, dbidx) != NULL) {
      state = UIWIDGET_ENABLE;
    }
    uiWidgetSetState (manage->wcont [MANAGE_W_MENUITEM_UNDO_REMOVE], state);
  } else {
    logMsg (LOG_DBG, LOG_INFO, "new-selection: other set sel-dbidx");
    manage->seldbidx = dbidx;
  }

  if (manage->maincurrtab == MANAGE_TAB_MAIN_SL) {
    if (manage->sbssonglist) {
      logMsg (LOG_DBG, LOG_INFO, "new-selection: set last-tab song-sel");
      manage->lasttabsel = MANAGE_TAB_SL_SONGSEL;
    }
  }

  manageNewSelectionMoveCheck (manage, dbidx);

  logProcEnd ("");
  return UICB_CONT;
}

static bool
manageNewSelectionSonglist (void *udata, int32_t dbidx)
{
  manageui_t  *manage = udata;

  logProcBegin ();
  if (manage->selbypass) {
    logProcEnd ("sel-bypass");
    return UICB_CONT;
  }
  if (dbidx < 0) {
    logProcEnd ("bad-dbidx");
    return UICB_CONT;
  }

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: select within song list");
  logMsg (LOG_DBG, LOG_INFO, "new-selection: set sl-dbidx");
  manage->songlistdbidx = dbidx;
  logMsg (LOG_DBG, LOG_INFO, "new-selection: set last-tab sl");
  manage->lasttabsel = MANAGE_TAB_SONGLIST;

  manageNewSelectionMoveCheck (manage, dbidx);

  logProcEnd ("");
  return UICB_CONT;
}

static bool
manageSwitchToSongEditorSL (void *udata)
{
  manageui_t  *manage = udata;

  if (manage->maincurrtab == MANAGE_TAB_MAIN_SL) {
    manage->lasttabsel = manage->slcurrtab;
    if (uimusicqGetCount (manage->currmusicq) == 0) {
      logMsg (LOG_DBG, LOG_INFO, "sw-to-se: sl: set last-tab song-sel (no data)");
      manage->lasttabsel = MANAGE_TAB_SL_SONGSEL;
    }
  }
  if (manage->maincurrtab == MANAGE_TAB_MAIN_MM) {
    logMsg (LOG_DBG, LOG_INFO, "sw-to-se: mm: set last-tab song-sel");
    manage->lasttabsel = MANAGE_TAB_SL_SONGSEL;
  }
  return manageSwitchToSongEditor (manage);
}

static bool
manageSwitchToSongEditorSS (void *udata)
{
  manageui_t  *manage = udata;

  manage->lasttabsel = MANAGE_TAB_SL_SONGSEL;
  return manageSwitchToSongEditor (manage);
}

static bool
manageSwitchToSongEditor (manageui_t *manage)
{
  int         pagenum;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: switch to song editor");
  if (manage->maincurrtab != MANAGE_TAB_MAIN_MM) {
    pagenum = uivnbGetPage (manage->mainvnb, MANAGE_TAB_MAIN_MM);
    uivnbSetPage (manage->mainvnb, pagenum);
  }
  if (manage->mmcurrtab != MANAGE_TAB_SONGEDIT) {
    pagenum = uinbutilIDGetPage (manage->nbtabid [MANAGE_NB_MM], MANAGE_TAB_SONGEDIT);
    uiNotebookSetPage (manage->wcont [MANAGE_W_MM_NB], pagenum);
  }

  /* the notebook-switch-page callback will load the song editor data */

  logProcEnd ("");
  return UICB_CONT;
}

static bool
manageSongEditSaveCallback (void *udata, int32_t dbidx)
{
  manageui_t  *manage = udata;
  char        tmp [40];

  logProcBegin ();

  if (manage->dbchangecount > MANAGE_DB_COUNT_SAVE) {
    dbBackup ();
    manage->dbchangecount = 0;
  }

  /* this fetches the song from in-memory, which has already been updated */
  songdbWriteDB (manage->songdb, dbidx, manage->seforcesave);

  /* the database has been updated, tell the other processes to reload  */
  /* this particular entry */
  snprintf (tmp, sizeof (tmp), "%" PRId32 "%c%d", dbidx,
      MSG_ARGS_RS, MUSICDB_ENTRY_UNK);
  connSendMessage (manage->conn, ROUTE_STARTERUI, MSG_DB_ENTRY_UPDATE, tmp);

  manageRePopulateData (manage);

  /* the grouping must be re-built when a song is saved */
  groupingRebuild (manage->grouping, manage->musicdb);

  /* re-load the song into the song editor */
  /* it is unknown if called from saving a favorite or from the song editor */
  /* the overhead is minor */
  manage->songeditdbidx = dbidx;
  manageReloadSongData (manage);

  ++manage->dbchangecount;

  logProcEnd ("");
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
  hasorig = audiosrcOriginalExists (songGetStr (song, TAG_URI));
  if (hasorig) {
    manage->enablerestoreorig = true;
  } else {
    manage->enablerestoreorig = false;
  }
  if (manage->wcont [MANAGE_W_MENUITEM_RESTORE_ORIG] == NULL) {
    return;
  }
  if (hasorig) {
    uiWidgetSetState (manage->wcont [MANAGE_W_MENUITEM_RESTORE_ORIG], UIWIDGET_ENABLE);
  } else {
    uiWidgetSetState (manage->wcont [MANAGE_W_MENUITEM_RESTORE_ORIG], UIWIDGET_DISABLE);
  }
}

static bool
manageTrimSilence (void *udata)
{
  manageui_t    *manage = udata;
  song_t        *song;
  char          ffn [BDJ4_PATH_MAX];
  double        sstart;
  double        send;
  int32_t       startval;
  int32_t       endval;
  int           rc;

  if (manage->songeditdbidx < 0) {
    return UICB_STOP;
  }

  song = dbGetByIdx (manage->musicdb, manage->songeditdbidx);
  audiosrcFullPath (songGetStr (song, TAG_URI), ffn, sizeof (ffn), NULL, 0);
  rc = aaSilenceDetect (ffn, &sstart, &send);
  if (rc == 0) {
    startval = (int32_t) (sstart * 1000.0);
    endval = (int32_t) (send * 1000.0);
    uisongeditSetSongStartEnd (manage->mmsongedit, startval, endval);
  }

  return UICB_CONT;
}

static bool
manageWriteAudioTags (void *udata)
{
  manageui_t    *manage = udata;
  song_t        *song;
  int           rc;
  int           origwritetags;

  if (manage->songeditdbidx < 0) {
    return UICB_STOP;
  }

  song = dbGetByIdx (manage->musicdb, manage->songeditdbidx);
  /* force a save, there may be no actual changes to the song */
  manage->seforcesave = true;
  songSetChanged (song);
  /* make sure write-tags is on */
  origwritetags = bdjoptGetNum (OPT_G_WRITETAGS);
  bdjoptSetNum (OPT_G_WRITETAGS, WRITE_TAGS_ALL);
  rc = manageSongEditSaveCallback (manage, manage->songeditdbidx);
  /* restore write-tags to the original */
  bdjoptSetNum (OPT_G_WRITETAGS, origwritetags);
  return rc;
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
  hasorig = audiosrcOriginalExists (songGetStr (song, TAG_URI));
  rc = uiaaDialog (manage->uiaa, songGetNum (song, TAG_ADJUSTFLAGS), hasorig);
  return rc;
}

static bool
manageApplyAdjCallback (void *udata, int32_t aaflags)
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
  manageReloadSongData (manage);
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
  manageReloadSongData (manage);
  uisongeditClearChanged (manage->mmsongedit, UISONGEDIT_ALL);
  uisongeditEditAllSetFields (manage->mmsongedit, UISONGEDIT_EDITALL_OFF);
  manage->ineditall = false;

  return UICB_CONT;
}

static void
manageReloadSongData (manageui_t *manage)
{
  song_t    *song;

  logProcBegin ();
  song = dbGetByIdx (manage->musicdb, manage->songeditdbidx);
  if (song == NULL) {
    return;
  }
  manageSetBPMCounter (manage, song);
  if (manage->mmcurrtab == MANAGE_TAB_SONGEDIT) {
    uisongeditLoadData (manage->mmsongedit, song,
        manage->songeditdbidx, UISONGEDIT_ALL);
  }
  if (manage->mmcurrtab == MANAGE_TAB_AUDIOID) {
    manageAudioIdLoad (manage->manageaudioid, song, manage->songeditdbidx);
  }
  manageSetEditMenuItems (manage);
  logProcEnd ("");
}

static void
manageNewSelectionMoveCheck (manageui_t *manage, dbidx_t dbidx)
{
  /* next/previous processing */
  if (manage->maincurrtab == MANAGE_TAB_MAIN_MM &&
      (manage->mmcurrtab == MANAGE_TAB_SONGEDIT ||
      manage->mmcurrtab == MANAGE_TAB_AUDIOID)) {
    manage->songeditdbidx = dbidx;
    manageReloadSongData (manage);
  }
}

/* the song edit dbidx is set based on which tab the user came from */
static void
manageSetSongEditDBIdx (manageui_t *manage, int mainlasttabsel, int mmlasttabsel)
{
  logProcBegin ();
  if (mainlasttabsel == MANAGE_TAB_MAIN_SL &&
      manage->slcurrtab == MANAGE_TAB_SONGLIST) {
    manage->songeditdbidx = manage->songlistdbidx;
    if (manage->lasttabsel == MANAGE_TAB_SL_SONGSEL) {
      manage->songeditdbidx = manage->seldbidx;
    }
  }
  if (mainlasttabsel == MANAGE_TAB_MAIN_SL &&
      manage->slcurrtab == MANAGE_TAB_SL_SONGSEL) {
    manage->songeditdbidx = manage->seldbidx;
  }
  if (mainlasttabsel == MANAGE_TAB_MAIN_MM &&
      mmlasttabsel == MANAGE_TAB_MM) {
    if (uisfPlaylistInUse (manage->uisongfilter)) {
      manage->songeditdbidx = manage->songlistdbidx;
    } else {
      manage->songeditdbidx = manage->seldbidx;
    }
  }
  logProcEnd ("");
}

/* itunes */

static bool
managePlaylistImportiTunes (void *udata)
{
  manageui_t  *manage = udata;
  char        tbuff [BDJ4_PATH_MAX];

  if (manage->importitunesactive) {
    return UICB_STOP;
  }

  manage->importitunesactive = true;
  uiLabelSetText (manage->minfo.errorMsg, "");
  uiLabelSetText (manage->minfo.statusMsg, "");
  if (! itunesConfigured ()) {
    /* CONTEXT: manage ui: status message: itunes is not configured */
    snprintf (tbuff, sizeof (tbuff), _("%s is not configured."), ITUNES_NAME);
    uiLabelSetText (manage->minfo.errorMsg, tbuff);
    return UICB_CONT;
  }

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: import itunes");
  manage->impitunesstate = BDJ4_STATE_START;

  logProcEnd ("");
  return UICB_CONT;
}

static void
manageiTunesCreateDialog (manageui_t *manage)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *uiwidgetp;
  char        tbuff [50];

  logProcBegin ();
  if (manage->wcont [MANAGE_W_ITUNES_SEL_DIALOG] != NULL) {
    logProcEnd ("already");
    return;
  }

  /* CONTEXT: import from itunes: title for the dialog */
  snprintf (tbuff, sizeof (tbuff), _("Import from %s"), ITUNES_NAME);
  manage->wcont [MANAGE_W_ITUNES_SEL_DIALOG] = uiCreateDialog (manage->minfo.window,
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

  vbox = uiCreateVertBox (NULL);
  uiDialogPackInDialog (manage->wcont [MANAGE_W_ITUNES_SEL_DIALOG], vbox);
  uiWidgetSetAllMargins (vbox, 4);

  hbox = uiCreateHorizBox (NULL);
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: import from itunes: select the itunes playlist to use (iTunes Playlist) */
  snprintf (tbuff, sizeof (tbuff), _("%s Playlist"), ITUNES_NAME);
  uiwidgetp = uiCreateColonLabel (tbuff);
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  /* returns the button */
  manageiTunesDialogCreateList (manage);
  manage->itunesdd = uiddCreate ("mui-itunes-sel",
      manage->wcont [MANAGE_W_ITUNES_SEL_DIALOG], hbox, DD_PACK_START,
      manage->itunesddlist, DD_LIST_TYPE_STR,
      "", DD_REPLACE_TITLE, NULL);

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox (NULL);
  uiBoxPackStart (vbox, hbox);

  uiwcontFree (vbox);
  uiwcontFree (hbox);

  logProcEnd ("");
}

static void
manageiTunesDialogCreateList (manageui_t *manage)
{
  const char  *skey;
  ilist_t     *pllist;
  ilistidx_t  idx;

  logProcBegin ();

  pllist = ilistAlloc ("itunes-pl-sel", LIST_ORDERED);

  idx = 0;
  itunesStartIteratePlaylists (manage->itunes);
  while ((skey = itunesIteratePlaylists (manage->itunes)) != NULL) {
    ilistSetStr (pllist, idx, DD_LIST_DISP, skey);
    ilistSetStr (pllist, idx, DD_LIST_KEY_STR, skey);
    ++idx;
  }
  manage->itunesddlist = pllist;
  logProcEnd ("");
}

static bool
manageiTunesDialogResponseHandler (void *udata, int32_t responseid)
{
  manageui_t  *manage = udata;

  logProcBegin ();

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
      char        tbuff [MAX_PL_NM_LEN];

      logMsg (LOG_DBG, LOG_ACTIONS, "= action: itunes: import");
      plname = uiddGetSelectionStr (manage->itunesdd);
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
          snprintf (tbuff, sizeof (tbuff), "%d%c%d%c%" PRId32,
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

  logProcEnd ("");
  return UICB_CONT;
}

/* music manager */

void
manageBuildUIMusicManager (manageui_t *manage)
{
  uiwcont_t  *uip;
  uiwcont_t  *uiwidgetp;
  uiwcont_t  *vbox;

  logProcBegin ();
  /* music manager */
  vbox = uiCreateVertBox (NULL);
  uivnbAppendPage (manage->mainvnb, vbox,
      /* CONTEXT: manage-ui: name of music manager notebook tab */
      _("Music Manager"), MANAGE_TAB_MAIN_MM);
  uiWidgetSetAllMargins (vbox, 2);

  /* music manager: player */
  uiwidgetp = uiplayerBuildUI (manage->mmplayer);
  uiBoxPackStart (vbox, uiwidgetp);

  manage->wcont [MANAGE_W_MM_NB] = uiCreateNotebook ("nb-manage");
  uiBoxPackStartExpand (vbox, manage->wcont [MANAGE_W_MM_NB]);

  /* music manager: song selection tab*/
  uip = uisongselBuildUI (manage->mmsongsel, manage->minfo.window);
  uiWidgetExpandHoriz (uiwidgetp);
  uiNotebookAppendPage (manage->wcont [MANAGE_W_MM_NB], uip,
      /* CONTEXT: manage-ui: name of song selection notebook tab */
      _("Music Manager"), NULL);
  uinbutilIDAdd (manage->nbtabid [MANAGE_NB_MM], MANAGE_TAB_MM);

  /* music manager: song editor tab */

  manage->uict = uicopytagsInit (manage->minfo.window, manage->minfo.options);
  manage->uiaa = uiaaInit (manage->minfo.window, manage->minfo.options);
  manage->callbacks [MANAGE_CB_APPLY_ADJ] = callbackInitI (
      manageApplyAdjCallback, manage);
  uiaaSetResponseCallback (manage->uiaa, manage->callbacks [MANAGE_CB_APPLY_ADJ]);

  uip = uisongeditBuildUI (manage->mmsongsel, manage->mmsongedit, manage->minfo.window, manage->minfo.errorMsg);
  uiNotebookAppendPage (manage->wcont [MANAGE_W_MM_NB], uip,
      /* CONTEXT: manage-ui: name of song editor notebook tab */
      _("Song Editor"), NULL);
  uinbutilIDAdd (manage->nbtabid [MANAGE_NB_MM], MANAGE_TAB_SONGEDIT);

  /* music manager: audio identification tab */

  manage->manageaudioid = manageAudioIdAlloc (&manage->minfo);
  manageAudioIdSetSaveCallback (manage->manageaudioid, manage->callbacks [MANAGE_CB_SAVE]);

  uip = manageAudioIdBuildUI (manage->manageaudioid, manage->mmsongsel);
  uiNotebookAppendPage (manage->wcont [MANAGE_W_MM_NB], uip,
      /* CONTEXT: manage-ui: name of audio identification notebook tab */
      _("Audio ID"), NULL);
  uinbutilIDAdd (manage->nbtabid [MANAGE_NB_MM], MANAGE_TAB_AUDIOID);

  manage->callbacks [MANAGE_CB_MM_NB] = callbackInitI (
      manageSwitchPageMM, manage);
  uiNotebookSetCallback (manage->wcont [MANAGE_W_MM_NB], manage->callbacks [MANAGE_CB_MM_NB]);

  uiwcontFree (vbox);

  logProcEnd ("");
}

void
manageMusicManagerMenu (manageui_t *manage)
{
  uiwcont_t  *menu = NULL;
  uiwcont_t  *menuitem = NULL;

  logProcBegin ();
  if (uiMenuIsInitialized (manage->wcont [MANAGE_W_MENU_MM])) {
    uiMenuDisplay (manage->wcont [MANAGE_W_MENU_MM]);
    manage->currmenu = manage->wcont [MANAGE_W_MENU_MM];
    return;
  }

  menuitem = uiMenuAddMainItem (manage->wcont [MANAGE_W_MENUBAR],
      /* CONTEXT: manage-ui: menu selection: actions for music manager */
      manage->wcont [MANAGE_W_MENU_MM], _("Actions"));
  menu = uiCreateSubMenu (menuitem);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_MM_SET_MARK,
      manageSameSongSetMark);
  /* CONTEXT: manage-ui: menu selection: music manager: set same-song mark */
  menuitem = uiMenuCreateItem (menu, _("Mark as Same Song"),
      manage->callbacks [MANAGE_MENU_CB_MM_SET_MARK]);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_MM_CLEAR_MARK,
      manageSameSongClearMark);
  /* CONTEXT: manage-ui: menu selection: music manager: clear same-song mark */
  menuitem = uiMenuCreateItem (menu, _("Clear Same Song Mark"),
      manage->callbacks [MANAGE_MENU_CB_MM_CLEAR_MARK]);
  uiwcontFree (menuitem);

  uiMenuAddSeparator (menu);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_MM_REMOVE_SONG,
      manageMarkSongRemoved);
  /* CONTEXT: manage-ui: menu selection: music manager: remove song */
  menuitem = uiMenuCreateItem (menu, _("Remove Song"),
      manage->callbacks [MANAGE_MENU_CB_MM_REMOVE_SONG]);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_MM_UNDO_REMOVE,
      manageUndoRemove);
  /* CONTEXT: manage-ui: menu selection: music manager: undo song removal */
  menuitem = uiMenuCreateItem (menu, _("Undo Song Removal"),
      manage->callbacks [MANAGE_MENU_CB_MM_UNDO_REMOVE]);
  uiWidgetSetState (menuitem, UIWIDGET_DISABLE);
  manage->wcont [MANAGE_W_MENUITEM_UNDO_REMOVE] = menuitem;

  manageSetMenuCallback (manage, MANAGE_MENU_CB_MM_UNDO_ALL_REMOVE,
      manageUndoRemoveAll);
  /* CONTEXT: manage-ui: menu selection: music manager: undo all song removals */
  menuitem = uiMenuCreateItem (menu, _("Undo All Song Removals"),
      manage->callbacks [MANAGE_MENU_CB_MM_UNDO_ALL_REMOVE]);
  uiWidgetSetState (menuitem, UIWIDGET_DISABLE);
  manage->wcont [MANAGE_W_MENUITEM_UNDO_ALL_REMOVE] = menuitem;

  uiMenuSetInitialized (manage->wcont [MANAGE_W_MENU_MM]);
  uiwcontFree (menu);

  uiMenuDisplay (manage->wcont [MANAGE_W_MENU_MM]);
  manage->currmenu = manage->wcont [MANAGE_W_MENU_MM];

  logProcEnd ("");
}

/* song list */

static void
manageSonglistMenu (manageui_t *manage)
{
  char        tbuff [MAX_PL_NM_LEN];
  uiwcont_t   *menu;
  uiwcont_t   *menuitem;

  logProcBegin ();
  if (uiMenuIsInitialized (manage->wcont [MANAGE_W_MENU_SL])) {
    uiMenuDisplay (manage->wcont [MANAGE_W_MENU_SL]);
    manage->currmenu = manage->wcont [MANAGE_W_MENU_SL];
    logProcEnd ("already");
    return;
  }

  /* edit */
  menuitem = uiMenuAddMainItem (manage->wcont [MANAGE_W_MENUBAR],
      /* CONTEXT: manage-ui: menu selection: song list: edit menu */
      manage->wcont [MANAGE_W_MENU_SL], _("Edit"));
  menu = uiCreateSubMenu (menuitem);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_LOAD, manageSonglistLoad);
  /* CONTEXT: manage-ui: menu selection: song list: edit menu: load */
  menuitem = uiMenuCreateItem (menu, _("Load"),
      manage->callbacks [MANAGE_MENU_CB_SL_LOAD]);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_NEW, manageSonglistNew);
  /* CONTEXT: manage-ui: menu selection: song list: edit menu: start new song list */
  menuitem = uiMenuCreateItem (menu, _("Start New Song List"),
      manage->callbacks [MANAGE_MENU_CB_SL_NEW]);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_COPY, manageSonglistCopy);
  /* CONTEXT: manage-ui: menu selection: song list: edit menu: create copy */
  menuitem = uiMenuCreateItem (menu, _("Create Copy"),
      manage->callbacks [MANAGE_MENU_CB_SL_COPY]);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_DELETE, manageSonglistDelete);
  /* CONTEXT: manage-ui: menu selection: song list: edit menu: delete song list */
  menuitem = uiMenuCreateItem (menu, _("Delete"),
      manage->callbacks [MANAGE_MENU_CB_SL_DELETE]);
  uiwcontFree (menuitem);

  /* actions */
  uiwcontFree (menu);
  menuitem = uiMenuAddMainItem (manage->wcont [MANAGE_W_MENUBAR],
      /* CONTEXT: manage-ui: menu selection: actions for song list */
      manage->wcont [MANAGE_W_MENU_SL], _("Actions"));
  menu = uiCreateSubMenu (menuitem);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_MIX, manageSonglistMix);
  /* CONTEXT: manage-ui: menu selection: song list: actions menu: rearrange the songs and create a new mix */
  menuitem = uiMenuCreateItem (menu, _("Mix"),
      manage->callbacks [MANAGE_MENU_CB_SL_MIX]);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_SWAP, manageSonglistSwap);
  /* CONTEXT: manage-ui: menu selection: song list: actions menu: swap two songs */
  menuitem = uiMenuCreateItem (menu, _("Swap"),
      manage->callbacks [MANAGE_MENU_CB_SL_SWAP]);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_TRUNCATE,
      manageSonglistTruncate);
  /* CONTEXT: manage-ui: menu selection: song list: actions menu: truncate the song list */
  menuitem = uiMenuCreateItem (menu, _("Truncate"),
      manage->callbacks [MANAGE_MENU_CB_SL_TRUNCATE]);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_MK_FROM_PL,
      manageSonglistCreateFromPlaylist);
  /* CONTEXT: manage-ui: menu selection: song list: actions menu: create a song list from a playlist */
  menuitem = uiMenuCreateItem (menu, _("Create from Playlist"),
      manage->callbacks [MANAGE_MENU_CB_SL_MK_FROM_PL]);
  uiwcontFree (menuitem);

  /* export */
  uiwcontFree (menu);
  menuitem = uiMenuAddMainItem (manage->wcont [MANAGE_W_MENUBAR],
      /* CONTEXT: manage-ui: menu selection: export actions for song list */
      manage->wcont [MANAGE_W_MENU_SL], _("Export"));
  menu = uiCreateSubMenu (menuitem);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_EXPORT,
      managePlaylistExport);
  /* CONTEXT: manage-ui: menu selection: song list: export */
  menuitem = uiMenuCreateItem (menu, _("Export Playlist"),
      manage->callbacks [MANAGE_MENU_CB_SL_EXPORT]);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_BDJ4_EXP,
      managePlaylistExportBDJ4);
  /* CONTEXT: manage-ui: menu selection: song list: export: export for ballroomdj */
  snprintf (tbuff, sizeof (tbuff), _("Export for %s"), BDJ4_NAME);
  menuitem = uiMenuCreateItem (menu, tbuff,
      manage->callbacks [MANAGE_MENU_CB_SL_BDJ4_EXP]);
  uiwcontFree (menuitem);

  /* import */
  uiwcontFree (menu);
  menuitem = uiMenuAddMainItem (manage->wcont [MANAGE_W_MENUBAR],
      /* CONTEXT: manage-ui: menu selection: import actions for song list */
      manage->wcont [MANAGE_W_MENU_SL], _("Import"));
  menu = uiCreateSubMenu (menuitem);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_IMPORT,
      managePlaylistImport);
  /* CONTEXT: manage-ui: menu selection: song list: import */
  menuitem = uiMenuCreateItem (menu, _("Import Playlist"),
      manage->callbacks [MANAGE_MENU_CB_SL_IMPORT]);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_BDJ4_IMP,
      managePlaylistImportBDJ4);
  /* CONTEXT: manage-ui: menu selection: song list: import: import from ballroomdj */
  snprintf (tbuff, sizeof (tbuff), _("Import from %s"), BDJ4_NAME);
  menuitem = uiMenuCreateItem (menu, tbuff,
      manage->callbacks [MANAGE_MENU_CB_SL_BDJ4_IMP]);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_ITUNES_IMP,
      managePlaylistImportiTunes);
  /* CONTEXT: manage-ui: menu selection: song list: import: import from itunes */
  snprintf (tbuff, sizeof (tbuff), _("Import from %s"), ITUNES_NAME);
  menuitem = uiMenuCreateItem (menu, tbuff,
      manage->callbacks [MANAGE_MENU_CB_SL_ITUNES_IMP]);
  uiwcontFree (menuitem);

  /* options */
  uiwcontFree (menu);
  menuitem = uiMenuAddMainItem (manage->wcont [MANAGE_W_MENUBAR],
      /* CONTEXT: manage-ui: menu selection: song list: options menu */
      manage->wcont [MANAGE_W_MENU_SL], _("Options"));
  menu = uiCreateSubMenu (menuitem);
  uiwcontFree (menuitem);

  manageSetMenuCallback (manage, MANAGE_MENU_CB_SL_SBS_EDIT,
      manageToggleSBSSonglist);
  /* CONTEXT: manage-ui: menu checkbox: side-by-side view (suggestion: combined view) */
  menuitem = uiMenuCreateCheckbox (menu, _("Side-by-Side View"),
      manage->sbssonglist, manage->callbacks [MANAGE_MENU_CB_SL_SBS_EDIT]);
  uiwcontFree (menuitem);

  uiMenuSetInitialized (manage->wcont [MANAGE_W_MENU_SL]);

  uiMenuDisplay (manage->wcont [MANAGE_W_MENU_SL]);
  manage->currmenu = manage->wcont [MANAGE_W_MENU_SL];

  uiwcontFree (menu);

  logProcEnd ("");
}

static bool
manageSonglistLoad (void *udata)
{
  manageui_t  *manage = udata;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: load songlist");
  uiLabelSetText (manage->minfo.statusMsg, "");
  manageSonglistSave (manage);
  selectFileDialog (SELFILE_SONGLIST, manage->minfo.window,
      manage->minfo.options, manage->callbacks [MANAGE_CB_SL_SEL_FILE]);
  logProcEnd ("");
  return UICB_CONT;
}

static int32_t
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
  char        oname [MAX_PL_NM_LEN];
  char        newname [MAX_PL_NM_LEN];

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: copy songlist");
  uiLabelSetText (manage->minfo.statusMsg, "");
  manageSonglistSave (manage);

  uimusicqGetSonglistName (manage->currmusicq, oname, sizeof (oname));

  /* CONTEXT: manage-ui: the new name after 'create copy' (e.g. "Copy of DJ-2022-04") */
  snprintf (newname, sizeof (newname), _("Copy of %s"), oname);
  if (manageCreatePlaylistCopy (manage->minfo.errorMsg, oname, newname)) {
    manageSetSonglistName (manage, newname);
    managePlaylistLoadFile (manage->managepl, newname, MANAGE_PRELOAD);
    manage->slbackupcreated = false;
  }
  logProcEnd ("");
  return UICB_CONT;
}

static bool
manageSonglistNew (void *udata)
{
  manageui_t  *manage = udata;

  uiLabelSetText (manage->minfo.statusMsg, "");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: new songlist");

  manageResetCreateNew (manage, PLTYPE_SONGLIST);
  return UICB_CONT;
}

static void
manageResetCreateNew (manageui_t *manage, pltype_t pltype)
{
  manageSonglistSave (manage);

  /* CONTEXT: manage-ui: song list: default name for a new song list */
  manageSetSonglistName (manage, _("New Song List"));
  manage->pltype = pltype;
  manage->slbackupcreated = false;
  uimusicqSetSelectionFirst (manage->currmusicq, manage->musicqManageIdx);
  uimusicqTruncateQueueCallback (manage->currmusicq);
  managePlaylistNew (manage->managepl, MANAGE_PRELOAD, pltype);
  /* the music manager must be reset to use the song selection as */
  /* there are no songs selected */
  manageNewSelectionSongSel (manage, manage->seldbidx);
}

static bool
manageSonglistDelete (void *udata)
{
  manageui_t  *manage = udata;
  char        oname [MAX_PL_NM_LEN];

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: new songlist");
  uimusicqGetSonglistName (manage->currmusicq, oname, sizeof (oname));

  manageDeletePlaylist (manage->musicdb, oname);
  /* no save */
  *manage->sloldname = '\0';
  manageResetCreateNew (manage, PLTYPE_SONGLIST);
  manageDeleteStatus (manage->minfo.statusMsg, oname);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
manageSonglistTruncate (void *udata)
{
  manageui_t  *manage = udata;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: truncate songlist");
  uimusicqTruncateQueueCallback (manage->currmusicq);
  logProcEnd ("");
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
  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: create from playlist");
  manageSonglistSave (manage);

  /* if there are no songs, preserve the song list name, as */
  /* the user may have typed in a new name before running create-from-pl */
  *manage->slpriorname = '\0';
  if (uimusicqGetCount (manage->currmusicq) <= 0) {
    uimusicqGetSonglistName (manage->currmusicq, manage->slpriorname, sizeof (manage->slpriorname));
  }
  manageSongListCFPLCreateDialog (manage);

  uiDialogShow (manage->wcont [MANAGE_W_CFPL_DIALOG]);

  x = nlistGetNum (manage->minfo.options, MANAGE_CFPL_POSITION_X);
  y = nlistGetNum (manage->minfo.options, MANAGE_CFPL_POSITION_Y);
  uiWindowMove (manage->wcont [MANAGE_W_CFPL_DIALOG], x, y, -1);

  logProcEnd ("");
  return UICB_CONT;
}

static void
manageSongListCFPLCreateDialog (manageui_t *manage)
{
  uiwcont_t  *vbox;
  uiwcont_t  *hbox;
  uiwcont_t  *uiwidgetp;
  uiwcont_t  *szgrp;  // labels

  logProcBegin ();
  if (manage->wcont [MANAGE_W_CFPL_DIALOG] != NULL) {
    logProcEnd ("already");
    return;
  }

  szgrp = uiCreateSizeGroupHoriz ();

  manage->wcont [MANAGE_W_CFPL_DIALOG] = uiCreateDialog (manage->minfo.window,
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

  vbox = uiCreateVertBox (NULL);
  uiDialogPackInDialog (manage->wcont [MANAGE_W_CFPL_DIALOG], vbox);
  uiWidgetSetAllMargins (vbox, 4);

  hbox = uiCreateHorizBox (NULL);
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: create from playlist: select the playlist to use */
  uiwidgetp = uiCreateColonLabel (_("Playlist"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  manage->cfpl = uiplaylistCreate (manage->wcont [MANAGE_W_CFPL_DIALOG],
      hbox, PL_LIST_AUTO_SEQ, NULL, UIPL_PACK_START, UIPL_FLAG_NONE);

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox (NULL);
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: create from playlist: set the maximum time for the song list */
  uiwidgetp = uiCreateColonLabel (_("Time Limit"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  /* FIX: no validation! */
  uiwidgetp = uiSpinboxTimeCreate (SB_TIME_BASIC, manage, "", NULL);
  uiSpinboxTimeSetValue (uiwidgetp, 3 * 60 * 1000);
  uiSpinboxSetRange (uiwidgetp, 0.0, 600000.0);
  uiBoxPackStart (hbox, uiwidgetp);
  manage->wcont [MANAGE_W_CFPL_TM_LIMIT] = uiwidgetp;

  uiwcontFree (vbox);
  uiwcontFree (hbox);
  uiwcontFree (szgrp);

  logProcEnd ("");
}

static bool
manageCFPLResponseHandler (void *udata, int32_t responseid)
{
  manageui_t  *manage = udata;
  int         x, y, ws;

  logProcBegin ();
  uiWindowGetPosition (manage->wcont [MANAGE_W_CFPL_DIALOG], &x, &y, &ws);
  nlistSetNum (manage->minfo.options, MANAGE_CFPL_POSITION_X, x);
  nlistSetNum (manage->minfo.options, MANAGE_CFPL_POSITION_Y, y);

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

  logProcEnd ("");
  return UICB_CONT;
}

static void
manageCFPLCreate (manageui_t *manage)
{
  const char  *fn;
  int32_t     stoptime;
  char        tbuff [40];

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: cfpl: create");

  manage->musicqupdated = false;

  fn = uiplaylistGetKey (manage->cfpl);

  if (fn == NULL || ! *fn) {
    manage->cfplactive = false;
    return;
  }

  dataFree (manage->cfplfn);
  manage->cfplfn = mdstrdup (fn);
  stoptime = uiSpinboxTimeGetValue (manage->wcont [MANAGE_W_CFPL_TM_LIMIT]);
  /* convert from mm:ss to hh:mm */
  stoptime *= 60;
  /* adjust : add in the current hh:mm */
  stoptime += mstime () - mstimestartofday ();

  snprintf (tbuff, sizeof (tbuff), "%d", manage->musicqManageIdx);
  connSendMessage (manage->conn, ROUTE_MAIN, MSG_QUEUE_CLEAR, tbuff);

  /* overriding the stop time will set the stop time for the next */
  /* playlist that is loaded */
  snprintf (tbuff, sizeof (tbuff), "%" PRId32, stoptime);
  connSendMessage (manage->conn, ROUTE_MAIN,
      MSG_PL_OVERRIDE_STOP_TIME, tbuff);

  /* the edit mode must be false to allow the stop time to be applied */
  manage->editmode = EDIT_FALSE;
  /* re-use songlist-load-file to load the auto/seq playlist */
  manageSonglistLoadFile (manage, fn, MANAGE_CREATE);

  /* make sure no save happens with the playlist being used */
  *manage->sloldname = '\0';
  manage->editmode = EDIT_TRUE;
  if (*manage->slpriorname) {
    manageSetSonglistName (manage, manage->slpriorname);
  } else {
    /* CONTEXT: manage-ui: song list: default name for a new song list */
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
  char        tnm [MAX_PL_NM_LEN];

  uimusicqGetSonglistName (manage->currmusicq, tnm, sizeof (tnm));
  manageSonglistSave (manage);

  /* copy the settings from the base playlist to the new song list */
  pl = playlistLoad (tnm, NULL, NULL);
  autopl = playlistLoad (manage->cfplfn, NULL, NULL);

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

  manage->cfplpostprocess = false;
}

static bool
manageSonglistMix (void *udata)
{
  manageui_t  *manage = udata;
  char        tbuff [40];

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: mix songlist");
  uiLabelSetText (manage->minfo.statusMsg, manage->minfo.pleasewaitmsg);
  snprintf (tbuff, sizeof (tbuff), "%d", manage->musicqManageIdx);
  connSendMessage (manage->conn, ROUTE_MAIN, MSG_QUEUE_MIX, tbuff);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
manageSonglistSwap (void *udata)
{
  manageui_t  *manage = udata;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: songlist swap");
  uimusicqSwap (manage->currmusicq, manage->musicqManageIdx);
  logProcEnd ("");
  return UICB_CONT;
}

static void
manageSonglistLoadFile (void *udata, const char *fn, int preloadflag)
{
  manageui_t  *manage = udata;
  char        tbuff [MAX_PL_NM_LEN];

  logProcBegin ();
  if (manage->inload) {
    logProcEnd ("in-load");
    return;
  }

  if (preloadflag == MANAGE_CREATE) {
    if (! playlistExists (fn)) {
      logProcEnd ("no-playlist");
      return;
    }
  }
  if (preloadflag == MANAGE_STD || preloadflag == MANAGE_PRELOAD) {
    if (! songlistExists (fn)) {
      logProcEnd ("no-songlist");
      return;
    }
  }

  manage->inload = true;

  /* create-from-playlist already did a save */
  if (preloadflag == MANAGE_STD) {
    manageSonglistSave (manage);
  }

  /* ask the main player process to not send music queue updates */
  /* the selbypass flag cannot be used due to timing issues */
  snprintf (tbuff, sizeof (tbuff), "%d", manage->musicqManageIdx);
  connSendMessage (manage->conn, ROUTE_MAIN, MSG_MUSICQ_DATA_SUSPEND, tbuff);

  /* truncate from the first selection */
  uimusicqSetSelectionFirst (manage->currmusicq, manage->musicqManageIdx);
  uimusicqTruncateQueueCallback (manage->currmusicq);

  msgbuildQueuePlaylist (tbuff, sizeof (tbuff),
      manage->musicqManageIdx, fn, manage->editmode);
  connSendMessage (manage->conn, ROUTE_MAIN, MSG_QUEUE_PLAYLIST, tbuff);

  snprintf (tbuff, sizeof (tbuff), "%d", manage->musicqManageIdx);
  connSendMessage (manage->conn, ROUTE_MAIN, MSG_MUSICQ_DATA_RESUME, tbuff);

  stpecpy (tbuff, tbuff + sizeof (tbuff), fn);
  /* CONTEXT: playlist: the name of the history song list */
  if (strcmp (fn, _("History")) == 0 ||
      strcmp (fn, "History") == 0) {
    /* CONTEXT: playlist: copy of the history song list */
    snprintf (tbuff, sizeof (tbuff), _("Copy of %s"), fn);
  }

  manageSetSonglistName (manage, tbuff);
  if (preloadflag != MANAGE_PRELOAD) {
    managePlaylistLoadFile (manage->managepl, fn, MANAGE_PRELOAD);
  }
  manage->slbackupcreated = false;
  manage->inload = false;
  logProcEnd ("");
}

/* callback to load playlist upon songlist/sequence load */
static int32_t
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
  managePlaylistNew (manage->managepl, MANAGE_PRELOAD, PLTYPE_AUTO);
  return UICB_CONT;
}

/* callback to load upon playlist load */
static int32_t
manageLoadSonglistCB (void *udata, const char *fn)
{
  manageui_t    *manage = udata;

  logProcBegin ();
  /* the load will save any current song list */
  manageSonglistLoadFile (manage, fn, MANAGE_PRELOAD);
  manageSequenceLoadFile (manage->manageseq, fn, MANAGE_PRELOAD);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
manageToggleSBSSonglist (void *udata)
{
  manageui_t  *manage = udata;
  uimusicq_t  *ouimusicq = NULL;
  uimusicq_t  *uimusicq = NULL;
  int         val;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: toggle side-by-side songlist");

  ouimusicq = manageGetCurrMusicQ (manage);

  val = nlistGetNum (manage->minfo.options, MANAGE_SBS_SONGLIST);
  val = ! val;
  nlistSetNum (manage->minfo.options, MANAGE_SBS_SONGLIST, val);
  manage->sbssonglist = val;
  if (manage->sbssonglist) {
    manage->currmusicq = manage->slsbsmusicq;
  } else {
    manage->currmusicq = manage->slmusicq;
  }

  uimusicq = manageGetCurrMusicQ (manage);

  if (manage->musicqManageIdx == MUSICQ_SL) {
    char    name [MAX_PL_NM_LEN];

    uimusicqCopySelectList (ouimusicq, uimusicq);
    uimusicqGetSonglistName (ouimusicq, name, sizeof (name));
    manageSetSonglistName (manage, name);
  }

  manageSetSBSSonglist (manage);
  logProcEnd ("");
  return UICB_CONT;
}

/* sets up the tabs to display the appropriate musicq */
/* runs the load-musicq process to get the display correct */
static void
manageSetSBSSonglist (manageui_t *manage)
{
  logProcBegin ();

  if (manage->sbssonglist) {
    uiWidgetShow (manage->wcont [MANAGE_W_SL_SBS_MUSICQ_TAB]);
    uiWidgetHide (manage->wcont [MANAGE_W_SL_MUSICQ_TAB]);
    uiWidgetHide (manage->wcont [MANAGE_W_SONGSEL_TAB]);
    uiNotebookSetPage (manage->wcont [MANAGE_W_SONGLIST_NB], 0);
  } else {
    uiWidgetHide (manage->wcont [MANAGE_W_SL_SBS_MUSICQ_TAB]);
    uiWidgetShow (manage->wcont [MANAGE_W_SL_MUSICQ_TAB]);
    uiWidgetShow (manage->wcont [MANAGE_W_SONGSEL_TAB]);
    uiNotebookSetPage (manage->wcont [MANAGE_W_SONGLIST_NB], 1);
  }

  manageLoadMusicQueue (manage, manage->musicqManageIdx);
  logProcEnd ("");
}

static void
manageSetSonglistName (manageui_t *manage, const char *nm)
{
  stpecpy (manage->sloldname, manage->sloldname + sizeof (manage->sloldname), nm);
  uimusicqSetSonglistName (manage->currmusicq, nm);
  logMsg (LOG_DBG, LOG_INFO, "song list name set: %s", nm);
}

static void
manageSonglistSave (manageui_t *manage)
{
  char        name [MAX_PL_NM_LEN];
  char        nnm [BDJ4_PATH_MAX];
  bool        notvalid;

  logProcBegin ();
  if (manage == NULL) {
    logProcEnd ("null-manage");
    return;
  }
  if (! *manage->sloldname) {
    logProcEnd ("no-sl-old-name");
    return;
  }

  if (manage->currmusicq == NULL) {
    logProcEnd ("no-musicq");
    return;
  }

  if (uimusicqGetCount (manage->currmusicq) <= 0) {
    logProcEnd ("count <= 0");
    return;
  }

  uimusicqGetSonglistName (manage->currmusicq, name, sizeof (name));
  notvalid = false;
  if (uimusicqSonglistNameIsNotValid (manage->currmusicq)) {
    stpecpy (name, name + sizeof (name), manage->sloldname);
    uimusicqSetSonglistName (manage->currmusicq, manage->sloldname);
    notvalid = true;
  }

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
  uimusicqSave (manage->musicdb, manage->musicqupdate [MUSICQ_SL], name);
  playlistCheckAndCreate (name, manage->pltype);
  managePlaylistLoadFile (manage->managepl, name, MANAGE_PRELOAD);

  if (notvalid) {
    /* set the message after the entry field has been reset */
    /* CONTEXT: Saving Song List: Error message for invalid song list name. */
    uiLabelSetText (manage->minfo.errorMsg, _("Invalid name. Using old name."));
  }

  logProcEnd ("");
}

static bool
managePlayProcessSonglist (void *udata, int32_t dbidx, int32_t mqidx)
{
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: play from songlist");
  manageQueueProcess (udata, dbidx, mqidx, DISP_SEL_SONGLIST, MANAGE_PLAY);
  return UICB_CONT;
}

static bool
managePlayProcessSBSSonglist (void *udata, int32_t dbidx, int32_t mqidx)
{
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: play from side-by-side songlist");
  manageQueueProcess (udata, dbidx, mqidx, DISP_SEL_SBS_SONGLIST, MANAGE_PLAY);
  return UICB_CONT;
}

static bool
managePlayProcessMusicManager (void *udata, int32_t dbidx, int32_t mqidx)
{
  manageui_t  *manage = udata;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: play from mm");

  /* if using the song editor, the play button should play the song */
  /* being currently edited.  */
  /* if there is a multi-selection, uisongselui */
  /* will play the incorrect selection */
  if (manage->mmcurrtab == MANAGE_TAB_SONGEDIT) {
    dbidx = manage->songeditdbidx;
  }

  manageQueueProcess (manage, dbidx, mqidx, DISP_SEL_MM, MANAGE_PLAY);
  return UICB_CONT;
}

static bool
manageQueueProcessSonglist (void *udata, int32_t dbidx)
{
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: queue to songlist");
  manageQueueProcess (udata, dbidx, MUSICQ_SL, DISP_SEL_SONGLIST, MANAGE_QUEUE);
  return UICB_CONT;
}

static bool
manageQueueProcessSBSSongList (void *udata, int32_t dbidx)
{
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: queue to side-by-side songlist");
  manageQueueProcess (udata, dbidx, MUSICQ_SL, DISP_SEL_SBS_SONGLIST, MANAGE_QUEUE);
  return UICB_CONT;
}

static void
manageQueueProcess (void *udata, dbidx_t dbidx, int mqidx, int dispsel, int action)
{
  manageui_t  *manage = udata;
  char        tbuff [100];
  qidx_t      loc = QUEUE_LOC_LAST;
  uimusicq_t  *uimusicq = NULL;

  logProcBegin ();
  if (dispsel == DISP_SEL_SONGLIST) {
    uimusicq = manage->slmusicq;
  }
  if (dispsel == DISP_SEL_SBS_SONGLIST) {
    uimusicq = manage->slsbsmusicq;
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
    manage->musicqueueprocessflag = false;
    manage->lastinsertlocation = loc;
    snprintf (tbuff, sizeof (tbuff), "%d%c%" PRId32 "%c%" PRId32, mqidx,
        MSG_ARGS_RS, loc, MSG_ARGS_RS, dbidx);
    connSendMessage (manage->conn, ROUTE_MAIN, MSG_MUSICQ_INSERT, tbuff);
  }

  if (action == MANAGE_PLAY) {
    char  tmp [40];

    snprintf (tmp, sizeof (tmp), "%d", mqidx);
    connSendMessage (manage->conn, ROUTE_MAIN, MSG_QUEUE_CLEAR, tmp);
    snprintf (tbuff, sizeof (tbuff), "%d%c%" PRId32 "%c%" PRId32, mqidx,
        MSG_ARGS_RS, QUEUE_LOC_LAST, MSG_ARGS_RS, dbidx);
    connSendMessage (manage->conn, ROUTE_MAIN, MSG_MUSICQ_INSERT, tbuff);
    /* and play the next song */
    connSendMessage (manage->conn, ROUTE_MAIN, MSG_CMD_NEXTSONG_PLAY, NULL);
  }
  logProcEnd ("");
}

static nlistidx_t
manageLoadMusicQueue (manageui_t *manage, int mqidx)
{
  nlistidx_t    newcount = 0;

  uimusicqProcessMusicQueueData (manage->currmusicq, mqidx);
  newcount = uimusicqGetCount (manage->currmusicq);

  return newcount;
}

/* export and import (m3u, xspf, jspf) */

static bool
managePlaylistExport (void *udata)
{
  manageui_t  *manage = udata;
  char        slname [MAX_PL_NM_LEN];

  if (manage->exportactive) {
    return UICB_STOP;
  }

  logProcBegin ();
  manage->exportactive = true;
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: export");

  manageSonglistSave (manage);

  uimusicqGetSonglistName (manage->currmusicq, slname, sizeof (slname));
  uiexpplDialog (manage->uiexppl, slname);

  manage->exportactive = false;
  logProcEnd ("");
  return UICB_CONT;
}

static bool
managePlaylistExportRespHandler (void *udata, const char *fname, int type)
{
  manageui_t  *manage = udata;
  char        slname [MAX_PL_NM_LEN];

  uimusicqGetSonglistName (manage->currmusicq, slname, sizeof (slname));
  uimusicqExport (manage->currmusicq, manage->musicqupdate [MUSICQ_SL],
      fname, slname, type);

  return UICB_CONT;
}

/* import playlist */

static bool
managePlaylistImport (void *udata)
{
  manageui_t  *manage = udata;

  if (manage->importplactive) {
    return UICB_STOP;
  }

  manage->importplactive = true;
  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: import");

  uiimpplDialog (manage->uiimppl, NULL);

  manage->importplactive = false;
  logProcEnd ("");
  return UICB_CONT;
}

static bool
managePlaylistImportRespHandler (void *udata)
{
  manageui_t  *manage = udata;
  const char  *uri;
  const char  *oplname;
  int         imptype;
  int         askey;
  char        tbuff [BDJ4_PATH_MAX];

  imptype = uiimpplGetType (manage->uiimppl);
  if (imptype == AUDIOSRC_TYPE_NONE) {
    return UICB_CONT;
  }

  uri = uiimpplGetURI (manage->uiimppl);
  askey = uiimpplGetASKey (manage->uiimppl);
  oplname = uiimpplGetOrigName (manage->uiimppl);
  manage->impplname = uiimpplGetNewName (manage->uiimppl);

  if (imptype == AUDIOSRC_TYPE_PODCAST) {
    manageResetCreateNew (manage, PLTYPE_PODCAST);
  } else {
    manageResetCreateNew (manage, PLTYPE_SONGLIST);
  }

  pathbldMakePath (tbuff, sizeof (tbuff),
      manage->impplname, BDJ4_SONGLIST_EXT, PATHBLD_MP_DREL_DATA);
  /* podcasts overwrite the old playlist */
  if (! fileopFileExists (tbuff) || imptype == AUDIOSRC_TYPE_PODCAST) {
    manageSetSonglistName (manage, manage->impplname);
  }

  /* clear the entire queue */
  uimusicqTruncateQueueCallback (manage->currmusicq);

  uiutilsProgressStatus (manage->minfo.statusMsg, 0, 0);
  uiimpplUpdateStatus (manage->uiimppl, 0, 0);
  uiUIProcessWaitEvents ();

  slistFree (manage->songidxlist);
  manage->songidxlist = slistAlloc ("mui-imppl-song-idx", LIST_UNORDERED, NULL);
  manage->imppl = impplInit (manage->songidxlist,
      imptype, uri, oplname, manage->impplname, askey);
  impplSetDB (manage->imppl, manage->musicdb);

  manage->impplstate = BDJ4_STATE_START;
  mstimeset (&manage->impplChkTime, 200);

  /* as the database may need to be re-loaded, wait for the */
  /* re-load to finish.  the pl-imp-finalize process will run from */
  /* the main loop */

  return UICB_CONT;
}

static void
managePlaylistImportFinalize (manageui_t *manage)
{
  slistidx_t  iteridx;
  const char  *nm;

  impplSetDB (manage->imppl, manage->musicdb);
  impplFinalize (manage->imppl);

  if (impplGetType (manage->imppl) == AUDIOSRC_TYPE_PODCAST) {
    manage->pltype = PLTYPE_PODCAST;
  }

  slistStartIterator (manage->songidxlist, &iteridx);
  while ((nm = slistIterateKey (manage->songidxlist, &iteridx)) != NULL) {
    dbidx_t     dbidx;
    song_t    *song;

    song = dbGetByName (manage->musicdb, nm);
    dbidx = songGetNum (song, TAG_DBIDX);
    manageQueueProcess (manage, dbidx, manage->musicqManageIdx,
        DISP_SEL_SONGLIST, MANAGE_QUEUE_LAST);
  }

  if (impplIsDBChanged (manage->imppl)) {
    manageRePopulateData (manage);
  }

  impplFree (manage->imppl);
  manage->imppl = NULL;
}

/* export/import bdj4 */

static bool
managePlaylistExportBDJ4 (void *udata)
{
  manageui_t  *manage = udata;

  if (manage->exportbdj4active) {
    return UICB_STOP;
  }

  manage->exportbdj4active = true;
  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: export bdj4");

  manageSonglistSave (manage);

  uieibdj4Dialog (manage->uieibdj4, UIEIBDJ4_EXPORT);

  manage->exportbdj4active = false;
  logProcEnd ("");
  return UICB_CONT;
}

static bool
managePlaylistImportBDJ4 (void *udata)
{
  manageui_t  *manage = udata;

  if (manage->importbdj4active) {
    return UICB_STOP;
  }

  manage->importbdj4active = true;
  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: import bdj4");

  manageSonglistSave (manage);

  uieibdj4Dialog (manage->uieibdj4, UIEIBDJ4_IMPORT);

  manage->importbdj4active = false;
  logProcEnd ("");
  return UICB_CONT;
}

static bool
manageExportBDJ4ResponseHandler (void *udata)
{
  manageui_t  *manage = udata;
  char        slname [MAX_PL_NM_LEN];
  char        *dir = NULL;
  nlist_t     *dbidxlist;

  uimusicqGetSonglistName (manage->currmusicq, slname, sizeof (slname));
  dbidxlist = uimusicqGetDBIdxList (manage->musicqupdate [MUSICQ_SL]);

  dir = uieibdj4GetDir (manage->uieibdj4);
  nlistSetStr (manage->minfo.options, MANAGE_EXP_BDJ4_DIR, dir);

  pathNormalizePath (dir, strlen (dir));
  eibdj4Free (manage->eibdj4);
  manage->eibdj4 = eibdj4Init (manage->musicdb, dir, EIBDJ4_EXPORT);
  dataFree (dir);

  eibdj4SetPlaylist (manage->eibdj4, slname);
  eibdj4SetDBIdxList (manage->eibdj4, dbidxlist);

  manage->expimpbdj4state = BDJ4_STATE_PROCESS;
  mstimeset (&manage->eibdj4ChkTime, 200);

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
  nlistSetStr (manage->minfo.options, MANAGE_IMP_BDJ4_DIR, dir);
  plname = uieibdj4GetPlaylist (manage->uieibdj4);
  newname = uieibdj4GetNewName (manage->uieibdj4);

  pathNormalizePath (dir, strlen (dir));
  eibdj4Free (manage->eibdj4);
  manage->eibdj4 = eibdj4Init (manage->musicdb, dir, EIBDJ4_IMPORT);
  dataFree (dir);

  eibdj4SetPlaylist (manage->eibdj4, plname);
  eibdj4SetNewName (manage->eibdj4, newname);

  manage->expimpbdj4state = BDJ4_STATE_PROCESS;
  mstimeset (&manage->eibdj4ChkTime, 400);

  return UICB_CONT;
}

/* general */

static bool
manageSwitchPageMain (void *udata, int32_t pagenum)
{
  manageui_t  *manage = udata;

  manageSwitchPage (manage, pagenum, MANAGE_NB_MAIN);
  return UICB_CONT;
}

static bool
manageSwitchPageSonglist (void *udata, int32_t pagenum)
{
  manageui_t  *manage = udata;

  manageSwitchPage (manage, pagenum, MANAGE_NB_SONGLIST);
  return UICB_CONT;
}

static bool
manageSwitchPageMM (void *udata, int32_t pagenum)
{
  manageui_t  *manage = udata;

  manageSwitchPage (manage, pagenum, MANAGE_NB_MM);
  return UICB_CONT;
}

static void
manageSwitchPage (manageui_t *manage, int pagenum, int which)
{
  int         id;
  int         mainlasttabsel;
  int         mmlasttabsel;
  bool        mainnb = false;
  bool        slnb = false;
  bool        mmnb = false;
  uinbtabid_t *nbtabid = NULL;

  logProcBegin ();
  uiLabelSetText (manage->minfo.errorMsg, "");
  uiLabelSetText (manage->minfo.statusMsg, "");

  /* need to know which notebook is selected so that the correct id value */
  /* can be retrieved */
  nbtabid = manage->nbtabid [which];
  if (which == MANAGE_NB_SONGLIST) {
    logMsg (LOG_DBG, LOG_INFO, "switch page in songlist");
    contInstanceSetUIPlayer (manage->continst, manage->slplayer);
    slnb = true;
  } else if (which == MANAGE_NB_MM) {
    logMsg (LOG_DBG, LOG_INFO, "switch page in mm");
    contInstanceSetUIPlayer (manage->continst, manage->mmplayer);
    mmnb = true;
  } else {
    logMsg (LOG_DBG, LOG_INFO, "switch page in main");
    mainnb = true;
  }

  mainlasttabsel = manage->maincurrtab;
  mmlasttabsel = manage->mmcurrtab;

  if (mainnb) {
    if (manage->maincurrtab == MANAGE_TAB_MAIN_SL) {
      logMsg (LOG_DBG, LOG_INFO, "last tab: songlist");
      manageSonglistSave (manage);
    }
    if (manage->maincurrtab == MANAGE_TAB_MAIN_SEQ) {
      logMsg (LOG_DBG, LOG_INFO, "last tab: sequence");
      manageSequenceSave (manage->manageseq);
    }
    if (manage->maincurrtab == MANAGE_TAB_MAIN_PLMGMT) {
      logMsg (LOG_DBG, LOG_INFO, "last tab: playlist");
      managePlaylistSave (manage->managepl);
    }
  }

  id = uinbutilIDGet (nbtabid, pagenum);
  if (mainnb) {
    id = uivnbGetID (manage->mainvnb);
  }

  if (manage->currmenu != NULL) {
    uiMenuClear (manage->currmenu);
  }

  if (mainnb) {
    manage->maincurrtab = id;
  }
  if (slnb) {
    manage->slcurrtab = id;
  }
  if (mmnb) {
    manage->mmcurrtab = id;
  }

  switch (id) {
    case MANAGE_TAB_MAIN_SL: {
      logMsg (LOG_DBG, LOG_INFO, "new tab: main-sl");
      manageSetDisplayPerSelection (manage, mainlasttabsel);
      manageSonglistLoadCheck (manage);
      break;
    }
    case MANAGE_TAB_MAIN_SEQ: {
      logMsg (LOG_DBG, LOG_INFO, "new tab: main-seq");
      manageSequenceLoadCheck (manage->manageseq);
      manage->currmenu = manageSequenceMenu (manage->manageseq,
          manage->wcont [MANAGE_W_MENUBAR]);
      break;
    }
    case MANAGE_TAB_MAIN_PLMGMT: {
      logMsg (LOG_DBG, LOG_INFO, "new tab: main-pl");
      managePlaylistLoadCheck (manage->managepl);
      manage->currmenu = managePlaylistMenu (manage->managepl, manage->wcont [MANAGE_W_MENUBAR]);
      break;
    }
    case MANAGE_TAB_MAIN_MM: {
      logMsg (LOG_DBG, LOG_INFO, "new tab: main-mm");
      manageSetDisplayPerSelection (manage, mainlasttabsel);
      break;
    }
    case MANAGE_TAB_MAIN_UPDDB: {
      break;
    }
    default: {
      /* sub-tabs are handled below */
      break;
    }
  }

  if (mainnb && id == MANAGE_TAB_MAIN_SL) {
    /* force menu selection */
    slnb = true;
    id = manage->slcurrtab;
  }
  if (mainnb && id == MANAGE_TAB_MAIN_MM) {
    /* force menu selection */
    mmnb = true;
    id = manage->mmcurrtab;
  }

  /* always update the songedit-dbidx */
  manageSetSongEditDBIdx (manage, mainlasttabsel, mmlasttabsel);

  switch (id) {
    case MANAGE_TAB_SONGLIST: {
      logMsg (LOG_DBG, LOG_INFO, "new sub-tab: sl-songlist");
      manageSonglistMenu (manage);
      break;
    }
    case MANAGE_TAB_SL_SONGSEL: {
      logMsg (LOG_DBG, LOG_INFO, "new sub-tab: sl-songsel");
      /* this is necessary, as switching back to the sl-songlist tab */
      /* does not call this */
      manageSetDisplayPerSelection (manage, mainlasttabsel);
      break;
    }
    case MANAGE_TAB_STATISTICS: {
      logMsg (LOG_DBG, LOG_INFO, "new sub-tab: sl-stats");
      break;
    }
    case MANAGE_TAB_MM: {
      logMsg (LOG_DBG, LOG_INFO, "new sub-tab: mm-mm");
      manageMusicManagerMenu (manage);
      break;
    }
    case MANAGE_TAB_SONGEDIT: {
      logMsg (LOG_DBG, LOG_INFO, "new sub-tab: mm-se");
      manageSongEditMenu (manage);
      manageReloadSongData (manage);
      break;
    }
    case MANAGE_TAB_AUDIOID: {
      logMsg (LOG_DBG, LOG_INFO, "new sub-tab: mm-audid");
      manage->currmenu = manageAudioIDMenu (manage->manageaudioid,
          manage->wcont [MANAGE_W_MENUBAR]);
      manageReloadSongData (manage);
      break;
    }
    default: {
      break;
    }
  }
  logProcEnd ("");
}

/* when switching pages, the music manager must be adjusted to */
/* display the song selection list or the playlist depending on */
/* what was last selected. the song selection lists never display */
/* the playlist.  This is all very messy, especially for the filter */
/* display, but makes the song editor easier to use and more intuitive. */
/* note that this routine gets called upon the appropriate page switch. */
static void
manageSetDisplayPerSelection (manageui_t *manage, int mainlasttab)
{
  logProcBegin ();

  /* switching to the song-list */
  if (manage->maincurrtab == MANAGE_TAB_MAIN_SL) {
    bool    plinuse;

    /* save current playlist-in-use status */
    plinuse = uisfPlaylistInUse (manage->uisongfilter);
    uisfHidePlaylistDisplay (manage->uisongfilter);

    if (manage->slcurrtab == MANAGE_TAB_SL_SONGSEL || manage->sbssonglist) {
      /* the song filter must be updated, as it is shared */
      /* the apply-song-filter call will reset the apply-callback correctly */
      uisfClearPlaylist (manage->uisongfilter);
      manage->selbypass = true;
      if (manage->sbssonglist) {
        uisongselApplySongFilter (manage->slsbssongsel);
      } else {
        uisongselApplySongFilter (manage->slsongsel);
      }
      manage->selbypass = false;
    }

    /* switching from music-manager */
    if (mainlasttab == MANAGE_TAB_MAIN_MM &&
        (plinuse || manage->lastmmdisp == MANAGE_DISP_SONG_LIST)) {
      nlistidx_t    nidx;

      /* get the selection from mm, and set it for the sle */
      nidx = uisongselGetSelectLocation (manage->mmsongsel);
      uimusicqSetSelectLocation (manage->currmusicq, manage->musicqManageIdx, nidx);
    }

    /* switching from music-manager */
    if (mainlasttab == MANAGE_TAB_MAIN_MM &&
        manage->lastmmdisp == MANAGE_DISP_SONG_SEL) {
      if (manage->sbssonglist) {
        uisongselCopySelectList (manage->mmsongsel, manage->slsbssongsel);
      } else {
        uisongselCopySelectList (manage->mmsongsel, manage->slsongsel);
      }
    }
  }

  /* switching to the music-manager */
  if (manage->maincurrtab == MANAGE_TAB_MAIN_MM) {
    char    slname [MAX_PL_NM_LEN];
    int     lasttab;

    /* use a copy, as it will change */
    lasttab = manage->lasttabsel;
    uisfShowPlaylistDisplay (manage->uisongfilter);

    /* switching to mm  */
    /*   a) from the song list - has songs */
    /*   b) last selection was on the song list */
    if (mainlasttab == MANAGE_TAB_MAIN_SL) {
      if ((manage->slcurrtab == MANAGE_TAB_SONGLIST &&
          uimusicqGetCount (manage->currmusicq) > 0) ||
          lasttab == MANAGE_TAB_SONGLIST) {
        nlistidx_t    idx;

        /* the song list must be saved, otherwise the song filter */
        /* can't load it */
        manageSonglistSave (manage);
        uimusicqGetSonglistName (manage->currmusicq, slname, sizeof (slname));
        uisfSetPlaylist (manage->uisongfilter, slname);
        manage->selbypass = true;
        /* the apply-song-filter call will reset the apply-callback correctly */
        uisongselApplySongFilter (manage->mmsongsel);
        manage->selbypass = false;
        manage->lastmmdisp = MANAGE_DISP_SONG_LIST;
        idx = uimusicqGetSelectLocation (manage->currmusicq, manage->musicqManageIdx);
        uisongselSetSelection (manage->mmsongsel, idx);
      }
    }

    /* switching to mm */
    /*   a) from the song selection tab */
    /*   b) from the song list - no songs */
    /*   c) last selection was on the song selection */
    if (mainlasttab == MANAGE_TAB_MAIN_SL) {
      if (manage->slcurrtab == MANAGE_TAB_SL_SONGSEL ||
          uimusicqGetCount (manage->currmusicq) == 0 ||
          lasttab == MANAGE_TAB_SL_SONGSEL) {
        uisfClearPlaylist (manage->uisongfilter);
        manage->selbypass = true;
        /* the apply-song-filter call will reset the apply-callback correctly */
        uisongselApplySongFilter (manage->mmsongsel);
        manage->selbypass = false;
        if (manage->sbssonglist) {
          uisongselCopySelectList (manage->slsbssongsel, manage->mmsongsel);
        } else {
          uisongselCopySelectList (manage->slsongsel, manage->mmsongsel);
        }
        manage->lastmmdisp = MANAGE_DISP_SONG_SEL;
      }
    }
  }
  logProcEnd ("");
}

static void
manageSetMenuCallback (manageui_t *manage, int midx, callbackFunc cb)
{
  logProcBegin ();
  manage->callbacks [midx] = callbackInit (cb, manage, NULL);
  logProcEnd ("");
}

/* the current songlist may be renamed or deleted. */
/* check for this and if the songlist has */
/* disappeared, reset */
static void
manageSonglistLoadCheck (manageui_t *manage)
{
  char  name [MAX_PL_NM_LEN];

  logProcBegin ();
  if (! *manage->sloldname) {
    logProcEnd ("no-old-name");
    return;
  }

  uimusicqGetSonglistName (manage->currmusicq, name, sizeof (name));

  if (! songlistExists (name)) {
    logMsg (LOG_DBG, LOG_INFO, "no songlist %s", name);
    /* make sure no save happens */
    *manage->sloldname = '\0';
    manageResetCreateNew (manage, PLTYPE_SONGLIST);
  } else {
    manageLoadPlaylistCB (manage, name);
  }
  logProcEnd ("");
}

static void
manageProcessDatabaseUpdate (manageui_t *manage)
{
  manage->dbloaded = false;
  connSendMessage (manage->conn, ROUTE_STARTERUI, MSG_DB_RELOAD, NULL);

  samesongFree (manage->samesong);
  manage->musicdb = bdj4ReloadDatabase (manage->musicdb);
  manage->minfo.musicdb = manage->musicdb;
  manage->samesong = samesongAlloc (manage->musicdb);

  manageUpdateDBPointers (manage);

  uisongselApplySongFilter (manage->slsongsel);
}

static void
manageUpdateDBPointers (manageui_t *manage)
{
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
  uisongeditSetDatabase (manage->mmsongedit, manage->musicdb);
  songdbSetMusicDB (manage->songdb, manage->musicdb);
}


static uimusicq_t *
manageGetCurrMusicQ (manageui_t *manage)
{
  uimusicq_t  *uimusicq = NULL;

  if (! manage->sbssonglist) {
    uimusicq = manage->slmusicq;
  }
  if (manage->sbssonglist) {
    uimusicq = manage->slsbsmusicq;
  }

  return uimusicq;
}

/* bpm counter */

static bool
manageStartBPMCounter (void *udata)
{
  manageui_t  *manage = udata;
  const char  *targv [2];
  int         targc = 0;

  logProcBegin ();

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
  logProcEnd ("");
  return UICB_CONT;
}

static void
manageSetBPMCounter (manageui_t *manage, song_t *song)
{
  ilistidx_t  danceIdx;

  logProcBegin ();

  danceIdx = songGetNum (song, TAG_DANCE);
  if (danceIdx < 0) {
    return;
  }
  manage->currtimesig = danceGetTimeSignature (danceIdx);

  manageSendBPMCounter (manage);
  logProcEnd ("");
}

static void
manageSendBPMCounter (manageui_t *manage)
{
  char        tbuff [60];

  logProcBegin ();
  if (! manage->bpmcounterstarted) {
    logProcEnd ("not-started");
    return;
  }

  snprintf (tbuff, sizeof (tbuff), "%d", manage->currtimesig);
  connSendMessage (manage->conn, ROUTE_BPM_COUNTER, MSG_BPM_TIMESIG, tbuff);
  logProcEnd ("");
}

/* same song */

static bool
manageSameSongSetMark (void *udata)
{
  manageui_t  *manage = udata;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: set mark");
  manageSameSongChangeMark (manage, MANAGE_SET_MARK);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
manageSameSongClearMark (void *udata)
{
  manageui_t  *manage = udata;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: clear mark");
  manageSameSongChangeMark (manage, MANAGE_CLEAR_MARK);
  logProcEnd ("");
  return UICB_CONT;
}

static void
manageSameSongChangeMark (manageui_t *manage, int flag)
{
  nlist_t     *sellist;
  nlistidx_t  iteridx;
  dbidx_t     dbidx;

  logProcBegin ();
  sellist = uisongselGetSelectedList (manage->mmsongsel);

  if (flag == MANAGE_SET_MARK) {
    samesongSet (manage->samesong, sellist);
  }
  if (flag == MANAGE_CLEAR_MARK) {
    samesongClear (manage->samesong, sellist);
  }

  nlistStartIterator (sellist, &iteridx);
  while ((dbidx = nlistIterateKey (sellist, &iteridx)) >= 0) {
    songdbWriteDB (manage->songdb, dbidx, false);
  }

  nlistFree (sellist);
  /* the same song marks only appear in the music manager */
  uisongselPopulateData (manage->mmsongsel);
  logProcEnd ("");
}

/* remove song */

static bool
manageMarkSongRemoved (void *udata)
{
  manageui_t  *manage = udata;
  nlist_t     *sellist;
  nlistidx_t  iteridx;
  dbidx_t     dbidx;

  logProcBegin ();

  sellist = uisongselGetSelectedList (manage->mmsongsel);

  nlistStartIterator (sellist, &iteridx);
  while ((dbidx = nlistIterateKey (sellist, &iteridx)) >= 0) {
    song_t  *song;

    song = dbGetByIdx (manage->musicdb, dbidx);
    if (song == NULL) {
      continue;
    }
    nlistSetStr (manage->removelist, dbidx, songGetStr (song, TAG_URI));
    dbMarkEntryRemoved (manage->musicdb, dbidx);
  }

  manageRePopulateData (manage);

  uiWidgetSetState (manage->wcont [MANAGE_W_MENUITEM_UNDO_REMOVE], UIWIDGET_ENABLE);
  uiWidgetSetState (manage->wcont [MANAGE_W_MENUITEM_UNDO_ALL_REMOVE], UIWIDGET_ENABLE);

  nlistFree (sellist);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
manageUndoRemove (void *udata)
{
  manageui_t  *manage = udata;
  nlist_t     *sellist;
  nlist_t     *trmlist;
  nlistidx_t  iteridx;
  dbidx_t     dbidx;
  dbidx_t     tdbidx;

  sellist = uisongselGetSelectedList (manage->mmsongsel);

  nlistStartIterator (sellist, &iteridx);
  while ((dbidx = nlistIterateKey (sellist, &iteridx)) >= 0) {
    song_t  *song;

    song = dbGetByIdx (manage->musicdb, dbidx);
    if (song == NULL) {
      continue;
    }
    dbClearEntryRemoved (manage->musicdb, dbidx);
  }

  trmlist = nlistAlloc ("remove-list", LIST_ORDERED, NULL);

  nlistStartIterator (sellist, &iteridx);
  while ((tdbidx = nlistIterateKey (sellist, &iteridx)) >= 0) {
    if (nlistGetStr (manage->removelist, dbidx) != NULL) {
      /* set entry in removelist to null */
      nlistSetStr (manage->removelist, dbidx, NULL);
    }
  }

  /* rebuild the removelist, skipping those entries that were set to null */
  nlistStartIterator (manage->removelist, &iteridx);
  while ((dbidx = nlistIterateKey (manage->removelist, &iteridx)) >= 0) {
    if (nlistGetStr (manage->removelist, dbidx) == NULL) {
      continue;
    }
    nlistSetStr (trmlist, dbidx, nlistGetStr (manage->removelist, dbidx));
  }

  nlistFree (manage->removelist);
  manage->removelist = trmlist;

  manageRePopulateData (manage);

  uiWidgetSetState (manage->wcont [MANAGE_W_MENUITEM_UNDO_REMOVE], UIWIDGET_ENABLE);
  if (nlistGetCount (manage->removelist) == 0) {
    uiWidgetSetState (manage->wcont [MANAGE_W_MENUITEM_UNDO_ALL_REMOVE], UIWIDGET_ENABLE);
  }

  nlistFree (sellist);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
manageUndoRemoveAll (void *udata)
{
  manageui_t  *manage = udata;
  nlistidx_t  iteridx;
  dbidx_t     dbidx;

  nlistStartIterator (manage->removelist, &iteridx);
  while ((dbidx = nlistIterateKey (manage->removelist, &iteridx)) >= 0) {
    dbClearEntryRemoved (manage->musicdb, dbidx);
  }

  nlistFree (manage->removelist);
  manage->removelist = nlistAlloc ("remove-list", LIST_ORDERED, NULL);
  uiWidgetSetState (manage->wcont [MANAGE_W_MENUITEM_UNDO_REMOVE], UIWIDGET_DISABLE);
  uiWidgetSetState (manage->wcont [MANAGE_W_MENUITEM_UNDO_ALL_REMOVE], UIWIDGET_DISABLE);

  manageRePopulateData (manage);

  return UICB_CONT;
}

/* renames the audio file, or if not a file-type, */
/* usually clears the entry from the database. */
/* this routine is called upon music manager exit, so the database */
/* will be re-loaded on startup */
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
    if (! audiosrcRemove (songfn)) {
      /* only need to know if file-type */
      if (audiosrcGetType (songfn) != AUDIOSRC_TYPE_FILE) {
        /* completely clears the entry from the database */
        dbRemoveSong (manage->musicdb, dbidx);
      }
    }
  }
}

