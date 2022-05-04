/*
 * bdj4main
 *  Main entry point for the player process.
 *  Handles startup of the player, mobile marquee and remote control.
 *  Handles playlists and the music queue.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "bdj4.h"
#include "bdj4init.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "bdjvars.h"
#include "conn.h"
#include "dance.h"
#include "dancesel.h"
#include "filedata.h"
#include "fileop.h"
#include "lock.h"
#include "log.h"
#include "musicdb.h"
#include "musicq.h"
#include "osutils.h"
#include "pathbld.h"
#include "playlist.h"
#include "procutil.h"
#include "progstate.h"
#include "queue.h"
#include "slist.h"
#include "sock.h"
#include "sockh.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"
#include "webclient.h"

typedef enum {
  MOVE_UP = -1,
  MOVE_DOWN = 1,
} mainmove_t;

#define PREP_SIZE     5
#define MAIN_NOT_SET  -1

/* playlistCache contains all of the playlists that have been seen */
/* so that playlist lookups can be processed */
typedef struct {
  progstate_t       *progstate;
  procutil_t        *processes [ROUTE_MAX];
  conn_t            *conn;
  int               startflags;
  musicdb_t         *musicdb;
  slist_t           *playlistCache;
  queue_t           *playlistQueue [MUSICQ_MAX];
  musicq_t          *musicQueue;
  nlist_t           *danceCounts;
  musicqidx_t       musicqPlayIdx;
  musicqidx_t       musicqManageIdx;
  int               musicqDeferredPlayIdx;
  slist_t           *announceList;
  playerstate_t     playerState;
  ssize_t           gap;
  webclient_t       *webclient;
  char              *mobmqUserkey;
  int               stopwaitcount;
  bool              musicqChanged [MUSICQ_MAX];
  bool              marqueeChanged [MUSICQ_MAX];
  bool              playWhenQueued : 1;
  bool              switchQueueWhenEmpty : 1;
  bool              finished : 1;
  bool              marqueestarted : 1;
} maindata_t;

static int      mainProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static int      mainProcessing (void *udata);
static bool     mainListeningCallback (void *tmaindata, programstate_t programState);
static bool     mainConnectingCallback (void *tmaindata, programstate_t programState);
static bool     mainHandshakeCallback (void *tmaindata, programstate_t programState);
static void     mainStartMarquee (maindata_t *mainData);
static bool     mainStoppingCallback (void *tmaindata, programstate_t programState);
static bool     mainStopWaitCallback (void *tmaindata, programstate_t programState);
static bool     mainClosingCallback (void *tmaindata, programstate_t programState);
static void     mainSendMusicQueueData (maindata_t *mainData, int musicqidx);
static void     mainSendMarqueeData (maindata_t *mainData);
static char     * mainSongGetDanceDisplay (maindata_t *mainData, ssize_t idx);
static void     mainSendMobileMarqueeData (maindata_t *mainData);
static void     mainMobilePostCallback (void *userdata, char *resp, size_t len);
static void     mainQueueClear (maindata_t *mainData, char *args);
static void     mainQueueDance (maindata_t *mainData, char *args, ssize_t count);
static void     mainQueuePlaylist (maindata_t *mainData, char *plname);
static void     mainSigHandler (int sig);
static void     mainMusicQueueFill (maindata_t *mainData);
static void     mainMusicQueuePrep (maindata_t *mainData);
static char     *mainPrepSong (maindata_t *maindata, song_t *song,
                    char *sfname, char *plname, bdjmsgprep_t flag);
static void     mainTogglePause (maindata_t *mainData, char *args);
static void     mainMusicqMove (maindata_t *mainData, char *args, mainmove_t direction);
static void     mainMusicqMoveTop (maindata_t *mainData, char *args);
static void     mainMusicqClear (maindata_t *mainData, char *args);
static void     mainMusicqRemove (maindata_t *mainData, char *args);
static void     mainMusicqInsert (maindata_t *mainData, bdjmsgroute_t route, char *args);
static void     mainMusicqSetPlayback (maindata_t *mainData, char *args);
static void     mainMusicqSwitch (maindata_t *mainData, musicqidx_t newidx);
static void     mainPlaybackBegin (maindata_t *mainData);
static void     mainMusicQueuePlay (maindata_t *mainData);
static void     mainMusicQueueFinish (maindata_t *mainData);
static void     mainMusicQueueNext (maindata_t *mainData);
static bool     mainCheckMusicQueue (song_t *song, void *tdata);
static ilistidx_t mainMusicQueueHistory (void *mainData, ilistidx_t idx);
static void     mainSendDanceList (maindata_t *mainData, bdjmsgroute_t route);
static void     mainSendPlaylistList (maindata_t *mainData, bdjmsgroute_t route);
static void     mainSendPlayerStatus (maindata_t *mainData, char *playerResp);
static void     mainSendMusicqStatus (maindata_t *mainData, char *rbuff, size_t siz);
static void     mainDanceCountsInit (maindata_t *mainData);
static void     mainParseIntNum (char *args, int *a, ssize_t *b);
static void     mainParseIntStr (char *args, int *a, char **b);
static void     mainSendFinished (maindata_t *mainData);

static long globalCounter = 0;
static int  gKillReceived = 0;

int
main (int argc, char *argv[])
{
  maindata_t    mainData;
  uint16_t      listenPort;


  mainData.progstate = progstateInit ("main");
  progstateSetCallback (mainData.progstate, STATE_LISTENING,
      mainListeningCallback, &mainData);
  progstateSetCallback (mainData.progstate, STATE_CONNECTING,
      mainConnectingCallback, &mainData);
  progstateSetCallback (mainData.progstate, STATE_WAIT_HANDSHAKE,
      mainHandshakeCallback, &mainData);
  progstateSetCallback (mainData.progstate, STATE_STOPPING,
      mainStoppingCallback, &mainData);
  progstateSetCallback (mainData.progstate, STATE_STOP_WAIT,
      mainStopWaitCallback, &mainData);
  progstateSetCallback (mainData.progstate, STATE_CLOSING,
      mainClosingCallback, &mainData);

  mainData.playlistCache = NULL;
  mainData.musicQueue = NULL;
  mainData.danceCounts = NULL;
  mainData.musicqPlayIdx = MUSICQ_A;
  mainData.musicqManageIdx = MUSICQ_A;
  mainData.musicqDeferredPlayIdx = MAIN_NOT_SET;
  mainData.playerState = PL_STATE_STOPPED;
  mainData.webclient = NULL;
  mainData.mobmqUserkey = NULL;
  mainData.playWhenQueued = true;
  mainData.switchQueueWhenEmpty = false;
  mainData.finished = false;
  mainData.marqueestarted = false;
  mainData.stopwaitcount = 0;
  for (musicqidx_t i = 0; i < MUSICQ_MAX; ++i) {
    mainData.playlistQueue [i] = NULL;
    mainData.musicqChanged [i] = false;
    mainData.marqueeChanged [i] = false;
  }
  procutilInitProcesses (mainData.processes);

  osSetStandardSignals (mainSigHandler);

  mainData.startflags = bdj4startup (argc, argv, &mainData.musicdb,
      "m", ROUTE_MAIN, BDJ4_INIT_NONE);
  logProcBegin (LOG_PROC, "main");

  if (bdjoptGetNum (OPT_P_MOBILEMARQUEE) == MOBILEMQ_INTERNET) {
    mainData.webclient = webclientAlloc (&mainData, mainMobilePostCallback);
  }

  mainData.conn = connInit (ROUTE_MAIN);
  mainData.gap = bdjoptGetNum (OPT_P_GAP);
  mainData.playlistCache = slistAlloc ("playlist-list", LIST_ORDERED,
      playlistFree);
  for (musicqidx_t i = 0; i < MUSICQ_MAX; ++i) {
    mainData.playlistQueue [i] = queueAlloc (NULL);
  }
  mainData.musicQueue = musicqAlloc ();
  mainDanceCountsInit (&mainData);
  mainData.announceList = slistAlloc ("announcements", LIST_ORDERED, NULL);

  listenPort = bdjvarsGetNum (BDJVL_MAIN_PORT);
  sockhMainLoop (listenPort, mainProcessMsg, mainProcessing, &mainData);
  connFree (mainData.conn);
  progstateFree (mainData.progstate);
  logProcEnd (LOG_PROC, "main", "");
  logEnd ();
  return 0;
}

/* internal routines */

static bool
mainStoppingCallback (void *tmaindata, programstate_t programState)
{
  maindata_t    *mainData = tmaindata;

  logProcBegin (LOG_PROC, "mainStoppingCallback");

  procutilStopAllProcess (mainData->processes, mainData->conn, false);
  connDisconnect (mainData->conn, ROUTE_STARTERUI);
  logProcEnd (LOG_PROC, "mainStoppingCallback", "");
  return true;
}

static bool
mainStopWaitCallback (void *tmaindata, programstate_t programState)
{
  maindata_t  *mainData = tmaindata;
  bool        rc = false;

  logProcBegin (LOG_PROC, "mainStopWaitCallback");

  rc = connCheckAll (mainData->conn);
  if (rc == false) {
    ++mainData->stopwaitcount;
    if (mainData->stopwaitcount > STOP_WAIT_COUNT_MAX) {
      rc = true;
    }
  }

  if (rc) {
    connDisconnectAll (mainData->conn);
  }

  logProcEnd (LOG_PROC, "mainStopWaitCallback", "");
  return rc;
}

