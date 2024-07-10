/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include "mongoose.h"

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
  char            msgqpba [40];
  int             stopwaitcount;
  char            *locknm;
  uint16_t        port;
  char            *user;
  char            *pass;
  mstime_t        danceListTimer;
  char            *danceList;
  mstime_t        playlistListTimer;
  char            *playlistList;
  char            *playerStatus;
  char            *currSong;
  websrv_t        *websrv;
  bool            enabled : 1;
} remctrldata_t;

static bool     remctrlConnectingCallback (void *udata, programstate_t programState);
static bool     remctrlHandshakeCallback (void *udata, programstate_t programState);
static bool     remctrlInitDataCallback (void *udata, programstate_t programState);
static bool     remctrlStoppingCallback (void *udata, programstate_t programState);
static bool     remctrlStopWaitCallback (void *udata, programstate_t programState);
static bool     remctrlClosingCallback (void *udata, programstate_t programState);
static void     remctrlEventHandler (struct mg_connection *c, int ev,
                    void *ev_data, void *userdata);
static int      remctrlProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static int      remctrlProcessing (void *udata);
static void     remctrlProcessDanceList (remctrldata_t *remctrlData, char *danceList);
static void     remctrlProcessPlaylistList (remctrldata_t *remctrlData, char *playlistList);
static void     remctrlSigHandler (int sig);

static int  gKillReceived = 0;

int
main (int argc, char *argv[])
{
  remctrldata_t   remctrlData;
  uint16_t        listenPort;
  uint32_t        flags;

#if BDJ4_MEM_DEBUG
  mdebugInit ("rc");
#endif

  osSetStandardSignals (remctrlSigHandler);

  flags = BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  bdj4startup (argc, argv, NULL, "rc", ROUTE_REMCTRL, &flags);

  remctrlData.enabled = (bdjoptGetNum (OPT_P_REMOTECONTROL) != 0);
  if (! remctrlData.enabled) {
    lockRelease (remctrlData.locknm, PATHBLD_MP_USEIDX);
    exit (0);
  }

  remctrlData.danceList = "";
  mstimeset (&remctrlData.danceListTimer, 0);
  remctrlData.user = mdstrdup (bdjoptGetStr (OPT_P_REMCONTROLUSER));
  remctrlData.pass = mdstrdup (bdjoptGetStr (OPT_P_REMCONTROLPASS));
  remctrlData.playerStatus = NULL;
  remctrlData.currSong = NULL;
  remctrlData.playlistList = "";
  mstimeset (&remctrlData.playlistListTimer, 0);
  remctrlData.port = bdjoptGetNum (OPT_P_REMCONTROLPORT);
  remctrlData.progstate = progstateInit ("remctrl");
  remctrlData.websrv = NULL;
  remctrlData.stopwaitcount = 0;
  snprintf (remctrlData.msgqpba, sizeof (remctrlData.msgqpba), "%d", MUSICQ_PB_A);

  progstateSetCallback (remctrlData.progstate, STATE_CONNECTING,
      remctrlConnectingCallback, &remctrlData);
  progstateSetCallback (remctrlData.progstate, STATE_WAIT_HANDSHAKE,
      remctrlHandshakeCallback, &remctrlData);
  progstateSetCallback (remctrlData.progstate, STATE_INITIALIZE_DATA,
      remctrlInitDataCallback, &remctrlData);
  progstateSetCallback (remctrlData.progstate, STATE_STOPPING,
      remctrlStoppingCallback, &remctrlData);
  progstateSetCallback (remctrlData.progstate, STATE_STOP_WAIT,
      remctrlStopWaitCallback, &remctrlData);
  progstateSetCallback (remctrlData.progstate, STATE_CLOSING,
      remctrlClosingCallback, &remctrlData);

  remctrlData.conn = connInit (ROUTE_REMCTRL);

  remctrlData.websrv = websrvInit (remctrlData.port, remctrlEventHandler, &remctrlData);

  listenPort = bdjvarsGetNum (BDJVL_REMCTRL_PORT);
  sockhMainLoop (listenPort, remctrlProcessMsg, remctrlProcessing, &remctrlData);
  connFree (remctrlData.conn);
  progstateFree (remctrlData.progstate);
  logEnd ();

#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return 0;
}

/* internal routines */

static bool
remctrlStoppingCallback (void *udata, programstate_t programState)
{
  remctrldata_t   *remctrlData = udata;

  connDisconnectAll (remctrlData->conn);
  return STATE_FINISHED;
}

