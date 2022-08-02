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
#include <stdarg.h>

#include <gtk/gtk.h>

#include "bdj4.h"
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "bdjvarsdf.h"
#include "bdjvarsdfload.h"
#include "configui.h"
#include "conn.h"
#include "dance.h"
#include "datafile.h"
#include "dirop.h"
#include "dispsel.h"
#include "dnctypes.h"
#include "filedata.h"
#include "fileop.h"
#include "filemanip.h"
#include "genre.h"
#include "ilist.h"
#include "level.h"
#include "localeutil.h"
#include "lock.h"
#include "log.h"
#include "musicq.h"
#include "nlist.h"
#include "orgopt.h"
#include "orgutil.h"
#include "osuiutils.h"
#include "osutils.h"
#include "pathbld.h"
#include "pathutil.h"
#include "pli.h"          // needed for volume sinklist
#include "progstate.h"
#include "rating.h"
#include "slist.h"
#include "sock.h"
#include "sockh.h"
#include "songfilter.h"
#include "status.h"
#include "sysvars.h"
#include "tagdef.h"
#include "templateutil.h"
#include "tmutil.h"
#include "ui.h"
#include "uinbutil.h"
#include "uiduallist.h"
#include "validate.h"
#include "volsink.h"
#include "volume.h"     // needed for volume sink list
#include "webclient.h"

typedef struct configui {
  progstate_t       *progstate;
  char              *locknm;
  conn_t            *conn;
  int               dbgflags;
  int               stopwaitcount;
  datafile_t        *filterDisplayDf;
  nlist_t           *filterDisplaySel;
  nlist_t           *filterLookup;
  confuigui_t       gui;
  /* options */
  datafile_t        *optiondf;
  nlist_t           *options;
} configui_t;

static datafilekey_t configuidfkeys [CONFUI_KEY_MAX] = {
  { "CONFUI_POS_X",     CONFUI_POSITION_X,    VALUE_NUM, NULL, -1 },
  { "CONFUI_POS_Y",     CONFUI_POSITION_Y,    VALUE_NUM, NULL, -1 },
  { "CONFUI_SIZE_X",    CONFUI_SIZE_X,        VALUE_NUM, NULL, -1 },
  { "CONFUI_SIZE_Y",    CONFUI_SIZE_Y,        VALUE_NUM, NULL, -1 },
};

static bool     confuiHandshakeCallback (void *udata, programstate_t programState);
static bool     confuiStoppingCallback (void *udata, programstate_t programState);
static bool     confuiStopWaitCallback (void *udata, programstate_t programState);
static bool     confuiClosingCallback (void *udata, programstate_t programState);
static void     confuiBuildUI (configui_t *confui);
static void     confuiBuildUIPlayer (configui_t *confui);
static void     confuiBuildUIMarquee (configui_t *confui);
static void     confuiBuildUIUserInterface (configui_t *confui);
static void     confuiBuildUIDispSettings (configui_t *confui);
static void     confuiBuildUIFilterDisplay (configui_t *confui);
static void     confuiBuildUIOrganization (configui_t *confui);
static void     confuiBuildUIMobileRemoteControl (configui_t *confui);
static void     confuiBuildUIMobileMarquee (configui_t *confui);
static void     confuiBuildUIDebugOptions (configui_t *confui);
static int      confuiMainLoop  (void *tconfui);
static int      confuiProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static bool     confuiCloseWin (void *udata);
static void     confuiSigHandler (int sig);

static void confuiPopulateOptions (configui_t *confui);

/* display settings */
static bool   confuiDispSettingChg (void *udata);
static void   confuiDispSaveTable (confuigui_t *gui, int selidx);

/* misc */
static void confuiLoadTagList (configui_t *confui);
static bool confuiMobmqTypeChg (void *udata);
static bool confuiMobmqPortChg (void *udata);
static int  confuiMobmqNameChg (uientry_t *entry, void *udata);
static int  confuiMobmqTitleChg (uientry_t *entry, void *udata);
static bool confuiRemctrlChg (void *udata, int value);
static bool confuiRemctrlPortChg (void *udata);

