/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include "bdj4.h"
#include "bdj4init.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "conn.h"
#include "fileop.h"
#include "lock.h"
#include "log.h"
#include "mdebug.h"
#include "musicq.h"
#include "ossignal.h"
#include "pathbld.h"
#include "progstate.h"
#include "sock.h"
#include "sockh.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"
#include "websrv.h"

typedef struct {
  conn_t          *conn;
  progstate_t     *progstate;
  int             stopwaitcount;
  char            *locknm;
  uint16_t        port;
  const char      *user;
  const char      *pass;
  websrv_t        *websrv;
  bool            enabled;
} bdjsrv_t;

static bool     bdjsrvHandshakeCallback (void *udata, programstate_t programState);
static bool     bdjsrvStoppingCallback (void *udata, programstate_t programState);
static bool     bdjsrvStopWaitCallback (void *udata, programstate_t programState);
static bool     bdjsrvClosingCallback (void *udata, programstate_t programState);
static void     bdjsrvEventHandler (void *userdata, const char *query, const char *querydata, const char *uri);
static int      bdjsrvProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static int      bdjsrvProcessing (void *udata);
static void     bdjsrvSigHandler (int sig);

static int  gKillReceived = 0;

int
main (int argc, char *argv[])
{
  bdjsrv_t        bdjsrv;
  uint16_t        listenPort;
  uint32_t        flags;
  const char      *srvuri;

#if BDJ4_MEM_DEBUG
  mdebugInit ("bdjsrv");
#endif

  osSetStandardSignals (bdjsrvSigHandler);

  flags = BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  bdj4startup (argc, argv, NULL, "srv", ROUTE_SERVER, &flags);

  srvuri = bdjoptGetStr (OPT_P_BDJ4_SERVER);
  bdjsrv.enabled =
      bdjoptGetNum (OPT_G_BDJ4_SERVER_DISP) &&
      (srvuri == NULL || *srvuri == '\0') &&
      bdjoptGetStr (OPT_P_BDJ4_SERVER_USER) != NULL &&
      bdjoptGetStr (OPT_P_BDJ4_SERVER_PASS) != NULL &&
      bdjoptGetNum (OPT_P_BDJ4_SERVER_PORT) >= 8000;
  if (! bdjsrv.enabled) {
    lockRelease (bdjsrv.locknm, PATHBLD_MP_USEIDX);
    exit (0);
  }

  bdjsrv.user = bdjoptGetStr (OPT_P_BDJ4_SERVER_USER);
  bdjsrv.pass = bdjoptGetStr (OPT_P_BDJ4_SERVER_PASS);
  bdjsrv.port = bdjoptGetNum (OPT_P_BDJ4_SERVER_PORT);
  bdjsrv.progstate = progstateInit ("bdjsrv");
  bdjsrv.websrv = NULL;
  bdjsrv.stopwaitcount = 0;

  progstateSetCallback (bdjsrv.progstate, STATE_WAIT_HANDSHAKE,
      bdjsrvHandshakeCallback, &bdjsrv);
  progstateSetCallback (bdjsrv.progstate, STATE_STOPPING,
      bdjsrvStoppingCallback, &bdjsrv);
  progstateSetCallback (bdjsrv.progstate, STATE_STOP_WAIT,
      bdjsrvStopWaitCallback, &bdjsrv);
  progstateSetCallback (bdjsrv.progstate, STATE_CLOSING,
      bdjsrvClosingCallback, &bdjsrv);

  bdjsrv.conn = connInit (ROUTE_SERVER);

  bdjsrv.websrv = websrvInit (bdjsrv.port, bdjsrvEventHandler, &bdjsrv);

  listenPort = bdjvarsGetNum (BDJVL_PORT_SERVER);
  sockhMainLoop (listenPort, bdjsrvProcessMsg, bdjsrvProcessing, &bdjsrv);
  connFree (bdjsrv.conn);
  progstateFree (bdjsrv.progstate);
  logEnd ();

#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return 0;
}

/* internal routines */

static bool
bdjsrvStoppingCallback (void *udata, programstate_t programState)
{
  bdjsrv_t   *bdjsrv = udata;

  connDisconnectAll (bdjsrv->conn);
  return STATE_FINISHED;
}

static bool
bdjsrvStopWaitCallback (void *tbdjsrv, programstate_t programState)
{
  bdjsrv_t *bdjsrv = tbdjsrv;
  bool        rc;

  rc = connWaitClosed (bdjsrv->conn, &bdjsrv->stopwaitcount);
  return rc;
}

static bool
bdjsrvClosingCallback (void *udata, programstate_t programState)
{
  bdjsrv_t   *bdjsrv = udata;

  bdj4shutdown (ROUTE_SERVER, NULL);

  websrvFree (bdjsrv->websrv);

  return STATE_FINISHED;
}

