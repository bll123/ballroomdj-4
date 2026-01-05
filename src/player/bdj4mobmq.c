/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
#include "bdj4intl.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjvars.h"
#include "conn.h"
#include "fileop.h"
#include "lock.h"
#include "log.h"
#include "mdebug.h"
#include "ossignal.h"
#include "pathbld.h"
#include "progstate.h"
#include "sockh.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"
#include "webclient.h"
#include "websrv.h"

typedef struct {
  const char    *webresponse;
  size_t        webresplen;
} mobmqwebresp_t;

typedef struct {
  conn_t          *conn;
  progstate_t     *progstate;
  int             stopwaitcount;
  char            *title;
  websrv_t        *websrv;
  webclient_t     *webclient;
  char            *marqueeData;
  const char      *tag;
  const char      *key;
  mobmqwebresp_t  mobmqwebresp;
  char            tdata [800];
  uint16_t        port;
  int             type;
  bool            finished;
} mobmqdata_t;

static bool     mobmqConnectingCallback (void *tmmdata, programstate_t programState);
static bool     mobmqHandshakeCallback (void *tmmdata, programstate_t programState);
static bool     mobmqStoppingCallback (void *tmmdata, programstate_t programState);
static bool     mobmqStopWaitCallback (void *tmmdata, programstate_t programState);
static bool     mobmqClosingCallback (void *tmmdata, programstate_t programState);
static void     mobmqEventHandler (void *userdata, const char *query, const char *uri);
static int      mobmqProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static int      mobmqProcessing (void *udata);
static void     mobmqSigHandler (int sig);
static void mobmqInternetProcess (mobmqdata_t *mobmqdata);
static const char *mobmqBuildResponse (mobmqdata_t *mobmqdata);
static void mobmqWebResponseCallback (void *userdata, const char *resp, size_t len, time_t tm);

static int  gKillReceived = 0;

int
main (int argc, char *argv[])
{
  mobmqdata_t     mobmqdata;
  uint16_t        listenPort;
  const char      *tval;
  uint32_t        flags;

#if BDJ4_MEM_DEBUG
  mdebugInit ("mm");
#endif

  osSetStandardSignals (mobmqSigHandler);

  flags = BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  bdj4startup (argc, argv, NULL, "mm", ROUTE_MOBILEMQ, &flags);
  mobmqdata.conn = connInit (ROUTE_MOBILEMQ);

  mobmqdata.type = bdjoptGetNum (OPT_P_MOBMQ_TYPE);
  if (mobmqdata.type == MOBMQ_TYPE_OFF) {
    bdj4shutdown (ROUTE_MOBILEMQ, NULL);
    connFree (mobmqdata.conn);
    exit (0);
  }

  mobmqdata.progstate = progstateInit ("mobilemq");

  progstateSetCallback (mobmqdata.progstate, PROGSTATE_CONNECTING,
      mobmqConnectingCallback, &mobmqdata);
  progstateSetCallback (mobmqdata.progstate, PROGSTATE_WAIT_HANDSHAKE,
      mobmqHandshakeCallback, &mobmqdata);
  progstateSetCallback (mobmqdata.progstate, PROGSTATE_STOPPING,
      mobmqStoppingCallback, &mobmqdata);
  progstateSetCallback (mobmqdata.progstate, PROGSTATE_STOP_WAIT,
      mobmqStopWaitCallback, &mobmqdata);
  progstateSetCallback (mobmqdata.progstate, PROGSTATE_CLOSING,
      mobmqClosingCallback, &mobmqdata);

  tval = bdjoptGetStr (OPT_P_MOBMQ_TITLE);
  mobmqdata.title = NULL;
  if (tval != NULL) {
    mobmqdata.title = mdstrdup (tval);
  }
  mobmqdata.tag = bdjoptGetStr (OPT_P_MOBMQ_TAG);
  mobmqdata.key = bdjoptGetStr (OPT_P_MOBMQ_KEY);
  mobmqdata.port = bdjoptGetNum (OPT_P_MOBMQ_PORT);

  mobmqdata.websrv = NULL;
  mobmqdata.webclient = NULL;
  mobmqdata.marqueeData = NULL;
  mobmqdata.stopwaitcount = 0;
  mobmqdata.finished = false;

  if (mobmqdata.type == MOBMQ_TYPE_LOCAL) {
    mobmqdata.websrv = websrvInit (mobmqdata.port, mobmqEventHandler,
        &mobmqdata, WEBSRV_TLS_OFF);
  }
  if (mobmqdata.type == MOBMQ_TYPE_INTERNET) {
    mobmqdata.webclient = webclientAlloc (&mobmqdata.mobmqwebresp, mobmqWebResponseCallback);
    webclientSetTimeout (mobmqdata.webclient, 1000);
  }

  listenPort = bdjvarsGetNum (BDJVL_PORT_MOBILEMQ);
  sockhMainLoop (listenPort, mobmqProcessMsg, mobmqProcessing, &mobmqdata);
  connFree (mobmqdata.conn);
  progstateFree (mobmqdata.progstate);
  logEnd ();
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return 0;
}