static bool gKillReceived = false;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  char            *uifont = NULL;
  nlist_t         *tlist = NULL;
  nlist_t         *llist = NULL;
  nlistidx_t      iteridx;
  int             count;
  char            *p = NULL;
  volsinklist_t   sinklist;
  configui_t      confui;
  volume_t        *volume = NULL;
  orgopt_t        *orgopt = NULL;
  int             flags;
  char            tbuff [MAXPATHLEN];
  char            tbuffb [MAXPATHLEN];
  char            tbuffc [MAXPATHLEN];


  confui.progstate = progstateInit ("configui");
  progstateSetCallback (confui.progstate, STATE_WAIT_HANDSHAKE,
      confuiHandshakeCallback, &confui);
  confui.locknm = NULL;
  confui.conn = NULL;
  confui.dbgflags = 0;
  confui.stopwaitcount = 0;
  confui.filterDisplayDf = NULL;
  confui.filterDisplaySel = NULL;
  confui.filterLookup = NULL;

  confui.gui.localip = NULL;
  uiutilsUIWidgetInit (&confui.gui.window);
  uiutilsUICallbackInit (&confui.gui.closecb, NULL, NULL);
  uiutilsUIWidgetInit (&confui.gui.notebook);
  uiutilsUICallbackInit (&confui.gui.nbcb, NULL, NULL);
  confui.gui.nbtabid = uiutilsNotebookIDInit ();
  uiutilsUIWidgetInit (&confui.gui.vbox);
  uiutilsUIWidgetInit (&confui.gui.statusMsg);
  confui.gui.tablecurr = CONFUI_ID_NONE;
  confui.gui.dispsel = NULL;
  confui.gui.dispselduallist = NULL;
  confui.gui.edittaglist = NULL;
  confui.gui.listingtaglist = NULL;
  confui.gui.indancechange = false;

  confui.optiondf = NULL;
  confui.options = NULL;

  for (int i = 0; i < CONFUI_ID_TABLE_MAX; ++i) {
    for (int j = 0; j < CONFUI_TABLE_CB_MAX; ++j) {
      uiutilsUICallbackInit (&confui.gui.tables [i].callback [j], NULL, NULL);
    }
    confui.gui.tables [i].tree = NULL;
    confui.gui.tables [i].sel = NULL;
    confui.gui.tables [i].radiorow = 0;
    confui.gui.tables [i].togglecol = -1;
    confui.gui.tables [i].flags = CONFUI_TABLE_NONE;
    confui.gui.tables [i].changed = false;
    confui.gui.tables [i].currcount = 0;
    confui.gui.tables [i].saveidx = 0;
    confui.gui.tables [i].savelist = NULL;
    confui.gui.tables [i].listcreatefunc = NULL;
    confui.gui.tables [i].savefunc = NULL;
  }

  for (int i = 0; i < CONFUI_ITEM_MAX; ++i) {
    confui.gui.uiitem [i].widget = NULL;
    confui.gui.uiitem [i].basetype = CONFUI_NONE;
    confui.gui.uiitem [i].outtype = CONFUI_OUT_NONE;
    confui.gui.uiitem [i].bdjoptIdx = -1;
    confui.gui.uiitem [i].listidx = 0;
    confui.gui.uiitem [i].displist = NULL;
    confui.gui.uiitem [i].sbkeylist = NULL;
    confui.gui.uiitem [i].danceidx = DANCE_DANCE;
    uiutilsUIWidgetInit (&confui.gui.uiitem [i].uiwidget);
    uiutilsUICallbackInit (&confui.gui.uiitem [i].callback, NULL, NULL);
    confui.gui.uiitem [i].uri = NULL;

    if (i > CONFUI_BEGIN && i < CONFUI_COMBOBOX_MAX) {
      confui.gui.uiitem [i].dropdown = uiDropDownInit ();
    }
    if (i > CONFUI_ENTRY_MAX && i < CONFUI_SPINBOX_MAX) {
      if (i == CONFUI_SPINBOX_MAX_PLAY_TIME) {
        confui.gui.uiitem [i].spinbox = uiSpinboxTimeInit (SB_TIME_BASIC);
      } else {
        confui.gui.uiitem [i].spinbox = uiSpinboxTextInit ();
      }
    }
    if (i > CONFUI_SPINBOX_MAX && i < CONFUI_SWITCH_MAX) {
      confui.gui.uiitem [i].uiswitch = NULL;
    }
  }

  confui.gui.uiitem [CONFUI_ENTRY_DANCE_TAGS].entry = uiEntryInit (30, 100);
  confui.gui.uiitem [CONFUI_ENTRY_DANCE_ANNOUNCEMENT].entry = uiEntryInit (30, 300);
  confui.gui.uiitem [CONFUI_ENTRY_DANCE_DANCE].entry = uiEntryInit (30, 50);
  confui.gui.uiitem [CONFUI_ENTRY_MM_NAME].entry = uiEntryInit (10, 40);
  confui.gui.uiitem [CONFUI_ENTRY_MM_TITLE].entry = uiEntryInit (20, 100);
  confui.gui.uiitem [CONFUI_ENTRY_MUSIC_DIR].entry = uiEntryInit (50, 300);
  confui.gui.uiitem [CONFUI_ENTRY_PROFILE_NAME].entry = uiEntryInit (20, 30);
  confui.gui.uiitem [CONFUI_ENTRY_COMPLETE_MSG].entry = uiEntryInit (20, 30);
  confui.gui.uiitem [CONFUI_ENTRY_QUEUE_NM_A].entry = uiEntryInit (20, 30);
  confui.gui.uiitem [CONFUI_ENTRY_QUEUE_NM_B].entry = uiEntryInit (20, 30);
  confui.gui.uiitem [CONFUI_ENTRY_RC_PASS].entry = uiEntryInit (10, 20);
  confui.gui.uiitem [CONFUI_ENTRY_RC_USER_ID].entry = uiEntryInit (10, 30);
  confui.gui.uiitem [CONFUI_ENTRY_STARTUP].entry = uiEntryInit (50, 300);
  confui.gui.uiitem [CONFUI_ENTRY_SHUTDOWN].entry = uiEntryInit (50, 300);

  osSetStandardSignals (confuiSigHandler);

  flags = BDJ4_INIT_NO_DB_LOAD;
  confui.dbgflags = bdj4startup (argc, argv, NULL,
      "cu", ROUTE_CONFIGUI, flags);
  logProcBegin (LOG_PROC, "configui");

  confui.gui.dispsel = dispselAlloc ();

  orgopt = orgoptAlloc ();
  assert (orgopt != NULL);
  tlist = orgoptGetList (orgopt);

  confui.gui.uiitem [CONFUI_COMBOBOX_AO_PATHFMT].displist = tlist;
  slistStartIterator (tlist, &iteridx);
  confui.gui.uiitem [CONFUI_COMBOBOX_AO_PATHFMT].listidx = 0;
  count = 0;
  while ((p = slistIterateValueData (tlist, &iteridx)) != NULL) {
    if (strcmp (p, bdjoptGetStr (OPT_G_AO_PATHFMT)) == 0) {
      confui.gui.uiitem [CONFUI_COMBOBOX_AO_PATHFMT].listidx = count;
      break;
    }
    ++count;
  }

  volume = volumeInit ();
  assert (volume != NULL);
  volumeSinklistInit (&sinklist);
  volumeGetSinkList (volume, "", &sinklist);
  if (! volumeHaveSinkList (volume)) {
    pli_t     *pli;

    pli = pliInit (bdjoptGetStr (OPT_M_VOLUME_INTFC), "default");
    pliAudioDeviceList (pli, &sinklist);
  }

  tlist = nlistAlloc ("cu-audio-out", LIST_ORDERED, free);
  llist = nlistAlloc ("cu-audio-out-l", LIST_ORDERED, free);
  /* CONTEXT: configuration: audio: The default audio sink (audio output) */
  nlistSetStr (tlist, 0, _("Default"));
  nlistSetStr (llist, 0, "default");
  confui.gui.uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].listidx = 0;
  for (size_t i = 0; i < sinklist.count; ++i) {
    if (strcmp (sinklist.sinklist [i].name, bdjoptGetStr (OPT_MP_AUDIOSINK)) == 0) {
      confui.gui.uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].listidx = i + 1;
    }
    nlistSetStr (tlist, i + 1, sinklist.sinklist [i].description);
    nlistSetStr (llist, i + 1, sinklist.sinklist [i].name);
  }
  confui.gui.uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].displist = tlist;
  confui.gui.uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].sbkeylist = llist;

  volumeFreeSinkList (&sinklist);
  volumeFree (volume);

  confuiSpinboxTextInitDataNum (&confui.gui, "cu-audio-file-tags",
      CONFUI_SPINBOX_WRITE_AUDIO_FILE_TAGS,
      /* CONTEXT: configuration: write audio file tags: do not write any tags to the audio file */
      WRITE_TAGS_NONE, _("Don't Write"),
      /* CONTEXT: configuration: write audio file tags: only write BDJ tags to the audio file */
      WRITE_TAGS_BDJ_ONLY, _("BDJ Tags Only"),
      /* CONTEXT: configuration: write audio file tags: write all tags (BDJ and standard) to the audio file */
      WRITE_TAGS_ALL, _("All Tags"),
      -1);

  confuiSpinboxTextInitDataNum (&confui.gui, "cu-bpm",
      CONFUI_SPINBOX_BPM,
      /* CONTEXT: configuration: BPM: beats per minute (not bars per minute) */
      BPM_BPM, "BPM",
      /* CONTEXT: configuration: MPM: measures per minute (aka bars per minute) */
      BPM_MPM, "MPM",
      -1);

  confuiSpinboxTextInitDataNum (&confui.gui, "cu-dance-speed",
      CONFUI_SPINBOX_DANCE_SPEED,
      /* CONTEXT: configuration: dance speed */
      DANCE_SPEED_SLOW, _("slow"),
      /* CONTEXT: configuration: dance speed */
      DANCE_SPEED_NORMAL, _("normal"),
      /* CONTEXT: configuration: dance speed */
      DANCE_SPEED_FAST, _("fast"),
      -1);

  confuiSpinboxTextInitDataNum (&confui.gui, "cu-dance-time-sig",
      CONFUI_SPINBOX_DANCE_TIME_SIG,
      /* CONTEXT: configuration: dance time signature */
      DANCE_TIMESIG_24, _("2/4"),
      /* CONTEXT: configuration: dance time signature */
      DANCE_TIMESIG_34, _("3/4"),
      /* CONTEXT: configuration: dance time signature */
      DANCE_TIMESIG_44, _("4/4"),
      /* CONTEXT: configuration: dance time signature */
      DANCE_TIMESIG_48, _("4/8"),
      -1);

  /* CONTEXT: configuration: display settings for: song editor column N */
  snprintf (tbuff, sizeof (tbuff), _("Song Editor - Column %d"), 1);
  snprintf (tbuffb, sizeof (tbuffb), _("Song Editor - Column %d"), 2);
  snprintf (tbuffc, sizeof (tbuffc), _("Song Editor - Column %d"), 3);
  confuiSpinboxTextInitDataNum (&confui.gui, "cu-disp-settings",
      CONFUI_SPINBOX_DISP_SEL,
      /* CONTEXT: configuration: display settings for: music queue */
      DISP_SEL_MUSICQ, _("Music Queue"),
      /* CONTEXT: configuration: display settings for: requests */
      DISP_SEL_REQUEST, _("Request"),
      /* CONTEXT: configuration: display settings for: song list */
      DISP_SEL_SONGLIST, _("Song List"),
      /* CONTEXT: configuration: display settings for: song selection */
      DISP_SEL_SONGSEL, _("Song Selection"),
      /* CONTEXT: configuration: display settings for: easy song list */
      DISP_SEL_EZSONGLIST, _("Easy Song List"),
      /* CONTEXT: configuration: display settings for: easy song selection */
      DISP_SEL_EZSONGSEL, _("Easy Song Selection"),
      /* CONTEXT: configuration: display settings for: music manager */
      DISP_SEL_MM, _("Music Manager"),
      DISP_SEL_SONGEDIT_A, tbuff,
      DISP_SEL_SONGEDIT_B, tbuffb,
      DISP_SEL_SONGEDIT_C, tbuffc,
      -1);
  confui.gui.uiitem [CONFUI_SPINBOX_DISP_SEL].listidx = DISP_SEL_MUSICQ;

  confuiLoadHTMLList (&confui.gui);
  confuiLoadVolIntfcList (&confui.gui);
  confuiLoadPlayerIntfcList (&confui.gui);
  confuiLoadLocaleList (&confui.gui);
  confuiLoadDanceTypeList (&confui.gui);
  confuiLoadTagList (&confui);
  confuiLoadThemeList (&confui.gui);

  confuiSpinboxTextInitDataNum (&confui.gui, "cu-mob-mq",
      CONFUI_SPINBOX_MOBILE_MQ,
      /* CONTEXT: configuration: mobile marquee: off */
      MOBILEMQ_OFF, _("Off"),
      /* CONTEXT: configuration: mobile marquee: use local router */
      MOBILEMQ_LOCAL, _("Local"),
      /* CONTEXT: configuration: mobile marquee: route via the internet */
      MOBILEMQ_INTERNET, _("Internet"),
      -1);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "ds-songfilter", BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  confui.filterDisplayDf = datafileAllocParse ("cu-filter",
      DFTYPE_KEY_VAL, tbuff, filterdisplaydfkeys, FILTER_DISP_MAX, DATAFILE_NO_LOOKUP);
  confui.filterDisplaySel = datafileGetList (confui.filterDisplayDf);
  llist = nlistAlloc ("cu-filter-out", LIST_ORDERED, free);
  nlistStartIterator (confui.filterDisplaySel, &iteridx);
  nlistSetNum (llist, CONFUI_WIDGET_FILTER_GENRE, FILTER_DISP_GENRE);
  nlistSetNum (llist, CONFUI_WIDGET_FILTER_DANCELEVEL, FILTER_DISP_DANCELEVEL);
  nlistSetNum (llist, CONFUI_WIDGET_FILTER_STATUS, FILTER_DISP_STATUS);
  nlistSetNum (llist, CONFUI_WIDGET_FILTER_FAVORITE, FILTER_DISP_FAVORITE);
  nlistSetNum (llist, CONFUI_SWITCH_FILTER_STATUS_PLAYABLE, FILTER_DISP_STATUSPLAYABLE);
  confui.filterLookup = llist;

  listenPort = bdjvarsGetNum (BDJVL_CONFIGUI_PORT);
  confui.conn = connInit (ROUTE_CONFIGUI);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "configui", BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  confui.optiondf = datafileAllocParse ("configui-opt", DFTYPE_KEY_VAL, tbuff,
      configuidfkeys, CONFUI_KEY_MAX, DATAFILE_NO_LOOKUP);
  confui.options = datafileGetList (confui.optiondf);
  if (confui.options == NULL) {
    confui.options = nlistAlloc ("configui-opt", LIST_ORDERED, free);

    nlistSetNum (confui.options, CONFUI_POSITION_X, -1);
    nlistSetNum (confui.options, CONFUI_POSITION_Y, -1);
    nlistSetNum (confui.options, CONFUI_SIZE_X, 1200);
    nlistSetNum (confui.options, CONFUI_SIZE_Y, 600);
  }

  /* register these after calling the sub-window initialization */
  /* then these will be run last, after the other closing callbacks */
  progstateSetCallback (confui.progstate, STATE_STOPPING,
      confuiStoppingCallback, &confui);
  progstateSetCallback (confui.progstate, STATE_STOP_WAIT,
      confuiStopWaitCallback, &confui);
  progstateSetCallback (confui.progstate, STATE_CLOSING,
      confuiClosingCallback, &confui);

  uiUIInitialize ();
  uifont = bdjoptGetStr (OPT_MP_UIFONT);
  uiSetUIFont (uifont);

  confuiBuildUI (&confui);
  osuiFinalize ();

  sockhMainLoop (listenPort, confuiProcessMsg, confuiMainLoop, &confui);
  connFree (confui.conn);
  progstateFree (confui.progstate);
  orgoptFree (orgopt);

  logProcEnd (LOG_PROC, "configui", "");
  logEnd ();
  return status;
}

