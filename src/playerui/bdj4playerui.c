/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
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
#include "bdjvarsdfload.h"
#include "conn.h"
#include "datafile.h"
#include "dispsel.h"
#include "localeutil.h"
#include "lock.h"
#include "log.h"
#include "mdebug.h"
#include "msgparse.h"
#include "musicq.h"
#include "ossignal.h"
#include "osuiutils.h"
#include "pathbld.h"
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
#include "uireqext.h"
#include "uisongfilter.h"
#include "uisongsel.h"
#include "uiutils.h"

enum {
  PLUI_MENU_CB_PLAY_QUEUE,
  PLUI_MENU_CB_EXTRA_QUEUE,
  PLUI_MENU_CB_SWITCH_QUEUE,
  PLUI_MENU_CB_REQ_EXTERNAL,
  PLUI_MENU_CB_MQ_FONT_SZ,
  PLUI_MENU_CB_MQ_FIND,
  PLUI_CB_NOTEBOOK,
  PLUI_CB_CLOSE,
  PLUI_CB_PLAYBACK_QUEUE,
  PLUI_CB_FONT_SIZE,
  PLUI_CB_QUEUE_SL,
  PLUI_CB_SONG_SAVE,
  PLUI_CB_CLEAR_QUEUE,
  PLUI_CB_REQ_EXT,
  PLUI_CB_MAX,
};

typedef struct {
  progstate_t     *progstate;
  char            *locknm;
  conn_t          *conn;
  musicdb_t       *musicdb;
  musicqidx_t     musicqPlayIdx;
  musicqidx_t     musicqLastManageIdx;
  musicqidx_t     musicqManageIdx;
  dispsel_t       *dispsel;
  int             dbgflags;
  int             marqueeIsMaximized;
  int             marqueeFontSize;
  int             marqueeFontSizeFS;
  mstime_t        marqueeFontSizeCheck;
  int             stopwaitcount;
  mstime_t        clockCheck;
  uisongfilter_t  *uisongfilter;
  uireqext_t      *uireqext;
  /* notebook */
  UIWidget        notebook;
  uiutilsnbtabid_t *nbtabid;
  int             currpage;
  UIWidget        window;
  UIWidget        vbox;
  UICallback      callbacks [PLUI_CB_MAX];
  UIWidget        clock;
  UIWidget        musicqImage [MUSICQ_PB_MAX];
  uibutton_t      *setPlaybackButton;
  UIWidget        ledoffPixbuf;
  UIWidget        ledonPixbuf;
  UIWidget        marqueeFontSizeDialog;
  UIWidget        marqueeSpinBox;
  /* ui major elements */
  UIWidget        statusMsg;
  uiplayer_t      *uiplayer;
  uimusicq_t      *uimusicq;
  uisongsel_t     *uisongsel;
  /* options */
  datafile_t      *optiondf;
  nlist_t         *options;
  /* flags */
  bool            uibuilt : 1;
  bool            fontszdialogcreated : 1;
  bool            mainalready : 1;
} playerui_t;

static datafilekey_t playeruidfkeys [] = {
  { "FILTER_POS_X",             SONGSEL_FILTER_POSITION_X,    VALUE_NUM, NULL, -1 },
  { "FILTER_POS_Y",             SONGSEL_FILTER_POSITION_Y,    VALUE_NUM, NULL, -1 },
  { "MQ_REQ_EXT_X",             MQ_REQ_EXT_POSITION_X,        VALUE_NUM, NULL, -1 },
  { "MQ_REQ_EXT_Y",             MQ_REQ_EXT_POSITION_Y,        VALUE_NUM, NULL, -1 },
  { "PLUI_POS_X",               PLUI_POSITION_X,              VALUE_NUM, NULL, -1 },
  { "PLUI_POS_Y",               PLUI_POSITION_Y,              VALUE_NUM, NULL, -1 },
  { "PLUI_SIZE_X",              PLUI_SIZE_X,                  VALUE_NUM, NULL, -1 },
  { "PLUI_SIZE_Y",              PLUI_SIZE_Y,                  VALUE_NUM, NULL, -1 },
  { "SHOW_EXTRA_QUEUES",        PLUI_SHOW_EXTRA_QUEUES,       VALUE_NUM, NULL, -1 },
  { "SORT_BY",                  SONGSEL_SORT_BY,              VALUE_STR, NULL, -1 },
  { "SWITCH_QUEUE_WHEN_EMPTY",  PLUI_SWITCH_QUEUE_WHEN_EMPTY, VALUE_NUM, NULL, -1 },
};
enum {
  PLAYERUI_DFKEY_COUNT = (sizeof (playeruidfkeys) / sizeof (datafilekey_t)),
};

enum {
  PLUI_UPDATE_MAIN,
  PLUI_NO_UPDATE,
};

