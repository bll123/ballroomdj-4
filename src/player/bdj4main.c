/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
/*
 * bdj4main
 *  Main entry point for the player process.
 *  Handles startup of the player, marquee, mobile marquee and
 *      mobile remote control.
 *  Handles playlists and the music queue.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <signal.h>

#include "bdj4.h"
#include "bdj4init.h"
#include "bdj4intl.h"
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
#include "mdebug.h"
#include "musicdb.h"
#include "musicq.h"
#include "osprocess.h"
#include "ossignal.h"
#include "osutils.h"
#include "pathbld.h"
#include "player.h"
#include "playlist.h"
#include "procutil.h"
#include "progstate.h"
#include "queue.h"
#include "slist.h"
#include "sock.h"
#include "sockh.h"
#include "songsel.h"
#include "songutil.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"

typedef enum {
  MOVE_UP = -1,
  MOVE_DOWN = 1,
} mainmove_t;

enum {
  MAIN_PB_TYPE_PL,
  MAIN_PB_TYPE_SONG,
  MAIN_CHG_CLEAR,
  MAIN_CHG_START,
  MAIN_CHG_FINAL,
  MAIN_CALC_WITH_SPEED,
  MAIN_CALC_NO_SPEED,
};

enum {
  MAIN_PREP_SIZE = 5,
  MAIN_NOT_SET = -1,
  MAIN_TS_DEBUG_MAX = 6,
};

typedef struct {
  playlist_t        *playlist;
  int               playlistIdx;
} playlistitem_t;

typedef struct {
  progstate_t       *progstate;
  procutil_t        *processes [ROUTE_MAX];
  conn_t            *conn;
  long              startflags;
  musicdb_t         *musicdb;
  /* playlistCache contains all of the playlists that are used */
  /* so that playlist lookups can be processed */
  nlist_t           *playlistCache;
  queue_t           *playlistQueue [MUSICQ_MAX];
  musicq_t          *musicQueue;
  musicqidx_t       musicqPlayIdx;
  musicqidx_t       musicqManageIdx;
  int               musicqDeferredPlayIdx;
  slist_t           *announceList;
  playerstate_t     playerState;
  long              lastGapSent;
  char              *mobmqUserkey;
  int               stopwaitcount;
  char              *pbfinishArgs;
  int               pbfinishType;
  bdjmsgroute_t     pbfinishRoute;
  int               pbfinishrcv;
  long              ploverridestoptime;
  int               songplaysentcount;        // for testsuite
  int               musicqChanged [MUSICQ_MAX];
  bool              marqueeChanged [MUSICQ_MAX];
  bool              changeSuspend [MUSICQ_MAX];
  time_t            stopTime [MUSICQ_MAX];
  time_t            nStopTime [MUSICQ_MAX];
  bool              switchQueueWhenEmpty : 1;
  bool              finished : 1;
  bool              marqueestarted : 1;
  bool              waitforpbfinish : 1;
} maindata_t;

static int  mainProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route, bdjmsgmsg_t msg, char *args, void *udata);
static int  mainProcessing (void *udata);
static bool mainListeningCallback (void *tmaindata, programstate_t programState);
static bool mainConnectingCallback (void *tmaindata, programstate_t programState);
static bool mainHandshakeCallback (void *tmaindata, programstate_t programState);
static void mainStartMarquee (maindata_t *mainData);
static bool mainStoppingCallback (void *tmaindata, programstate_t programState);
static bool mainStopWaitCallback (void *tmaindata, programstate_t programState);
static bool mainClosingCallback (void *tmaindata, programstate_t programState);
static void mainSendMusicQueueData (maindata_t *mainData, int musicqidx);
static void mainSendMarqueeData (maindata_t *mainData);
static char * mainSongGetDanceDisplay (maindata_t *mainData, int idx);
static void mainSendMobileMarqueeData (maindata_t *mainData);
static void mainQueueClear (maindata_t *mainData, char *args);
static void mainQueueDance (maindata_t *mainData, char *args, int count);
static void mainQueuePlaylist (maindata_t *mainData, char *plname);
static void mainSigHandler (int sig);
static void mainMusicQueueFill (maindata_t *mainData);
static void mainMusicQueuePrep (maindata_t *mainData, musicqidx_t mqidx);
static void mainMusicqClearPreppedSongs (maindata_t *mainData, musicqidx_t mqidx, int idx);
static void mainMusicqClearPrep (maindata_t *mainData, musicqidx_t mqidx, int idx);
static char *mainPrepSong (maindata_t *maindata, int flag, musicqidx_t mqidx, song_t *song, char *sfname, int playlistIdx, long uniqueidx);
static void mainPlaylistClearQueue (maindata_t *mainData, char *args);
static void mainTogglePause (maindata_t *mainData, char *args);
static void mainMusicqMove (maindata_t *mainData, char *args, mainmove_t direction);
static void mainMusicqMoveTop (maindata_t *mainData, char *args);
static void mainMusicqClear (maindata_t *mainData, char *args);
static void mainMusicqRemove (maindata_t *mainData, char *args);
static void mainMusicqSwap (maindata_t *mainData, char *args);
static void mainNextSong (maindata_t *mainData);
static void mainMusicqInsert (maindata_t *mainData, bdjmsgroute_t route, char *args);
static void mainMusicqSetManage (maindata_t *mainData, char *args);
static void mainMusicqSetPlayback (maindata_t *mainData, char *args);
static void mainMusicqSendConfig (maindata_t *mainData);
static void mainMusicqSwitch (maindata_t *mainData, musicqidx_t newidx);
static void mainPlaybackBegin (maindata_t *mainData);
static void mainMusicQueuePlay (maindata_t *mainData);
static void mainMusicQueueFinish (maindata_t *mainData, const char *args);
static void mainMusicQueueNext (maindata_t *mainData, const char *args);
static ilistidx_t mainMusicQueueLookup (void *mainData, ilistidx_t idx);
static void mainSendDanceList (maindata_t *mainData, bdjmsgroute_t route);
static void mainSendPlaylistList (maindata_t *mainData, bdjmsgroute_t route);
static void mainSendPlayerStatus (maindata_t *mainData, char *playerResp);
static void mainSendMusicqStatus (maindata_t *mainData);
static int  mainParseMqidxNum (maindata_t *mainData, char *args, ilistidx_t *b, ilistidx_t *c);
static int  mainParseQueuePlaylist (maindata_t *mainData, char *args, char **b, int *editmode);
static int  mainMusicqIndexParse (maindata_t *mainData, const char *p);
static void mainSendFinished (maindata_t *mainData);
static long mainCalculateSongDuration (maindata_t *mainData, song_t *song, int playlistIdx, musicqidx_t mqidx, int speedCalcFlag);
static playlistitem_t * mainPlaylistItemCache (maindata_t *mainData, playlist_t *pl, int playlistIdx);
static void mainPlaylistItemFree (void *tplitem);
static void mainMusicqSetSuspend (maindata_t *mainData, char *args, bool value);
static void mainMusicQueueMix (maindata_t *mainData, char *args);
static void mainPlaybackFinishProcess (maindata_t *mainData, const char *args);
static void mainPlaybackSendSongFinish (maindata_t *mainData, const char *args);
static void mainStatusRequest (maindata_t *mainData, bdjmsgroute_t routefrom);
static void mainAddTemporarySong (maindata_t *mainData, char *args);
static time_t mainCalcStopTime (time_t stopTime);
static void mainQueueInfoRequest (maindata_t *mainData, bdjmsgroute_t routefrom, const char *args);
static bool mainCheckMusicQStopTime (maindata_t *mainData, time_t nStopTime);
static playlist_t * mainNextPlaylist (maindata_t *mainData);
static void mainChkMusicq (maindata_t *mainData, bdjmsgroute_t routefrom);

static long globalCounter = 0;
static int  gKillReceived = 0;

int
main (int argc, char *argv[])
{
  maindata_t    mainData;
  uint16_t      listenPort;

#if BDJ4_MEM_DEBUG
  mdebugInit ("main");
#endif

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
  mainData.musicqPlayIdx = MUSICQ_PB_A;
  mainData.musicqManageIdx = MUSICQ_PB_A;
  mainData.musicqDeferredPlayIdx = MAIN_NOT_SET;
  mainData.playerState = PL_STATE_STOPPED;
  mainData.mobmqUserkey = NULL;
  mainData.pbfinishArgs = NULL;
  mainData.switchQueueWhenEmpty = false;
  mainData.finished = false;
  mainData.marqueestarted = false;
  mainData.waitforpbfinish = false;
  mainData.pbfinishrcv = 0;
  mainData.stopwaitcount = 0;
  mainData.ploverridestoptime = 0;
  mainData.songplaysentcount = 0;
  for (musicqidx_t i = 0; i < MUSICQ_MAX; ++i) {
    mainData.playlistQueue [i] = NULL;
    mainData.musicqChanged [i] = MAIN_CHG_CLEAR;
    mainData.marqueeChanged [i] = false;
    mainData.changeSuspend [i] = false;
    mainData.stopTime [i] = 0;
    mainData.nStopTime [i] = 0;
  }

  procutilInitProcesses (mainData.processes);
  osSetStandardSignals (mainSigHandler);

  mainData.startflags = BDJ4_INIT_ALL;
  bdj4startup (argc, argv, &mainData.musicdb,
      "main", ROUTE_MAIN, &mainData.startflags);
  logProcBegin (LOG_PROC, "main");

  mainData.lastGapSent = -2;
  /* calculate the stop time once only per queue */
  for (musicqidx_t i = 0; i < MUSICQ_MAX; ++i) {
    mainData.stopTime [i] = bdjoptGetNumPerQueue (OPT_Q_STOP_AT_TIME, i);
    mainData.nStopTime [i] = mainCalcStopTime (mainData.stopTime [i]);
  }

  mainData.conn = connInit (ROUTE_MAIN);

  mainData.playlistCache = nlistAlloc ("playlist-list", LIST_ORDERED,
      playlistFree);
  for (musicqidx_t i = 0; i < MUSICQ_MAX; ++i) {
    char  tmp [40];

    snprintf (tmp, sizeof (tmp), "playlist-q-%d", i);
    mainData.playlistQueue [i] = queueAlloc (tmp, mainPlaylistItemFree);
  }
  mainData.musicQueue = musicqAlloc (mainData.musicdb);
  mainData.announceList = slistAlloc ("announcements", LIST_ORDERED, NULL);

  listenPort = bdjvarsGetNum (BDJVL_MAIN_PORT);
  sockhMainLoop (listenPort, mainProcessMsg, mainProcessing, &mainData);
  connFree (mainData.conn);
  progstateFree (mainData.progstate);
  logProcEnd (LOG_PROC, "main", "");
  logEnd ();
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return 0;
}

/* internal routines */

static bool
mainStoppingCallback (void *tmaindata, programstate_t programState)
{
  maindata_t    *mainData = tmaindata;

  logProcBegin (LOG_PROC, "mainStoppingCallback");

  procutilStopAllProcess (mainData->processes, mainData->conn, false);
  connDisconnectAll (mainData->conn);
  logProcEnd (LOG_PROC, "mainStoppingCallback", "");
  return STATE_FINISHED;
}

static bool
mainStopWaitCallback (void *tmaindata, programstate_t programState)
{
  maindata_t  *mainData = tmaindata;
  bool        rc;

  rc = connWaitClosed (mainData->conn, &mainData->stopwaitcount);
  return rc;
}

static bool
mainClosingCallback (void *tmaindata, programstate_t programState)
{
  maindata_t    *mainData = tmaindata;
  char          *script;

  logProcBegin (LOG_PROC, "mainClosingCallback");

  nlistFree (mainData->playlistCache);
  for (musicqidx_t i = 0; i < MUSICQ_MAX; ++i) {
    if (mainData->playlistQueue [i] != NULL) {
      queueFree (mainData->playlistQueue [i]);
    }
  }
  if (mainData->musicQueue != NULL) {
    musicqFree (mainData->musicQueue);
  }
  slistFree (mainData->announceList);
  dataFree (mainData->mobmqUserkey);
  dataFree (mainData->pbfinishArgs);

  procutilStopAllProcess (mainData->processes, mainData->conn, true);
  procutilFreeAll (mainData->processes);

  script = bdjoptGetStr (OPT_M_SHUTDOWNSCRIPT);
  if (script != NULL &&
      *script &&
      fileopFileExists (script)) {
    const char  *targv [2];

    targv [0] = script;
    targv [1] = NULL;
    osProcessStart (targv, OS_PROC_DETACH, NULL, NULL);
  }

  bdj4shutdown (ROUTE_MAIN, mainData->musicdb);

  logProcEnd (LOG_PROC, "mainClosingCallback", "");
  return STATE_FINISHED;
}

