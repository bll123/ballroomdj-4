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

#include <glib.h>
#include <zlib.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjvars.h"
#include "colorutils.h"
#include "conn.h"
#include "dirlist.h"
#include "dirop.h"
#include "fileop.h"
#include "inline.h"
#include "lock.h"
#include "log.h"
#include "nlist.h"
#include "osprocess.h"
#include "ossignal.h"
#include "osuiutils.h"
#include "pathbld.h"
#include "pathutil.h"
#include "procutil.h"
#include "progstate.h"
#include "sock.h"
#include "sockh.h"
#include "sysvars.h"
#include "templateutil.h"
#include "ui.h"
#include "volreg.h"
#include "webclient.h"

typedef enum {
  START_STATE_NONE,
  START_STATE_DELAY,
  START_STATE_SUPPORT_INIT,
  START_STATE_SUPPORT_SEND_MSG,
  START_STATE_SUPPORT_SEND_INFO,
  START_STATE_SUPPORT_SEND_FILES_DATA,
  START_STATE_SUPPORT_SEND_FILES_PROFILE,
  START_STATE_SUPPORT_SEND_FILES_MACHINE,
  START_STATE_SUPPORT_SEND_FILES_MACH_PROF,
  START_STATE_SUPPORT_SEND_DIAG,
  START_STATE_SUPPORT_SEND_DB_PRE,
  START_STATE_SUPPORT_SEND_DB,
  START_STATE_SUPPORT_FINISH,
  START_STATE_SUPPORT_SEND_FILE,
} startstate_t;

enum {
  SF_CONF_ONLY,
  SF_ALL,
  SF_MAC_DIAG,
  CLOSE_REQUEST,
  CLOSE_CRASH,
};

enum {
  MAX_WEB_RESP_SZ = 2048,
};

enum {
  START_CB_PLAYER,
  START_CB_MANAGE,
  START_CB_CONFIG,
  START_CB_SUPPORT,
  START_CB_EXIT,
  START_CB_SEND_SUPPORT,
  START_CB_MENU_STOP_ALL,
  START_CB_MENU_DEL_PROFILE,
  START_CB_MENU_ALT_SETUP,
  START_CB_SUPPORT_RESP,
  START_CB_SUPPORT_MSG_RESP,
  START_CB_MAX,
};

enum {
  START_LINK_CB_WIKI,
  START_LINK_CB_FORUM,
  START_LINK_CB_TICKETS,
  START_LINK_CB_MAX,
};

typedef struct {
  UICallback    cb;
  char          *uri;
} startlinkcb_t;

typedef struct {
  progstate_t     *progstate;
  char            *locknm;
  procutil_t      *processes [ROUTE_MAX];
  conn_t          *conn;
  int             currprofile;
  int             newprofile;
  int             maxProfileWidth;
  startstate_t    startState;
  startstate_t    nextState;
  startstate_t    delayState;
  int             delayCount;
  time_t          lastPluiStart;
  char            *supportDir;
  slist_t         *supportFileList;
  int             sendType;
  slistidx_t      supportFileIterIdx;
  char            *supportInFname;
  char            *supportOutFname;
  webclient_t     *webclient;
  char            ident [80];
  char            latestversion [60];
  char            *webresponse;
  int             mainstart [ROUTE_MAX];
  int             started [ROUTE_MAX];
  int             stopwaitcount;
  mstime_t        pluiCheckTime;
  nlist_t         *proflist;
  nlist_t         *profidxlist;
  UICallback      callbacks [START_CB_MAX];
  startlinkcb_t   macoslinkcb [START_LINK_CB_MAX];
  uispinbox_t     *profilesel;
  UIWidget        supportDialog;
  UIWidget        supportMsgDialog;
  UIWidget        supportSendFiles;
  UIWidget        supportSendDB;
  UIWidget        window;
  UIWidget        supportStatus;
  UIWidget        statusMsg;
  UIWidget        profileAccent;
  uitextbox_t     *supporttb;
  uientry_t       *supportsubject;
  uientry_t       *supportemail;
  /* options */
  datafile_t      *optiondf;
  nlist_t         *options;
} startui_t;

enum {
  STARTERUI_POSITION_X,
  STARTERUI_POSITION_Y,
  STARTERUI_SIZE_X,
  STARTERUI_SIZE_Y,
  STARTERUI_KEY_MAX,
};

static datafilekey_t starteruidfkeys [STARTERUI_KEY_MAX] = {
  { "STARTERUI_POS_X",     STARTERUI_POSITION_X,    VALUE_NUM, NULL, -1 },
  { "STARTERUI_POS_Y",     STARTERUI_POSITION_Y,    VALUE_NUM, NULL, -1 },
  { "STARTERUI_SIZE_X",    STARTERUI_SIZE_X,        VALUE_NUM, NULL, -1 },
  { "STARTERUI_SIZE_Y",    STARTERUI_SIZE_Y,        VALUE_NUM, NULL, -1 },
};

enum {
  SUPPORT_BUFF_SZ = (10*1024*1024),
  LOOP_DELAY = 5,
};

static bool     starterInitDataCallback (void *udata, programstate_t programState);
static bool     starterStoppingCallback (void *udata, programstate_t programState);
static bool     starterStopWaitCallback (void *udata, programstate_t programState);
static bool     starterClosingCallback (void *udata, programstate_t programState);
static void     starterBuildUI (startui_t *starter);
static int      starterMainLoop  (void *tstarter);
static int      starterProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static void     starterCloseProcess (startui_t *starter, bdjmsgroute_t routefrom, int reason);
static void     starterStartMain (startui_t *starter, bdjmsgroute_t routefrom, char *args);
static void     starterStopMain (startui_t *starter, bdjmsgroute_t routefrom);
static bool     starterCloseCallback (void *udata);
static void     starterSigHandler (int sig);

static bool     starterStartPlayerui (void *udata);
static bool     starterStartManageui (void *udata);
static bool     starterStartConfig (void *udata);

static int      starterGetProfiles (startui_t *starter);
static const char * starterSetProfile (void *udata, int idx);
static int      starterCheckProfile (startui_t *starter);

static bool     starterProcessSupport (void *udata);
static void     starterWebResponseCallback (void *userdata, char *resp, size_t len);
static bool     starterSupportResponseHandler (void *udata, long responseid);
static bool     starterCreateSupportDialog (void *udata);
static bool     starterSupportMsgHandler (void *udata, long responseid);
static void     starterSendFilesInit (startui_t *starter, char *dir, int type);
static void     starterSendFiles (startui_t *starter);
static void     starterSendFile (startui_t *starter, char *origfn, char *fn);

static void     starterCompressFile (char *infn, char *outfn);
static z_stream * starterGzipInit (char *out, int outsz);
static void     starterGzip (z_stream *zs, const char* in, int insz);
static size_t   starterGzipEnd (z_stream *zs);

static bool     starterStopAllProcesses (void *udata);
static int      starterCountProcesses (startui_t *starter);

static bool     starterWikiLinkHandler (void *udata);
static bool     starterForumLinkHandler (void *udata);
static bool     starterTicketLinkHandler (void *udata);
static void     starterLinkHandler (void *udata, int cbidx);

static void     starterSetWindowPosition (startui_t *starter);
static void     starterLoadOptions (startui_t *starter);
static bool     starterSetUpAlternate (void *udata);
static void     starterQuickConnect (startui_t *starter, bdjmsgroute_t route);
static void     starterSendPlayerActive (startui_t *starter);
static bool     starterDeleteProfile (void *udata);

static bool gKillReceived = false;
static bool gNewProfile = false;
static bool gStopProgram = false;

