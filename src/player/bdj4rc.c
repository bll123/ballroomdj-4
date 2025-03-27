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
  char            msgqpba [40];
  int             stopwaitcount;
  uint16_t        port;
  char            *user;
  char            *pass;
  mstime_t        danceListTimer;
  char            *danceList;
  mstime_t        playlistNamesTimer;
  char            *playlistNames;
  char            *playerStatus;
  char            *currSong;
  websrv_t        *websrv;
  bool            enabled;
} remctrl_t;

static bool     remctrlConnectingCallback (void *udata, programstate_t programState);
static bool     remctrlHandshakeCallback (void *udata, programstate_t programState);
static bool     remctrlInitDataCallback (void *udata, programstate_t programState);
static bool     remctrlStoppingCallback (void *udata, programstate_t programState);
static bool     remctrlStopWaitCallback (void *udata, programstate_t programState);
static bool     remctrlClosingCallback (void *udata, programstate_t programState);
static void     remctrlEventHandler (void *userdata, const char *query, const char *uri);
static int      remctrlProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static int      remctrlProcessing (void *udata);
static void     remctrlProcessDanceList (remctrl_t *remctrl, char *danceList);
static void     remctrlProcessPlaylistNames (remctrl_t *remctrl, char *playlistNames);
static void     remctrlSigHandler (int sig);

static int  gKillReceived = 0;

int
main (int argc, char *argv[])
{
  remctrl_t   remctrl;
  uint16_t        listenPort;
  uint32_t        flags;

#if BDJ4_MEM_DEBUG
  mdebugInit ("rc");
#endif

  osSetStandardSignals (remctrlSigHandler);

  flags = BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  bdj4startup (argc, argv, NULL, "rc", ROUTE_REMCTRL, &flags);

  remctrl.enabled = (bdjoptGetNum (OPT_P_REMOTECONTROL) != 0);
  if (! remctrl.enabled) {
    bdj4shutdown (ROUTE_REMCTRL, NULL);
    exit (0);
  }

  remctrl.danceList = "";
  mstimeset (&remctrl.danceListTimer, 0);
  remctrl.user = mdstrdup (bdjoptGetStr (OPT_P_REMCONTROLUSER));
  remctrl.pass = mdstrdup (bdjoptGetStr (OPT_P_REMCONTROLPASS));
  remctrl.playerStatus = NULL;
  remctrl.currSong = NULL;
  remctrl.playlistNames = "";
  mstimeset (&remctrl.playlistNamesTimer, 0);
  remctrl.port = bdjoptGetNum (OPT_P_REMCONTROLPORT);
  remctrl.progstate = progstateInit ("remctrl");
  remctrl.websrv = NULL;
  remctrl.stopwaitcount = 0;
  snprintf (remctrl.msgqpba, sizeof (remctrl.msgqpba), "%d", MUSICQ_PB_A);

  progstateSetCallback (remctrl.progstate, STATE_CONNECTING,
      remctrlConnectingCallback, &remctrl);
  progstateSetCallback (remctrl.progstate, STATE_WAIT_HANDSHAKE,
      remctrlHandshakeCallback, &remctrl);
  progstateSetCallback (remctrl.progstate, STATE_INITIALIZE_DATA,
      remctrlInitDataCallback, &remctrl);
  progstateSetCallback (remctrl.progstate, STATE_STOPPING,
      remctrlStoppingCallback, &remctrl);
  progstateSetCallback (remctrl.progstate, STATE_STOP_WAIT,
      remctrlStopWaitCallback, &remctrl);
  progstateSetCallback (remctrl.progstate, STATE_CLOSING,
      remctrlClosingCallback, &remctrl);

  remctrl.conn = connInit (ROUTE_REMCTRL);

  remctrl.websrv = websrvInit (remctrl.port, remctrlEventHandler,
      &remctrl, WEBSRV_TLS_OFF);

  listenPort = bdjvarsGetNum (BDJVL_PORT_REMCTRL);
  sockhMainLoop (listenPort, remctrlProcessMsg, remctrlProcessing, &remctrl);
  connFree (remctrl.conn);
  progstateFree (remctrl.progstate);
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
  remctrl_t   *remctrl = udata;

  connDisconnectAll (remctrl->conn);
  return STATE_FINISHED;
}

static bool
remctrlStopWaitCallback (void *tremctrl, programstate_t programState)
{
  remctrl_t *remctrl = tremctrl;
  bool        rc;

  rc = connWaitClosed (remctrl->conn, &remctrl->stopwaitcount);
  return rc;
}