static bool     pluiConnectingCallback (void *udata, programstate_t programState);
static bool     pluiHandshakeCallback (void *udata, programstate_t programState);
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
/* queue selection handlers */
static bool     pluiSwitchPage (void *udata, long pagenum);
static void     pluiPlaybackButtonHideShow (playerui_t *plui, long pagenum);
static bool     pluiProcessSetPlaybackQueue (void *udata);
static void     pluiSetPlaybackQueue (playerui_t *plui, musicqidx_t newqueue, int updateFlag);
static void     pluiSetManageQueue (playerui_t *plui, musicqidx_t newqueue);
/* option handlers */
static bool     pluiToggleExtraQueues (void *udata);
static void     pluiSetExtraQueues (playerui_t *plui);
static bool     pluiToggleSwitchQueue (void *udata);
static void     pluiSetSwitchQueue (playerui_t *plui);
static bool     pluiMarqueeFontSizeDialog (void *udata);
static void     pluiCreateMarqueeFontSizeDialog (playerui_t *plui);
static bool     pluiMarqueeFontSizeDialogResponse (void *udata, long responseid);
static void     pluiMarqueeFontSizeChg (GtkSpinButton *fb, gpointer udata);
static bool     pluiMarqueeFind (void *udata);
static void     pluisetMarqueeIsMaximized (playerui_t *plui, char *args);
static void     pluisetMarqueeFontSizes (playerui_t *plui, char *args);
static bool     pluiQueueProcess (void *udata, long dbidx, int mqidx);
static bool     pluiSongSaveCallback (void *udata, long dbidx);
static bool     pluiClearQueueCallback (void *udata);
static void     pluiPushHistory (playerui_t *plui, const char *args);
static bool     pluiRequestExternalDialog (void *udata);
static bool     pluiReqextCallback (void *udata);

static int gKillReceived = 0;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  playerui_t      plui;
  char            *uifont;
  char            tbuff [MAXPATHLEN];

#if BDJ4_MEM_DEBUG
  mdebugInit ("plui");
