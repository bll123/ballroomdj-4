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
#include "musicq.h"
#include "osuiutils.h"
#include "osutils.h"
#include "pathbld.h"
#include "progstate.h"
#include "slist.h"
#include "sock.h"
#include "sockh.h"
#include "sysvars.h"
#include "tmutil.h"
#include "ui.h"
#include "uimusicq.h"
#include "uiplayer.h"
#include "uisongsel.h"
#include "uiutils.h"

enum {
  PLUI_MENU_CB_PLAY_QUEUE,
  PLUI_MENU_CB_EXTRA_QUEUE,
  PLUI_MENU_CB_SWITCH_QUEUE,
  PLUI_MENU_CB_MQ_FONT_SZ,
  PLUI_MENU_CB_MAX,
};

typedef struct {
  progstate_t     *progstate;
  char            *locknm;
  conn_t          *conn;
  musicdb_t       *musicdb;
  musicqidx_t     musicqPlayIdx;
  musicqidx_t     musicqManageIdx;
  dispsel_t       *dispsel;
  int             dbgflags;
  int             marqueeIsMaximized;
  int             marqueeFontSize;
  int             marqueeFontSizeFS;
  mstime_t        marqueeFontSizeCheck;
  int             stopwaitcount;
  mstime_t        clockCheck;
  songfilter_t    *songfilter;
  /* notebook */
  UIWidget        notebook;
  UICallback      nbcb;
  uiutilsnbtabid_t *nbtabid;
  int             currpage;
  UIWidget        window;
  UICallback      closecb;
  UIWidget        vbox;
  UICallback      menucb [PLUI_MENU_CB_MAX];
  UIWidget        clock;
  UIWidget        musicqImage [MUSICQ_MAX];
  UIWidget        setPlaybackButton;
  UICallback      setpbqcb;
  UIWidget        ledoffPixbuf;
  UIWidget        ledonPixbuf;
  GtkWidget       *marqueeFontSizeDialog;
  UIWidget        marqueeSpinBox;
  /* ui major elements */
  uiplayer_t      *uiplayer;
  uimusicq_t      *uimusicq;
  uisongsel_t     *uisongsel;
  /* options */
  datafile_t      *optiondf;
  nlist_t         *options;
  /* flags */
  bool            uibuilt : 1;
} playerui_t;

static datafilekey_t playeruidfkeys [] = {
  { "FILTER_POS_X",             SONGSEL_FILTER_POSITION_X,    VALUE_NUM, NULL, -1 },
  { "FILTER_POS_Y",             SONGSEL_FILTER_POSITION_Y,    VALUE_NUM, NULL, -1 },
  { "PLAY_WHEN_QUEUED",         PLUI_PLAY_WHEN_QUEUED,        VALUE_NUM, NULL, -1 },
  { "PLUI_POS_X",               PLUI_POSITION_X,              VALUE_NUM, NULL, -1 },
  { "PLUI_POS_Y",               PLUI_POSITION_Y,              VALUE_NUM, NULL, -1 },
  { "PLUI_SIZE_X",              PLUI_SIZE_X,                  VALUE_NUM, NULL, -1 },
  { "PLUI_SIZE_Y",              PLUI_SIZE_Y,                  VALUE_NUM, NULL, -1 },
  { "SHOW_EXTRA_QUEUES",        PLUI_SHOW_EXTRA_QUEUES,       VALUE_NUM, NULL, -1 },
  { "SORT_BY",                  SONGSEL_SORT_BY,              VALUE_STR, NULL, -1 },
  { "SWITCH_QUEUE_WHEN_EMPTY",  PLUI_SWITCH_QUEUE_WHEN_EMPTY, VALUE_NUM, NULL, -1 },
};
#define PLAYERUI_DFKEY_COUNT (sizeof (playeruidfkeys) / sizeof (datafilekey_t))