static int
mainProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  maindata_t  *mainData;
  bool        dbgdisp = false;
  char        *targs = NULL;

  mainData = (maindata_t *) udata;

  if (args != NULL) {
    targs = mdstrdup (args);
  }

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MAIN: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (mainData->conn, routefrom);
          dbgdisp = true;
          break;
        }
        case MSG_SOCKET_CLOSE: {
          procutilCloseProcess (mainData->processes [routefrom],
              mainData->conn, routefrom);
          procutilFreeRoute (mainData->processes, routefrom);
          connDisconnect (mainData->conn, routefrom);
          dbgdisp = true;
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          progstateShutdownProcess (mainData->progstate);
          dbgdisp = true;
          break;
        }
        case MSG_PLAYLIST_CLEARPLAY: {
          char  *ttargs;

          ttargs = mdstrdup (targs);
          mainQueueClear (mainData, ttargs);
          mdfree (ttargs);
          mainNextSong (mainData);
          if (mainData->waitforpbfinish) {
            mainData->pbfinishArgs = mdstrdup (targs);
            mainData->pbfinishType = MAIN_PB_TYPE_PL;
          } else {
            mainQueuePlaylist (mainData, targs);
          }
          dbgdisp = true;
          break;
        }
        case MSG_QUEUE_CLEAR: {
          /* clears both the playlist queue and the music queue */
          logMsg (LOG_DBG, LOG_MSGS, "got: queue-clear");
          mainQueueClear (mainData, targs);
          dbgdisp = true;
          break;
        }
        case MSG_QUEUE_CLEAR_PLAY: {
          char  *ttargs;

          /* clears both the playlist queue and the music queue */
          /* does a next song and starts playing */
          logMsg (LOG_DBG, LOG_MSGS, "got: queue-clear-play");
          ttargs = mdstrdup (targs);
          mainQueueClear (mainData, ttargs);
          mdfree (ttargs);
          mainNextSong (mainData);
          /* if the player is paused, multiple selections will not start */
          /* playing, but in most cases, this works */
          if (mainData->waitforpbfinish) {
            mainData->pbfinishArgs = mdstrdup (targs);
            mainData->pbfinishType = MAIN_PB_TYPE_SONG;
            mainData->pbfinishRoute = routefrom;
          } else {
            mainMusicqInsert (mainData, routefrom, targs);
          }
          dbgdisp = true;
          break;
        }
        case MSG_QUEUE_PLAYLIST: {
          logMsg (LOG_DBG, LOG_MSGS, "got: playlist-queue %s", targs);
          mainQueuePlaylist (mainData, targs);
          dbgdisp = true;
          break;
        }
        case MSG_QUEUE_DANCE: {
          mainQueueDance (mainData, targs, 1);
          dbgdisp = true;
          break;
        }
        case MSG_QUEUE_DANCE_5: {
          mainQueueDance (mainData, targs, 5);
          dbgdisp = true;
          break;
        }
        case MSG_QUEUE_SWITCH_EMPTY: {
          mainData->switchQueueWhenEmpty = atoi (targs);
          dbgdisp = true;
          break;
        }
        case MSG_QUEUE_MIX: {
          mainMusicQueueMix (mainData, targs);
          dbgdisp = true;
          break;
        }
        case MSG_CMD_PLAY: {
          mainMusicQueuePlay (mainData);
          dbgdisp = true;
          break;
        }
        case MSG_CMD_NEXTSONG: {
          mainNextSong (mainData);
          dbgdisp = true;
          break;
        }
        case MSG_CMD_PLAYPAUSE: {
          if (mainData->playerState == PL_STATE_PLAYING ||
              mainData->playerState == PL_STATE_IN_FADEOUT) {
            connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_PLAY_PAUSE, NULL);
          } else {
            mainMusicQueuePlay (mainData);
          }
          dbgdisp = true;
          break;
        }
        case MSG_PLAYBACK_BEGIN: {
          /* do any begin song processing */
          mainPlaybackBegin (mainData);
          dbgdisp = true;
          break;
        }
        case MSG_PLAYBACK_FINISH_STOP: {
          mainMusicQueueFinish (mainData, targs);
          dbgdisp = true;
          break;
        }
        case MSG_PLAYBACK_FINISH: {
          mainPlaybackFinishProcess (mainData, targs);
          dbgdisp = true;
          break;
        }
        case MSG_PL_OVERRIDE_STOP_TIME: {
          /* this message overrides the stop time for the */
          /* next queue-playlist message */
          mainData->ploverridestoptime = atol (targs);
          dbgdisp = true;
          break;
        }
        case MSG_PL_CLEAR_QUEUE: {
          mainPlaylistClearQueue (mainData, targs);
          dbgdisp = true;
          break;
        }
        case MSG_MUSICQ_TOGGLE_PAUSE: {
          mainTogglePause (mainData, targs);
          dbgdisp = true;
          break;
        }
        case MSG_MUSICQ_MOVE_DOWN: {
          mainMusicqMove (mainData, targs, MOVE_DOWN);
          dbgdisp = true;
          break;
        }
        case MSG_MUSICQ_MOVE_TOP: {
          mainMusicqMoveTop (mainData, targs);
          dbgdisp = true;
          break;
        }
        case MSG_MUSICQ_MOVE_UP: {
          mainMusicqMove (mainData, targs, MOVE_UP);
          dbgdisp = true;
          break;
        }
        case MSG_MUSICQ_REMOVE: {
          mainMusicqRemove (mainData, targs);
          dbgdisp = true;
          break;
        }
        case MSG_MUSICQ_SWAP: {
          mainMusicqSwap (mainData, targs);
          dbgdisp = true;
          break;
        }
        case MSG_MUSICQ_TRUNCATE: {
          mainMusicqClear (mainData, targs);
          dbgdisp = true;
          break;
        }
        case MSG_MUSICQ_INSERT: {
          mainMusicqInsert (mainData, routefrom, targs);
          dbgdisp = true;
          break;
        }
        case MSG_MUSICQ_SET_MANAGE: {
          mainMusicqSetManage (mainData, targs);
          dbgdisp = true;
          break;
        }
        case MSG_MUSICQ_SET_PLAYBACK: {
          mainMusicqSetPlayback (mainData, targs);
          dbgdisp = true;
          break;
        }
        case MSG_MUSICQ_SET_LEN: {
          /* a temporary setting used for the song list editor */
          bdjoptSetNum (OPT_G_PLAYERQLEN, atol (targs));
          dbgdisp = true;
          break;
        }
        case MSG_MUSICQ_DATA_SUSPEND: {
          mainMusicqSetSuspend (mainData, targs, true);
          dbgdisp = true;
          break;
        }
        case MSG_MUSICQ_DATA_RESUME: {
          mainMusicqSetSuspend (mainData, targs, false);
          dbgdisp = true;
          break;
        }
        case MSG_PLAYER_STATE: {
          mainData->playerState = (playerstate_t) atol (targs);
          logMsg (LOG_DBG, LOG_MSGS, "got: pl-state: %d/%s",
              mainData->playerState, plstateDebugText (mainData->playerState));
          mainData->marqueeChanged [mainData->musicqPlayIdx] = true;
          if (mainData->playerState == PL_STATE_STOPPED) {
            ++mainData->pbfinishrcv;
          }
          dbgdisp = true;
          break;
        }
        case MSG_GET_DANCE_LIST: {
          mainSendDanceList (mainData, routefrom);
          dbgdisp = true;
          break;
        }
        case MSG_GET_PLAYLIST_LIST: {
          mainSendPlaylistList (mainData, routefrom);
          dbgdisp = true;
          break;
        }
        case MSG_PLAYER_STATUS_DATA: {
          mainSendPlayerStatus (mainData, targs);
          // dbgdisp = true;
          break;
        }
        case MSG_START_MARQUEE: {
          mainStartMarquee (mainData);
          dbgdisp = true;
          break;
        }
        case MSG_DB_ENTRY_UPDATE: {
          dbLoadEntry (mainData->musicdb, atol (targs));
          for (int i = 0; i < MUSICQ_MAX; ++i) {
            int   musicqLen;

            musicqLen = musicqGetLen (mainData->musicQueue, i);
            if (musicqLen <= 0) {
              continue;
            }
            mainData->musicqChanged [i] = MAIN_CHG_START;
            mainData->marqueeChanged [i] = true;
          }
          dbgdisp = true;
          break;
        }
        case MSG_DATABASE_UPDATE: {
          mainData->musicdb = bdj4ReloadDatabase (mainData->musicdb);
          musicqSetDatabase (mainData->musicQueue, mainData->musicdb);
          for (int i = 0; i < MUSICQ_MAX; ++i) {
            mainData->musicqChanged [i] = MAIN_CHG_START;
            mainData->marqueeChanged [i] = true;
          }
          dbgdisp = true;
          break;
        }
        case MSG_MAIN_REQ_STATUS: {
          mainStatusRequest (mainData, routefrom);
          dbgdisp = true;
          break;
        }
        case MSG_MAIN_REQ_QUEUE_INFO: {
          mainQueueInfoRequest (mainData, routefrom, targs);
          dbgdisp = true;
          break;
        }
        case MSG_DB_ENTRY_TEMP_ADD: {
          mainAddTemporarySong (mainData, targs);
          dbgdisp = true;
          break;
        }
        case MSG_CHK_MAIN_MUSICQ: {
          mainChkMusicq (mainData, routefrom);
          dbgdisp = true;
          break;
        }
        case MSG_CHK_MAIN_RESET_SENT: {
          mainData->songplaysentcount = 0;
          dbgdisp = true;
          break;
        }
        case MSG_CHK_MAIN_SET_GAP: {
          bdjoptSetNumPerQueue (OPT_Q_GAP, atoi (targs), mainData->musicqPlayIdx);
          connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_SET_PLAYBACK_GAP, targs);
          dbgdisp = true;
          break;
        }
        case MSG_CHK_MAIN_SET_MAXPLAYTIME: {
          bdjoptSetNumPerQueue (OPT_Q_MAXPLAYTIME, atol (targs), mainData->musicqPlayIdx);
          dbgdisp = true;
          break;
        }
        case MSG_CHK_MAIN_SET_STOPATTIME: {
          mainData->stopTime [mainData->musicqPlayIdx] = atol (targs);
          mainData->nStopTime [mainData->musicqPlayIdx] =
              mainCalcStopTime (mainData->stopTime [mainData->musicqPlayIdx]);
          dbgdisp = true;
          break;
        }
        case MSG_CHK_MAIN_SET_PLAYANNOUNCE: {
          bdjoptSetNumPerQueue (OPT_Q_PLAY_ANNOUNCE, atoi (targs), mainData->musicqPlayIdx);
          dbgdisp = true;
          break;
        }
        case MSG_CHK_MAIN_SET_QUEUE_ACTIVE: {
          bdjoptSetNumPerQueue (OPT_Q_ACTIVE, atoi (targs), mainData->musicqPlayIdx);
          dbgdisp = true;
          break;
        }
        case MSG_CHK_MAIN_SET_PLAY_WHEN_QUEUED: {
          bdjoptSetNumPerQueue (OPT_Q_PLAY_WHEN_QUEUED, atoi (targs), mainData->musicqPlayIdx);
          dbgdisp = true;
          break;
        }
        case MSG_CHK_MAIN_SET_PAUSE_EACH_SONG: {
          bdjoptSetNumPerQueue (OPT_Q_PAUSE_EACH_SONG, atoi (targs), mainData->musicqPlayIdx);
          dbgdisp = true;
          break;
        }
        case MSG_CHK_MAIN_SET_FADEIN: {
          bdjoptSetNumPerQueue (OPT_Q_FADEINTIME, atoi (targs), mainData->musicqPlayIdx);
          mainMusicqSendConfig (mainData);
          dbgdisp = true;
          break;
        }
        case MSG_CHK_MAIN_SET_FADEOUT: {
          bdjoptSetNumPerQueue (OPT_Q_FADEOUTTIME, atoi (targs), mainData->musicqPlayIdx);
          mainMusicqSendConfig (mainData);
          dbgdisp = true;
          break;
        }
        case MSG_CHK_CLEAR_PREP_Q: {
          /* clear any prepped announcements */
          slistFree (mainData->announceList);
          mainData->announceList = slistAlloc ("announcements", LIST_ORDERED, NULL);
          connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_CHK_CLEAR_PREP_Q, NULL);
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

  if (dbgdisp) {
    logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
        routefrom, msgRouteDebugText (routefrom),
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
  }
  dataFree (targs);

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

  for (int i = 0; i < MUSICQ_MAX; ++i) {
    if (mainData->changeSuspend [i] == false &&
        mainData->musicqChanged [i] == MAIN_CHG_FINAL) {
      mainSendMusicQueueData (mainData, i);
      mainData->musicqChanged [i] = MAIN_CHG_CLEAR;
    }
    if (mainData->musicqChanged [i] == MAIN_CHG_START) {
      mainData->musicqChanged [i] = MAIN_CHG_FINAL;
    }
  }

  if (mainData->processes [ROUTE_MARQUEE] != NULL &&
      ! connIsConnected (mainData->conn, ROUTE_MARQUEE)) {
    connConnect (mainData->conn, ROUTE_MARQUEE);
  }

  /* for the marquee, only the currently selected playback queue is */
  /* of interest */
  if (mainData->marqueeChanged [mainData->musicqPlayIdx]) {
    mainSendMarqueeData (mainData);
    mainSendMobileMarqueeData (mainData);
    mainData->marqueeChanged [mainData->musicqPlayIdx] = false;
    if (mainData->finished && mainData->playerState == PL_STATE_STOPPED) {
      mainData->finished = false;
    }
  }

  /* Do this after sending the latest musicq / marquee data. */
  /* Wait for both the PLAYBACK_FINISH message and for the player to stop */
  /* The 'finished' message is sent to the playerui after the player has */
  /* stopped, and it is necessary to wait for after it is sent before */
  /* queueing the next song or playlist. */
  /* Note that it is possible for the player to already be stopped. */
  if (mainData->waitforpbfinish) {
    if (mainData->pbfinishrcv == 2) {
      if (mainData->pbfinishArgs != NULL) {
        if (mainData->pbfinishType == MAIN_PB_TYPE_PL) {
          mainQueuePlaylist (mainData, mainData->pbfinishArgs);
        }
        if (mainData->pbfinishType == MAIN_PB_TYPE_SONG) {
          mainMusicqInsert (mainData, mainData->pbfinishRoute, mainData->pbfinishArgs);
        }
        mdfree (mainData->pbfinishArgs);
        mainData->pbfinishArgs = NULL;
      }
      mainData->pbfinishrcv = 0;
      mainData->waitforpbfinish = false;
    }
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
      const char  *targv [2];

      targv [0] = script;
      targv [1] = NULL;
      osProcessStart (targv, OS_PROC_DETACH, NULL, NULL);
    }

    mainData->processes [ROUTE_PLAYER] = procutilStartProcess (
        ROUTE_PLAYER, "bdj4player", flags, NULL);
    if (bdjoptGetNum (OPT_P_MOBILEMARQUEE)) {
      mainData->processes [ROUTE_MOBILEMQ] = procutilStartProcess (
          ROUTE_MOBILEMQ, "bdj4mobmq", flags, NULL);
    }
    if (bdjoptGetNum (OPT_P_REMOTECONTROL)) {
      mainData->processes [ROUTE_REMCTRL] = procutilStartProcess (
          ROUTE_REMCTRL, "bdj4rc", flags, NULL);
    }
  }

  if (bdjoptGetNum (OPT_P_MARQUEE_SHOW) != MARQUEE_SHOW_OFF &&
      (mainData->startflags & BDJ4_INIT_NO_START) != BDJ4_INIT_NO_START &&
      (mainData->startflags & BDJ4_INIT_NO_MARQUEE) != BDJ4_INIT_NO_MARQUEE) {
    mainStartMarquee (mainData);
  }

  logProcEnd (LOG_PROC, "mainListeningCallback", "");
  return STATE_FINISHED;
}

