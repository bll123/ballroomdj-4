/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
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
#include "callback.h"
#include "conn.h"
#include "datafile.h"
#include "dispsel.h"
#include "localeutil.h"
#include "lock.h"
#include "log.h"
#include "mdebug.h"
#include "mp3exp.h"
#include "msgparse.h"
#include "musicq.h"
#include "ossignal.h"
#include "osuiutils.h"
#include "pathbld.h"
#include "playlist.h"
#include "pli.h"
#include "progstate.h"
#include "slist.h"
#include "sock.h"
#include "sockh.h"
#include "song.h"
#include "songdb.h"
#include "sysvars.h"
#include "tmutil.h"
#include "ui.h"
#include "uimusicq.h"
#include "uinbutil.h"
#include "uiplayer.h"
#include "uiquickedit.h"
#include "uiextreq.h"
#include "uisongfilter.h"
#include "uisongsel.h"
#include "uiutils.h"

enum {
  PLUI_MENU_CB_PLAY_QUEUE,
  PLUI_MENU_CB_EXTRA_QUEUE,
  PLUI_MENU_CB_SWITCH_QUEUE,
  PLUI_MENU_CB_REQ_EXT_DIALOG,
  PLUI_MENU_CB_MQ_FONT_SZ,
  PLUI_MENU_CB_MQ_FIND,
  PLUI_MENU_CB_EXP_MP3,
  PLUI_MENU_CB_QE_CURRENT,
  PLUI_MENU_CB_QE_SELECTED,
  PLUI_MENU_CB_RELOAD,
  PLUI_CB_NOTEBOOK,
  PLUI_CB_CLOSE,
  PLUI_CB_PLAYBACK_QUEUE,
  PLUI_CB_FONT_SIZE,
  PLUI_CB_QUEUE_SL,
  PLUI_CB_SONG_SAVE,
  PLUI_CB_CLEAR_QUEUE,
  PLUI_CB_REQ_EXT,
  PLUI_CB_QUICK_EDIT,
  PLUI_CB_KEYB,
  PLUI_CB_FONT_SZ_CHG,
  PLUI_CB_DRAG_DROP,
  PLUI_CB_MAX,
};

enum {
  PLUI_W_WINDOW,
  PLUI_W_MAIN_VBOX,
  PLUI_W_NOTEBOOK,
  PLUI_W_CLOCK,
  PLUI_W_LED_OFF,
  PLUI_W_LED_ON,
  PLUI_W_MQ_FONT_SZ_DIALOG,
  PLUI_W_MQ_SZ,
  PLUI_W_STATUS_MSG,
  PLUI_W_ERROR_MSG,
  PLUI_W_MENU_QE_CURR,
  PLUI_W_MENU_QE_SEL,
  PLUI_W_MENU_EXT_REQ,
  PLUI_W_MENU_RELOAD,
  PLUI_W_KEY_HNDLR,
  PLUI_W_SET_PB_BUTTON,
  PLUI_W_MAX,
};

enum {
  RESET_VOL_NO,
  RESET_VOL_CURR,
};

typedef struct {
  progstate_t     *progstate;
  char            *locknm;
  conn_t          *conn;
  musicdb_t       *musicdb;
  songdb_t        *songdb;
  int             musicqPlayIdx;
  int             musicqRequestIdx;
  int             musicqManageIdx;
  dispsel_t       *dispsel;
  long            dbgflags;
  int             marqueeIsMaximized;
  int             marqueeFontSize;
  int             marqueeFontSizeFS;
  mstime_t        marqueeFontSizeCheck;
  int             stopwaitcount;
  mstime_t        clockCheck;
  uisongfilter_t  *uisongfilter;
  uiwcont_t       *wcont [PLUI_W_MAX];
  int             pliSupported;
  /* quick edit */
  uiqe_t          *uiqe;
  int             resetvolume;
  /* external request */
  int             extreqRow;
  uiextreq_t      *uiextreq;
  /* notebook */
  uinbtabid_t     *nbtabid;
  int             currpage;
  callback_t      *callbacks [PLUI_CB_MAX];
  uiwcont_t       *musicqImage [MUSICQ_PB_MAX];
  /* ui major elements */
  uiplayer_t      *uiplayer;
  uimusicq_t      *uimusicq;
  uisongsel_t     *uisongsel;
  /* options */
  datafile_t      *optiondf;
  nlist_t         *options;
  /* export mp3 */
  mp3exp_t        *mp3exp;
  mstime_t        expmp3chkTime;
  int             expmp3state;
  /* flags */
  bool            fontszdialogcreated : 1;
  bool            mainalready : 1;
  bool            mainreattach : 1;
  bool            marqueeoff : 1;
  bool            mqfontsizeactive : 1;
  bool            optionsalloc : 1;
  bool            reloadinit : 1;
  bool            inreload : 1;
  bool            startmainsent : 1;
  bool            stopping : 1;
  bool            uibuilt : 1;
  bool            haveplisupport : 1;
} playerui_t;

static datafilekey_t playeruidfkeys [] = {
  { "EXP_MP3_DIR",              EXP_MP3_DIR,                VALUE_STR, NULL, DF_NORM },
  { "FILTER_POS_X",             SONGSEL_FILTER_POSITION_X,  VALUE_NUM, NULL, DF_NORM },
  { "FILTER_POS_Y",             SONGSEL_FILTER_POSITION_Y,  VALUE_NUM, NULL, DF_NORM },
  { "PLUI_POS_X",               PLUI_POSITION_X,            VALUE_NUM, NULL, DF_NORM },
  { "PLUI_POS_Y",               PLUI_POSITION_Y,            VALUE_NUM, NULL, DF_NORM },
  { "PLUI_RESTART_POS",         PLUI_RESTART_POSITION,      VALUE_NUM, NULL, DF_NORM },
  { "PLUI_SIZE_X",              PLUI_SIZE_X,                VALUE_NUM, NULL, DF_NORM },
  { "PLUI_SIZE_Y",              PLUI_SIZE_Y,                VALUE_NUM, NULL, DF_NORM },
  { "REQ_EXT_DIR",              REQ_EXT_DIR,                VALUE_STR, NULL, DF_NORM },
  { "REQ_EXT_X",                REQ_EXT_POSITION_X,         VALUE_NUM, NULL, DF_NORM },
  { "REQ_EXT_Y",                REQ_EXT_POSITION_Y,         VALUE_NUM, NULL, DF_NORM },
  { "SHOW_EXTRA_QUEUES",        PLUI_SHOW_EXTRA_QUEUES,     VALUE_NUM, NULL, DF_NORM },
  { "SORT_BY",                  SONGSEL_SORT_BY,            VALUE_STR, NULL, DF_NORM },
  { "SWITCH_QUEUE_WHEN_EMPTY",  PLUI_SWITCH_QUEUE_WHEN_EMPTY, VALUE_NUM, NULL, DF_NORM },
};
enum {
  PLAYERUI_DFKEY_COUNT = (sizeof (playeruidfkeys) / sizeof (datafilekey_t)),
};

static datafilekey_t reloaddfkeys [] = {
  { "CURRENT",  RELOAD_CURR, VALUE_STR, NULL, DF_NORM },
};
enum {
  RELOAD_DFKEY_COUNT = (sizeof (reloaddfkeys) / sizeof (datafilekey_t)),
};

enum {
  PLUI_UPDATE_MAIN,
  PLUI_NO_UPDATE,
};

static bool     pluiConnectingCallback (void *udata, programstate_t programState);
static bool     pluiHandshakeCallback (void *udata, programstate_t programState);
static bool     pluiInitDataCallback (void *udata, programstate_t programState);
static bool     pluiStoppingCallback (void *udata, programstate_t programState);
static bool     pluiStopWaitCallback (void *udata, programstate_t programState);
static bool     pluiClosingCallback (void *udata, programstate_t programState);
static void     pluiBuildUI (playerui_t *plui);
static void     pluiInitializeUI (playerui_t *plui);
static int      pluiMainLoop (void *tplui);
static void     pluiClock (playerui_t *plui);
static int      pluiProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static bool     pluiCloseWin (void *udata);
static void     pluiSigHandler (int sig);
static char *   pluiExportMP3Dialog (playerui_t *plui);
/* queue selection handlers */
static bool     pluiSwitchPage (void *udata, long pagenum);
static void     pluiPlaybackButtonHideShow (playerui_t *plui, long pagenum);
static bool     pluiProcessSetPlaybackQueue (void *udata);
static void     pluiSetPlaybackQueue (playerui_t *plui, int newqueue, int updateFlag);
static void     pluiSetManageQueue (playerui_t *plui, int newqueue);
/* option handlers */
static bool     pluiToggleExtraQueues (void *udata);
static void     pluiSetExtraQueues (playerui_t *plui);
static void     pluiSetSupported (playerui_t *plui);
static bool     pluiToggleSwitchQueue (void *udata);
static void     pluiSetSwitchQueue (playerui_t *plui);
static bool     pluiMarqueeFontSizeDialog (void *udata);
static void     pluiCreateMarqueeFontSizeDialog (playerui_t *plui);
static bool     pluiMarqueeFontSizeDialogResponse (void *udata, long responseid);
static bool     pluiMarqueeFontSizeChg (void *udata);
static bool     pluiMarqueeRecover (void *udata);
static void     pluisetMarqueeIsMaximized (playerui_t *plui, char *args);
static void     pluisetMarqueeFontSizes (playerui_t *plui, char *args);
static bool     pluiQueueProcess (void *udata, long dbidx);
static bool     pluiSongSaveCallback (void *udata, long dbidx);
static bool     pluiClearQueueCallback (void *udata);
static void     pluiPushHistory (playerui_t *plui, const char *args);
static bool     pluiRequestExternalDialog (void *udata);
static bool     pluiExtReqCallback (void *udata);
static bool     pluiQuickEditCurrent (void *udata);
static bool     pluiQuickEditSelected (void *udata);
static bool     pluiQuickEditCallback (void *udata);
static bool     pluiReload (void *udata);
static void     pluiReloadCurrent (playerui_t *plui);
static void     pluiReloadSave (playerui_t *plui, int mqidx);
static void     pluiReloadSaveCurrent (playerui_t *plui);
static bool     pluiKeyEvent (void *udata);
static bool     pluiExportMP3 (void *udata);
static bool     pluiDragDropCallback (void *udata, const char *uri, int row);