static bool     pluiConnectingCallback (void *udata, programstate_t programState);
static bool     pluiHandshakeCallback (void *udata, programstate_t programState);
static bool     pluiStoppingCallback (void *udata, programstate_t programState);
static bool     pluiStopWaitCallback (void *udata, programstate_t programState);
static bool     pluiClosingCallback (void *udata, programstate_t programState);
static void     pluiBuildUI (playerui_t *plui);
static int      pluiMainLoop  (void *tplui);
static void     pluiClock (playerui_t *plui);
static int      pluiProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static bool     pluiCloseWin (void *udata);
static void     pluiSigHandler (int sig);
/* queue selection handlers */
static bool     pluiSwitchPage (void *udata, int pagenum);
static bool     pluiProcessSetPlaybackQueue (void *udata);
static void     pluiSetPlaybackQueue (playerui_t *plui, musicqidx_t newqueue);
/* option handlers */
static bool     pluiTogglePlayWhenQueued (void *udata);
static void     pluiSetPlayWhenQueued (playerui_t *plui);
static bool     pluiToggleExtraQueues (void *udata);
static void     pluiSetExtraQueues (playerui_t *plui);
static bool     pluiToggleSwitchQueue (void *udata);
static void     pluiSetSwitchQueue (playerui_t *plui);
static bool     pluiMarqueeFontSizeDialog (void *udata);
static void     pluiCreateMarqueeFontSizeDialog (playerui_t *plui);
static void     pluiMarqueeFontSizeDialogResponse (GtkDialog *d, gint responseid, gpointer udata);
static void     pluiMarqueeFontSizeChg (GtkSpinButton *fb, gpointer udata);
static void     pluisetMarqueeIsMaximized (playerui_t *plui, char *args);
static void     pluisetMarqueeFontSizes (playerui_t *plui, char *args);
static void     pluiInitializeSongFilter (playerui_t *plui, nlist_t *options);


static int gKillReceived = 0;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  playerui_t      plui;
  char            *uifont;
  char            tbuff [MAXPATHLEN];


  uiutilsUIWidgetInit (&plui.window);
  uiutilsUIWidgetInit (&plui.clock);
  uiutilsUIWidgetInit (&plui.notebook);
  plui.marqueeFontSizeDialog = NULL;
  plui.progstate = progstateInit ("playerui");
  progstateSetCallback (plui.progstate, STATE_CONNECTING,
      pluiConnectingCallback, &plui);
  progstateSetCallback (plui.progstate, STATE_WAIT_HANDSHAKE,
      pluiHandshakeCallback, &plui);
  plui.uiplayer = NULL;
  plui.uimusicq = NULL;
  plui.uisongsel = NULL;
  plui.musicqPlayIdx = MUSICQ_A;
  plui.musicqManageIdx = MUSICQ_A;
  plui.marqueeIsMaximized = false;
  plui.marqueeFontSize = 36;
  plui.marqueeFontSizeFS = 60;
  mstimeset (&plui.marqueeFontSizeCheck, 3600000);
  mstimeset (&plui.clockCheck, 0);
  plui.stopwaitcount = 0;
  plui.nbtabid = uiutilsNotebookIDInit ();
  plui.songfilter = NULL;
  plui.uibuilt = false;
  plui.currpage = 0;

  osSetStandardSignals (pluiSigHandler);

  plui.dbgflags = bdj4startup (argc, argv, &plui.musicdb,
      "pu", ROUTE_PLAYERUI, BDJ4_INIT_NONE);
  logProcBegin (LOG_PROC, "playerui");

  plui.dispsel = dispselAlloc ();

  listenPort = bdjvarsGetNum (BDJVL_PLAYERUI_PORT);
  plui.conn = connInit (ROUTE_PLAYERUI);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "playerui", BDJ4_CONFIG_EXT, PATHBLD_MP_USEIDX);
  plui.optiondf = datafileAllocParse ("playerui-opt", DFTYPE_KEY_VAL, tbuff,
      playeruidfkeys, PLAYERUI_DFKEY_COUNT, DATAFILE_NO_LOOKUP);
  plui.options = datafileGetList (plui.optiondf);
  if (plui.options == NULL) {
    plui.options = nlistAlloc ("playerui-opt", LIST_ORDERED, free);

    nlistSetNum (plui.options, PLUI_PLAY_WHEN_QUEUED, true);
    nlistSetNum (plui.options, PLUI_SHOW_EXTRA_QUEUES, false);
    nlistSetNum (plui.options, PLUI_SWITCH_QUEUE_WHEN_EMPTY, false);
    nlistSetNum (plui.options, SONGSEL_FILTER_POSITION_X, -1);
    nlistSetNum (plui.options, SONGSEL_FILTER_POSITION_Y, -1);
    nlistSetNum (plui.options, PLUI_POSITION_X, -1);
    nlistSetNum (plui.options, PLUI_POSITION_Y, -1);
    nlistSetNum (plui.options, PLUI_SIZE_X, 1000);
    nlistSetNum (plui.options, PLUI_SIZE_Y, 600);
    nlistSetStr (plui.options, SONGSEL_SORT_BY, "TITLE");
  }

  plui.songfilter = songfilterAlloc ();
  pluiInitializeSongFilter (&plui, plui.options);

  plui.uiplayer = uiplayerInit (plui.progstate, plui.conn, plui.musicdb);
  plui.uimusicq = uimusicqInit (plui.conn, plui.musicdb,
      plui.dispsel, UIMUSICQ_FLAGS_NONE, DISP_SEL_MUSICQ);
  plui.uisongsel = uisongselInit ("plui-req", plui.conn, plui.musicdb,
      plui.dispsel, plui.options,
      SONG_FILTER_FOR_PLAYBACK, DISP_SEL_REQUEST);
  uisongselInitializeSongFilter (plui.uisongsel, plui.songfilter);

  /* register these after calling the sub-window initialization */
  /* then these will be run last, after the other closing callbacks */
  progstateSetCallback (plui.progstate, STATE_STOPPING,
      pluiStoppingCallback, &plui);
  progstateSetCallback (plui.progstate, STATE_STOP_WAIT,
      pluiStopWaitCallback, &plui);
  progstateSetCallback (plui.progstate, STATE_CLOSING,
      pluiClosingCallback, &plui);

  uiUIInitialize ();
  uifont = bdjoptGetStr (OPT_MP_UIFONT);
  uiSetUIFont (uifont);

  pluiBuildUI (&plui);
  sockhMainLoop (listenPort, pluiProcessMsg, pluiMainLoop, &plui);
  connFree (plui.conn);
  progstateFree (plui.progstate);
  logProcEnd (LOG_PROC, "playerui", "");
  logEnd ();
  return status;
}