static bool
mainClosingCallback (void *tmaindata, programstate_t programState)
{
  maindata_t    *mainData = tmaindata;
  char          *script;

  logProcBegin (LOG_PROC, "mainClosingCallback");

  if (mainData->playlistCache != NULL) {
    slistFree (mainData->playlistCache);
  }
  for (musicqidx_t i = 0; i < MUSICQ_MAX; ++i) {
    if (mainData->playlistQueue [i] != NULL) {
      queueFree (mainData->playlistQueue [i]);
    }
  }
  if (mainData->musicQueue != NULL) {
    musicqFree (mainData->musicQueue);
  }
  if (mainData->announceList != NULL) {
    slistFree (mainData->announceList);
  }
  if (mainData->webclient != NULL) {
    webclientClose (mainData->webclient);
  }
  if (mainData->mobmqUserkey != NULL) {
    free (mainData->mobmqUserkey);
  }
  if (mainData->danceCounts != NULL) {
    nlistFree (mainData->danceCounts);
  }

  procutilStopAllProcess (mainData->processes, mainData->conn, true);
  procutilFreeAll (mainData->processes);

  script = bdjoptGetStr (OPT_M_SHUTDOWNSCRIPT);
  if (script != NULL &&
      *script &&
      fileopFileExists (script)) {
    char  *targv [2];

    targv [0] = script;
    targv [1] = NULL;
    osProcessStart (targv, OS_PROC_DETACH, NULL, NULL);
  }

  bdj4shutdown (ROUTE_MAIN, mainData->musicdb);

  logProcEnd (LOG_PROC, "mainClosingCallback", "");
  return true;
}