static bool
remctrlClosingCallback (void *udata, programstate_t programState)
{
  remctrl_t   *remctrl = udata;

  bdj4shutdown (ROUTE_REMCTRL, NULL);

  websrvFree (remctrl->websrv);
  dataFree (remctrl->user);
  dataFree (remctrl->pass);
  dataFree (remctrl->playerStatus);
  dataFree (remctrl->currSong);
  if (*remctrl->danceList) {
    dataFree (remctrl->danceList);
  }
  if (*remctrl->playlistNames) {
    dataFree (remctrl->playlistNames);
  }

  return STATE_FINISHED;
}

static void
remctrlEventHandler (void *userdata, const char *query, const char *uri)
{
  remctrl_t     *remctrl = userdata;
  char          user [40];
  char          pass [40];
  char          tbuff [300];

  websrvGetUserPass (remctrl->websrv, user, sizeof (user), pass, sizeof (pass));

  if (user [0] == '\0' || pass [0] == '\0') {
    websrvReply (remctrl->websrv, WEB_UNAUTHORIZED,
        "Content-type: text/plain; charset=utf-8\r\n"
        "WWW-Authenticate: Basic realm=BDJ4 Remote\r\n",
        WEB_RESP_UNAUTH);
  } else if (strcmp (user, remctrl->user) != 0 ||
      strcmp (pass, remctrl->pass) != 0) {
    websrvReply (remctrl->websrv, WEB_UNAUTHORIZED,
        "Content-type: text/plain; charset=utf-8\r\n"
        "WWW-Authenticate: Basic realm=BDJ4 Remote\r\n",
        WEB_RESP_UNAUTH);
  } else if (strcmp (uri, "/getstatus") == 0) {
    if (remctrl->playerStatus == NULL || remctrl->currSong == NULL) {
      websrvReply (remctrl->websrv, WEB_NO_CONTENT,
          "Content-type: text/plain; charset=utf-8\r\n"
          "Cache-Control: max-age=0\r\n",
          WEB_RESP_NO_CONTENT);
    } else {
      char    *jsbuff;
      char    *jp;
      char    *jend;

      jsbuff = mdmalloc (BDJMSG_MAX);
      *jsbuff = '\0';
      jp = jsbuff;
      jend = jsbuff + BDJMSG_MAX;

      jp = stpecpy (jp, jend, remctrl->playerStatus);
      jp = stpecpy (jp, jend, remctrl->currSong);

      websrvReply (remctrl->websrv, WEB_OK,
          "Content-type: text/plain; charset=utf-8\r\n"
          "Cache-Control: max-age=0\r\n", jsbuff);
      dataFree (jsbuff);
    }
  } else if (strcmp (uri, "/getcurrsong") == 0) {
    if (remctrl->currSong == NULL) {
      websrvReply (remctrl->websrv, WEB_NO_CONTENT,
          "Content-type: text/plain; charset=utf-8\r\n"
          "Cache-Control: max-age=0\r\n",
          WEB_RESP_NO_CONTENT);
    } else {
      websrvReply (remctrl->websrv, WEB_OK,
          "Content-type: text/plain; charset=utf-8\r\n"
          "Cache-Control: max-age=0\r\n",
          remctrl->currSong);
    }
  } else if (strcmp (uri, "/cmd") == 0) {
    bool        ok = true;
    char        querycmd [80];
    char        *ptr;
    const char  *querydata = NULL;

    *tbuff = '\0';
    ptr = strstr (query, " ");
    if (ptr != NULL) {
      size_t      len;

      len = ptr - query + 1;
      if (len <= sizeof (querycmd)) {
        stpecpy (querycmd, querycmd + len, query);
        querydata = ptr + 1;
      }
    }

    if (strcmp (querycmd, "clear") == 0) {
      /* clears any playlists and truncates the music queue */
      connSendMessage (remctrl->conn,
          ROUTE_MAIN, MSG_QUEUE_CLEAR, remctrl->msgqpba);
      /* and clear the current playing song */
      connSendMessage (remctrl->conn,
          ROUTE_PLAYER, MSG_PLAY_NEXTSONG, NULL);
    } else if (strcmp (querycmd, "fade") == 0) {
      connSendMessage (remctrl->conn,
          ROUTE_PLAYER, MSG_PLAY_FADE, NULL);
    } else if (strcmp (querycmd, "nextsong") == 0) {
      connSendMessage (remctrl->conn,
          ROUTE_PLAYER, MSG_PLAY_NEXTSONG, NULL);
    } else if (strcmp (querycmd, "pauseatend") == 0) {
      connSendMessage (remctrl->conn,
          ROUTE_PLAYER, MSG_PLAY_PAUSEATEND, NULL);
    } else if (strcmp (querycmd, "play") == 0) {
      connSendMessage (remctrl->conn,
          ROUTE_MAIN, MSG_CMD_PLAYPAUSE, remctrl->msgqpba);
    } else if (strcmp (querycmd, "playlistclearplay") == 0) {
      /* clears any playlists and truncates the music queue */
      connSendMessage (remctrl->conn,
          ROUTE_MAIN, MSG_QUEUE_CLEAR, remctrl->msgqpba);
      /* and clear the current playing song */
      connSendMessage (remctrl->conn,
          ROUTE_PLAYER, MSG_PLAY_NEXTSONG, NULL);
      /* then queue the new playlist */
      snprintf (tbuff, sizeof (tbuff), "%d%c%s", MUSICQ_PB_A, MSG_ARGS_RS, querydata);
      connSendMessage (remctrl->conn,
          ROUTE_MAIN, MSG_QUEUE_PLAYLIST, tbuff);
      /* and play */
      connSendMessage (remctrl->conn,
          ROUTE_MAIN, MSG_CMD_PLAY, NULL);
    } else if (strcmp (querycmd, "playlistqueue") == 0) {
      /* queue-playlist is always not in edit mode */
      /* the edit-flag will parse as a null; main handles this */
      /* this may change at a future date */
      snprintf (tbuff, sizeof (tbuff), "%d%c%s", MUSICQ_PB_A, MSG_ARGS_RS, querydata);
      connSendMessage (remctrl->conn,
          ROUTE_MAIN, MSG_QUEUE_PLAYLIST, tbuff);
    } else if (strcmp (querycmd, "queue") == 0) {
      snprintf (tbuff, sizeof (tbuff), "%d%c%s%c%d",
          MUSICQ_PB_A, MSG_ARGS_RS, querydata, MSG_ARGS_RS, 1);
      connSendMessage (remctrl->conn,
          ROUTE_MAIN, MSG_QUEUE_DANCE, tbuff);
      connSendMessage (remctrl->conn,
          ROUTE_MAIN, MSG_CMD_PLAY, NULL);
    } else if (strcmp (querycmd, "queue5") == 0) {
      snprintf (tbuff, sizeof (tbuff), "%d%c%s%c%d",
          MUSICQ_PB_A, MSG_ARGS_RS, querydata, MSG_ARGS_RS, 5);
      connSendMessage (remctrl->conn,
          ROUTE_MAIN, MSG_QUEUE_DANCE, tbuff);
      connSendMessage (remctrl->conn,
          ROUTE_MAIN, MSG_CMD_PLAY, NULL);
    } else if (strcmp (querycmd, "repeat") == 0) {
      connSendMessage (remctrl->conn,
          ROUTE_PLAYER, MSG_PLAY_REPEAT, NULL);
    } else if (strcmp (querycmd, "speed") == 0) {
      connSendMessage (remctrl->conn,
          ROUTE_PLAYER, MSG_PLAY_SPEED, querydata);
    } else if (strcmp (querycmd, "volume") == 0) {
      connSendMessage (remctrl->conn,
          ROUTE_PLAYER, MSG_PLAYER_VOLUME, querydata);
    } else if (strcmp (querycmd, "volmute") == 0) {
      connSendMessage (remctrl->conn,
          ROUTE_PLAYER, MSG_PLAYER_VOL_MUTE, NULL);
    } else {
      ok = false;
    }

    if (ok) {
      websrvReply (remctrl->websrv, WEB_OK,
          "Content-type: text/plain; charset=utf-8\r\n"
          "Cache-Control: max-age=0\r\n",
          "");
    } else {
      websrvReply (remctrl->websrv, WEB_BAD_REQUEST,
          "Content-type: text/plain; charset=utf-8\r\n"
          "Cache-Control: max-age=0\r\n",
          WEB_RESP_BAD_REQ);
    }
  } else if (strcmp (uri, "/getdancelist") == 0) {
    websrvReply (remctrl->websrv, WEB_OK,
        "Content-type: text/plain; charset=utf-8\r\n"
        "Cache-Control: max-age=0\r\n",
        remctrl->danceList);
  } else if (strcmp (uri, "/getplaylistsel") == 0) {
    websrvReply (remctrl->websrv, WEB_OK,
        "Content-type: text/plain; charset=utf-8\r\n"
        "Cache-Control: max-age=0\r\n",
        remctrl->playlistNames);
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
    websrvServeFile (remctrl->websrv, sysvarsGetStr (SV_BDJ4_DREL_HTTP), turi);
  }
}