#endif


  uiutilsUIWidgetInit (&plui.window);
  uiutilsUIWidgetInit (&plui.clock);
  uiutilsUIWidgetInit (&plui.notebook);
  uiutilsUIWidgetInit (&plui.marqueeFontSizeDialog);
  plui.progstate = progstateInit ("playerui");
  progstateSetCallback (plui.progstate, STATE_CONNECTING,
      pluiConnectingCallback, &plui);
  progstateSetCallback (plui.progstate, STATE_WAIT_HANDSHAKE,
      pluiHandshakeCallback, &plui);
  plui.uiplayer = NULL;
  plui.uimusicq = NULL;
  plui.uisongsel = NULL;
  plui.musicqPlayIdx = MUSICQ_PB_A;
  plui.musicqLastManageIdx = MUSICQ_PB_A;
  plui.musicqManageIdx = MUSICQ_PB_A;
  plui.marqueeIsMaximized = false;
  plui.marqueeFontSize = 36;
  plui.marqueeFontSizeFS = 60;
  mstimeset (&plui.marqueeFontSizeCheck, 3600000);
  mstimeset (&plui.clockCheck, 0);
  plui.stopwaitcount = 0;
  plui.nbtabid = uiutilsNotebookIDInit ();
  plui.uisongfilter = NULL;
  plui.uireqext = NULL;
  plui.uibuilt = false;
  plui.fontszdialogcreated = false;
  plui.currpage = 0;
  plui.mainalready = false;
  plui.setPlaybackButton = NULL;

  osSetStandardSignals (pluiSigHandler);

  plui.dbgflags = bdj4startup (argc, argv, &plui.musicdb,
      "plui", ROUTE_PLAYERUI, BDJ4_INIT_NONE);
  logProcBegin (LOG_PROC, "playerui");

  plui.dispsel = dispselAlloc ();

  listenPort = bdjvarsGetNum (BDJVL_PLAYERUI_PORT);
  plui.conn = connInit (ROUTE_PLAYERUI);

  pathbldMakePath (tbuff, sizeof (tbuff),
      PLAYERUI_OPT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  plui.optiondf = datafileAllocParse ("playerui-opt", DFTYPE_KEY_VAL, tbuff,
      playeruidfkeys, PLAYERUI_DFKEY_COUNT);
  plui.options = datafileGetList (plui.optiondf);
  if (plui.options == NULL) {
    datafileFree (plui.optiondf);
    plui.optiondf = NULL;
    plui.options = nlistAlloc ("playerui-opt", LIST_ORDERED, NULL);

    nlistSetNum (plui.options, PLUI_SHOW_EXTRA_QUEUES, false);
    nlistSetNum (plui.options, PLUI_SWITCH_QUEUE_WHEN_EMPTY, false);
    nlistSetNum (plui.options, SONGSEL_FILTER_POSITION_X, -1);
    nlistSetNum (plui.options, SONGSEL_FILTER_POSITION_Y, -1);
    nlistSetNum (plui.options, PLUI_POSITION_X, -1);
    nlistSetNum (plui.options, PLUI_POSITION_Y, -1);
    nlistSetNum (plui.options, PLUI_SIZE_X, 1000);
    nlistSetNum (plui.options, PLUI_SIZE_Y, 600);
    nlistSetNum (plui.options, MQ_REQ_EXT_POSITION_X, -1);
    nlistSetNum (plui.options, MQ_REQ_EXT_POSITION_Y, -1);
    nlistSetStr (plui.options, SONGSEL_SORT_BY, "TITLE");
  }

  uiUIInitialize ();
  uifont = bdjoptGetStr (OPT_MP_UIFONT);
  uiSetUIFont (uifont);

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
  connSendMessage (plui->conn, ROUTE_STARTERUI, MSG_STOP_MAIN, NULL);

  uiWindowGetSize (&plui->window, &x, &y);
  nlistSetNum (plui->options, PLUI_SIZE_X, x);
  nlistSetNum (plui->options, PLUI_SIZE_Y, y);
  uiWindowGetPosition (&plui->window, &x, &y, &ws);
  nlistSetNum (plui->options, PLUI_POSITION_X, x);
  nlistSetNum (plui->options, PLUI_POSITION_Y, y);

  connDisconnect (plui->conn, ROUTE_STARTERUI);
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
  char          fn [MAXPATHLEN];

  logProcBegin (LOG_PROC, "pluiClosingCallback");

  uiCloseWindow (&plui->window);
  uiWidgetClearPersistent (&plui->ledonPixbuf);
  uiWidgetClearPersistent (&plui->ledoffPixbuf);
  uiButtonFree (plui->setPlaybackButton);

  pathbldMakePath (fn, sizeof (fn),
      PLAYERUI_OPT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("playerui", fn, playeruidfkeys, PLAYERUI_DFKEY_COUNT, plui->options, 0);

  bdj4shutdown (ROUTE_PLAYERUI, plui->musicdb);
  dispselFree (plui->dispsel);

  uiutilsNotebookIDFree (plui->nbtabid);
  uisfFree (plui->uisongfilter);
  uireqextFree (plui->uireqext);
  if (plui->optiondf != NULL) {
    datafileFree (plui->optiondf);
  } else if (plui->options != NULL) {
    nlistFree (plui->options);
  }

  uiplayerFree (plui->uiplayer);
  uimusicqFree (plui->uimusicq);
  uisongselFree (plui->uisongsel);
  uiCleanup ();

  logProcEnd (LOG_PROC, "pluiClosingCallback", "");
  return STATE_FINISHED;
}

static void
pluiBuildUI (playerui_t *plui)
{
  UIWidget    menubar;
  UIWidget    menu;
  UIWidget    menuitem;
  UIWidget    hbox;
  UIWidget    uiwidget;
  UIWidget    *uiwidgetp;
  uibutton_t  *uibutton;
  char        *str;
  char        imgbuff [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];
  int         x, y;

  logProcBegin (LOG_PROC, "pluiBuildUI");

  pathbldMakePath (tbuff, sizeof (tbuff),  "led_off", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DIR_IMG);
  uiImageFromFile (&plui->ledoffPixbuf, tbuff);
  uiImageGetPixbuf (&plui->ledoffPixbuf);
  uiWidgetMakePersistent (&plui->ledoffPixbuf);

  pathbldMakePath (tbuff, sizeof (tbuff),  "led_on", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DIR_IMG);
  uiImageFromFile (&plui->ledonPixbuf, tbuff);
  uiImageGetPixbuf (&plui->ledonPixbuf);
  uiWidgetMakePersistent (&plui->ledonPixbuf);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_player", BDJ4_IMG_SVG_EXT, PATHBLD_MP_DIR_IMG);
  uiutilsUICallbackInit (&plui->callbacks [PLUI_CB_CLOSE],
      pluiCloseWin, plui, NULL);
  /* CONTEXT: playerui: main window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Player"),
      bdjoptGetStr (OPT_P_PROFILENAME));
  uiCreateMainWindow (&plui->window, &plui->callbacks [PLUI_CB_CLOSE],
      tbuff, imgbuff);

  pluiInitializeUI (plui);

  uiCreateVertBox (&plui->vbox);
  uiBoxPackInWindow (&plui->window, &plui->vbox);
  uiWidgetSetAllMargins (&plui->vbox, 2);

  /* menu */
  uiutilsAddAccentColorDisplay (&plui->vbox, &hbox, &uiwidget);
  uiWidgetExpandHoriz (&hbox);

  uiCreateMenubar (&menubar);
  uiBoxPackStart (&hbox, &menubar);

  uiCreateLabel (&plui->clock, "");
  uiBoxPackEnd (&hbox, &plui->clock);
  uiWidgetSetMarginStart (&plui->clock, 4);
  uiWidgetDisable (&plui->clock);

  uiCreateLabel (&uiwidget, "");
  uiLabelSetColor (&uiwidget, bdjoptGetStr (OPT_P_UI_ERROR_COL));
  uiBoxPackEnd (&hbox, &uiwidget);
  uiutilsUIWidgetCopy (&plui->statusMsg, &uiwidget);

  /* CONTEXT: playerui: menu selection: options for the player */
  uiMenuCreateItem (&menubar, &menuitem, _("Options"), NULL);

  uiCreateSubMenu (&menuitem, &menu);

  uiutilsUICallbackInit (&plui->callbacks [PLUI_MENU_CB_EXTRA_QUEUE],
      pluiToggleExtraQueues, plui, NULL);
  /* CONTEXT: playerui: menu checkbox: show the extra queues (in addition to the main music queue) */
  uiMenuCreateCheckbox (&menu, &menuitem, _("Show Extra Queues"),
      nlistGetNum (plui->options, PLUI_SHOW_EXTRA_QUEUES),
      &plui->callbacks [PLUI_MENU_CB_EXTRA_QUEUE]);

  uiutilsUICallbackInit (&plui->callbacks [PLUI_MENU_CB_SWITCH_QUEUE],
      pluiToggleSwitchQueue, plui, NULL);
  /* CONTEXT: playerui: menu checkbox: when a queue is emptied, switch playback to the next queue */
  uiMenuCreateCheckbox (&menu, &menuitem, _("Switch Queue When Empty"),
      nlistGetNum (plui->options, PLUI_SWITCH_QUEUE_WHEN_EMPTY),
      &plui->callbacks [PLUI_MENU_CB_SWITCH_QUEUE]);

  /* CONTEXT: playerui: menu selection: actions for the player */
  uiMenuCreateItem (&menubar, &menuitem, _("Actions"), NULL);

  uiCreateSubMenu (&menuitem, &menu);

  uiutilsUICallbackInit (&plui->callbacks [PLUI_MENU_CB_REQ_EXTERNAL],
      pluiRequestExternalDialog, plui, NULL);
  /* CONTEXT: playerui: menu selection: action: request external */
  uiMenuCreateItem (&menu, &menuitem, _("Request External"),
      &plui->callbacks [PLUI_MENU_CB_REQ_EXTERNAL]);

  /* CONTEXT: playerui: menu selection: marquee related options */
  uiMenuCreateItem (&menubar, &menuitem, _("Marquee"), NULL);

  uiCreateSubMenu (&menuitem, &menu);

  uiutilsUICallbackInit (&plui->callbacks [PLUI_MENU_CB_MQ_FONT_SZ],
      pluiMarqueeFontSizeDialog, plui, NULL);
  /* CONTEXT: playerui: menu selection: marquee: change the marquee font size */
  uiMenuCreateItem (&menu, &menuitem, _("Font Size"),
      &plui->callbacks [PLUI_MENU_CB_MQ_FONT_SZ]);

  uiutilsUICallbackInit (&plui->callbacks [PLUI_MENU_CB_MQ_FIND],
      pluiMarqueeFind, plui, NULL);
  /* CONTEXT: playerui: menu selection: marquee: bring the marquee window back to the main screen */
  uiMenuCreateItem (&menu, &menuitem, _("Recover Marquee"),
      &plui->callbacks [PLUI_MENU_CB_MQ_FIND]);

  /* player */
  uiwidgetp = uiplayerBuildUI (plui->uiplayer);
  uiBoxPackStart (&plui->vbox, uiwidgetp);

  uiCreateNotebook (&plui->notebook);
  uiBoxPackStartExpand (&plui->vbox, &plui->notebook);

  uiutilsUICallbackLongInit (&plui->callbacks [PLUI_CB_NOTEBOOK],
      pluiSwitchPage, plui);
  uiNotebookSetCallback (&plui->notebook, &plui->callbacks [PLUI_CB_NOTEBOOK]);

  uiutilsUICallbackInit (&plui->callbacks [PLUI_CB_PLAYBACK_QUEUE],
      pluiProcessSetPlaybackQueue, plui, NULL);
  uibutton = uiCreateButton (&plui->callbacks [PLUI_CB_PLAYBACK_QUEUE],
      /* CONTEXT: playerui: select the current queue for playback */
      _("Set Queue for Playback"), NULL);
  plui->setPlaybackButton = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiNotebookSetActionWidget (&plui->notebook, uiwidgetp);
  uiWidgetShowAll (uiwidgetp);

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

    uiwidgetp = uimusicqBuildUI (plui->uimusicq, &plui->window, i,
        &plui->statusMsg, NULL);
    uiCreateHorizBox (&hbox);
    if (tabtype == UI_TAB_HISTORY) {
      /* CONTEXT: playerui: name of the history tab : displayed played songs */
      str = _("History");
    } else {
      str = bdjoptGetStrPerQueue (OPT_Q_QUEUE_NAME, i);
    }
    uiCreateLabel (&uiwidget, str);
    uiBoxPackStart (&hbox, &uiwidget);
    if (i < MUSICQ_HISTORY) {
      uiImageNew (&plui->musicqImage [i]);
      uiImageSetFromPixbuf (&plui->musicqImage [i], &plui->ledonPixbuf);
      uiWidgetSetMarginStart (&plui->musicqImage [i], 1);
      uiBoxPackStart (&hbox, &plui->musicqImage [i]);
    }

    uiNotebookAppendPage (&plui->notebook, uiwidgetp, &hbox);
    uiutilsNotebookIDAdd (plui->nbtabid, tabtype);
    uiWidgetShowAll (&hbox);
  }

  /* request tab */
  uiwidgetp = uisongselBuildUI (plui->uisongsel, &plui->window);
  /* CONTEXT: playerui: name of request tab : lists the songs in the database */
  uiCreateLabel (&uiwidget, _("Request"));
  uiNotebookAppendPage (&plui->notebook, uiwidgetp, &uiwidget);
  uiutilsNotebookIDAdd (plui->nbtabid, UI_TAB_SONGSEL);

  x = nlistGetNum (plui->options, PLUI_SIZE_X);
  y = nlistGetNum (plui->options, PLUI_SIZE_Y);
  uiWindowSetDefaultSize (&plui->window, x, y);

  uiWidgetShowAll (&plui->window);

  x = nlistGetNum (plui->options, PLUI_POSITION_X);
  y = nlistGetNum (plui->options, PLUI_POSITION_Y);
  uiWindowMove (&plui->window, x, y, -1);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_player", BDJ4_IMG_PNG_EXT, PATHBLD_MP_DIR_IMG);
  osuiSetIcon (imgbuff);
  pluiPlaybackButtonHideShow (plui, 0);

  uiutilsUICallbackLongInit (&plui->callbacks [PLUI_CB_SONG_SAVE],
      pluiSongSaveCallback, plui);
  uimusicqSetSongSaveCallback (plui->uimusicq, &plui->callbacks [PLUI_CB_SONG_SAVE]);
  uisongselSetSongSaveCallback (plui->uisongsel, &plui->callbacks [PLUI_CB_SONG_SAVE]);

  uiutilsUICallbackInit (&plui->callbacks [PLUI_CB_CLEAR_QUEUE],
      pluiClearQueueCallback, plui, NULL);
  uimusicqSetClearQueueCallback (plui->uimusicq, &plui->callbacks [PLUI_CB_CLEAR_QUEUE]);

  plui->uibuilt = true;

  logProcEnd (LOG_PROC, "pluiBuildUI", "");
}