static int
mainProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  maindata_t      *mainData;

  mainData = (maindata_t *) udata;

  /* this just reduces the amount of stuff in the log */
  if (msg != MSG_MUSICQ_STATUS_DATA && msg != MSG_PLAYER_STATUS_DATA) {
    logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
        routefrom, msgRouteDebugText (routefrom),
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
  }

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MAIN: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (mainData->conn, routefrom);
          break;
        }
        case MSG_SOCKET_CLOSE: {
          procutilCloseProcess (mainData->processes [routefrom],
              mainData->conn, routefrom);
          procutilFreeRoute (mainData->processes, routefrom);
          connDisconnect (mainData->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          progstateShutdownProcess (mainData->progstate);
          break;
        }
        case MSG_PLAYLIST_CLEARPLAY: {
          mainQueueClear (mainData, args);
          /* clear out any playing song */
          connSendMessage (mainData->conn, ROUTE_PLAYER,
              MSG_PLAY_NEXTSONG, NULL);
          mainQueuePlaylist (mainData, args);
          break;
        }
        case MSG_QUEUE_CLEAR: {
          /* clears both the playlist queue and the music queue */
          logMsg (LOG_DBG, LOG_MSGS, "got: queue-clear");
          mainQueueClear (mainData, args);
          break;
        }
        case MSG_QUEUE_PLAYLIST: {
          logMsg (LOG_DBG, LOG_MSGS, "got: playlist-queue %s", args);
          mainQueuePlaylist (mainData, args);
          break;
        }
        case MSG_QUEUE_DANCE: {
          mainQueueDance (mainData, args, 1);
          break;
        }
        case MSG_QUEUE_DANCE_5: {
          mainQueueDance (mainData, args, 5);
          break;
        }
        case MSG_QUEUE_PLAY_ON_ADD: {
          mainData->playWhenQueued = atoi (args);
          break;
        }
        case MSG_QUEUE_SWITCH_EMPTY: {
          mainData->switchQueueWhenEmpty = atoi (args);
          break;
        }
        case MSG_PLAY_PLAY: {
          mainMusicQueuePlay (mainData);
          break;
        }
        case MSG_PLAY_PLAYPAUSE: {
          if (mainData->playerState == PL_STATE_PLAYING ||
              mainData->playerState == PL_STATE_IN_FADEOUT) {
            connSendMessage (mainData->conn, ROUTE_PLAYER,
                MSG_PLAY_PAUSE, NULL);
          } else {
            mainMusicQueuePlay (mainData);
          }
          break;
        }
        case MSG_PLAYBACK_BEGIN: {
          /* do any begin song processing */
          mainPlaybackBegin (mainData);
          break;
        }
        case MSG_PLAYBACK_STOP: {
          mainMusicQueueFinish (mainData);
          break;
        }
        case MSG_PLAYBACK_FINISH: {
          mainMusicQueueNext (mainData);
          break;
        }
        case MSG_MUSICQ_TOGGLE_PAUSE: {
          mainTogglePause (mainData, args);
          break;
        }
        case MSG_MUSICQ_MOVE_DOWN: {
          mainMusicqMove (mainData, args, MOVE_DOWN);
          break;
        }
        case MSG_MUSICQ_MOVE_TOP: {
          mainMusicqMoveTop (mainData, args);
          break;
        }
        case MSG_MUSICQ_MOVE_UP: {
          mainMusicqMove (mainData, args, MOVE_UP);
          break;
        }
        case MSG_MUSICQ_REMOVE: {
          mainMusicqRemove (mainData, args);
          break;
        }
        case MSG_MUSICQ_TRUNCATE: {
          mainMusicqClear (mainData, args);
          break;
        }
        case MSG_MUSICQ_INSERT: {
          mainMusicqInsert (mainData, routefrom, args);
          break;
        }
        case MSG_MUSICQ_SET_PLAYBACK: {
          mainMusicqSetPlayback (mainData, args);
          break;
        }
        case MSG_PLAYER_STATE: {
          mainData->playerState = (playerstate_t) atol (args);
          logMsg (LOG_DBG, LOG_MSGS, "got: player-state: %d", mainData->playerState);
          mainData->marqueeChanged [mainData->musicqPlayIdx] = true;
          break;
        }
        case MSG_GET_DANCE_LIST: {
          mainSendDanceList (mainData, routefrom);
          break;
        }
        case MSG_GET_PLAYLIST_LIST: {
          mainSendPlaylistList (mainData, routefrom);
          break;
        }
        case MSG_PLAYER_STATUS_DATA: {
          mainSendPlayerStatus (mainData, args);
          break;
        }
        case MSG_START_MARQUEE: {
          mainStartMarquee (mainData);
          break;
        }
        case MSG_DATABASE_UPDATE: {
          mainData->musicdb = bdj4ReloadDatabase (mainData->musicdb);
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
mainProcessing (void *udata)
{
  maindata_t  *mainData = udata;
  int         stop = false;

  if (! progstateIsRunning (mainData->progstate)) {
    progstateProcess (mainData->progstate);
    if (progstateCurrState (mainData->progstate) == STATE_CLOSED) {
      stop = true;
    }
    if (gKillReceived) {
      progstateShutdownProcess (mainData->progstate);
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    }
    return stop;
  }

  connProcessUnconnected (mainData->conn);

  if (mainData->processes [ROUTE_MARQUEE] != NULL &&
      ! connIsConnected (mainData->conn, ROUTE_MARQUEE)) {
    connConnect (mainData->conn, ROUTE_MARQUEE);
  }

  for (int i = 0; i < MUSICQ_MAX; ++i) {
    if (mainData->musicqChanged [i]) {
      mainSendMusicQueueData (mainData, i);
      mainData->musicqChanged [i] = false;
    }
  }

  /* for the marquee, only the currently selected playback queue is */
  /* of interest */
  if (mainData->marqueeChanged [mainData->musicqPlayIdx]) {
    mainSendMarqueeData (mainData);
    mainSendMobileMarqueeData (mainData);
    mainData->marqueeChanged [mainData->musicqPlayIdx] = false;
  }

  if (gKillReceived) {
    progstateShutdownProcess (mainData->progstate);
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  return stop;
}

static bool
mainListeningCallback (void *tmaindata, programstate_t programState)
{
  maindata_t    *mainData = tmaindata;
  int           flags;

  logProcBegin (LOG_PROC, "mainListeningCallback");

  flags = PROCUTIL_DETACH;
  if ((mainData->startflags & BDJ4_INIT_NO_DETACH) == BDJ4_INIT_NO_DETACH) {
    flags = PROCUTIL_NO_DETACH;
  }

  if ((mainData->startflags & BDJ4_INIT_NO_START) != BDJ4_INIT_NO_START) {
    char          *script;

    script = bdjoptGetStr (OPT_M_STARTUPSCRIPT);
    if (script != NULL &&
        *script &&
        fileopFileExists (script)) {
      char  *targv [2];

      targv [0] = script;
      targv [1] = NULL;
      osProcessStart (targv, OS_PROC_DETACH, NULL, NULL);
    }

    mainData->processes [ROUTE_PLAYER] = procutilStartProcess (
        ROUTE_PLAYER, "bdj4player", flags, NULL);
    if (bdjoptGetNum (OPT_P_MOBILEMARQUEE) != MOBILEMQ_OFF) {
      mainData->processes [ROUTE_MOBILEMQ] = procutilStartProcess (
          ROUTE_MOBILEMQ, "bdj4mobmq", flags, NULL);
    }
    if (bdjoptGetNum (OPT_P_REMOTECONTROL)) {
      mainData->processes [ROUTE_REMCTRL] = procutilStartProcess (
          ROUTE_REMCTRL, "bdj4rc", flags, NULL);
    }
  }

  if ((mainData->startflags & BDJ4_INIT_NO_START) != BDJ4_INIT_NO_START &&
      (mainData->startflags & BDJ4_INIT_NO_MARQUEE) != BDJ4_INIT_NO_MARQUEE) {
    mainStartMarquee (mainData);
  }

  logProcEnd (LOG_PROC, "mainListeningCallback", "");
  return true;
}

static bool
mainConnectingCallback (void *tmaindata, programstate_t programState)
{
  maindata_t    *mainData = tmaindata;
  int           connMax = 0;
  int           connCount = 0;
  bool          rc = false;

  logProcBegin (LOG_PROC, "mainConnectingCallback");

  connProcessUnconnected (mainData->conn);

  if ((mainData->startflags & BDJ4_INIT_NO_START) != BDJ4_INIT_NO_START) {
    if (! connIsConnected (mainData->conn, ROUTE_PLAYER)) {
      connConnect (mainData->conn, ROUTE_PLAYER);
    }
    if (bdjoptGetNum (OPT_P_MOBILEMARQUEE) != MOBILEMQ_OFF) {
      if (! connIsConnected (mainData->conn, ROUTE_MOBILEMQ)) {
        connConnect (mainData->conn, ROUTE_MOBILEMQ);
      }
    }
    if (bdjoptGetNum (OPT_P_REMOTECONTROL)) {
      if (! connIsConnected (mainData->conn, ROUTE_REMCTRL)) {
        connConnect (mainData->conn, ROUTE_REMCTRL);
      }
    }
    if ((mainData->startflags & BDJ4_INIT_NO_MARQUEE) != BDJ4_INIT_NO_MARQUEE) {
      if (! connIsConnected (mainData->conn, ROUTE_MARQUEE)) {
        connConnect (mainData->conn, ROUTE_MARQUEE);
      }
    }
  }

  ++connMax;
  if (connIsConnected (mainData->conn, ROUTE_PLAYER)) {
    ++connCount;
  }
  if (bdjoptGetNum (OPT_P_MOBILEMARQUEE) != MOBILEMQ_OFF) {
    ++connMax;
    if (connIsConnected (mainData->conn, ROUTE_MOBILEMQ)) {
      ++connCount;
    }
  }
  if (bdjoptGetNum (OPT_P_REMOTECONTROL)) {
    ++connMax;
    if (connIsConnected (mainData->conn, ROUTE_REMCTRL)) {
      ++connCount;
    }
  }
  if ((mainData->startflags & BDJ4_INIT_NO_MARQUEE) != BDJ4_INIT_NO_MARQUEE) {
    ++connMax;
    if (connIsConnected (mainData->conn, ROUTE_MARQUEE)) {
      ++connCount;
    }
  }

  if (connCount == connMax) {
    rc = true;
  }

  logProcEnd (LOG_PROC, "mainConnectingCallback", "");
  return rc;
}

static bool
mainHandshakeCallback (void *tmaindata, programstate_t programState)
{
  maindata_t    *mainData = tmaindata;
  bool          rc = false;
  int           conn = 0;

  logProcBegin (LOG_PROC, "mainHandshakeCallback");

  connProcessUnconnected (mainData->conn);

  if (connHaveHandshake (mainData->conn, ROUTE_STARTERUI)) {
    ++conn;
  }
  if (connHaveHandshake (mainData->conn, ROUTE_PLAYER)) {
    ++conn;
  }
  if (connHaveHandshake (mainData->conn, ROUTE_PLAYERUI) ||
      connHaveHandshake (mainData->conn, ROUTE_MANAGEUI)) {
    ++conn;
  }
  /* main must have a connection to the player, starterui, and */
  /* one of manageui and playerui */
  if (conn == 3) {
    connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_MAIN_READY, NULL);
    rc = true;
  }

  logProcEnd (LOG_PROC, "mainHandshakeCallback", "");
  return rc;
}

static void
mainStartMarquee (maindata_t *mainData)
{
  char  *theme;
  char  *targv [2];
  int   idx = 0;
  int           flags;

  if (mainData->marqueestarted) {
    return;
  }

  /* set the GTK theme for the marquee */
  theme = bdjoptGetStr (OPT_MP_MQ_THEME);
  osSetEnv ("GTK_THEME", theme);

  if ((mainData->startflags & BDJ4_INIT_HIDE_MARQUEE) == BDJ4_INIT_HIDE_MARQUEE) {
    targv [idx++] = "--hidemarquee";
  }
  targv [idx++] = NULL;

  flags = PROCUTIL_DETACH;
  if ((mainData->startflags & BDJ4_INIT_NO_DETACH) == BDJ4_INIT_NO_DETACH) {
    flags = PROCUTIL_NO_DETACH;
  }

  mainData->processes [ROUTE_MARQUEE] = procutilStartProcess (
      ROUTE_MARQUEE, "bdj4marquee", flags, targv);
  mainData->marqueestarted = true;
}


static void
mainSendMusicQueueData (maindata_t *mainData, int musicqidx)
{
  char        tbuff [200];
  char        sbuff [4100];
  ssize_t     musicqLen;
  slistidx_t  dbidx;
  song_t      *song;
  int         flags;
  int         pflag;
  int         dispidx;
  int         uniqueidx;

  logProcBegin (LOG_PROC, "mainSendMusicQueueData");

  musicqLen = musicqGetLen (mainData->musicQueue, musicqidx);

  sbuff [0] = '\0';
  snprintf (sbuff, sizeof (sbuff), "%d%c", musicqidx, MSG_ARGS_RS);

  for (ssize_t i = 1; i <= musicqLen; ++i) {
    song = musicqGetByIdx (mainData->musicQueue, musicqidx, i);
    if (song != NULL) {
      dispidx = musicqGetDispIdx (mainData->musicQueue, musicqidx, i);
      snprintf (tbuff, sizeof (tbuff), "%d%c", dispidx, MSG_ARGS_RS);
      strlcat (sbuff, tbuff, sizeof (sbuff));
      uniqueidx = musicqGetUniqueIdx (mainData->musicQueue, musicqidx, i);
      snprintf (tbuff, sizeof (tbuff), "%d%c", uniqueidx, MSG_ARGS_RS);
      strlcat (sbuff, tbuff, sizeof (sbuff));
      dbidx = songGetNum (song, TAG_DBIDX);
      snprintf (tbuff, sizeof (tbuff), "%d%c", dbidx, MSG_ARGS_RS);
      strlcat (sbuff, tbuff, sizeof (sbuff));
      flags = musicqGetFlags (mainData->musicQueue, musicqidx, i);
      pflag = false;
      if ((flags & MUSICQ_FLAG_PAUSE) == MUSICQ_FLAG_PAUSE) {
        pflag = true;
      }
      snprintf (tbuff, sizeof (tbuff), "%d%c", pflag, MSG_ARGS_RS);
      strlcat (sbuff, tbuff, sizeof (sbuff));
    }
  }

  connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_MUSIC_QUEUE_DATA, sbuff);

  /* if the playerui is active, don't send the musicq data to the manageui. */
  /* the data would overwrite the song list. should not be running both. */
  if (! connHaveHandshake (mainData->conn, ROUTE_PLAYERUI)) {
    connSendMessage (mainData->conn, ROUTE_MANAGEUI, MSG_MUSIC_QUEUE_DATA, sbuff);
  }
  logProcEnd (LOG_PROC, "mainSendMusicQueueData", "");
}

static void
mainSendMarqueeData (maindata_t *mainData)
{
  char        tbuff [200];
  char        sbuff [3096];
  char        *dstr;
  char        *tstr;
  ssize_t     mqLen;
  ssize_t     musicqLen;

  logProcBegin (LOG_PROC, "mainSendMarqueeData");

  if (mainData->playerState == PL_STATE_STOPPED &&
      mainData->finished) {
    mainSendFinished (mainData);
    mainData->finished = false;
    return;
  }

  if (! mainData->marqueestarted) {
    logProcEnd (LOG_PROC, "mainSendMarqueeData", "not-started");
    return;
  }

  mqLen = bdjoptGetNum (OPT_P_MQQLEN);
  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);

  /* artist/title, dance(s) */

  sbuff [0] = '\0';

  dstr = musicqGetData (mainData->musicQueue, mainData->musicqPlayIdx, 0, TAG_ARTIST);
  if (dstr == NULL || *dstr == '\0') { dstr = MSG_ARGS_EMPTY_STR; }
  tstr = musicqGetData (mainData->musicQueue, mainData->musicqPlayIdx, 0, TAG_TITLE);
  if (tstr == NULL || *tstr == '\0') { tstr = MSG_ARGS_EMPTY_STR; }
  snprintf (tbuff, sizeof (tbuff), "%s%c%s%c", dstr, MSG_ARGS_RS, tstr, MSG_ARGS_RS);
  strlcat (sbuff, tbuff, sizeof (sbuff));

  if (musicqLen > 0) {
    for (ssize_t i = 0; i <= mqLen; ++i) {
      if ((i > 0 && mainData->playerState == PL_STATE_IN_GAP) ||
          (i > 1 && mainData->playerState == PL_STATE_IN_FADEOUT)) {
        dstr = MSG_ARGS_EMPTY_STR;
      } else if (i > musicqLen) {
        dstr = MSG_ARGS_EMPTY_STR;
      } else {
        dstr = mainSongGetDanceDisplay (mainData, i);
      }

      snprintf (tbuff, sizeof (tbuff), "%s%c", dstr, MSG_ARGS_RS);
      strlcat (sbuff, tbuff, sizeof (sbuff));
    }
  }

  connSendMessage (mainData->conn, ROUTE_MARQUEE,
      MSG_MARQUEE_DATA, sbuff);

  logProcEnd (LOG_PROC, "mainSendMarqueeData", "");
}