int
main (int argc, char *argv[])
{
  int             status = 0;
  startui_t       starter;
  uint16_t        listenPort;
  char            *uifont;
  int             flags;


  starter.progstate = progstateInit ("starterui");
  progstateSetCallback (starter.progstate, STATE_INITIALIZE_DATA,
      starterInitDataCallback, &starter);
  progstateSetCallback (starter.progstate, STATE_STOPPING,
      starterStoppingCallback, &starter);
  progstateSetCallback (starter.progstate, STATE_STOP_WAIT,
      starterStopWaitCallback, &starter);
  progstateSetCallback (starter.progstate, STATE_CLOSING,
      starterClosingCallback, &starter);
  starter.conn = NULL;
  starter.maxProfileWidth = 0;
  starter.startState = START_STATE_NONE;
  starter.nextState = START_STATE_NONE;
  starter.delayState = START_STATE_NONE;
  starter.delayCount = 0;
  starter.supportDir = NULL;
  starter.supportFileList = NULL;
  starter.sendType = SF_CONF_ONLY;
  starter.supportInFname = NULL;
  starter.supportOutFname = NULL;
  starter.webclient = NULL;
  starter.supporttb = NULL;
  starter.lastPluiStart = mstime ();
  strcpy (starter.ident, "");
  strcpy (starter.latestversion, "");
  for (int i = 0; i < ROUTE_MAX; ++i) {
    starter.mainstart [i] = 0;
    starter.started [i] = 0;
  }
  starter.stopwaitcount = 0;
  mstimeset (&starter.pluiCheckTime, 0);
  starter.proflist = NULL;
  starter.profidxlist = NULL;
  for (int i = 0; i < START_LINK_CB_MAX; ++i) {
    starter.macoslinkcb [i].uri = NULL;
  }
  uiutilsUIWidgetInit (&starter.window);
  uiutilsUIWidgetInit (&starter.supportStatus);
  uiutilsUIWidgetInit (&starter.supportSendFiles);
  uiutilsUIWidgetInit (&starter.supportSendDB);
  starter.optiondf = NULL;
  starter.options = NULL;

  procutilInitProcesses (starter.processes);

  osSetStandardSignals (starterSigHandler);

  flags = BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD |
      BDJ4_INIT_NO_LOCK;
  bdj4startup (argc, argv, NULL, "st", ROUTE_STARTERUI, flags);
  logProcBegin (LOG_PROC, "starterui");

  starter.profilesel = uiSpinboxTextInit ();

  starterLoadOptions (&starter);

  uiUIInitialize ();
  uifont = bdjoptGetStr (OPT_MP_UIFONT);
  uiSetUIFont (uifont);

  starterBuildUI (&starter);
  osuiFinalize ();

  while (! gStopProgram) {
    long loglevel;

    gNewProfile = false;
    listenPort = bdjvarsGetNum (BDJVL_STARTERUI_PORT);
    starter.conn = connInit (ROUTE_STARTERUI);

    sockhMainLoop (listenPort, starterProcessMsg, starterMainLoop, &starter);

    if (gNewProfile) {
      connDisconnectAll (starter.conn);
      connFree (starter.conn);
      logEnd ();
      loglevel = bdjoptGetNum (OPT_G_DEBUGLVL);
      logStart (lockName (ROUTE_STARTERUI), "st", loglevel);
    }
  }
  connFree (starter.conn);
  progstateFree (starter.progstate);
  logProcEnd (LOG_PROC, "starterui", "");
  logEnd ();
  return status;
}

/* internal routines */

