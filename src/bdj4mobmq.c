#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <signal.h>
#include <sys/time.h> // for mongoose
#include <dirent.h> // for mongoose

/* winsock2.h should come before windows.h */
#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>       // for mongoose
#endif

#include "mongoose.h"

#include "bdj4.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjvars.h"
#include "conn.h"
#include "lock.h"
#include "log.h"
#include "pathbld.h"
#include "process.h"
#include "progstart.h"
#include "sockh.h"
#include "sysvars.h"
#include "tmutil.h"
#include "websrv.h"

typedef struct {
  conn_t          *conn;
  progstart_t     *progstart;
  char            *locknm;
  uint16_t        port;
  bdjmobilemq_t   type;
  char            *name;
  char            *title;
  websrv_t        *websrv;
  char            *marqueeData;
} mobmqdata_t;

static bool     mobmqConnectingCallback (void *tmmdata, programstate_t programState);
static bool     mobmqHandshakeCallback (void *tmmdata, programstate_t programState);
static bool     mobmqStoppingCallback (void *tmmdata, programstate_t programState);
static bool     mobmqClosingCallback (void *tmmdata, programstate_t programState);
static void     mobmqEventHandler (struct mg_connection *c, int ev,
                    void *ev_data, void *userdata);
static int      mobmqProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static int      mobmqProcessing (void *udata);
static void     mobmqSigHandler (int sig);

static int            gKillReceived = 0;

int
main (int argc, char *argv[])
{
  mobmqdata_t     mobmqData;
  int             c = 0;
  int             rc = 0;
  int             option_index = 0;
  loglevel_t      loglevel = LOG_IMPORTANT | LOG_MAIN;
  uint16_t        listenPort;
  char            *tval;

  static struct option bdj_options [] = {
    { "mobilemq",   no_argument,        NULL,   0 },
    { "debug",      required_argument,  NULL,   'd' },
    { "profile",    required_argument,  NULL,   'p' },
    { NULL,         0,                  NULL,   0 }
  };

#if _define_SIGHUP
  processCatchSignal (mobmqSigHandler, SIGHUP);
#endif
  processCatchSignal (mobmqSigHandler, SIGINT);
  processDefaultSignal (SIGTERM);

  sysvarsInit (argv[0]);

  while ((c = getopt_long_only (argc, argv, "p:d:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'd': {
        if (optarg) {
          loglevel = (loglevel_t) atoi (optarg);
        }
        break;
      }
      case 'p': {
        if (optarg) {
          sysvarSetNum (SVL_BDJIDX, atol (optarg));
        }
        break;
      }
      default: {
        break;
      }
    }
  }

  logStartAppend ("mobilemarquee", "mm", loglevel);
  logMsg (LOG_SESS, LOG_IMPORTANT, "Using profile %ld", lsysvars [SVL_BDJIDX]);

  mobmqData.locknm = lockName (ROUTE_MOBILEMQ);
  rc = lockAcquire (mobmqData.locknm, PATHBLD_MP_USEIDX);
  if (rc < 0) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: mobilemq: unable to acquire lock: profile: %zd", lsysvars [SVL_BDJIDX]);
    logMsg (LOG_SESS, LOG_IMPORTANT, "ERR: mobilemq: unable to acquire lock: profile: %zd", lsysvars [SVL_BDJIDX]);
    logEnd ();
    exit (0);
  }

  bdjvarsInit ();
  bdjoptInit ();
  mobmqData.conn = connInit (ROUTE_MARQUEE);

  mobmqData.type = (bdjmobilemq_t) bdjoptGetNum (OPT_P_MOBILEMARQUEE);
  if (mobmqData.type == MOBILEMQ_OFF) {
    lockRelease (mobmqData.locknm, PATHBLD_MP_USEIDX);
    exit (0);
  }

  mobmqData.progstart = progstartInit ("mobilemq");
  progstartSetCallback (mobmqData.progstart, STATE_CONNECTING,
      mobmqConnectingCallback, &mobmqData);
  progstartSetCallback (mobmqData.progstart, STATE_WAIT_HANDSHAKE,
      mobmqHandshakeCallback, &mobmqData);
  progstartSetCallback (mobmqData.progstart, STATE_STOPPING,
      mobmqStoppingCallback, &mobmqData);
  progstartSetCallback (mobmqData.progstart, STATE_CLOSING,
      mobmqClosingCallback, &mobmqData);
  mobmqData.port = bdjoptGetNum (OPT_P_MOBILEMQPORT);
  mobmqData.name = NULL;
  tval = bdjoptGetData (OPT_P_MOBILEMQTAG);
  if (tval != NULL) {
    mobmqData.name = strdup (tval);
  }
  tval = bdjoptGetData (OPT_P_MOBILEMQTITLE);
  mobmqData.title = NULL;
  if (tval != NULL) {
    mobmqData.title = strdup (tval);
  }
  mobmqData.websrv = NULL;
  mobmqData.marqueeData = NULL;

  mobmqData.websrv = websrvInit (mobmqData.port, mobmqEventHandler, &mobmqData);

  listenPort = bdjvarsl [BDJVL_MOBILEMQ_PORT];
  sockhMainLoop (listenPort, mobmqProcessMsg, mobmqProcessing, &mobmqData);

  while (progstartShutdownProcess (mobmqData.progstart) != STATE_CLOSED) {
    ;
  }
  progstartFree (mobmqData.progstart);
  logEnd ();
  return 0;
}

