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
#include <stdarg.h>

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
#include "progstate.h"
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
  long              dbgflags;
  int               stopwaitcount;
  datafile_t        *filterDisplayDf;
  confuigui_t       gui;
  /* options */
  datafile_t        *optiondf;
  nlist_t           *options;
  bool              optionsalloc : 1;
} configui_t;

static datafilekey_t configuidfkeys [CONFUI_KEY_MAX] = {
  { "CONFUI_POS_X",     CONFUI_POSITION_X,    VALUE_NUM, NULL, -1 },
  { "CONFUI_POS_Y",     CONFUI_POSITION_Y,    VALUE_NUM, NULL, -1 },
  { "CONFUI_SIZE_X",    CONFUI_SIZE_X,        VALUE_NUM, NULL, -1 },
  { "CONFUI_SIZE_Y",    CONFUI_SIZE_Y,        VALUE_NUM, NULL, -1 },
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

#if BDJ4_MEM_DEBUG
  mdebugInit ("cfui");
#endif

  confui.progstate = progstateInit ("configui");
  progstateSetCallback (confui.progstate, STATE_WAIT_HANDSHAKE,
      confuiHandshakeCallback, &confui);
  confui.locknm = NULL;
  confui.conn = NULL;
  confui.dbgflags = 0;
  confui.stopwaitcount = 0;
  confui.filterDisplayDf = NULL;

  confui.gui.localip = NULL;
  confui.gui.window = NULL;
  confui.gui.closecb = NULL;
  confui.gui.notebook = NULL;
  confui.gui.nbcb = NULL;
  confui.gui.nbtabid = uinbutilIDInit ();
  confui.gui.vbox = NULL;
  uiwcontInit (&confui.gui.statusMsg);
  confui.gui.tablecurr = CONFUI_ID_NONE;
  confui.gui.dispsel = NULL;
  confui.gui.dispselduallist = NULL;
  confui.gui.filterDisplaySel = NULL;
  confui.gui.edittaglist = NULL;
  confui.gui.listingtaglist = NULL;
  confui.gui.inbuild = false;
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
    confui.gui.tables [i].uitree = NULL;
    confui.gui.tables [i].sel = NULL;
    confui.gui.tables [i].flags = CONFUI_TABLE_NONE;
    confui.gui.tables [i].changed = false;
    confui.gui.tables [i].currcount = 0;
    confui.gui.tables [i].saveidx = 0;
    confui.gui.tables [i].savelist = NULL;
    confui.gui.tables [i].listcreatefunc = NULL;
    confui.gui.tables [i].savefunc = NULL;
    for (int j = 0; j < CONFUI_BUTTON_TABLE_MAX; ++j) {
      confui.gui.tables [i].buttons [j] = NULL;
    }
  }

  for (int i = 0; i < CONFUI_ITEM_MAX; ++i) {
    confui.gui.uiitem [i].uibutton = NULL;
    confui.gui.uiitem [i].basetype = CONFUI_NONE;
    confui.gui.uiitem [i].outtype = CONFUI_OUT_NONE;
    confui.gui.uiitem [i].bdjoptIdx = -1;
    confui.gui.uiitem [i].listidx = 0;
    confui.gui.uiitem [i].displist = NULL;
    confui.gui.uiitem [i].sbkeylist = NULL;
    confui.gui.uiitem [i].danceidx = DANCE_DANCE;
    uiwcontInit (&confui.gui.uiitem [i].uiwidget);
    confui.gui.uiitem [i].uiwidgetp = NULL;
    confui.gui.uiitem [i].callback = NULL;
    confui.gui.uiitem [i].uri = NULL;

    if (i > CONFUI_COMBOBOX_BEGIN && i < CONFUI_COMBOBOX_MAX) {
      confui.gui.uiitem [i].dropdown = uiDropDownInit ();
    }
    if (i > CONFUI_SPINBOX_BEGIN && i < CONFUI_SPINBOX_MAX) {
      if (i == CONFUI_SPINBOX_Q_MAX_PLAY_TIME ||
          i == CONFUI_SPINBOX_Q_STOP_AT_TIME) {
        confui.gui.uiitem [i].spinbox = uiSpinboxTimeInit (SB_TIME_BASIC);
      } else {
        confui.gui.uiitem [i].spinbox = uiSpinboxInit ();
      }
    }
    if (i > CONFUI_SWITCH_BEGIN && i < CONFUI_SWITCH_MAX) {
      confui.gui.uiitem [i].uiswitch = NULL;
    }
  }

  confui.gui.uiitem [CONFUI_ENTRY_DANCE_TAGS].entry = uiEntryInit (30, 100);
  confui.gui.uiitem [CONFUI_ENTRY_DANCE_DANCE].entry = uiEntryInit (30, 50);
  confui.gui.uiitem [CONFUI_ENTRY_MM_TITLE].entry = uiEntryInit (20, 100);
  confui.gui.uiitem [CONFUI_ENTRY_PROFILE_NAME].entry = uiEntryInit (20, 30);
  confui.gui.uiitem [CONFUI_ENTRY_COMPLETE_MSG].entry = uiEntryInit (20, 30);
  confui.gui.uiitem [CONFUI_ENTRY_QUEUE_NM].entry = uiEntryInit (20, 30);
  confui.gui.uiitem [CONFUI_ENTRY_RC_PASS].entry = uiEntryInit (10, 20);
  confui.gui.uiitem [CONFUI_ENTRY_RC_USER_ID].entry = uiEntryInit (10, 30);

  confui.gui.uiitem [CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT].entry = uiEntryInit (30, 300);
  confui.gui.uiitem [CONFUI_ENTRY_CHOOSE_ITUNES_DIR].entry = uiEntryInit (50, 300);
  confui.gui.uiitem [CONFUI_ENTRY_CHOOSE_ITUNES_XML].entry = uiEntryInit (50, 300);
  confui.gui.uiitem [CONFUI_ENTRY_CHOOSE_MUSIC_DIR].entry = uiEntryInit (50, 300);
  confui.gui.uiitem [CONFUI_ENTRY_CHOOSE_SHUTDOWN].entry = uiEntryInit (50, 300);
  confui.gui.uiitem [CONFUI_ENTRY_CHOOSE_STARTUP].entry = uiEntryInit (50, 300);

  osSetStandardSignals (confuiSigHandler);

  confui.dbgflags = BDJ4_INIT_NO_DB_LOAD;
  bdj4startup (argc, argv, NULL, "cfui", ROUTE_CONFIGUI, &confui.dbgflags);
  logProcBegin (LOG_PROC, "configui");

  confui.gui.dispsel = dispselAlloc ();

  confuiInitGeneral (&confui.gui);
  confuiInitPlayer (&confui.gui);
  confuiInitMusicQs (&confui.gui);
  confuiInitMarquee (&confui.gui);
  confuiInitOrganization (&confui.gui);
  confuiInitDispSettings (&confui.gui);
  confuiInitEditDances (&confui.gui);
  confuiInitiTunes (&confui.gui);
  confuiInitMobileRemoteControl (&confui.gui);

  confuiLoadTagList (&confui);
  confuiLoadThemeList (&confui.gui);

  confuiInitMobileMarquee (&confui.gui);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "ds-songfilter", BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  confui.filterDisplayDf = datafileAllocParse ("cu-filter",
      DFTYPE_KEY_VAL, tbuff, filterdisplaydfkeys, FILTER_DISP_MAX);
  confui.gui.filterDisplaySel = datafileGetList (confui.filterDisplayDf);
  llist = nlistAlloc ("cu-filter-out", LIST_ORDERED, NULL);
  nlistStartIterator (confui.gui.filterDisplaySel, &iteridx);
  nlistSetNum (llist, CONFUI_WIDGET_FILTER_GENRE, FILTER_DISP_GENRE);
  nlistSetNum (llist, CONFUI_WIDGET_FILTER_DANCELEVEL, FILTER_DISP_DANCELEVEL);
  nlistSetNum (llist, CONFUI_WIDGET_FILTER_STATUS, FILTER_DISP_STATUS);
  nlistSetNum (llist, CONFUI_WIDGET_FILTER_FAVORITE, FILTER_DISP_FAVORITE);
  nlistSetNum (llist, CONFUI_SWITCH_FILTER_STATUS_PLAYABLE, FILTER_DISP_STATUSPLAYABLE);
  confui.gui.filterLookup = llist;

  listenPort = bdjvarsGetNum (BDJVL_CONFIGUI_PORT);
  confui.conn = connInit (ROUTE_CONFIGUI);

  pathbldMakePath (tbuff, sizeof (tbuff),
      CONFIGUI_OPT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  confui.optiondf = datafileAllocParse ("configui-opt", DFTYPE_KEY_VAL, tbuff,
      configuidfkeys, CONFUI_KEY_MAX);
  confui.options = datafileGetList (confui.optiondf);
  if (confui.options == NULL) {
    confui.optionsalloc = true;
    confui.options = nlistAlloc ("configui-opt", LIST_ORDERED, NULL);

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
  uiSetUICSS (uiutilsGetCurrentFont (),
      bdjoptGetStr (OPT_P_UI_ACCENT_COL),
      bdjoptGetStr (OPT_P_UI_ERROR_COL));

  confuiBuildUI (&confui);
  osuiFinalize ();

  sockhMainLoop (listenPort, confuiProcessMsg, confuiMainLoop, &confui);
  connFree (confui.conn);
  progstateFree (confui.progstate);

  confuiCleanOrganization (&confui.gui);
  confuiCleaniTunes (&confui.gui);

  logProcEnd (LOG_PROC, "configui", "");
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
  char          fn [MAXPATHLEN];
  char          tmp [40];

  logProcBegin (LOG_PROC, "confuiStoppingCallback");

  confuiPopulateOptions (&confui->gui);

  snprintf (tmp, sizeof (tmp), "%"PRId64, bdjoptGetNum (OPT_G_DEBUGLVL));
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

  pathbldMakePath (fn, sizeof (fn),
      CONFIGUI_OPT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("configui", fn, configuidfkeys,
      CONFUI_KEY_MAX, confui->options, 0);

  pathbldMakePath (fn, sizeof (fn),
      "ds-songfilter", BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("ds-songfilter", fn, filterdisplaydfkeys,
      FILTER_DISP_MAX, confui->gui.filterDisplaySel, 0);

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

  uiCloseWindow (confui->gui.window);
  uiCleanup ();

  uiwcontFree (confui->gui.window);

  for (int i = CONFUI_COMBOBOX_BEGIN + 1; i < CONFUI_COMBOBOX_MAX; ++i) {
    uiDropDownFree (confui->gui.uiitem [i].dropdown);
  }

  for (int i = CONFUI_ENTRY_BEGIN + 1; i < CONFUI_ENTRY_MAX; ++i) {
    uiEntryFree (confui->gui.uiitem [i].entry);
  }
  for (int i = CONFUI_ENTRY_CHOOSE_BEGIN + 1; i < CONFUI_ENTRY_CHOOSE_MAX; ++i) {
    uiEntryFree (confui->gui.uiitem [i].entry);
    uiButtonFree (confui->gui.uiitem [i].uibutton);
  }
  for (int i = CONFUI_SPINBOX_BEGIN + 1; i < CONFUI_SPINBOX_MAX; ++i) {
    uiSpinboxFree (confui->gui.uiitem [i].spinbox);
    /* the mq and ui-theme share the list */
    if (i == CONFUI_SPINBOX_UI_THEME) {
      continue;
    }
    nlistFree (confui->gui.uiitem [i].displist);
    nlistFree (confui->gui.uiitem [i].sbkeylist);
  }
  for (int i = CONFUI_SWITCH_BEGIN + 1; i < CONFUI_SWITCH_MAX; ++i) {
    uiSwitchFree (confui->gui.uiitem [i].uiswitch);
  }
  for (int i = CONFUI_WIDGET_BEGIN + 1; i < CONFUI_WIDGET_MAX; ++i) {
    dataFree (confui->gui.uiitem [i].uri);
  }
  for (int i = 0; i < CONFUI_ITEM_MAX; ++i) {
    callbackFree (confui->gui.uiitem [i].callback);
    uiwcontFree (confui->gui.uiitem [i].uiwidgetp);
  }
  for (int i = 0; i < CONFUI_ID_TABLE_MAX; ++i) {
    confuiTableFree (&confui->gui, i);
  }

  uiwcontFree (confui->gui.vbox);
  uiduallistFree (confui->gui.dispselduallist);
  datafileFree (confui->filterDisplayDf);
  nlistFree (confui->gui.filterLookup);
  dispselFree (confui->gui.dispsel);
  dataFree (confui->gui.localip);
  datafileFree (confui->optiondf);
  if (confui->optionsalloc) {
    nlistFree (confui->options);
  }
  uinbutilIDFree (confui->gui.nbtabid);
  slistFree (confui->gui.listingtaglist);
  slistFree (confui->gui.edittaglist);
  callbackFree (confui->gui.closecb);
  callbackFree (confui->gui.nbcb);
  uiwcontFree (confui->gui.notebook);

  bdj4shutdown (ROUTE_CONFIGUI, NULL);

  logProcEnd (LOG_PROC, "confuiClosingCallback", "");
  return STATE_FINISHED;
}

static void
confuiBuildUI (configui_t *confui)
{
  uiwcont_t     *hbox;
  uiwcont_t     uiwidget;
  char          imgbuff [MAXPATHLEN];
  char          tbuff [MAXPATHLEN];
  int           x, y;
  uiutilsaccent_t accent;

  logProcBegin (LOG_PROC, "confuiBuildUI");

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_config", ".svg", PATHBLD_MP_DIR_IMG);
  /* CONTEXT: configuration: configuration user interface window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Configuration"),
      bdjoptGetStr (OPT_P_PROFILENAME));
  confui->gui.closecb = callbackInit (confuiCloseWin, confui, NULL);
  confui->gui.window = uiCreateMainWindow (confui->gui.closecb, tbuff, imgbuff);

  confui->gui.vbox = uiCreateVertBox ();
  uiWidgetExpandHoriz (confui->gui.vbox);
  uiWidgetExpandVert (confui->gui.vbox);
  uiWidgetSetAllMargins (confui->gui.vbox, 2);
  uiBoxPackInWindow (confui->gui.window, confui->gui.vbox);

  uiutilsAddAccentColorDisplay (confui->gui.vbox, &accent);
  hbox = accent.hbox;
  uiwcontFree (accent.label);

  uiCreateLabelOld (&uiwidget, "");
  uiWidgetSetClass (&uiwidget, ERROR_CLASS);
  uiBoxPackEnd (hbox, &uiwidget);
  uiwcontCopy (&confui->gui.statusMsg, &uiwidget);
  uiwcontFree (hbox);

  confui->gui.notebook = uiCreateNotebook ();
  uiWidgetSetClass (confui->gui.notebook, "confnotebook");
  uiNotebookTabPositionLeft (confui->gui.notebook);
  uiBoxPackStartExpand (confui->gui.vbox, confui->gui.notebook);

  confuiBuildUIGeneral (&confui->gui);
  confuiBuildUIPlayer (&confui->gui);
  confuiBuildUIMusicQs (&confui->gui);
  confuiBuildUIMarquee (&confui->gui);
  confuiBuildUIUserInterface (&confui->gui);
  confuiBuildUIDispSettings (&confui->gui);
  confuiBuildUIFilterDisplay (&confui->gui);
  confuiBuildUIOrganization (&confui->gui);
  confuiBuildUIEditDances (&confui->gui);
  confuiBuildUIEditRatings (&confui->gui);
  confuiBuildUIEditStatus (&confui->gui);
  confuiBuildUIEditLevels (&confui->gui);
  confuiBuildUIEditGenres (&confui->gui);
  confuiBuildUIiTunes (&confui->gui);
  confuiBuildUIMobileRemoteControl (&confui->gui);
  confuiBuildUIMobileMarquee (&confui->gui);
  confuiBuildUIDebug (&confui->gui);

  confui->gui.nbcb = callbackInitLong (confuiSwitchTable, &confui->gui);
  uiNotebookSetCallback (confui->gui.notebook, confui->gui.nbcb);

  x = nlistGetNum (confui->options, CONFUI_SIZE_X);
  y = nlistGetNum (confui->options, CONFUI_SIZE_Y);
  uiWindowSetDefaultSize (confui->gui.window, x, y);

  uiWidgetShowAll (confui->gui.window);

  x = nlistGetNum (confui->options, CONFUI_POSITION_X);
  y = nlistGetNum (confui->options, CONFUI_POSITION_Y);
  uiWindowMove (confui->gui.window, x, y, -1);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_config", ".png", PATHBLD_MP_DIR_IMG);
  osuiSetIcon (imgbuff);

  logProcEnd (LOG_PROC, "confuiBuildUI", "");
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

  for (int i = CONFUI_ENTRY_BEGIN + 1; i < CONFUI_ENTRY_MAX; ++i) {
    uiEntryValidate (confui->gui.uiitem [i].entry, false);
  }
  for (int i = CONFUI_ENTRY_CHOOSE_BEGIN + 1; i < CONFUI_ENTRY_CHOOSE_MAX; ++i) {
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