/* internal routines */

static bool
confuiStoppingCallback (void *udata, programstate_t programState)
{
  configui_t    * confui = udata;
  int           x, y, ws;
  char          fn [MAXPATHLEN];

  logProcBegin (LOG_PROC, "confuiStoppingCallback");

  confuiPopulateOptions (confui);
  bdjoptSave ();
  for (confuiident_t i = 0; i < CONFUI_ID_TABLE_MAX; ++i) {
    confuiTableSave (&confui->gui, i);
  }

  uiWindowGetSize (&confui->gui.window, &x, &y);
  nlistSetNum (confui->options, CONFUI_SIZE_X, x);
  nlistSetNum (confui->options, CONFUI_SIZE_Y, y);
  uiWindowGetPosition (&confui->gui.window, &x, &y, &ws);
  nlistSetNum (confui->options, CONFUI_POSITION_X, x);
  nlistSetNum (confui->options, CONFUI_POSITION_Y, y);

  pathbldMakePath (fn, sizeof (fn),
      "configui", BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("configui", fn, configuidfkeys,
      CONFUI_KEY_MAX, confui->options);

  pathbldMakePath (fn, sizeof (fn),
      "ds-songfilter", BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("ds-songfilter", fn, filterdisplaydfkeys,
      FILTER_DISP_MAX, confui->filterDisplaySel);

  connDisconnect (confui->conn, ROUTE_STARTERUI);

  logProcEnd (LOG_PROC, "confuiStoppingCallback", "");
  return STATE_FINISHED;
}

static bool
confuiStopWaitCallback (void *udata, programstate_t programState)
{
  configui_t  * confui = udata;
  bool        rc;

  rc = connWaitClosed (confui->conn, &confui->stopwaitcount);
  return rc;
}

static bool
confuiClosingCallback (void *udata, programstate_t programState)
{
  configui_t    *confui = udata;

  logProcBegin (LOG_PROC, "confuiClosingCallback");

  uiCloseWindow (&confui->gui.window);

  for (int i = CONFUI_BEGIN + 1; i < CONFUI_COMBOBOX_MAX; ++i) {
    uiDropDownFree (confui->gui.uiitem [i].dropdown);
  }

  for (int i = CONFUI_COMBOBOX_MAX + 1; i < CONFUI_ENTRY_MAX; ++i) {
    uiEntryFree (confui->gui.uiitem [i].entry);
  }

  for (int i = CONFUI_ENTRY_MAX + 1; i < CONFUI_SPINBOX_MAX; ++i) {
    uiSpinboxTextFree (confui->gui.uiitem [i].spinbox);
    /* the mq and ui-theme share the list */
    if (i == CONFUI_SPINBOX_UI_THEME) {
      continue;
    }
    if (confui->gui.uiitem [i].displist != NULL) {
      nlistFree (confui->gui.uiitem [i].displist);
    }
    if (confui->gui.uiitem [i].sbkeylist != NULL) {
      nlistFree (confui->gui.uiitem [i].sbkeylist);
    }
  }

  for (int i = CONFUI_SPINBOX_MAX + 1; i < CONFUI_SWITCH_MAX; ++i) {
    uiSwitchFree (confui->gui.uiitem [i].uiswitch);
  }

  for (int i = CONFUI_SWITCH_MAX + 1; i < CONFUI_ITEM_MAX; ++i) {
    if (confui->gui.uiitem [i].uri != NULL) {
      free (confui->gui.uiitem [i].uri);
    }
  }

  if (confui->gui.dispselduallist != NULL) {
    uiduallistFree (confui->gui.dispselduallist);
  }
  if (confui->filterDisplayDf != NULL) {
    datafileFree (confui->filterDisplayDf);
  }
  if (confui->filterLookup != NULL) {
    nlistFree (confui->filterLookup);
  }
  dispselFree (confui->gui.dispsel);
  if (confui->gui.localip != NULL) {
    free (confui->gui.localip);
  }
  if (confui->optiondf != NULL) {
    datafileFree (confui->optiondf);
  } else if (confui->options != NULL) {
    nlistFree (confui->options);
  }
  if (confui->gui.nbtabid != NULL) {
    uiutilsNotebookIDFree (confui->gui.nbtabid);
  }
  if (confui->gui.listingtaglist != NULL) {
    slistFree (confui->gui.listingtaglist);
  }
  if (confui->gui.edittaglist != NULL) {
    slistFree (confui->gui.edittaglist);
  }

  bdj4shutdown (ROUTE_CONFIGUI, NULL);

  uiCleanup ();

  logProcEnd (LOG_PROC, "confuiClosingCallback", "");
  return STATE_FINISHED;
}

static void
confuiBuildUI (configui_t *confui)
{
  UIWidget      menubar;
  UIWidget      hbox;
  UIWidget      uiwidget;
  char          imgbuff [MAXPATHLEN];
  char          tbuff [MAXPATHLEN];
  int           x, y;

  logProcBegin (LOG_PROC, "confuiBuildUI");

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_config", ".svg", PATHBLD_MP_IMGDIR);
  /* CONTEXT: configuration: configuration user interface window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Configuration"),
      bdjoptGetStr (OPT_P_PROFILENAME));
  uiutilsUICallbackInit (&confui->gui.closecb, confuiCloseWin, confui);
  uiCreateMainWindow (&confui->gui.window, &confui->gui.closecb, tbuff, imgbuff);

  uiCreateVertBox (&confui->gui.vbox);
  uiWidgetExpandHoriz (&confui->gui.vbox);
  uiWidgetExpandVert (&confui->gui.vbox);
  uiWidgetSetAllMargins (&confui->gui.vbox, uiBaseMarginSz * 2);
  uiBoxPackInWindow (&confui->gui.window, &confui->gui.vbox);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&confui->gui.vbox, &hbox);

  uiCreateLabel (&uiwidget, "");
  uiWidgetSetSizeRequest (&uiwidget, 25, 25);
  uiWidgetSetMarginStart (&uiwidget, uiBaseMarginSz * 3);
  uiLabelSetBackgroundColor (&uiwidget, bdjoptGetStr (OPT_P_UI_PROFILE_COL));
  uiBoxPackEnd (&hbox, &uiwidget);

  uiCreateLabel (&uiwidget, "");
  uiLabelSetColor (&uiwidget, bdjoptGetStr (OPT_P_UI_ERROR_COL));
  uiBoxPackEnd (&hbox, &uiwidget);
  uiutilsUIWidgetCopy (&confui->gui.statusMsg, &uiwidget);

  uiCreateMenubar (&menubar);
  uiBoxPackStart (&hbox, &menubar);

  uiCreateNotebook (&confui->gui.notebook);
  uiNotebookTabPositionLeft (&confui->gui.notebook);
  uiBoxPackStartExpand (&confui->gui.vbox, &confui->gui.notebook);

  confuiBuildUIGeneral (&confui->gui);
  confuiBuildUIPlayer (confui);
  confuiBuildUIMarquee (confui);
  confuiBuildUIUserInterface (confui);
  confuiBuildUIDispSettings (confui);
  confuiBuildUIFilterDisplay (confui);
  confuiBuildUIOrganization (confui);
  confuiBuildUIEditDances (&confui->gui);
  confuiBuildUIEditRatings (&confui->gui);
  confuiBuildUIEditStatus (&confui->gui);
  confuiBuildUIEditLevels (&confui->gui);
  confuiBuildUIEditGenres (&confui->gui);
  confuiBuildUIMobileRemoteControl (confui);
  confuiBuildUIMobileMarquee (confui);
  confuiBuildUIDebugOptions (confui);

  uiutilsUICallbackLongInit (&confui->gui.nbcb, confuiSwitchTable, &confui->gui);
  uiNotebookSetCallback (&confui->gui.notebook, &confui->gui.nbcb);

  x = nlistGetNum (confui->options, CONFUI_SIZE_X);
  y = nlistGetNum (confui->options, CONFUI_SIZE_Y);
  uiWindowSetDefaultSize (&confui->gui.window, x, y);

  uiWidgetShowAll (&confui->gui.window);

  x = nlistGetNum (confui->options, CONFUI_POSITION_X);
  y = nlistGetNum (confui->options, CONFUI_POSITION_Y);
  uiWindowMove (&confui->gui.window, x, y, -1);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_config", ".png", PATHBLD_MP_IMGDIR);
  osuiSetIcon (imgbuff);

  logProcEnd (LOG_PROC, "confuiBuildUI", "");
}

static void
confuiBuildUIPlayer (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      sg;
  UIWidget      sgB;
  char          *mqnames [MUSICQ_PB_MAX];

  /* CONTEXT: configuration: The name of the music queue */
  mqnames [MUSICQ_PB_A] = _("Queue A Name");
  /* CONTEXT: configuration: The name of the music queue */
  mqnames [MUSICQ_PB_B] = _("Queue B Name");

  logProcBegin (LOG_PROC, "confuiBuildUIPlayer");
  uiCreateVertBox (&vbox);

  /* player options */
  confuiMakeNotebookTab (&vbox, &confui->gui,
      /* CONTEXT: configuration: options associated with the player */
      _("Player Options"), CONFUI_ID_NONE);
  uiCreateSizeGroupHoriz (&sg);
  uiCreateSizeGroupHoriz (&sgB);

  /* CONTEXT: configuration: which player interface to use */
  confuiMakeItemSpinboxText (&confui->gui, &vbox, &sg, NULL, _("Player"),
      CONFUI_SPINBOX_PLAYER, OPT_M_PLAYER_INTFC,
      CONFUI_OUT_STR, confui->gui.uiitem [CONFUI_SPINBOX_PLAYER].listidx, NULL);

  /* CONTEXT: configuration: which audio interface to use */
  confuiMakeItemSpinboxText (&confui->gui, &vbox, &sg, NULL, _("Audio"),
      CONFUI_SPINBOX_VOL_INTFC, OPT_M_VOLUME_INTFC,
      CONFUI_OUT_STR, confui->gui.uiitem [CONFUI_SPINBOX_VOL_INTFC].listidx, NULL);

  /* CONTEXT: configuration: which audio sink (output) to use */
  confuiMakeItemSpinboxText (&confui->gui, &vbox, &sg, NULL, _("Audio Output"),
      CONFUI_SPINBOX_AUDIO_OUTPUT, OPT_MP_AUDIOSINK,
      CONFUI_OUT_STR, confui->gui.uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].listidx, NULL);

  /* CONTEXT: configuration: the volume used when starting the player */
  confuiMakeItemSpinboxNum (&confui->gui, &vbox, &sg, &sgB, _("Default Volume"),
      CONFUI_WIDGET_DEFAULT_VOL, OPT_P_DEFAULTVOLUME,
      10, 100, bdjoptGetNum (OPT_P_DEFAULTVOLUME), NULL);

  /* CONTEXT: configuration: the amount of time to do a volume fade-in when playing a song */
  confuiMakeItemSpinboxDouble (&confui->gui, &vbox, &sg, &sgB, _("Fade In Time"),
      CONFUI_WIDGET_FADE_IN_TIME, OPT_P_FADEINTIME,
      0.0, 2.0, (double) bdjoptGetNum (OPT_P_FADEINTIME) / 1000.0);

  /* CONTEXT: configuration: the amount of time to do a volume fade-out when playing a song */
  confuiMakeItemSpinboxDouble (&confui->gui, &vbox, &sg, &sgB, _("Fade Out Time"),
      CONFUI_WIDGET_FADE_OUT_TIME, OPT_P_FADEOUTTIME,
      0.0, 10.0, (double) bdjoptGetNum (OPT_P_FADEOUTTIME) / 1000.0);

  /* CONTEXT: configuration: the amount of time to wait inbetween songs */
  confuiMakeItemSpinboxDouble (&confui->gui, &vbox, &sg, &sgB, _("Gap Between Songs"),
      CONFUI_WIDGET_GAP, OPT_P_GAP,
      0.0, 60.0, (double) bdjoptGetNum (OPT_P_GAP) / 1000.0);

  /* CONTEXT: configuration: the maximum amount of time to play a song */
  confuiMakeItemSpinboxTime (&confui->gui, &vbox, &sg, &sgB, _("Maximum Play Time"),
      CONFUI_SPINBOX_MAX_PLAY_TIME, OPT_P_MAXPLAYTIME,
      bdjoptGetNum (OPT_P_MAXPLAYTIME));

  /* CONTEXT: configuration: the &sgB, of the music queue to display */
  confuiMakeItemSpinboxNum (&confui->gui, &vbox, &sg, &sgB, _("Queue Length"),
      CONFUI_WIDGET_PL_QUEUE_LEN, OPT_G_PLAYERQLEN,
      20, 400, bdjoptGetNum (OPT_G_PLAYERQLEN), NULL);

  /* CONTEXT: configuration: The completion message displayed on the marquee when a playlist is finished */
  confuiMakeItemEntry (&confui->gui, &vbox, &sg, _("Completion Message"),
      CONFUI_ENTRY_COMPLETE_MSG, OPT_P_COMPLETE_MSG,
      bdjoptGetStr (OPT_P_COMPLETE_MSG));

  for (musicqidx_t i = 0; i < MUSICQ_PB_MAX; ++i) {
    confuiMakeItemEntry (&confui->gui, &vbox, &sg, mqnames [i],
        CONFUI_ENTRY_QUEUE_NM_A + i, OPT_P_QUEUE_NAME_A + i,
        bdjoptGetStr (OPT_P_QUEUE_NAME_A + i));
  }
  logProcEnd (LOG_PROC, "confuiBuildUIPlayer", "");
}

static void
confuiBuildUIMarquee (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIMarquee");
  uiCreateVertBox (&vbox);

  /* marquee options */
  confuiMakeNotebookTab (&vbox, &confui->gui,
      /* CONTEXT: configuration: options associated with the marquee */
      _("Marquee Options"), CONFUI_ID_NONE);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: The theme to use for the marquee display */
  confuiMakeItemSpinboxText (&confui->gui, &vbox, &sg, NULL, _("Marquee Theme"),
      CONFUI_SPINBOX_MQ_THEME, OPT_MP_MQ_THEME,
      CONFUI_OUT_STR, confui->gui.uiitem [CONFUI_SPINBOX_MQ_THEME].listidx, NULL);

  /* CONTEXT: configuration: The font to use for the marquee display */
  confuiMakeItemFontButton (&confui->gui, &vbox, &sg, _("Marquee Font"),
      CONFUI_WIDGET_MQ_FONT, OPT_MP_MQFONT,
      bdjoptGetStr (OPT_MP_MQFONT));

  /* CONTEXT: configuration: the length of the queue displayed on the marquee */
  confuiMakeItemSpinboxNum (&confui->gui, &vbox, &sg, NULL, _("Queue Length"),
      CONFUI_WIDGET_MQ_QUEUE_LEN, OPT_P_MQQLEN,
      1, 20, bdjoptGetNum (OPT_P_MQQLEN), NULL);

  /* CONTEXT: configuration: marquee: show the song information (artist/title) on the marquee */
  confuiMakeItemSwitch (&confui->gui, &vbox, &sg, _("Show Song Information"),
      CONFUI_SWITCH_MQ_SHOW_SONG_INFO, OPT_P_MQ_SHOW_INFO,
      bdjoptGetNum (OPT_P_MQ_SHOW_INFO), NULL);

  /* CONTEXT: configuration: marquee: the accent color used for the marquee */
  confuiMakeItemColorButton (&confui->gui, &vbox, &sg, _("Accent Colour"),
      CONFUI_WIDGET_MQ_ACCENT_COLOR, OPT_P_MQ_ACCENT_COL,
      bdjoptGetStr (OPT_P_MQ_ACCENT_COL));

  /* CONTEXT: configuration: marquee: minimize the marquee when the player is started */
  confuiMakeItemSwitch (&confui->gui, &vbox, &sg, _("Hide Marquee on Start"),
      CONFUI_SWITCH_MQ_HIDE_ON_START, OPT_P_HIDE_MARQUEE_ON_START,
      bdjoptGetNum (OPT_P_HIDE_MARQUEE_ON_START), NULL);
  logProcEnd (LOG_PROC, "confuiBuildUIMarquee", "");
}

static void
confuiBuildUIUserInterface (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      sg;
  char          *tstr;

  logProcBegin (LOG_PROC, "confuiBuildUIUserInterface");
  uiCreateVertBox (&vbox);

  /* user interface */
  confuiMakeNotebookTab (&vbox, &confui->gui,
      /* CONTEXT: configuration: options associated with the user interface */
      _("User Interface"), CONFUI_ID_NONE);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: the theme to use for the user interface */
  confuiMakeItemSpinboxText (&confui->gui, &vbox, &sg, NULL, _("Theme"),
      CONFUI_SPINBOX_UI_THEME, OPT_MP_UI_THEME, CONFUI_OUT_STR,
      confui->gui.uiitem [CONFUI_SPINBOX_UI_THEME].listidx, NULL);

  tstr = bdjoptGetStr (OPT_MP_UIFONT);
  if (! *tstr) {
    tstr = sysvarsGetStr (SV_FONT_DEFAULT);
  }
  /* CONTEXT: configuration: the font to use for the user interface */
  confuiMakeItemFontButton (&confui->gui, &vbox, &sg, _("Font"),
      CONFUI_WIDGET_UI_FONT, OPT_MP_UIFONT, tstr);

  tstr = bdjoptGetStr (OPT_MP_LISTING_FONT);
  if (! *tstr) {
    tstr = sysvarsGetStr (SV_FONT_DEFAULT);
  }
  /* CONTEXT: configuration: the font to use for the queues and song lists */
  confuiMakeItemFontButton (&confui->gui, &vbox, &sg, _("Listing Font"),
      CONFUI_WIDGET_UI_LISTING_FONT, OPT_MP_LISTING_FONT, tstr);

  /* CONTEXT: configuration: the accent color to use for the user interface */
  confuiMakeItemColorButton (&confui->gui, &vbox, &sg, _("Accent Colour"),
      CONFUI_WIDGET_UI_ACCENT_COLOR, OPT_P_UI_ACCENT_COL,
      bdjoptGetStr (OPT_P_UI_ACCENT_COL));
  logProcEnd (LOG_PROC, "confuiBuildUIUserInterface", "");
}

static void
confuiBuildUIDispSettings (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      uiwidget;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIDispSettings");
  uiutilsUIWidgetInit (&uiwidget);
  uiutilsUIWidgetInit (&sg);

  uiCreateVertBox (&vbox);

  /* display settings */
  confuiMakeNotebookTab (&vbox, &confui->gui,
      /* CONTEXT: configuration: change which fields are displayed in different contexts */
      _("Display Settings"), CONFUI_ID_DISP_SEL_LIST);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: display settings: which set of display settings to update */
  confuiMakeItemSpinboxText (&confui->gui, &vbox, &sg, NULL, _("Display"),
      CONFUI_SPINBOX_DISP_SEL, -1, CONFUI_OUT_NUM,
      confui->gui.uiitem [CONFUI_SPINBOX_DISP_SEL].listidx,
      confuiDispSettingChg);

  confui->gui.tables [CONFUI_ID_DISP_SEL_LIST].flags = CONFUI_TABLE_NONE;
  confui->gui.tables [CONFUI_ID_DISP_SEL_TABLE].flags = CONFUI_TABLE_NONE;

  confui->gui.dispselduallist = uiCreateDualList (&vbox,
      DUALLIST_FLAGS_NONE, NULL, NULL);
  logProcEnd (LOG_PROC, "confuiBuildUIDispSettings", "");
}

static void
confuiBuildUIFilterDisplay (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      sg;
  nlistidx_t    val;

  logProcBegin (LOG_PROC, "confuiBuildUIFilterDisplay");
  uiCreateVertBox (&vbox);

  /* filter display */
  confuiMakeNotebookTab (&vbox, &confui->gui,
      /* CONTEXT: configuration: song filter display settings */
      _("Filter Display"), CONFUI_ID_FILTER);
  uiCreateSizeGroupHoriz (&sg);

  val = nlistGetNum (confui->filterDisplaySel, FILTER_DISP_GENRE);
  /* CONTEXT: configuration: filter display: checkbox: genre */
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, _("Genre"),
      CONFUI_WIDGET_FILTER_GENRE, -1, val);
  confui->gui.uiitem [CONFUI_WIDGET_FILTER_GENRE].outtype = CONFUI_OUT_CB;

  val = nlistGetNum (confui->filterDisplaySel, FILTER_DISP_DANCELEVEL);
  /* CONTEXT: configuration: filter display: checkbox: dance level */
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, _("Dance Level"),
      CONFUI_WIDGET_FILTER_DANCELEVEL, -1, val);
  confui->gui.uiitem [CONFUI_WIDGET_FILTER_DANCELEVEL].outtype = CONFUI_OUT_CB;

  val = nlistGetNum (confui->filterDisplaySel, FILTER_DISP_STATUS);
  /* CONTEXT: configuration: filter display: checkbox: status */
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, _("Status"),
      CONFUI_WIDGET_FILTER_STATUS, -1, val);
  confui->gui.uiitem [CONFUI_WIDGET_FILTER_STATUS].outtype = CONFUI_OUT_CB;

  val = nlistGetNum (confui->filterDisplaySel, FILTER_DISP_FAVORITE);
  /* CONTEXT: configuration: filter display: checkbox: favorite selection */
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, _("Favorite"),
      CONFUI_WIDGET_FILTER_FAVORITE, -1, val);
  confui->gui.uiitem [CONFUI_WIDGET_FILTER_FAVORITE].outtype = CONFUI_OUT_CB;

  val = nlistGetNum (confui->filterDisplaySel, FILTER_DISP_STATUSPLAYABLE);
  /* CONTEXT: configuration: filter display: checkbox: status is playable */
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, _("Playable Status"),
      CONFUI_SWITCH_FILTER_STATUS_PLAYABLE, -1, val);
  confui->gui.uiitem [CONFUI_SWITCH_FILTER_STATUS_PLAYABLE].outtype = CONFUI_OUT_CB;
  logProcEnd (LOG_PROC, "confuiBuildUIFilterDisplay", "");
}