/* internal routines */

static bool
mobmqStoppingCallback (void *tmmdata, programstate_t programState)
{
  mobmqdata_t   *mobmqData = tmmdata;

  connDisconnectAll (mobmqData->conn);
  return true;
}

static bool
mobmqClosingCallback (void *tmmdata, programstate_t programState)
{
  mobmqdata_t   *mobmqData = tmmdata;

  connFree (mobmqData->conn);

  websrvFree (mobmqData->websrv);
  if (mobmqData->name != NULL) {
    free (mobmqData->name);
  }
  if (mobmqData->title != NULL) {
    free (mobmqData->title);
  }
  if (mobmqData->marqueeData != NULL) {
    free (mobmqData->marqueeData);
  }
  bdjoptFree ();
  bdjvarsCleanup ();
  lockRelease (mobmqData->locknm, PATHBLD_MP_USEIDX);

  return true;
}

static void
mobmqEventHandler (struct mg_connection *c, int ev, void *ev_data, void *userdata)
{
  mobmqdata_t   *mobmqData = userdata;
  char          *data = NULL;
  char          *title = NULL;
  char          tbuff [400];

  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;

    if (mg_http_match_uri (hm, "/mmupdate")) {
      data = mobmqData->marqueeData;
      title = mobmqData->title;
      if (data == NULL) {
        if (title == NULL) {
          title = "";
        }
        snprintf (tbuff, sizeof (tbuff),
            "{ "
            "\"title\" : \"%s\","
            "\"current\" : \"Not Playing\","
            "\"skip\" : \"true\""
            "}",
            title);
        data = tbuff;
      }
      mg_http_reply (c, 200, "Content-Type: application/json\r\n",
          "%s\r\n", data);
    } else if (mg_http_match_uri (hm, "#.key") ||
        mg_http_match_uri (hm, "#.crt") ||
        mg_http_match_uri (hm, "#.pem") ||
        mg_http_match_uri (hm, "#.csr")) {
      mg_http_reply (c, 404, NULL, "");
    } else {
      struct mg_http_serve_opts opts = { .root_dir = sysvars [SV_BDJ4HTTPDIR] };
      mg_http_serve_dir (c, hm, &opts);
    }
  }
}

static int
mobmqProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  mobmqdata_t     *mobmqData = udata;

  logMsg (LOG_DBG, LOG_MSGS, "got: from: %ld route: %ld msg:%ld args:%s",
      routefrom, route, msg, args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MOBILEMQ: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (mobmqData->conn, routefrom);
          connConnectResponse (mobmqData->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          progstartShutdownProcess (mobmqData->progstart);
          return 1;
        }
        case MSG_MARQUEE_DATA: {
          if (mobmqData->marqueeData != NULL) {
            free (mobmqData->marqueeData);
          }
          mobmqData->marqueeData = strdup (args);
          assert (mobmqData->marqueeData != NULL);
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
  mobmqdata_t     *mobmqData = udata;
  websrv_t        *websrv = mobmqData->websrv;


  if (! progstartIsRunning (mobmqData->progstart)) {
    progstartProcess (mobmqData->progstart);
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    }
    return gKillReceived;
  }

  websrvProcess (websrv);

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  return gKillReceived;
}

static bool
mobmqConnectingCallback (void *tmmdata, programstate_t programState)
{
  mobmqdata_t   *mobmqData = tmmdata;
  bool          rc = false;

  if (! connIsConnected (mobmqData->conn, ROUTE_MAIN)) {
    connConnect (mobmqData->conn, ROUTE_MAIN);
  }

  if (connIsConnected (mobmqData->conn, ROUTE_MAIN)) {
    rc = true;
  }

  return rc;
}

static bool
mobmqHandshakeCallback (void *tmmdata, programstate_t programState)
{
  mobmqdata_t   *mobmqData = tmmdata;
  bool          rc = false;

  if (connHaveHandshake (mobmqData->conn, ROUTE_MAIN)) {
    rc = true;
  }

  return rc;
}

static void
mobmqSigHandler (int sig)
{
  gKillReceived = 1;
}