static bool
remctrlStopWaitCallback (void *tremctrl, programstate_t programState)
{
  remctrldata_t *remctrl = tremctrl;
  bool        rc;

  rc = connWaitClosed (remctrl->conn, &remctrl->stopwaitcount);
  return rc;
}

static bool
remctrlClosingCallback (void *udata, programstate_t programState)
{
  remctrldata_t   *remctrlData = udata;

  bdj4shutdown (ROUTE_REMCTRL, NULL);

  websrvFree (remctrlData->websrv);
  dataFree (remctrlData->user);
  dataFree (remctrlData->pass);
  dataFree (remctrlData->playerStatus);
  dataFree (remctrlData->currSong);
  if (*remctrlData->danceList) {
    dataFree (remctrlData->danceList);
  }
  if (*remctrlData->playlistList) {
    dataFree (remctrlData->playlistList);
  }

  return STATE_FINISHED;
}

static void
remctrlEventHandler (struct mg_connection *c, int ev,
    void *ev_data, void *userdata)
{
  remctrldata_t *remctrlData = userdata;
  char          user [40];
  char          pass [40];
  char          querystr [200];
  char          *tokptr;
  char          *qstrptr;
  char          tbuff [300];

  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;

    /* "user" is now user name and "pass" is now password from request */
    mg_http_creds (hm, user, sizeof(user), pass, sizeof(pass));

    mg_url_decode (hm->query.ptr, hm->query.len, querystr, sizeof (querystr), 1);

    *tbuff = '\0';
    qstrptr = NULL;
    if (*querystr) {
      logMsg (LOG_DBG, LOG_BASIC, "process: %s", querystr);

      qstrptr = strtok_r (querystr, " ", &tokptr);

      if (qstrptr != NULL) {
        /* point at the data following the querystr */
        if (hm->query.len > strlen (querystr)) {
          qstrptr += strlen (querystr) + 1;
          logMsg (LOG_DBG, LOG_BASIC, "  args: %s", qstrptr);
          snprintf (tbuff, sizeof (tbuff), "%d%c%s", MUSICQ_PB_A, MSG_ARGS_RS, qstrptr);
        }
      }
    }

    if (user [0] == '\0' || pass [0] == '\0') {
      mg_http_reply (c, 401,
          "Content-type: text/plain; charset=utf-8\r\n"
          "WWW-Authenticate: Basic realm=BDJ4 Remote\r\n",
          "Unauthorized");
    } else if (strcmp (user, remctrlData->user) != 0 ||
        strcmp (pass, remctrlData->pass) != 0) {
      mg_http_reply (c, 401,
          "Content-type: text/plain; charset=utf-8\r\n"
          "WWW-Authenticate: Basic realm=BDJ4 Remote\r\n",
          "Unauthorized");
    } else if (mg_http_match_uri (hm, "/getstatus")) {
      if (remctrlData->playerStatus == NULL || remctrlData->currSong == NULL) {
        mg_http_reply (c, 204,
            "Content-type: text/plain; charset=utf-8\r\n"
            "Cache-Control: max-age=0\r\n",
            "");
      } else {
        char    *jsbuff;

        jsbuff = mdmalloc (BDJMSG_MAX);
        *jsbuff = '\0';

        strlcpy (jsbuff, remctrlData->playerStatus, BDJMSG_MAX);
        strlcat (jsbuff, remctrlData->currSong, BDJMSG_MAX);

        mg_http_reply (c, 200,
            "Content-type: text/plain; charset=utf-8\r\n"
            "Cache-Control: max-age=0\r\n", jsbuff);
        dataFree (jsbuff);
      }
    } else if (mg_http_match_uri (hm, "/getcurrsong")) {
      if (remctrlData->currSong == NULL) {
        mg_http_reply (c, 204,
            "Content-type: text/plain; charset=utf-8\r\n"
            "Cache-Control: max-age=0\r\n",
            "");
      } else {
        mg_http_reply (c, 200,
            "Content-type: text/plain; charset=utf-8\r\n"
            "Cache-Control: max-age=0\r\n",
            remctrlData->currSong);
      }
    } else if (mg_http_match_uri (hm, "/cmd")) {
      bool ok = true;

      if (strcmp (querystr, "clear") == 0) {
        /* clears any playlists and truncates the music queue */
        connSendMessage (remctrlData->conn,
            ROUTE_MAIN, MSG_QUEUE_CLEAR, remctrlData->msgqpba);
        /* and clear the current playing song */
        connSendMessage (remctrlData->conn,
            ROUTE_PLAYER, MSG_PLAY_NEXTSONG, NULL);
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
            ROUTE_MAIN, MSG_CMD_PLAYPAUSE, remctrlData->msgqpba);
      } else if (strcmp (querystr, "playlistclearplay") == 0) {
        /* clears any playlists and truncates the music queue */
        connSendMessage (remctrlData->conn,
            ROUTE_MAIN, MSG_QUEUE_CLEAR, remctrlData->msgqpba);
        /* and clear the current playing song */
        connSendMessage (remctrlData->conn,
            ROUTE_PLAYER, MSG_PLAY_NEXTSONG, NULL);
        /* then queue the new playlist */
        connSendMessage (remctrlData->conn,
            ROUTE_MAIN, MSG_QUEUE_PLAYLIST, tbuff);
        /* and play */
        connSendMessage (remctrlData->conn,
            ROUTE_MAIN, MSG_CMD_PLAY, NULL);
      } else if (strcmp (querystr, "playlistqueue") == 0) {
        /* queue-playlist is always not in edit mode */
        /* the edit-flag will parse as a null; main handles this */
        /* this may change at a future date */
        connSendMessage (remctrlData->conn,
            ROUTE_MAIN, MSG_QUEUE_PLAYLIST, tbuff);
      } else if (strcmp (querystr, "queue") == 0) {
        connSendMessage (remctrlData->conn,
            ROUTE_MAIN, MSG_QUEUE_DANCE, tbuff);
        connSendMessage (remctrlData->conn,
            ROUTE_MAIN, MSG_CMD_PLAY, remctrlData->msgqpba);
      } else if (strcmp (querystr, "queue5") == 0) {
        connSendMessage (remctrlData->conn,
            ROUTE_MAIN, MSG_QUEUE_DANCE_5, tbuff);
        connSendMessage (remctrlData->conn,
            ROUTE_MAIN, MSG_CMD_PLAY, remctrlData->msgqpba);
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
        mg_http_reply (c, 200,
            "Content-type: text/plain; charset=utf-8\r\n"
            "Cache-Control: max-age=0\r\n",
            "");
      } else {
        mg_http_reply (c, 400,
            "Content-type: text/plain; charset=utf-8\r\n"
            "Cache-Control: max-age=0\r\n",
            "");
      }
    } else if (mg_http_match_uri (hm, "/getdancelist")) {
      mg_http_reply (c, 200,
          "Content-type: text/plain; charset=utf-8\r\n"
          "Cache-Control: max-age=0\r\n",
          remctrlData->danceList);
    } else if (mg_http_match_uri (hm, "/getplaylistsel")) {
      mg_http_reply (c, 200,
          "Content-type: text/plain; charset=utf-8\r\n"
          "Cache-Control: max-age=0\r\n",
          remctrlData->playlistList);
    } else if (mg_http_match_uri (hm, "#.key") ||
        mg_http_match_uri (hm, "#.crt") ||
        mg_http_match_uri (hm, "#.pem") ||
        mg_http_match_uri (hm, "#.csr") ||
        mg_http_match_uri (hm, "../")) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "%.*s", (int) hm->uri.len, hm->uri.ptr);
      mg_http_reply (c, 403, NULL, "%s", "Forbidden");
    } else {
      char          tbuff [200];
      char          tbuffb [MAXPATHLEN];
      struct mg_str turi;
      struct mg_str tmpuri;
      bool          reloc = false;

      struct mg_http_serve_opts opts = { .root_dir = sysvarsGetStr (SV_BDJ4_DREL_HTTP) };
      snprintf (tbuff, sizeof (tbuff), "%.*s", (int) hm->uri.len, hm->uri.ptr);
      pathbldMakePath (tbuffb, sizeof (tbuffb),
          tbuff, "", PATHBLD_MP_DREL_HTTP);
      if (! fileopFileExists (tbuffb)) {
        turi = mg_str ("/bdj4remote.html");
        reloc = true;
      }

      if (reloc) {
        tmpuri = hm->uri;
        hm->uri = turi;
      }
      logMsg (LOG_DBG, LOG_IMPORTANT, "serve: %.*s", (int) hm->uri.len, hm->uri.ptr);
      mg_http_serve_dir (c, hm, &opts);
      if (reloc) {
        hm->uri = tmpuri;
      }
    }
  }
}