static bool
mainConnectingCallback (void *tmaindata, programstate_t programState)
{
  maindata_t    *mainData = tmaindata;
  int           connMax = 0;
  int           connCount = 0;
  bool          rc = STATE_NOT_FINISH;

  logProcBegin (LOG_PROC, "mainConnectingCallback");

  connProcessUnconnected (mainData->conn);

  if ((mainData->startflags & BDJ4_INIT_NO_START) != BDJ4_INIT_NO_START) {
    if (! connIsConnected (mainData->conn, ROUTE_PLAYER)) {
      connConnect (mainData->conn, ROUTE_PLAYER);
    }
    if (bdjoptGetNum (OPT_P_MOBILEMARQUEE)) {
      if (! connIsConnected (mainData->conn, ROUTE_MOBILEMQ)) {
        connConnect (mainData->conn, ROUTE_MOBILEMQ);
      }
    }
    if (bdjoptGetNum (OPT_P_REMOTECONTROL)) {
      if (! connIsConnected (mainData->conn, ROUTE_REMCTRL)) {
        connConnect (mainData->conn, ROUTE_REMCTRL);
      }
    }
    if (bdjoptGetNum (OPT_P_MARQUEE_SHOW) != MARQUEE_SHOW_OFF &&
        (mainData->startflags & BDJ4_INIT_NO_MARQUEE) != BDJ4_INIT_NO_MARQUEE) {
      if (! connIsConnected (mainData->conn, ROUTE_MARQUEE)) {
        connConnect (mainData->conn, ROUTE_MARQUEE);
      }
    }
  }

  ++connMax;
  if (connIsConnected (mainData->conn, ROUTE_PLAYER)) {
    ++connCount;
  }
  if (bdjoptGetNum (OPT_P_MOBILEMARQUEE)) {
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
  if (bdjoptGetNum (OPT_P_MARQUEE_SHOW) != MARQUEE_SHOW_OFF &&
      (mainData->startflags & BDJ4_INIT_NO_MARQUEE) != BDJ4_INIT_NO_MARQUEE) {
    ++connMax;
    if (connIsConnected (mainData->conn, ROUTE_MARQUEE)) {
      ++connCount;
    }
  }

  if (connCount == connMax) {
    rc = STATE_FINISHED;
  }

  logProcEnd (LOG_PROC, "mainConnectingCallback", "");
  return rc;
}

static bool
mainHandshakeCallback (void *tmaindata, programstate_t programState)
{
  maindata_t    *mainData = tmaindata;
  bool          rc = STATE_NOT_FINISH;
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
  if (connHaveHandshake (mainData->conn, ROUTE_TEST_SUITE)) {
    /* no connection to starterui or playerui/manageui */
    conn += 2;
  }
  /* main must have a connection to the player, starterui, and */
  /* one of manageui and playerui */
  /* alternatively, a connection to the player and the testsuite */
  if (conn == 3) {
    char    tmp [40];
    long    gap;

    gap = bdjoptGetNumPerQueue (OPT_Q_GAP, mainData->musicqPlayIdx);

    if (gap != mainData->lastGapSent) {
      snprintf (tmp, sizeof (tmp), "%ld", gap);
      connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_SET_PLAYBACK_GAP, tmp);
      mainData->lastGapSent = gap;
    }

    /* send the player the fade-in and fade-out times for the playback queue */
    mainMusicqSendConfig (mainData);

    connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_MAIN_READY, NULL);
    rc = STATE_FINISHED;
  }

  logProcEnd (LOG_PROC, "mainHandshakeCallback", "");
  return rc;
}

static void
mainStartMarquee (maindata_t *mainData)
{
  char        *theme;
  const char  *targv [2];
  int         idx = 0;
  int         flags = 0;

  if (mainData->marqueestarted) {
    return;
  }

  if (bdjoptGetNum (OPT_P_MARQUEE_SHOW) == MARQUEE_SHOW_OFF) {
    return;
  }

  /* set the theme for the marquee */
  theme = bdjoptGetStr (OPT_MP_MQ_THEME);
#if BDJ4_USE_GTK
  osSetEnv ("GTK_THEME", theme);
#endif

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
  char        *sbuff = NULL;
  int         musicqLen;
  slistidx_t  dbidx;
  song_t      *song;
  int         flags;
  int         pauseind;
  int         dispidx;
  long        uniqueidx;
  ssize_t     qDuration;

  logProcBegin (LOG_PROC, "mainSendMusicQueueData");

  musicqLen = musicqGetLen (mainData->musicQueue, musicqidx);

  qDuration = musicqGetDuration (mainData->musicQueue, musicqidx);
  dbidx = musicqGetByIdx (mainData->musicQueue, musicqidx, 0);

  sbuff = mdmalloc (BDJMSG_MAX);
  snprintf (sbuff, BDJMSG_MAX, "%d%c%"PRId64"%c%d%c",
      musicqidx, MSG_ARGS_RS, (int64_t) qDuration, MSG_ARGS_RS,
      dbidx, MSG_ARGS_RS);

  for (int i = 1; i <= musicqLen; ++i) {
    dbidx = musicqGetByIdx (mainData->musicQueue, musicqidx, i);
    song = dbGetByIdx (mainData->musicdb, dbidx);
    if (song != NULL) {
      dispidx = musicqGetDispIdx (mainData->musicQueue, musicqidx, i);
      snprintf (tbuff, sizeof (tbuff), "%d%c", dispidx, MSG_ARGS_RS);
      strlcat (sbuff, tbuff, BDJMSG_MAX);
      uniqueidx = musicqGetUniqueIdx (mainData->musicQueue, musicqidx, i);
      snprintf (tbuff, sizeof (tbuff), "%ld%c", uniqueidx, MSG_ARGS_RS);
      strlcat (sbuff, tbuff, BDJMSG_MAX);
      snprintf (tbuff, sizeof (tbuff), "%d%c", dbidx, MSG_ARGS_RS);
      strlcat (sbuff, tbuff, BDJMSG_MAX);
      flags = musicqGetFlags (mainData->musicQueue, musicqidx, i);
      pauseind = false;
      if ((flags & MUSICQ_FLAG_PAUSE) == MUSICQ_FLAG_PAUSE) {
        pauseind = true;
      }
      snprintf (tbuff, sizeof (tbuff), "%d%c", pauseind, MSG_ARGS_RS);
      strlcat (sbuff, tbuff, BDJMSG_MAX);
    }
  }

  /* only the displayable queues need to be updated in the playerui */
  if (musicqidx < MUSICQ_DISP_MAX &&
      connHaveHandshake (mainData->conn, ROUTE_PLAYERUI)) {
    connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_MUSIC_QUEUE_DATA, sbuff);
  }
  /* only the song list and the internal playback queue need updating in */
  /* the manageui */
  if (musicqidx >= MUSICQ_DISP_MAX &&
      connHaveHandshake (mainData->conn, ROUTE_MANAGEUI)) {
    connSendMessage (mainData->conn, ROUTE_MANAGEUI, MSG_MUSIC_QUEUE_DATA, sbuff);
  }
  dataFree (sbuff);
  logProcEnd (LOG_PROC, "mainSendMusicQueueData", "");
}

static void
mainSendMarqueeData (maindata_t *mainData)
{
  char        tbuff [200];
  char        *sbuff = NULL;
  char        *dstr = NULL;
  char        *tstr = NULL;
  int         mqLen;
  int         musicqLen;
  time_t      currTime;
  time_t      qdur = 0;

  logProcBegin (LOG_PROC, "mainSendMarqueeData");

  if (mainData->playerState == PL_STATE_STOPPED &&
      mainData->finished) {
    logMsg (LOG_DBG, LOG_MAIN, "sending finished");
    mainSendFinished (mainData);
    return;
  }

  if (! mainData->marqueestarted) {
    logProcEnd (LOG_PROC, "mainSendMarqueeData", "not-started");
    return;
  }

  mqLen = bdjoptGetNum (OPT_P_MQQLEN);
  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);

  /* artist/title, dance(s) */

  sbuff = mdmalloc (BDJMSG_MAX);
  sbuff [0] = '\0';

  dstr = musicqGetData (mainData->musicQueue, mainData->musicqPlayIdx, 0, TAG_ARTIST);
  if (dstr == NULL || *dstr == '\0') { dstr = MSG_ARGS_EMPTY_STR; }
  tstr = musicqGetData (mainData->musicQueue, mainData->musicqPlayIdx, 0, TAG_TITLE);
  if (tstr == NULL || *tstr == '\0') { tstr = MSG_ARGS_EMPTY_STR; }
  snprintf (tbuff, sizeof (tbuff), "%s%c%s%c", dstr, MSG_ARGS_RS, tstr, MSG_ARGS_RS);
  strlcat (sbuff, tbuff, BDJMSG_MAX);

  currTime = mstime ();
  if (musicqLen > 0) {
    for (int i = 0; i <= mqLen; ++i) {
      dbidx_t   dbidx;
      song_t    *song;

      if (mainData->stopTime [mainData->musicqPlayIdx] > 0 &&
          (currTime + qdur) > mainData->nStopTime [mainData->musicqPlayIdx]) {
        /* process stoptime for the marquee display */
        dstr = MSG_ARGS_EMPTY_STR;
      } else if ((i > 0 && mainData->playerState == PL_STATE_IN_GAP) ||
          (i > 1 && mainData->playerState == PL_STATE_IN_FADEOUT)) {
        dstr = MSG_ARGS_EMPTY_STR;
      } else if (i > musicqLen) {
        dstr = MSG_ARGS_EMPTY_STR;
      } else {
        dstr = mainSongGetDanceDisplay (mainData, i);
      }

      /* queue duration data needed for stoptime check */
      dbidx = musicqGetByIdx (mainData->musicQueue, mainData->musicqPlayIdx, i);
      if (dbidx >= 0) {
        int   plidx;

        song = dbGetByIdx (mainData->musicdb, dbidx);
        plidx = musicqGetPlaylistIdx (mainData->musicQueue, mainData->musicqPlayIdx, i);
        qdur += mainCalculateSongDuration (mainData, song, plidx, mainData->musicqPlayIdx, MAIN_CALC_WITH_SPEED);
      }

      snprintf (tbuff, sizeof (tbuff), "%s%c", dstr, MSG_ARGS_RS);
      strlcat (sbuff, tbuff, BDJMSG_MAX);
    }
  }

  connSendMessage (mainData->conn, ROUTE_MARQUEE, MSG_MARQUEE_DATA, sbuff);
  dataFree (sbuff);
  logProcEnd (LOG_PROC, "mainSendMarqueeData", "");
}

static char *
mainSongGetDanceDisplay (maindata_t *mainData, int idx)
{
  char      *tstr;
  char      *dstr;
  dbidx_t   dbidx;
  song_t    *song;

  logProcBegin (LOG_PROC, "mainSongGetDanceDisplay");

  tstr = NULL;
  dbidx = musicqGetByIdx (mainData->musicQueue, mainData->musicqPlayIdx, idx);
  song = dbGetByIdx (mainData->musicdb, dbidx);
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
  char        *jbuff = NULL;
  char        *title = NULL;
  char        *dstr = NULL;
  int         mqLen = 0;
  int         musicqLen = 0;
  time_t      currTime;
  time_t      qdur = 0;

  logProcBegin (LOG_PROC, "mainSendMobileMarqueeData");

  if (! bdjoptGetNum (OPT_P_MOBILEMARQUEE)) {
    logProcEnd (LOG_PROC, "mainSendMobileMarqueeData", "is-off");
    return;
  }
  if (mainData->finished) {
    logProcEnd (LOG_PROC, "mainSendMobileMarqueeData", "finished");
    return;
  }

  jbuff = mdmalloc (BDJMSG_MAX);

  mqLen = bdjoptGetNum (OPT_P_MQQLEN);
  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);
  title = bdjoptGetStr (OPT_P_MOBILEMQTITLE);
  if (title == NULL) {
    title = "";
  }

  strlcpy (jbuff, "{ ", BDJMSG_MAX);

  snprintf (tbuff, sizeof (tbuff), "\"mqlen\" : \"%d\", ", mqLen);
  strlcat (jbuff, tbuff, BDJMSG_MAX);
  snprintf (tbuff, sizeof (tbuff), "\"title\" : \"%s\"", title);
  strlcat (jbuff, tbuff, BDJMSG_MAX);

  currTime = mstime ();
  if (musicqLen > 0) {
    for (int i = 0; i <= mqLen; ++i) {
      dbidx_t   dbidx;
      song_t    *song;

      /* process stoptime for the marquee display */
      if (mainData->stopTime [mainData->musicqPlayIdx] > 0 &&
          (currTime + qdur) > mainData->nStopTime [mainData->musicqPlayIdx]) {
        dstr = "";
      } else if ((i > 0 && mainData->playerState == PL_STATE_IN_GAP) ||
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

      /* queue duration data needed for stoptime check */
      dbidx = musicqGetByIdx (mainData->musicQueue, mainData->musicqPlayIdx, i);
      if (dbidx >= 0) {
        int   plidx;

        song = dbGetByIdx (mainData->musicdb, dbidx);
        plidx = musicqGetPlaylistIdx (mainData->musicQueue, mainData->musicqPlayIdx, i);
        qdur += mainCalculateSongDuration (mainData, song, plidx, mainData->musicqPlayIdx, MAIN_CALC_WITH_SPEED);
      }

      if (i == 0) {
        snprintf (tbuff, sizeof (tbuff), "\"current\" : \"%s\"", dstr);
      } else {
        snprintf (tbuff, sizeof (tbuff), "\"mq%d\" : \"%s\"", i, dstr);
      }
      strlcat (jbuff, ", ", BDJMSG_MAX);
      strlcat (jbuff, tbuff, BDJMSG_MAX);
    }
  } else {
    strlcat (jbuff, ", ", BDJMSG_MAX);
    strlcat (jbuff, "\"skip\" : \"true\"", sizeof (jbuff));
  }
  strlcat (jbuff, " }", BDJMSG_MAX);

  connSendMessage (mainData->conn, ROUTE_MOBILEMQ, MSG_MARQUEE_DATA, jbuff);

  dataFree (jbuff);
  logProcEnd (LOG_PROC, "mainSendMobileMarqueeData", "");
}