static int gKillReceived = 0;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  playerui_t      plui;
  char            tbuff [MAXPATHLEN];

#if BDJ4_MEM_DEBUG
  mdebugInit ("plui");
#endif

  plui.progstate = progstateInit ("playerui");
  progstateSetCallback (plui.progstate, STATE_CONNECTING,
      pluiConnectingCallback, &plui);
  progstateSetCallback (plui.progstate, STATE_WAIT_HANDSHAKE,
      pluiHandshakeCallback, &plui);
  progstateSetCallback (plui.progstate, STATE_INITIALIZE_DATA,
      pluiInitDataCallback, &plui);

  plui.uiplayer = NULL;
  plui.uimusicq = NULL;
  plui.uisongsel = NULL;
  plui.musicqPlayIdx = MUSICQ_PB_A;
  plui.musicqRequestIdx = MUSICQ_PB_A;
  plui.musicqManageIdx = MUSICQ_PB_A;
  plui.marqueeIsMaximized = false;
  plui.marqueeFontSize = 36;
  plui.marqueeFontSizeFS = 60;
  mstimeset (&plui.marqueeFontSizeCheck, 3600000);
  mstimeset (&plui.clockCheck, 0);
  plui.stopwaitcount = 0;
  plui.nbtabid = uinbutilIDInit ();
  plui.uisongfilter = NULL;
  plui.uiqe = NULL;
  plui.extreqRow = -1;
  plui.uiextreq = NULL;
  plui.uibuilt = false;
  plui.fontszdialogcreated = false;
  plui.currpage = 0;
  plui.startmainsent = false;
  plui.stopping = false;
  plui.mainalready = false;
  plui.mainreattach = false;
  plui.marqueeoff = false;
  plui.reloadinit = false;
  plui.inreload = false;
  plui.mqfontsizeactive = false;
  plui.expmp3state = BDJ4_STATE_OFF;
  for (int i = 0; i < PLUI_CB_MAX; ++i) {
    plui.callbacks [i] = NULL;
  }
  for (int i = 0; i < MUSICQ_PB_MAX; ++i) {
    plui.musicqImage [i] = NULL;
  }
  for (int i = 0; i < PLUI_W_MAX; ++i) {
    plui.wcont [i] = NULL;
  }
  plui.pliSupported = PLI_SUPPORT_NONE;
  plui.haveplisupport = false;

  osSetStandardSignals (pluiSigHandler);

  plui.dbgflags = BDJ4_INIT_ALL;
  bdj4startup (argc, argv, &plui.musicdb,
      "plui", ROUTE_PLAYERUI, &plui.dbgflags);
  logProcBegin (LOG_PROC, "playerui");

  plui.songdb = songdbAlloc (plui.musicdb);

  if (bdjoptGetNum (OPT_P_MARQUEE_SHOW) == MARQUEE_SHOW_OFF) {
    plui.marqueeoff = true;
  }
  plui.dispsel = dispselAlloc (DISP_SEL_LOAD_PLAYER);

  listenPort = bdjvarsGetNum (BDJVL_PLAYERUI_PORT);
  plui.conn = connInit (ROUTE_PLAYERUI);

  pathbldMakePath (tbuff, sizeof (tbuff),
      PLAYERUI_OPT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  plui.optiondf = datafileAllocParse ("ui-player", DFTYPE_KEY_VAL, tbuff,
      playeruidfkeys, PLAYERUI_DFKEY_COUNT, DF_NO_OFFSET, NULL);
  plui.options = datafileGetList (plui.optiondf);
  plui.optionsalloc = false;
  if (plui.options == NULL || nlistGetCount (plui.options) == 0) {
    plui.optionsalloc = true;
    plui.options = nlistAlloc ("ui-player", LIST_ORDERED, NULL);

    nlistSetNum (plui.options, PLUI_SHOW_EXTRA_QUEUES, false);
    nlistSetNum (plui.options, PLUI_SWITCH_QUEUE_WHEN_EMPTY, false);
    nlistSetNum (plui.options, SONGSEL_FILTER_POSITION_X, -1);
    nlistSetNum (plui.options, SONGSEL_FILTER_POSITION_Y, -1);
    nlistSetNum (plui.options, PLUI_POSITION_X, -1);
    nlistSetNum (plui.options, PLUI_POSITION_Y, -1);
    nlistSetNum (plui.options, PLUI_SIZE_X, 1000);
    nlistSetNum (plui.options, PLUI_SIZE_Y, 600);
    nlistSetNum (plui.options, PLUI_RESTART_POSITION, -1);
    nlistSetNum (plui.options, REQ_EXT_POSITION_X, -1);
    nlistSetNum (plui.options, REQ_EXT_POSITION_Y, -1);
    nlistSetStr (plui.options, REQ_EXT_DIR, "");
    nlistSetNum (plui.options, EXP_IMP_BDJ4_POSITION_X, -1);
    nlistSetNum (plui.options, EXP_IMP_BDJ4_POSITION_Y, -1);
    nlistSetStr (plui.options, EXP_MP3_DIR, "");
    nlistSetStr (plui.options, SONGSEL_SORT_BY, "TITLE");
  }

  uiUIInitialize (sysvarsGetNum (SVL_LOCALE_DIR));
  uiSetUICSS (uiutilsGetCurrentFont (),
      bdjoptGetStr (OPT_P_UI_ACCENT_COL),
      bdjoptGetStr (OPT_P_UI_ERROR_COL));

  pluiBuildUI (&plui);
  osuiFinalize ();

  /* register these after calling the sub-window initialization */
  /* then these will be run last, after the other closing callbacks */
  progstateSetCallback (plui.progstate, STATE_STOPPING,
      pluiStoppingCallback, &plui);
  progstateSetCallback (plui.progstate, STATE_STOP_WAIT,
      pluiStopWaitCallback, &plui);
  progstateSetCallback (plui.progstate, STATE_CLOSING,
      pluiClosingCallback, &plui);

  sockhMainLoop (listenPort, pluiProcessMsg, pluiMainLoop, &plui);
  connFree (plui.conn);
  progstateFree (plui.progstate);
  logProcEnd (LOG_PROC, "playerui", "");
  logEnd ();
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return status;
}

/* internal routines */

static bool
pluiStoppingCallback (void *udata, programstate_t programState)
{
  playerui_t    * plui = udata;
  int           x, y, ws;

  logProcBegin (LOG_PROC, "pluiStoppingCallback");
  if (! plui->marqueeoff &&
      connHaveHandshake (plui->conn, ROUTE_MAIN) &&
      connIsConnected (plui->conn, ROUTE_MARQUEE)) {
    connSendMessage (plui->conn, ROUTE_MAIN, MSG_STOP_MARQUEE, NULL);
  }

  connSendMessage (plui->conn, ROUTE_STARTERUI, MSG_STOP_MAIN, NULL);

  uiWindowGetSize (plui->wcont [PLUI_W_WINDOW], &x, &y);
  nlistSetNum (plui->options, PLUI_SIZE_X, x);
  nlistSetNum (plui->options, PLUI_SIZE_Y, y);
  uiWindowGetPosition (plui->wcont [PLUI_W_WINDOW], &x, &y, &ws);
  nlistSetNum (plui->options, PLUI_POSITION_X, x);
  nlistSetNum (plui->options, PLUI_POSITION_Y, y);

  connDisconnectAll (plui->conn);

  logProcEnd (LOG_PROC, "pluiStoppingCallback", "");
  return STATE_FINISHED;
}

static bool
pluiStopWaitCallback (void *udata, programstate_t programState)
{
  playerui_t  * plui = udata;
  bool        rc;

  rc = connWaitClosed (plui->conn, &plui->stopwaitcount);
  return rc;
}

static bool
pluiClosingCallback (void *udata, programstate_t programState)
{
  playerui_t    *plui = udata;

  logProcBegin (LOG_PROC, "pluiClosingCallback");

  uiCloseWindow (plui->wcont [PLUI_W_WINDOW]);
  uiCleanup ();

  uiWidgetClearPersistent (plui->wcont [PLUI_W_LED_ON]);
  uiWidgetClearPersistent (plui->wcont [PLUI_W_LED_OFF]);

  for (int i = 0; i < PLUI_CB_MAX; ++i) {
    callbackFree (plui->callbacks [i]);
  }
  for (int i = 0; i < MUSICQ_PB_MAX; ++i) {
    uiwcontFree (plui->musicqImage [i]);
  }
  for (int i = 0; i < PLUI_W_MAX; ++i) {
    uiwcontFree (plui->wcont [i]);
  }

  datafileSave (plui->optiondf, NULL, plui->options, DF_NO_OFFSET, 1);

  bdj4shutdown (ROUTE_PLAYERUI, plui->musicdb);
  dispselFree (plui->dispsel);
  songdbFree (plui->songdb);

  uinbutilIDFree (plui->nbtabid);
  uisfFree (plui->uisongfilter);
  uiqeFree (plui->uiqe);
  uiextreqFree (plui->uiextreq);
  if (plui->optionsalloc) {
    nlistFree (plui->options);
  }
  datafileFree (plui->optiondf);

  uiplayerFree (plui->uiplayer);
  uimusicqFree (plui->uimusicq);
  uisongselFree (plui->uisongsel);

  logProcEnd (LOG_PROC, "pluiClosingCallback", "");
  return STATE_FINISHED;
}