static int
remctrlProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  remctrldata_t     *remctrlData = udata;

  /* this just reduces the amount of stuff in the log */
  if (msg != MSG_PLAYER_STATUS_DATA) {
    logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
        routefrom, msgRouteDebugText (routefrom),
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
  }

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_REMCTRL: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (remctrlData->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          progstateShutdownProcess (remctrlData->progstate);
          break;
        }
        case MSG_DANCE_LIST_DATA: {
          remctrlProcessDanceList (remctrlData, args);
          break;
        }
        case MSG_PLAYLIST_LIST_DATA: {
          remctrlProcessPlaylistList (remctrlData, args);
          break;
        }
        case MSG_PLAYER_STATUS_DATA: {
          dataFree (remctrlData->playerStatus);
          remctrlData->playerStatus = mdstrdup (args);
          break;
        }
        case MSG_CURR_SONG_DATA: {
          dataFree (remctrlData->currSong);
          remctrlData->currSong = mdstrdup (args);
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
  remctrldata_t *remctrlData = udata;
  websrv_t      *websrv = remctrlData->websrv;
  int           stop = false;


  if (! progstateIsRunning (remctrlData->progstate)) {
    progstateProcess (remctrlData->progstate);
    if (progstateCurrState (remctrlData->progstate) == STATE_CLOSED) {
      stop = true;
    }
    if (gKillReceived) {
      progstateShutdownProcess (remctrlData->progstate);
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    }
    return stop;
  }

  connProcessUnconnected (remctrlData->conn);

  websrvProcess (websrv);

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (remctrlData->progstate);
  }
  return stop;
}