static int
remctrlProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  remctrl_t     *remctrl = udata;

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
          connProcessHandshake (remctrl->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          progstateShutdownProcess (remctrl->progstate);
          break;
        }
        case MSG_DANCE_LIST_DATA: {
          remctrlProcessDanceList (remctrl, args);
          break;
        }
        case MSG_PLAYLIST_LIST_DATA: {
          remctrlProcessPlaylistNames (remctrl, args);
          break;
        }
        case MSG_PLAYER_STATUS_DATA: {
          dataFree (remctrl->playerStatus);
          remctrl->playerStatus = mdstrdup (args);
          break;
        }
        case MSG_CURR_SONG_DATA: {
          dataFree (remctrl->currSong);
          remctrl->currSong = mdstrdup (args);
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
  remctrl_t *remctrl = udata;
  websrv_t      *websrv = remctrl->websrv;
  int           stop = SOCKH_CONTINUE;


  if (! progstateIsRunning (remctrl->progstate)) {
    progstateProcess (remctrl->progstate);
    if (progstateCurrState (remctrl->progstate) == STATE_CLOSED) {
      stop = SOCKH_STOP;
    }
    if (gKillReceived) {
      progstateShutdownProcess (remctrl->progstate);
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    }
    return stop;
  }

  connProcessUnconnected (remctrl->conn);

  websrvProcess (websrv);

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (remctrl->progstate);
  }
  return stop;
}