static bool
starterInitDataCallback (void *udata, programstate_t programState)
{
  startui_t   *starter = udata;
  char        tbuff [MAXPATHLEN];

  pathbldMakePath (tbuff, sizeof (tbuff),
      "newinstall", BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
  if (fileopFileExists (tbuff)) {
    const char  *targv [2];
    int         targc = 0;

    targv [targc++] = NULL;
    starter->processes [ROUTE_HELPERUI] = procutilStartProcess (
        ROUTE_HELPERUI, "bdj4helperui", PROCUTIL_DETACH, targv);
    fileopDelete (tbuff);
  }

  return STATE_FINISHED;
}

static bool
starterStoppingCallback (void *udata, programstate_t programState)
{
  startui_t   *starter = udata;
  int         x, y, ws;

  logProcBegin (LOG_PROC, "starterStoppingCallback");

  for (int route = 0; route < ROUTE_MAX; ++route) {
    if (starter->started [route] > 0) {
      procutilStopProcess (starter->processes [route],
          starter->conn, route, false);
      starter->started [route] = 0;
    }
  }

  uiWindowGetSize (&starter->window, &x, &y);
  nlistSetNum (starter->options, STARTERUI_SIZE_X, x);
  nlistSetNum (starter->options, STARTERUI_SIZE_Y, y);
  uiWindowGetPosition (&starter->window, &x, &y, &ws);
  nlistSetNum (starter->options, STARTERUI_POSITION_X, x);
  nlistSetNum (starter->options, STARTERUI_POSITION_Y, y);

  procutilStopAllProcess (starter->processes, starter->conn, false);

  logProcEnd (LOG_PROC, "starterStoppingCallback", "");
  return STATE_FINISHED;
}

static bool
starterStopWaitCallback (void *udata, programstate_t programState)
{
  startui_t   *starter = udata;
  bool        rc;

  rc = connWaitClosed (starter->conn, &starter->stopwaitcount);
  return rc;
}

static bool
starterClosingCallback (void *udata, programstate_t programState)
{
  startui_t   *starter = udata;
  char        fn [MAXPATHLEN];

  logProcBegin (LOG_PROC, "starterClosingCallback");
  uiCloseWindow (&starter->window);

  procutilStopAllProcess (starter->processes, starter->conn, true);
  procutilFreeAll (starter->processes);

  bdj4shutdown (ROUTE_STARTERUI, NULL);

  webclientClose (starter->webclient);
  uiTextBoxFree (starter->supporttb);
  uiSpinboxTextFree (starter->profilesel);

  pathbldMakePath (fn, sizeof (fn),
      STARTERUI_OPT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("starterui", fn, starteruidfkeys, STARTERUI_KEY_MAX, starter->options);

  for (int i = 0; i < START_LINK_CB_MAX; ++i) {
    dataFree (starter->macoslinkcb [i].uri);
  }
  if (starter->optiondf != NULL) {
    datafileFree (starter->optiondf);
  } else if (starter->options != NULL) {
    nlistFree (starter->options);
  }
  dataFree (starter->supportDir);
  slistFree (starter->supportFileList);
  nlistFree (starter->proflist);
  nlistFree (starter->profidxlist);

  logProcEnd (LOG_PROC, "starterClosingCallback", "");
  return STATE_FINISHED;
}

static void
starterBuildUI (startui_t  *starter)
{
  UIWidget  uiwidget;
  UIWidget  *uiwidgetp;
  UIWidget  menubar;
  UIWidget  menu;
  UIWidget  menuitem;
  UIWidget  vbox;
  UIWidget  bvbox;
  UIWidget  hbox;
  UIWidget  sg;
  char      imgbuff [MAXPATHLEN];
  char      tbuff [MAXPATHLEN];
  int       dispidx;

  logProcBegin (LOG_PROC, "starterBuildUI");
  uiutilsUIWidgetInit (&vbox);
  uiutilsUIWidgetInit (&bvbox);
  uiutilsUIWidgetInit (&hbox);
  uiCreateSizeGroupHoriz (&sg);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", BDJ4_IMG_SVG_EXT, PATHBLD_MP_IMGDIR);
  uiutilsUICallbackInit (&starter->callbacks [START_CB_EXIT],
      starterCloseCallback, starter, NULL);
  uiCreateMainWindow (&starter->window,
      &starter->callbacks [START_CB_EXIT],
      bdjoptGetStr (OPT_P_PROFILENAME), imgbuff);

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, uiBaseMarginSz * 2);
  uiBoxPackInWindow (&starter->window, &vbox);

  uiCreateHorizBox (&hbox);
  uiWidgetSetMarginTop (&hbox, uiBaseMarginSz * 4);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateLabel (&uiwidget, "");
  uiWidgetSetSizeRequest (&uiwidget, 25, 25);
  uiWidgetSetMarginStart (&uiwidget, uiBaseMarginSz * 3);
  uiLabelSetBackgroundColor (&uiwidget, bdjoptGetStr (OPT_P_UI_PROFILE_COL));
  uiBoxPackEnd (&hbox, &uiwidget);
  uiutilsUIWidgetCopy (&starter->profileAccent, &uiwidget);

  uiCreateLabel (&uiwidget, "");
  uiLabelSetColor (&uiwidget, bdjoptGetStr (OPT_P_UI_ERROR_COL));
  uiBoxPackEnd (&hbox, &uiwidget);
  uiutilsUIWidgetCopy (&starter->statusMsg, &uiwidget);

  uiCreateMenubar (&menubar);
  uiBoxPackStart (&hbox, &menubar);

  /* CONTEXT: starterui: action menu for the starter user interface */
  uiMenuCreateItem (&menubar, &menuitem, _("Actions"), NULL);

  uiCreateSubMenu (&menuitem, &menu);

  /* CONTEXT: starterui: menu item: stop all BDJ4 processes */
  snprintf (tbuff, sizeof (tbuff), _("Stop All %s Processes"), BDJ4_NAME);
  uiutilsUICallbackInit (&starter->callbacks [START_CB_MENU_STOP_ALL],
      starterStopAllProcesses, starter, NULL);
  uiMenuCreateItem (&menu, &menuitem, tbuff,
      &starter->callbacks [START_CB_MENU_STOP_ALL]);

  uiutilsUICallbackInit (&starter->callbacks [START_CB_MENU_DEL_PROFILE],
      starterDeleteProfile, starter, NULL);
  /* CONTEXT: starterui: menu item: delete profile */
  uiMenuCreateItem (&menu, &menuitem, _("Delete Profile"),
      &starter->callbacks [START_CB_MENU_DEL_PROFILE]);

  pathbldMakePath (tbuff, sizeof (tbuff),
      ALT_COUNT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
  if (fileopFileExists (tbuff)) {
    /* CONTEXT: starterui: menu item: install in alternate folder */
    snprintf (tbuff, sizeof (tbuff), _("Set Up Alternate Folder"));
    uiutilsUICallbackInit (&starter->callbacks [START_CB_MENU_ALT_SETUP],
        starterSetUpAlternate, starter, NULL);
    uiMenuCreateItem (&menu, &menuitem, tbuff,
        &starter->callbacks [START_CB_MENU_ALT_SETUP]);
  }

  /* main display */
  uiCreateHorizBox (&hbox);
  uiWidgetSetMarginTop (&hbox, uiBaseMarginSz * 4);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: starterui: profile to be used when starting BDJ4 */
  uiCreateColonLabel (&uiwidget, _("Profile"));
  uiBoxPackStart (&hbox, &uiwidget);

  /* get the profile list after bdjopt has been initialized */
  dispidx = starterGetProfiles (starter);
  uiSpinboxTextCreate (starter->profilesel, starter);
  uiSpinboxTextSet (starter->profilesel, 0,
      nlistGetCount (starter->proflist), starter->maxProfileWidth,
      starter->proflist, NULL, starterSetProfile);
  uiSpinboxTextSetValue (starter->profilesel, dispidx);
  uiwidgetp = uiSpinboxGetUIWidget (starter->profilesel);
  uiWidgetSetMarginStart (uiwidgetp, uiBaseMarginSz * 4);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiBoxPackStart (&hbox, uiwidgetp);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateVertBox (&bvbox);
  uiBoxPackStart (&hbox, &bvbox);

  pathbldMakePath (tbuff, sizeof (tbuff),
     "bdj4_icon", ".svg", PATHBLD_MP_IMGDIR);
  uiImageScaledFromFile (&uiwidget, tbuff, 128);
  uiWidgetExpandHoriz (&uiwidget);
  uiWidgetSetAllMargins (&uiwidget, uiBaseMarginSz * 10);
  uiBoxPackStart (&hbox, &uiwidget);

  uiutilsUICallbackInit (&starter->callbacks [START_CB_PLAYER],
      starterStartPlayerui, starter, NULL);
  uiCreateButton (&uiwidget,
      &starter->callbacks [START_CB_PLAYER],
      /* CONTEXT: starterui: button: starts the player user interface */
      _("Player"), NULL);
  uiWidgetSetMarginTop (&uiwidget, uiBaseMarginSz * 2);
  uiWidgetAlignHorizStart (&uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);
  uiBoxPackStart (&bvbox, &uiwidget);
  uiButtonAlignLeft (&uiwidget);

  uiutilsUICallbackInit (&starter->callbacks [START_CB_MANAGE],
      starterStartManageui, starter, NULL);
  uiCreateButton (&uiwidget,
      &starter->callbacks [START_CB_MANAGE],
      /* CONTEXT: starterui: button: starts the management user interface */
      _("Manage"), NULL);
  uiWidgetSetMarginTop (&uiwidget, uiBaseMarginSz * 2);
  uiWidgetAlignHorizStart (&uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);
  uiBoxPackStart (&bvbox, &uiwidget);
  uiButtonAlignLeft (&uiwidget);

  uiutilsUICallbackInit (&starter->callbacks [START_CB_CONFIG],
      starterStartConfig, starter, NULL);
  uiCreateButton (&uiwidget,
      &starter->callbacks [START_CB_CONFIG],
      /* CONTEXT: starterui: button: starts the configuration user interface */
      _("Configure"), NULL);
  uiWidgetSetMarginTop (&uiwidget, uiBaseMarginSz * 2);
  uiWidgetAlignHorizStart (&uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);
  uiBoxPackStart (&bvbox, &uiwidget);
  uiButtonAlignLeft (&uiwidget);

  uiutilsUICallbackInit (&starter->callbacks [START_CB_SUPPORT],
      starterProcessSupport, starter, NULL);
  uiCreateButton (&uiwidget,
      &starter->callbacks [START_CB_SUPPORT],
      /* CONTEXT: starterui: button: support : support information */
      _("Support"), NULL);
  uiWidgetSetMarginTop (&uiwidget, uiBaseMarginSz * 2);
  uiWidgetAlignHorizStart (&uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);
  uiBoxPackStart (&bvbox, &uiwidget);
  uiButtonAlignLeft (&uiwidget);

  uiCreateButton (&uiwidget,
      &starter->callbacks [START_CB_EXIT],
      /* CONTEXT: starterui: button: exits BDJ4 (exits everything) */
      _("Exit"), NULL);
  uiWidgetSetMarginTop (&uiwidget, uiBaseMarginSz * 2);
  uiWidgetAlignHorizStart (&uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);
  uiBoxPackStart (&bvbox, &uiwidget);
  uiButtonAlignLeft (&uiwidget);

  starterSetWindowPosition (starter);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", ".png", PATHBLD_MP_IMGDIR);
  osuiSetIcon (imgbuff);

  uiWidgetShowAll (&starter->window);

  logProcEnd (LOG_PROC, "starterBuildUI", "");
}

int
starterMainLoop (void *tstarter)
{
  startui_t   *starter = tstarter;
  int         stop = FALSE;
  /* support message handling */
  char        tbuff [MAXPATHLEN];
  char        ofn [MAXPATHLEN];


  uiUIProcessEvents ();

  if (gNewProfile) {
    return true;
  }

  if (! progstateIsRunning (starter->progstate)) {
    progstateProcess (starter->progstate);
    if (progstateCurrState (starter->progstate) == STATE_CLOSED) {
      gStopProgram = true;
      stop = true;
    }
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      progstateShutdownProcess (starter->progstate);
      gKillReceived = false;
    }
    return stop;
  }

  /* will connect to any process from which handshakes have been received */
  connProcessUnconnected (starter->conn);

  /* try to connect if not connected */
  for (int route = 0; route < ROUTE_MAX; ++route) {
    if (starter->started [route]) {
      if (! connIsConnected (starter->conn, route)) {
        connConnect (starter->conn, route);
      }
    }
  }

  if (starter->started [ROUTE_PLAYERUI] &&
      mstimeCheck (&starter->pluiCheckTime) &&
      connIsConnected (starter->conn, ROUTE_PLAYERUI)) {
    /* check and see if the playerui is still connected */
    connSendMessage (starter->conn, ROUTE_PLAYERUI, MSG_NULL, NULL);
    if (! connIsConnected (starter->conn, ROUTE_PLAYERUI)) {
      starterCloseProcess (starter, ROUTE_PLAYERUI, CLOSE_CRASH);
    }
    mstimeset (&starter->pluiCheckTime, 500);
  }

  switch (starter->startState) {
    case START_STATE_NONE: {
      break;
    }
    case START_STATE_DELAY: {
      ++starter->delayCount;
      if (starter->delayCount > LOOP_DELAY) {
        starter->delayCount = 0;
        starter->startState = starter->delayState;
      }
      break;
    }
    case START_STATE_SUPPORT_INIT: {
      char        datestr [40];
      char        tmstr [40];

      tmutilDstamp (datestr, sizeof (datestr));
      tmutilShortTstamp (tmstr, sizeof (tmstr));
      snprintf (starter->ident, sizeof (starter->ident), "%s-%s-%s",
          sysvarsGetStr (SV_USER_MUNGE), datestr, tmstr);

      if (starter->webclient == NULL) {
        starter->webclient = webclientAlloc (starter, starterWebResponseCallback);
      }
      /* CONTEXT: starterui: support: status message */
      snprintf (tbuff, sizeof (tbuff), _("Sending Support Message"));
      uiLabelSetText (&starter->supportStatus, tbuff);
      starter->delayCount = 0;
      starter->delayState = START_STATE_SUPPORT_SEND_MSG;
      starter->startState = START_STATE_DELAY;
      break;
    }
    case START_STATE_SUPPORT_SEND_MSG: {
      const char  *email;
      const char  *subj;
      char        *msg;
      FILE        *fh;

      email = uiEntryGetValue (starter->supportemail);
      subj = uiEntryGetValue (starter->supportsubject);
      msg = uiTextBoxGetValue (starter->supporttb);

      strlcpy (tbuff, "support.txt", sizeof (tbuff));
      fh = fopen (tbuff, "w");
      if (fh != NULL) {
        fprintf (fh, " Ident  : %s\n", starter->ident);
        fprintf (fh, " E-Mail : %s\n", email);
        fprintf (fh, " Subject: %s\n", subj);
        fprintf (fh, " Message:\n%s\n", msg);
        fclose (fh);
      }
      free (msg);

      pathbldMakePath (ofn, sizeof (ofn),
          tbuff, ".gz.b64", PATHBLD_MP_TMPDIR);
      starterSendFile (starter, tbuff, ofn);
      fileopDelete (tbuff);

      /* CONTEXT: starterui: support: status message */
      snprintf (tbuff, sizeof (tbuff), _("Sending %s Information"), BDJ4_NAME);
      uiLabelSetText (&starter->supportStatus, tbuff);
      starter->delayCount = 0;
      starter->delayState = START_STATE_SUPPORT_SEND_INFO;
      starter->startState = START_STATE_DELAY;
      break;
    }
    case START_STATE_SUPPORT_SEND_INFO: {
      char        prog [MAXPATHLEN];
      char        arg [40];
      const char  *targv [4];
      int         targc = 0;

      pathbldMakePath (prog, sizeof (prog),
          "bdj4info", sysvarsGetStr (SV_OS_EXEC_EXT), PATHBLD_MP_EXECDIR);
      strlcpy (arg, "--bdj4", sizeof (arg));
      strlcpy (tbuff, "bdj4info.txt", sizeof (tbuff));
      targv [targc++] = prog;
      targv [targc++] = arg;
      targv [targc++] = NULL;
      osProcessStart (targv, OS_PROC_WAIT, NULL, tbuff);
      pathbldMakePath (ofn, sizeof (ofn),
          tbuff, ".gz.b64", PATHBLD_MP_TMPDIR);
      starterSendFile (starter, tbuff, ofn);
      fileopDelete (tbuff);

      strlcpy (tbuff, "VERSION.txt", sizeof (tbuff));
      pathbldMakePath (ofn, sizeof (ofn),
          tbuff, ".gz.b64", PATHBLD_MP_TMPDIR);
      starterSendFile (starter, tbuff, ofn);

      starter->startState = START_STATE_SUPPORT_SEND_FILES_DATA;
      break;
    }
    case START_STATE_SUPPORT_SEND_FILES_DATA: {
      bool        sendfiles;

      sendfiles = uiToggleButtonIsActive (&starter->supportSendFiles);
      if (! sendfiles) {
        starter->startState = START_STATE_SUPPORT_SEND_DIAG;
        break;
      }

      pathbldMakePath (tbuff, sizeof (tbuff),
          "", "", PATHBLD_MP_DATA);
      starterSendFilesInit (starter, tbuff, SF_CONF_ONLY);
      starter->startState = START_STATE_SUPPORT_SEND_FILE;
      starter->nextState = START_STATE_SUPPORT_SEND_FILES_PROFILE;
      break;
    }
    case START_STATE_SUPPORT_SEND_FILES_PROFILE: {
      pathbldMakePath (tbuff, sizeof (tbuff),
          "", "", PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
      starterSendFilesInit (starter, tbuff, SF_CONF_ONLY);
      starter->startState = START_STATE_SUPPORT_SEND_FILE;
      starter->nextState = START_STATE_SUPPORT_SEND_FILES_MACHINE;
      break;
    }
    case START_STATE_SUPPORT_SEND_FILES_MACHINE: {
      pathbldMakePath (tbuff, sizeof (tbuff),
          "", "", PATHBLD_MP_DATA | PATHBLD_MP_HOSTNAME);
      starterSendFilesInit (starter, tbuff, SF_CONF_ONLY);
      starter->startState = START_STATE_SUPPORT_SEND_FILE;
      starter->nextState = START_STATE_SUPPORT_SEND_FILES_MACH_PROF;
      break;
    }
    case START_STATE_SUPPORT_SEND_FILES_MACH_PROF: {
      pathbldMakePath (tbuff, sizeof (tbuff),
          "", "", PATHBLD_MP_DATA | PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
      /* will end up with the backup bdjconfig.txt file also, but that's ok */
      /* need all of the log files */
      starterSendFilesInit (starter, tbuff, SF_ALL);
      starter->startState = START_STATE_SUPPORT_SEND_FILE;
      starter->nextState = START_STATE_SUPPORT_SEND_DIAG;
      break;
    }
    case START_STATE_SUPPORT_SEND_DIAG: {
      pathbldMakePath (ofn, sizeof (ofn),
          "core", "", PATHBLD_MP_DATATOPDIR);
      if (fileopFileExists (ofn)) {
        strlcpy (tbuff, "core", sizeof (tbuff));
        starterSendFile (starter, tbuff, ofn);
      }

      snprintf (tbuff, sizeof (tbuff), "%s/Library/Logs/DiagnosticReports",
          sysvarsGetStr (SV_HOME));
      starterSendFilesInit (starter, tbuff, SF_MAC_DIAG);

      starter->startState = START_STATE_SUPPORT_SEND_FILE;
      starter->nextState = START_STATE_SUPPORT_SEND_DB_PRE;
      break;
    }
    case START_STATE_SUPPORT_SEND_DB_PRE: {
      bool        senddb;

      senddb = uiToggleButtonIsActive (&starter->supportSendDB);
      if (! senddb) {
        starter->startState = START_STATE_SUPPORT_FINISH;
        break;
      }
      /* CONTEXT: starterui: support: status message */
      snprintf (tbuff, sizeof (tbuff), _("Sending %s"), "data/musicdb.dat");
      uiLabelSetText (&starter->supportStatus, tbuff);
      starterSendFilesInit (starter, tbuff, SF_CONF_ONLY);
      starter->delayCount = 0;
      starter->delayState = START_STATE_SUPPORT_SEND_DB;
      starter->startState = START_STATE_DELAY;
      break;
    }
    case START_STATE_SUPPORT_SEND_DB: {
      bool        senddb;

      senddb = uiToggleButtonIsActive (&starter->supportSendDB);
      if (senddb) {
        strlcpy (tbuff, "data/musicdb.dat", sizeof (tbuff));
        pathbldMakePath (ofn, sizeof (ofn),
            "musicdb.dat", ".gz.b64", PATHBLD_MP_TMPDIR);
        starterSendFile (starter, tbuff, ofn);
      }
      starter->startState = START_STATE_SUPPORT_FINISH;
      break;
    }
    case START_STATE_SUPPORT_FINISH: {
      webclientClose (starter->webclient);
      starter->webclient = NULL;
      uiEntryFree (starter->supportsubject);
      uiEntryFree (starter->supportemail);
      uiDialogDestroy (&starter->supportMsgDialog);
      starter->startState = START_STATE_NONE;
      break;
    }
    case START_STATE_SUPPORT_SEND_FILE: {
      starterSendFiles (starter);
      starter->delayCount = 0;
      starter->delayState = starter->startState;
      starter->startState = START_STATE_DELAY;
      break;
    }
  }

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (starter->progstate);
    gKillReceived = false;
  }
  return stop;
}

static int
starterProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  startui_t       *starter = udata;

  logProcBegin (LOG_PROC, "starterProcessMsg");

  logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
      routefrom, msgRouteDebugText (routefrom),
      route, msgRouteDebugText (route), msg, msgDebugText (msg), args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_STARTERUI: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          if (! connIsConnected (starter->conn, routefrom)) {
            connConnect (starter->conn, routefrom);
          }
          connProcessHandshake (starter->conn, routefrom);
          break;
        }
        case MSG_SOCKET_CLOSE: {
          starterCloseProcess (starter, routefrom, CLOSE_REQUEST);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          progstateShutdownProcess (starter->progstate);
          gKillReceived = false;
          break;
        }
        case MSG_REQ_PLAYERUI_ACTIVE: {
          if (routefrom == ROUTE_MANAGEUI) {
            starterSendPlayerActive (starter);
          }
          break;
        }
        case MSG_START_MAIN: {
          starterStartMain (starter, routefrom, args);
          break;
        }
        case MSG_STOP_MAIN: {
          starterStopMain (starter, routefrom);
          break;
        }
        case MSG_DB_ENTRY_UPDATE: {
          connSendMessage (starter->conn, ROUTE_MAIN, msg, args);
          connSendMessage (starter->conn, ROUTE_PLAYERUI, msg, args);
          break;
        }
        case MSG_DATABASE_UPDATE: {
          connSendMessage (starter->conn, ROUTE_MAIN, msg, args);
          connSendMessage (starter->conn, ROUTE_PLAYERUI, msg, args);
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

  logProcEnd (LOG_PROC, "starterProcessMsg", "");

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  return gKillReceived;
}

static void
starterCloseProcess (startui_t *starter, bdjmsgroute_t routefrom, int request)
{
  int   wasstarted;

  if (request == CLOSE_CRASH) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: %d/%s crashed", routefrom,
        msgRouteDebugText (routefrom));
  }

  wasstarted = starter->started [routefrom];

  procutilCloseProcess (starter->processes [routefrom],
      starter->conn, routefrom);
  procutilFreeRoute (starter->processes, routefrom);
  starter->processes [routefrom] = NULL;
  connDisconnect (starter->conn, routefrom);
  starter->started [routefrom] = 0;
  if (routefrom == ROUTE_MAIN) {
    starter->started [ROUTE_MAIN] = 0;
    starter->mainstart [ROUTE_PLAYERUI] = 0;
    starter->mainstart [ROUTE_MANAGEUI] = 0;
  }
  if (routefrom == ROUTE_PLAYERUI) {
    starterSendPlayerActive (starter);
  }

  if (request == CLOSE_CRASH && wasstarted && routefrom == ROUTE_PLAYERUI) {
    time_t  curr;

    curr = mstime ();
    if (curr > starter->lastPluiStart + 60000) {
      starterStartPlayerui (starter);
    }
  }
}