/* internal routines */

static bool
mobmqStoppingCallback (void *tmmdata, programstate_t programState)
{
  mobmqdata_t   *mobmqdata = tmmdata;

  connDisconnectAll (mobmqdata->conn);
  return STATE_FINISHED;
}

static bool
mobmqStopWaitCallback (void *tmobmq, programstate_t programState)
{
  mobmqdata_t *mobmq = tmobmq;
  bool        rc;

  rc = connWaitClosed (mobmq->conn, &mobmq->stopwaitcount);
  return rc;
}

static bool
mobmqClosingCallback (void *tmmdata, programstate_t programState)
{
  mobmqdata_t   *mobmqdata = tmmdata;

  bdj4shutdown (ROUTE_MOBILEMQ, NULL);

  websrvFree (mobmqdata->websrv);
  webclientClose (mobmqdata->webclient);
  dataFree (mobmqdata->title);
  dataFree (mobmqdata->marqueeData);

  return STATE_FINISHED;
}

static void
mobmqEventHandler (void *userdata, const char *query, const char *uri)
{
  mobmqdata_t   *mobmqdata = userdata;

  if (strcmp (uri, "/mmupdate") == 0) {
    const char *data = NULL;

    data = mobmqBuildResponse (mobmqdata);
    websrvReply (mobmqdata->websrv, WEB_OK,
        "Content-Type: application/json\r\n", data);
  } else {
    char          path [BDJ4_PATH_MAX];
    const char    *turi = uri;

    if (*uri == '/') {
      ++uri;
    }
    pathbldMakePath (path, sizeof (path), uri, "", PATHBLD_MP_DREL_HTTP);
    if (! fileopFileExists (path)) {
      turi = "/mobilemq.html";
    }

    logMsg (LOG_DBG, LOG_IMPORTANT, "serve: %s", turi);
    websrvServeFile (mobmqdata->websrv, sysvarsGetStr (SV_BDJ4_DREL_HTTP), turi);
  }
}