static bool
remctrlConnectingCallback (void *udata, programstate_t programState)
{
  remctrl_t   *remctrl = udata;
  bool            rc = STATE_NOT_FINISH;

  if (! connIsConnected (remctrl->conn, ROUTE_MAIN)) {
    connConnect (remctrl->conn, ROUTE_MAIN);
  }

  if (! connIsConnected (remctrl->conn, ROUTE_PLAYER)) {
    connConnect (remctrl->conn, ROUTE_PLAYER);
  }

  connProcessUnconnected (remctrl->conn);

  if (connIsConnected (remctrl->conn, ROUTE_MAIN) &&
      connIsConnected (remctrl->conn, ROUTE_PLAYER)) {
    rc = STATE_FINISHED;
  }

  return rc;
}

static bool
remctrlHandshakeCallback (void *udata, programstate_t programState)
{
  remctrl_t   *remctrl = udata;
  bool            rc = STATE_NOT_FINISH;

  connProcessUnconnected (remctrl->conn);

  if (connHaveHandshake (remctrl->conn, ROUTE_MAIN) &&
      connHaveHandshake (remctrl->conn, ROUTE_PLAYER)) {
    rc = STATE_FINISHED;
  }

  return rc;
}


static bool
remctrlInitDataCallback (void *udata, programstate_t programState)
{
  remctrl_t   *remctrl = udata;
  bool            rc = STATE_NOT_FINISH;

  if (connHaveHandshake (remctrl->conn, ROUTE_MAIN)) {
    if (*remctrl->danceList == '\0' &&
        mstimeCheck (&remctrl->danceListTimer)) {
      connSendMessage (remctrl->conn, ROUTE_MAIN,
          MSG_GET_DANCE_LIST, NULL);
      mstimeset (&remctrl->danceListTimer, 500);
    }
    if (*remctrl->playlistNames == '\0' &&
        mstimeCheck (&remctrl->playlistNamesTimer)) {
      connSendMessage (remctrl->conn, ROUTE_MAIN,
          MSG_GET_PLAYLIST_LIST, NULL);
      mstimeset (&remctrl->playlistNamesTimer, 500);
    }
  }
  if (*remctrl->danceList && *remctrl->playlistNames) {
    rc = STATE_FINISHED;
  }

  return rc;
}


static void
remctrlProcessDanceList (remctrl_t *remctrl, char *danceList)
{
  char        *didx;
  char        *dstr;
  char        *tokstr;
  char        tbuff [200];
  char        obuff [3096];
  char        *p = obuff;
  char        *end = obuff + sizeof (obuff);

  obuff [0] = '\0';

  didx = strtok_r (danceList, MSG_ARGS_RS_STR, &tokstr);
  dstr = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  while (didx != NULL) {
    snprintf (tbuff, sizeof (tbuff), "<option value=\"%ld\">%s</option>\n", atol (didx), dstr);
    p = stpecpy (p, end, tbuff);
    didx = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    dstr = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  }

  if (*remctrl->danceList) {
    mdfree (remctrl->danceList);
  }
  remctrl->danceList = mdstrdup (obuff);
}


static void
remctrlProcessPlaylistNames (remctrl_t *remctrl, char *playlistNames)
{
  char        *fnm;
  char        *plnm;
  char        *tokstr;
  char        tbuff [200];
  char        obuff [3096];
  char        *p = obuff;
  char        *end = obuff + sizeof (obuff);

  obuff [0] = '\0';

  fnm = strtok_r (playlistNames, MSG_ARGS_RS_STR, &tokstr);
  plnm = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  while (fnm != NULL) {
    snprintf (tbuff, sizeof (tbuff), "<option value=\"%s\">%s</option>\n", fnm, plnm);
    p = stpecpy (p, end, tbuff);
    fnm = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    plnm = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  }

  if (*remctrl->playlistNames) {
    mdfree (remctrl->playlistNames);
  }
  remctrl->playlistNames = mdstrdup (obuff);
}


static void
remctrlSigHandler (int sig)
{
  gKillReceived = 1;
}