static void
pluiBuildUI (playerui_t *plui)
{
  uiwcont_t   *menubar;
  uiwcont_t   *menu;
  uiwcont_t   *menuitem;
  uiwcont_t   *hbox;
  uiwcont_t   *uip;
  uiwcont_t   *uiwidgetp;
  const char  *str;
  char        imgbuff [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];
  int         x, y;
  void        *tempp;
  uiutilsaccent_t accent;

  logProcBegin (LOG_PROC, "pluiBuildUI");

  pathbldMakePath (tbuff, sizeof (tbuff),  "led_off", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DIR_IMG);
  plui->wcont [PLUI_W_LED_OFF] = uiImageFromFile (tbuff);
  uiImageConvertToPixbuf (plui->wcont [PLUI_W_LED_OFF]);
  uiWidgetMakePersistent (plui->wcont [PLUI_W_LED_OFF]);

  pathbldMakePath (tbuff, sizeof (tbuff),  "led_on", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DIR_IMG);
  plui->wcont [PLUI_W_LED_ON] = uiImageFromFile (tbuff);
  uiImageConvertToPixbuf (plui->wcont [PLUI_W_LED_ON]);
  uiWidgetMakePersistent (plui->wcont [PLUI_W_LED_ON]);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_player", BDJ4_IMG_SVG_EXT, PATHBLD_MP_DIR_IMG);
  plui->callbacks [PLUI_CB_CLOSE] = callbackInit (
      pluiCloseWin, plui, NULL);
  /* CONTEXT: playerui: main window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Player"),
      bdjoptGetStr (OPT_P_PROFILENAME));
  plui->wcont [PLUI_W_WINDOW] = uiCreateMainWindow (plui->callbacks [PLUI_CB_CLOSE],
      tbuff, imgbuff);

  pluiInitializeUI (plui);

  plui->wcont [PLUI_W_MAIN_VBOX] = uiCreateVertBox ();
  uiWindowPackInWindow (plui->wcont [PLUI_W_WINDOW], plui->wcont [PLUI_W_MAIN_VBOX]);
  uiWidgetSetAllMargins (plui->wcont [PLUI_W_MAIN_VBOX], 2);

  plui->wcont [PLUI_W_KEY_HNDLR] = uiKeyAlloc ();
  plui->callbacks [PLUI_CB_KEYB] = callbackInit (
      pluiKeyEvent, plui, NULL);
  uiKeySetKeyCallback (plui->wcont [PLUI_W_KEY_HNDLR],
      plui->wcont [PLUI_W_MAIN_VBOX], plui->callbacks [PLUI_CB_KEYB]);

  /* menu */
  uiutilsAddProfileColorDisplay (plui->wcont [PLUI_W_MAIN_VBOX], &accent);
  hbox = accent.hbox;
  uiwcontFree (accent.label);
  uiWidgetExpandHoriz (hbox);

  menubar = uiCreateMenubar ();
  uiBoxPackStart (hbox, menubar);

  plui->wcont [PLUI_W_CLOCK] = uiCreateLabel ("");
  uiBoxPackEnd (hbox, plui->wcont [PLUI_W_CLOCK]);
  uiWidgetSetMarginStart (plui->wcont [PLUI_W_CLOCK], 4);
  uiWidgetSetState (plui->wcont [PLUI_W_CLOCK], UIWIDGET_DISABLE);

  uiwidgetp = uiCreateLabel ("");
  uiWidgetSetClass (uiwidgetp, ERROR_CLASS);
  uiBoxPackEnd (hbox, uiwidgetp);
  plui->wcont [PLUI_W_ERROR_MSG] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("");
  uiWidgetSetClass (uiwidgetp, ACCENT_CLASS);
  uiBoxPackEnd (hbox, uiwidgetp);
  plui->wcont [PLUI_W_STATUS_MSG] = uiwidgetp;

  /* actions */
  /* CONTEXT: playerui: menu selection: actions for the player */
  menuitem = uiMenuCreateItem (menubar, _("Actions"), NULL);
  menu = uiCreateSubMenu (menuitem);
  uiwcontFree (menuitem);

  plui->callbacks [PLUI_MENU_CB_REQ_EXT_DIALOG] = callbackInit (
      pluiRequestExternalDialog, plui, NULL);
  /* CONTEXT: playerui: menu selection: action: external request */
  menuitem = uiMenuCreateItem (menu, _("External Request"),
      plui->callbacks [PLUI_MENU_CB_REQ_EXT_DIALOG]);
  plui->wcont [PLUI_W_MENU_EXT_REQ] = menuitem;

  plui->callbacks [PLUI_MENU_CB_QE_CURRENT] = callbackInit (
      pluiQuickEditCurrent, plui, NULL);
  /* CONTEXT: playerui: menu selection: action: quick edit current song */
  menuitem = uiMenuCreateItem (menu, _("Quick Edit: Current"),
      plui->callbacks [PLUI_MENU_CB_QE_CURRENT]);
  plui->wcont [PLUI_W_MENU_QE_CURR] = menuitem;

  plui->callbacks [PLUI_MENU_CB_QE_SELECTED] = callbackInit (
      pluiQuickEditSelected, plui, NULL);
  /* CONTEXT: playerui: menu selection: action: quick edit selected song */
  menuitem = uiMenuCreateItem (menu, _("Quick Edit: Selected"),
      plui->callbacks [PLUI_MENU_CB_QE_SELECTED]);
  plui->wcont [PLUI_W_MENU_QE_SEL] = menuitem;

  plui->callbacks [PLUI_MENU_CB_RELOAD] = callbackInit (
      pluiReload, plui, NULL);
  /* CONTEXT: playerui: menu selection: action: reload */
  menuitem = uiMenuCreateItem (menu, _("Reload"),
      plui->callbacks [PLUI_MENU_CB_RELOAD]);
  plui->wcont [PLUI_W_MENU_RELOAD] = menuitem;

  uiwcontFree (menu);

  /* marquee */
  /* CONTEXT: playerui: menu selection: marquee related options (suggested: song display) */
  menuitem = uiMenuCreateItem (menubar, _("Marquee"), NULL);
  if (plui->marqueeoff) {
    uiWidgetSetState (menuitem, UIWIDGET_DISABLE);
  }
  menu = uiCreateSubMenu (menuitem);
  uiwcontFree (menuitem);

  plui->callbacks [PLUI_MENU_CB_MQ_FONT_SZ] = callbackInit (
      pluiMarqueeFontSizeDialog, plui, NULL);
  /* CONTEXT: playerui: menu selection: marquee: change the marquee font size */
  menuitem = uiMenuCreateItem (menu, _("Font Size"),
      plui->callbacks [PLUI_MENU_CB_MQ_FONT_SZ]);
  uiwcontFree (menuitem);

  plui->callbacks [PLUI_MENU_CB_MQ_FIND] = callbackInit (
      pluiMarqueeRecover, plui, NULL);
  /* CONTEXT: playerui: menu selection: marquee: bring the marquee window back to the main screen */
  menuitem = uiMenuCreateItem (menu, _("Recover Marquee"),
      plui->callbacks [PLUI_MENU_CB_MQ_FIND]);
  uiwcontFree (menuitem);

  uiwcontFree (menu);

  /* export */
  /* CONTEXT: playerui: menu selection: export */
  menuitem = uiMenuCreateItem (menubar, _("Export"), NULL);
  menu = uiCreateSubMenu (menuitem);
  uiwcontFree (menuitem);

  plui->callbacks [PLUI_MENU_CB_EXP_MP3] = callbackInit (
      pluiExportMP3, plui, NULL);
  /* CONTEXT: player ui: menu selection: export: export as MP3 */
  snprintf (tbuff, sizeof (tbuff), _("Export as %s"), BDJ4_MP3_LABEL);
  menuitem = uiMenuCreateItem (menu, tbuff,
      plui->callbacks [PLUI_MENU_CB_EXP_MP3]);
  /* a missing audio adjust file will not stop startup */
  tempp = bdjvarsdfGet (BDJVDF_AUDIO_ADJUST);
  if (tempp == NULL) {
    uiWidgetSetState (menuitem, UIWIDGET_DISABLE);
  }
  uiwcontFree (menuitem);

  uiwcontFree (menu);

  /* options */
  /* CONTEXT: playerui: menu selection: options for the player */
  menuitem = uiMenuCreateItem (menubar, _("Options"), NULL);
  menu = uiCreateSubMenu (menuitem);
  uiwcontFree (menuitem);

  plui->callbacks [PLUI_MENU_CB_EXTRA_QUEUE] = callbackInit (
      pluiToggleExtraQueues, plui, NULL);
  /* CONTEXT: playerui: menu checkbox: show the extra queues (in addition to the main music queue) */
  menuitem = uiMenuCreateCheckbox (menu, _("Show Extra Queues"),
      nlistGetNum (plui->options, PLUI_SHOW_EXTRA_QUEUES),
      plui->callbacks [PLUI_MENU_CB_EXTRA_QUEUE]);
  uiwcontFree (menuitem);

  plui->callbacks [PLUI_MENU_CB_SWITCH_QUEUE] = callbackInit (
      pluiToggleSwitchQueue, plui, NULL);
  /* CONTEXT: playerui: menu checkbox: when a queue is emptied, switch playback to the next queue */
  menuitem = uiMenuCreateCheckbox (menu, _("Switch Queue When Empty"),
      nlistGetNum (plui->options, PLUI_SWITCH_QUEUE_WHEN_EMPTY),
      plui->callbacks [PLUI_MENU_CB_SWITCH_QUEUE]);
  uiwcontFree (menuitem);

  /* player */
  uiwidgetp = uiplayerBuildUI (plui->uiplayer);
  uiBoxPackStart (plui->wcont [PLUI_W_MAIN_VBOX], uiwidgetp);

  plui->wcont [PLUI_W_NOTEBOOK] = uiCreateNotebook ();
  uiBoxPackStartExpand (plui->wcont [PLUI_W_MAIN_VBOX], plui->wcont [PLUI_W_NOTEBOOK]);

  plui->callbacks [PLUI_CB_NOTEBOOK] = callbackInitLong (
      pluiSwitchPage, plui);
  uiNotebookSetCallback (plui->wcont [PLUI_W_NOTEBOOK], plui->callbacks [PLUI_CB_NOTEBOOK]);

  plui->callbacks [PLUI_CB_PLAYBACK_QUEUE] = callbackInit (
      pluiProcessSetPlaybackQueue, plui, NULL);
  uiwidgetp = uiCreateButton (plui->callbacks [PLUI_CB_PLAYBACK_QUEUE],
      /* CONTEXT: playerui: select the current queue for playback */
      _("Set Queue for Playback"), NULL);
  uiNotebookSetActionWidget (plui->wcont [PLUI_W_NOTEBOOK], uiwidgetp);
  uiWidgetShowAll (uiwidgetp);
  plui->wcont [PLUI_W_SET_PB_BUTTON] = uiwidgetp;

  plui->callbacks [PLUI_CB_DRAG_DROP] = callbackInitStrInt (
      pluiDragDropCallback, plui);

  for (int i = 0; i < MUSICQ_DISP_MAX; ++i) {
    int   tabtype;
    /* music queue tab */

    if (! bdjoptGetNumPerQueue (OPT_Q_DISPLAY, i)) {
      /* do not display */
      continue;
    }

    tabtype = UI_TAB_MUSICQ;
    if (i == MUSICQ_HISTORY) {
      tabtype = UI_TAB_HISTORY;
    }

    uip = uimusicqBuildUI (plui->uimusicq, plui->wcont [PLUI_W_WINDOW], i,
        plui->wcont [PLUI_W_ERROR_MSG], NULL);

    uiwcontFree (hbox);
    hbox = uiCreateHorizBox ();
    if (tabtype == UI_TAB_HISTORY) {
      /* CONTEXT: playerui: name of the history tab : displayed played songs */
      str = _("History");
    } else {
      str = bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, i);
    }

    uiwidgetp = uiCreateLabel (str);
    uiBoxPackStart (hbox, uiwidgetp);

    if (tabtype == UI_TAB_MUSICQ) {
      plui->musicqImage [i] = uiImageNew ();
      uiImageSetFromPixbuf (plui->musicqImage [i], plui->wcont [PLUI_W_LED_ON]);
      uiWidgetSetMarginStart (plui->musicqImage [i], 1);
      uiBoxPackStart (hbox, plui->musicqImage [i]);

      uimusicqDragDropSetURICallback (plui->uimusicq, i, plui->callbacks [PLUI_CB_DRAG_DROP]);
    }

    uiNotebookAppendPage (plui->wcont [PLUI_W_NOTEBOOK], uip, hbox);
    uiwcontFree (uiwidgetp);
    uinbutilIDAdd (plui->nbtabid, tabtype);
    uiWidgetShowAll (hbox);
  }

  /* request tab */
  uip = uisongselBuildUI (plui->uisongsel, plui->wcont [PLUI_W_WINDOW]);
  /* CONTEXT: playerui: name of request tab : lists the songs in the database */
  uiwidgetp = uiCreateLabel (_("Request"));
  uiNotebookAppendPage (plui->wcont [PLUI_W_NOTEBOOK], uip, uiwidgetp);
  uinbutilIDAdd (plui->nbtabid, UI_TAB_SONGSEL);
  uiwcontFree (uiwidgetp);

  x = nlistGetNum (plui->options, PLUI_SIZE_X);
  y = nlistGetNum (plui->options, PLUI_SIZE_Y);
  uiWindowSetDefaultSize (plui->wcont [PLUI_W_WINDOW], x, y);

  uiWidgetShowAll (plui->wcont [PLUI_W_WINDOW]);

  x = nlistGetNum (plui->options, PLUI_POSITION_X);
  y = nlistGetNum (plui->options, PLUI_POSITION_Y);
  uiWindowMove (plui->wcont [PLUI_W_WINDOW], x, y, -1);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_player", BDJ4_IMG_PNG_EXT, PATHBLD_MP_DIR_IMG);
  osuiSetIcon (imgbuff);
  pluiPlaybackButtonHideShow (plui, 0);

  plui->callbacks [PLUI_CB_SONG_SAVE] = callbackInitLong (
      pluiSongSaveCallback, plui);
  uimusicqSetSongSaveCallback (plui->uimusicq, plui->callbacks [PLUI_CB_SONG_SAVE]);
  uisongselSetSongSaveCallback (plui->uisongsel, plui->callbacks [PLUI_CB_SONG_SAVE]);

  plui->callbacks [PLUI_CB_CLEAR_QUEUE] = callbackInit (
      pluiClearQueueCallback, plui, NULL);
  uimusicqSetClearQueueCallback (plui->uimusicq, plui->callbacks [PLUI_CB_CLEAR_QUEUE]);

  plui->uibuilt = true;

  uiwcontFree (hbox);
  uiwcontFree (menu);
  uiwcontFree (menubar);

  logProcEnd (LOG_PROC, "pluiBuildUI", "");
}