static void
confuiBuildUIOrganization (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIOrganization");
  uiCreateVertBox (&vbox);

  /* organization */
  confuiMakeNotebookTab (&vbox, &confui->gui,
      /* CONTEXT: configuration: options associated with how audio files are organized */
      _("Organisation"), CONFUI_ID_ORGANIZATION);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: the audio file organization path */
  confuiMakeItemCombobox (&confui->gui, &vbox, &sg, _("Organisation Path"),
      CONFUI_COMBOBOX_AO_PATHFMT, OPT_G_AO_PATHFMT,
      confuiOrgPathSelect, bdjoptGetStr (OPT_G_AO_PATHFMT));
  /* CONTEXT: configuration: examples displayed for the audio file organization path */
  confuiMakeItemLabelDisp (&confui->gui, &vbox, &sg, _("Examples"),
      CONFUI_WIDGET_AO_EXAMPLE_1, -1);
  confuiMakeItemLabelDisp (&confui->gui, &vbox, &sg, "",
      CONFUI_WIDGET_AO_EXAMPLE_2, -1);
  confuiMakeItemLabelDisp (&confui->gui, &vbox, &sg, "",
      CONFUI_WIDGET_AO_EXAMPLE_3, -1);
  confuiMakeItemLabelDisp (&confui->gui, &vbox, &sg, "",
      CONFUI_WIDGET_AO_EXAMPLE_4, -1);

  /* CONTEXT: configuration: checkbox: is automatic organization enabled */
  confuiMakeItemSwitch (&confui->gui, &vbox, &sg, _("Auto Organise"),
      CONFUI_SWITCH_AUTO_ORGANIZE, OPT_G_AUTOORGANIZE,
      bdjoptGetNum (OPT_G_AUTOORGANIZE), NULL);
  logProcEnd (LOG_PROC, "confuiBuildUIOrganization", "");
}