/* clears both the playlist and music queues */
static void
mainQueueClear (maindata_t *mainData, char *args)
{
  int   mi;
  char  *p;
  char  *tokstr = NULL;

  logProcBegin (LOG_PROC, "mainQueueClear");

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  mi = mainMusicqIndexParse (mainData, p);

  logMsg (LOG_DBG, LOG_BASIC, "clear music queue");
  /* clear the playlist queue */
  /* otherwise an automatic or sequenced playlist will simply */
  /* fill up the music queue again */
  queueClear (mainData->playlistQueue [mi], 0);
  mainMusicqClearPreppedSongs (mainData, mi, 1);
  musicqClear (mainData->musicQueue, mi, 1);
  mainData->musicqChanged [mi] = MAIN_CHG_START;
  mainData->marqueeChanged [mi] = true;
  logProcEnd (LOG_PROC, "mainQueueClear", "");
}

static void
mainQueueDance (maindata_t *mainData, char *args, int count)
{
  playlistitem_t  *plitem;
  playlist_t      *playlist;
  int             mi;
  ilistidx_t      danceIdx;
  ilistidx_t      musicqLen;
  bool            playwhenqueued;

  logProcBegin (LOG_PROC, "mainQueueDance");

  /* get the musicq length before any songs are added */
  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);

  mi = mainParseMqidxNum (mainData, args, &danceIdx, NULL);

  logMsg (LOG_DBG, LOG_BASIC, "queue dance %d %d %d", mi, danceIdx, count);
  /* CONTEXT: player: the name of the special playlist for queueing a dance */
  if ((playlist = playlistLoad (_("QueueDance"), mainData->musicdb)) == NULL) {
    playlist = playlistCreate ("main_queue_dance", PLTYPE_AUTO, mainData->musicdb);
  }
  playlistSetConfigNum (playlist, PLAYLIST_STOP_AFTER, count);
  /* clear all dance selected/counts */
  playlistResetAll (playlist);
  /* this will also set 'selected' */
  playlistSetDanceCount (playlist, danceIdx, 1);
  logMsg (LOG_DBG, LOG_BASIC, "Queue Playlist: %s", "queue-dance");
  plitem = mainPlaylistItemCache (mainData, playlist, globalCounter++);
  queuePush (mainData->playlistQueue [mi], plitem);
  logMsg (LOG_DBG, LOG_MAIN, "push pl %s", "queue-dance");
  mainMusicQueueFill (mainData);
  mainMusicQueuePrep (mainData, mainData->musicqPlayIdx);
  mainData->musicqChanged [mi] = MAIN_CHG_START;
  mainSendMusicqStatus (mainData);

  playwhenqueued = bdjoptGetNumPerQueue (OPT_Q_PLAY_WHEN_QUEUED, mainData->musicqPlayIdx);
  if (playwhenqueued &&
      mainData->musicqPlayIdx == (musicqidx_t) mi &&
      mainData->playerState == PL_STATE_STOPPED &&
      musicqLen == 0) {
    mainMusicQueuePlay (mainData);
  }
  logProcEnd (LOG_PROC, "mainQueueDance", "");
}

static void
mainQueuePlaylist (maindata_t *mainData, char *args)
{
  int             mi;
  playlistitem_t  *plitem = NULL;
  playlist_t      *playlist = NULL;
  char            *plname;
  ilistidx_t      musicqLen;
  int             editmode;
  bool            playwhenqueued;

  logProcBegin (LOG_PROC, "mainQueuePlaylist");

  /* get the musicq length before any songs are added */
  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);

  mi = mainParseQueuePlaylist (mainData, args, &plname, &editmode);

  playlist = playlistLoad (plname, mainData->musicdb);
  if (playlist == NULL) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: Queue Playlist failed: %s", plname);
    return;
  }

  /* check and see if a stop time override is in effect */
  /* if so, set the playlist's stop time */
  if (mainData->ploverridestoptime > 0) {
    playlistSetConfigNum (playlist, PLAYLIST_STOP_TIME,
        mainData->ploverridestoptime);
  }
  mainData->ploverridestoptime = 0;

  logMsg (LOG_DBG, LOG_BASIC, "Queue Playlist: %d %s edit-mode:%d", mi, plname, editmode);
  playlistSetEditMode (playlist, editmode);

  plitem = mainPlaylistItemCache (mainData, playlist, globalCounter++);
  queuePush (mainData->playlistQueue [mainData->musicqManageIdx], plitem);
  logMsg (LOG_DBG, LOG_MAIN, "push pl %s", plname);
  mainMusicQueueFill (mainData);
  mainMusicQueuePrep (mainData, mainData->musicqPlayIdx);
  mainData->musicqChanged [mi] = MAIN_CHG_START;

  mainSendMusicqStatus (mainData);

  playwhenqueued = bdjoptGetNumPerQueue (OPT_Q_PLAY_WHEN_QUEUED, mainData->musicqPlayIdx);
  if (playwhenqueued &&
      mainData->musicqPlayIdx == (musicqidx_t) mi &&
      mainData->playerState == PL_STATE_STOPPED &&
      musicqLen == 0) {
    mainMusicQueuePlay (mainData);
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
  int             playerqLen;
  int             currlen;
  playlistitem_t  *plitem = NULL;
  playlist_t      *playlist = NULL;
  pltype_t        pltype = PLTYPE_SONGLIST;
  bool            stopatflag = false;
  long            dur;
  int             editmode = EDIT_FALSE;
  time_t          stopTime = 0;
  time_t          nStopTime = 0;

  logProcBegin (LOG_PROC, "mainMusicQueueFill");

  plitem = queueGetFirst (mainData->playlistQueue [mainData->musicqManageIdx]);
  playlist = NULL;
  if (plitem != NULL) {
    playlist = plitem->playlist;
  }
  if (playlist != NULL) {
    pltype = (pltype_t) playlistGetConfigNum (playlist, PLAYLIST_TYPE);
    editmode = playlistGetEditMode (playlist);
  }

  playerqLen = bdjoptGetNum (OPT_G_PLAYERQLEN);
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

  logMsg (LOG_DBG, LOG_BASIC, "fill: %d < %d", currlen, playerqLen);

  stopTime = playlistGetConfigNum (playlist, PLAYLIST_STOP_TIME);
  if (stopTime <= 0) {
    /* playlist stop time is not configured, use the queue stop time */
    stopTime = mainData->stopTime [mainData->musicqManageIdx];
  }
  if (editmode == EDIT_FALSE && stopTime > 0) {
    nStopTime = mainCalcStopTime (stopTime);
    stopatflag = mainCheckMusicQStopTime (mainData, nStopTime);
  }

  /* want current + playerqLen songs */
  while (playlist != NULL && currlen <= playerqLen && stopatflag == false) {
    song_t  *song = NULL;

    song = playlistGetNextSong (playlist, currlen,
        mainMusicQueueLookup, mainData);

    if (song != NULL) {
      logMsg (LOG_DBG, LOG_MAIN, "push song to musicq");
      dur = mainCalculateSongDuration (mainData, song,
          plitem->playlistIdx, mainData->musicqManageIdx, MAIN_CALC_WITH_SPEED);
      musicqPush (mainData->musicQueue, mainData->musicqManageIdx,
          songGetNum (song, TAG_DBIDX), plitem->playlistIdx, dur);
      mainData->musicqChanged [mainData->musicqManageIdx] = MAIN_CHG_START;
      mainData->marqueeChanged [mainData->musicqManageIdx] = true;
      currlen = musicqGetLen (mainData->musicQueue, mainData->musicqManageIdx);

      if (pltype == PLTYPE_AUTO) {
        plitem = queueGetFirst (mainData->playlistQueue [mainData->musicqManageIdx]);
        playlist = NULL;
        if (plitem != NULL) {
          playlist = plitem->playlist;
        }
        if (playlist != NULL && song != NULL) {
          playlistAddCount (playlist, song);
        }
      }
    }

    if (editmode == EDIT_FALSE && stopTime > 0) {
      stopatflag = mainCheckMusicQStopTime (mainData, nStopTime);
    }

    if (song == NULL || stopatflag) {
      logMsg (LOG_DBG, LOG_MAIN, "song is null or stop-at");
      playlist = mainNextPlaylist (mainData);
      stopatflag = false;
      stopTime = playlistGetConfigNum (playlist, PLAYLIST_STOP_TIME);
      if (stopTime > 0) {
        nStopTime = mainCalcStopTime (stopTime);
      }
      if (editmode == EDIT_FALSE && stopTime > 0) {
        stopatflag = mainCheckMusicQStopTime (mainData, nStopTime);
      }
      continue;
    }
  }

  logProcEnd (LOG_PROC, "mainMusicQueueFill", "");
}

static void
mainMusicQueuePrep (maindata_t *mainData, musicqidx_t mqidx)
{
  playlist_t    *playlist = NULL;
  int           announceflag = false;

  logProcBegin (LOG_PROC, "mainMusicQueuePrep");

  /* 5 is the number of songs to prep ahead of time */
  for (int i = 0; i < MAIN_PREP_SIZE; ++i) {
    char          *sfname = NULL;
    dbidx_t       dbidx;
    song_t        *song = NULL;
    musicqflag_t  flags;
    char          *annfname = NULL;
    int           playlistIdx;

    dbidx = musicqGetByIdx (mainData->musicQueue, mqidx, i);
    if (dbidx < 0) {
      continue;
    }

    song = dbGetByIdx (mainData->musicdb, dbidx);
    flags = musicqGetFlags (mainData->musicQueue, mqidx, i);
    playlistIdx = musicqGetPlaylistIdx (mainData->musicQueue, mqidx, i);
    announceflag = bdjoptGetNumPerQueue (OPT_Q_PLAY_ANNOUNCE, mqidx);
    if (announceflag != 1 && playlistIdx != MUSICQ_PLAYLIST_EMPTY) {
      playlist = nlistGetData (mainData->playlistCache, playlistIdx);
      announceflag = playlistGetConfigNum (playlist, PLAYLIST_ANNOUNCE);
    }

    if (i == 1) {
      long  gap;
      long  plgap = -1;
      char  tmp [40];

      gap = bdjoptGetNumPerQueue (OPT_Q_GAP, mqidx);
      /* special case: the manage ui playback queue has no gap */
      /* the manage ui playback queue otherwise inherits from the music q */
      if (mqidx == MUSICQ_MNG_PB) {
        gap = 0;
      }
      plgap = playlistGetConfigNum (playlist, PLAYLIST_GAP);
      if (plgap >= 0) {
        gap = plgap;
      }

      snprintf (tmp, sizeof (tmp), "%ld", gap);
      connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_SET_PLAYBACK_GAP, tmp);
    }

    if (song != NULL &&
        (flags & MUSICQ_FLAG_PREP) != MUSICQ_FLAG_PREP) {
      long uniqueidx;

      musicqSetFlag (mainData->musicQueue, mqidx, i, MUSICQ_FLAG_PREP);
      uniqueidx = musicqGetUniqueIdx (mainData->musicQueue, mqidx, i);
      sfname = songGetStr (song, TAG_FILE);
      annfname = mainPrepSong (mainData, PREP_SONG, mqidx, song, sfname,
          playlistIdx, uniqueidx);

      if (announceflag == 1) {
        if (annfname != NULL && *annfname) {
          musicqSetFlag (mainData->musicQueue, mqidx,
              i, MUSICQ_FLAG_ANNOUNCE);
          musicqSetAnnounce (mainData->musicQueue, mqidx, i, annfname);
        }
      }
    }
  }
  logProcEnd (LOG_PROC, "mainMusicQueuePrep", "");
}

static void
mainMusicqClearPreppedSongs (maindata_t *mainData, musicqidx_t mqidx, int idx)
{
  for (int i = idx; i < MAIN_PREP_SIZE * 3; ++i) {
    mainMusicqClearPrep (mainData, mqidx, i);
  }
}

static void
mainMusicqClearPrep (maindata_t *mainData, musicqidx_t mqidx, int idx)
{
  dbidx_t       dbidx;
  song_t        *song;
  musicqflag_t  flags;
  long          uniqueidx;


  flags = musicqGetFlags (mainData->musicQueue, mqidx, idx);
  uniqueidx = musicqGetUniqueIdx (mainData->musicQueue, mqidx, idx);

  if ((flags & MUSICQ_FLAG_PREP) == MUSICQ_FLAG_PREP) {
    musicqClearFlag (mainData->musicQueue, mqidx, idx, MUSICQ_FLAG_PREP);

    dbidx = musicqGetByIdx (mainData->musicQueue, mqidx, idx);
    if (dbidx < 0) {
      return;
    }

    song = dbGetByIdx (mainData->musicdb, dbidx);
    if (song != NULL) {
      char  tmp [200];

      snprintf (tmp, sizeof (tmp), "%ld%c%s",
          uniqueidx, MSG_ARGS_RS, songGetStr (song, TAG_FILE));
      connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_SONG_CLEAR_PREP, tmp);
    }
  }
}