static void
pluiInitializeUI (playerui_t *plui)
{
  plui->uiplayer = uiplayerInit (plui->progstate, plui->conn, plui->musicdb);

  plui->uiqe = uiqeInit (plui->wcont [PLUI_W_WINDOW],
      plui->musicdb, plui->options);
  plui->callbacks [PLUI_CB_QUICK_EDIT] = callbackInit (
      pluiQuickEditCallback,
      plui, "musicq: quick edit response");
  uiqeSetResponseCallback (plui->uiqe, plui->callbacks [PLUI_CB_QUICK_EDIT]);

  plui->uiextreq = uiextreqInit (plui->wcont [PLUI_W_WINDOW],
      plui->musicdb, plui->options);
  plui->callbacks [PLUI_CB_REQ_EXT] = callbackInit (
      pluiExtReqCallback,
      plui, "musicq: external request response");
  uiextreqSetResponseCallback (plui->uiextreq, plui->callbacks [PLUI_CB_REQ_EXT]);

  plui->uimusicq = uimusicqInit ("plui", plui->conn, plui->musicdb,
      plui->dispsel, DISP_SEL_MUSICQ);

  plui->uisongfilter = uisfInit (plui->wcont [PLUI_W_WINDOW], plui->options,
      SONG_FILTER_FOR_PLAYBACK);
  plui->uisongsel = uisongselInit ("plui-req", plui->conn, plui->musicdb,
      plui->dispsel, NULL, plui->options,
      plui->uisongfilter, DISP_SEL_REQUEST);
  plui->callbacks [PLUI_CB_QUEUE_SL] = callbackInitLong (
      pluiQueueProcess, plui);
  uisongselSetQueueCallback (plui->uisongsel,
      plui->callbacks [PLUI_CB_QUEUE_SL]);
  uimusicqSetQueueCallback (plui->uimusicq,
      plui->callbacks [PLUI_CB_QUEUE_SL]);
}


static int
pluiMainLoop (void *tplui)
{
  playerui_t  *plui = tplui;
  int         stop = false;

  uiUIProcessEvents ();

  if (! progstateIsRunning (plui->progstate)) {
    progstateProcess (plui->progstate);
    if (progstateCurrState (plui->progstate) == STATE_CLOSED) {
      stop = true;
    }
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      plui->stopping = true;
      progstateShutdownProcess (plui->progstate);
      gKillReceived = 0;
    }
    return stop;
  }

  if (mstimeCheck (&plui->clockCheck)) {
    pluiClock (plui);
  }

  if (plui->expmp3state == BDJ4_STATE_PROCESS) {
    if (mstimeCheck (&plui->expmp3chkTime)) {
      int   count, tot;

      mp3ExportGetCount (plui->mp3exp, &count, &tot);
      uiutilsProgressStatus (plui->wcont [PLUI_W_STATUS_MSG], count, tot);
      mstimeset (&plui->expmp3chkTime, 500);
    }

    if (mp3ExportQueue (plui->mp3exp)) {
      uiutilsProgressStatus (plui->wcont [PLUI_W_STATUS_MSG], -1, -1);
      mp3ExportFree (plui->mp3exp);
      plui->expmp3state = BDJ4_STATE_OFF;
    }
  }

  if (plui->expmp3state == BDJ4_STATE_START) {
    char  tmp [40];

    /* CONTEXT: export as mp3: please wait... status message */
    uiLabelSetText (plui->wcont [PLUI_W_STATUS_MSG], _("Please wait\xe2\x80\xa6"));

    snprintf (tmp, sizeof (tmp), "%d", plui->musicqManageIdx);
    connSendMessage (plui->conn, ROUTE_MAIN, MSG_MAIN_REQ_QUEUE_INFO, tmp);
    plui->expmp3state = BDJ4_STATE_WAIT;
  }

  if (! plui->marqueeoff &&
      mstimeCheck (&plui->marqueeFontSizeCheck)) {
    char        tbuff [40];
    int         sz;

    if (plui->marqueeIsMaximized) {
      sz = plui->marqueeFontSizeFS;
    } else {
      sz = plui->marqueeFontSize;
    }
    snprintf (tbuff, sizeof (tbuff), "%d", sz);
    connSendMessage (plui->conn, ROUTE_MARQUEE, MSG_MARQUEE_SET_FONT_SZ, tbuff);
    mstimeset (&plui->marqueeFontSizeCheck, 3600000);
  }

  connProcessUnconnected (plui->conn);

  uiplayerMainLoop (plui->uiplayer);
  uimusicqMainLoop (plui->uimusicq);
  uisongselMainLoop (plui->uisongsel);
  uiextreqProcess (plui->uiextreq);

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    plui->stopping = true;
    progstateShutdownProcess (plui->progstate);
    gKillReceived = 0;
  }
  return stop;
}

static void
pluiClock (playerui_t *plui)
{
  char        tbuff [100];

  if (! plui->uibuilt) {
    return;
  }

  uiLabelSetText (plui->wcont [PLUI_W_CLOCK],
      tmutilDisp (tbuff, sizeof (tbuff), bdjoptGetNum (OPT_G_CLOCK_DISP)));
  if (bdjoptGetNum (OPT_G_CLOCK_DISP) == TM_CLOCK_OFF) {
    mstimeset (&plui->clockCheck, 3600000);
  } else {
    mstimeset (&plui->clockCheck, 200);
  }
}