static void
pluiInitializeUI (playerui_t *plui)
{
  plui->uiplayer = uiplayerInit (plui->progstate, plui->conn, plui->musicdb);

  plui->uireqext = uireqextInit (&plui->window, plui->options);
  uiutilsUICallbackInit (
      &plui->callbacks [PLUI_CB_REQ_EXT], pluiReqextCallback,
      plui, "musicq: request external response");
  uireqextSetResponseCallback (plui->uireqext, &plui->callbacks [PLUI_CB_REQ_EXT]);

  plui->uimusicq = uimusicqInit ("plui", plui->conn, plui->musicdb,
      plui->dispsel, DISP_SEL_MUSICQ);

  plui->uisongfilter = uisfInit (&plui->window, plui->options,
      SONG_FILTER_FOR_PLAYBACK);
  plui->uisongsel = uisongselInit ("plui-req", plui->conn, plui->musicdb,
      plui->dispsel, NULL, plui->options,
      plui->uisongfilter, DISP_SEL_REQUEST);
  uiutilsUICallbackLongIntInit (&plui->callbacks [PLUI_CB_QUEUE_SL],
      pluiQueueProcess, plui);
  uisongselSetQueueCallback (plui->uisongsel,
      &plui->callbacks [PLUI_CB_QUEUE_SL]);
  uimusicqSetQueueCallback (plui->uimusicq,
      &plui->callbacks [PLUI_CB_QUEUE_SL]);
}