static char *
mainSongGetDanceDisplay (maindata_t *mainData, ssize_t idx)
{
  char      *tstr;
  char      *dstr;
  song_t    *song;

  logProcBegin (LOG_PROC, "mainSongGetDanceDisplay");

  tstr = NULL;
  song = musicqGetByIdx (mainData->musicQueue, mainData->musicqPlayIdx, idx);
  if (song != NULL) {
    /* if the song has an unknown dance, the marquee display */
    /* will be filled in with the dance name. */
    tstr = songGetStr (song, TAG_MQDISPLAY);
  }
  if (tstr != NULL && *tstr) {
    dstr = tstr;
  } else {
    dstr = musicqGetDance (mainData->musicQueue, mainData->musicqPlayIdx, idx);
  }
  if (dstr == NULL) {
    dstr = MSG_ARGS_EMPTY_STR;
  }

  logProcEnd (LOG_PROC, "mainSongGetDanceDisplay", "");
  return dstr;
}


static void
mainSendMobileMarqueeData (maindata_t *mainData)
{
  char        tbuff [200];
  char        tbuffb [200];
  char        qbuff [4096];
  char        jbuff [4096];
  char        *title = NULL;
  char        *dstr = NULL;
  char        *tag = NULL;
  ssize_t     mqLen = 0;
  ssize_t     musicqLen = 0;

  logProcBegin (LOG_PROC, "mainSendMobileMarqueeData");

  if (bdjoptGetNum (OPT_P_MOBILEMARQUEE) == MOBILEMQ_OFF) {
    logProcEnd (LOG_PROC, "mainSendMobileMarqueeData", "is-off");
    return;
  }

  mqLen = bdjoptGetNum (OPT_P_MQQLEN);
  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);
  title = bdjoptGetStr (OPT_P_MOBILEMQTITLE);
  if (title == NULL) {
    title = "";
  }

  strlcpy (jbuff, "{ ", sizeof (jbuff));

  snprintf (tbuff, sizeof (tbuff), "\"mqlen\" : \"%zd\", ", mqLen);
  strlcat (jbuff, tbuff, sizeof (jbuff));
  snprintf (tbuff, sizeof (tbuff), "\"title\" : \"%s\"", title);
  strlcat (jbuff, tbuff, sizeof (jbuff));

  if (musicqLen > 0) {
    for (ssize_t i = 0; i <= mqLen; ++i) {
      if ((i > 0 && mainData->playerState == PL_STATE_IN_GAP) ||
          (i > 1 && mainData->playerState == PL_STATE_IN_FADEOUT)) {
        dstr = "";
      } else if (i > musicqLen) {
        dstr = "";
      } else {
        dstr = mainSongGetDanceDisplay (mainData, i);
        if (dstr == NULL) {
          dstr = "";
        }
      }

      if (i == 0) {
        snprintf (tbuff, sizeof (tbuff), "\"current\" : \"%s\"", dstr);
      } else {
        snprintf (tbuff, sizeof (tbuff), "\"mq%zd\" : \"%s\"", i, dstr);
      }
      strlcat (jbuff, ", ", sizeof (jbuff));
      strlcat (jbuff, tbuff, sizeof (jbuff));
    }
  } else {
    strlcat (jbuff, ", ", sizeof (jbuff));
    strlcat (jbuff, "\"skip\" : \"true\"", sizeof (jbuff));
  }
  strlcat (jbuff, " }", sizeof (jbuff));

  connSendMessage (mainData->conn, ROUTE_MOBILEMQ, MSG_MARQUEE_DATA, jbuff);

  if (bdjoptGetNum (OPT_P_MOBILEMARQUEE) == MOBILEMQ_LOCAL) {
    logProcEnd (LOG_PROC, "mainSendMobileMarqueeData", "is-local");
    return;
  }

  /* internet mode from here on */

  tag = bdjoptGetStr (OPT_P_MOBILEMQTAG);
  if (tag != NULL && *tag != '\0') {
    if (mainData->mobmqUserkey == NULL) {
      pathbldMakePath (tbuff, sizeof (tbuff),
          "mmq", ".key", PATHBLD_MP_USEIDX);
      mainData->mobmqUserkey = filedataReadAll (tbuff, NULL);
    }

    snprintf (tbuff, sizeof (tbuff), "%s/%s",
        sysvarsGetStr (SV_MOBMQ_HOST), sysvarsGetStr (SV_MOBMQ_POST_URI));
    snprintf (qbuff, sizeof (qbuff), "v=2&mqdata=%s&key=%s&tag=%s",
        jbuff, "93457645", tag);
    if (mainData->mobmqUserkey != NULL) {
      snprintf (tbuffb, sizeof (tbuffb), "&userkey=%s", mainData->mobmqUserkey);
      strlcat (qbuff, tbuffb, sizeof (qbuff));
    }
    webclientPost (mainData->webclient, tbuff, qbuff);
  }
  logProcEnd (LOG_PROC, "mainSendMobileMarqueeData", "");
}

static void
mainMobilePostCallback (void *userdata, char *resp, size_t len)
{
  maindata_t    *mainData = userdata;
  char          tbuff [MAXPATHLEN];

  logProcBegin (LOG_PROC, "mainMobilePostCallback");

  if (strncmp (resp, "OK", 2) == 0) {
    ;
  } else if (strncmp (resp, "NG", 2) == 0) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: unable to post mobmq data: %.*s", (int) len, resp);
  } else {
    FILE    *fh;

    /* this should be the user key */
    strlcpy (tbuff, resp, 3);
    tbuff [2] = '\0';
    mainData->mobmqUserkey = strdup (tbuff);
    /* need to save this for future use */
    pathbldMakePath (tbuff, sizeof (tbuff),
        "mmq", ".key", PATHBLD_MP_USEIDX);
    fh = fopen (tbuff, "w");
    fprintf (fh, "%.*s", (int) len, resp);
    fclose (fh);
  }
  logProcEnd (LOG_PROC, "mainMobilePostCallback", "");
}