static void
starterStartMain (startui_t *starter, bdjmsgroute_t routefrom, char *args)
{
  int         flags;
  const char  *targv [2];
  int         targc = 0;

  if (starter->started [ROUTE_MAIN] == 0) {
    flags = PROCUTIL_DETACH;
    if (atoi (args)) {
      targv [targc++] = "--nomarquee";
    }
    targv [targc++] = NULL;

    starter->processes [ROUTE_MAIN] = procutilStartProcess (
        ROUTE_MAIN, "bdj4main", flags, targv);
  }

  /* if this ui already requested a start, let the ui know */
  if ( starter->mainstart [routefrom] ) {
    connSendMessage (starter->conn, routefrom, MSG_MAIN_ALREADY, NULL);
  }
  /* prevent multiple starts from the same ui */
  if ( ! starter->mainstart [routefrom] ) {
    ++starter->started [ROUTE_MAIN];
  }
  ++starter->mainstart [routefrom];
}


static void
starterStopMain (startui_t *starter, bdjmsgroute_t routefrom)
{
  if (starter->started [ROUTE_MAIN]) {
    --starter->started [ROUTE_MAIN];
    if (starter->started [ROUTE_MAIN] <= 0) {
      procutilStopProcess (starter->processes [ROUTE_MAIN],
          starter->conn, ROUTE_MAIN, false);
      starter->started [ROUTE_MAIN] = 0;
    }
    starter->mainstart [routefrom] = 0;
  }
}