/* internal routines */

static bool
pluiStoppingCallback (void *udata, programstate_t programState)
{
  playerui_t    * plui = udata;
  int           x, y;

  logProcBegin (LOG_PROC, "pluiStoppingCallback");
  connSendMessage (plui->conn, ROUTE_STARTERUI, MSG_STOP_MAIN, NULL);

  uiWindowGetSize (&plui->window, &x, &y);
  nlistSetNum (plui->options, PLUI_SIZE_X, x);
  nlistSetNum (plui->options, PLUI_SIZE_Y, y);
  uiWindowGetPosition (&plui->window, &x, &y);
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

  pathbldMakePath (fn, sizeof (fn),
      "playerui", BDJ4_CONFIG_EXT, PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("playerui", fn, playeruidfkeys, PLAYERUI_DFKEY_COUNT, plui->options);

  uiWidgetClearPersistent (&plui->ledonPixbuf);
  uiWidgetClearPersistent (&plui->ledoffPixbuf);

  bdj4shutdown (ROUTE_PLAYERUI, plui->musicdb);
  dispselFree (plui->dispsel);

  if (plui->nbtabid != NULL) {
    uiutilsNotebookIDFree (plui->nbtabid);
  }
  if (plui->options != datafileGetList (plui->optiondf)) {
    nlistFree (plui->options);
  }
  if (plui->songfilter != NULL) {
    songfilterFree (plui->songfilter);
  }
  datafileFree (plui->optiondf);

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
  char        *str;
  char        imgbuff [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];
  int         x, y;

  logProcBegin (LOG_PROC, "pluiBuildUI");

  pathbldMakePath (tbuff, sizeof (tbuff),  "led_off", ".svg",
      PATHBLD_MP_IMGDIR);
  uiImageFromFile (&plui->ledoffPixbuf, tbuff);
  uiImageGetPixbuf (&plui->ledoffPixbuf);
  uiWidgetMakePersistent (&plui->ledoffPixbuf);

  pathbldMakePath (tbuff, sizeof (tbuff),  "led_on", ".svg",
      PATHBLD_MP_IMGDIR);
  uiImageFromFile (&plui->ledonPixbuf, tbuff);
  uiImageGetPixbuf (&plui->ledonPixbuf);
  uiWidgetMakePersistent (&plui->ledonPixbuf);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", ".svg", PATHBLD_MP_IMGDIR);
  uiutilsUICallbackInit (&plui->closecb, pluiCloseWin, plui);
  uiCreateMainWindow (&plui->window, &plui->closecb,
      bdjoptGetStr (OPT_P_PROFILENAME), imgbuff);

  uiCreateVertBox (&plui->vbox);
  uiBoxPackInWindow (&plui->window, &plui->vbox);
  uiWidgetSetAllMargins (&plui->vbox, uiBaseMarginSz * 2);

  /* menu */
  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&plui->vbox, &hbox);

  uiCreateMenubar (&menubar);
  uiBoxPackStart (&hbox, &menubar);

  uiCreateLabel (&plui->clock, "");
  uiBoxPackEnd (&hbox, &plui->clock);
  uiLabelDarkenColor (&plui->clock, bdjoptGetStr (OPT_P_UI_ACCENT_COL));

  /* CONTEXT: menu selection: options for the player */
  uiMenuCreateItem (&menubar, &menuitem, _("Options"), NULL);

  uiCreateSubMenu (&menuitem, &menu);

  /* CONTEXT: menu checkbox: start playback when a dance or playlist is queued */
  uiutilsUICallbackInit (&plui->menucb [PLUI_MENU_CB_PLAY_QUEUE],
      pluiTogglePlayWhenQueued, plui);
  uiMenuCreateCheckbox (&menu, &menuitem, _("Play When Queued"),
      nlistGetNum (plui->options, PLUI_PLAY_WHEN_QUEUED),
      &plui->menucb [PLUI_MENU_CB_PLAY_QUEUE]);

  /* CONTEXT: menu checkbox: show the extra queues (in addition to the main music queue) */
  uiutilsUICallbackInit (&plui->menucb [PLUI_MENU_CB_EXTRA_QUEUE],
      pluiToggleExtraQueues, plui);
  uiMenuCreateCheckbox (&menu, &menuitem, _("Show Extra Queues"),
      nlistGetNum (plui->options, PLUI_SHOW_EXTRA_QUEUES),
      &plui->menucb [PLUI_MENU_CB_EXTRA_QUEUE]);

  /* CONTEXT: menu checkbox: when a queue is emptied, switch playback to the next queue */
  uiutilsUICallbackInit (&plui->menucb [PLUI_MENU_CB_SWITCH_QUEUE],
      pluiToggleSwitchQueue, plui);
  uiMenuCreateCheckbox (&menu, &menuitem, _("Switch Queue When Empty"),
      nlistGetNum (plui->options, PLUI_SWITCH_QUEUE_WHEN_EMPTY),
      &plui->menucb [PLUI_MENU_CB_SWITCH_QUEUE]);

  /* CONTEXT: menu selection: marquee related options */
  uiMenuCreateItem (&menubar, &menuitem, _("Marquee"), NULL);

  uiCreateSubMenu (&menuitem, &menu);

  /* CONTEXT: menu selection: marquee: change the marquee font size */
  uiutilsUICallbackInit (&plui->menucb [PLUI_MENU_CB_MQ_FONT_SZ],
      pluiMarqueeFontSizeDialog, plui);
  uiMenuCreateItem (&menu, &menuitem, _("Font Size"),
      &plui->menucb [PLUI_MENU_CB_MQ_FONT_SZ]);

  /* player */
  uiwidgetp = uiplayerBuildUI (plui->uiplayer);
  uiBoxPackStart (&plui->vbox, uiwidgetp);

  uiCreateNotebook (&plui->notebook);
  uiBoxPackStartExpand (&plui->vbox, &plui->notebook);

  uiutilsUICallbackIntInit (&plui->nbcb, pluiSwitchPage, plui);
  uiNotebookSetCallback (&plui->notebook, &plui->nbcb);

  uiutilsUICallbackInit (&plui->setpbqcb,
      pluiProcessSetPlaybackQueue, plui);
  uiCreateButton (&uiwidget, &plui->setpbqcb,
      /* CONTEXT: select the current queue for playback */
      _("Set Queue for Playback"), NULL, NULL, NULL);
  uiNotebookSetActionWidget (&plui->notebook, &uiwidget);
  uiWidgetShowAll (&uiwidget);
  uiutilsUIWidgetCopy (&plui->setPlaybackButton, &uiwidget);

  for (musicqidx_t i = 0; i < MUSICQ_MAX; ++i) {
    /* music queue tab */

    uiwidgetp = uimusicqBuildUI (plui->uimusicq, plui->window.widget, i);
    uiCreateHorizBox (&hbox);
    str = bdjoptGetStr (OPT_P_QUEUE_NAME_A + i);
    uiCreateLabel (&uiwidget, str);
    uiBoxPackStart (&hbox, &uiwidget);
    uiImageNew (&plui->musicqImage [i]);
    uiImageSetFromPixbuf (&plui->musicqImage [i], &plui->ledonPixbuf);
    uiWidgetSetMarginStart (&plui->musicqImage [i], uiBaseMarginSz);
    uiBoxPackStart (&hbox, &plui->musicqImage [i]);

    uiNotebookAppendPage (&plui->notebook, uiwidgetp, &hbox);
    uiutilsNotebookIDAdd (plui->nbtabid, UI_TAB_MUSICQ);
    uiWidgetShowAll (&hbox);
  }

  /* request tab */
  uiwidgetp = uisongselBuildUI (plui->uisongsel, plui->window.widget);
  /* CONTEXT: name of request tab : lists the songs in the database */
  uiCreateLabel (&uiwidget, _("Request"));
  uiNotebookAppendPage (&plui->notebook, uiwidgetp, &uiwidget);
  uiutilsNotebookIDAdd (plui->nbtabid, UI_TAB_SONGSEL);

  x = nlistGetNum (plui->options, PLUI_SIZE_X);
  y = nlistGetNum (plui->options, PLUI_SIZE_Y);
  uiWindowSetDefaultSize (&plui->window, x, y);

  uiWidgetShowAll (&plui->window);

  x = nlistGetNum (plui->options, PLUI_POSITION_X);
  y = nlistGetNum (plui->options, PLUI_POSITION_Y);
  uiWindowMove (&plui->window, x, y);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", ".png", PATHBLD_MP_IMGDIR);
  osuiSetIcon (imgbuff);

  plui->uibuilt = true;
  logProcEnd (LOG_PROC, "pluiBuildUI", "");
}