static void
confuiBuildUIMobileRemoteControl (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIMobileRemoteControl");
  uiCreateVertBox (&vbox);

  /* mobile remote control */
  confuiMakeNotebookTab (&vbox, &confui->gui,
      /* CONTEXT: configuration: options associated with mobile remote control */
      _("Mobile Remote Control"), CONFUI_ID_REM_CONTROL);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: remote control: checkbox: enable/disable */
  confuiMakeItemSwitch (&confui->gui, &vbox, &sg, _("Enable Remote Control"),
      CONFUI_SWITCH_RC_ENABLE, OPT_P_REMOTECONTROL,
      bdjoptGetNum (OPT_P_REMOTECONTROL),
      confuiRemctrlChg);

  /* CONTEXT: configuration: remote control: the HTML template to use */
  confuiMakeItemSpinboxText (&confui->gui, &vbox, &sg, NULL, _("HTML Template"),
      CONFUI_SPINBOX_RC_HTML_TEMPLATE, OPT_G_REMCONTROLHTML,
      CONFUI_OUT_STR, confui->gui.uiitem [CONFUI_SPINBOX_RC_HTML_TEMPLATE].listidx, NULL);

  /* CONTEXT: configuration: remote control: the user ID for sign-on to remote control */
  confuiMakeItemEntry (&confui->gui, &vbox, &sg, _("User ID"),
      CONFUI_ENTRY_RC_USER_ID,  OPT_P_REMCONTROLUSER,
      bdjoptGetStr (OPT_P_REMCONTROLUSER));

  /* CONTEXT: configuration: remote control: the password for sign-on to remote control */
  confuiMakeItemEntry (&confui->gui, &vbox, &sg, _("Password"),
      CONFUI_ENTRY_RC_PASS, OPT_P_REMCONTROLPASS,
      bdjoptGetStr (OPT_P_REMCONTROLPASS));

  /* CONTEXT: configuration: remote control: the port to use for remote control */
  confuiMakeItemSpinboxNum (&confui->gui, &vbox, &sg, NULL, _("Port"),
      CONFUI_WIDGET_RC_PORT, OPT_P_REMCONTROLPORT,
      8000, 30000, bdjoptGetNum (OPT_P_REMCONTROLPORT),
      confuiRemctrlPortChg);

  /* CONTEXT: configuration: remote control: the link to display the QR code for remote control */
  confuiMakeItemLink (&confui->gui, &vbox, &sg, _("QR Code"),
      CONFUI_WIDGET_RC_QR_CODE, "");
  logProcEnd (LOG_PROC, "confuiBuildUIMobileRemoteControl", "");
}