static bool
pluiConnectingCallback (void *udata, programstate_t programState)
{
  playerui_t   *plui = udata;
  bool        rc = STATE_NOT_FINISH;

  logProcBegin (LOG_PROC, "pluiConnectingCallback");

  if (! connIsConnected (plui->conn, ROUTE_STARTERUI)) {
    connConnect (plui->conn, ROUTE_STARTERUI);
  }
  if (! connIsConnected (plui->conn, ROUTE_MAIN)) {
    connConnect (plui->conn, ROUTE_MAIN);
  }
  if (! connIsConnected (plui->conn, ROUTE_PLAYER)) {
    connConnect (plui->conn, ROUTE_PLAYER);
  }
  if (! plui->marqueeoff) {
    if (! connIsConnected (plui->conn, ROUTE_MARQUEE)) {
      connConnect (plui->conn, ROUTE_MARQUEE);
    }
  }

  connProcessUnconnected (plui->conn);

  if (connIsConnected (plui->conn, ROUTE_STARTERUI)) {
    rc = STATE_FINISHED;
  }

  logProcEnd (LOG_PROC, "pluiConnectingCallback", "");
  return rc;
}

static bool
pluiHandshakeCallback (void *udata, programstate_t programState)
{
  playerui_t    *plui = udata;
  bool          rc = STATE_NOT_FINISH;

  logProcBegin (LOG_PROC, "pluiHandshakeCallback");

  if (! connIsConnected (plui->conn, ROUTE_MAIN)) {
    connConnect (plui->conn, ROUTE_MAIN);
  }
  if (! connIsConnected (plui->conn, ROUTE_PLAYER)) {
    connConnect (plui->conn, ROUTE_PLAYER);
  }

  if (! plui->marqueeoff) {
    if (! connIsConnected (plui->conn, ROUTE_MARQUEE)) {
      connConnect (plui->conn, ROUTE_MARQUEE);
    }
  }

  connProcessUnconnected (plui->conn);

  if (plui->startmainsent == false &&
      connHaveHandshake (plui->conn, ROUTE_STARTERUI)) {
    connSendMessage (plui->conn, ROUTE_STARTERUI, MSG_START_MAIN, "0");
    plui->startmainsent = true;
  }

  if (! plui->marqueeoff &&
      connHaveHandshake (plui->conn, ROUTE_MAIN) &&
      ! connIsConnected (plui->conn, ROUTE_MARQUEE)) {
    connSendMessage (plui->conn, ROUTE_MAIN, MSG_START_MARQUEE, NULL);
  }

  if (connHaveHandshake (plui->conn, ROUTE_STARTERUI) &&
      connHaveHandshake (plui->conn, ROUTE_MAIN) &&
      connHaveHandshake (plui->conn, ROUTE_PLAYER) &&
      (plui->marqueeoff ||
      connHaveHandshake (plui->conn, ROUTE_MARQUEE))) {

    if (plui->mainalready) {
      connSendMessage (plui->conn, ROUTE_MAIN, MSG_MAIN_REQ_STATUS, NULL);
    }
    if (! plui->mainalready || plui->mainreattach) {
      pluiSetPlaybackQueue (plui, plui->musicqPlayIdx, PLUI_UPDATE_MAIN);
      pluiSetManageQueue (plui, plui->musicqManageIdx);
      pluiSetSwitchQueue (plui);
    }

    /* on one computer at least, the extra queues are showing up */
    /* when they should not */
    /* flushing all the ui events seems to prevent that */
    uiUIProcessWaitEvents ();

    pluiSetExtraQueues (plui);
    connSendMessage (plui->conn, ROUTE_PLAYER, MSG_PLAYER_SUPPORT, NULL);
    progstateLogTime (plui->progstate, "time-to-start-gui");
    rc = STATE_FINISHED;
  }

  logProcEnd (LOG_PROC, "pluiHandshakeCallback", "");
  return rc;
}

static bool
pluiInitDataCallback (void *udata, programstate_t programState)
{
  playerui_t    *plui = udata;
  bool          rc = STATE_NOT_FINISH;

  logProcBegin (LOG_PROC, "pluiInitDataCallback");

  if (plui->haveplisupport) {
    pluiSetSupported (plui);
    rc = STATE_FINISHED;
  }

  logProcEnd (LOG_PROC, "pluiInitDataCallback", "");
  return rc;
}