static int
pluiMainLoop (void *tplui)
{
  playerui_t  *plui = tplui;
  int         stop = FALSE;

  uiUIProcessEvents ();

  if (! progstateIsRunning (plui->progstate)) {
    progstateProcess (plui->progstate);
    if (progstateCurrState (plui->progstate) == STATE_CLOSED) {
      stop = true;
    }
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      progstateShutdownProcess (plui->progstate);
      gKillReceived = 0;
    }
    return stop;
  }

  if (mstimeCheck (&plui->clockCheck)) {
    pluiClock (plui);
  }

  if (mstimeCheck (&plui->marqueeFontSizeCheck)) {
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
  uireqextProcess (plui->uireqext);

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
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

  uiLabelSetText (&plui->clock,
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
  if (! connIsConnected (plui->conn, ROUTE_MARQUEE)) {
    connConnect (plui->conn, ROUTE_MARQUEE);
  }

  connProcessUnconnected (plui->conn);

  if (connIsConnected (plui->conn, ROUTE_STARTERUI)) {
    connSendMessage (plui->conn, ROUTE_STARTERUI, MSG_START_MAIN, "0");
    rc = STATE_FINISHED;
  }

  logProcEnd (LOG_PROC, "pluiConnectingCallback", "");
  return rc;
}

static bool
pluiHandshakeCallback (void *udata, programstate_t programState)
{
  playerui_t   *plui = udata;
  bool          rc = STATE_NOT_FINISH;

  logProcBegin (LOG_PROC, "pluiHandshakeCallback");

  if (! connIsConnected (plui->conn, ROUTE_MAIN)) {
    connConnect (plui->conn, ROUTE_MAIN);
  }
  if (! connIsConnected (plui->conn, ROUTE_PLAYER)) {
    connConnect (plui->conn, ROUTE_PLAYER);
  }
  if (! connIsConnected (plui->conn, ROUTE_MARQUEE)) {
    connConnect (plui->conn, ROUTE_MARQUEE);
  }

  connProcessUnconnected (plui->conn);

  if (connHaveHandshake (plui->conn, ROUTE_MAIN) &&
      ! connIsConnected (plui->conn, ROUTE_MARQUEE)) {
    connSendMessage (plui->conn, ROUTE_MAIN, MSG_START_MARQUEE, NULL);
  }

  if (connHaveHandshake (plui->conn, ROUTE_STARTERUI) &&
      connHaveHandshake (plui->conn, ROUTE_MAIN) &&
      connHaveHandshake (plui->conn, ROUTE_PLAYER) &&
      connHaveHandshake (plui->conn, ROUTE_MARQUEE)) {
    if (plui->mainalready) {
      connSendMessage (plui->conn, ROUTE_MAIN, MSG_MAIN_REQ_STATUS, NULL);
    }
    if (! plui->mainalready) {
      pluiSetPlaybackQueue (plui, plui->musicqPlayIdx, PLUI_UPDATE_MAIN);
      pluiSetManageQueue (plui, plui->musicqManageIdx);
      pluiSetSwitchQueue (plui);
    }
    pluiSetExtraQueues (plui);
    progstateLogTime (plui->progstate, "time-to-start-gui");
    rc = STATE_FINISHED;
  }

  logProcEnd (LOG_PROC, "pluiHandshakeCallback", "");
  return rc;
}

static int
pluiProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  playerui_t  *plui = udata;
  bool        dbgdisp = false;
  char        *targs = NULL;

  logProcBegin (LOG_PROC, "pluiProcessMsg");

  if (args != NULL) {
    targs = mdstrdup (args);
  }

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_PLAYERUI: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (plui->conn, routefrom);
          dbgdisp = true;
          break;
        }
        case MSG_SOCKET_CLOSE: {
          connDisconnect (plui->conn, routefrom);
          dbgdisp = true;
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          progstateShutdownProcess (plui->progstate);
          dbgdisp = true;
          break;
        }
        case MSG_WINDOW_FIND: {
          uiWindowFind (&plui->window);
          break;
        }
        case MSG_QUEUE_SWITCH: {
          pluiSetPlaybackQueue (plui, atoi (targs), PLUI_UPDATE_MAIN);
          dbgdisp = true;
          break;
        }
        case MSG_MARQUEE_IS_MAX: {
          pluisetMarqueeIsMaximized (plui, targs);
          dbgdisp = true;
          break;
        }
        case MSG_MARQUEE_FONT_SIZES: {
          pluisetMarqueeFontSizes (plui, targs);
          dbgdisp = true;
          break;
        }
        case MSG_DB_ENTRY_UPDATE: {
          dbLoadEntry (plui->musicdb, atol (targs));
          uisongselPopulateData (plui->uisongsel);
          dbgdisp = true;
          break;
        }
        case MSG_SONG_SELECT: {
          mp_songselect_t   *songselect;

          songselect = msgparseSongSelect (args);
          /* the display is offset by 1, as the 0 index is the current song */
          --songselect->loc;
          uimusicqProcessSongSelect (plui->uimusicq, songselect);
          msgparseSongSelectFree (songselect);
          dbgdisp = true;
          break;
        }
        case MSG_MUSIC_QUEUE_DATA: {
          mp_musicqupdate_t   *musicqupdate;
          int                 updflag;

          musicqupdate = msgparseMusicQueueData (args);
          if (musicqupdate == NULL) {
            break;
          }

          if ((int) musicqupdate->mqidx >= MUSICQ_DISP_MAX ||
              ! bdjoptGetNumPerQueue (OPT_Q_DISPLAY, musicqupdate->mqidx)) {
            logMsg (LOG_DBG, LOG_MAIN, "music queue data: mq idx %d not valid", musicqupdate->mqidx);
            break;
          }

          updflag = UISONGSEL_MARK_UPDATE;
          if (musicqupdate->mqidx == MUSICQ_PB_A) {
            updflag = UISONGSEL_MARK_REPLACE;
          }

          if ((int) musicqupdate->mqidx < MUSICQ_DISP_MAX) {
            /* if displayed */
            if (bdjoptGetNumPerQueue (OPT_Q_DISPLAY, musicqupdate->mqidx)) {
              uimusicqProcessMusicQueueData (plui->uimusicq, musicqupdate);
              /* the music queue data is used to display the mark */
              /* indicating that the song is already in the song list */
              uisongselProcessMusicQueueData (plui->uisongsel, musicqupdate,
                  updflag);
            }
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
          dbgdisp = true;
          break;
        }
        case MSG_SONG_FINISH: {
          uiLabelSetText (&plui->statusMsg, "");
          pluiPushHistory (plui, targs);
          dbgdisp = true;
          break;
        }
        case MSG_MAIN_ALREADY: {
          plui->mainalready = true;
          /* CONTEXT: player-ui: status message */
          uiLabelSetText (&plui->statusMsg, _("Recovered from crash."));
          dbgdisp = true;
          break;
        }
        case MSG_MAIN_CURR_MANAGE: {
          int     mqidx;

          mqidx = atoi (args);
          plui->musicqLastManageIdx = mqidx;
          plui->musicqManageIdx = mqidx;
          uimusicqSetManageIdx (plui->uimusicq, mqidx);
          dbgdisp = true;
          break;
        }
        case MSG_MAIN_CURR_PLAY: {
          int     mqidx;

          mqidx = atoi (args);
          pluiSetPlaybackQueue (plui, mqidx, PLUI_NO_UPDATE);
          dbgdisp = true;
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

  if (dbgdisp) {
    logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
        routefrom, msgRouteDebugText (routefrom),
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
  }
  dataFree (targs);

  /* due to the db update message, these must be applied afterwards */
  uiplayerProcessMsg (routefrom, route, msg, args, plui->uiplayer);

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

  tabid = uiutilsNotebookIDGet (plui->nbtabid, pagenum);
  plui->currpage = pagenum;
  /* do not call set-manage-queue on the request tab */
  if (tabid == UI_TAB_MUSICQ) {
    pluiSetManageQueue (plui, pagenum);
  }
  if (tabid == UI_TAB_HISTORY) {
    pluiSetManageQueue (plui, MUSICQ_HISTORY);
  }

  pluiPlaybackButtonHideShow (plui, pagenum);
  logProcEnd (LOG_PROC, "pluiSwitchPage", "");
  return UICB_CONT;
}

static void
pluiPlaybackButtonHideShow (playerui_t *plui, long pagenum)
{
  int         tabid;
  UIWidget    *uiwidgetp;

  tabid = uiutilsNotebookIDGet (plui->nbtabid, pagenum);

  uiwidgetp = uiButtonGetUIWidget (plui->setPlaybackButton);
  uiWidgetHide (uiwidgetp);
  if (tabid == UI_TAB_MUSICQ) {
    if (nlistGetNum (plui->options, PLUI_SHOW_EXTRA_QUEUES)) {
      uiWidgetShow (uiwidgetp);
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
pluiSetPlaybackQueue (playerui_t *plui, musicqidx_t newQueue, int updateFlag)
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
      uiImageSetFromPixbuf (&plui->musicqImage [i], &plui->ledonPixbuf);
    } else {
      uiImageSetFromPixbuf (&plui->musicqImage [i], &plui->ledoffPixbuf);
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
pluiSetManageQueue (playerui_t *plui, musicqidx_t mqidx)
{
  char  tbuff [40];

  /* need to save the last managed to handle history queue button */
  plui->musicqLastManageIdx = plui->musicqManageIdx;
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
  uiutilsNotebookIDStartIterator (plui->nbtabid, &pagenum);
  while ((tabid = uiutilsNotebookIDIterate (plui->nbtabid, &pagenum)) >= 0) {
    if (tabid == UI_TAB_MUSICQ && pagenum > 0) {
      if (! show && plui->currpage == pagenum) {
        resetcurr = true;
      }
      uiNotebookHideShowPage (&plui->notebook, pagenum, show);
    }
  }
  if (resetcurr) {
    /* the tab currently displayed is being hidden */
    plui->currpage = 0;
  }

  pluiPlaybackButtonHideShow (plui, plui->currpage);
  logProcEnd (LOG_PROC, "pluiSetExtraQueues", "");
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
  snprintf (tbuff, sizeof (tbuff), "%"PRId64,
      nlistGetNum (plui->options, PLUI_SWITCH_QUEUE_WHEN_EMPTY));
  connSendMessage (plui->conn, ROUTE_MAIN, MSG_QUEUE_SWITCH_EMPTY, tbuff);
  logProcEnd (LOG_PROC, "pluiSetSwitchQueue", "");
}

static bool
pluiMarqueeFontSizeDialog (void *udata)
{
  playerui_t      *plui = udata;
  int             sz;

  logProcBegin (LOG_PROC, "pluiMarqueeFontSizeDialog");

  if (! plui->fontszdialogcreated) {
    pluiCreateMarqueeFontSizeDialog (plui);
  }

  if (plui->marqueeIsMaximized) {
    sz = plui->marqueeFontSizeFS;
  } else {
    sz = plui->marqueeFontSize;
  }

  uiSpinboxSetValue (&plui->marqueeSpinBox, (double) sz);
  uiWidgetShowAll (&plui->marqueeFontSizeDialog);

  logProcEnd (LOG_PROC, "pluiMarqueeFontSizeDialog", "");
  return UICB_CONT;
}

static void
pluiCreateMarqueeFontSizeDialog (playerui_t *plui)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;

  logProcBegin (LOG_PROC, "pluiCreateMarqueeFontSizeDialog");

  uiutilsUICallbackLongInit (&plui->callbacks [PLUI_CB_FONT_SIZE],
      pluiMarqueeFontSizeDialogResponse, plui);
  uiCreateDialog (&plui->marqueeFontSizeDialog, &plui->window,
      &plui->callbacks [PLUI_CB_FONT_SIZE],
      /* CONTEXT: playerui: marquee font size dialog: window title */
      _("Marquee Font Size"),
      /* CONTEXT: playerui: marquee font size dialog: action button */
      _("Close"),
      RESPONSE_CLOSE,
      NULL
      );

  uiCreateVertBox (&vbox);
  uiDialogPackInDialog (&plui->marqueeFontSizeDialog, &vbox);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: playerui: marquee font size dialog: the font size selector */
  uiCreateColonLabel (&uiwidget, _("Font Size"));
  uiBoxPackStart (&hbox, &uiwidget);

  uiSpinboxIntCreate (&plui->marqueeSpinBox);
  uiSpinboxSet (&plui->marqueeSpinBox, 10.0, 300.0);
  uiSpinboxSetValue (&plui->marqueeSpinBox, 36.0);
  uiBoxPackStart (&hbox, &plui->marqueeSpinBox);
  g_signal_connect (plui->marqueeSpinBox.widget, "value-changed",
      G_CALLBACK (pluiMarqueeFontSizeChg), plui);

  /* the dialog doesn't have any space above the buttons */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateLabel (&uiwidget, "");
  uiBoxPackStart (&hbox, &uiwidget);
  plui->fontszdialogcreated = true;

  logProcEnd (LOG_PROC, "pluiCreateMarqueeFontSizeDialog", "");
}

static bool
pluiMarqueeFontSizeDialogResponse (void *udata, long responseid)
{
  playerui_t  *plui = udata;

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      uiutilsUIWidgetInit (&plui->marqueeFontSizeDialog);
      plui->fontszdialogcreated = false;
      break;
    }
    case RESPONSE_CLOSE: {
      uiWidgetHide (&plui->marqueeFontSizeDialog);
      break;
    }
  }
  return UICB_CONT;
}

static void
pluiMarqueeFontSizeChg (GtkSpinButton *sb, gpointer udata)
{
  playerui_t  *plui = udata;
  int         fontsz;
  double      value;

  value = uiSpinboxGetValue (&plui->marqueeSpinBox);
  fontsz = (int) round (value);
  if (plui->marqueeIsMaximized) {
    plui->marqueeFontSizeFS = fontsz;
  } else {
    plui->marqueeFontSize = fontsz;
  }
  mstimeset (&plui->marqueeFontSizeCheck, 100);
}

static bool
pluiMarqueeFind (void *udata)
{
  playerui_t  *plui = udata;

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
pluiQueueProcess (void *udata, long dbidx, int mqidx)
{
  playerui_t  *plui = udata;
  long        loc;
  char        tbuff [100];

  if (mqidx == MUSICQ_CURRENT) {
    mqidx = plui->musicqManageIdx;
  }
  if (mqidx == MUSICQ_LAST) {
    mqidx = plui->musicqLastManageIdx;
  }

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

  songWriteDB (plui->musicdb, dbidx);

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
  dbidx_t   dbidx;
  char      tbuff [100];

  dbidx = atol (args);

  snprintf (tbuff, sizeof (tbuff), "%d%c%d%c%d", MUSICQ_HISTORY,
      MSG_ARGS_RS, 999, MSG_ARGS_RS, dbidx);
  connSendMessage (plui->conn, ROUTE_MAIN, MSG_MUSICQ_INSERT, tbuff);
}

static bool
pluiRequestExternalDialog (void *udata)
{
  playerui_t    *plui = udata;
  bool          rc;

  rc = uireqextDialog (plui->uireqext);
  return rc;
}

static bool
pluiReqextCallback (void *udata)
{
  playerui_t    *plui = udata;
  song_t        *song;

  song = uireqextGetSong (plui->uireqext);
  if (song != NULL) {
    dbidx_t     dbidx;
    char        *songentrytext;

    /* add to the player's copy of the database */
    dbidx = dbAddTemporarySong (plui->musicdb, song);

    songentrytext = uireqextGetSongEntryText (plui->uireqext);
    if (songentrytext != NULL) {
      char        *tbuff;

      tbuff = mdmalloc (BDJMSG_MAX);
      snprintf (tbuff, BDJMSG_MAX, "%s%c%d%c%s",
          songGetStr (song, TAG_FILE), MSG_ARGS_RS,
          dbidx, MSG_ARGS_RS,
          songentrytext);
      connSendMessage (plui->conn, ROUTE_MAIN, MSG_DB_ENTRY_TEMP_ADD, tbuff);
      dataFree (tbuff);
      pluiQueueProcess (plui, dbidx, MUSICQ_CURRENT);
    }
  }
  return UICB_CONT;
}
