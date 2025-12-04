/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
#include <stdarg.h>

#include "asconf.h"
#include "bdj4.h"
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjvars.h"
#include "configui.h"
#include "conn.h"
#include "dance.h"
#include "datafile.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "ossignal.h"
#include "osuiutils.h"
#include "pathbld.h"
#include "pathutil.h"
#include "pli.h"
#include "progstate.h"
#include "quickedit.h"
#include "sockh.h"
#include "songfilter.h"
#include "sysvars.h"
#include "ui.h"
#include "callback.h"
#include "uinbutil.h"
#include "uiutils.h"

typedef struct configui {
  progstate_t       *progstate;
  char              *locknm;
  conn_t            *conn;
  uint32_t          dbgflags;
  int               stopwaitcount;
  songfilter_t      *sf;
  quickedit_t       *qe;
  confuigui_t       gui;
  /* options */
  datafile_t        *optiondf;
  nlist_t           *options;
  bool              optionsalloc;
} configui_t;

static datafilekey_t configuidfkeys [CONFUI_KEY_MAX] = {
  { "CONFUI_POS_X",     CONFUI_POSITION_X,    VALUE_NUM, NULL, DF_NORM },
  { "CONFUI_POS_Y",     CONFUI_POSITION_Y,    VALUE_NUM, NULL, DF_NORM },
  { "CONFUI_SIZE_X",    CONFUI_SIZE_X,        VALUE_NUM, NULL, DF_NORM },
  { "CONFUI_SIZE_Y",    CONFUI_SIZE_Y,        VALUE_NUM, NULL, DF_NORM },
};

static bool confuiHandshakeCallback (void *udata, programstate_t programState);
static bool confuiStoppingCallback (void *udata, programstate_t programState);
static bool confuiStopWaitCallback (void *udata, programstate_t programState);
static bool confuiClosingCallback (void *udata, programstate_t programState);
static void confuiBuildUI (configui_t *confui);
static int  confuiMainLoop  (void *tconfui);
static int  confuiProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route, bdjmsgmsg_t msg, char *args, void *udata);
static bool confuiCloseWin (void *udata);
static void confuiSigHandler (int sig);


/* misc */
static void confuiLoadTagList (configui_t *confui);

static bool gKillReceived = false;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  nlist_t         *llist = NULL;
  nlistidx_t      iteridx;
  configui_t      confui;
  char            tbuff [MAXPATHLEN];
  uisetup_t       uisetup;

#if BDJ4_MEM_DEBUG
  mdebugInit ("cfui");