static int
pluiProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  playerui_t  *plui = udata;
  char        *targs = NULL;

  logProcBegin (LOG_PROC, "pluiProcessMsg");

  if (args != NULL) {
    targs = mdstrdup (args);
  }

  if (msg != MSG_MUSIC_QUEUE_DATA &&
      msg != MSG_PLAYER_STATUS_DATA) {
    logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
        routefrom, msgRouteDebugText (routefrom),
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
  }

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_PLAYERUI: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (plui->conn, routefrom);
          break;
        }
        case MSG_SOCKET_CLOSE: {
              connDisconnect (plui->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          plui->stopping = true;
          progstateShutdownProcess (plui->progstate);
          break;
        }
        case MSG_WINDOW_FIND: {
          uiWindowFind (plui->wcont [PLUI_W_WINDOW]);
          break;
        }
        case MSG_QUEUE_SWITCH: {
          pluiSetPlaybackQueue (plui, atoi (targs), PLUI_UPDATE_MAIN);
          break;
        }
        case MSG_MARQUEE_IS_MAX: {
          pluisetMarqueeIsMaximized (plui, targs);
          break;
        }
        case MSG_MARQUEE_FONT_SIZES: {
          pluisetMarqueeFontSizes (plui, targs);
          break;
        }
        case MSG_DB_ENTRY_UPDATE: {
          dbLoadEntry (plui->musicdb, atol (targs));
          uisongselPopulateData (plui->uisongsel);
          break;
        }
        case MSG_DB_ENTRY_REMOVE: {
          dbMarkEntryRemoved (plui->musicdb, atol (targs));
          uisongselPopulateData (plui->uisongsel);
          break;
        }
        case MSG_DB_ENTRY_UNREMOVE: {
          dbClearEntryRemoved (plui->musicdb, atol (targs));
          uisongselPopulateData (plui->uisongsel);
          break;
        }
        case MSG_SONG_SELECT: {
          mp_songselect_t   *songselect;

          songselect = msgparseSongSelect (targs);
          /* the display is offset by 1, as the 0 index is the current song */
          --songselect->loc;
          uimusicqProcessSongSelect (plui->uimusicq, songselect);
          msgparseSongSelectFree (songselect);
          break;
        }
        case MSG_MUSIC_QUEUE_DATA: {
          mp_musicqupdate_t   *musicqupdate;

          if (plui->stopping) {
            break;
          }

          musicqupdate = msgparseMusicQueueData (targs);
          if (musicqupdate == NULL) {
            break;
          }

          if ((int) musicqupdate->mqidx >= MUSICQ_DISP_MAX ||
              ! bdjoptGetNumPerQueue (OPT_Q_DISPLAY, musicqupdate->mqidx)) {
            logMsg (LOG_DBG, LOG_INFO, "ERR: music queue data: mq idx %d not valid", musicqupdate->mqidx);
            break;
          }

          if ((int) musicqupdate->mqidx < MUSICQ_DISP_MAX) {
            /* if displayed */
            if (bdjoptGetNumPerQueue (OPT_Q_DISPLAY, musicqupdate->mqidx)) {
              uimusicqProcessMusicQueueData (plui->uimusicq, musicqupdate);
              /* the music queue data is used to display the mark */
              /* indicating that the song is already in the song list */
              uisongselProcessMusicQueueData (plui->uisongsel, musicqupdate);
            }
          }

          pluiReloadSave (plui, musicqupdate->mqidx);

          if (musicqupdate->mqidx == MUSICQ_HISTORY) {
            const char  *name = _("History");

            uimusicqSetManageIdx (plui->uimusicq, MUSICQ_HISTORY);
            uimusicqSave (plui->uimusicq, name);
            playlistCheckAndCreate (name, PLTYPE_SONGLIST);
            uimusicqSetManageIdx (plui->uimusicq, plui->musicqManageIdx);
          }
          msgparseMusicQueueDataFree (musicqupdate);
          break;
        }
        case MSG_DATABASE_UPDATE: {
          plui->musicdb = bdj4ReloadDatabase (plui->musicdb);
          uiplayerSetDatabase (plui->uiplayer, plui->musicdb);
          uisongselSetDatabase (plui->uisongsel, plui->musicdb);
          uimusicqSetDatabase (plui->uimusicq, plui->musicdb);
          uisongselApplySongFilter (plui->uisongsel);
          break;
        }
        case MSG_SONG_FINISH: {
          uiLabelSetText (plui->wcont [PLUI_W_STATUS_MSG], "");
          uiLabelSetText (plui->wcont [PLUI_W_ERROR_MSG], "");
          pluiPushHistory (plui, targs);
          break;
        }
        case MSG_MAIN_START_RECONN: {
          plui->mainalready = true;
          /* CONTEXT: player-ui: error message */
          uiLabelSetText (plui->wcont [PLUI_W_ERROR_MSG], _("Recovered from crash."));
          break;
        }
        case MSG_MAIN_START_REATTACH: {
          plui->mainalready = true;
          plui->mainreattach = true;
          break;
        }
        case MSG_MAIN_CURR_MANAGE: {
          int     mqidx;

          mqidx = atoi (targs);
          plui->musicqManageIdx = mqidx;
          uimusicqSetManageIdx (plui->uimusicq, mqidx);
          break;
        }
        case MSG_MAIN_CURR_PLAY: {
          int     mqidx;

          mqidx = atoi (targs);
          pluiSetPlaybackQueue (plui, mqidx, PLUI_NO_UPDATE);
          break;
        }
        case MSG_MAIN_QUEUE_INFO: {
          char    *dir = NULL;

          dir = pluiExportMP3Dialog (plui);
          if (dir == NULL) {
            uiLabelSetText (plui->wcont [PLUI_W_STATUS_MSG], "");
            plui->expmp3state = BDJ4_STATE_OFF;
          } else {
            plui->mp3exp = mp3ExportInit (mdstrdup (args), plui->musicdb,
                dir, plui->musicqManageIdx);
            plui->expmp3state = BDJ4_STATE_PROCESS;
            mstimeset (&plui->expmp3chkTime, 500);
          }
          dataFree (dir);
          break;
        }
        case MSG_PLAYER_SUPPORT: {
          plui->pliSupported = atoi (args);
          plui->haveplisupport = true;
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

  dataFree (targs);

  /* due to the db update message, these must be applied afterwards */
  uiplayerProcessMsg (routefrom, route, msg, args, plui->uiplayer);

  /* for reload processing, if a musicq-status-data message was received, */
  /* update reload-curr */
  if (msg == MSG_MUSICQ_STATUS_DATA) {
    pluiReloadSaveCurrent (plui);
  }

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  logProcEnd (LOG_PROC, "pluiProcessMsg", "");
  return gKillReceived;
}


static bool
pluiCloseWin (void *udata)
{
  playerui_t   *plui = udata;

  logProcBegin (LOG_PROC, "pluiCloseWin");
  if (progstateCurrState (plui->progstate) <= STATE_RUNNING) {
    plui->stopping = true;
    progstateShutdownProcess (plui->progstate);
    logMsg (LOG_DBG, LOG_MSGS, "got: close win request");
    logProcEnd (LOG_PROC, "pluiCloseWin", "not-done");
    return UICB_STOP;
  }

  logProcEnd (LOG_PROC, "pluiCloseWin", "");
  return UICB_STOP;
}

static void
pluiSigHandler (int sig)
{
  gKillReceived = 1;
}

static char *
pluiExportMP3Dialog (playerui_t *plui)
{
  uiwcont_t   *windowp;
  uiselect_t  *selectdata;
  char        *dir;
  char        tbuff [200];
  const char  *defdir = NULL;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: export mp3");

  windowp = plui->wcont [PLUI_W_WINDOW];

  defdir = nlistGetStr (plui->options, EXP_MP3_DIR);
  if (defdir == NULL || ! *defdir) {
    defdir = sysvarsGetStr (SV_BDJ4_DREL_TMP);
  }

  /* CONTEXT: export as mp3: title of save dialog */
  snprintf (tbuff, sizeof (tbuff), _("Export as %s"), BDJ4_MP3_LABEL);
  selectdata = uiDialogCreateSelect (windowp,
      tbuff, defdir, NULL, NULL, NULL);
  dir = uiSelectDirDialog (selectdata);
  nlistSetStr (plui->options, EXP_MP3_DIR, dir);
  mdfree (selectdata);
  return dir;
}

static bool
pluiSwitchPage (void *udata, long pagenum)
{
  playerui_t  *plui = udata;
  int         tabid;

  logProcBegin (LOG_PROC, "pluiSwitchPage");

  if (! plui->uibuilt) {
    logProcEnd (LOG_PROC, "pluiSwitchPage", "no-ui");
    return UICB_STOP;
  }

  tabid = uinbutilIDGet (plui->nbtabid, pagenum);
  plui->currpage = pagenum;
  /* do not call set-manage-queue on the request tab */
  if (tabid == UI_TAB_MUSICQ) {
    pluiSetManageQueue (plui, pagenum);
  }
  if (tabid == UI_TAB_HISTORY) {
    pluiSetManageQueue (plui, MUSICQ_HISTORY);
  }

  if (tabid == UI_TAB_SONGSEL) {
    uiWidgetSetState (plui->wcont [PLUI_W_MENU_QE_CURR], UIWIDGET_DISABLE);
    uiWidgetSetState (plui->wcont [PLUI_W_MENU_QE_SEL], UIWIDGET_DISABLE);
    uiWidgetSetState (plui->wcont [PLUI_W_MENU_EXT_REQ], UIWIDGET_DISABLE);
  } else {
    uiWidgetSetState (plui->wcont [PLUI_W_MENU_QE_CURR], UIWIDGET_ENABLE);
    uiWidgetSetState (plui->wcont [PLUI_W_MENU_QE_SEL], UIWIDGET_ENABLE);
    if (tabid == UI_TAB_MUSICQ) {
      uiWidgetSetState (plui->wcont [PLUI_W_MENU_EXT_REQ], UIWIDGET_ENABLE);
    } else {
      uiWidgetSetState (plui->wcont [PLUI_W_MENU_EXT_REQ], UIWIDGET_DISABLE);
    }
  }

  pluiPlaybackButtonHideShow (plui, pagenum);
  logProcEnd (LOG_PROC, "pluiSwitchPage", "");
  return UICB_CONT;
}

static void
pluiPlaybackButtonHideShow (playerui_t *plui, long pagenum)
{
  int         tabid;

  tabid = uinbutilIDGet (plui->nbtabid, pagenum);

  uiWidgetHide (plui->wcont [PLUI_W_SET_PB_BUTTON]);
  if (tabid == UI_TAB_MUSICQ) {
    if (nlistGetNum (plui->options, PLUI_SHOW_EXTRA_QUEUES)) {
      uiWidgetShow (plui->wcont [PLUI_W_SET_PB_BUTTON]);
    }
  }
}

static bool
pluiProcessSetPlaybackQueue (void *udata)
{
  playerui_t      *plui = udata;

  logProcBegin (LOG_PROC, "pluiProcessSetPlaybackQueue");
  pluiSetPlaybackQueue (plui, plui->musicqManageIdx, PLUI_UPDATE_MAIN);
  logProcEnd (LOG_PROC, "pluiProcessSetPlaybackQueue", "");
  return UICB_CONT;
}

static void
pluiSetPlaybackQueue (playerui_t *plui, int newQueue, int updateFlag)
{
  char            tbuff [40];

  logProcBegin (LOG_PROC, "pluiSetPlaybackQueue");

  plui->musicqPlayIdx = newQueue;

  for (int i = 0; i < MUSICQ_PB_MAX; ++i) {
    if (! bdjoptGetNumPerQueue (OPT_Q_DISPLAY, i)) {
      /* not displayed */
      continue;
    }

    if ((int) plui->musicqPlayIdx == i) {
      uiImageSetFromPixbuf (plui->musicqImage [i], plui->wcont [PLUI_W_LED_ON]);
    } else {
      uiImageSetFromPixbuf (plui->musicqImage [i], plui->wcont [PLUI_W_LED_OFF]);
    }
  }

  /* if showextraqueues is off, reject any attempt to switch playback. */
  /* let main know what queue is being used */
  uimusicqSetPlayIdx (plui->uimusicq, plui->musicqPlayIdx);
  if (updateFlag == PLUI_UPDATE_MAIN) {
    snprintf (tbuff, sizeof (tbuff), "%d", plui->musicqPlayIdx);
    connSendMessage (plui->conn, ROUTE_MAIN, MSG_MUSICQ_SET_PLAYBACK, tbuff);
  }
  logProcEnd (LOG_PROC, "pluiSetPlaybackQueue", "");
}

static void
pluiSetManageQueue (playerui_t *plui, int mqidx)
{
  char  tbuff [40];

  if (mqidx < MUSICQ_PB_MAX) {
    int   val;

    plui->musicqRequestIdx = mqidx;
    val = nlistGetNum (plui->options, PLUI_SHOW_EXTRA_QUEUES);
    if (val) {
      uisongselSetRequestLabel (plui->uisongsel,
          bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, mqidx));
      uimusicqSetRequestLabel (plui->uimusicq,
          bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, mqidx));
    } else {
      uisongselSetRequestLabel (plui->uisongsel, "");
      uimusicqSetRequestLabel (plui->uimusicq, "");
    }
  }

  plui->musicqManageIdx = mqidx;
  uimusicqSetManageIdx (plui->uimusicq, mqidx);
  snprintf (tbuff, sizeof (tbuff), "%d", mqidx);
  connSendMessage (plui->conn, ROUTE_MAIN, MSG_MUSICQ_SET_MANAGE, tbuff);
}

static bool
pluiToggleExtraQueues (void *udata)
{
  playerui_t      *plui = udata;
  listnum_t       val;

  logProcBegin (LOG_PROC, "pluiToggleExtraQueues");
  val = nlistGetNum (plui->options, PLUI_SHOW_EXTRA_QUEUES);
  val = ! val;
  nlistSetNum (plui->options, PLUI_SHOW_EXTRA_QUEUES, val);
  /* calls the switch page handler, the managed music queue will be set */
  pluiSetExtraQueues (plui);
  if (! val) {
    pluiSetPlaybackQueue (plui, MUSICQ_PB_A, PLUI_UPDATE_MAIN);
    uisongselSetRequestLabel (plui->uisongsel, "");
    uimusicqSetRequestLabel (plui->uimusicq, "");
  } else {
    uisongselSetRequestLabel (plui->uisongsel,
        bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, plui->musicqManageIdx));
    uimusicqSetRequestLabel (plui->uimusicq,
        bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, plui->musicqManageIdx));
  }
  logProcEnd (LOG_PROC, "pluiToggleExtraQueues", "");
  return UICB_CONT;
}

static void
pluiSetExtraQueues (playerui_t *plui)
{
  int             tabid;
  int             pagenum;
  bool            show;
  bool            resetcurr = false;

  logProcBegin (LOG_PROC, "pluiSetExtraQueues");
  if (! plui->uibuilt) {
    logProcEnd (LOG_PROC, "pluiSetExtraQueues", "no-notebook");
    return;
  }

  show = nlistGetNum (plui->options, PLUI_SHOW_EXTRA_QUEUES);
  uinbutilIDStartIterator (plui->nbtabid, &pagenum);
  while ((tabid = uinbutilIDIterate (plui->nbtabid, &pagenum)) >= 0) {
    if (tabid == UI_TAB_MUSICQ && pagenum > 0) {
      if (! show && plui->currpage == pagenum) {
        resetcurr = true;
      }
      uiNotebookHideShowPage (plui->wcont [PLUI_W_NOTEBOOK], pagenum, show);
    }
  }
  if (resetcurr) {
    /* the tab currently displayed is being hidden */
    plui->currpage = 0;
  }

  pluiPlaybackButtonHideShow (plui, plui->currpage);
  logProcEnd (LOG_PROC, "pluiSetExtraQueues", "");
}

static void
pluiSetSupported (playerui_t *plui)
{
  logProcBegin (LOG_PROC, "pluiSetSupported");
  if (! plui->uibuilt) {
    logProcEnd (LOG_PROC, "pluiSetSupported", "no-ui");
    return;
  }

  if (! pliCheckSupport (plui->pliSupported, PLI_SUPPORT_SPEED)) {
    uiplayerDisableSpeed (plui->uiplayer);
  }
  if (! pliCheckSupport (plui->pliSupported, PLI_SUPPORT_SEEK)) {
    uiplayerDisableSeek (plui->uiplayer);
  }
  logProcEnd (LOG_PROC, "pluiSetSupported", "");
}