static void
bdjsrvEventHandler (void *userdata, const char *query, const char *querydata,
    const char *uri)
{
  bdjsrv_t *bdjsrv = userdata;
  char          user [40];
  char          pass [40];
  char          tbuff [300];

  websrvGetUserPass (bdjsrv->websrv, user, sizeof (user), pass, sizeof (pass));

fprintf (stderr, "srv: uri: %s\n", uri);
fprintf (stderr, "srv: query: %s\n", query);
fprintf (stderr, "srv: query-data: %s\n", querydata);
  *tbuff = '\0';
  snprintf (tbuff, sizeof (tbuff), "%d%c%s", MUSICQ_PB_A, MSG_ARGS_RS, querydata);

  if (user [0] == '\0' || pass [0] == '\0') {
    websrvReply (bdjsrv->websrv, 401,
        "Content-type: text/plain; charset=utf-8\r\n"
        "WWW-Authenticate: Basic realm=BDJ4\r\n",
        "Unauthorized");
  } else if (strcmp (user, bdjsrv->user) != 0 ||
      strcmp (pass, bdjsrv->pass) != 0) {
    websrvReply (bdjsrv->websrv, 401,
        "Content-type: text/plain; charset=utf-8\r\n"
        "WWW-Authenticate: Basic realm=BDJ4\r\n",
        "Unauthorized");
  } else if (strcmp (uri, "/exists") == 0) {
    char    rbuff [10];
    char    *rptr;
    char    *rend;

    *rbuff = '\0';
    rptr = rbuff;
    rend = rbuff + sizeof (rbuff);
// ### need to actually check
    rptr = stpecpy (rptr, rend, "T");

    websrvReply (bdjsrv->websrv, 200,
        "Content-type: text/plain; charset=utf-8\r\n"
        "Cache-Control: max-age=0\r\n", rbuff);
    dataFree (rbuff);
  } else if (strcmp (uri, "/get") == 0) {
    char          path [MAXPATHLEN];
    const char    *turi = uri;

    if (*uri == '/') {
      ++uri;
    }
// ### fix
    pathbldMakePath (path, sizeof (path), uri, "", PATHBLD_MP_DREL_HTTP);
    logMsg (LOG_DBG, LOG_IMPORTANT, "serve: %s", turi);
    websrvServeFile (bdjsrv->websrv, sysvarsGetStr (SV_BDJ4_DREL_HTTP), turi);
  } else if (strcmp (uri, "/plnames") == 0) {
    websrvReply (bdjsrv->websrv, 200,
        "Content-type: text/plain; charset=utf-8\r\n"
        "Cache-Control: max-age=0\r\n",
        "");
  } else {
    char          path [MAXPATHLEN];
    const char    *turi = uri;

    if (*uri == '/') {
      ++uri;
    }
    pathbldMakePath (path, sizeof (path), uri, "", PATHBLD_MP_DREL_HTTP);
    if (! fileopFileExists (path)) {
      turi = "/bdj4remote.html";
    }

    logMsg (LOG_DBG, LOG_IMPORTANT, "serve: %s", turi);
    websrvServeFile (bdjsrv->websrv, sysvarsGetStr (SV_BDJ4_DREL_HTTP), turi);
  }
}

static int
bdjsrvProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  bdjsrv_t     *bdjsrv = udata;

  /* this just reduces the amount of stuff in the log */
  if (msg != MSG_PLAYER_STATUS_DATA) {
    logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
        routefrom, msgRouteDebugText (routefrom),
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
  }

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_SERVER: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (bdjsrv->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          progstateShutdownProcess (bdjsrv->progstate);
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

  return 0;
}

static int
bdjsrvProcessing (void *udata)
{
  bdjsrv_t *bdjsrv = udata;
  websrv_t      *websrv = bdjsrv->websrv;
  int           stop = SOCKH_CONTINUE;


  if (! progstateIsRunning (bdjsrv->progstate)) {
    progstateProcess (bdjsrv->progstate);
    if (progstateCurrState (bdjsrv->progstate) == STATE_CLOSED) {
      stop = SOCKH_STOP;
    }
    if (gKillReceived) {
      progstateShutdownProcess (bdjsrv->progstate);
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    }
    return stop;
  }

  connProcessUnconnected (bdjsrv->conn);

  websrvProcess (websrv);

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (bdjsrv->progstate);
  }
  return stop;
}

static bool
bdjsrvHandshakeCallback (void *udata, programstate_t programState)
{
  bdjsrv_t   *bdjsrv = udata;
  bool            rc = STATE_NOT_FINISH;

  connProcessUnconnected (bdjsrv->conn);
  rc = STATE_FINISHED;
  return rc;
}

static void
bdjsrvSigHandler (int sig)
{
  gKillReceived = 1;
}