static char *
mainPrepSong (maindata_t *mainData, int prepflag, musicqidx_t mqidx,
    song_t *song, char *sfname, int playlistIdx, long uniqueidx)
{
  char          tbuff [1024];
  playlist_t    *playlist = NULL;
  ssize_t       dur = 0;
  ssize_t       songstart = 0;
  int           speed = 100;
  double        voladjperc = 0;
  int           announceflag = false;
  ilistidx_t    danceidx;
  char          *annfname = NULL;

  logProcBegin (LOG_PROC, "mainPrepSong");

  sfname = songGetStr (song, TAG_FILE);
  voladjperc = songGetDouble (song, TAG_VOLUMEADJUSTPERC);
  if (voladjperc == LIST_DOUBLE_INVALID) {
    voladjperc = 0.0;
  }

  songstart = songGetNum (song, TAG_SONGSTART);
  if (songstart < 0) { songstart = 0; }
  dur = songGetNum (song, TAG_DURATION);

  speed = songGetNum (song, TAG_SPEEDADJUSTMENT);
  if (speed < 0) {
    speed = 100;
  }

  /* announcements don't need any of the following... */
  if (prepflag != PREP_ANNOUNCE) {
    dur = mainCalculateSongDuration (mainData, song, playlistIdx, mqidx, MAIN_CALC_WITH_SPEED);

    announceflag =
        bdjoptGetNumPerQueue (OPT_Q_PLAY_ANNOUNCE, mainData->musicqManageIdx);
    if (announceflag != 1) {
      playlist = nlistGetData (mainData->playlistCache, playlistIdx);
      announceflag = playlistGetConfigNum (playlist, PLAYLIST_ANNOUNCE);
    }
    if (announceflag == 1) {
      dance_t       *dances;
      song_t        *tsong;

      danceidx = songGetNum (song, TAG_DANCE);
      dances = bdjvarsdfGet (BDJVDF_DANCES);
      annfname = danceGetStr (dances, danceidx, DANCE_ANNOUNCE);
      if (annfname != NULL && *annfname) {
        listnum_t   tval;

        tsong = dbGetByName (mainData->musicdb, annfname);
        if (tsong != NULL) {
          tval = slistGetNum (mainData->announceList, annfname);
          if (tval == LIST_VALUE_INVALID) {
            /* only prep the announcement if it has not been prepped before */
            /* announcements are prepped with a uniqueidx = PL_UNIQUE_ANN */
            mainPrepSong (mainData, PREP_ANNOUNCE, mqidx, tsong, annfname, playlistIdx, PL_UNIQUE_ANN);
          }
          slistSetNum (mainData->announceList, annfname, 1);
        } else {
          annfname = NULL;
        }
      } /* found an announcement for the dance */
    } /* announcements are on */
  } /* if this is a normal song */

  snprintf (tbuff, sizeof (tbuff), "%s%c%"PRId64"%c%"PRId64"%c%d%c%.1f%c%d%c%ld",
      sfname, MSG_ARGS_RS,
      (int64_t) dur, MSG_ARGS_RS,
      (int64_t) songstart, MSG_ARGS_RS,
      speed, MSG_ARGS_RS,
      voladjperc, MSG_ARGS_RS,
      prepflag, MSG_ARGS_RS,
      uniqueidx);

  logMsg (LOG_DBG, LOG_MAIN, "prep song %s", sfname);
  connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_SONG_PREP, tbuff);
  logProcEnd (LOG_PROC, "mainPrepSong", "");
  return annfname;
}

static void
mainPlaylistClearQueue (maindata_t *mainData, char *args)
{
  int   mi;

  logProcBegin (LOG_PROC, "mainPlaylistClearQueue");

  /* used by the song list editor */
  /* after a song list is created via a sequenced or automatic playlist */
  /* then the playlist queue needs to be reset */

  mi = mainMusicqIndexParse (mainData, args);
  queueClear (mainData->playlistQueue [mi], 0);
  logProcEnd (LOG_PROC, "mainPlaylistClearQueue", "");
}

static void
mainTogglePause (maindata_t *mainData, char *args)
{
  int           mi;
  ilistidx_t    idx;
  ilistidx_t    musicqLen;
  musicqflag_t  flags;

  logProcBegin (LOG_PROC, "mainTogglePause");

  mi = mainParseMqidxNum (mainData, args, &idx, NULL);

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
  mainData->musicqChanged [mi] = MAIN_CHG_START;
  logProcEnd (LOG_PROC, "mainTogglePause", "");
}

static void
mainMusicqMove (maindata_t *mainData, char *args, mainmove_t direction)
{
  int           mi;
  ilistidx_t    fromidx;
  ilistidx_t    toidx;
  ilistidx_t    musicqLen;

  logProcBegin (LOG_PROC, "mainMusicqMove");


  mi = mainParseMqidxNum (mainData, args, &fromidx, NULL);

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
  mainData->musicqChanged [mi] = MAIN_CHG_START;
  mainData->marqueeChanged [mi] = true;
  mainMusicQueuePrep (mainData, mainData->musicqPlayIdx);

  if (toidx == 0) {
    mainSendMusicqStatus (mainData);
  }
  logProcEnd (LOG_PROC, "mainMusicqMove", "");
}

static void
mainMusicqMoveTop (maindata_t *mainData, char *args)
{
  int         mi;
  ilistidx_t  fromidx;
  ilistidx_t  toidx;
  ilistidx_t  musicqLen;

  logProcBegin (LOG_PROC, "mainMusicqMoveTop");

  mi = mainParseMqidxNum (mainData, args, &fromidx, NULL);

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
  mainData->musicqChanged [mi] = MAIN_CHG_START;
  mainData->marqueeChanged [mi] = true;
  mainMusicQueuePrep (mainData, mainData->musicqPlayIdx);
  logProcEnd (LOG_PROC, "mainMusicqMoveTop", "");
}

static void
mainMusicqClear (maindata_t *mainData, char *args)
{
  int         mi;
  ilistidx_t  idx;

  logProcBegin (LOG_PROC, "mainMusicqClear");

  mi = mainParseMqidxNum (mainData, args, &idx, NULL);

  mainMusicqClearPreppedSongs (mainData, mainData->musicqManageIdx, idx);
  musicqClear (mainData->musicQueue, mainData->musicqManageIdx, idx);
  /* there may be other playlists in the playlist queue */
  mainMusicQueueFill (mainData);
  mainMusicQueuePrep (mainData, mainData->musicqPlayIdx);
  mainData->musicqChanged [mi] = MAIN_CHG_START;
  mainData->marqueeChanged [mi] = true;
  logProcEnd (LOG_PROC, "mainMusicqClear", "");
}

static void
mainMusicqRemove (maindata_t *mainData, char *args)
{
  int           mi;
  ilistidx_t    idx;


  logProcBegin (LOG_PROC, "mainMusicqRemove");

  mi = mainParseMqidxNum (mainData, args, &idx, NULL);

  mainMusicqClearPrep (mainData, mainData->musicqManageIdx, idx);
  musicqRemove (mainData->musicQueue, mainData->musicqManageIdx, idx);
  mainMusicQueueFill (mainData);
  mainMusicQueuePrep (mainData, mainData->musicqPlayIdx);
  mainData->musicqChanged [mi] = MAIN_CHG_START;
  mainData->marqueeChanged [mi] = true;
  logProcEnd (LOG_PROC, "mainMusicqRemove", "");
}

static void
mainMusicqSwap (maindata_t *mainData, char *args)
{
  int           mi;
  ilistidx_t    fromidx;
  ilistidx_t    toidx;


  logProcBegin (LOG_PROC, "mainMusicqSwap");

  mi = mainParseMqidxNum (mainData, args, &fromidx, &toidx);
  if (fromidx >= 1 && toidx >= 1) {
    musicqSwap (mainData->musicQueue, mainData->musicqManageIdx, fromidx, toidx);
    mainMusicQueuePrep (mainData, mainData->musicqPlayIdx);
    mainData->musicqChanged [mi] = MAIN_CHG_START;
    mainData->marqueeChanged [mi] = true;
  }
  logProcEnd (LOG_PROC, "mainMusicqSwap", "");
}

static void
mainNextSong (maindata_t *mainData)
{
  int   currlen;

  currlen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);
  if (currlen > 0) {
    if (mainData->playerState == PL_STATE_STOPPED) {
      mainMusicqClearPrep (mainData, mainData->musicqPlayIdx, 0);
    }
    connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_PLAY_NEXTSONG, NULL);
    if (mainData->playerState == PL_STATE_PAUSED ||
        mainData->playerState == PL_STATE_STOPPED) {
      mainData->waitforpbfinish = true;
      mainData->pbfinishrcv = 0;
      if (mainData->playerState == PL_STATE_STOPPED) {
        /* already stopped, will not receive a stop message */
        ++mainData->pbfinishrcv;
      }
    }
  }
}

static void
mainMusicqInsert (maindata_t *mainData, bdjmsgroute_t routefrom, char *args)
{
  char      *tokstr = NULL;
  char      *p = NULL;
  long      idx;
  dbidx_t   dbidx;
  song_t    *song = NULL;
  long      currlen;


  logProcBegin (LOG_PROC, "mainMusicqInsert");

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  if (p == NULL) {
    logProcEnd (LOG_PROC, "mainMusicqInsert", "parse-fail-a");
    return;
  }
  mainMusicqIndexParse (mainData, p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (p == NULL) {
    logProcEnd (LOG_PROC, "mainMusicqInsert", "parse-fail-b");
    return;
  }
  idx = atol (p);
  /* want to insert after the selection */
  idx += 1;
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (p == NULL) {
    logProcEnd (LOG_PROC, "mainMusicqInsert", "parse-fail-c");
    return;
  }
  dbidx = atol (p);

  song = dbGetByIdx (mainData->musicdb, dbidx);

  currlen = musicqGetLen (mainData->musicQueue, mainData->musicqManageIdx);
  if (currlen == 0 &&
      mainData->musicqPlayIdx != mainData->musicqManageIdx) {
    musicqPushHeadEmpty (mainData->musicQueue, mainData->musicqManageIdx);
  }

  if (song != NULL) {
    long  loc;
    long  dur;
    int   musicqLen;
    bool  playwhenqueued;

    dur = mainCalculateSongDuration (mainData, song,
        MUSICQ_PLAYLIST_EMPTY, mainData->musicqManageIdx, MAIN_CALC_WITH_SPEED);
    loc = musicqInsert (mainData->musicQueue, mainData->musicqManageIdx,
        idx, dbidx, dur);
    mainData->musicqChanged [mainData->musicqManageIdx] = MAIN_CHG_START;
    mainData->marqueeChanged [mainData->musicqManageIdx] = true;
    mainMusicQueuePrep (mainData, mainData->musicqPlayIdx);
    mainSendMusicqStatus (mainData);
    musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);

    playwhenqueued = bdjoptGetNumPerQueue (OPT_Q_PLAY_WHEN_QUEUED, mainData->musicqPlayIdx);
    if (mainData->playerState == PL_STATE_STOPPED &&
        playwhenqueued &&
        mainData->musicqPlayIdx == mainData->musicqManageIdx &&
        musicqLen == 1) {
      mainMusicQueuePlay (mainData);
    }
    if (loc > 0) {
      char  tbuff [40];

      snprintf (tbuff, sizeof (tbuff), "%d%c%ld",
          mainData->musicqManageIdx, MSG_ARGS_RS, loc);
      connSendMessage (mainData->conn, routefrom, MSG_SONG_SELECT, tbuff);
    }
  }
  logProcEnd (LOG_PROC, "mainMusicqInsert", "");
}

static void
mainMusicqSetManage (maindata_t *mainData, char *args)
{
  musicqidx_t   mqidx;

  logProcBegin (LOG_PROC, "mainMusicqSetManage");

  mqidx = atoi (args);
  if (mqidx < 0 || mqidx >= MUSICQ_MAX) {
    logProcEnd (LOG_PROC, "mainMusicqSetManage", "bad-idx");
    return;
  }

  mainData->musicqManageIdx = mqidx;
  logProcEnd (LOG_PROC, "mainMusicqSetManage", "");
}

static void
mainMusicqSetPlayback (maindata_t *mainData, char *args)
{
  musicqidx_t   mqidx;

  logProcBegin (LOG_PROC, "mainMusicqSetPlayback");

  mqidx = atoi (args);
  if (mqidx < 0 || mqidx >= MUSICQ_MAX) {
    logProcEnd (LOG_PROC, "mainMusicqSetPlayback", "bad-idx");
    return;
  }

  if (mqidx == mainData->musicqPlayIdx) {
    mainData->musicqDeferredPlayIdx = MAIN_NOT_SET;
  } else if (mainData->playerState != PL_STATE_STOPPED) {
    /* if the player is playing something, the musicq index cannot */
    /* be changed as yet */
    mainData->musicqDeferredPlayIdx = mqidx;
    mainMusicQueuePrep (mainData, mqidx);
  } else {
    mainMusicqSwitch (mainData, mqidx);
  }
  logProcEnd (LOG_PROC, "mainMusicqSetPlayback", "");
}

static void
mainMusicqSendConfig (maindata_t *mainData)
{
  char          tmp [40];

  snprintf (tmp, sizeof (tmp), "%"PRId64,
      (int64_t) bdjoptGetNumPerQueue (OPT_Q_FADEINTIME, mainData->musicqPlayIdx));
  connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_SET_PLAYBACK_FADEIN, tmp);
  snprintf (tmp, sizeof (tmp), "%"PRId64,
      (int64_t) bdjoptGetNumPerQueue (OPT_Q_FADEOUTTIME, mainData->musicqPlayIdx));
  connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_SET_PLAYBACK_FADEOUT, tmp);
}

