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

#include "bdj4.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjvars.h"
#include "lock.h"
#include "log.h"
#include "process.h"
#include "sock.h"
#include "sockh.h"
#include "sysvars.h"
#include "tmutil.h"
#include "websrv.h"

#define SOCKOF(idx) (remctrlData->processes [idx].sock)

typedef struct {
  processconn_t   processes [PROCESS_MAX];
  programstate_t  programState;
  mstime_t        tm;
  mstime_t        statusCheck;
  uint16_t        port;
  char            *user;
  char            *pass;
  int             haveData;
  char            *danceList;
  char            *playlistList;
  char            *playerStatus;
  websrv_t        *websrv;
  bool            enabled : 1;
} remctrldata_t;

static void rcEventHandler (struct mg_connection *c, int ev,
    void *ev_data, void *userdata);
static int  remctrlProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata);
static int  remctrlProcessing (void *udata);
static void remctrlSigHandler (int sig);
static void remctrlConnectProcess (remctrldata_t *remctrlData,
    processconnidx_t idx, bdjmsgroute_t route, uint16_t port);
static void remctrlDisconnectProcess (remctrldata_t *remctrlData,
    processconnidx_t idx, bdjmsgroute_t route);

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

  remctrlData.danceList = "";
  remctrlData.haveData = 0;
  remctrlData.pass = strdup (bdjoptGetData (OPT_P_REMCONTROLPASS));
  remctrlData.playerStatus = NULL;
  mstimestart (&remctrlData.statusCheck);
  remctrlData.playlistList = "";
  remctrlData.port = bdjoptGetNum (OPT_P_REMCONTROLPORT);
  remctrlData.programState = STATE_INITIALIZING;
  remctrlData.user = strdup (bdjoptGetData (OPT_P_REMCONTROLUSER));
  remctrlData.websrv = NULL;
  for (processconnidx_t i = PROCESS_PLAYER; i < PROCESS_MAX; ++i) {
    remctrlData.processes [i].sock = INVALID_SOCKET;
    remctrlData.processes [i].started = false;
    remctrlData.processes [i].handshake = false;
  }

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
  if (remctrlData.playerStatus != NULL) {
    free (remctrlData.playerStatus);
  }
  if (*remctrlData.danceList) {
    free (remctrlData.danceList);
  }
  if (*remctrlData.playlistList) {
    free (remctrlData.playlistList);
  }
  bdjoptFree ();
  bdjvarsCleanup ();
  logMsg (LOG_SESS, LOG_IMPORTANT, "time-to-end: %ld ms", mstimeend (&remctrlData.tm));
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
  char          querystr [40];
  char          *tokptr;
  char          *qstrptr;

  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;

    mg_http_creds(hm, user, sizeof(user), pass, sizeof(pass)); // "user" is now user name and "pass" is now password from request

    mg_url_decode (hm->query.ptr, hm->query.len, querystr, sizeof (querystr), 1);
    qstrptr = strtok_r (querystr, " ", &tokptr);
    qstrptr = strtok_r (NULL, " ", &tokptr);

    if (*querystr) {
      logMsg (LOG_DBG, LOG_BASIC, "process: %s", querystr);
      if (qstrptr != NULL) {
        logMsg (LOG_DBG, LOG_BASIC, "  args: %s", qstrptr);
      }
    }
    if (user [0] == '\0' || pass [0] == '\0') {
      mg_http_reply (c, 401, "Content-type: text/plain; charset=utf-8\r\n"
          "WWW-Authenticate: Basic realm=BallroomDJ 4 Remote\r\n", "Unauthorized");
    } else if (strcmp (user, remctrlData->user) != 0 ||
        strcmp (pass, remctrlData->pass) != 0) {
      mg_http_reply (c, 401, "Content-type: text/plain; charset=utf-8\r\n"
          "WWW-Authenticate: Basic realm=BallroomDJ 4 Remote\r\n", "Unauthorized");
    } else if (mg_http_match_uri (hm, "/getstatus")) {
      if (remctrlData->playerStatus == NULL) {
        mg_http_reply (c, 204, "Content-type: text/plain; charset=utf-8\r\n"
            "Cache-Control max-age=0\r\n", "");
      } else {
        mg_http_reply (c, 200, "Content-type: text/plain; charset=utf-8\r\n"
            "Cache-Control max-age=0\r\n", remctrlData->playerStatus);
      }
    } else if (mg_http_match_uri (hm, "/cmd")) {
      bool ok = true;

      if (strcmp (querystr, "clear") == 0) {
        sockhSendMessage (SOCKOF (PROCESS_MAIN),
            ROUTE_REMCTRL, ROUTE_MAIN, MSG_QUEUE_CLEAR, NULL);
      } else if (strcmp (querystr, "fade") == 0) {
        sockhSendMessage (SOCKOF (PROCESS_PLAYER),
            ROUTE_REMCTRL, ROUTE_PLAYER, MSG_PLAY_FADE, NULL);
      } else if (strcmp (querystr, "nextsong") == 0) {
        sockhSendMessage (SOCKOF (PROCESS_PLAYER),
            ROUTE_REMCTRL, ROUTE_PLAYER, MSG_PLAY_NEXTSONG, NULL);
      } else if (strcmp (querystr, "pauseatend") == 0) {
        sockhSendMessage (SOCKOF (PROCESS_PLAYER),
            ROUTE_REMCTRL, ROUTE_PLAYER, MSG_PLAY_PAUSEATEND, NULL);
      } else if (strcmp (querystr, "play") == 0) {
        sockhSendMessage (SOCKOF (PROCESS_MAIN),
            ROUTE_REMCTRL, ROUTE_MAIN, MSG_PLAY_PLAYPAUSE, NULL);
      } else if (strcmp (querystr, "playlistclearplay") == 0) {
        sockhSendMessage (SOCKOF (PROCESS_MAIN),
            ROUTE_REMCTRL, ROUTE_MAIN, MSG_PLAYLIST_CLEARPLAY, qstrptr);
      } else if (strcmp (querystr, "playlistqueue") == 0) {
        sockhSendMessage (SOCKOF (PROCESS_MAIN),
            ROUTE_REMCTRL, ROUTE_MAIN, MSG_PLAYLIST_QUEUE, qstrptr);
      } else if (strcmp (querystr, "repeat") == 0) {
        sockhSendMessage (SOCKOF (PROCESS_PLAYER),
            ROUTE_REMCTRL, ROUTE_PLAYER, MSG_PLAY_REPEAT, NULL);
      } else if (strcmp (querystr, "speed") == 0) {
        sockhSendMessage (SOCKOF (PROCESS_PLAYER),
            ROUTE_REMCTRL, ROUTE_PLAYER, MSG_PLAY_RATE, qstrptr);
      } else if (strcmp (querystr, "volume") == 0) {
        sockhSendMessage (SOCKOF (PROCESS_PLAYER),
            ROUTE_REMCTRL, ROUTE_PLAYER, MSG_PLAYER_VOLUME, qstrptr);
      } else if (strcmp (querystr, "volmute") == 0) {
        sockhSendMessage (SOCKOF (PROCESS_PLAYER),
            ROUTE_REMCTRL, ROUTE_PLAYER, MSG_PLAYER_VOL_MUTE, NULL);
      } else {
        ok = false;
      }
      if (ok) {
        mg_http_reply (c, 200, "Content-type: text/plain; charset=utf-8\r\n",
            "Cache-Control max-age=0\r\n", NULL);
      } else {
        mg_http_reply (c, 400, "Content-type: text/plain; charset=utf-8\r\n",
            "Cache-Control max-age=0\r\n", NULL);
      }
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
      struct mg_http_serve_opts opts = { .root_dir = sysvars [SV_BDJ4HTTPDIR] };
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
          if (routefrom == ROUTE_MAIN) {
            remctrlData->processes [PROCESS_MAIN].handshake = true;
          }
          if (routefrom == ROUTE_PLAYER) {
            remctrlData->processes [PROCESS_PLAYER].handshake = true;
          }
          break;
        }
        case MSG_EXIT_REQUEST: {
          mstimestart (&remctrlData->tm);
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          remctrlDisconnectProcess (remctrlData, PROCESS_MAIN, ROUTE_MAIN);
          remctrlDisconnectProcess (remctrlData, PROCESS_PLAYER, ROUTE_PLAYER);
          return 1;
        }
        case MSG_DANCE_LIST_DATA: {
          remctrlData->haveData++;
          remctrlData->danceList = strdup (args);
          break;
        }
        case MSG_PLAYLIST_LIST_DATA: {
          remctrlData->haveData++;
          remctrlData->playlistList = strdup (args);
          break;
        }
        case MSG_STATUS_DATA: {
          if (remctrlData->playerStatus != NULL) {
            free (remctrlData->playerStatus);
          }
          remctrlData->playerStatus = strdup (args);
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

  if (remctrlData->programState == STATE_CONNECTING) {
    uint16_t  port;

    if (SOCKOF (PROCESS_MAIN) == INVALID_SOCKET) {
      port = bdjvarsl [BDJVL_MAIN_PORT];
      remctrlConnectProcess (remctrlData, PROCESS_MAIN, ROUTE_MAIN, port);
    }

    if (SOCKOF (PROCESS_PLAYER) == INVALID_SOCKET) {
      port = bdjvarsl [BDJVL_PLAYER_PORT];
      remctrlConnectProcess (remctrlData, PROCESS_PLAYER, ROUTE_PLAYER, port);
    }

    if (SOCKOF (PROCESS_MAIN) != INVALID_SOCKET &&
        SOCKOF (PROCESS_PLAYER) != INVALID_SOCKET) {
      remctrlData->programState = STATE_WAIT_HANDSHAKE;
    }
  }

  if (remctrlData->programState == STATE_WAIT_HANDSHAKE) {
    if (remctrlData->processes [PROCESS_MAIN].handshake &&
        remctrlData->processes [PROCESS_PLAYER].handshake) {
      remctrlData->programState = STATE_RUNNING;
      logMsg (LOG_SESS, LOG_IMPORTANT, "running: time-to-start: %ld ms", mstimeend (&remctrlData->tm));
      sockhSendMessage (SOCKOF (PROCESS_MAIN), ROUTE_REMCTRL, ROUTE_MAIN,
          MSG_GET_DANCE_LIST, NULL);
      sockhSendMessage (SOCKOF (PROCESS_MAIN), ROUTE_REMCTRL, ROUTE_MAIN,
          MSG_GET_PLAYLIST_LIST, NULL);
    }
  }

  if (remctrlData->programState != STATE_RUNNING ||
      remctrlData->haveData < 2) {
    /* all of the processing that follows requires a running state */
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    }
    return gKillReceived;
  }

  if (mstimeCheck (&remctrlData->statusCheck)) {
    sockhSendMessage (SOCKOF (PROCESS_MAIN),
        ROUTE_REMCTRL, ROUTE_MAIN, MSG_GET_STATUS, NULL);
    mstimeset (&remctrlData->statusCheck, 500);
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

static void
remctrlConnectProcess (remctrldata_t *remctrlData, processconnidx_t idx,
    bdjmsgroute_t route, uint16_t port)
{
  int         err;

  SOCKOF (idx) = sockConnect (port, &err, 1000);
  if (SOCKOF (idx) != INVALID_SOCKET) {
    sockhSendMessage (SOCKOF (idx), ROUTE_REMCTRL, route,
        MSG_HANDSHAKE, NULL);
    if (route == ROUTE_MAIN) {
      logMsg (LOG_DBG, LOG_MAIN, "player: wait for handshake");
    }
  }
}

static void
remctrlDisconnectProcess (remctrldata_t *remctrlData, processconnidx_t idx,
    bdjmsgroute_t route)
{
  if (SOCKOF (idx) != INVALID_SOCKET) {
    sockhSendMessage (SOCKOF (idx), ROUTE_REMCTRL, route,
        MSG_SOCKET_CLOSE, NULL);
    sockClose (SOCKOF (idx));
    SOCKOF (idx) = INVALID_SOCKET;
  }
}