static int
pluiMainLoop (void *tplui)
{
  playerui_t  *plui = tplui;
  int         stop = FALSE;

  if (! stop) {
    uiUIProcessEvents ();
  }

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

  uiLabelSetText (&plui->clock, tmutilDisp (tbuff, sizeof (tbuff)));
  mstimeset (&plui->clockCheck, 1000);
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
    pluiSetPlayWhenQueued (plui);
    pluiSetSwitchQueue (plui);
    pluiSetPlaybackQueue (plui, MUSICQ_A);
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
  playerui_t       *plui = udata;

  logProcBegin (LOG_PROC, "pluiProcessMsg");

  logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
      routefrom, msgRouteDebugText (routefrom),
      route, msgRouteDebugText (route), msg, msgDebugText (msg), args);

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
          progstateShutdownProcess (plui->progstate);
          break;
        }
        case MSG_QUEUE_SWITCH: {
          pluiSetPlaybackQueue (plui, atoi (args));
          break;
        }
        case MSG_MARQUEE_IS_MAX: {
          pluisetMarqueeIsMaximized (plui, args);
          break;
        }
        case MSG_MARQUEE_FONT_SIZES: {
          pluisetMarqueeFontSizes (plui, args);
          break;
        }
        case MSG_DATABASE_UPDATE: {
          plui->musicdb = bdj4ReloadDatabase (plui->musicdb);
          uiplayerSetDatabase (plui->uiplayer, plui->musicdb);
          uisongselSetDatabase (plui->uisongsel, plui->musicdb);
          uimusicqSetDatabase (plui->uimusicq, plui->musicdb);
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
  uiplayerProcessMsg (routefrom, route, msg, args, plui->uiplayer);
  uisongselProcessMsg (routefrom, route, msg, args, plui->uisongsel);
  uimusicqProcessMsg (routefrom, route, msg, args, plui->uimusicq);

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
pluiSwitchPage (void *udata, int pagenum)
{
  playerui_t  *plui = udata;
  int         tabid;

  logProcBegin (LOG_PROC, "pluiSwitchPage");

  tabid = uiutilsNotebookIDGet (plui->nbtabid, pagenum);

  plui->currpage = pagenum;
  uiWidgetHide (&plui->setPlaybackButton);
  if (tabid == UI_TAB_MUSICQ) {
    plui->musicqManageIdx = pagenum;
    uimusicqSetManageIdx (plui->uimusicq, pagenum);
    if (nlistGetNum (plui->options, PLUI_SHOW_EXTRA_QUEUES)) {
      uiWidgetShow (&plui->setPlaybackButton);
    }
  }
  logProcEnd (LOG_PROC, "pluiSwitchPage", "");
  return UICB_CONT;
}

static bool
pluiProcessSetPlaybackQueue (void *udata)
{
  playerui_t      *plui = udata;

  logProcBegin (LOG_PROC, "pluiProcessSetPlaybackQueue");
  pluiSetPlaybackQueue (plui, plui->musicqManageIdx);
  logProcEnd (LOG_PROC, "pluiProcessSetPlaybackQueue", "");
  return UICB_CONT;
}

static void
pluiSetPlaybackQueue (playerui_t *plui, musicqidx_t newQueue)
{
  char            tbuff [40];

  logProcBegin (LOG_PROC, "pluiSetPlaybackQueue");

  plui->musicqPlayIdx = newQueue;
  for (musicqidx_t i = 0; i < MUSICQ_MAX; ++i) {
    if (plui->musicqPlayIdx == i) {
      uiImageSetFromPixbuf (&plui->musicqImage [i], &plui->ledonPixbuf);
    } else {
      uiImageSetFromPixbuf (&plui->musicqImage [i], &plui->ledoffPixbuf);
    }
  }

  /* if showextraqueues is off, reject any attempt to switch playback. */
  /* let main know what queue is being used */
  uimusicqSetPlayIdx (plui->uimusicq, plui->musicqPlayIdx);
  snprintf (tbuff, sizeof (tbuff), "%d", plui->musicqPlayIdx);
  connSendMessage (plui->conn, ROUTE_MAIN, MSG_MUSICQ_SET_PLAYBACK, tbuff);
  logProcEnd (LOG_PROC, "pluiSetPlaybackQueue", "");
}

static bool
pluiTogglePlayWhenQueued (void *udata)
{
  playerui_t      *plui = udata;
  ssize_t         val;

  logProcBegin (LOG_PROC, "pluiTogglePlayWhenQueued");
  val = nlistGetNum (plui->options, PLUI_PLAY_WHEN_QUEUED);
  val = ! val;
  nlistSetNum (plui->options, PLUI_PLAY_WHEN_QUEUED, val);
  pluiSetPlayWhenQueued (plui);
  logProcEnd (LOG_PROC, "pluiTogglePlayWhenQueued", "");
  return UICB_CONT;
}

static void
pluiSetPlayWhenQueued (playerui_t *plui)
{
  char  tbuff [40];

  logProcBegin (LOG_PROC, "pluiSetPlayWhenQueued");
  snprintf (tbuff, sizeof (tbuff), "%zd",
      nlistGetNum (plui->options, PLUI_PLAY_WHEN_QUEUED));
  connSendMessage (plui->conn, ROUTE_MAIN, MSG_QUEUE_PLAY_ON_ADD, tbuff);
  logProcEnd (LOG_PROC, "pluiSetPlayWhenQueued", "");
}


static bool
pluiToggleExtraQueues (void *udata)
{
  playerui_t      *plui = udata;
  ssize_t         val;

  logProcBegin (LOG_PROC, "pluiToggleExtraQueues");
  val = nlistGetNum (plui->options, PLUI_SHOW_EXTRA_QUEUES);
  val = ! val;
  nlistSetNum (plui->options, PLUI_SHOW_EXTRA_QUEUES, val);
  /* calls the switch page handler, the managed music queue will be set */
  pluiSetExtraQueues (plui);
  if (! val) {
    pluiSetPlaybackQueue (plui, MUSICQ_A);
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
  /* make sure the set playback button is in the proper state */
  /* this will trigger the switch-page event and call pluiSwitchPage() */
  uiNotebookSetPage (&plui->notebook, plui->currpage);
  logProcEnd (LOG_PROC, "pluiSetExtraQueues", "");
}

static bool
pluiToggleSwitchQueue (void *udata)
{
  playerui_t      *plui = udata;
  ssize_t         val;

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
  snprintf (tbuff, sizeof (tbuff), "%zd",
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

  if (plui->marqueeFontSizeDialog == NULL) {
    pluiCreateMarqueeFontSizeDialog (plui);
  }

  if (plui->marqueeIsMaximized) {
    sz = plui->marqueeFontSizeFS;
  } else {
    sz = plui->marqueeFontSize;
  }

  uiSpinboxSetValue (&plui->marqueeSpinBox, (double) sz);
  uiWidgetShowAllW (plui->marqueeFontSizeDialog);

  logProcEnd (LOG_PROC, "pluiMarqueeFontSizeDialog", "");
  return UICB_CONT;
}



static void
pluiCreateMarqueeFontSizeDialog (playerui_t *plui)
{
  GtkWidget     *content;
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;

  logProcBegin (LOG_PROC, "pluiCreateMarqueeFontSizeDialog");

  plui->marqueeFontSizeDialog = gtk_dialog_new_with_buttons (
      /* CONTEXT: marquee font size dialog: window title */
      _("Marquee Font Size"),
      GTK_WINDOW (plui->window.widget),
      GTK_DIALOG_DESTROY_WITH_PARENT,
      /* CONTEXT: marquee font size dialog: action button */
      _("Close"),
      GTK_RESPONSE_CLOSE,
      NULL
      );

  content = gtk_dialog_get_content_area (GTK_DIALOG (plui->marqueeFontSizeDialog));

  uiCreateVertBox (&vbox);
  uiBoxPackInWindowWU (content, &vbox);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: marquee font size dialog: the font size selector */
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

  g_signal_connect (plui->marqueeFontSizeDialog, "response",
      G_CALLBACK (pluiMarqueeFontSizeDialogResponse), plui);
  logProcEnd (LOG_PROC, "pluiCreateMarqueeFontSizeDialog", "");
}

static void
pluiMarqueeFontSizeDialogResponse (GtkDialog *d, gint responseid, gpointer udata)
{
  playerui_t  *plui = udata;

  switch (responseid) {
    case GTK_RESPONSE_DELETE_EVENT: {
      plui->marqueeFontSizeDialog = NULL;
      break;
    }
    case GTK_RESPONSE_CLOSE: {
      uiWidgetHideW (plui->marqueeFontSizeDialog);
      break;
    }
  }
}

static void
pluiMarqueeFontSizeChg (GtkSpinButton *sb, gpointer udata)
{
  playerui_t  *plui = udata;
  int         fontsz;
  GtkAdjustment *adjustment;
  double      value;

  adjustment = gtk_spin_button_get_adjustment (sb);
  value = gtk_adjustment_get_value (adjustment);
  fontsz = (int) round (value);
  if (plui->marqueeIsMaximized) {
    plui->marqueeFontSizeFS = fontsz;
  } else {
    plui->marqueeFontSize = fontsz;
  }
  mstimeset (&plui->marqueeFontSizeCheck, 100);
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

static void
pluiInitializeSongFilter (playerui_t *plui, nlist_t *options)
{
  songfilterSetSort (plui->songfilter,
      nlistGetStr (options, SONGSEL_SORT_BY));
}
