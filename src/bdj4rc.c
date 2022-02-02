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
#include "bdjstring.h"
#include "bdjvars.h"
#include "conn.h"
#include "lock.h"
#include "log.h"
#include "pathbld.h"
#include "process.h"
#include "progstart.h"
#include "sock.h"
#include "sockh.h"
#include "sysvars.h"
#include "tmutil.h"
#include "websrv.h"

typedef struct {
  conn_t          *conn;
  progstart_t     *progstart;
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

static bool     remctrlConnectingCallback (void *udata, programstate_t programState);
static bool     remctrlHandshakeCallback (void *udata, programstate_t programState);
static bool     remctrlInitDataCallback (void *udata, programstate_t programState);
static bool     remctrlStoppingCallback (void *udata, programstate_t programState);
static bool     remctrlClosingCallback (void *udata, programstate_t programState);
static void     remctrlEventHandler (struct mg_connection *c, int ev,
                    void *ev_data, void *userdata);
static int      remctrlProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static int      remctrlProcessing (void *udata);
static void     remctrlProcessDanceList (remctrldata_t *remctrlData, char *danceList);
static void     remctrlProcessPlaylistList (remctrldata_t *remctrlData, char *playlistList);
static void     remctrlSigHandler (int sig);

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

  logStartAppend ("remotecontrol", "rc", loglevel);
  logMsg (LOG_SESS, LOG_IMPORTANT, "Using profile %ld", lsysvars [SVL_BDJIDX]);

  rc = lockAcquire (REMCTRL_LOCK_FN, PATHBLD_MP_USEIDX);
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
    lockRelease (REMCTRL_LOCK_FN, PATHBLD_MP_USEIDX);
    exit (0);
  }

  remctrlData.danceList = "";
  remctrlData.haveData = 0;
  remctrlData.pass = strdup (bdjoptGetData (OPT_P_REMCONTROLPASS));
  remctrlData.playerStatus = NULL;
  remctrlData.playlistList = "";
  remctrlData.port = bdjoptGetNum (OPT_P_REMCONTROLPORT);
  remctrlData.progstart = progstartInit ("remctrl");
  progstartSetCallback (remctrlData.progstart, STATE_CONNECTING,
      remctrlConnectingCallback, &remctrlData);
  progstartSetCallback (remctrlData.progstart, STATE_WAIT_HANDSHAKE,
      remctrlHandshakeCallback, &remctrlData);
  progstartSetCallback (remctrlData.progstart, STATE_INITIALIZE_DATA,
      remctrlInitDataCallback, &remctrlData);
  progstartSetCallback (remctrlData.progstart, STATE_STOPPING,
      remctrlStoppingCallback, &remctrlData);
  progstartSetCallback (remctrlData.progstart, STATE_CLOSING,
      remctrlClosingCallback, &remctrlData);
  remctrlData.user = strdup (bdjoptGetData (OPT_P_REMCONTROLUSER));
  remctrlData.websrv = NULL;
  remctrlData.conn = connInit (ROUTE_REMCTRL);

  remctrlData.websrv = websrvInit (remctrlData.port, remctrlEventHandler, &remctrlData);

  listenPort = bdjvarsl [BDJVL_REMCTRL_PORT];
  sockhMainLoop (listenPort, remctrlProcessMsg, remctrlProcessing, &remctrlData);

  while (progstartShutdownProcess (remctrlData.progstart) != STATE_CLOSED) {
    ;
  }
  progstartFree (remctrlData.progstart);
  logEnd ();

  return 0;
}

/* internal routines */

static bool
remctrlStoppingCallback (void *udata, programstate_t programState)
{
  remctrldata_t   *remctrlData = udata;

  connDisconnectAll (remctrlData->conn);
  return true;
}