static void
mainMusicqSwitch (maindata_t *mainData, musicqidx_t newMusicqIdx)
{
  dbidx_t       dbidx;
  song_t        *song;
  musicqidx_t   oldplayidx;

  logProcBegin (LOG_PROC, "mainMusicqSwitch");

  oldplayidx = mainData->musicqPlayIdx;
  mainData->musicqManageIdx = oldplayidx;
  /* manage idx is pointing to the previous musicq play idx */
  musicqPushHeadEmpty (mainData->musicQueue, mainData->musicqManageIdx);
  /* the prior musicq has changed */
  mainData->musicqChanged [mainData->musicqManageIdx] = MAIN_CHG_START;
  /* send the prior musicq update while the play idx is still set */
  mainSendMusicqStatus (mainData);

  /* now change the play idx */
  mainData->musicqPlayIdx = newMusicqIdx;
  mainData->musicqManageIdx = mainData->musicqPlayIdx;
  mainData->musicqDeferredPlayIdx = MAIN_NOT_SET;

  /* send the player the fade-in and fade-out times for this queue */
  mainMusicqSendConfig (mainData);

  /* having switched the playback music queue, the songs must be prepped */
  mainMusicQueuePrep (mainData, mainData->musicqPlayIdx);

  dbidx = musicqGetCurrent (mainData->musicQueue, mainData->musicqPlayIdx);
  song = dbGetByIdx (mainData->musicdb, dbidx);
  if (song == NULL) {
    musicqPop (mainData->musicQueue, mainData->musicqPlayIdx);
  }

  /* and now send the musicq update for the new play idx */
  mainSendMusicqStatus (mainData);
  mainData->musicqChanged [mainData->musicqPlayIdx] = MAIN_CHG_START;
  mainData->marqueeChanged [mainData->musicqPlayIdx] = true;
  logProcEnd (LOG_PROC, "mainMusicqSwitch", "");
}

static void
mainPlaybackBegin (maindata_t *mainData)
{
  musicqflag_t  flags;
  bool          pauseind;

  logProcBegin (LOG_PROC, "mainPlaybackBegin");

  /* if the pause flag is on for this queue, */
  /* tell the player to turn on the pause-at-end */
  pauseind = bdjoptGetNumPerQueue (OPT_Q_PAUSE_EACH_SONG, mainData->musicqPlayIdx);
  if (pauseind) {
    connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_PLAY_PAUSEATEND, NULL);
    logProcEnd (LOG_PROC, "mainPlaybackBegin", "pause-each-song");
    return;
  }

  /* if the pause flag is on for this entry in the music queue, */
  /* tell the player to turn on the pause-at-end */
  flags = musicqGetFlags (mainData->musicQueue, mainData->musicqPlayIdx, 0);
  if ((flags & MUSICQ_FLAG_PAUSE) == MUSICQ_FLAG_PAUSE) {
    connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_PLAY_PAUSEATEND, NULL);
  }
  logProcEnd (LOG_PROC, "mainPlaybackBegin", "");
}

static void
mainMusicQueuePlay (maindata_t *mainData)
{
  musicqflag_t      flags;
  char              *sfname;
  dbidx_t           dbidx;
  long              uniqueidx;
  song_t            *song;
  int               currlen;
  musicqidx_t       origMusicqPlayIdx;
  time_t            currTime;

  logProcBegin (LOG_PROC, "mainMusicQueuePlay");

  currTime = mstime ();
  if (mainData->stopTime [mainData->musicqPlayIdx] > 0 &&
      currTime > mainData->nStopTime [mainData->musicqPlayIdx]) {
    logMsg (LOG_DBG, LOG_MAIN, "past stop-at time: finished <= true");
    mainData->finished = true;
    mainData->marqueeChanged [mainData->musicqPlayIdx] = true;
    /* reset the stop time so that the player can be re-started */
    mainData->stopTime [mainData->musicqPlayIdx] = 0;
    mainData->nStopTime [mainData->musicqPlayIdx] = 0;
  }

  if (mainData->musicqDeferredPlayIdx != MAIN_NOT_SET) {
    mainMusicqSwitch (mainData, mainData->musicqDeferredPlayIdx);
    mainData->musicqDeferredPlayIdx = MAIN_NOT_SET;
  }

  logMsg (LOG_DBG, LOG_BASIC, "pl-state: %d/%s",
      mainData->playerState, plstateDebugText (mainData->playerState));

  if (! mainData->finished && mainData->playerState != PL_STATE_PAUSED) {
    /* grab a song out of the music queue and start playing */
    logMsg (LOG_DBG, LOG_MAIN, "player sent a finish; get song, start");
    dbidx = musicqGetCurrent (mainData->musicQueue, mainData->musicqPlayIdx);
    song = dbGetByIdx (mainData->musicdb, dbidx);
    if (song != NULL) {
      char  tmp [1024];

      flags = musicqGetFlags (mainData->musicQueue, mainData->musicqPlayIdx, 0);
      uniqueidx = musicqGetUniqueIdx (mainData->musicQueue, mainData->musicqPlayIdx, 0);
      if ((flags & MUSICQ_FLAG_ANNOUNCE) == MUSICQ_FLAG_ANNOUNCE) {
        char      *annfname;

        annfname = musicqGetAnnounce (mainData->musicQueue, mainData->musicqPlayIdx, 0);
        if (annfname != NULL) {
          snprintf (tmp, sizeof (tmp), "%d%c%s", PL_UNIQUE_ANN, MSG_ARGS_RS, annfname);
          connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_SONG_PLAY, tmp);
        }
      }
      sfname = songGetStr (song, TAG_FILE);
      snprintf (tmp, sizeof (tmp), "%ld%c%s", uniqueidx, MSG_ARGS_RS, sfname);
      connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_SONG_PLAY, tmp);
      ++mainData->songplaysentcount;
    }

    currlen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);
    origMusicqPlayIdx = mainData->musicqPlayIdx;

    if (song == NULL && currlen == 0) {
      logMsg (LOG_DBG, LOG_MAIN, "no songs left in queue");
      if (mainData->switchQueueWhenEmpty) {
        char    tmp [40];

        logMsg (LOG_DBG, LOG_MAIN, "switch queues");
        mainData->musicqPlayIdx = musicqNextQueue (mainData->musicqPlayIdx);

        currlen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);

        /* locate a queue that has songs in it */
        while (mainData->musicqPlayIdx != origMusicqPlayIdx &&
            currlen == 0) {
          mainData->musicqPlayIdx = musicqNextQueue (mainData->musicqPlayIdx);
          currlen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);
        }

        /* the songs must be prepped */
        mainMusicQueuePrep (mainData, mainData->musicqPlayIdx);

        if (currlen > 0) {
          snprintf (tmp, sizeof (tmp), "%d", mainData->musicqPlayIdx);
          connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_QUEUE_SWITCH, tmp);
          /* and start up playback for the new queue */
          /* the next-song flag is always 0 here */
          /* if the state is stopped or paused, music-q-next will start up */
          /* playback */
          mainMusicQueueNext (mainData, "0");
        } else {
          /* there is no music to play; tell the player to clear its display */
          logMsg (LOG_DBG, LOG_MAIN, "sqwe:true no music to play: finished <= true");
          mainData->finished = true;
          mainData->marqueeChanged [mainData->musicqPlayIdx] = true;
        }
      } else {
        logMsg (LOG_DBG, LOG_MAIN, "no more songs; pl-state: %d/%s; finished <= true",
            mainData->playerState, plstateDebugText (mainData->playerState));
        mainData->finished = true;
        mainData->marqueeChanged [mainData->musicqPlayIdx] = true;
      }
    } /* if the song was null and the queue is empty */
  } /* song is not paused */

  /* this handles the user-selected play button when the song is paused */
  /* it no longer handles in-gap due to some changes */
  /* this might need to be moved to the message handler */
  if (! mainData->finished && mainData->playerState == PL_STATE_PAUSED) {
    logMsg (LOG_DBG, LOG_MAIN, "player is paused, send play msg");
    connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_PLAY_PLAY, NULL);
  }

  logProcEnd (LOG_PROC, "mainMusicQueuePlay", "");
}

static void
mainMusicQueueFinish (maindata_t *mainData, const char *args)
{
  playlist_t    *playlist;
  dbidx_t       dbidx;
  song_t        *song;
  int           playlistIdx;

  logProcBegin (LOG_PROC, "mainMusicQueueFinish");

  mainPlaybackSendSongFinish (mainData, args);

  /* let the playlist know this song has been played */
  dbidx = musicqGetCurrent (mainData->musicQueue, mainData->musicqPlayIdx);
  song = dbGetByIdx (mainData->musicdb, dbidx);
  playlistIdx = musicqGetPlaylistIdx (mainData->musicQueue, mainData->musicqPlayIdx, 0);
  if (playlistIdx != MUSICQ_PLAYLIST_EMPTY) {
    playlist = nlistGetData (mainData->playlistCache, playlistIdx);
    if (playlist != NULL && song != NULL) {
      playlistAddPlayed (playlist, song);
    }
  }

  musicqPop (mainData->musicQueue, mainData->musicqPlayIdx);
  mainData->musicqChanged [mainData->musicqPlayIdx] = MAIN_CHG_START;
  mainData->marqueeChanged [mainData->musicqPlayIdx] = true;
  mainSendMusicqStatus (mainData);
  logProcEnd (LOG_PROC, "mainMusicQueueFinish", "");
}

static void
mainMusicQueueNext (maindata_t *mainData, const char *args)
{
  logProcBegin (LOG_PROC, "mainMusicQueueNext");

  mainMusicQueueFinish (mainData, args);
  if (mainData->playerState != PL_STATE_STOPPED &&
      mainData->playerState != PL_STATE_PAUSED) {
    mainMusicQueuePlay (mainData);
  }
  mainMusicQueueFill (mainData);
  mainMusicQueuePrep (mainData, mainData->musicqPlayIdx);

  if (mainData->playerState == PL_STATE_STOPPED ||
      mainData->playerState == PL_STATE_PAUSED) {
    if (musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx) == 0) {
      mainData->finished = true;
    }
  }
  logProcEnd (LOG_PROC, "mainMusicQueueNext", "");
}

static ilistidx_t
mainMusicQueueLookup (void *tmaindata, ilistidx_t idx)
{
  maindata_t    *mainData = tmaindata;
  ilistidx_t    didx = -1;
  dbidx_t       dbidx;
  song_t        *song;

  logProcBegin (LOG_PROC, "mainMusicQueueLookup");

  if (idx < 0 ||
      idx >= musicqGetLen (mainData->musicQueue, mainData->musicqManageIdx)) {
    logProcEnd (LOG_PROC, "mainMusicQueueLookup", "bad-idx");
    return -1;
  }

  dbidx = musicqGetByIdx (mainData->musicQueue, mainData->musicqManageIdx, idx);
  song = dbGetByIdx (mainData->musicdb, dbidx);
  if (song != NULL) {
    didx = songGetNum (song, TAG_DANCE);
  }
  logProcEnd (LOG_PROC, "mainMusicQueueLookup", "");
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
  char          *rbuff = NULL;
  slistidx_t    iteridx;

  logProcBegin (LOG_PROC, "mainSendDanceList");

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  danceList = danceGetDanceList (dances);

  rbuff = mdmalloc (BDJMSG_MAX);
  rbuff [0] = '\0';
  slistStartIterator (danceList, &iteridx);
  while ((dancenm = slistIterateKey (danceList, &iteridx)) != NULL) {
    idx = slistGetNum (danceList, dancenm);
    snprintf (tbuff, sizeof (tbuff), "%d%c%s%c",
        idx, MSG_ARGS_RS, dancenm, MSG_ARGS_RS);
    strlcat (rbuff, tbuff, BDJMSG_MAX);
  }

  connSendMessage (mainData->conn, route, MSG_DANCE_LIST_DATA, rbuff);
  dataFree (rbuff);
  logProcEnd (LOG_PROC, "mainSendDanceList", "");
}

static void
mainSendPlaylistList (maindata_t *mainData, bdjmsgroute_t route)
{
  slist_t       *plList = NULL;
  char          *plfnm = NULL;
  char          *plnm = NULL;
  char          tbuff [200];
  char          *rbuff = NULL;
  slistidx_t    iteridx;

  logProcBegin (LOG_PROC, "mainSendPlaylistList");

  plList = playlistGetPlaylistList (PL_LIST_NORMAL);

  rbuff = mdmalloc (BDJMSG_MAX);
  rbuff [0] = '\0';
  slistStartIterator (plList, &iteridx);
  while ((plnm = slistIterateKey (plList, &iteridx)) != NULL) {
    plfnm = slistGetStr (plList, plnm);
    snprintf (tbuff, sizeof (tbuff), "%s%c%s%c",
        plfnm, MSG_ARGS_RS, plnm, MSG_ARGS_RS);
    strlcat (rbuff, tbuff, BDJMSG_MAX);
  }

  slistFree (plList);

  connSendMessage (mainData->conn, route, MSG_PLAYLIST_LIST_DATA, rbuff);
  dataFree (rbuff);
  logProcEnd (LOG_PROC, "mainSendPlaylistList", "");
}