static bool
starterCloseCallback (void *udata)
{
  startui_t   *starter = udata;

  if (progstateCurrState (starter->progstate) <= STATE_RUNNING) {
    progstateShutdownProcess (starter->progstate);
    logMsg (LOG_DBG, LOG_MSGS, "got: close win request");
  }

  return UICB_STOP;
}

static void
starterSigHandler (int sig)
{
  gKillReceived = true;
}

static bool
starterStartPlayerui (void *udata)
{
  startui_t      *starter = udata;

  if (starterCheckProfile (starter) < 0) {
    return UICB_STOP;
  }

  starter->lastPluiStart = mstime ();

  starter->processes [ROUTE_PLAYERUI] = procutilStartProcess (
      ROUTE_PLAYERUI, "bdj4playerui", PROCUTIL_DETACH, NULL);
  starter->started [ROUTE_PLAYERUI] = true;
  mstimeset (&starter->pluiCheckTime, 500);
  starterSendPlayerActive (starter);
  return UICB_CONT;
}

static bool
starterStartManageui (void *udata)
{
  startui_t      *starter = udata;

  if (starterCheckProfile (starter) < 0) {
    return UICB_STOP;
  }
  starter->processes [ROUTE_MANAGEUI] = procutilStartProcess (
      ROUTE_MANAGEUI, "bdj4manageui", PROCUTIL_DETACH, NULL);
  starter->started [ROUTE_MANAGEUI] = true;
  return UICB_CONT;
}

static bool
starterStartConfig (void *udata)
{
  startui_t      *starter = udata;

  if (starterCheckProfile (starter) < 0) {
    return UICB_STOP;
  }
  starter->processes [ROUTE_CONFIGUI] = procutilStartProcess (
      ROUTE_CONFIGUI, "bdj4configui", PROCUTIL_DETACH, NULL);
  starter->started [ROUTE_CONFIGUI] = true;
  return UICB_CONT;
}

static bool
starterProcessSupport (void *udata)
{
  startui_t     *starter = udata;
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  UIWidget      uidialog;
  UIWidget      sg;
  char          tbuff [MAXPATHLEN];
  char          uri [MAXPATHLEN];
  char          *builddate;
  char          *rlslvl;

  if (starterCheckProfile (starter) < 0) {
    return UICB_STOP;
  }

  if (*starter->latestversion == '\0') {
    if (starter->webclient == NULL) {
      starter->webclient = webclientAlloc (starter, starterWebResponseCallback);
    }
    snprintf (uri, sizeof (uri), "%s/%s",
        sysvarsGetStr (SV_HOST_WEB), sysvarsGetStr (SV_WEB_VERSION_FILE));
    webclientGet (starter->webclient, uri);
    strlcpy (starter->latestversion, starter->webresponse, sizeof (starter->latestversion));
    stringTrim (starter->latestversion);
  }

  uiutilsUICallbackLongInit (&starter->callbacks [START_CB_SUPPORT_RESP],
      starterSupportResponseHandler, starter);
  uiCreateDialog (&uidialog, &starter->window,
      &starter->callbacks [START_CB_SUPPORT_RESP],
      /* CONTEXT: starterui: title for the support dialog */
      _("Support"),
      /* CONTEXT: starterui: support dialog: closes the dialog */
      _("Close"),
      RESPONSE_CLOSE,
      NULL
      );

  uiCreateSizeGroupHoriz (&sg);

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, uiBaseMarginSz * 2);
  uiDialogPackInDialog (&uidialog, &vbox);

  /* begin line */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: starterui: basic support dialog, version display */
  snprintf (tbuff, sizeof (tbuff), _("%s Version"), BDJ4_NAME);
  uiCreateColonLabel (&uiwidget, tbuff);
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  builddate = sysvarsGetStr (SV_BDJ4_BUILDDATE);
  rlslvl = sysvarsGetStr (SV_BDJ4_RELEASELEVEL);
  if (strcmp (rlslvl, "") == 0) {
    builddate = "";
  }
  snprintf (tbuff, sizeof (tbuff), "%s %s %s",
      sysvarsGetStr (SV_BDJ4_VERSION), builddate, rlslvl);
  uiCreateLabel (&uiwidget, tbuff);
  uiBoxPackStart (&hbox, &uiwidget);

  /* begin line */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: starterui: basic support dialog, latest version display */
  uiCreateColonLabel (&uiwidget, _("Latest Version"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  uiCreateLabel (&uiwidget, starter->latestversion);
  uiBoxPackStart (&hbox, &uiwidget);

  /* begin line */
  /* CONTEXT: starterui: basic support dialog, list of support options */
  uiCreateColonLabel (&uiwidget, _("Support options"));
  uiBoxPackStart (&vbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  /* begin line */
  snprintf (uri, sizeof (uri), "%s%s",
      sysvarsGetStr (SV_HOST_WIKI), sysvarsGetStr (SV_URI_WIKI));
  /* CONTEXT: starterui: basic support dialog: support option */
  snprintf (tbuff, sizeof (tbuff), _("%s Wiki"), BDJ4_NAME);
  uiCreateLink (&uiwidget, tbuff, uri);
  if (isMacOS ()) {
    uiutilsUICallbackInit (&starter->macoslinkcb [START_LINK_CB_WIKI].cb,
        starterWikiLinkHandler, starter, NULL);
    starter->macoslinkcb [START_LINK_CB_WIKI].uri = strdup (uri);
    uiLinkSetActivateCallback (&uiwidget,
        &starter->macoslinkcb [START_LINK_CB_WIKI].cb);
  }
  uiBoxPackStart (&vbox, &uiwidget);

  /* begin line */
  snprintf (uri, sizeof (uri), "%s%s",
      sysvarsGetStr (SV_HOST_FORUM), sysvarsGetStr (SV_URI_FORUM));
  /* CONTEXT: starterui: basic support dialog: support option */
  snprintf (tbuff, sizeof (tbuff), _("%s Forums"), BDJ4_NAME);
  uiCreateLink (&uiwidget, tbuff, uri);
  if (isMacOS ()) {
    uiutilsUICallbackInit (&starter->macoslinkcb [START_LINK_CB_FORUM].cb,
        starterForumLinkHandler, starter, NULL);
    starter->macoslinkcb [START_LINK_CB_FORUM].uri = strdup (uri);
    uiLinkSetActivateCallback (&uiwidget,
        &starter->macoslinkcb [START_LINK_CB_FORUM].cb);
  }
  uiBoxPackStart (&vbox, &uiwidget);

  /* begin line */
  snprintf (uri, sizeof (uri), "%s%s",
      sysvarsGetStr (SV_HOST_TICKET), sysvarsGetStr (SV_URI_TICKET));
  /* CONTEXT: starterui: basic support dialog: support option */
  snprintf (tbuff, sizeof (tbuff), _("%s Support Tickets"), BDJ4_NAME);
  uiCreateLink (&uiwidget, tbuff, uri);
  if (isMacOS ()) {
    uiutilsUICallbackInit (&starter->macoslinkcb [START_LINK_CB_TICKETS].cb,
        starterTicketLinkHandler, starter, NULL);
    starter->macoslinkcb [START_LINK_CB_TICKETS].uri = strdup (uri);
    uiLinkSetActivateCallback (&uiwidget,
        &starter->macoslinkcb [START_LINK_CB_TICKETS].cb);
  }
  uiBoxPackStart (&vbox, &uiwidget);

  /* begin line */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiutilsUICallbackInit (&starter->callbacks [START_CB_SEND_SUPPORT],
      starterCreateSupportDialog, starter, NULL);
  uiCreateButton (&uiwidget,
      &starter->callbacks [START_CB_SEND_SUPPORT],
      /* CONTEXT: starterui: basic support dialog: button: support option */
      _("Send Support Message"), NULL);
  uiBoxPackStart (&hbox, &uiwidget);

  /* the dialog doesn't have any space above the buttons */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateLabel (&uiwidget, " ");
  uiBoxPackStart (&hbox, &uiwidget);

  uiWidgetShowAll (&uidialog);
  uiutilsUIWidgetCopy (&starter->supportDialog, &uidialog);
  return UICB_CONT;
}


static bool
starterSupportResponseHandler (void *udata, long responseid)
{
  startui_t *starter = udata;

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      break;
    }
    case RESPONSE_CLOSE: {
      uiDialogDestroy (&starter->supportDialog);
      break;
    }
  }
  return UICB_CONT;
}

