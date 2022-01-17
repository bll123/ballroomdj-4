#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>

#include "mongoose.h"

#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjvars.h"
#include "lock.h"
#include "log.h"
#include "process.h"
#include "sockh.h"
#include "sysvars.h"
#include "tmutil.h"
#include "websrv.h"

typedef struct {
  Sock_t          mainSock;
  programstate_t  programState;
  mstime_t        tm;
  uint16_t        port;
  char            *user;
  char            *pass;
  int             haveData;
  char            *danceList;
  char            *playlistList;
  websrv_t        *websrv;
  bool            enabled : 1;
  bool            mainHandshake : 1;
} remctrldata_t;

static void rcEventHandler (struct mg_connection *c, int ev,
    void *ev_data, void *userdata);
static int  remctrlProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata);
static int  remctrlProcessing (void *udata);
static void remctrlSigHandler (int sig);

static int            gKillReceived = 0;

int
main (int argc, char *argv[])
{
  remctrldata_t   remctrlData;
  int             c = 0;
  int             rc = 0;
  int             option_index = 0;
  loglevel_t      loglevel = LOG_IMPORTANT | LOG_MAIN;
  uint16_t        listenPort;

  static struct option bdj_options [] = {
    { "debug",      required_argument,  NULL,   'd' },
    { "profile",    required_argument,  NULL,   'p' },
    { NULL,         0,                  NULL,   0 }
  };

  mstimestart (&remctrlData.tm);
#if _define_SIGHUP
  processCatchSignal (remctrlSigHandler, SIGHUP);
#endif
  processCatchSignal (remctrlSigHandler, SIGINT);
  processDefaultSignal (SIGTERM);

  sysvarsInit (argv[0]);

  while ((c = getopt_long (argc, argv, "", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'd': {
        if (optarg) {
          loglevel = atoi (optarg);
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

  logStartAppend ("remotecontrol", "rc", loglevel);
  logMsg (LOG_SESS, LOG_IMPORTANT, "Using profile %ld", lsysvars [SVL_BDJIDX]);

  rc = lockAcquire (REMCTRL_LOCK_FN, DATAUTIL_MP_USEIDX);
  if (rc < 0) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: remctrl: unable to acquire lock: profile: %zd", lsysvars [SVL_BDJIDX]);
    logMsg (LOG_SESS, LOG_IMPORTANT, "ERR: remctrl: unable to acquire lock: profile: %zd", lsysvars [SVL_BDJIDX]);
    logEnd ();
    exit (0);
  }

  bdjvarsInit ();
  bdjoptInit ();

  remctrlData.enabled = (bdjoptGetNum (OPT_P_REMOTECONTROL) != 0);
  if (! remctrlData.enabled) {
    lockRelease (REMCTRL_LOCK_FN, DATAUTIL_MP_USEIDX);
    exit (0);
  }

  remctrlData.mainSock = INVALID_SOCKET;
  remctrlData.programState = STATE_INITIALIZING;
  remctrlData.port = bdjoptGetNum (OPT_P_REMCONTROLPORT);
  remctrlData.user = strdup (bdjoptGetData (OPT_P_REMCONTROLUSER));
  remctrlData.pass = strdup (bdjoptGetData (OPT_P_REMCONTROLPASS));
  remctrlData.haveData = 0;
  remctrlData.danceList = "";
  remctrlData.playlistList = "";
  remctrlData.websrv = NULL;
  remctrlData.mainHandshake = false;

  remctrlData.websrv = websrvInit (remctrlData.port, rcEventHandler, &remctrlData);

  listenPort = bdjvarsl [BDJVL_REMCONTROL_PORT];
  sockhMainLoop (listenPort, remctrlProcessMsg, remctrlProcessing, &remctrlData);

  websrvFree (remctrlData.websrv);
  if (remctrlData.user != NULL) {
    free (remctrlData.user);
  }
  if (remctrlData.pass != NULL) {
    free (remctrlData.pass);
  }
  if (*remctrlData.danceList) {
    free (remctrlData.danceList);
  }
  if (*remctrlData.playlistList) {
    free (remctrlData.playlistList);
  }
  bdjoptFree ();
  bdjvarsCleanup ();
  logMsg (LOG_SESS, LOG_IMPORTANT, "running: time-to-end: %ld ms", mstimeend (&remctrlData.tm));
  logEnd ();
  lockRelease (REMCTRL_LOCK_FN, DATAUTIL_MP_USEIDX);
  return 0;
}

/* internal routines */

static void
rcEventHandler (struct mg_connection *c, int ev, void *ev_data, void *userdata)
{
  remctrldata_t *remctrlData = userdata;
  char          user [40];
  char          pass [40];

  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    mg_http_creds(hm, user, sizeof(user), pass, sizeof(pass)); // "user" is now user name and "pass" is now password from request

    if (user [0] == '\0' || pass [0] == '\0') {
      mg_http_reply (c, 401, "Content-type: text/plain; charset=utf-8\r\n"
          "WWW-Authenticate: Basic realm=BallroomDJ 4 Remote\r\n", "Unauthorized");
    } else if (strcmp (user, remctrlData->user) != 0 ||
        strcmp (pass, remctrlData->pass) != 0) {
      mg_http_reply (c, 403, "Content-type: text/plain; charset=utf-8\r\n"
          "WWW-Authenticate: Basic realm=BallroomDJ 4 Remote\r\n", "Unauthorized");
    } else if (mg_http_match_uri (hm, "/bdj4update")) {
    } else if (mg_http_match_uri (hm, "/cmd")) {
fprintf (stderr, "uri: %.*s\n", (int) hm->uri.len, hm->uri.ptr);
fprintf (stderr, "  query: %.*s\n", (int) hm->query.len, hm->query.ptr);
    } else if (mg_http_match_uri (hm, "/getdancelist")) {
      mg_http_reply (c, 200, "Content-type: text/plain; charset=utf-8\r\n",
          remctrlData->danceList);
    } else if (mg_http_match_uri (hm, "/getplaylistsel")) {
      mg_http_reply (c, 200, "Content-type: text/plain; charset=utf-8\r\n",
          remctrlData->playlistList);
    } else if (mg_http_match_uri (hm, "#.key") ||
        mg_http_match_uri (hm, "#.crt") ||
        mg_http_match_uri (hm, "#.pem") ||
        mg_http_match_uri (hm, "#.csr")) {
      mg_http_reply (c, 404, NULL, "");
    } else {
      struct mg_http_serve_opts opts = { .root_dir = "http" };
      mg_http_serve_dir (c, hm, &opts);
    }
  }
}

static int
remctrlProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  remctrldata_t     *remctrlData = udata;

  logMsg (LOG_DBG, LOG_MSGS, "got: from: %ld route: %ld msg:%ld args:%s",
      routefrom, route, msg, args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_REMCTRL: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          remctrlData->mainHandshake = true;
          break;
        }
        case MSG_EXIT_REQUEST: {
          mstimestart (&remctrlData->tm);
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          sockhSendMessage (remctrlData->mainSock, ROUTE_REMCTRL, ROUTE_MAIN,
              MSG_SOCKET_CLOSE, NULL);
          sockClose (remctrlData->mainSock);
          remctrlData->mainSock = INVALID_SOCKET;
          return 1;
        }
        case MSG_DANCE_LIST_DATA: {
fprintf (stderr, "dance list:\n%s\n", args);
          remctrlData->haveData++;
          remctrlData->danceList = strdup (args);
          break;
        }
        case MSG_PLAYLIST_LIST_DATA: {
fprintf (stderr, "playlist list:\n%s\n", args);
          remctrlData->haveData++;
          remctrlData->playlistList = strdup (args);
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
remctrlProcessing (void *udata)
{
  remctrldata_t     *remctrlData = udata;
  websrv_t        *websrv = remctrlData->websrv;


  if (remctrlData->programState == STATE_INITIALIZING) {
    remctrlData->programState = STATE_CONNECTING;
  }

  if (remctrlData->programState == STATE_CONNECTING &&
      remctrlData->mainSock == INVALID_SOCKET) {
    int       err;
    uint16_t  mainPort;

    mainPort = bdjvarsl [BDJVL_MAIN_PORT];
    remctrlData->mainSock = sockConnect (mainPort, &err, 1000);
    if (remctrlData->mainSock != INVALID_SOCKET) {
      sockhSendMessage (remctrlData->mainSock, ROUTE_REMCTRL, ROUTE_MAIN,
          MSG_HANDSHAKE, NULL);
      remctrlData->programState = STATE_WAIT_HANDSHAKE;
    }
  }

  if (remctrlData->programState == STATE_WAIT_HANDSHAKE) {
    if (remctrlData->mainHandshake) {
      remctrlData->programState = STATE_RUNNING;
      logMsg (LOG_SESS, LOG_IMPORTANT, "running: time-to-start: %ld ms", mstimeend (&remctrlData->tm));
      sockhSendMessage (remctrlData->mainSock, ROUTE_REMCTRL, ROUTE_MAIN,
          MSG_GET_DANCE_LIST, NULL);
      sockhSendMessage (remctrlData->mainSock, ROUTE_REMCTRL, ROUTE_MAIN,
          MSG_GET_PLAYLIST_LIST, NULL);
    }
  }

/* ### FIX change to < 2 */
  if (remctrlData->programState != STATE_RUNNING ||
      remctrlData->haveData < 1) {
    /* all of the processing that follows requires a running state */
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

static void
remctrlSigHandler (int sig)
{
  gKillReceived = 1;
}