/* clears both the playlist and music queues */
static void
mainQueueClear (maindata_t *mainData, char *args)
{
  int   mi;
  int   startpos = 0;
  char  *p;
  char  *tokstr = NULL;

  logProcBegin (LOG_PROC, "mainQueueClear");


  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  mi = atoi (p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  startpos = atoi (p);

  mainData->musicqManageIdx = mi;

  logMsg (LOG_DBG, LOG_BASIC, "clear music queue");
  queueClear (mainData->playlistQueue [mi], 0);
  musicqClear (mainData->musicQueue, mi, startpos);
  mainData->musicqChanged [mi] = true;
  mainData->marqueeChanged [mi] = true;
  logProcEnd (LOG_PROC, "mainQueueClear", "");
}

static void
mainQueueDance (maindata_t *mainData, char *args, ssize_t count)
{
  playlist_t      *playlist;
  char            plname [60];
  int             mi;
  ssize_t         danceIdx;

  logProcBegin (LOG_PROC, "mainQueueDance");

  mainParseIntNum (args, &mi, &danceIdx);
  mainData->musicqManageIdx = mi;

  logMsg (LOG_DBG, LOG_BASIC, "queue dance %d %zd %zd", mi, danceIdx, count);
  snprintf (plname, sizeof (plname), "_main_dance_%zd_%ld",
      danceIdx, globalCounter++);
  playlist = playlistAlloc (mainData->musicdb);
  playlistCreate (playlist, plname, PLTYPE_AUTO, NULL);
  playlistSetConfigNum (playlist, PLAYLIST_STOP_AFTER, count);
  /* this will also set 'selected' */
  playlistSetDanceCount (playlist, danceIdx, 1);
  logMsg (LOG_DBG, LOG_BASIC, "Queue Playlist: %s", plname);
  slistSetData (mainData->playlistCache, plname, playlist);
  queuePush (mainData->playlistQueue [mi], playlist);
  logMsg (LOG_DBG, LOG_MAIN, "push pl %s", plname);
  mainMusicQueueFill (mainData);
  mainMusicQueuePrep (mainData);
  mainData->marqueeChanged [mi] = true;
  mainData->musicqChanged [mi] = true;
  mainSendMusicqStatus (mainData, NULL, 0);
  if (mainData->playWhenQueued &&
      mainData->musicqPlayIdx == (musicqidx_t) mi) {
    mainMusicQueuePlay (mainData);
  }
  logProcEnd (LOG_PROC, "mainQueueDance", "");
}

static void
mainQueuePlaylist (maindata_t *mainData, char *args)
{
  int             mi;
  playlist_t      *playlist;
  char            *plname;
  int             rc;

  logProcBegin (LOG_PROC, "mainQueuePlaylist");

  mainParseIntStr (args, &mi, &plname);
  mainData->musicqManageIdx = mi;

  playlist = playlistAlloc (mainData->musicdb);
  rc = playlistLoad (playlist, plname);
  if (rc == 0) {
    logMsg (LOG_DBG, LOG_BASIC, "Queue Playlist: %d %s", mi, plname);
    slistSetData (mainData->playlistCache, plname, playlist);
    queuePush (mainData->playlistQueue [mainData->musicqManageIdx], playlist);
    logMsg (LOG_DBG, LOG_MAIN, "push pl %s", plname);
    mainMusicQueueFill (mainData);
    mainMusicQueuePrep (mainData);
    mainData->marqueeChanged [mi] = true;
    mainData->musicqChanged [mi] = true;

    mainSendMusicqStatus (mainData, NULL, 0);
    if (mainData->playWhenQueued &&
        mainData->musicqPlayIdx == (musicqidx_t) mi) {
      mainMusicQueuePlay (mainData);
    }
  } else {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Queue Playlist failed: %s", plname);
  }

  logProcEnd (LOG_PROC, "mainQueuePlaylist", "");
}

static void
mainSigHandler (int sig)
{
  gKillReceived = 1;
}

static void
mainMusicQueueFill (maindata_t *mainData)
{
  ssize_t     mqLen;
  ssize_t     currlen;
  playlist_t  *playlist = NULL;
  pltype_t    pltype;
  bool        stopatflag = false;

  logProcBegin (LOG_PROC, "mainMusicQueueFill");

  playlist = queueGetCurrent (mainData->playlistQueue [mainData->musicqManageIdx]);
  pltype = (pltype_t) playlistGetConfigNum (playlist, PLAYLIST_TYPE);

  mqLen = bdjoptGetNum (OPT_G_PLAYERQLEN);
  currlen = musicqGetLen (mainData->musicQueue, mainData->musicqManageIdx);

  /* if this is not the queue selected for playback, then push  */
  /* an empty musicq item on to the head of the queue.          */
  /* this allows the display to work properly for the user      */
  /* when multiple queues are in use.                           */

  if (currlen == 0 &&
      mainData->musicqPlayIdx != mainData->musicqManageIdx) {
    musicqPushHeadEmpty (mainData->musicQueue, mainData->musicqManageIdx);
    ++currlen;
  }

  logMsg (LOG_DBG, LOG_BASIC, "fill: %ld < %ld", currlen, mqLen);

  /* want current + mqLen songs */
  while (playlist != NULL && currlen <= mqLen && stopatflag == false) {
    ssize_t   stopTime;
    song_t    *song = NULL;

    song = playlistGetNextSong (playlist, mainData->danceCounts,
        currlen, mainCheckMusicQueue, mainMusicQueueHistory, mainData);
    if (song == NULL) {
      logMsg (LOG_DBG, LOG_MAIN, "song is null");
      queuePop (mainData->playlistQueue [mainData->musicqManageIdx]);
      playlist = queueGetCurrent (mainData->playlistQueue [mainData->musicqManageIdx]);
      continue;
    }
    logMsg (LOG_DBG, LOG_MAIN, "push song to musicq");
    musicqPush (mainData->musicQueue, mainData->musicqManageIdx, song, playlistGetName (playlist));
    mainData->musicqChanged [mainData->musicqManageIdx] = true;
    mainData->marqueeChanged [mainData->musicqManageIdx] = true;
    currlen = musicqGetLen (mainData->musicQueue, mainData->musicqManageIdx);

    if (pltype == PLTYPE_AUTO) {
      playlist = queueGetCurrent (mainData->playlistQueue [mainData->musicqManageIdx]);
      if (playlist != NULL && song != NULL) {
        playlistAddCount (playlist, song);
      }
    }

    stopTime = playlistGetConfigNum (playlist, PLAYLIST_STOP_TIME);
    if (stopTime > 0) {
      ssize_t       currTime;
      ssize_t       qDuration;
      ssize_t       nStopTime;

      currTime = mstime ();
      /* stop time is in hours+minutes; need to convert it to real time */
      nStopTime = mstimestartofday ();
      nStopTime += stopTime;
      if (nStopTime < currTime) {
        nStopTime += (24 * 3600 * 1000);
      }

      qDuration = musicqGetDuration (mainData->musicQueue, mainData->musicqManageIdx);
      if (currTime + qDuration > nStopTime) {
        stopatflag = true;
      }
    }
  }

  logProcEnd (LOG_PROC, "mainMusicQueueFill", "");
}

static void
mainMusicQueuePrep (maindata_t *mainData)
{
  playlist_t    *playlist;
  ssize_t       plannounce;

  logProcBegin (LOG_PROC, "mainMusicQueuePrep");

    /* 5 is the number of songs to prep ahead of time */
  for (ssize_t i = 0; i < PREP_SIZE; ++i) {
    char          *sfname = NULL;
    song_t        *song = NULL;
    musicqflag_t  flags;
    char          *plname = NULL;
    char          *annfname = NULL;

    song = musicqGetByIdx (mainData->musicQueue, mainData->musicqManageIdx, i);
    flags = musicqGetFlags (mainData->musicQueue, mainData->musicqManageIdx, i);
    plname = musicqGetPlaylistName (mainData->musicQueue, mainData->musicqManageIdx, i);

    if (song != NULL &&
        (flags & MUSICQ_FLAG_PREP) != MUSICQ_FLAG_PREP) {
      musicqSetFlag (mainData->musicQueue, mainData->musicqManageIdx,
          i, MUSICQ_FLAG_PREP);

      plannounce = false;
      if (plname != NULL) {
        playlist = slistGetData (mainData->playlistCache, plname);
        plannounce = playlistGetConfigNum (playlist, PLAYLIST_ANNOUNCE);
      }
      annfname = mainPrepSong (mainData, song, sfname, plname, PREP_SONG);

      if (plannounce == 1) {
        if (annfname != NULL && strcmp (annfname, "") != 0 ) {
          musicqSetFlag (mainData->musicQueue, mainData->musicqManageIdx,
              i, MUSICQ_FLAG_ANNOUNCE);
          musicqSetAnnounce (mainData->musicQueue, mainData->musicqManageIdx,
              i, annfname);
        }
      }
    }
  }
  logProcEnd (LOG_PROC, "mainMusicQueuePrep", "");
}


static char *
mainPrepSong (maindata_t *mainData, song_t *song,
    char *sfname, char *plname, bdjmsgprep_t flag)
{
  char          tbuff [MAXPATHLEN];
  playlist_t    *playlist = NULL;
  ssize_t       dur = 0;
  ssize_t       maxdur = -1;
  ssize_t       pldur = -1;
  ssize_t       plddur = -1;
  ssize_t       plgap = -1;
  ssize_t       songstart = 0;
  ssize_t       songend = -1;
  ssize_t       speed = 100;
  double        voladjperc = 0;
  ssize_t       gap = 0;
  ssize_t       plannounce = 0;
  ilistidx_t    danceidx;
  char          *annfname = NULL;

  logProcBegin (LOG_PROC, "mainPrepSong");

  sfname = songGetStr (song, TAG_FILE);
  dur = songGetNum (song, TAG_DURATION);
  voladjperc = songGetDouble (song, TAG_VOLUMEADJUSTPERC);
  if (voladjperc < 0.0) {
    voladjperc = 0.0;
  }
  gap = 0;
  songstart = 0;
  speed = 100;

    /* announcements don't need any of the following... */
  if (flag != PREP_ANNOUNCE) {
    maxdur = bdjoptGetNum (OPT_P_MAXPLAYTIME);
    songstart = songGetNum (song, TAG_SONGSTART);
    if (songstart < 0) {
      songstart = 0;
    }
    songend = songGetNum (song, TAG_SONGEND);
    if (songend < 0) {
      songend = 0;
    }
    speed = songGetNum (song, TAG_SPEEDADJUSTMENT);
    if (speed < 0) {
      speed = 100;
    }

    pldur = LIST_VALUE_INVALID;
    if (plname != NULL) {
      playlist = slistGetData (mainData->playlistCache, plname);
      pldur = playlistGetConfigNum (playlist, PLAYLIST_MAX_PLAY_TIME);
    }
      /* apply songend if set to a reasonable value */
    logMsg (LOG_DBG, LOG_MAIN, "dur: %zd songstart: %zd songend: %zd",
        dur, songstart, songend);
    if (songend >= 10000 && dur > songend) {
      dur = songend;
      logMsg (LOG_DBG, LOG_MAIN, "dur-songend: %zd", dur);
    }
    /* adjust the song's duration by the songstart value */
    if (songstart > 0) {
      dur -= songstart;
      logMsg (LOG_DBG, LOG_MAIN, "dur-songstart: %zd", dur);
    }

    /* after adjusting the duration by song start/end, then adjust */
    /* the duration by the speed of the song */
    /* this is the real duration for the song */
    if (speed != 100) {
      double      drate;
      double      ddur;

      drate = (double) speed / 100.0;
      ddur = (double) dur / drate;
      dur = (ssize_t) ddur;
    }

    /* if the playlist has a maximum play time specified for a dance */
    /* it overrides any of the other max play times */
    danceidx = songGetNum (song, TAG_DANCE);
    plddur = playlistGetDanceNum (playlist, danceidx, PLDANCE_MAXPLAYTIME);
    if (plddur >= 5000) {
      dur = plddur;
      logMsg (LOG_DBG, LOG_MAIN, "dur-pldur: %zd", dur);
    } else {
        /* the playlist max-play-time overrides the global max-play-time */
      if (pldur >= 5000) {
        if (dur > pldur) {
          dur = pldur;
          logMsg (LOG_DBG, LOG_MAIN, "dur-pldur: %zd", dur);
        }
      } else if (dur > maxdur) {
        dur = maxdur;
        logMsg (LOG_DBG, LOG_MAIN, "dur-maxdur: %zd", dur);
      }
    }

    gap = mainData->gap;
    plgap = playlistGetConfigNum (playlist, PLAYLIST_GAP);
    if (plgap >= 0) {
      gap = plgap;
    }

    plannounce = playlistGetConfigNum (playlist, PLAYLIST_ANNOUNCE);
    if (plannounce == 1) {
      dance_t       *dances;
      song_t        *tsong;

      danceidx = songGetNum (song, TAG_DANCE);
      dances = bdjvarsdfGet (BDJVDF_DANCES);
      annfname = danceGetStr (dances, danceidx, DANCE_ANNOUNCE);
      if (annfname != NULL) {
        ssize_t   tval;

        tsong = dbGetByName (mainData->musicdb, annfname);
        if (tsong != NULL) {
          tval = slistGetNum (mainData->announceList, annfname);
          if (tval == LIST_VALUE_INVALID) {
            /* only prep the announcement if it has not been prepped before */
            mainPrepSong (mainData, tsong, annfname, plname, PREP_ANNOUNCE);
          }
          slistSetNum (mainData->announceList, annfname, 1);
        } else {
          annfname = NULL;
        }
      } /* found an announcement for the dance */
    } /* announcements are on in the playlist */
  } /* if this is a normal song */

  snprintf (tbuff, sizeof (tbuff), "%s%c%zd%c%zd%c%zd%c%.1f%c%zd%c%d", sfname,
      MSG_ARGS_RS, dur, MSG_ARGS_RS, songstart, MSG_ARGS_RS, speed,
      MSG_ARGS_RS, voladjperc, MSG_ARGS_RS, gap, MSG_ARGS_RS, flag);

  logMsg (LOG_DBG, LOG_MAIN, "prep song %s", sfname);
  connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_SONG_PREP, tbuff);
  logProcEnd (LOG_PROC, "mainPrepSong", "");
  return annfname;
}