static int
starterGetProfiles (startui_t *starter)
{
  int         count;
  size_t      max;
  size_t      len;
  char        *pname = NULL;
  int         availprof = -1;
  nlist_t     *proflist = NULL;
  nlist_t     *profidxlist = NULL;
  bool        profileinuse = false;
  int         dispidx = -1;

  starter->newprofile = -1;
  starter->currprofile = sysvarsGetNum (SVL_BDJIDX);

  /* want the 'new profile' selection to always be last */
  /* and its index may not be last */
  /* use two lists, one with the display ordering, and an index list */
  proflist = nlistAlloc ("profile-list", LIST_ORDERED, free);
  profidxlist = nlistAlloc ("profile-idx-list", LIST_ORDERED, NULL);
  max = 0;

  count = 0;
  for (int i = 0; i < BDJOPT_MAX_PROFILES; ++i) {
    sysvarsSetNum (SVL_BDJIDX, i);

    if (bdjoptProfileExists ()) {
      pid_t   pid;

      pid = lockExists (lockName (ROUTE_STARTERUI), PATHBLD_MP_USEIDX);
      if (pid > 0) {
        /* do not add this selection to the profile list */
        if (starter->currprofile == i) {
          profileinuse = true;
        }
        continue;
      }

      if (profileinuse) {
        starter->currprofile = i;
        profileinuse = false;
      }

      if (i == starter->currprofile) {
        dispidx = count;
      }

      pname = bdjoptGetProfileName ();
      if (pname != NULL) {
        len = strlen (pname);
        max = len > max ? len : max;
        nlistSetStr (proflist, count, pname);
        nlistSetNum (profidxlist, count, i);
        free (pname);
      }
      ++count;
    } else if (availprof == -1) {
      if (i == starter->currprofile) {
        profileinuse = true;
      }
      availprof = i;
    }
  }

  /* CONTEXT: starterui: selection to create a new profile */
  nlistSetStr (proflist, count, _("Create Profile"));
  nlistSetNum (profidxlist, count, availprof);
  starter->newprofile = availprof;
  len = strlen (nlistGetStr (proflist, count));
  max = len > max ? len : max;
  starter->maxProfileWidth = (int) max;
  if (profileinuse) {
    dispidx = count;
    starter->currprofile = availprof;
  }

  nlistFree (starter->proflist);
  starter->proflist = proflist;
  nlistFree (starter->profidxlist);
  starter->profidxlist = profidxlist;

  sysvarsSetNum (SVL_BDJIDX, starter->currprofile);

  if (starter->currprofile != starter->newprofile) {
    bdjoptInit ();
    uiWindowSetTitle (&starter->window, bdjoptGetStr (OPT_P_PROFILENAME));
    uiLabelSetBackgroundColor (&starter->profileAccent,
        bdjoptGetStr (OPT_P_UI_PROFILE_COL));
    starterLoadOptions (starter);
    starterSetWindowPosition (starter);
  }

  bdjvarsAdjustPorts ();
  return dispidx;
}

static const char *
starterSetProfile (void *udata, int idx)
{
  startui_t   *starter = udata;
  const char  *disp;
  int         dispidx;
  int         profidx;
  int         chg;

  dispidx = uiSpinboxTextGetValue (starter->profilesel);
  disp = nlistGetStr (starter->proflist, dispidx);
  profidx = nlistGetNum (starter->profidxlist, dispidx);

  chg = profidx != starter->currprofile && profidx != starter->newprofile;
  starter->currprofile = profidx;
  sysvarsSetNum (SVL_BDJIDX, profidx);

  if (chg) {
    bdjoptInit ();

    uiLabelSetText (&starter->statusMsg, "");
    uiWindowSetTitle (&starter->window, bdjoptGetStr (OPT_P_PROFILENAME));
    uiLabelSetBackgroundColor (&starter->profileAccent,
        bdjoptGetStr (OPT_P_UI_PROFILE_COL));

    bdjvarsAdjustPorts ();
    gNewProfile = true;
  }

  return disp;
}

static int
starterCheckProfile (startui_t *starter)
{
  int   rc;

  uiLabelSetText (&starter->statusMsg, "");

  if (sysvarsGetNum (SVL_BDJIDX) == starter->newprofile) {
    char  tbuff [100];
    int   profidx;

    bdjoptInit ();
    profidx = sysvarsGetNum (SVL_BDJIDX);

    /* CONTEXT: starterui: name of the new profile (New profile 9) */
    snprintf (tbuff, sizeof (tbuff), _("New Profile %d"), profidx);
    bdjoptSetStr (OPT_P_PROFILENAME, tbuff);
    uiWindowSetTitle (&starter->window, tbuff);

    /* select a completely random color */
    createRandomColor (tbuff, sizeof (tbuff));
    bdjoptSetStr (OPT_P_UI_PROFILE_COL, tbuff);
    uiLabelSetBackgroundColor (&starter->profileAccent,
        bdjoptGetStr (OPT_P_UI_PROFILE_COL));

    bdjoptSave ();

    templateImageCopy (bdjoptGetStr (OPT_P_UI_ACCENT_COL));
    templateDisplaySettingsCopy ();
    /* do not re-run the new profile init */
    starter->newprofile = -1;
  }

  rc = lockAcquire (lockName (ROUTE_STARTERUI), PATHBLD_MP_USEIDX);
  if (rc < 0) {
    /* CONTEXT: starterui: profile is already in use */
    uiLabelSetText (&starter->statusMsg, _("Profile in use"));
  } else {
    uiWidgetDisable (uiSpinboxGetUIWidget (starter->profilesel));
  }

  return rc;
}