#endif

  confui.progstate = progstateInit ("configui");
  progstateSetCallback (confui.progstate, PROGSTATE_WAIT_HANDSHAKE,
      confuiHandshakeCallback, &confui);
  confui.locknm = NULL;
  confui.conn = NULL;
  confui.dbgflags = 0;
  confui.stopwaitcount = 0;
  confui.sf = NULL;
  confui.qe = NULL;

  confui.gui.asconf = NULL;
  confui.gui.localip = NULL;
  confui.gui.window = NULL;
  confui.gui.closecb = NULL;
  confui.gui.mainvnb = NULL;
  confui.gui.nbcb = NULL;
  confui.gui.vbox = NULL;
  confui.gui.statusMsg = NULL;
  confui.gui.errorMsg = NULL;
  confui.gui.pliSupported = PLI_SUPPORT_NONE;
  confui.gui.tablecurr = CONFUI_ID_NONE;
  confui.gui.dispsel = NULL;
  confui.gui.dispselduallist = NULL;
  confui.gui.filterDisplaySel = NULL;
  confui.gui.quickeditDisplaySel = NULL;
  confui.gui.edittaglist = NULL;
  confui.gui.audioidtaglist = NULL;
  confui.gui.listingtaglist = NULL;
  confui.gui.marqueetaglist = NULL;
  confui.gui.pluitaglist = NULL;
  confui.gui.inbuild = false;
  confui.gui.valid = 0;
  confui.gui.changed = false;
  confui.gui.dancedkey = LIST_VALUE_INVALID;
  confui.gui.inchange = false;
  confui.gui.org = NULL;
  confui.gui.itunes = NULL;
  confui.gui.filterLookup = NULL;

  confui.optiondf = NULL;
  confui.options = NULL;
  confui.optionsalloc = false;

  for (int i = 0; i < CONFUI_ID_TABLE_MAX; ++i) {
    for (int j = 0; j < CONFUI_TABLE_CB_MAX; ++j) {
      confui.gui.tables [i].callbacks [j] = NULL;
    }
    confui.gui.tables [i].uivl = NULL;
    confui.gui.tables [i].flags = CONFUI_TABLE_NONE;
    confui.gui.tables [i].changed = false;
    confui.gui.tables [i].currcount = 0;
    confui.gui.tables [i].saveidx = 0;
    confui.gui.tables [i].savelist = NULL;
    confui.gui.tables [i].listcreatefunc = NULL;
    confui.gui.tables [i].savefunc = NULL;
    confui.gui.tables [i].addfunc = NULL;
    confui.gui.tables [i].removefunc = NULL;
    confui.gui.tables [i].movefunc = NULL;
    for (int j = 0; j < CONFUI_BUTTON_TABLE_MAX; ++j) {
      confui.gui.tables [i].buttons [j] = NULL;
    }
  }

  for (int i = 0; i < CONFUI_ITEM_MAX; ++i) {
    confui.gui.uiitem [i].gui = &confui.gui;
    confui.gui.uiitem [i].uibutton = NULL;
    confui.gui.uiitem [i].basetype = CONFUI_NONE;
    confui.gui.uiitem [i].outtype = CONFUI_OUT_NONE;
    confui.gui.uiitem [i].bdjoptIdx = -1;
    confui.gui.uiitem [i].listidx = 0;
    confui.gui.uiitem [i].debuglvl = 0;
    confui.gui.uiitem [i].ddlist = NULL;
    confui.gui.uiitem [i].displist = NULL;
    confui.gui.uiitem [i].sbkeylist = NULL;
    confui.gui.uiitem [i].danceitemidx = DANCE_DANCE;
    confui.gui.uiitem [i].uilabelp = NULL;
    confui.gui.uiitem [i].uiwidgetp = NULL;
    confui.gui.uiitem [i].callback = NULL;
    confui.gui.uiitem [i].sfcb.entry = NULL;
    confui.gui.uiitem [i].sfcb.window = NULL;
    confui.gui.uiitem [i].uidd = NULL;
    confui.gui.uiitem [i].uri = NULL;
    confui.gui.uiitem [i].changed = false;
    confui.gui.uiitem [i].valid = true;
    confui.gui.uiitem [i].entrysz = 20;
    confui.gui.uiitem [i].entrymaxsz = 100;
    confui.gui.uiitem [i].widx = i;
  }

  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_ACRCLOUD_API_HOST, 45, 100);
  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_ACRCLOUD_API_KEY, 40, 40);
  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_ACRCLOUD_API_SECRET, 45, 45);
  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_AUDIOSRC_PASS, 10, 20);
  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_AUDIOSRC_URI, 45, 100);
  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_AUDIOSRC_USER, 10, 20);
  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_COMPLETE_MSG, 20, 30);
  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_DANCE_DANCE, 30, 50);
  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_DANCE_TAGS, 30, 100);
  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_MOBMQ_TITLE, 20, 100);
  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_PLAYER_ARGS, 45, 100);
  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_PROFILE_NAME, 20, 30);
  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_QUEUE_NM, 20, 30);
  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_RC_PASS, 10, 20);
  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_RC_USER_ID, 10, 30);

  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT, 30, 300);
  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_CHOOSE_ITUNES_DIR, 50, 300);
  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_CHOOSE_ITUNES_XML, 50, 300);
  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_CHOOSE_MUSIC_DIR, 50, 300);
  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_CHOOSE_SHUTDOWN, 50, 300);
  confuiEntrySetSize (&confui.gui, CONFUI_ENTRY_CHOOSE_STARTUP, 50, 300);

  osSetStandardSignals (confuiSigHandler);

  confui.dbgflags = BDJ4_INIT_NO_DB_LOAD;
  bdj4startup (argc, argv, NULL, "cfui", ROUTE_CONFIGUI, &confui.dbgflags);
  logProcBegin ();

  confui.gui.dispsel = dispselAlloc (DISP_SEL_LOAD_ALL);

  confuiInitGeneral (&confui.gui);
  confuiInitPlayer (&confui.gui);
  confuiInitMusicQs (&confui.gui);
  confuiInitMarquee (&confui.gui);
  confuiInitOrganization (&confui.gui);
  confuiInitDispSettings (&confui.gui);
  confuiInitEditDances (&confui.gui);
  confuiInitiTunes (&confui.gui);
  confuiAudioSourceInit (&confui.gui);
  confuiInitMobileRemoteControl (&confui.gui);

  confuiLoadTagList (&confui);
  confuiLoadThemeList (&confui.gui);

  confuiInitMobileMarquee (&confui.gui);

  confui.sf = songfilterAlloc ();
  confui.gui.filterDisplaySel = songfilterGetList (confui.sf);
  llist = nlistAlloc ("cu-filter-out", LIST_ORDERED, NULL);
  nlistStartIterator (confui.gui.filterDisplaySel, &iteridx);
  for (int i = 0; i < FILTER_DISP_MAX; ++i) {
    int     j;

    j = CONFUI_WIDGET_FILTER_START + 1 + i;
    nlistSetNum (llist, j, i);
  }
  confui.gui.filterLookup = llist;

  confui.qe = quickeditAlloc ();
  confui.gui.quickeditDisplaySel = quickeditGetList (confui.qe);
  llist = nlistAlloc ("cu-quickedit-out", LIST_ORDERED, NULL);
  nlistStartIterator (confui.gui.quickeditDisplaySel, &iteridx);
  for (int i = 0; i < QUICKEDIT_DISP_MAX; ++i) {
    int     j;

    j = CONFUI_WIDGET_QUICKEDIT_START + 1 + i;
    nlistSetNum (llist, j, i);
  }
  confui.gui.quickeditLookup = llist;

  listenPort = bdjvarsGetNum (BDJVL_PORT_CONFIGUI);
  confui.conn = connInit (ROUTE_CONFIGUI);

  pathbldMakePath (tbuff, sizeof (tbuff),
      CONFIGUI_OPT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  confui.optiondf = datafileAllocParse ("configui-opt", DFTYPE_KEY_VAL, tbuff,
      configuidfkeys, CONFUI_KEY_MAX, DF_NO_OFFSET, NULL);
  confui.options = datafileGetList (confui.optiondf);
  if (confui.options == NULL || nlistGetCount (confui.options) == 0) {
    confui.optionsalloc = true;
    confui.options = nlistAlloc ("configui-opt", LIST_ORDERED, NULL);

    nlistSetNum (confui.options, CONFUI_POSITION_X, -1);
    nlistSetNum (confui.options, CONFUI_POSITION_Y, -1);
    nlistSetNum (confui.options, CONFUI_SIZE_X, 1200);
    nlistSetNum (confui.options, CONFUI_SIZE_Y, 600);
  }

  /* register these after calling the sub-window initialization */
  /* then these will be run last, after the other closing callbacks */
  progstateSetCallback (confui.progstate, PROGSTATE_STOPPING,
      confuiStoppingCallback, &confui);
  progstateSetCallback (confui.progstate, PROGSTATE_STOP_WAIT,
      confuiStopWaitCallback, &confui);
  progstateSetCallback (confui.progstate, PROGSTATE_CLOSING,
      confuiClosingCallback, &confui);

  uiUIInitialize (sysvarsGetNum (SVL_LOCALE_TEXT_DIR));
  uiutilsInitSetup (&uisetup);
  uiSetUICSS (&uisetup);

  confuiBuildUI (&confui);
  osuiFinalize ();

  sockhMainLoop (listenPort, confuiProcessMsg, confuiMainLoop, &confui);
  connFree (confui.conn);
  progstateFree (confui.progstate);

  confuiCleanOrganization (&confui.gui);
  confuiCleaniTunes (&confui.gui);

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
confuiStoppingCallback (void *udata, programstate_t programState)
{
  configui_t    * confui = udata;
  int           x, y, ws;
  char          tmp [40];

  logProcBegin ();

  confuiPopulateOptions (&confui->gui);

  snprintf (tmp, sizeof (tmp), "%" PRId64, bdjoptGetNum (OPT_G_DEBUGLVL));
  connSendMessage (confui->conn, ROUTE_STARTERUI, MSG_DEBUG_LEVEL, tmp);

  bdjoptSave ();
  for (confuiident_t i = 0; i < CONFUI_ID_TABLE_MAX; ++i) {
    confuiTableSave (&confui->gui, i);
  }
  confuiSaveiTunes (&confui->gui);

  uiWindowGetSize (confui->gui.window, &x, &y);
  nlistSetNum (confui->options, CONFUI_SIZE_X, x);
  nlistSetNum (confui->options, CONFUI_SIZE_Y, y);
  uiWindowGetPosition (confui->gui.window, &x, &y, &ws);
  nlistSetNum (confui->options, CONFUI_POSITION_X, x);
  nlistSetNum (confui->options, CONFUI_POSITION_Y, y);

  datafileSave (confui->optiondf, NULL, confui->options, DF_NO_OFFSET, 1);

  songfilterSave (confui->sf, confui->gui.filterDisplaySel);
  quickeditSave (confui->qe, confui->gui.quickeditDisplaySel);

  connDisconnect (confui->conn, ROUTE_STARTERUI);

  logProcEnd ("");
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

  logProcBegin ();

  uiCloseWindow (confui->gui.window);
  uiCleanup ();

  uiwcontFree (confui->gui.window);

  for (int i = CONFUI_DD_BEGIN + 1; i < CONFUI_DD_MAX; ++i) {
    uiddFree (confui->gui.uiitem [i].uidd);
    ilistFree (confui->gui.uiitem [i].ddlist);
  }
  for (int i = CONFUI_ENTRY_BEGIN + 1; i < CONFUI_ENTRY_MAX; ++i) {
    uiwcontFree (confui->gui.uiitem [i].uiwidgetp);
  }
  for (int i = CONFUI_ENTRY_CHOOSE_BEGIN + 1; i < CONFUI_ENTRY_CHOOSE_MAX; ++i) {
    uiwcontFree (confui->gui.uiitem [i].uiwidgetp);
    uiwcontFree (confui->gui.uiitem [i].uibutton);
  }
  for (int i = CONFUI_SPINBOX_BEGIN + 1; i < CONFUI_SPINBOX_MAX; ++i) {
    uiwcontFree (confui->gui.uiitem [i].uiwidgetp);
    nlistFree (confui->gui.uiitem [i].displist);
    nlistFree (confui->gui.uiitem [i].sbkeylist);
  }
  for (int i = CONFUI_WIDGET_BEGIN + 1; i < CONFUI_WIDGET_MAX; ++i) {
    dataFree (confui->gui.uiitem [i].uri);
    uiwcontFree (confui->gui.uiitem [i].uiwidgetp);
  }
  for (int i = 0; i < CONFUI_ITEM_MAX; ++i) {
    uiwcontFree (confui->gui.uiitem [i].uilabelp);
    callbackFree (confui->gui.uiitem [i].callback);
  }
  for (int i = 0; i < CONFUI_ID_TABLE_MAX; ++i) {
    confuiTableFree (&confui->gui, i);
  }

  uiwcontFree (confui->gui.vbox);
  uiwcontFree (confui->gui.statusMsg);
  uiwcontFree (confui->gui.errorMsg);
  uiduallistFree (confui->gui.dispselduallist);
  songfilterFree (confui->sf);
  quickeditFree (confui->qe);
  nlistFree (confui->gui.filterLookup);
  nlistFree (confui->gui.quickeditLookup);
  dispselFree (confui->gui.dispsel);
  dataFree (confui->gui.localip);
  datafileFree (confui->optiondf);
  if (confui->optionsalloc) {
    nlistFree (confui->options);
  }
  slistFree (confui->gui.edittaglist);
  slistFree (confui->gui.audioidtaglist);
  slistFree (confui->gui.listingtaglist);
  slistFree (confui->gui.marqueetaglist);
  slistFree (confui->gui.pluitaglist);
  callbackFree (confui->gui.closecb);
  callbackFree (confui->gui.nbcb);
  uivnbFree (confui->gui.mainvnb);

  bdj4shutdown (ROUTE_CONFIGUI, NULL);

  logProcEnd ("");
  return STATE_FINISHED;
}

static void
confuiBuildUI (configui_t *confui)
{
  uiwcont_t     *hbox;
  uiwcont_t     *uiwidgetp;
  char          imgbuff [MAXPATHLEN];
  char          tbuff [MAXPATHLEN];
  int           x, y;
  uiutilsaccent_t accent;

  logProcBegin ();

  /* CONTEXT: configuration: configuration user interface window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Configuration"),
      bdjoptGetStr (OPT_P_PROFILENAME));
  confui->gui.closecb = callbackInit (confuiCloseWin, confui, NULL);
  confui->gui.window = uiCreateMainWindow (
      confui->gui.closecb, tbuff, "bdj4_icon_config");

  confui->gui.vbox = uiCreateVertBox ();
  uiWindowPackInWindow (confui->gui.window, confui->gui.vbox);
  uiWidgetSetAllMargins (confui->gui.vbox, 4);
  uiWidgetExpandHoriz (confui->gui.vbox);
  uiWidgetExpandVert (confui->gui.vbox);

  uiutilsAddProfileColorDisplay (confui->gui.vbox, &accent);
  hbox = accent.hbox;
  uiwcontFree (accent.cbox);

  uiwidgetp = uiCreateLabel ("");
  uiWidgetAddClass (uiwidgetp, ACCENT_CLASS);
  uiBoxPackEnd (hbox, uiwidgetp);
  confui->gui.statusMsg = uiwidgetp;

  uiwidgetp = uiCreateLabel ("");
  uiWidgetAddClass (uiwidgetp, ERROR_CLASS);
  uiBoxPackEnd (hbox, uiwidgetp);
  confui->gui.errorMsg = uiwidgetp;

  confui->gui.mainvnb = uivnbCreate (confui->gui.vbox);

  confuiBuildUIGeneral (&confui->gui);
  confuiBuildUIPlayer (&confui->gui);
  confuiBuildUIMusicQs (&confui->gui);
  confuiBuildUIMarquee (&confui->gui);
  confuiBuildUIUserInterface (&confui->gui);
  confuiBuildUIDispSettings (&confui->gui);
  confuiBuildUIDialogDisplay (&confui->gui);
  confuiBuildUIOrganization (&confui->gui);
  confuiBuildUIEditDances (&confui->gui);
  confuiBuildUIEditRatings (&confui->gui);
  confuiBuildUIEditStatus (&confui->gui);
  confuiBuildUIEditLevels (&confui->gui);
  confuiBuildUIEditGenres (&confui->gui);
  confuiBuildUIiTunes (&confui->gui);
  confuiAudioSourceBuildUI (&confui->gui);
  confuiBuildUIMobileRemoteControl (&confui->gui);
  confuiBuildUIMobileMarquee (&confui->gui);
  confuiBuildUIDebug (&confui->gui);

  confui->gui.nbcb = callbackInitI (confuiSwitchTable, &confui->gui);
  uivnbSetCallback (confui->gui.mainvnb, confui->gui.nbcb);

  x = nlistGetNum (confui->options, CONFUI_SIZE_X);
  y = nlistGetNum (confui->options, CONFUI_SIZE_Y);
  uiWindowSetDefaultSize (confui->gui.window, x, y);

  uiWidgetShowAll (confui->gui.window);

  x = nlistGetNum (confui->options, CONFUI_POSITION_X);
  y = nlistGetNum (confui->options, CONFUI_POSITION_Y);
  uiWindowMove (confui->gui.window, x, y, -1);

#if BDJ4_UI_GTK3
  {
    int ax, ay;
    int bx, by;
    int bugxoffset;
    int bugyoffset;

    /* work around a gtk bug */
    /* the widget offsets in the first page of the notebook are incorrect */
    /* they appear to not include the notebook tab width */
    /* use this offset to pass to the drop-down */
    /* but only for affected widgets */

    /* first widget on the first page */
    uiWidgetGetPosition (confui->gui.uiitem [CONFUI_ENTRY_CHOOSE_MUSIC_DIR].uilabelp, &ax, &ay);
    /* first widget on the second page */
    uiWidgetGetPosition (confui->gui.uiitem [CONFUI_SPINBOX_PLI].uilabelp, &bx, &by);
    bugxoffset = bx - ax;
    bugyoffset = by - ay;
    uiddSetOffset (confui->gui.uiitem [CONFUI_DD_LOCALE].uidd, bugxoffset, bugyoffset);
  }
#endif

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_config", ".png", PATHBLD_MP_DIR_IMG);
  osuiSetIcon (imgbuff);

  uiwcontFree (hbox);

  logProcEnd ("");
}