static void
mainTogglePause (maindata_t *mainData, char *args)
{
  int           mi;
  ssize_t       idx;
  ssize_t       musicqLen;
  musicqflag_t  flags;

  logProcBegin (LOG_PROC, "mainTogglePause");

  mainParseIntNum (args, &mi, &idx);
  mainData->musicqManageIdx = mi;

  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqManageIdx);
  if (idx <= 0 || idx > musicqLen) {
    logProcEnd (LOG_PROC, "mainTogglePause", "bad-idx");
    return;
  }

  flags = musicqGetFlags (mainData->musicQueue, mainData->musicqManageIdx, idx);
  if ((flags & MUSICQ_FLAG_PAUSE) == MUSICQ_FLAG_PAUSE) {
    musicqClearFlag (mainData->musicQueue, mainData->musicqManageIdx, idx, MUSICQ_FLAG_PAUSE);
  } else {
    musicqSetFlag (mainData->musicQueue, mainData->musicqManageIdx, idx, MUSICQ_FLAG_PAUSE);
  }
  mainData->musicqChanged [mi] = true;
  logProcEnd (LOG_PROC, "mainTogglePause", "");
}

static void
mainMusicqMove (maindata_t *mainData, char *args, mainmove_t direction)
{
  int           mi;
  ssize_t       fromidx;
  ssize_t       toidx;
  ssize_t       musicqLen;

  logProcBegin (LOG_PROC, "mainMusicqMove");


  mainParseIntNum (args, &mi, &fromidx);
  mainData->musicqManageIdx = mi;

  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqManageIdx);

  toidx = fromidx + direction;
  if (fromidx == 1 && toidx == 0 &&
      mainData->playerState != PL_STATE_STOPPED) {
    logProcEnd (LOG_PROC, "mainMusicqMove", "to-0-playing");
    return;
  }

  if (toidx > fromidx && fromidx >= musicqLen) {
    logProcEnd (LOG_PROC, "mainMusicqMove", "bad-move");
    return;
  }
  if (toidx < fromidx && fromidx < 1) {
    logProcEnd (LOG_PROC, "mainMusicqMove", "bad-move");
    return;
  }

  musicqMove (mainData->musicQueue, mainData->musicqManageIdx, fromidx, toidx);
  mainData->musicqChanged [mi] = true;
  mainData->marqueeChanged [mi] = true;
  mainMusicQueuePrep (mainData);

  if (toidx == 0) {
    mainSendMusicqStatus (mainData, NULL, 0);
  }
  logProcEnd (LOG_PROC, "mainMusicqMove", "");
}

static void
mainMusicqMoveTop (maindata_t *mainData, char *args)
{
  int     mi;
  ssize_t fromidx;
  ssize_t toidx;
  ssize_t musicqLen;

  logProcBegin (LOG_PROC, "mainMusicqMoveTop");

  mainParseIntNum (args, &mi, &fromidx);
  mainData->musicqManageIdx = mi;

  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqManageIdx);

  if (fromidx >= musicqLen || fromidx <= 1) {
    logProcEnd (LOG_PROC, "mainMusicqMoveTop", "bad-move");
    return;
  }

  toidx = fromidx - 1;
  while (toidx != 0) {
    musicqMove (mainData->musicQueue, mainData->musicqManageIdx, fromidx, toidx);
    fromidx--;
    toidx--;
  }
  mainData->musicqChanged [mi] = true;
  mainData->marqueeChanged [mi] = true;
  mainMusicQueuePrep (mainData);
  logProcEnd (LOG_PROC, "mainMusicqMoveTop", "");
}

static void
mainMusicqClear (maindata_t *mainData, char *args)
{
  int     mi;
  ssize_t idx;

  logProcBegin (LOG_PROC, "mainMusicqClear");

  mainParseIntNum (args, &mi, &idx);
  mainData->musicqManageIdx = mi;


  musicqClear (mainData->musicQueue, mainData->musicqManageIdx, idx);
  /* there may be other playlists in the playlist queue */
  mainMusicQueueFill (mainData);
  mainMusicQueuePrep (mainData);
  mainData->musicqChanged [mi] = true;
  mainData->marqueeChanged [mi] = true;
  logProcEnd (LOG_PROC, "mainMusicqClear", "");
}

static void
mainMusicqRemove (maindata_t *mainData, char *args)
{
  int     mi;
  ssize_t idx;

  logProcBegin (LOG_PROC, "mainMusicqRemove");


  mainParseIntNum (args, &mi, &idx);
  mainData->musicqManageIdx = mi;

  musicqRemove (mainData->musicQueue, mainData->musicqManageIdx, idx);
  mainMusicQueueFill (mainData);
  mainMusicQueuePrep (mainData);
  mainData->musicqChanged [mi] = true;
  mainData->marqueeChanged [mi] = true;
  logProcEnd (LOG_PROC, "mainMusicqRemove", "");
}