static bool
starterCreateSupportDialog (void *udata)
{
  startui_t     *starter = udata;
  UIWidget      uiwidget;
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uidialog;
  UIWidget      sg;
  uitextbox_t *tb;

  uiutilsUIWidgetInit (&uiwidget);
  uiutilsUIWidgetInit (&vbox);
  uiutilsUIWidgetInit (&hbox);

  uiutilsUICallbackLongInit (&starter->callbacks [START_CB_SUPPORT_MSG_RESP],
      starterSupportMsgHandler, starter);
  uiCreateDialog (&uidialog, &starter->window,
      &starter->callbacks [START_CB_SUPPORT_MSG_RESP],
      /* CONTEXT: starterui: title for the support message dialog */
      _("Support Message"),
      /* CONTEXT: starterui: support message dialog: closes the dialog */
      _("Close"),
      RESPONSE_CLOSE,
      /* CONTEXT: starterui: support message dialog: sends the support message */
      _("Send Support Message"),
      RESPONSE_APPLY,
      NULL
      );
  uiWindowSetDefaultSize (&uidialog, -1, 400);

  uiCreateSizeGroupHoriz (&sg);

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, uiBaseMarginSz * 2);
  uiDialogPackInDialog (&uidialog, &vbox);

  /* line 1 */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: starterui: sending support message: user's e-mail address */
  uiCreateColonLabel (&uiwidget, _("E-Mail Address"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  starter->supportemail = uiEntryInit (50, 100);
  uiEntryCreate (starter->supportemail);
  uiBoxPackStart (&hbox, uiEntryGetUIWidget (starter->supportemail));

  /* line 2 */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: starterui: sending support message: subject of message */
  uiCreateColonLabel (&uiwidget, _("Subject"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  starter->supportsubject = uiEntryInit (50, 100);
  uiEntryCreate (starter->supportsubject);
  uiBoxPackStart (&hbox, uiEntryGetUIWidget (starter->supportsubject));

  /* line 3 */
  /* CONTEXT: starterui: sending support message: message text */
  uiCreateColonLabel (&uiwidget, _("Message"));
  uiBoxPackStart (&vbox, &uiwidget);

  /* line 4 */
  tb = uiTextBoxCreate (200);
  uiTextBoxHorizExpand (tb);
  uiTextBoxVertExpand (tb);
  uiBoxPackStartExpand (&vbox, uiTextBoxGetScrolledWindow (tb));
  starter->supporttb = tb;

  /* line 5 */
  /* CONTEXT: starterui: sending support message: checkbox: option to send data files */
  uiCreateCheckButton (&starter->supportSendFiles, _("Attach Data Files"), 0);
  uiBoxPackStart (&vbox, &starter->supportSendFiles);

  /* line 6 */
  /* CONTEXT: starterui: sending support message: checkbox: option to send database */
  uiCreateCheckButton (&starter->supportSendDB, _("Attach Database"), 0);
  uiBoxPackStart (&vbox, &starter->supportSendDB);

  /* line 7 */
  uiCreateLabel (&uiwidget, "");
  uiBoxPackStart (&vbox, &uiwidget);
  uiLabelEllipsizeOn (&uiwidget);
  uiLabelSetColor (&uiwidget, bdjoptGetStr (OPT_P_UI_ACCENT_COL));
  uiutilsUIWidgetCopy (&starter->supportStatus, &uiwidget);

  uiWidgetShowAll (&uidialog);
  uiutilsUIWidgetCopy (&starter->supportMsgDialog, &uidialog);
  return UICB_CONT;
}


static bool
starterSupportMsgHandler (void *udata, long responseid)
{
  startui_t   *starter = udata;

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      break;
    }
    case RESPONSE_CLOSE: {
      uiDialogDestroy (&starter->supportMsgDialog);
      break;
    }
    case RESPONSE_APPLY: {
      starter->startState = START_STATE_SUPPORT_INIT;
      break;
    }
  }
  return UICB_CONT;
}

static void
starterSendFilesInit (startui_t *starter, char *dir, int type)
{
  slist_t     *list;
  char        *ext = BDJ4_CONFIG_EXT;

  if (type == SF_ALL) {
    ext = NULL;
  }
  if (type == SF_MAC_DIAG) {
    ext = ".crash";
  }
  starter->sendType = type;
  list = dirlistBasicDirList (dir, ext);
  starter->supportFileList = list;
  slistStartIterator (list, &starter->supportFileIterIdx);
  dataFree (starter->supportDir);
  starter->supportDir = strdup (dir);
}

static void
starterSendFiles (startui_t *starter)
{
  char        *fn;
  char        ifn [MAXPATHLEN];
  char        ofn [MAXPATHLEN];
  char        tbuff [100];

  if (starter->supportInFname != NULL) {
    starterSendFile (starter, starter->supportInFname, starter->supportOutFname);
    if (starter->sendType == SF_MAC_DIAG) {
      fileopDelete (starter->supportInFname);
    }
    free (starter->supportInFname);
    free (starter->supportOutFname);
    starter->supportInFname = NULL;
    starter->supportOutFname = NULL;
    return;
  }

  if ((fn = slistIterateKey (starter->supportFileList,
      &starter->supportFileIterIdx)) == NULL) {
    slistFree (starter->supportFileList);
    starter->supportFileList = NULL;
    dataFree (starter->supportDir);
    starter->supportDir = NULL;
    starter->startState = starter->nextState;
    return;
  }

  if (starter->sendType == SF_MAC_DIAG) {
    if (strncmp (fn, "bdj4", 4) != 0) {
      /* skip any .crash file that is not a bdj4 crash */
      return;
    }
  }

  strlcpy (ifn, starter->supportDir, sizeof (ifn));
  strlcat (ifn, "/", sizeof (ifn));
  strlcat (ifn, fn, sizeof (ifn));
  pathbldMakePath (ofn, sizeof (ofn),
      fn, ".gz.b64", PATHBLD_MP_TMPDIR);
  starter->supportInFname = strdup (ifn);
  starter->supportOutFname = strdup (ofn);
  /* CONTEXT: starterui: support: status message */
  snprintf (tbuff, sizeof (tbuff), _("Sending %s"), ifn);
  uiLabelSetText (&starter->supportStatus, tbuff);
}

static void
starterSendFile (startui_t *starter, char *origfn, char *fn)
{
  char        uri [1024];
  const char  *query [7];

  starterCompressFile (origfn, fn);
  snprintf (uri, sizeof (uri), "%s%s",
      sysvarsGetStr (SV_HOST_SUPPORTMSG), sysvarsGetStr (SV_URI_SUPPORTMSG));
  query [0] = "key";
  query [1] = "9034545";
  query [2] = "ident";
  query [3] = starter->ident;
  query [4] = "origfn";
  if (starter->sendType == SF_MAC_DIAG) {
    pathinfo_t    *pi;

    pi = pathInfo (origfn);
    query [5] = pi->filename;
    pathInfoFree (pi);
  } else {
    query [5] = origfn;
  }
  query [6] = NULL;
  webclientUploadFile (starter->webclient, uri, query, fn);
  fileopDelete (fn);
}

static void
starterWebResponseCallback (void *userdata, char *resp, size_t len)
{
  startui_t *starter = userdata;

  starter->webresponse = resp;
  return;
}

static void
starterCompressFile (char *infn, char *outfn)
{
  FILE      *infh = NULL;
  FILE      *outfh = NULL;
  char      *buff;
  char      *obuff;
  char      *data;
  size_t    r;
  size_t    olen;
  z_stream  *zs;

  infh = fileopOpen (infn, "rb");
  if (infh == NULL) {
    return;
  }
  outfh = fileopOpen (outfn, "wb");
  if (outfh == NULL) {
    return;
  }

  buff = malloc (SUPPORT_BUFF_SZ);
  assert (buff != NULL);
  /* if the database becomes so large that 10 megs compressed can't hold it */
  /* then there will be a problem */
  obuff = malloc (SUPPORT_BUFF_SZ);
  assert (obuff != NULL);

  zs = starterGzipInit (obuff, SUPPORT_BUFF_SZ);
  while ((r = fread (buff, 1, SUPPORT_BUFF_SZ, infh)) > 0) {
    starterGzip (zs, buff, r);
  }
  olen = starterGzipEnd (zs);
  data = g_base64_encode ((const guchar *) obuff, olen);
  fwrite (data, strlen (data), 1, outfh);
  free (data);

  fclose (infh);
  fclose (outfh);
  free (buff);
  free (obuff);
}

static z_stream *
starterGzipInit (char *out, int outsz)
{
  z_stream *zs;

  zs = malloc (sizeof (z_stream));
  zs->zalloc = Z_NULL;
  zs->zfree = Z_NULL;
  zs->opaque = Z_NULL;
  zs->avail_in = (uInt) 0;
  zs->next_in = (Bytef *) NULL;
  zs->avail_out = (uInt) outsz;
  zs->next_out = (Bytef *) out;

  // hard to believe they don't have a macro for gzip encoding, "Add 16" is the best thing zlib can do:
  // "Add 16 to windowBits to write a simple gzip header and trailer around the compressed data instead of a zlib wrapper"
  deflateInit2 (zs, 6, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);
  return zs;
}

static void
starterGzip (z_stream *zs, const char* in, int insz)
{
  zs->avail_in = (uInt) insz;
  zs->next_in = (Bytef *) in;

  deflate (zs, Z_NO_FLUSH);
}

static size_t
starterGzipEnd (z_stream *zs)
{
  size_t    olen;

  zs->avail_in = (uInt) 0;
  zs->next_in = (Bytef *) NULL;

  deflate (zs, Z_FINISH);
  olen = zs->total_out;
  deflateEnd (zs);
  free (zs);
  return olen;
}

static bool
starterStopAllProcesses (void *udata)
{
  startui_t     *starter = udata;
  bdjmsgroute_t route;
  char          *locknm;
  pid_t         pid;
  int           count;
  char          tbuff [MAXPATHLEN];


  logProcBegin (LOG_PROC, "starterStopAllProcesses");
  fprintf (stderr, "stop-all-processes\n");

  count = starterCountProcesses (starter);
  if (count == 0) {
    logProcEnd (LOG_PROC, "starterStopAllProcesses", "begin-none");
    fprintf (stderr, "done\n");
    return UICB_CONT;
  }

  /* send the standard exit request to the main controlling processes first */
  logMsg (LOG_DBG, LOG_IMPORTANT, "send exit request to ui");
  fprintf (stderr, "send exit request to ui\n");
  starterQuickConnect (starter, ROUTE_PLAYERUI);
  connSendMessage (starter->conn, ROUTE_PLAYERUI, MSG_EXIT_REQUEST, NULL);
  starterQuickConnect (starter, ROUTE_MANAGEUI);
  connSendMessage (starter->conn, ROUTE_MANAGEUI, MSG_EXIT_REQUEST, NULL);
  starterQuickConnect (starter, ROUTE_CONFIGUI);
  connSendMessage (starter->conn, ROUTE_CONFIGUI, MSG_EXIT_REQUEST, NULL);

  logMsg (LOG_DBG, LOG_IMPORTANT, "sleeping");
  fprintf (stderr, "sleeping\n");
  mssleep (1000);

  count = starterCountProcesses (starter);
  if (count == 0) {
    logProcEnd (LOG_PROC, "starterStopAllProcesses", "after-ui");
    fprintf (stderr, "done\n");
    return UICB_CONT;
  }

  if (isWindows ()) {
    /* windows is slow */
    mssleep (1500);
  }

  /* send the exit request to main */
  starterQuickConnect (starter, ROUTE_MAIN);
  logMsg (LOG_DBG, LOG_IMPORTANT, "send exit request to main");
  fprintf (stderr, "send exit request to main\n");
  connSendMessage (starter->conn, ROUTE_MAIN, MSG_EXIT_REQUEST, NULL);
  logMsg (LOG_DBG, LOG_IMPORTANT, "sleeping");
  fprintf (stderr, "sleeping\n");
  mssleep (1500);

  count = starterCountProcesses (starter);
  if (count == 0) {
    logProcEnd (LOG_PROC, "starterStopAllProcesses", "after-main");
    fprintf (stderr, "done\n");
    return UICB_CONT;
  }

  /* see which lock files exist, and send exit requests to them */
  count = 0;
  for (route = ROUTE_NONE + 1; route < ROUTE_MAX; ++route) {
    if (route == ROUTE_STARTERUI) {
      continue;
    }

    locknm = lockName (route);
    pid = lockExists (locknm, PATHBLD_MP_USEIDX);
    if (pid > 0) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "route %d %s exists; send exit request",
          route, msgRouteDebugText (route));
      fprintf (stderr, "route %d %s exists; send exit request\n",
          route, msgRouteDebugText (route));
      starterQuickConnect (starter, route);
      connSendMessage (starter->conn, route, MSG_EXIT_REQUEST, NULL);
      ++count;
    }
  }

  if (count <= 0) {
    logProcEnd (LOG_PROC, "starterStopAllProcesses", "after-exit-all");
    fprintf (stderr, "done\n");
    return UICB_CONT;
  }

  logMsg (LOG_DBG, LOG_IMPORTANT, "sleeping");
  fprintf (stderr, "sleeping\n");
  mssleep (1500);

  /* see which lock files still exist and kill the processes */
  count = 0;
  for (route = ROUTE_NONE + 1; route < ROUTE_MAX; ++route) {
    if (route == ROUTE_STARTERUI) {
      continue;
    }

    locknm = lockName (route);
    pid = lockExists (locknm, PATHBLD_MP_USEIDX);
    if (pid > 0) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "lock %d %s exists; send terminate",
          route, msgRouteDebugText (route));
      fprintf (stderr, "lock %d %s exists; send terminate\n",
          route, msgRouteDebugText (route));
      procutilTerminate (pid, false);
      ++count;
    }
  }

  if (count <= 0) {
    logProcEnd (LOG_PROC, "starterStopAllProcesses", "after-term");
    fprintf (stderr, "done\n");
    return UICB_CONT;
  }

  logMsg (LOG_DBG, LOG_IMPORTANT, "sleeping");
  fprintf (stderr, "sleeping\n");
  mssleep (1500);

  /* see which lock files still exist and kill the processes with */
  /* a signal that is not caught; remove the lock file */
  for (route = ROUTE_NONE + 1; route < ROUTE_MAX; ++route) {
    if (route == ROUTE_STARTERUI) {
      continue;
    }

    locknm = lockName (route);
    pid = lockExists (locknm, PATHBLD_MP_USEIDX);
    if (pid > 0) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "lock %d %s exists; force terminate",
          route, msgRouteDebugText (route));
      fprintf (stderr, "lock %d %s exists; force terminate\n",
          route, msgRouteDebugText (route));
      procutilTerminate (pid, true);
    }
    mssleep (100);
    pid = lockExists (locknm, PATHBLD_MP_USEIDX);
    if (pid > 0) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "removing lock %d %s",
          route, msgRouteDebugText (route));
      fprintf (stderr, "removing lock %d %s\n",
          route, msgRouteDebugText (route));
      fileopDelete (locknm);
    }
  }

  pathbldMakePath (tbuff, sizeof (tbuff),
      VOLREG_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
  fileopDelete (tbuff);
  volregClearBDJ4Flag ();

  fprintf (stderr, "done\n");
  logProcEnd (LOG_PROC, "starterStopAllProcesses", "");
  return UICB_CONT;
}