static bool
pluiToggleSwitchQueue (void *udata)
{
  playerui_t      *plui = udata;
  listnum_t       val;

  logProcBegin (LOG_PROC, "pluiToggleSwitchQueue");
  val = nlistGetNum (plui->options, PLUI_SWITCH_QUEUE_WHEN_EMPTY);
  val = ! val;
  nlistSetNum (plui->options, PLUI_SWITCH_QUEUE_WHEN_EMPTY, val);
  pluiSetSwitchQueue (plui);
  logProcEnd (LOG_PROC, "pluiToggleSwitchQueue", "");
  return UICB_CONT;
}

static void
pluiSetSwitchQueue (playerui_t *plui)
{
  char  tbuff [40];

  logProcBegin (LOG_PROC, "pluiSetSwitchQueue");
  snprintf (tbuff, sizeof (tbuff), "%" PRId64,
      nlistGetNum (plui->options, PLUI_SWITCH_QUEUE_WHEN_EMPTY));
  connSendMessage (plui->conn, ROUTE_MAIN, MSG_QUEUE_SWITCH_EMPTY, tbuff);
  logProcEnd (LOG_PROC, "pluiSetSwitchQueue", "");
}

static bool
pluiMarqueeFontSizeDialog (void *udata)
{
  playerui_t      *plui = udata;
  int             sz;

  if (plui->mqfontsizeactive) {
    return UICB_STOP;
  }

  logProcBegin (LOG_PROC, "pluiMarqueeFontSizeDialog");
  plui->mqfontsizeactive = true;

  if (! plui->fontszdialogcreated) {
    pluiCreateMarqueeFontSizeDialog (plui);
  }

  if (plui->marqueeIsMaximized) {
    sz = plui->marqueeFontSizeFS;
  } else {
    sz = plui->marqueeFontSize;
  }

  uiSpinboxSetValue (plui->wcont [PLUI_W_MQ_SZ], (double) sz);
  uiDialogShow (plui->wcont [PLUI_W_MQ_FONT_SZ_DIALOG]);

  logProcEnd (LOG_PROC, "pluiMarqueeFontSizeDialog", "");
  return UICB_CONT;
}

static void
pluiCreateMarqueeFontSizeDialog (playerui_t *plui)
{
  uiwcont_t    *vbox;
  uiwcont_t    *hbox;
  uiwcont_t    *uiwidgetp;

  logProcBegin (LOG_PROC, "pluiCreateMarqueeFontSizeDialog");

  plui->callbacks [PLUI_CB_FONT_SIZE] = callbackInitLong (
      pluiMarqueeFontSizeDialogResponse, plui);
  plui->wcont [PLUI_W_MQ_FONT_SZ_DIALOG] = uiCreateDialog (plui->wcont [PLUI_W_WINDOW],
      plui->callbacks [PLUI_CB_FONT_SIZE],
      /* CONTEXT: playerui: marquee font size dialog: window title */
      _("Marquee Font Size"),
      /* CONTEXT: playerui: marquee font size dialog: action button */
      _("Close"),
      RESPONSE_CLOSE,
      NULL
      );

  vbox = uiCreateVertBox ();
  uiDialogPackInDialog (plui->wcont [PLUI_W_MQ_FONT_SZ_DIALOG], vbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: playerui: marquee font size dialog: the font size selector */
  uiwidgetp = uiCreateColonLabel (_("Font Size"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  plui->wcont [PLUI_W_MQ_SZ] = uiSpinboxIntCreate ();
  uiSpinboxSet (plui->wcont [PLUI_W_MQ_SZ], 10.0, 300.0);
  uiSpinboxSetValue (plui->wcont [PLUI_W_MQ_SZ], 36.0);
  uiBoxPackStart (hbox, plui->wcont [PLUI_W_MQ_SZ]);

  plui->callbacks [PLUI_CB_FONT_SZ_CHG] = callbackInit (
      pluiMarqueeFontSizeChg, plui, NULL);
  uiSpinboxSetValueChangedCallback (plui->wcont [PLUI_W_MQ_SZ],
      plui->callbacks [PLUI_CB_FONT_SZ_CHG]);

  /* the dialog doesn't have any space above the buttons */
  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);
  plui->fontszdialogcreated = true;

  uiwcontFree (vbox);
  uiwcontFree (hbox);

  logProcEnd (LOG_PROC, "pluiCreateMarqueeFontSizeDialog", "");
}

static bool
pluiMarqueeFontSizeDialogResponse (void *udata, long responseid)
{
  playerui_t  *plui = udata;

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      uiwcontFree (plui->wcont [PLUI_W_MQ_FONT_SZ_DIALOG]);
      plui->wcont [PLUI_W_MQ_FONT_SZ_DIALOG] = NULL;
      plui->fontszdialogcreated = false;
      plui->mqfontsizeactive = false;
      break;
    }
    case RESPONSE_CLOSE: {
      uiWidgetHide (plui->wcont [PLUI_W_MQ_FONT_SZ_DIALOG]);
      plui->mqfontsizeactive = false;
      break;
    }
  }
  return UICB_CONT;
}

static bool
pluiMarqueeFontSizeChg (void *udata)
{
  playerui_t  *plui = udata;
  int         fontsz;
  double      value;

  value = uiSpinboxGetValue (plui->wcont [PLUI_W_MQ_SZ]);
  fontsz = (int) round (value);
  if (plui->marqueeIsMaximized) {
    plui->marqueeFontSizeFS = fontsz;
  } else {
    plui->marqueeFontSize = fontsz;
  }
  mstimeset (&plui->marqueeFontSizeCheck, 100);

  return UICB_CONT;
}

static bool
pluiMarqueeRecover (void *udata)
{
  playerui_t  *plui = udata;

  if (plui->marqueeoff) {
    return UICB_CONT;
  }

  connSendMessage (plui->conn, ROUTE_MARQUEE, MSG_WINDOW_FIND, NULL);
  return UICB_CONT;
}


static void
pluisetMarqueeIsMaximized (playerui_t *plui, char *args)
{
  int   val = atoi (args);

  plui->marqueeIsMaximized = val;
}