static void
confuiBuildUIMobileMarquee (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIMobileMarquee");
  uiCreateVertBox (&vbox);

  /* mobile marquee */
  confuiMakeNotebookTab (&vbox, &confui->gui,
      /* CONTEXT: configuration: options associated with the mobile marquee */
      _("Mobile Marquee"), CONFUI_ID_MOBILE_MQ);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: set mobile marquee mode (off/local/internet) */
  confuiMakeItemSpinboxText (&confui->gui, &vbox, &sg, NULL, _("Mobile Marquee"),
      CONFUI_SPINBOX_MOBILE_MQ, OPT_P_MOBILEMARQUEE,
      CONFUI_OUT_NUM, bdjoptGetNum (OPT_P_MOBILEMARQUEE),
      confuiMobmqTypeChg);

  /* CONTEXT: configuration: the port to use for the mobile marquee */
  confuiMakeItemSpinboxNum (&confui->gui, &vbox, &sg, NULL, _("Port"),
      CONFUI_WIDGET_MMQ_PORT, OPT_P_MOBILEMQPORT,
      8000, 30000, bdjoptGetNum (OPT_P_MOBILEMQPORT),
      confuiMobmqPortChg);

  /* CONTEXT: configuration: the name to use for the mobile marquee internet mode */
  confuiMakeItemEntry (&confui->gui, &vbox, &sg, _("Name"),
      CONFUI_ENTRY_MM_NAME, OPT_P_MOBILEMQTAG,
      bdjoptGetStr (OPT_P_MOBILEMQTAG));
  uiEntrySetValidate (confui->gui.uiitem [CONFUI_ENTRY_MM_NAME].entry,
      confuiMobmqNameChg, &confui->gui, UIENTRY_IMMEDIATE);

  /* CONTEXT: configuration: the title to display on the mobile marquee */
  confuiMakeItemEntry (&confui->gui, &vbox, &sg, _("Title"),
      CONFUI_ENTRY_MM_TITLE, OPT_P_MOBILEMQTITLE,
      bdjoptGetStr (OPT_P_MOBILEMQTITLE));
  uiEntrySetValidate (confui->gui.uiitem [CONFUI_ENTRY_MM_TITLE].entry,
      confuiMobmqTitleChg, &confui->gui, UIENTRY_IMMEDIATE);

  /* CONTEXT: configuration: mobile marquee: the link to display the QR code for the mobile marquee */
  confuiMakeItemLink (&confui->gui, &vbox, &sg, _("QR Code"),
      CONFUI_WIDGET_MMQ_QR_CODE, "");
  logProcEnd (LOG_PROC, "confuiBuildUIMobileMarquee", "");
}

static void
confuiBuildUIDebugOptions (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      sg;
  nlistidx_t    val;

  logProcBegin (LOG_PROC, "confuiBuildUIDebugOptions");
  uiCreateVertBox (&vbox);

  /* debug options */
  confuiMakeNotebookTab (&vbox, &confui->gui,
      /* CONTEXT: configuration: debug options that can be turned on and off */
      _("Debug Options"), CONFUI_ID_NONE);
  uiCreateSizeGroupHoriz (&sg);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateVertBox (&vbox);
  uiBoxPackStart (&hbox, &vbox);

  val = bdjoptGetNum (OPT_G_DEBUGLVL);
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, "Important",
      CONFUI_WIDGET_DEBUG_1, -1,
      (val & 1));
  confui->gui.uiitem [CONFUI_WIDGET_DEBUG_1].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, "Basic",
      CONFUI_WIDGET_DEBUG_2, -1,
      (val & 2));
  confui->gui.uiitem [CONFUI_WIDGET_DEBUG_2].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, "Messages",
      CONFUI_WIDGET_DEBUG_4, -1,
      (val & 4));
  confui->gui.uiitem [CONFUI_WIDGET_DEBUG_4].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, "Main",
      CONFUI_WIDGET_DEBUG_8, -1,
      (val & 8));
  confui->gui.uiitem [CONFUI_WIDGET_DEBUG_8].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, "Actions",
      CONFUI_WIDGET_DEBUG_16, -1,
      (val & 16));
  confui->gui.uiitem [CONFUI_WIDGET_DEBUG_16].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, "List",
      CONFUI_WIDGET_DEBUG_32, -1,
      (val & 32));
  confui->gui.uiitem [CONFUI_WIDGET_DEBUG_32].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, "Song Selection",
      CONFUI_WIDGET_DEBUG_64, -1,
      (val & 64));
  confui->gui.uiitem [CONFUI_WIDGET_DEBUG_64].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, "Dance Selection",
      CONFUI_WIDGET_DEBUG_128, -1,
      (val & 128));
  confui->gui.uiitem [CONFUI_WIDGET_DEBUG_128].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, "Volume",
      CONFUI_WIDGET_DEBUG_256, -1,
      (val & 256));
  confui->gui.uiitem [CONFUI_WIDGET_DEBUG_256].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, "Socket",
      CONFUI_WIDGET_DEBUG_512, -1,
      (val & 512));

  uiCreateVertBox (&vbox);
  uiBoxPackStart (&hbox, &vbox);

  uiCreateSizeGroupHoriz (&sg);

  confui->gui.uiitem [CONFUI_WIDGET_DEBUG_512].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, "Database",
      CONFUI_WIDGET_DEBUG_1024, -1,
      (val & 1024));
  confui->gui.uiitem [CONFUI_WIDGET_DEBUG_1024].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, "Random Access File",
      CONFUI_WIDGET_DEBUG_2048, -1,
      (val & 2048));
  confui->gui.uiitem [CONFUI_WIDGET_DEBUG_2048].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, "Procedures",
      CONFUI_WIDGET_DEBUG_4096, -1,
      (val & 4096));
  confui->gui.uiitem [CONFUI_WIDGET_DEBUG_4096].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, "Player",
      CONFUI_WIDGET_DEBUG_8192, -1,
      (val & 8192));
  confui->gui.uiitem [CONFUI_WIDGET_DEBUG_8192].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, "Datafile",
      CONFUI_WIDGET_DEBUG_16384, -1,
      (val & 16384));
  confui->gui.uiitem [CONFUI_WIDGET_DEBUG_16384].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, "Process",
      CONFUI_WIDGET_DEBUG_32768, -1,
      (val & 32768));
  confui->gui.uiitem [CONFUI_WIDGET_DEBUG_32768].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, "Web Server",
      CONFUI_WIDGET_DEBUG_65536, -1,
      (val & 65536));
  confui->gui.uiitem [CONFUI_WIDGET_DEBUG_65536].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, "Web Client",
      CONFUI_WIDGET_DEBUG_131072, -1,
      (val & 131072));
  confui->gui.uiitem [CONFUI_WIDGET_DEBUG_131072].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, "Database Update",
      CONFUI_WIDGET_DEBUG_262144, -1,
      (val & 262144));
  confui->gui.uiitem [CONFUI_WIDGET_DEBUG_262144].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (&confui->gui, &vbox, &sg, "Program State",
      CONFUI_WIDGET_DEBUG_524288, -1,
      (val & 524288));
  confui->gui.uiitem [CONFUI_WIDGET_DEBUG_524288].outtype = CONFUI_OUT_DEBUG;
  logProcEnd (LOG_PROC, "confuiBuildUIDebugOptions", "");
}