static int
starterCountProcesses (startui_t *starter)
{
  bdjmsgroute_t route;
  char          *locknm;
  pid_t         pid;
  int           count;

  logProcBegin (LOG_PROC, "starterCountProcesses");
  count = 0;
  for (route = ROUTE_NONE + 1; route < ROUTE_MAX; ++route) {
    if (route == ROUTE_STARTERUI) {
      continue;
    }

    locknm = lockName (route);
    pid = lockExists (locknm, PATHBLD_MP_USEIDX);
    if (pid > 0) {
      ++count;
    }
  }

  logProcEnd (LOG_PROC, "starterCountProcesses", "");
  return count;
}


static inline bool
starterForumLinkHandler (void *udata)
{
  starterLinkHandler (udata, START_LINK_CB_FORUM);
  return UICB_STOP;
}

static inline bool
starterWikiLinkHandler (void *udata)
{
  starterLinkHandler (udata, START_LINK_CB_WIKI);
  return UICB_STOP;
}

static inline bool
starterTicketLinkHandler (void *udata)
{
  starterLinkHandler (udata, START_LINK_CB_TICKETS);
  return UICB_STOP;
}

static void
starterLinkHandler (void *udata, int cbidx)
{
  startui_t *starter = udata;
  char        *uri;
  char        tmp [200];

  uri = starter->macoslinkcb [cbidx].uri;
  if (uri != NULL) {
    snprintf (tmp, sizeof (tmp), "open %s", uri);
    system (tmp);
  }
}

static void
starterSetWindowPosition (startui_t *starter)
{
  int   x, y;

  x = nlistGetNum (starter->options, STARTERUI_POSITION_X);
  y = nlistGetNum (starter->options, STARTERUI_POSITION_Y);
  uiWindowMove (&starter->window, x, y, -1);
}

static void
starterLoadOptions (startui_t *starter)
{
  char  tbuff [MAXPATHLEN];

  if (starter->optiondf != NULL) {
    datafileFree (starter->optiondf);
  } else if (starter->options != NULL) {
    nlistFree (starter->options);
  }
  starter->optiondf = NULL;
  starter->options = NULL;

  pathbldMakePath (tbuff, sizeof (tbuff),
      STARTERUI_OPT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  starter->optiondf = datafileAllocParse ("starterui-opt", DFTYPE_KEY_VAL, tbuff,
      starteruidfkeys, STARTERUI_KEY_MAX);
  starter->options = datafileGetList (starter->optiondf);
  if (starter->options == NULL) {
    starter->options = nlistAlloc ("starterui-opt", LIST_ORDERED, free);

    nlistSetNum (starter->options, STARTERUI_POSITION_X, -1);
    nlistSetNum (starter->options, STARTERUI_POSITION_Y, -1);
    nlistSetNum (starter->options, STARTERUI_SIZE_X, 1200);
    nlistSetNum (starter->options, STARTERUI_SIZE_Y, 800);
  }
}

static bool
starterSetUpAlternate (void *udata)
{
  char        prog [MAXPATHLEN];
  const char  *targv [5];
  int         targc = 0;


  logProcBegin (LOG_PROC, "starterSetUpAlternate");

  pathbldMakePath (prog, sizeof (prog),
      "bdj4", sysvarsGetStr (SV_OS_EXEC_EXT), PATHBLD_MP_EXECDIR);
  targv [targc++] = prog;
  targv [targc++] = "--bdj4altsetup";
  targv [targc++] = NULL;
  osProcessStart (targv, OS_PROC_DETACH, NULL, NULL);

  logProcEnd (LOG_PROC, "starterSetUpAlternate", "");
  return UICB_CONT;
}

static void
starterQuickConnect (startui_t *starter, bdjmsgroute_t route)
{
  int   count;

  count = 0;
  while (! connIsConnected (starter->conn, route)) {
    connConnect (starter->conn, route);
    if (count > 5) {
      break;
    }
    mssleep (10);
    ++count;
  }
}

static void
starterSendPlayerActive (startui_t *starter)
{
  char  tmp [40];

  snprintf (tmp, sizeof (tmp), "%d", starter->started [ROUTE_PLAYERUI]);
  connSendMessage (starter->conn, ROUTE_MANAGEUI, MSG_PLAYERUI_ACTIVE, tmp);
}


static bool
starterDeleteProfile (void *udata)
{
  startui_t *starter = udata;
  int       dispidx;
  char      tbuff [MAXPATHLEN];

  if (starter->currprofile == 0 ||
      starter->currprofile == starter->newprofile) {
    /* CONTEXT: starter: status message */
    uiLabelSetText (&starter->statusMsg, _("Profile may not be deleted."));
    return UICB_STOP;
  }

  logEnd ();

  pathbldMakePath (tbuff, sizeof (tbuff), "", "",
      PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  if (fileopIsDirectory (tbuff)) {
    diropDeleteDir (tbuff);
  }
  pathbldMakePath (tbuff, sizeof (tbuff), "", "",
      PATHBLD_MP_DATA | PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
  if (fileopIsDirectory (tbuff)) {
    diropDeleteDir (tbuff);
  }
  pathbldMakePath (tbuff, sizeof (tbuff), "", "",
      PATHBLD_MP_IMGDIR | PATHBLD_MP_USEIDX);
  if (fileopIsDirectory (tbuff)) {
    diropDeleteDir (tbuff);
  }

  dispidx = starterGetProfiles (starter);
  uiSpinboxTextSetValue (starter->profilesel, dispidx);
  uiSpinboxTextSet (starter->profilesel, 0,
      nlistGetCount (starter->proflist), starter->maxProfileWidth,
      starter->proflist, NULL, starterSetProfile);

  /* CONTEXT: starter: status message (restart BDJ4) */
  snprintf (tbuff, sizeof (tbuff), _("Restart %s."), BDJ4_NAME);
  uiLabelSetText (&starter->statusMsg, tbuff);
  return UICB_CONT;
}

