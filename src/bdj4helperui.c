#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>

#include "bdj4.h"
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjvars.h"
#include "conn.h"
#include "log.h"
#include "osutils.h"
#include "osuiutils.h"
#include "pathbld.h"
#include "progstate.h"
#include "sockh.h"
#include "ui.h"
#include "uiutils.h"

typedef struct {
  progstate_t     *progstate;
  conn_t          *conn;
  UIWidget        window;
  uitextbox_t     *tb;
  UICallback      closeCallback;
  UICallback      nextCallback;
} helperui_t;

static void     helperInitText (void);
static bool     helperStoppingCallback (void *udata, programstate_t programState);
static bool     helperClosingCallback (void *udata, programstate_t programState);
static void     helperBuildUI (helperui_t *helper);
gboolean        helperMainLoop  (void *thelper);
static int      helperProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static void     helperSigHandler (int sig);
static bool     helperCloseCallback (void *udata);
static bool     helperNextCallback (void *udata);

static int gKillReceived = 0;

typedef enum {
  HELP_STATE_INTRO,
  HELP_STATE_MUSIC_LOC,
  HELP_STATE_MUSIC_ORG,
  HELP_STATE_BUILD_DB,
  HELP_STATE_MAKE_MANUAL_PL,
  HELP_STATE_PLAY_MANUAL_PL,
  HELP_STATE_WHERE_SUPPORT,
  HELP_STATE_MAX,
} hstate_t;

char *helptitle [HELP_STATE_MAX] = {
  [HELP_STATE_INTRO] = "",
  [HELP_STATE_MUSIC_LOC] = "",
  [HELP_STATE_MUSIC_ORG] = "",
  [HELP_STATE_BUILD_DB] = "",
  [HELP_STATE_MAKE_MANUAL_PL] = "",
  [HELP_STATE_PLAY_MANUAL_PL] = "",
  [HELP_STATE_WHERE_SUPPORT] = "",
};

char *helptext [HELP_STATE_MAX] = {
  [HELP_STATE_INTRO] = "",
  [HELP_STATE_MUSIC_LOC] = "",
  [HELP_STATE_MUSIC_ORG] = "",
  [HELP_STATE_BUILD_DB] = "",
  [HELP_STATE_MAKE_MANUAL_PL] = "",
  [HELP_STATE_PLAY_MANUAL_PL] = "",
  [HELP_STATE_WHERE_SUPPORT] = "",
};

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  helperui_t      helper;
  char            *uifont;
  int             flags;


  helperInitText ();

  helper.tb = NULL;
  helper.conn = NULL;
  uiutilsUIWidgetInit (&helper.window);

  helper.progstate = progstateInit ("helperui");
  progstateSetCallback (helper.progstate, STATE_STOPPING,
      helperStoppingCallback, &helper);
  progstateSetCallback (helper.progstate, STATE_CLOSING,
      helperClosingCallback, &helper);

  osSetStandardSignals (helperSigHandler);

  flags = BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  bdj4startup (argc, argv, NULL, "st", ROUTE_HELPERUI, flags);
  logProcBegin (LOG_PROC, "helperui");

  listenPort = bdjvarsGetNum (BDJVL_HELPERUI_PORT);
  helper.conn = connInit (ROUTE_HELPERUI);

  uiUIInitialize ();
  uifont = bdjoptGetStr (OPT_MP_UIFONT);
  uiSetUIFont (uifont);

  helperBuildUI (&helper);
  sockhMainLoop (listenPort, helperProcessMsg, helperMainLoop, &helper);
  connFree (helper.conn);
  progstateFree (helper.progstate);
  logProcEnd (LOG_PROC, "helperui", "");
  logEnd ();
  return status;
}

/* internal routines */

static bool
helperStoppingCallback (void *udata, programstate_t programState)
{
  helperui_t   *helper = udata;

  connDisconnectAll (helper->conn);
  return true;
}

static bool
helperClosingCallback (void *udata, programstate_t programState)
{
  helperui_t   *helper = udata;

  logProcBegin (LOG_PROC, "helperClosingCallback");
  uiCloseWindow (&helper->window);

  bdj4shutdown (ROUTE_HELPERUI, NULL);

  if (helper->tb != NULL) {
    uiTextBoxFree (helper->tb);
  }

  logProcEnd (LOG_PROC, "helperClosingCallback", "");
  return true;
}