static void
mainSendPlayerStatus (maindata_t *mainData, char *playerResp)
{
  char    tbuff [200];
  char    tbuff2 [40];
  char    *jsbuff = NULL;
  char    *timerbuff = NULL;
  char    *tokstr = NULL;
  char    *p;
  int     jsonflag;
  int     musicqLen;
  char    *data;
  dbidx_t dbidx;
  song_t  *song;

  logProcBegin (LOG_PROC, "mainSendPlayerStatus");

  connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_PLAYER_STATUS_DATA, playerResp);
  connSendMessage (mainData->conn, ROUTE_MANAGEUI, MSG_PLAYER_STATUS_DATA, playerResp);

  jsonflag = bdjoptGetNum (OPT_P_REMOTECONTROL);

  if (jsonflag) {
    jsbuff = mdmalloc (BDJMSG_MAX);
  }
  timerbuff = mdmalloc (BDJMSG_MAX);

  if (jsonflag) {
    strlcpy (jsbuff, "{ ", BDJMSG_MAX);

    p = "stop";
    switch (mainData->playerState) {
      case PL_STATE_UNKNOWN:
      case PL_STATE_STOPPED: {
        p = "stop";
        break;
      }
      case PL_STATE_LOADING:
      case PL_STATE_PLAYING:
      case PL_STATE_IN_FADEOUT:
      case PL_STATE_IN_GAP: {
        p = "play";
        break;
      }
      case PL_STATE_PAUSED: {
        p = "pause";
        break;
      }
      default: {
        /* invalid state - this would be an error */
        p = "stop";
        break;
      }
    }
    snprintf (tbuff, sizeof (tbuff),
        "\"playstate\" : \"%s\"", p);
    strlcat (jsbuff, tbuff, BDJMSG_MAX);
  }

  p = strtok_r (playerResp, MSG_ARGS_RS_STR, &tokstr);
  if (p != NULL && jsonflag) {
    snprintf (tbuff, sizeof (tbuff),
        "\"repeat\" : \"%s\"", p);
    strlcat (jsbuff, ", ", BDJMSG_MAX);
    strlcat (jsbuff, tbuff, BDJMSG_MAX);
  }

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (p != NULL && jsonflag) {
    snprintf (tbuff, sizeof (tbuff),
        "\"pauseatend\" : \"%s\"", p);
    strlcat (jsbuff, ", ", BDJMSG_MAX);
    strlcat (jsbuff, tbuff, BDJMSG_MAX);
  }

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (p != NULL && jsonflag) {
    snprintf (tbuff, sizeof (tbuff),
        "\"vol\" : \"%s%%\"", p);
    strlcat (jsbuff, ", ", BDJMSG_MAX);
    strlcat (jsbuff, tbuff, BDJMSG_MAX);
  }

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (p != NULL && jsonflag) {
    snprintf (tbuff, sizeof (tbuff),
        "\"speed\" : \"%s%%\"", p);
    strlcat (jsbuff, ", ", BDJMSG_MAX);
    strlcat (jsbuff, tbuff, BDJMSG_MAX);
  }

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (p != NULL && jsonflag) {
    snprintf (tbuff, sizeof (tbuff),
        "\"playedtime\" : \"%s\"", tmutilToMS (atol (p), tbuff2, sizeof (tbuff2)));
    strlcat (jsbuff, ", ", BDJMSG_MAX);
    strlcat (jsbuff, tbuff, BDJMSG_MAX);
  }

  if (p != NULL) {
    /* for marquee */
    snprintf (tbuff, sizeof (tbuff), "%s%c", p, MSG_ARGS_RS);
    strlcpy (timerbuff, tbuff, BDJMSG_MAX);
  }

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (p != NULL && jsonflag) {
    snprintf (tbuff, sizeof (tbuff),
        "\"duration\" : \"%s\"", tmutilToMS (atol (p), tbuff2, sizeof (tbuff2)));
    strlcat (jsbuff, ", ", BDJMSG_MAX);
    strlcat (jsbuff, tbuff, BDJMSG_MAX);
  }

  if (p != NULL) {
    /* for marquee */
    snprintf (tbuff, sizeof (tbuff), "%s", p);
    strlcat (timerbuff, tbuff, BDJMSG_MAX);
  }

  if (mainData->marqueestarted) {
    connSendMessage (mainData->conn, ROUTE_MARQUEE, MSG_MARQUEE_TIMER, timerbuff);
  }

  if (! jsonflag) {
    logProcEnd (LOG_PROC, "mainSendPlayerStatus", "no-json");
    dataFree (jsbuff);
    dataFree (timerbuff);
    return;
  }

  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);
  snprintf (tbuff, sizeof (tbuff),
      "\"qlength\" : \"%d\"", musicqLen);
  strlcat (jsbuff, ", ", BDJMSG_MAX);
  strlcat (jsbuff, tbuff, BDJMSG_MAX);

  /* dance */
  data = musicqGetDance (mainData->musicQueue, mainData->musicqPlayIdx, 0);
  if (data == NULL) { data = ""; }
  snprintf (tbuff, sizeof (tbuff),
      "\"dance\" : \"%s\"", data);
  strlcat (jsbuff, ", ", BDJMSG_MAX);
  strlcat (jsbuff, tbuff, BDJMSG_MAX);

  dbidx = musicqGetByIdx (mainData->musicQueue, mainData->musicqPlayIdx, 0);
  song = dbGetByIdx (mainData->musicdb, dbidx);

  /* artist */
  data = songGetStr (song, TAG_ARTIST);
  if (data == NULL) { data = ""; }
  snprintf (tbuff, sizeof (tbuff),
      "\"artist\" : \"%s\"", data);
  strlcat (jsbuff, ", ", BDJMSG_MAX);
  strlcat (jsbuff, tbuff, BDJMSG_MAX);

  /* title */
  data = songGetStr (song, TAG_TITLE);
  if (data == NULL) { data = ""; }
  snprintf (tbuff, sizeof (tbuff),
      "\"title\" : \"%s\"", data);
  strlcat (jsbuff, ", ", BDJMSG_MAX);
  strlcat (jsbuff, tbuff, BDJMSG_MAX);

  strlcat (jsbuff, " }", BDJMSG_MAX);

  connSendMessage (mainData->conn, ROUTE_REMCTRL, MSG_PLAYER_STATUS_DATA, jsbuff);
  dataFree (jsbuff);
  dataFree (timerbuff);
  logProcEnd (LOG_PROC, "mainSendPlayerStatus", "");
}

static void
mainSendMusicqStatus (maindata_t *mainData)
{
  char    tbuff [40];
  dbidx_t dbidx;

  logProcBegin (LOG_PROC, "mainSendMusicqStatus");

  dbidx = musicqGetByIdx (mainData->musicQueue, mainData->musicqPlayIdx, 0);
  snprintf (tbuff, sizeof (tbuff), "%d", dbidx);

  connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_MUSICQ_STATUS_DATA, tbuff);
  connSendMessage (mainData->conn, ROUTE_MANAGEUI, MSG_MUSICQ_STATUS_DATA, tbuff);
  logProcEnd (LOG_PROC, "mainSendMusicqStatus", "");
}

/* calls mainMusicqIndexParse, */
/* which will set musicqManageIdx */
static int
mainParseMqidxNum (maindata_t *mainData, char *args, ilistidx_t *b, ilistidx_t *c)
{
  int     mi;
  char    *p;
  char    *tokstr;

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  mi = mainMusicqIndexParse (mainData, p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  *b = 0;
  if (p != NULL) {
    *b = atol (p);
  }
  if (c != NULL) {
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    *c = 0;
    if (p != NULL && *p) {
      *c = atol (p);
    }
  }

  return mi;
}

/* calls mainMusicqIndexParse, */
/* which will set musicqManageIdx */
static int
mainParseQueuePlaylist (maindata_t *mainData, char *args, char **b, int *editmode)
{
  int   mi;
  char  *p;
  char  *tokstr;

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  mi = mainMusicqIndexParse (mainData, p);

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  *b = p;

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  *editmode = EDIT_FALSE;

  /* one of the few messages where a null is allowed */
  /* the remote-control module's playlist-clear-play and queue-playlist */
  /* messages do not include the edit mode flag */
  /* this may change at a future date */
  if (p != NULL) {
    *editmode = atoi (p);
  }

  return mi;
}

/* sets musicqManageIdx */
static inline int
mainMusicqIndexParse (maindata_t *mainData, const char *p)
{
  int   mi = MUSICQ_CURRENT;

  if (p != NULL) {
    mi = atoi (p);
  }
  if (mi != MUSICQ_CURRENT && mi >= 0 && mi < MUSICQ_MAX) {
    /* the managed queue is changed to the requested queue */
    /* otherwise, just leave musicqManageIdx as is */
    mainData->musicqManageIdx = mi;
  }

  return mainData->musicqManageIdx;
}

static void
mainSendFinished (maindata_t *mainData)
{
  connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_FINISHED, NULL);
  connSendMessage (mainData->conn, ROUTE_MANAGEUI, MSG_FINISHED, NULL);
  if (mainData->marqueestarted) {
    connSendMessage (mainData->conn, ROUTE_MARQUEE, MSG_FINISHED, NULL);
  }
  if (bdjoptGetNum (OPT_P_MOBILEMARQUEE)) {
    connSendMessage (mainData->conn, ROUTE_MOBILEMQ, MSG_FINISHED, NULL);
  }
}

static long
mainCalculateSongDuration (maindata_t *mainData, song_t *song,
    int playlistIdx, musicqidx_t mqidx, int speedCalcFlag)
{
  long        maxdur;
  long        dur;
  playlist_t  *playlist = NULL;
  long        pldncmaxdur;
  long        plmaxdur;
  long        songstart;
  long        songend;
  int         speed;
  ilistidx_t  danceidx;


  if (song == NULL) {
    return 0;
  }

  logProcBegin (LOG_PROC, "mainCalculateSongDuration");

  songstart = 0;
  speed = 100;
  dur = songGetNum (song, TAG_DURATION);
  logMsg (LOG_DBG, LOG_MAIN, "song base-dur: %ld", dur);

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

  /* apply songend if set to a reasonable value */
  logMsg (LOG_DBG, LOG_MAIN, "dur: %ld songstart: %ld songend: %ld",
      dur, songstart, songend);
  if (songend >= 10000 && dur > songend) {
    dur = songend;
    logMsg (LOG_DBG, LOG_MAIN, "dur-songend: %ld", dur);
  }
  /* adjust the song's duration by the songstart value */
  if (songstart > 0) {
    dur -= songstart;
    logMsg (LOG_DBG, LOG_MAIN, "dur-songstart: %ld", dur);
  }

  /* after adjusting the duration by song start/end, then adjust */
  /* the duration by the speed of the song */
  /* this is the real duration for the song */
  if (speedCalcFlag == MAIN_CALC_WITH_SPEED && speed != 100) {
    dur = songutilAdjustPosReal (dur, speed);
    logMsg (LOG_DBG, LOG_MAIN, "dur-speed: %ld", dur);
  }

  maxdur = bdjoptGetNumPerQueue (OPT_Q_MAXPLAYTIME, mqidx);
  logMsg (LOG_DBG, LOG_MAIN, "dur: %ld", dur);
  logMsg (LOG_DBG, LOG_MAIN, "dur-q-maxdur: %ld", maxdur);

  /* the playlist pointer is needed to get the playlist dance-max-dur */
  plmaxdur = LIST_VALUE_INVALID;
  if (playlistIdx != MUSICQ_PLAYLIST_EMPTY) {
    playlist = nlistGetData (mainData->playlistCache, playlistIdx);
    plmaxdur = playlistGetConfigNum (playlist, PLAYLIST_MAX_PLAY_TIME);
  }

  /* if the playlist has a maximum play time specified for a dance */
  /* it overrides any of the other max play times */
  danceidx = songGetNum (song, TAG_DANCE);
  pldncmaxdur = playlistGetDanceNum (playlist, danceidx, PLDANCE_MAXPLAYTIME);
  if (pldncmaxdur >= 5000 && dur > pldncmaxdur) {
    dur = pldncmaxdur;
    logMsg (LOG_DBG, LOG_MAIN, "dur-pl-dnc-dur: %ld", dur);
  } else {
    /* the playlist max-play-time overrides the global max-play-time */
    if (plmaxdur >= 5000 && dur > plmaxdur) {
      dur = plmaxdur;
      logMsg (LOG_DBG, LOG_MAIN, "dur-pl-dur: %ld", dur);
    } else if (maxdur >= 5000 && dur > maxdur) {
      dur = maxdur;
      logMsg (LOG_DBG, LOG_MAIN, "dur-q-dur: %ld", dur);
    }
  }

  logProcEnd (LOG_PROC, "mainCalculateSongDuration", "");
  return dur;
}

static playlistitem_t *
mainPlaylistItemCache (maindata_t *mainData, playlist_t *pl, int playlistIdx)
{
  playlistitem_t  *plitem;

  plitem = mdmalloc (sizeof (playlistitem_t));
  plitem->playlist = pl;
  plitem->playlistIdx = playlistIdx;
  nlistSetData (mainData->playlistCache, playlistIdx, pl);
  return plitem;
}

static void
mainPlaylistItemFree (void *tplitem)
{
  playlistitem_t *plitem = tplitem;

  if (plitem != NULL) {
    /* the playlist data is owned by the playlistCache and is freed by it */
    mdfree (plitem);
  }
}

static void
mainMusicqSetSuspend (maindata_t *mainData, char *args, bool value)
{
  int     mqidx;

  mqidx = atoi (args);
  if (mqidx < 0 || mqidx >= MUSICQ_MAX) {
    return;
  }
  mainData->changeSuspend [mqidx] = value;
}

