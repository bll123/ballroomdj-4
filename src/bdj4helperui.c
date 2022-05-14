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
  char            *locknm;
  conn_t          *conn;
  UIWidget        window;
  /* gtk stuff */
} helperui_t;

static bool     helperStoppingCallback (void *udata, programstate_t programState);
static bool     helperClosingCallback (void *udata, programstate_t programState);
static void     helperBuildUI (helperui_t *helper);
gboolean        helperMainLoop  (void *thelper);
static int      helperProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static gboolean helperCloseWin (GtkWidget *window, GdkEvent *event, gpointer userdata);
static void     helperSigHandler (int sig);

static int gKillReceived = 0;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  helperui_t      helper;
  char            *uifont;
  int             flags;


  helper.progstate = progstateInit ("helperui");
  progstateSetCallback (helper.progstate, STATE_STOPPING,
      helperStoppingCallback, &helper);
  progstateSetCallback (helper.progstate, STATE_CLOSING,
      helperClosingCallback, &helper);
  uiutilsUIWidgetInit (&helper.window);

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
  uiCreateMainWindow (&helper->window, BDJ4_LONG_NAME, imgbuff,
      helperCloseWin, helper);

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, uiBaseMarginSz * 2);
  uiBoxPackInWindow (&helper->window, &vbox);


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


static gboolean
helperCloseWin (GtkWidget *window, GdkEvent *event, gpointer userdata)
{
  helperui_t   *helper = userdata;

  if (progstateCurrState (helper->progstate) <= STATE_RUNNING) {
    progstateShutdownProcess (helper->progstate);
    logMsg (LOG_DBG, LOG_MSGS, "got: close win request");
    return TRUE;
  }

  return FALSE;
}

static void
helperSigHandler (int sig)
{
  gKillReceived = 1;
}