static bool
remctrlClosingCallback (void *udata, programstate_t programState)
{
  remctrldata_t   *remctrlData = udata;

  connFree (remctrlData->conn);

  websrvFree (remctrlData->websrv);
  if (remctrlData->user != NULL) {
    free (remctrlData->user);
  }
  if (remctrlData->pass != NULL) {
    free (remctrlData->pass);
  }
  if (remctrlData->playerStatus != NULL) {
    free (remctrlData->playerStatus);
  }
  if (*remctrlData->danceList) {
    free (remctrlData->danceList);
  }
  if (*remctrlData->playlistList) {
    free (remctrlData->playlistList);
  }
  bdjoptFree ();
  bdjvarsCleanup ();
  lockRelease (REMCTRL_LOCK_FN, PATHBLD_MP_USEIDX);

  return true;
}

static void
remctrlEventHandler (struct mg_connection *c, int ev,
    void *ev_data, void *userdata)
{
  remctrldata_t *remctrlData = userdata;
  char          user [40];
  char          pass [40];
  char          querystr [40];
  char          *tokptr;
  char          *qstrptr;
  char          tbuff [200];

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
        snprintf (tbuff, sizeof (tbuff), "0%c%s", MSG_ARGS_RS, qstrptr);
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
            "Cache-Control: max-age=0\r\n", "");
      } else {
        mg_http_reply (c, 200, "Content-type: text/plain; charset=utf-8\r\n"
            "Cache-Control: max-age=0\r\n", remctrlData->playerStatus);
      }
    } else if (mg_http_match_uri (hm, "/cmd")) {
      bool ok = true;

      if (strcmp (querystr, "clear") == 0) {
        connSendMessage (remctrlData->conn,
            ROUTE_MAIN, MSG_QUEUE_CLEAR, "0");
      } else if (strcmp (querystr, "fade") == 0) {
        connSendMessage (remctrlData->conn,
            ROUTE_PLAYER, MSG_PLAY_FADE, NULL);
      } else if (strcmp (querystr, "nextsong") == 0) {
        connSendMessage (remctrlData->conn,
            ROUTE_PLAYER, MSG_PLAY_NEXTSONG, NULL);
      } else if (strcmp (querystr, "pauseatend") == 0) {
        connSendMessage (remctrlData->conn,
            ROUTE_PLAYER, MSG_PLAY_PAUSEATEND, NULL);
      } else if (strcmp (querystr, "play") == 0) {
        connSendMessage (remctrlData->conn,
            ROUTE_MAIN, MSG_PLAY_PLAYPAUSE, "0");
      } else if (strcmp (querystr, "playlistclearplay") == 0) {
        connSendMessage (remctrlData->conn,
            ROUTE_MAIN, MSG_PLAYLIST_CLEARPLAY, tbuff);
      } else if (strcmp (querystr, "playlistqueue") == 0) {
        connSendMessage (remctrlData->conn,
            ROUTE_MAIN, MSG_QUEUE_PLAYLIST, tbuff);
      } else if (strcmp (querystr, "queue") == 0) {
        connSendMessage (remctrlData->conn,
            ROUTE_MAIN, MSG_QUEUE_DANCE, tbuff);
      } else if (strcmp (querystr, "queue5") == 0) {
        connSendMessage (remctrlData->conn,
            ROUTE_MAIN, MSG_QUEUE_DANCE_5, tbuff);
      } else if (strcmp (querystr, "repeat") == 0) {
        connSendMessage (remctrlData->conn,
            ROUTE_PLAYER, MSG_PLAY_REPEAT, NULL);
      } else if (strcmp (querystr, "speed") == 0) {
        connSendMessage (remctrlData->conn,
            ROUTE_PLAYER, MSG_PLAY_SPEED, qstrptr);
      } else if (strcmp (querystr, "volume") == 0) {
        connSendMessage (remctrlData->conn,
            ROUTE_PLAYER, MSG_PLAYER_VOLUME, qstrptr);
      } else if (strcmp (querystr, "volmute") == 0) {
        connSendMessage (remctrlData->conn,
            ROUTE_PLAYER, MSG_PLAYER_VOL_MUTE, NULL);
      } else {
        ok = false;
      }
      if (ok) {
        mg_http_reply (c, 200, "Content-type: text/plain; charset=utf-8\r\n",
            "Cache-Control: max-age=0\r\n", NULL);
      } else {
        mg_http_reply (c, 400, "Content-type: text/plain; charset=utf-8\r\n",
            "Cache-Control: max-age=0\r\n", NULL);
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

  if (msg != MSG_STATUS_DATA) {
    logMsg (LOG_DBG, LOG_MSGS, "got: from: %ld route: %ld msg:%ld args:%s",
        routefrom, route, msg, args);
  }

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_REMCTRL: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (remctrlData->conn, routefrom);
          connConnectResponse (remctrlData->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          progstartShutdownProcess (remctrlData->progstart);
          return 1;
        }
        case MSG_DANCE_LIST_DATA: {
          remctrlData->haveData++;
          remctrlProcessDanceList (remctrlData, args);
          break;
        }
        case MSG_PLAYLIST_LIST_DATA: {
          remctrlData->haveData++;
          remctrlProcessPlaylistList (remctrlData, args);
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


  if (! progstartIsRunning (remctrlData->progstart)) {
    progstartProcess (remctrlData->progstart);
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
remctrlConnectingCallback (void *udata, programstate_t programState)
{
  remctrldata_t   *remctrlData = udata;
  bool            rc = false;

  if (! connIsConnected (remctrlData->conn, ROUTE_MAIN)) {
    connConnect (remctrlData->conn, ROUTE_MAIN);
  }

  if (! connIsConnected (remctrlData->conn, ROUTE_PLAYER)) {
    connConnect (remctrlData->conn, ROUTE_PLAYER);
  }

  if (connIsConnected (remctrlData->conn, ROUTE_MAIN) &&
      connIsConnected (remctrlData->conn, ROUTE_PLAYER)) {
    rc = true;
  }

  return rc;
}

static bool
remctrlHandshakeCallback (void *udata, programstate_t programState)
{
  remctrldata_t   *remctrlData = udata;
  bool            rc = false;

  if (connHaveHandshake (remctrlData->conn, ROUTE_MAIN) &&
      connHaveHandshake (remctrlData->conn, ROUTE_PLAYER)) {
    connSendMessage (remctrlData->conn, ROUTE_MAIN,
        MSG_GET_DANCE_LIST, NULL);
    connSendMessage (remctrlData->conn, ROUTE_MAIN,
        MSG_GET_PLAYLIST_LIST, NULL);
    rc = true;
  }

  return rc;
}


static bool
remctrlInitDataCallback (void *udata, programstate_t programState)
{
  remctrldata_t   *remctrlData = udata;
  bool            rc = false;

  if (remctrlData->haveData == 2) {
    rc = true;
  }

  return rc;
}


static void
remctrlProcessDanceList (remctrldata_t *remctrlData, char *danceList)
{
  char        *didx;
  char        *dstr;
  char        *tokstr;
  char        tbuff [200];
  char        obuff [3096];

  obuff [0] = '\0';

  didx = strtok_r (danceList, MSG_ARGS_RS_STR, &tokstr);
  dstr = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  while (didx != NULL) {
    snprintf (tbuff, sizeof (tbuff), "<option value=\"%ld\">%s</option>\n", atol (didx), dstr);
    strlcat (obuff, tbuff, sizeof (obuff));
    didx = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    dstr = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  }

  remctrlData->danceList = strdup (obuff);
}


static void
remctrlProcessPlaylistList (remctrldata_t *remctrlData, char *playlistList)
{
  char        *fnm;
  char        *plnm;
  char        *tokstr;
  char        tbuff [200];
  char        obuff [3096];

  obuff [0] = '\0';

  fnm = strtok_r (playlistList, MSG_ARGS_RS_STR, &tokstr);
  plnm = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  while (fnm != NULL) {
    snprintf (tbuff, sizeof (tbuff), "<option value=\"%s\">%s</option>\n", fnm, plnm);
    strlcat (obuff, tbuff, sizeof (obuff));
    fnm = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    plnm = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  }

  remctrlData->playlistList = strdup (obuff);
}


static void
remctrlSigHandler (int sig)
{
  gKillReceived = 1;
}