static void
mainMusicqInsert (maindata_t *mainData, bdjmsgroute_t routefrom, char *args)
{
  int       mi;
  char      *tokstr = NULL;
  char      *p = NULL;
  ssize_t   idx;
  ssize_t   loc;
  dbidx_t   dbidx;
  song_t    *song = NULL;
  char      tbuff [40];

  logProcBegin (LOG_PROC, "mainMusicqInsert");

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  mi = atoi (p);
  if (mi != MUSICQ_CURRENT) {
    mainData->musicqManageIdx = mi;
  }
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  idx = atol (p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  dbidx = atol (p);

  song = dbGetByIdx (mainData->musicdb, dbidx);

  if (song != NULL) {
    loc = musicqInsert (mainData->musicQueue, mainData->musicqManageIdx, idx, song);
    mainData->musicqChanged [mainData->musicqManageIdx] = true;
    mainData->marqueeChanged [mainData->musicqManageIdx] = true;
    mainMusicQueuePrep (mainData);
    mainSendMusicqStatus (mainData, NULL, 0);
    if (mainData->playWhenQueued &&
        mainData->musicqPlayIdx == (musicqidx_t) mainData->musicqManageIdx) {
      mainMusicQueuePlay (mainData);
    }
    if (loc > 0) {
      snprintf (tbuff, sizeof (tbuff), "%zd", loc);
      connSendMessage (mainData->conn, routefrom, MSG_SONG_SELECT, tbuff);
    }
  }
  logProcEnd (LOG_PROC, "mainMusicqInsert", "");
}

static void
mainMusicqSetPlayback (maindata_t *mainData, char *args)
{
  int         qidx;

  logProcBegin (LOG_PROC, "mainMusicqSetPlayback");

  qidx = atoi (args);
  if (qidx < 0 || qidx >= MUSICQ_MAX) {
    logProcEnd (LOG_PROC, "mainMusicqSetPlayback", "bad-idx");
    return;
  }

  if ((musicqidx_t) qidx == mainData->musicqPlayIdx) {
    mainData->musicqDeferredPlayIdx = MAIN_NOT_SET;
  } else if (mainData->playerState != PL_STATE_STOPPED) {
    /* if the player is playing something, the musicq index cannot */
    /* be changed as yet */
    mainData->musicqDeferredPlayIdx = qidx;
  } else {
    mainMusicqSwitch (mainData, qidx);
  }
  logProcEnd (LOG_PROC, "mainMusicqSetPlayback", "");
}

static void
mainMusicqSwitch (maindata_t *mainData, musicqidx_t newMusicqIdx)
{
  song_t        *song;

  logProcBegin (LOG_PROC, "mainMusicqSwitch");


  mainData->musicqManageIdx = mainData->musicqPlayIdx;
  mainData->musicqPlayIdx = newMusicqIdx;
  /* manage idx is pointing to the previous musicq play idx */
  musicqPushHeadEmpty (mainData->musicQueue, mainData->musicqManageIdx);
  /* the prior musicq has changed */
  mainData->musicqChanged [mainData->musicqManageIdx] = true;

  mainData->musicqManageIdx = mainData->musicqPlayIdx;
  mainData->musicqDeferredPlayIdx = MAIN_NOT_SET;

  song = musicqGetCurrent (mainData->musicQueue, mainData->musicqPlayIdx);
  if (song == NULL) {
    musicqPop (mainData->musicQueue, mainData->musicqPlayIdx);
  }

  /* and the new musicq has changed */
  mainSendMusicqStatus (mainData, NULL, 0);
  mainData->musicqChanged [mainData->musicqPlayIdx] = true;
  mainData->marqueeChanged [mainData->musicqPlayIdx] = true;
  logProcEnd (LOG_PROC, "mainMusicqSwitch", "");
}

static void
mainPlaybackBegin (maindata_t *mainData)
{
  musicqflag_t  flags;

  logProcBegin (LOG_PROC, "mainPlaybackBegin");

  /* if the pause flag is on for this entry in the music queue, */
  /* tell the player to turn on the pause-at-end */
  flags = musicqGetFlags (mainData->musicQueue, mainData->musicqPlayIdx, 0);
  if ((flags & MUSICQ_FLAG_PAUSE) == MUSICQ_FLAG_PAUSE) {
    connSendMessage (mainData->conn, ROUTE_PLAYER,
        MSG_PLAY_PAUSEATEND, NULL);
  }
  logProcEnd (LOG_PROC, "mainPlaybackBegin", "");
}

static void
mainMusicQueuePlay (maindata_t *mainData)
{
  musicqflag_t      flags;
  char              *sfname;
  song_t            *song;
  ssize_t           currlen;
  musicqidx_t       origMusicqPlayIdx;

  logProcBegin (LOG_PROC, "mainMusicQueuePlay");

  if (mainData->musicqDeferredPlayIdx != MAIN_NOT_SET) {
    mainMusicqSwitch (mainData, mainData->musicqDeferredPlayIdx);
    mainData->musicqDeferredPlayIdx = MAIN_NOT_SET;
  }

  mainData->musicqManageIdx = mainData->musicqPlayIdx;

  logMsg (LOG_DBG, LOG_BASIC, "playerState: %d", mainData->playerState);

  /* grab a song out of the music queue and start playing */
  logMsg (LOG_DBG, LOG_MAIN, "player is stopped/in-fadeout/in-gap, get song, start");
  song = musicqGetCurrent (mainData->musicQueue, mainData->musicqPlayIdx);
  if (song != NULL) {
    flags = musicqGetFlags (mainData->musicQueue, mainData->musicqPlayIdx, 0);
    if ((flags & MUSICQ_FLAG_ANNOUNCE) == MUSICQ_FLAG_ANNOUNCE) {
      char      *annfname;

      annfname = musicqGetAnnounce (mainData->musicQueue, mainData->musicqPlayIdx, 0);
      if (annfname != NULL) {
        connSendMessage (mainData->conn, ROUTE_PLAYER,
            MSG_SONG_PLAY, annfname);
      }
    }
    sfname = songGetStr (song, TAG_FILE);
    connSendMessage (mainData->conn, ROUTE_PLAYER,
        MSG_SONG_PLAY, sfname);
  }

  currlen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);
  origMusicqPlayIdx = mainData->musicqPlayIdx;

  if (song == NULL && currlen == 0) {
    if (mainData->switchQueueWhenEmpty) {
      char    tmp [40];

      mainData->musicqPlayIdx = musicqNextQueue (mainData->musicqPlayIdx);
      mainData->musicqManageIdx = mainData->musicqPlayIdx;
      currlen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);

      /* locate a queue that has songs in it */
      while (mainData->musicqPlayIdx != origMusicqPlayIdx &&
          currlen == 0) {
        mainData->musicqPlayIdx = musicqNextQueue (mainData->musicqPlayIdx);
        mainData->musicqManageIdx = mainData->musicqPlayIdx;
        currlen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);
      }

      if (currlen > 0) {
        snprintf (tmp, sizeof (tmp), "%d", mainData->musicqPlayIdx);
        connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_QUEUE_SWITCH, tmp);
        /* and start up playback for the new queue */
        mainMusicQueueNext (mainData);
        /* since the player state is stopped, must re-start playback */
        mainMusicQueuePlay (mainData);
      } else {
        /* there is no music to play; tell the player to clear its display */
        mainData->finished = true;
        mainData->marqueeChanged [mainData->musicqPlayIdx] = true;
      }
    } else {
      mainData->finished = true;
      mainData->marqueeChanged [mainData->musicqPlayIdx] = true;
    }
  } /* if the song was null and the queue is empty */

  /* this handles the user-selected play button when the song is paused */
  /* it no longer handles in-gap due to some changes */
  /* this might need to be moved to the message handler */
  if (mainData->playerState == PL_STATE_PAUSED) {
    logMsg (LOG_DBG, LOG_MAIN, "player is paused, send play msg");
    connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_PLAY_PLAY, NULL);
  }

  logProcEnd (LOG_PROC, "mainMusicQueuePlay", "");
}

static void
mainMusicQueueFinish (maindata_t *mainData)
{
  playlist_t    *playlist;
  song_t        *song;
  ilistidx_t    danceIdx;
  char          *plname;

  logProcBegin (LOG_PROC, "mainMusicQueueFinish");

  mainData->musicqManageIdx = mainData->musicqPlayIdx;

  /* let the playlist know this song has been played */
  song = musicqGetCurrent (mainData->musicQueue, mainData->musicqPlayIdx);
  plname = musicqGetPlaylistName (mainData->musicQueue, mainData->musicqPlayIdx, 0);
  if (plname != NULL) {
    playlist = slistGetData (mainData->playlistCache, plname);
    if (playlist != NULL && song != NULL) {
      playlistAddPlayed (playlist, song);
    }
  }
  /* update the dance counts */
  if (song != NULL) {
    danceIdx = songGetNum (song, TAG_DANCE);
    if (danceIdx >= 0) {
      nlistDecrement (mainData->danceCounts, danceIdx);
    }
  }

  musicqPop (mainData->musicQueue, mainData->musicqPlayIdx);
  mainData->musicqChanged [mainData->musicqPlayIdx] = true;
  mainData->marqueeChanged [mainData->musicqPlayIdx] = true;
  mainSendMusicqStatus (mainData, NULL, 0);
  logProcEnd (LOG_PROC, "mainMusicQueueFinish", "");
}

static void
mainMusicQueueNext (maindata_t *mainData)
{
  logProcBegin (LOG_PROC, "mainMusicQueueNext");

  mainData->musicqManageIdx = mainData->musicqPlayIdx;

  mainMusicQueueFinish (mainData);
  if (mainData->playerState != PL_STATE_STOPPED &&
      mainData->playerState != PL_STATE_PAUSED) {
    mainMusicQueuePlay (mainData);
  }
  mainMusicQueueFill (mainData);
  mainMusicQueuePrep (mainData);
  logProcEnd (LOG_PROC, "mainMusicQueueNext", "");
}

static bool
mainCheckMusicQueue (song_t *song, void *tdata)
{
//  maindata_t  *mainData = tdata;

  return true;
}

static ilistidx_t
mainMusicQueueHistory (void *tmaindata, ilistidx_t idx)
{
  maindata_t    *mainData = tmaindata;
  ilistidx_t    didx;
  song_t        *song;

  logProcBegin (LOG_PROC, "mainMusicQueueHistory");

  if (idx < 0 ||
      idx >= musicqGetLen (mainData->musicQueue, mainData->musicqManageIdx)) {
    logProcEnd (LOG_PROC, "mainMusicQueueHistory", "bad-idx");
    return -1;
  }

  song = musicqGetByIdx (mainData->musicQueue, mainData->musicqManageIdx, idx);
  didx = songGetNum (song, TAG_DANCE);
  logProcEnd (LOG_PROC, "mainMusicQueueHistory", "");
  return didx;
}