static void
pluisetMarqueeFontSizes (playerui_t *plui, char *args)
{
  char      *p;
  char      *tokstr;

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  plui->marqueeFontSize = atoi (p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  plui->marqueeFontSizeFS = atoi (p);
}

static bool
pluiQueueProcess (void *udata, long dbidx)
{
  playerui_t  *plui = udata;
  long        loc;
  char        tbuff [100];
  int         mqidx;

  mqidx = plui->musicqRequestIdx;
  loc = uimusicqGetSelectLocation (plui->uimusicq, mqidx);

  /* increment the location by 1 as the tree-view index is one less than */
  /* the music queue index */
  snprintf (tbuff, sizeof (tbuff), "%d%c%ld%c%ld", mqidx,
      MSG_ARGS_RS, loc + 1, MSG_ARGS_RS, dbidx);
  connSendMessage (plui->conn, ROUTE_MAIN, MSG_MUSICQ_INSERT, tbuff);
  return UICB_CONT;
}

static bool
pluiSongSaveCallback (void *udata, long dbidx)
{
  playerui_t  *plui = udata;
  char        tmp [40];

  songdbWriteDB (plui->songdb, dbidx);

  /* the database has been updated, tell the other processes to reload  */
  /* this particular entry */
  snprintf (tmp, sizeof (tmp), "%ld", dbidx);
  connSendMessage (plui->conn, ROUTE_STARTERUI, MSG_DB_ENTRY_UPDATE, tmp);

  uisongselPopulateData (plui->uisongsel);

  return UICB_CONT;
}

static bool
pluiClearQueueCallback (void *udata)
{
  playerui_t  *plui = udata;
  char        tmp [40];

  snprintf (tmp, sizeof (tmp), "%d", plui->musicqManageIdx);
  connSendMessage (plui->conn, ROUTE_MAIN, MSG_PL_CLEAR_QUEUE, tmp);
  return UICB_CONT;
}

static void
pluiPushHistory (playerui_t *plui, const char *args)
{
  dbidx_t     dbidx;
  char        tbuff [100];

  dbidx = atol (args);

  snprintf (tbuff, sizeof (tbuff), "%d%c%d%c%d", MUSICQ_HISTORY,
      MSG_ARGS_RS, QUEUE_LOC_LAST, MSG_ARGS_RS, dbidx);
  connSendMessage (plui->conn, ROUTE_MAIN, MSG_MUSICQ_INSERT, tbuff);
}

static bool
pluiRequestExternalDialog (void *udata)
{
  playerui_t    *plui = udata;
  bool          rc;

  rc = uiextreqDialog (plui->uiextreq, NULL);
  return rc;
}

static bool
pluiExtReqCallback (void *udata)
{
  playerui_t    *plui = udata;
  song_t        *song;

  song = uiextreqGetSong (plui->uiextreq);

  if (song != NULL) {
    dbidx_t     dbidx;
    char        *tbuff;
    char        *tmp;

    /* add to the player's copy of the database */
    dbidx = dbAddTemporarySong (plui->musicdb, song);

    tbuff = mdmalloc (BDJMSG_MAX);
    tmp = songCreateSaveData (song);
    snprintf (tbuff, BDJMSG_MAX, "%s%c%d%c%s",
        songGetStr (song, TAG_URI),
        MSG_ARGS_RS, dbidx,
        MSG_ARGS_RS, tmp);
    connSendMessage (plui->conn, ROUTE_MAIN, MSG_DB_ENTRY_TEMP_ADD, tbuff);
    dataFree (tbuff);
    dataFree (tmp);

    if (plui->extreqRow >= 0) {
      uimusicqSetSelectLocation (plui->uimusicq, plui->musicqRequestIdx,
          plui->extreqRow);
      plui->extreqRow = -1;
    }

    pluiQueueProcess (plui, dbidx);
  }
  return UICB_CONT;
}

static bool
pluiQuickEditCurrent (void *udata)
{
  playerui_t    *plui = udata;
  bool          rc;
  dbidx_t       dbidx = -1;
  double        vol, speed;
  int           basevol;

  dbidx = uiplayerGetCurrSongIdx (plui->uiplayer);
  uiplayerGetVolumeSpeed (plui->uiplayer, &basevol, &vol, &speed);

  plui->resetvolume = RESET_VOL_CURR;
  rc = uiqeDialog (plui->uiqe, dbidx, speed, vol, basevol);
  return rc;
}

static bool
pluiQuickEditSelected (void *udata)
{
  playerui_t    *plui = udata;
  bool          rc;
  dbidx_t       dbidx;
  double        vol, speed;
  int           basevol;

  dbidx = uimusicqGetSelectionDbidx (plui->uimusicq);
  /* don't need vol and speed */
  uiplayerGetVolumeSpeed (plui->uiplayer, &basevol, &vol, &speed);
  plui->resetvolume = RESET_VOL_NO;
  rc = uiqeDialog (plui->uiqe, dbidx, LIST_DOUBLE_INVALID, LIST_DOUBLE_INVALID, basevol);
  return rc;
}

static bool
pluiQuickEditCallback (void *udata)
{
  playerui_t        *plui = udata;
  const uiqesave_t  *qeresp;
  song_t            *song;
  dbidx_t           dbidx;

  if (plui == NULL || plui->uiqe == NULL) {
    return UICB_CONT;
  }

  qeresp = uiqeGetResponseData (plui->uiqe);

  if (qeresp == NULL) {
    return UICB_CONT;
  }

  song = dbGetByIdx (plui->musicdb, qeresp->dbidx);
  if (song == NULL) {
    return UICB_CONT;
  }
  songSetNum (song, TAG_SPEEDADJUSTMENT, (int) qeresp->speed);
  songSetDouble (song, TAG_VOLUMEADJUSTPERC, qeresp->voladj);
  songSetNum (song, TAG_DANCERATING, qeresp->rating);
  songSetNum (song, TAG_DANCELEVEL, qeresp->level);

  pluiSongSaveCallback (plui, qeresp->dbidx);

  dbidx = uiplayerGetCurrSongIdx (plui->uiplayer);
  /* what to do if the current song has moved on to the next? */
  if (qeresp->dbidx == dbidx &&
      plui->resetvolume == RESET_VOL_CURR) {
    connSendMessage (plui->conn, ROUTE_PLAYER, MSG_PLAY_RESET_VOLUME, NULL);
  }

  return UICB_CONT;
}

static bool
pluiReload (void *udata)
{
  playerui_t    *plui = udata;

  plui->inreload = true;

  for (int mqidx = 0; mqidx < MUSICQ_DISP_MAX; ++mqidx) {
    char    tmp [100];
    char    tbuff [200];

    snprintf (tbuff, sizeof (tbuff), "%d%c%d", mqidx, MSG_ARGS_RS, 1);
    connSendMessage (plui->conn, ROUTE_MAIN, MSG_MUSICQ_TRUNCATE, tbuff);
    snprintf (tmp, sizeof (tmp), "%s-%d-%d", RELOAD_FN,
        (int) sysvarsGetNum (SVL_PROFILE_IDX), mqidx);
    msgbuildQueuePlaylist (tbuff, sizeof (tbuff), mqidx, tmp, EDIT_FALSE);
    connSendMessage (plui->conn, ROUTE_MAIN, MSG_QUEUE_PLAYLIST, tbuff);
  }

  pluiReloadCurrent (plui);

  plui->inreload = false;
  return UICB_CONT;
}

static void
pluiReloadCurrent (playerui_t *plui)
{
  datafile_t      *reloaddf;
  nlist_t         *reloaddata;
  char            tbuff [200];
  dbidx_t         dbidx;
  const char      *nm;
  song_t          *song;

  pathbldMakePath (tbuff, sizeof (tbuff),
      RELOAD_CURR_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  reloaddf = datafileAllocParse ("reload-curr", DFTYPE_KEY_VAL, tbuff,
      reloaddfkeys, RELOAD_DFKEY_COUNT, DF_NO_OFFSET, NULL);
  reloaddata = datafileGetList (reloaddf);
  if (reloaddata == NULL) {
    datafileFree (reloaddf);
    return;
  }

  nm = nlistGetStr (reloaddata, RELOAD_CURR);
  song = dbGetByName (plui->musicdb, nm);
  if (song != NULL) {
    dbidx = songGetNum (song, TAG_DBIDX);
    snprintf (tbuff, sizeof (tbuff), "%d%c%d%c%d", MUSICQ_PB_A,
        MSG_ARGS_RS, 0, MSG_ARGS_RS, dbidx);
    connSendMessage (plui->conn, ROUTE_MAIN, MSG_MUSICQ_INSERT, tbuff);
    snprintf (tbuff, sizeof (tbuff), "%d%c%d", MUSICQ_PB_A, MSG_ARGS_RS, 1);
    connSendMessage (plui->conn, ROUTE_MAIN, MSG_MUSICQ_MOVE_UP, tbuff);
  }
  datafileFree (reloaddf);
}

static void
pluiReloadSave (playerui_t *plui, int mqidx)
{
  char      tmp [MAXPATHLEN];

  if (plui->inreload) {
    return;
  }

  /* any time any music queue data is received, disable the */
  /* reload menu item */
  if (! plui->reloadinit) {
    uiWidgetSetState (plui->wcont [PLUI_W_MENU_RELOAD], UIWIDGET_DISABLE);

    /* any old reload data must be removed */
    /* this only needs to be done once */
    for (int tmqidx = 0; tmqidx < MUSICQ_DISP_MAX; ++tmqidx) {
      snprintf (tmp, sizeof (tmp), "%s-%d-%d", RELOAD_FN,
          (int) sysvarsGetNum (SVL_PROFILE_IDX), tmqidx);
      playlistDelete (tmp);
    }

    plui->reloadinit = true;
  }

  /* reload: save history also */
  if (mqidx < MUSICQ_DISP_MAX) {
    uimusicqSetManageIdx (plui->uimusicq, mqidx);
    /* the profile number must be added to the filename */
    /* as playlists are global and not per-profile */
    snprintf (tmp, sizeof (tmp), "%s-%d-%d", RELOAD_FN,
        (int) sysvarsGetNum (SVL_PROFILE_IDX), mqidx);
    uimusicqSave (plui->uimusicq, tmp);
    playlistCheckAndCreate (tmp, PLTYPE_SONGLIST);
    uimusicqSetManageIdx (plui->uimusicq, plui->musicqManageIdx);
  }
}

static void
pluiReloadSaveCurrent (playerui_t *plui)
{
  datafile_t      *reloaddf;
  nlist_t         *reloaddata;
  char            tbuff [MAXPATHLEN];
  dbidx_t         dbidx;
  song_t          *song;

  if (plui->inreload) {
    return;
  }

  pathbldMakePath (tbuff, sizeof (tbuff),
      RELOAD_CURR_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  reloaddf = datafileAllocParse ("save-curr", DFTYPE_KEY_VAL, tbuff,
      reloaddfkeys, RELOAD_DFKEY_COUNT, DF_NO_OFFSET, NULL);
  reloaddata = nlistAlloc ("reload-curr", LIST_ORDERED, NULL);
  dbidx = uiplayerGetCurrSongIdx (plui->uiplayer);
  song = dbGetByIdx (plui->musicdb, dbidx);
  if (song != NULL) {
    nlistSetStr (reloaddata, RELOAD_CURR, songGetStr (song, TAG_URI));
    datafileSave (reloaddf, NULL, reloaddata, DF_NO_OFFSET, 1);
  }
  nlistFree (reloaddata);
  datafileFree (reloaddf);
}

static bool
pluiKeyEvent (void *udata)
{
  playerui_t  *plui = udata;

  if (uiKeyIsPressEvent (plui->wcont [PLUI_W_KEY_HNDLR]) &&
      uiKeyIsAudioPlayKey (plui->wcont [PLUI_W_KEY_HNDLR])) {
    connSendMessage (plui->conn, ROUTE_MAIN, MSG_CMD_PLAYPAUSE, NULL);
    return UICB_STOP;
  }
  /* isaudiopausekey() also checks for the stop key */
  if (uiKeyIsPressEvent (plui->wcont [PLUI_W_KEY_HNDLR]) &&
      uiKeyIsAudioPauseKey (plui->wcont [PLUI_W_KEY_HNDLR])) {
    connSendMessage (plui->conn, ROUTE_PLAYER, MSG_PLAY_PAUSE, NULL);
    return UICB_STOP;
  }
  if (uiKeyIsPressEvent (plui->wcont [PLUI_W_KEY_HNDLR]) &&
      uiKeyIsAudioNextKey (plui->wcont [PLUI_W_KEY_HNDLR])) {
    connSendMessage (plui->conn, ROUTE_PLAYER, MSG_PLAY_NEXTSONG, NULL);
    return UICB_STOP;
  }
  if (uiKeyIsPressEvent (plui->wcont [PLUI_W_KEY_HNDLR]) &&
      uiKeyIsAudioPrevKey (plui->wcont [PLUI_W_KEY_HNDLR])) {
    connSendMessage (plui->conn, ROUTE_PLAYER, MSG_PLAY_SONG_BEGIN, NULL);
    return UICB_STOP;
  }

  return UICB_CONT;
}

static bool
pluiExportMP3 (void *udata)
{
  playerui_t  *plui = udata;

  if (plui->expmp3state != BDJ4_STATE_OFF) {
    return UICB_STOP;
  }

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: export mp3");
  plui->expmp3state = BDJ4_STATE_START;
  return UICB_CONT;
}

static bool
pluiDragDropCallback (void *udata, const char *uri, int row)
{
  playerui_t        *plui = udata;
  static const char *filepfx = "file://";
  int               filepfxlen = strlen (filepfx);

  if (strncmp (uri, filepfx, filepfxlen) != 0) {
    return UICB_STOP;
  }

  plui->musicqRequestIdx = plui->musicqManageIdx;
  plui->extreqRow = row;

  uiextreqDialog (plui->uiextreq, uri + filepfxlen);
  return UICB_CONT;
}