static void
mainMusicQueueMix (maindata_t *mainData, char *args)
{
  int           mqidx;
  dbidx_t       musicqLen;
  dbidx_t       dbidx;
  int           danceIdx;
  song_t        *song = NULL;
  nlist_t       *songList = NULL;
  nlist_t       *danceCounts = NULL;
  dancesel_t    *dancesel = NULL;
  songsel_t     *songsel = NULL;
  dance_t       *dances;
  int           totcount;
  int           currlen;

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  mqidx = mainMusicqIndexParse (mainData, args);

  danceCounts = nlistAlloc ("mq-mix-counts", LIST_ORDERED, NULL);
  songList = nlistAlloc ("mq-mix-song-list", LIST_ORDERED, NULL);

  musicqLen = musicqGetLen (mainData->musicQueue, mqidx);
  logMsg (LOG_DBG, LOG_BASIC, "mix: mq len: %d", musicqLen);
  totcount = 0;
  /* skip the empty head; there is no idx = musicqLen */
  for (int i = 1; i < musicqLen; ++i) {
    int   plidx;

    dbidx = musicqGetByIdx (mainData->musicQueue, mqidx, i);
    if (dbidx < 0) {
      continue;
    }
    plidx = musicqGetPlaylistIdx (mainData->musicQueue, mqidx, i);
    nlistSetNum (songList, dbidx, plidx);
    song = dbGetByIdx (mainData->musicdb, dbidx);
    danceIdx = songGetNum (song, TAG_DANCE);
    if (danceIdx >= 0) {
      nlistIncrement (danceCounts, danceIdx);
      ++totcount;
    } else {
      logMsg (LOG_DBG, LOG_BASIC, "mix: unknown dance skipped dbidx:%d", dbidx);
    }
  }
  logMsg (LOG_DBG, LOG_BASIC, "mix: total count: %d", totcount);
  logMsg (LOG_DBG, LOG_BASIC, "mix: counts len: %d", nlistGetCount (danceCounts));

  /* for the purposes of a mix, the countlist passed to the dancesel alloc */
  /* and the dance counts used for selection are identical */

  musicqClear (mainData->musicQueue, mqidx, 1);
  /* should already be an empty head on the queue */

  dancesel = danceselAlloc (danceCounts, mainMusicQueueLookup, mainData);
  songsel = songselAlloc (mainData->musicdb, danceCounts, songList, NULL);

  currlen = 0;
  while (currlen < totcount) {
    /* as there is always an empty head on the music queue, */
    /* the prior-index must point at currlen + 1 */
    danceIdx = danceselSelect (dancesel, currlen + 1);

    /* check and see if this dance is used up */
    if (nlistGetNum (danceCounts, danceIdx) == 0) {
      logMsg (LOG_DBG, LOG_MAIN, "mix: dance %d/%s at 0", danceIdx,
          danceGetStr (dances, danceIdx, DANCE_DANCE));
      continue;
    }

    song = songselSelect (songsel, danceIdx);
    if (song != NULL) {
      int   plidx;
      long  dur;

      songselSelectFinalize (songsel, danceIdx);
      logMsg (LOG_DBG, LOG_BASIC, "mix: (%d) d:%d/%s select: %s",
          currlen, danceIdx,
          danceGetStr (dances, danceIdx, DANCE_DANCE),
          songGetStr (song, TAG_FILE));
      dbidx = songGetNum (song, TAG_DBIDX);

      danceselAddCount (dancesel, danceIdx);
      danceselDecrementBase (dancesel, danceIdx);
      nlistDecrement (danceCounts, danceIdx);

      plidx = nlistGetNum (songList, dbidx);
      dur = mainCalculateSongDuration (mainData, song, plidx, mqidx, MAIN_CALC_WITH_SPEED);
      musicqPush (mainData->musicQueue, mqidx, dbidx, plidx, dur);
      ++currlen;
    }
  }

  songselFree (songsel);
  danceselFree (dancesel);
  nlistFree (danceCounts);
  nlistFree (songList);

  mainData->musicqChanged [mqidx] = MAIN_CHG_START;
  mainData->marqueeChanged [mqidx] = true;
}

static void
mainPlaybackFinishProcess (maindata_t *mainData, const char *args)
{
  mainMusicQueueNext (mainData, args);
  ++mainData->pbfinishrcv;
}

static void
mainPlaybackSendSongFinish (maindata_t *mainData, const char *args)
{
  char    tmp [40];
  int     flag;
  dbidx_t dbidx;

  flag = atoi (args);
  if (flag) {
    dbidx = musicqGetCurrent (mainData->musicQueue, mainData->musicqPlayIdx);
    snprintf (tmp, sizeof (tmp), "%d", dbidx);
    connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_SONG_FINISH, tmp);
  }
}

static void
mainStatusRequest (maindata_t *mainData, bdjmsgroute_t routefrom)
{
  char  tmp [40];

  for (int i = 0; i < MUSICQ_MAX; ++i) {
    mainData->musicqChanged [i] = MAIN_CHG_START;
  }
  snprintf (tmp, sizeof (tmp), "%d", mainData->musicqManageIdx);
  connSendMessage (mainData->conn, routefrom, MSG_MAIN_CURR_MANAGE, tmp);
  snprintf (tmp, sizeof (tmp), "%d", mainData->musicqPlayIdx);
  connSendMessage (mainData->conn, routefrom, MSG_MAIN_CURR_PLAY, tmp);
  mainSendMusicqStatus (mainData);
  /* send the last player state that has been recorded */
  snprintf (tmp, sizeof (tmp), "%d", mainData->playerState);
  connSendMessage (mainData->conn, routefrom, MSG_PLAYER_STATE, tmp);
}

static void
mainAddTemporarySong (maindata_t *mainData, char *args)
{
  char    *p;
  char    *tokstr;
  song_t  *song;
  dbidx_t tdbidx;
  dbidx_t dbidx;


  song = songAlloc ();
  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  if (p == NULL) {
    songFree (song);
    return;
  }
  songSetStr (song, TAG_FILE, p);

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (p == NULL) {
    songFree (song);
    return;
  }
  tdbidx = atol (p);
  songSetNum (song, TAG_DBIDX, tdbidx);

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (p == NULL) {
    songFree (song);
    return;
  }
  songParse (song, p, 0);
  songSetNum (song, TAG_TEMPORARY, true);

  dbidx = dbAddTemporarySong (mainData->musicdb, song);
  if (dbidx != tdbidx) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: databases out of sync");
  }
}

static time_t
mainCalcStopTime (time_t stopTime)
{
  time_t    currTime;
  time_t    nStopTime;

  if (stopTime <= 0) {
    return 0;
  }

  currTime = mstime ();
  /* stop time is in hours+minutes; need to convert it to real time */
  nStopTime = mstimestartofday ();
  nStopTime += stopTime;
  if (nStopTime < currTime) {
    nStopTime += (24 * 3600 * 1000);
  }

  return nStopTime;
}

static void
mainQueueInfoRequest (maindata_t *mainData, bdjmsgroute_t routefrom,
    const char *args)
{
  char        tbuff [200];
  char        *sbuff = NULL;
  int         musicqLen;
  slistidx_t  dbidx;
  song_t      *song;
  int         musicqidx;

  logProcBegin (LOG_PROC, "mainQueueInfoRequest");

  if (! *args) {
    return;
  }
  musicqidx = atoi (args);
  if (musicqidx < 0 || musicqidx >= MUSICQ_PB_MAX) {
    return;
  }

  musicqLen = musicqGetLen (mainData->musicQueue, musicqidx);

  sbuff = mdmalloc (BDJMSG_MAX);
  *sbuff = '\0';
  snprintf (sbuff, BDJMSG_MAX, "%d%c", musicqLen, MSG_ARGS_RS);

  for (int i = 0; i <= musicqLen; ++i) {
    dbidx = musicqGetByIdx (mainData->musicQueue, musicqidx, i);
    song = dbGetByIdx (mainData->musicdb, dbidx);
    if (song != NULL) {
      long        dur;
      int         plidx;
      int         gap;
      int         plgap;
      playlist_t  *playlist;

      snprintf (tbuff, sizeof (tbuff), "%d%c", dbidx, MSG_ARGS_RS);
      strlcat (sbuff, tbuff, BDJMSG_MAX);

      plidx = musicqGetPlaylistIdx (mainData->musicQueue, musicqidx, i);
      playlist = NULL;
      if (plidx >= 0) {
        playlist = nlistGetData (mainData->playlistCache, plidx);
      }

      dur = mainCalculateSongDuration (mainData, song, plidx, musicqidx, MAIN_CALC_NO_SPEED);
      snprintf (tbuff, sizeof (tbuff), "%ld%c", dur, MSG_ARGS_RS);
      strlcat (sbuff, tbuff, BDJMSG_MAX);

      gap = bdjoptGetNumPerQueue (OPT_Q_GAP, musicqidx);
      plgap = playlistGetConfigNum (playlist, PLAYLIST_GAP);
      if (plgap >= 0) {
        gap = plgap;
      }

      snprintf (tbuff, sizeof (tbuff), "%d%c", gap, MSG_ARGS_RS);
      strlcat (sbuff, tbuff, BDJMSG_MAX);
    }
  }

  connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_MAIN_QUEUE_INFO, sbuff);
  dataFree (sbuff);
  logProcEnd (LOG_PROC, "mainQueueInfoRequest", "");
}

static bool
mainCheckMusicQStopTime (maindata_t *mainData, time_t nStopTime)
{
  time_t  currTime;
  time_t  qDuration;
  bool    stopatflag = false;

  currTime = mstime ();
  qDuration = musicqGetDuration (mainData->musicQueue, mainData->musicqManageIdx);
  if (currTime + qDuration > nStopTime) {
    stopatflag = true;
  }

  return stopatflag;
}

static playlist_t *
mainNextPlaylist (maindata_t *mainData)
{
  playlistitem_t  *plitem = NULL;
  playlist_t      *playlist = NULL;

  plitem = queuePop (mainData->playlistQueue [mainData->musicqManageIdx]);
  mainPlaylistItemFree (plitem);
  plitem = queueGetFirst (mainData->playlistQueue [mainData->musicqManageIdx]);
  playlist = NULL;
  if (plitem != NULL) {
    playlist = plitem->playlist;
  }

  return playlist;
}

static void
mainChkMusicq (maindata_t *mainData, bdjmsgroute_t routefrom)
{
  char    tmp [2000];
  dbidx_t dbidx;
  dbidx_t qdbidx;
  dbidx_t mqdbidx [MUSICQ_PB_MAX][MAIN_TS_DEBUG_MAX];
  char    *title;
  char    *dance;
  song_t  *song;
  char    *songfn;
  int     bpm;

  dbidx = -1;
  qdbidx = -1;
  bpm = -1;
  title = MSG_ARGS_EMPTY_STR;
  dance = MSG_ARGS_EMPTY_STR;
  songfn = MSG_ARGS_EMPTY_STR;
  if (mainData->playerState == PL_STATE_PLAYING ||
      mainData->playerState == PL_STATE_IN_FADEOUT) {
    dbidx = musicqGetByIdx (mainData->musicQueue, mainData->musicqPlayIdx, 0);
    title = musicqGetData (mainData->musicQueue, mainData->musicqPlayIdx, 0, TAG_TITLE);
    dance = mainSongGetDanceDisplay (mainData, 0);
    song = dbGetByIdx (mainData->musicdb, dbidx);
    songfn = songGetStr (song, TAG_FILE);
    bpm = songGetNum (song, TAG_BPM);
  } else {
    qdbidx = musicqGetCurrent (mainData->musicQueue, mainData->musicqPlayIdx);
  }

  for (int mqidx = 0; mqidx < MUSICQ_PB_MAX; ++mqidx) {
    for (int idx = 0; idx < MAIN_TS_DEBUG_MAX; ++idx) {
      mqdbidx [mqidx][idx] = musicqGetByIdx (mainData->musicQueue, mqidx, idx);
    }
  }


  snprintf (tmp, sizeof (tmp),
      "mqmanage%c%d%c"
      "mqplay%c%d%c"
      "mq0len%c%d%c"
      "mq1len%c%d%c"
      "mq2len%c%d%c"
      "dbidx%c%d%c"
      "bpm%c%d%c"
      "qdbidx%c%d%c"
      "m-songfn%c%s%c"
      "title%c%s%c"
      "dance%c%s%c"
      "mq0idx0%c%d%c"
      "mq0idx1%c%d%c"
      "mq0idx2%c%d%c"
      "mq0idx3%c%d%c"
      "mq0idx4%c%d%c"
      "mq0idx5%c%d%c"
      "mq1idx0%c%d%c"
      "mq1idx1%c%d%c"
      "mq1idx2%c%d%c"
      "mq1idx3%c%d%c"
      "mq1idx4%c%d%c"
      "mq1idx5%c%d%c"
      "mq2idx0%c%d%c"
      "mq2idx1%c%d%c"
      "mq2idx2%c%d%c"
      "mq2idx3%c%d%c"
      "mq2idx4%c%d%c"
      "mq2idx5%c%d%c"
      "songplaysentcount%c%d",
      MSG_ARGS_RS, mainData->musicqManageIdx, MSG_ARGS_RS,
      MSG_ARGS_RS, mainData->musicqPlayIdx, MSG_ARGS_RS,
      MSG_ARGS_RS, musicqGetLen (mainData->musicQueue, 0), MSG_ARGS_RS,
      MSG_ARGS_RS, musicqGetLen (mainData->musicQueue, 1), MSG_ARGS_RS,
      MSG_ARGS_RS, musicqGetLen (mainData->musicQueue, 2), MSG_ARGS_RS,
      MSG_ARGS_RS, dbidx, MSG_ARGS_RS,
      MSG_ARGS_RS, bpm, MSG_ARGS_RS,
      MSG_ARGS_RS, qdbidx, MSG_ARGS_RS,
      MSG_ARGS_RS, songfn, MSG_ARGS_RS,
      MSG_ARGS_RS, title, MSG_ARGS_RS,
      MSG_ARGS_RS, dance, MSG_ARGS_RS,
      MSG_ARGS_RS, mqdbidx [0][0], MSG_ARGS_RS,
      MSG_ARGS_RS, mqdbidx [0][1], MSG_ARGS_RS,
      MSG_ARGS_RS, mqdbidx [0][2], MSG_ARGS_RS,
      MSG_ARGS_RS, mqdbidx [0][3], MSG_ARGS_RS,
      MSG_ARGS_RS, mqdbidx [0][4], MSG_ARGS_RS,
      MSG_ARGS_RS, mqdbidx [0][5], MSG_ARGS_RS,
      MSG_ARGS_RS, mqdbidx [1][0], MSG_ARGS_RS,
      MSG_ARGS_RS, mqdbidx [1][1], MSG_ARGS_RS,
      MSG_ARGS_RS, mqdbidx [1][2], MSG_ARGS_RS,
      MSG_ARGS_RS, mqdbidx [1][3], MSG_ARGS_RS,
      MSG_ARGS_RS, mqdbidx [1][4], MSG_ARGS_RS,
      MSG_ARGS_RS, mqdbidx [1][5], MSG_ARGS_RS,
      MSG_ARGS_RS, mqdbidx [2][0], MSG_ARGS_RS,
      MSG_ARGS_RS, mqdbidx [2][1], MSG_ARGS_RS,
      MSG_ARGS_RS, mqdbidx [2][2], MSG_ARGS_RS,
      MSG_ARGS_RS, mqdbidx [2][3], MSG_ARGS_RS,
      MSG_ARGS_RS, mqdbidx [2][4], MSG_ARGS_RS,
      MSG_ARGS_RS, mqdbidx [2][5], MSG_ARGS_RS,
      MSG_ARGS_RS, mainData->songplaysentcount);
  connSendMessage (mainData->conn, routefrom, MSG_CHK_MAIN_MUSICQ, tmp);
}