static bool
remctrlConnectingCallback (void *udata, programstate_t programState)
{
  remctrldata_t   *remctrlData = udata;
  bool            rc = STATE_NOT_FINISH;

  if (! connIsConnected (remctrlData->conn, ROUTE_MAIN)) {
    connConnect (remctrlData->conn, ROUTE_MAIN);
  }

  if (! connIsConnected (remctrlData->conn, ROUTE_PLAYER)) {
    connConnect (remctrlData->conn, ROUTE_PLAYER);
  }

  connProcessUnconnected (remctrlData->conn);

  if (connIsConnected (remctrlData->conn, ROUTE_MAIN) &&
      connIsConnected (remctrlData->conn, ROUTE_PLAYER)) {
    rc = STATE_FINISHED;
  }

  return rc;
}

static bool
remctrlHandshakeCallback (void *udata, programstate_t programState)
{
  remctrldata_t   *remctrlData = udata;
  bool            rc = STATE_NOT_FINISH;

  connProcessUnconnected (remctrlData->conn);

  if (connHaveHandshake (remctrlData->conn, ROUTE_MAIN) &&
      connHaveHandshake (remctrlData->conn, ROUTE_PLAYER)) {
    rc = STATE_FINISHED;
  }

  return rc;
}


static bool
remctrlInitDataCallback (void *udata, programstate_t programState)
{
  remctrldata_t   *remctrlData = udata;
  bool            rc = STATE_NOT_FINISH;

  if (connHaveHandshake (remctrlData->conn, ROUTE_MAIN)) {
    if (*remctrlData->danceList == '\0' &&
        mstimeCheck (&remctrlData->danceListTimer)) {
      connSendMessage (remctrlData->conn, ROUTE_MAIN,
          MSG_GET_DANCE_LIST, NULL);
      mstimeset (&remctrlData->danceListTimer, 500);
    }
    if (*remctrlData->playlistList == '\0' &&
        mstimeCheck (&remctrlData->playlistListTimer)) {
      connSendMessage (remctrlData->conn, ROUTE_MAIN,
          MSG_GET_PLAYLIST_LIST, NULL);
      mstimeset (&remctrlData->playlistListTimer, 500);
    }
  }
  if (*remctrlData->danceList && *remctrlData->playlistList) {
    rc = STATE_FINISHED;
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

  if (*remctrlData->danceList) {
    mdfree (remctrlData->danceList);
  }
  remctrlData->danceList = mdstrdup (obuff);
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

  if (*remctrlData->playlistList) {
    mdfree (remctrlData->playlistList);
  }
  remctrlData->playlistList = mdstrdup (obuff);
}


static void
remctrlSigHandler (int sig)
{
  gKillReceived = 1;
}