static void
mainSendDanceList (maindata_t *mainData, bdjmsgroute_t route)
{
  dance_t       *dances;
  slist_t       *danceList;
  slistidx_t    idx;
  char          *dancenm;
  char          tbuff [200];
  char          rbuff [3096];
  slistidx_t    iteridx;

  logProcBegin (LOG_PROC, "mainSendDanceList");

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  danceList = danceGetDanceList (dances);

  rbuff [0] = '\0';
  slistStartIterator (danceList, &iteridx);
  while ((dancenm = slistIterateKey (danceList, &iteridx)) != NULL) {
    idx = slistGetNum (danceList, dancenm);
    snprintf (tbuff, sizeof (tbuff), "%d%c%s%c",
        idx, MSG_ARGS_RS, dancenm, MSG_ARGS_RS);
    strlcat (rbuff, tbuff, sizeof (rbuff));
  }

  connSendMessage (mainData->conn, route, MSG_DANCE_LIST_DATA, rbuff);
  logProcEnd (LOG_PROC, "mainSendDanceList", "");
}

static void
mainSendPlaylistList (maindata_t *mainData, bdjmsgroute_t route)
{
  slist_t       *plList;
  char          *plfnm;
  char          *plnm;
  char          tbuff [200];
  char          rbuff [3096];
  slistidx_t    iteridx;

  logProcBegin (LOG_PROC, "mainSendPlaylistList");

  plList = playlistGetPlaylistList ();

  rbuff [0] = '\0';
  slistStartIterator (plList, &iteridx);
  while ((plnm = slistIterateKey (plList, &iteridx)) != NULL) {
    plfnm = slistGetStr (plList, plnm);
    snprintf (tbuff, sizeof (tbuff), "%s%c%s%c",
        plfnm, MSG_ARGS_RS, plnm, MSG_ARGS_RS);
    strlcat (rbuff, tbuff, sizeof (rbuff));
  }

  slistFree (plList);

  connSendMessage (mainData->conn, route, MSG_PLAYLIST_LIST_DATA, rbuff);
  logProcEnd (LOG_PROC, "mainSendPlaylistList", "");
}

static void
mainSendPlayerStatus (maindata_t *mainData, char *playerResp)
{
  char    tbuff [200];
  char    tbuff2 [40];
  char    rbuff [3096];
  char    timerbuff [1024];
  char    statusbuff [1024];
  char    *tokstr = NULL;
  char    *p;

  logProcBegin (LOG_PROC, "mainSendPlayerStatus");

  statusbuff [0] = '\0';

  strlcpy (rbuff, "{ ", sizeof (rbuff));

  p = strtok_r (playerResp, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"playstate\" : \"%s\"", p);
  strlcat (rbuff, tbuff, sizeof (rbuff));

  snprintf (tbuff, sizeof (tbuff), "%s%c", p, MSG_ARGS_RS);
  strlcat (statusbuff, tbuff, sizeof (statusbuff));

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"repeat\" : \"%s\"", p);
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  snprintf (tbuff, sizeof (tbuff), "%s%c", p, MSG_ARGS_RS);
  strlcat (statusbuff, tbuff, sizeof (statusbuff));

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"pauseatend\" : \"%s\"", p);
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  snprintf (tbuff, sizeof (tbuff), "%s%c", p, MSG_ARGS_RS);
  strlcat (statusbuff, tbuff, sizeof (statusbuff));

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"vol\" : \"%s%%\"", p);
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  snprintf (tbuff, sizeof (tbuff), "%s%c", p, MSG_ARGS_RS);
  strlcat (statusbuff, tbuff, sizeof (statusbuff));

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"speed\" : \"%s%%\"", p);
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  snprintf (tbuff, sizeof (tbuff), "%s%c", p, MSG_ARGS_RS);
  strlcat (statusbuff, tbuff, sizeof (statusbuff));

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"playedtime\" : \"%s\"", tmutilToMS (atol (p), tbuff2, sizeof (tbuff2)));
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  /* for marquee */
  snprintf (tbuff, sizeof (tbuff), "%s%c", p, MSG_ARGS_RS);
  strlcpy (timerbuff, tbuff, sizeof (timerbuff));

  snprintf (tbuff, sizeof (tbuff), "%s%c", p, MSG_ARGS_RS);
  strlcat (statusbuff, tbuff, sizeof (statusbuff));

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"duration\" : \"%s\"", tmutilToMS (atol (p), tbuff2, sizeof (tbuff2)));
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  /* for marquee */
  snprintf (tbuff, sizeof (tbuff), "%s", p);
  strlcat (timerbuff, tbuff, sizeof (timerbuff));

  snprintf (tbuff, sizeof (tbuff), "%s%c", p, MSG_ARGS_RS);
  strlcat (statusbuff, tbuff, sizeof (statusbuff));

  mainSendMusicqStatus (mainData, rbuff, sizeof (rbuff));

  strlcat (rbuff, " }", sizeof (rbuff));

  if (bdjoptGetNum (OPT_P_REMOTECONTROL)) {
    connSendMessage (mainData->conn, ROUTE_REMCTRL, MSG_PLAYER_STATUS_DATA, rbuff);
  }
  connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_PLAYER_STATUS_DATA, statusbuff);
  connSendMessage (mainData->conn, ROUTE_MANAGEUI, MSG_PLAYER_STATUS_DATA, statusbuff);
  if (mainData->marqueestarted) {
    connSendMessage (mainData->conn, ROUTE_MARQUEE, MSG_MARQUEE_TIMER, timerbuff);
  }
  logProcEnd (LOG_PROC, "mainSendPlayerStatus", "");
}

static void
mainSendMusicqStatus (maindata_t *mainData, char *rbuff, size_t siz)
{
  char    tbuff [200];
  char    statusbuff [40];
  ssize_t musicqLen;
  char    *data = NULL;
  bool    jsonflag = true;
  dbidx_t dbidx;
  song_t  *song;

  logProcBegin (LOG_PROC, "mainSendMusicqStatus");

  if (rbuff == NULL) {
    jsonflag = false;
  }

  statusbuff [0] = '\0';

  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);
  if (jsonflag) {
    snprintf (tbuff, sizeof (tbuff),
        "\"qlength\" : \"%zd\"", musicqLen);
    strlcat (rbuff, ", ", siz);
    strlcat (rbuff, tbuff, siz);
  }

  song = musicqGetByIdx (mainData->musicQueue, mainData->musicqPlayIdx, 0);

  /* dance */
  data = musicqGetDance (mainData->musicQueue, mainData->musicqPlayIdx, 0);
  if (data == NULL) { data = ""; }
  snprintf (tbuff, sizeof (tbuff),
      "\"dance\" : \"%s\"", data);
  if (jsonflag) {
    strlcat (rbuff, ", ", siz);
    strlcat (rbuff, tbuff, siz);
  }

  /* artist */
  data = songGetStr (song, TAG_ARTIST);
  if (data == NULL) { data = ""; }
  snprintf (tbuff, sizeof (tbuff),
      "\"artist\" : \"%s\"", data);
  if (jsonflag) {
    strlcat (rbuff, ", ", siz);
    strlcat (rbuff, tbuff, siz);
  }

  /* title */
  data = songGetStr (song, TAG_TITLE);
  if (data == NULL) { data = ""; }
  snprintf (tbuff, sizeof (tbuff),
      "\"title\" : \"%s\"", data);
  if (jsonflag) {
    strlcat (rbuff, ", ", siz);
    strlcat (rbuff, tbuff, siz);
  }

  dbidx = songGetNum (song, TAG_DBIDX);
  snprintf (tbuff, sizeof (tbuff), "%d%c", dbidx, MSG_ARGS_RS);
  strlcpy (statusbuff, tbuff, sizeof (statusbuff));

  if (dbidx >= 0) {
    connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_MUSICQ_STATUS_DATA, statusbuff);
    connSendMessage (mainData->conn, ROUTE_MANAGEUI, MSG_MUSICQ_STATUS_DATA, statusbuff);
  }
  logProcEnd (LOG_PROC, "mainSendMusicqStatus", "");
}

static void
mainDanceCountsInit (maindata_t *mainData)
{
  nlist_t     *dc;
  ilistidx_t  didx;
  nlistidx_t  iteridx;

  logProcBegin (LOG_PROC, "mainDanceCountsInit");

  dc = dbGetDanceCounts (mainData->musicdb);

  if (mainData->danceCounts != NULL) {
    nlistFree (mainData->danceCounts);
  }

  mainData->danceCounts = nlistAlloc ("main-dancecounts", LIST_ORDERED, NULL);
  nlistSetSize (mainData->danceCounts, nlistGetCount (dc));

  nlistStartIterator (dc, &iteridx);
  while ((didx = nlistIterateKey (dc, &iteridx)) >= 0) {
    nlistSetNum (mainData->danceCounts, didx, nlistGetNum (dc, didx));
  }
  logProcEnd (LOG_PROC, "mainDanceCountsInit", "");
}

static void
mainParseIntNum (char *args, int *a, ssize_t *b)
{
  char            *p;
  char            *tokstr;

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  *a = atoi (p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  *b = atol (p);
}

static void
mainParseIntStr (char *args, int *a, char **b)
{
  char            *p;
  char            *tokstr;

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  *a = atoi (p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  *b = p;
}

static void
mainSendFinished (maindata_t *mainData)
{
  connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_FINISHED, NULL);
  connSendMessage (mainData->conn, ROUTE_MANAGEUI, MSG_FINISHED, NULL);
  if (mainData->marqueestarted) {
    connSendMessage (mainData->conn, ROUTE_MARQUEE, MSG_FINISHED, NULL);
  }
}