static void
helperBuildUI (helperui_t  *helper)
{
  UIWidget            uiwidget;
  UIWidget            vbox;
  UIWidget            hbox;
  char                imgbuff [MAXPATHLEN];

  uiutilsUIWidgetInit (&uiwidget);
  uiutilsUIWidgetInit (&vbox);
  uiutilsUIWidgetInit (&hbox);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", ".svg", PATHBLD_MP_IMGDIR);
  uiutilsUICallbackInit (&helper->closeCallback, helperCloseCallback, helper);
  uiCreateMainWindow (&helper->window, &helper->closeCallback,
      BDJ4_LONG_NAME, imgbuff);

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, uiBaseMarginSz * 2);
  uiBoxPackInWindow (&helper->window, &vbox);

  helper->tb = uiTextBoxCreate (300);
  uiTextBoxVertExpand (helper->tb);
  uiTextBoxSetReadonly (helper->tb);
  uiBoxPackStart (&vbox, uiTextBoxGetScrolledWindow (helper->tb));

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiutilsUICallbackInit (&helper->nextCallback, helperNextCallback, helper);
  uiCreateButton (&uiwidget, &helper->nextCallback,
      /* CONTEXT: helperui: proceed to the next step */
      _("Next"), NULL, NULL, NULL);
  uiBoxPackEnd (&hbox, &uiwidget);

  uiCreateButton (&uiwidget, &helper->closeCallback,
      /* CONTEXT: helperui: exit the helper */
      _("Exit"), NULL, NULL, NULL);
  uiBoxPackEnd (&hbox, &uiwidget);

  uiWindowSetDefaultSize (&helper->window, 800, 300);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", ".png", PATHBLD_MP_IMGDIR);
  osuiSetIcon (imgbuff);

  uiWidgetShowAll (&helper->window);
}

int
helperMainLoop (void *thelper)
{
  helperui_t   *helper = thelper;
  int         stop = FALSE;
  /* support message handling */


  if (! stop) {
    uiUIProcessEvents ();
  }

  if (! progstateIsRunning (helper->progstate)) {
    progstateProcess (helper->progstate);
    if (progstateCurrState (helper->progstate) == STATE_CLOSED) {
      stop = true;
    }
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      progstateShutdownProcess (helper->progstate);
      gKillReceived = 0;
    }
    return stop;
  }

  connProcessUnconnected (helper->conn);

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (helper->progstate);
    gKillReceived = 0;
  }
  return stop;
}

static int
helperProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  helperui_t       *helper = udata;

  logProcBegin (LOG_PROC, "helperProcessMsg");

  logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
      routefrom, msgRouteDebugText (routefrom),
      route, msgRouteDebugText (route), msg, msgDebugText (msg), args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_HELPERUI: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (helper->conn, routefrom);
          break;
        }
        case MSG_SOCKET_CLOSE: {
          connDisconnect (helper->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          progstateShutdownProcess (helper->progstate);
          gKillReceived = 0;
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

  logProcEnd (LOG_PROC, "helperProcessMsg", "");

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  return gKillReceived;
}


static void
helperSigHandler (int sig)
{
  gKillReceived = 1;
}


static bool
helperCloseCallback (void *udata)
{
  helperui_t   *helper = udata;

  if (progstateCurrState (helper->progstate) <= STATE_RUNNING) {
    progstateShutdownProcess (helper->progstate);
    logMsg (LOG_DBG, LOG_MSGS, "got: close win request");
  }

  return UICB_STOP;
}

static bool
helperNextCallback (void *udata)
{
  return UICB_CONT;
}

static void
helperInitText (void)
{
  helptitle [HELP_STATE_INTRO] =
      /* CONTEXT: getting started helper: introduction title */
      _("Introduction");
  helptitle [HELP_STATE_MUSIC_LOC] =
      /* CONTEXT: getting started helper: music location title */
      _("Set the Location of the Music");
  helptitle [HELP_STATE_MUSIC_ORG] =
      /* CONTEXT: getting started helper: music organization title */
      _("How is the Music Organised");
  helptitle [HELP_STATE_BUILD_DB] =
      /* CONTEXT: getting started helper: build database title */
      _("Build the Database for the First Time");
  helptitle [HELP_STATE_MAKE_MANUAL_PL] =
      /* CONTEXT: getting started helper: create a manual playlist title */
      _("Create a Manual Playlist");
  helptitle [HELP_STATE_PLAY_MANUAL_PL] =
      /* CONTEXT:getting started helper: play a manual playlist title */
      _("Play a Manual Playlist");
  helptitle [HELP_STATE_WHERE_SUPPORT] =
      /* CONTEXT: getting started helper: getting support title */
      _("How to get Support");


  helptext [HELP_STATE_INTRO] =
      /* CONTEXT: getting started helper: introduction help text */
      _("intro");
  helptext [HELP_STATE_MUSIC_LOC] =
      /* CONTEXT: getting started helper: music location help text */
      _("music loc");
  helptext [HELP_STATE_MUSIC_ORG] =
      /* CONTEXT: getting started helper: music organization help text */
      _("music org");
  helptext [HELP_STATE_BUILD_DB] =
      /* CONTEXT: getting started helper: build database help text */
      _("build db");
  helptext [HELP_STATE_MAKE_MANUAL_PL] =
      /* CONTEXT: getting started helper: create a manual playlist help text */
      _("create manual pl");
  helptext [HELP_STATE_PLAY_MANUAL_PL] =
      /* CONTEXT: getting started helper: play a manual playlist help text */
      _("play manual pl");
  helptext [HELP_STATE_WHERE_SUPPORT] =
      /* CONTEXT: getting started helper: getting support help text */
      _("where is support");
}