static int
mobmqProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  mobmqdata_t     *mobmqdata = udata;

  logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
      routefrom, msgRouteDebugText (routefrom),
      route, msgRouteDebugText (route), msg, msgDebugText (msg), args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MOBILEMQ: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (mobmqdata->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          progstateShutdownProcess (mobmqdata->progstate);
          break;
        }
        case MSG_MARQUEE_DATA: {
          mobmqdata->finished = false;
          dataFree (mobmqdata->marqueeData);
          mobmqdata->marqueeData = mdstrdup (args);
          if (mobmqdata->type == MOBMQ_TYPE_INTERNET) {
            mobmqInternetProcess (mobmqdata);
          }
          break;
        }
        case MSG_FINISHED: {
          mobmqdata->finished = true;
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
mobmqProcessing (void *udata)
{
  mobmqdata_t   *mobmqdata = udata;
  websrv_t      *websrv = mobmqdata->websrv;
  int           stop = SOCKH_CONTINUE;


  if (! progstateIsRunning (mobmqdata->progstate)) {
    progstateProcess (mobmqdata->progstate);
    if (progstateCurrState (mobmqdata->progstate) == PROGSTATE_CLOSED) {
      stop = SOCKH_STOP;
    }
    if (gKillReceived) {
      progstateShutdownProcess (mobmqdata->progstate);
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    }
    return stop;
  }

  connProcessUnconnected (mobmqdata->conn);

  websrvProcess (websrv);

  if (gKillReceived) {
    progstateShutdownProcess (mobmqdata->progstate);
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  return stop;
}

static bool
mobmqConnectingCallback (void *tmmdata, programstate_t programState)
{
  mobmqdata_t   *mobmqdata = tmmdata;
  bool          rc = STATE_NOT_FINISH;

  connProcessUnconnected (mobmqdata->conn);

  if (! connIsConnected (mobmqdata->conn, ROUTE_MAIN)) {
    connConnect (mobmqdata->conn, ROUTE_MAIN);
  }

  if (connIsConnected (mobmqdata->conn, ROUTE_MAIN)) {
    rc = STATE_FINISHED;
  }

  return rc;
}

static bool
mobmqHandshakeCallback (void *tmmdata, programstate_t programState)
{
  mobmqdata_t   *mobmqdata = tmmdata;
  bool          rc = STATE_NOT_FINISH;

  connProcessUnconnected (mobmqdata->conn);

  if (connHaveHandshake (mobmqdata->conn, ROUTE_MAIN)) {
    rc = STATE_FINISHED;
  }

  return rc;
}

static void
mobmqSigHandler (int sig)
{
  gKillReceived = 1;
}

static void
mobmqInternetProcess (mobmqdata_t *mobmqdata)
{
  const char    *data;
  char          uri [200];
  char          tbuff [4096];

  data = mobmqBuildResponse (mobmqdata);
  snprintf (uri, sizeof (uri), "%s%s",
      bdjoptGetStr (OPT_HOST_MOBMQ), bdjoptGetStr (OPT_URI_MOBMQ_PHP));
  snprintf (tbuff, sizeof (tbuff),
      "key=%s"
      "&mobmqtag=%s"
      "&mobmqkey=%s"
      "&mqdata=%s",
      "73459734",     // key
      mobmqdata->tag,
      mobmqdata->key,
      data);
  webclientPost (mobmqdata->webclient, uri, tbuff);
}

static const char *
mobmqBuildResponse (mobmqdata_t *mobmqdata)
{
  const char  *data = NULL;
  const char  *disp = NULL;
  const char  *title = NULL;

  data = mobmqdata->marqueeData;
  title = mobmqdata->title;

  if (data == NULL || mobmqdata->finished) {
    /* I don't think this ever get reached...bdj4main handles the message */
    /* and the html changes current == "" to 'Not Playing'. */
    if (title == NULL) {
      title = "";
    }
    /* CONTEXT: mobile marquee: playback is not active */
    disp = _("Not Playing");
    if (mobmqdata->finished) {
      disp = bdjoptGetStr (OPT_P_COMPLETE_MSG);
    }
    snprintf (mobmqdata->tdata, sizeof (mobmqdata->tdata),
        "{ "
        "\"title\" : \"%s\","
        "\"current\" : \"%s\","
        "\"skip\" : \"true\""
        "}",
        title, disp);
    data = mobmqdata->tdata;
  }

  return data;
}

static void
mobmqWebResponseCallback (void *userdata, const char *resp, size_t len, time_t tm)
{
  mobmqwebresp_t *mobmqwebresp = userdata;

  mobmqwebresp->webresponse = resp;
  mobmqwebresp->webresplen = len;
  return;
}