static int
confuiMainLoop (void *tconfui)
{
  configui_t    *confui = tconfui;
  int           stop = SOCKH_CONTINUE;

  if (! stop) {
    uiUIProcessEvents ();
  }

  if (! progstateIsRunning (confui->progstate)) {
    progstateProcess (confui->progstate);
    if (progstateCurrState (confui->progstate) == STATE_CLOSED) {
      stop = SOCKH_STOP;
    }
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      progstateShutdownProcess (confui->progstate);
      gKillReceived = false;
    }
    return stop;
  }

  connProcessUnconnected (confui->conn);

  for (int i = CONFUI_COMBOBOX_MAX + 1; i < CONFUI_ENTRY_MAX; ++i) {
    uiEntryValidate (confui->gui.uiitem [i].entry, false);
  }

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (confui->progstate);
    gKillReceived = false;
  }
  return stop;
}

static bool
confuiHandshakeCallback (void *udata, programstate_t programState)
{
  configui_t   *confui = udata;

  logProcBegin (LOG_PROC, "confuiHandshakeCallback");
  connProcessUnconnected (confui->conn);

  progstateLogTime (confui->progstate, "time-to-start-gui");
  logProcEnd (LOG_PROC, "confuiHandshakeCallback", "");
  return STATE_FINISHED;
}