static int
confuiMainLoop (void *tconfui)
{
  configui_t    *confui = tconfui;
  int           stop = SOCKH_CONTINUE;

  uiUIProcessEvents ();

  if (! progstateIsRunning (confui->progstate)) {
    progstateProcess (confui->progstate);
    if (progstateCurrState (confui->progstate) == PROGSTATE_CLOSED) {
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

  for (int i = CONFUI_ENTRY_BEGIN + 1; i < CONFUI_ENTRY_MAX; ++i) {
    uiEntryValidate (confui->gui.uiitem [i].uiwidgetp, false);
  }
  for (int i = CONFUI_ENTRY_CHOOSE_BEGIN + 1; i < CONFUI_ENTRY_CHOOSE_MAX; ++i) {
    uiEntryValidate (confui->gui.uiitem [i].uiwidgetp, false);
  }

  confuiAudioSrcProcess (&confui->gui);

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

  logProcBegin ();
  connProcessUnconnected (confui->conn);

  progstateLogTime (confui->progstate, "time-to-start-gui");
  logProcEnd ("");
  return STATE_FINISHED;
}

static int
confuiProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  configui_t       *confui = udata;

  logProcBegin ();

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
          if (progstateCurrState (confui->progstate) <= PROGSTATE_RUNNING) {
            progstateShutdownProcess (confui->progstate);
          }
          break;
        }
        case MSG_WINDOW_FIND: {
          uiWindowFind (confui->gui.window);
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
  logProcEnd ("");
  return gKillReceived;
}


static bool
confuiCloseWin (void *udata)
{
  configui_t   *confui = udata;

  logProcBegin ();
  if (progstateCurrState (confui->progstate) <= PROGSTATE_RUNNING) {
    progstateShutdownProcess (confui->progstate);
    logMsg (LOG_DBG, LOG_MSGS, "got: close win request");
    logProcEnd ("not-done");
    return UICB_STOP;
  }

  logProcEnd ("");
  return UICB_STOP;
}

static void
confuiSigHandler (int sig)
{
  gKillReceived = true;
}

/* misc */

static void
confuiLoadTagList (configui_t *confui)
{
  slist_t       *llist = NULL;
  slist_t       *elist = NULL;
  slist_t       *aidlist = NULL;
  slist_t       *mlist = NULL;
  slist_t       *pluilist = NULL;

  logProcBegin ();

  llist = slistAlloc ("cu-list-tag-list", LIST_ORDERED, NULL);
  elist = slistAlloc ("cu-edit-tag-list", LIST_ORDERED, NULL);
  aidlist = slistAlloc ("cu-audio-id-tag-list", LIST_ORDERED, NULL);
  mlist = slistAlloc ("cu-marquee-tag-list", LIST_ORDERED, NULL);
  pluilist = slistAlloc ("cu-plui-tag-list", LIST_ORDERED, NULL);

  for (tagdefkey_t i = 0; i < TAG_KEY_MAX; ++i) {
    if (tagdefs [i].listingDisplay) {
      slistSetNum (llist, tagdefs [i].displayname, i);
    }
    if (tagdefs [i].isEditable ||
        (tagdefs [i].listingDisplay && tagdefs [i].editType == ET_LABEL)) {
      slistSetNum (elist, tagdefs [i].displayname, i);
    }
    if (tagdefs [i].isAudioID) {
      slistSetNum (aidlist, tagdefs [i].displayname, i);
    }
    if (tagdefs [i].marqueeDisp) {
      slistSetNum (mlist, tagdefs [i].displayname, i);
    }
    if (tagdefs [i].pluiDisp) {
      slistSetNum (pluilist, tagdefs [i].displayname, i);
    }
  }

  confui->gui.listingtaglist = llist;
  confui->gui.edittaglist = elist;
  confui->gui.audioidtaglist = aidlist;
  confui->gui.marqueetaglist = mlist;
  confui->gui.pluitaglist = pluilist;
  logProcEnd ("");
}