static int
confuiProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  configui_t       *confui = udata;

  logProcBegin (LOG_PROC, "confuiProcessMsg");

  logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
      routefrom, msgRouteDebugText (routefrom),
      route, msgRouteDebugText (route), msg, msgDebugText (msg), args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_CONFIGUI: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (confui->conn, routefrom);
          break;
        }
        case MSG_SOCKET_CLOSE: {
          connDisconnect (confui->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = false;
          if (progstateCurrState (confui->progstate) <= STATE_RUNNING) {
            progstateShutdownProcess (confui->progstate);
          }
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

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  logProcEnd (LOG_PROC, "confuiProcessMsg", "");
  return gKillReceived;
}


static bool
confuiCloseWin (void *udata)
{
  configui_t   *confui = udata;

  logProcBegin (LOG_PROC, "confuiCloseWin");
  if (progstateCurrState (confui->progstate) <= STATE_RUNNING) {
    progstateShutdownProcess (confui->progstate);
    logMsg (LOG_DBG, LOG_MSGS, "got: close win request");
    logProcEnd (LOG_PROC, "confuiCloseWin", "not-done");
    return UICB_STOP;
  }

  logProcEnd (LOG_PROC, "confuiCloseWin", "");
  return UICB_STOP;
}

static void
confuiSigHandler (int sig)
{
  gKillReceived = true;
}

static void
confuiPopulateOptions (configui_t *confui)
{
  const char  *sval;
  ssize_t     nval;
  nlistidx_t  selidx;
  double      dval;
  confuibasetype_t basetype;
  confuiouttype_t outtype;
  char        tbuff [MAXPATHLEN];
  GdkRGBA     gcolor;
  long        debug = 0;
  bool        accentcolorchanged = false;
  bool        profilecolorchanged = false;
  bool        localechanged = false;
  bool        themechanged = false;

  logProcBegin (LOG_PROC, "confuiPopulateOptions");
  for (int i = 0; i < CONFUI_ITEM_MAX; ++i) {
    sval = "fail";
    nval = -1;
    dval = -1.0;

    basetype = confui->gui.uiitem [i].basetype;
    outtype = confui->gui.uiitem [i].outtype;

    switch (basetype) {
      case CONFUI_NONE: {
        break;
      }
      case CONFUI_ENTRY: {
        sval = uiEntryGetValue (confui->gui.uiitem [i].entry);
        break;
      }
      case CONFUI_SPINBOX_TEXT: {
        nval = uiSpinboxTextGetValue (confui->gui.uiitem [i].spinbox);
        if (outtype == CONFUI_OUT_STR) {
          if (confui->gui.uiitem [i].sbkeylist != NULL) {
            sval = nlistGetStr (confui->gui.uiitem [i].sbkeylist, nval);
          } else {
            sval = nlistGetStr (confui->gui.uiitem [i].displist, nval);
          }
        }
        break;
      }
      case CONFUI_SPINBOX_NUM: {
        nval = (ssize_t) uiSpinboxGetValue (&confui->gui.uiitem [i].uiwidget);
        break;
      }
      case CONFUI_SPINBOX_DOUBLE: {
        dval = uiSpinboxGetValue (&confui->gui.uiitem [i].uiwidget);
        nval = (ssize_t) (dval * 1000.0);
        outtype = CONFUI_OUT_NUM;
        break;
      }
      case CONFUI_SPINBOX_TIME: {
        nval = (ssize_t) uiSpinboxTimeGetValue (confui->gui.uiitem [i].spinbox);
        break;
      }
      case CONFUI_COLOR: {
        gtk_color_chooser_get_rgba (
            GTK_COLOR_CHOOSER (confui->gui.uiitem [i].widget), &gcolor);
        snprintf (tbuff, sizeof (tbuff), "#%02x%02x%02x",
            (int) round (gcolor.red * 255.0),
            (int) round (gcolor.green * 255.0),
            (int) round (gcolor.blue * 255.0));
        sval = tbuff;
        break;
      }
      case CONFUI_FONT: {
        sval = gtk_font_chooser_get_font (
            GTK_FONT_CHOOSER (confui->gui.uiitem [i].widget));
        break;
      }
      case CONFUI_SWITCH: {
        nval = uiSwitchGetValue (confui->gui.uiitem [i].uiswitch);
        break;
      }
      case CONFUI_CHECK_BUTTON: {
        nval = uiToggleButtonIsActive (&confui->gui.uiitem [i].uiwidget);
        break;
      }
      case CONFUI_COMBOBOX: {
        sval = slistGetDataByIdx (confui->gui.uiitem [i].displist,
            confui->gui.uiitem [i].listidx);
        outtype = CONFUI_OUT_STR;
        break;
      }
    }

    if (i == CONFUI_SPINBOX_AUDIO_OUTPUT) {
      uispinbox_t  *spinbox;

      spinbox = confui->gui.uiitem [i].spinbox;
      if (! uiSpinboxIsChanged (spinbox)) {
        continue;
      }
    }

    switch (outtype) {
      case CONFUI_OUT_NONE: {
        break;
      }
      case CONFUI_OUT_STR: {
        if (i == CONFUI_SPINBOX_LOCALE) {
          if (strcmp (sysvarsGetStr (SV_LOCALE), sval) != 0) {
            localechanged = true;
          }
        }
        if (i == CONFUI_WIDGET_UI_ACCENT_COLOR) {
          if (strcmp (bdjoptGetStr (confui->gui.uiitem [i].bdjoptIdx), sval) != 0) {
            accentcolorchanged = true;
          }
        }
        if (i == CONFUI_WIDGET_UI_PROFILE_COLOR) {
          if (strcmp (bdjoptGetStr (confui->gui.uiitem [i].bdjoptIdx), sval) != 0) {
            profilecolorchanged = true;
          }
        }
        if (i == CONFUI_SPINBOX_UI_THEME) {
          if (strcmp (bdjoptGetStr (confui->gui.uiitem [i].bdjoptIdx), sval) != 0) {
            themechanged = true;
          }
        }
        bdjoptSetStr (confui->gui.uiitem [i].bdjoptIdx, sval);
        break;
      }
      case CONFUI_OUT_NUM: {
        bdjoptSetNum (confui->gui.uiitem [i].bdjoptIdx, nval);
        break;
      }
      case CONFUI_OUT_DOUBLE: {
        bdjoptSetNum (confui->gui.uiitem [i].bdjoptIdx, dval);
        break;
      }
      case CONFUI_OUT_BOOL: {
        bdjoptSetNum (confui->gui.uiitem [i].bdjoptIdx, nval);
        break;
      }
      case CONFUI_OUT_CB: {
        nlistSetNum (confui->filterDisplaySel,
            nlistGetNum (confui->filterLookup, i), nval);
        break;
      }
      case CONFUI_OUT_DEBUG: {
        if (nval) {
          long  idx;
          long  bitval;

          idx = i - CONFUI_WIDGET_DEBUG_1;
          bitval = 1 << idx;
          debug |= bitval;
        }
        break;
      }
    } /* out type */

    if (i == CONFUI_SPINBOX_LOCALE &&
        localechanged) {
      sysvarsSetStr (SV_LOCALE, sval);
      snprintf (tbuff, sizeof (tbuff), "%.2s", sval);
      sysvarsSetStr (SV_LOCALE_SHORT, tbuff);
      pathbldMakePath (tbuff, sizeof (tbuff),
          "locale", BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
      fileopDelete (tbuff);

      /* if the set locale does not match the system or default locale */
      /* save it in the locale file */
      if (strcmp (sval, sysvarsGetStr (SV_LOCALE_SYSTEM)) != 0 &&
          strcmp (sval, "en_GB") != 0) {
        FILE    *fh;

        fh = fopen (tbuff, "w");
        fprintf (fh, "%s\n", sval);
        fclose (fh);
      }
    }

    if (i == CONFUI_ENTRY_MUSIC_DIR) {
      strlcpy (tbuff, bdjoptGetStr (confui->gui.uiitem [i].bdjoptIdx), sizeof (tbuff));
      pathNormPath (tbuff, sizeof (tbuff));
      bdjoptSetStr (confui->gui.uiitem [i].bdjoptIdx, tbuff);
    }

    if (i == CONFUI_SPINBOX_UI_THEME && themechanged) {
      FILE    *fh;

      sval = bdjoptGetStr (confui->gui.uiitem [i].bdjoptIdx);
      pathbldMakePath (tbuff, sizeof (tbuff),
          "theme", BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
      /* if the theme name is the same as the current default theme */
      /* don't write out the theme file.  want to use the default */
      if (strcmp (sval, sysvarsGetStr (SV_THEME_DEFAULT)) != 0) {
        fh = fopen (tbuff, "w");
        if (sval != NULL) {
          fprintf (fh, "%s\n", sval);
        }
        fclose (fh);
      } else {
        fileopDelete (tbuff);
      }
    }

    if (i == CONFUI_WIDGET_UI_ACCENT_COLOR &&
        accentcolorchanged) {
      templateImageCopy (sval);
    }

    if (i == CONFUI_SPINBOX_RC_HTML_TEMPLATE) {
      sval = bdjoptGetStr (confui->gui.uiitem [i].bdjoptIdx);
      templateHttpCopy (sval, "bdj4remote.html");
    }
  } /* for each item */

  selidx = uiSpinboxTextGetValue (
      confui->gui.uiitem [CONFUI_SPINBOX_DISP_SEL].spinbox);
  confuiDispSaveTable (&confui->gui, selidx);

  bdjoptSetNum (OPT_G_DEBUGLVL, debug);
  logProcEnd (LOG_PROC, "confuiPopulateOptions", "");
}


/* display settings */

static bool
confuiDispSettingChg (void *udata)
{
  confuigui_t *gui = udata;
  int         oselidx;
  int         nselidx;

  logProcBegin (LOG_PROC, "confuiDispSettingChg");


  oselidx = gui->uiitem [CONFUI_SPINBOX_DISP_SEL].listidx;
  nselidx = uiSpinboxTextGetValue (
      gui->uiitem [CONFUI_SPINBOX_DISP_SEL].spinbox);
  gui->uiitem [CONFUI_SPINBOX_DISP_SEL].listidx = nselidx;

  confuiDispSaveTable (gui, oselidx);
  /* be sure to create the listing first */
  confuiCreateTagListingDisp (gui);
  confuiCreateTagSelectedDisp (gui);
  logProcEnd (LOG_PROC, "confuiDispSettingChg", "");
  return UICB_CONT;
}

static void
confuiDispSaveTable (confuigui_t *gui, int selidx)
{
  slist_t       *tlist;
  slist_t       *nlist;
  slistidx_t    val;
  slistidx_t    iteridx;
  const char    *tstr;

  logProcBegin (LOG_PROC, "confuiDispSaveTable");

  if (! uiduallistIsChanged (gui->dispselduallist)) {
    logProcEnd (LOG_PROC, "confuiDispSaveTable", "not-changed");
    return;
  }

  nlist = slistAlloc ("dispsel-save-tmp", LIST_UNORDERED, NULL);
  tlist = uiduallistGetList (gui->dispselduallist);
  slistStartIterator (tlist, &iteridx);
  while ((val = slistIterateValueNum (tlist, &iteridx)) >= 0) {
    tstr = tagdefs [val].tag;
    slistSetNum (nlist, tstr, 0);
  }

  dispselSave (gui->dispsel, selidx, nlist);

  slistFree (tlist);
  slistFree (nlist);
  logProcEnd (LOG_PROC, "confuiDispSaveTable", "");
}

/* misc */

static void
confuiLoadTagList (configui_t *confui)
{
  slist_t       *llist = NULL;
  slist_t       *elist = NULL;

  logProcBegin (LOG_PROC, "confuiLoadTagList");

  llist = slistAlloc ("cu-list-tag-list", LIST_ORDERED, NULL);
  elist = slistAlloc ("cu-edit-tag-list", LIST_ORDERED, NULL);

  for (tagdefkey_t i = 0; i < TAG_KEY_MAX; ++i) {
    if (tagdefs [i].listingDisplay) {
      slistSetNum (llist, tagdefs [i].displayname, i);
    }
    if (tagdefs [i].isEditable ||
        (tagdefs [i].listingDisplay && tagdefs [i].editType == ET_LABEL)) {
      slistSetNum (elist, tagdefs [i].displayname, i);
    }
  }

  confui->gui.listingtaglist = llist;
  confui->gui.edittaglist = elist;
  logProcEnd (LOG_PROC, "confuiLoadTagList", "");
}

static bool
confuiMobmqTypeChg (void *udata)
{
  confuigui_t   *gui = udata;
  double        value;
  long          nval;

  logProcBegin (LOG_PROC, "confuiMobmqTypeChg");
  value = uiSpinboxGetValue (&gui->uiitem [CONFUI_SPINBOX_MOBILE_MQ].uiwidget);
  nval = (long) value;
  bdjoptSetNum (OPT_P_MOBILEMARQUEE, nval);
  confuiUpdateMobmqQrcode (gui);
  logProcEnd (LOG_PROC, "confuiMobmqTypeChg", "");
  return UICB_CONT;
}

static bool
confuiMobmqPortChg (void *udata)
{
  confuigui_t   *gui = udata;
  double        value;
  long          nval;

  logProcBegin (LOG_PROC, "confuiMobmqPortChg");
  value = uiSpinboxGetValue (&gui->uiitem [CONFUI_WIDGET_MMQ_PORT].uiwidget);
  nval = (long) value;
  bdjoptSetNum (OPT_P_MOBILEMQPORT, nval);
  confuiUpdateMobmqQrcode (gui);
  logProcEnd (LOG_PROC, "confuiMobmqPortChg", "");
  return UICB_CONT;
}

static int
confuiMobmqNameChg (uientry_t *entry, void *udata)
{
  confuigui_t     *gui = udata;
  const char      *sval;

  logProcBegin (LOG_PROC, "confuiMobmqNameChg");
  sval = uiEntryGetValue (entry);
  bdjoptSetStr (OPT_P_MOBILEMQTAG, sval);
  confuiUpdateMobmqQrcode (gui);
  logProcEnd (LOG_PROC, "confuiMobmqNameChg", "");
  return UIENTRY_OK;
}

static int
confuiMobmqTitleChg (uientry_t *entry, void *udata)
{
  confuigui_t     *gui = udata;
  const char      *sval;

  logProcBegin (LOG_PROC, "confuiMobmqTitleChg");
  sval = uiEntryGetValue (entry);
  bdjoptSetStr (OPT_P_MOBILEMQTITLE, sval);
  confuiUpdateMobmqQrcode (gui);
  logProcEnd (LOG_PROC, "confuiMobmqTitleChg", "");
  return UIENTRY_OK;
}

static bool
confuiRemctrlChg (void *udata, int value)
{
  confuigui_t  *gui = udata;

  logProcBegin (LOG_PROC, "confuiRemctrlChg");
  bdjoptSetNum (OPT_P_REMOTECONTROL, value);
  confuiUpdateRemctrlQrcode (gui);
  logProcEnd (LOG_PROC, "confuiRemctrlChg", "");
  return UICB_CONT;
}

static bool
confuiRemctrlPortChg (void *udata)
{
  confuigui_t   *gui = udata;
  double        value;
  long          nval;

  logProcBegin (LOG_PROC, "confuiRemctrlPortChg");
  value = uiSpinboxGetValue (&gui->uiitem [CONFUI_WIDGET_RC_PORT].uiwidget);
  nval = (long) value;
  bdjoptSetNum (OPT_P_REMCONTROLPORT, nval);
  confuiUpdateRemctrlQrcode (gui);
  logProcEnd (LOG_PROC, "confuiRemctrlPortChg", "");
  return UICB_CONT;
}


