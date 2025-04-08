/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
#include <math.h>

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
#include "dispsel.h"
#include "filedata.h"
#include "fileop.h"
#include "grouping.h"
#include "lock.h"
#include "log.h"
#include "mdebug.h"
#include "msgparse.h"
#include "musicdb.h"
#include "musicq.h"
#include "osenv.h"
#include "osprocess.h"
#include "ossignal.h"
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
  uint32_t          startflags;
  musicdb_t         *musicdb;
  /* playlistCache contains all of the playlists that are used */
  /* so that playlist lookups can be processed */
  nlist_t           *playlistCache;
  queue_t           *playlistQueue [MUSICQ_MAX];
  musicq_t          *musicQueue;
  dispsel_t         *dispsel;
  slist_t           *announceList;
  char              *mobmqUserkey;
  grouping_t        *grouping;
  int               musicqPlayIdx;
  int               musicqDeferredPlayIdx;
  int               musicqLookupIdx;
  playerstate_t     playerState;
  int               stopwaitcount;
  int32_t           ploverridestoptime;
  int               songplaysentcount;        // for testsuite
  int               musicqChanged [MUSICQ_MAX];
  bool              changeSuspend [MUSICQ_MAX];
  time_t            stopTime [MUSICQ_MAX];
  time_t            nStopTime [MUSICQ_MAX];
  mstime_t          startWaitCheck;
  int32_t           startwaitTime;
  int32_t           lastGapSent;
  bool              switchQueueWhenEmpty;
  bool              finished;
  bool              marqueestarted;
  bool              waitforpbfinish;
  bool              marqueeChanged;
  bool              inannounce;
  bool              inStartWait;
} maindata_t;

static int  mainProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route, bdjmsgmsg_t msg, char *args, void *udata);
static int  mainProcessing (void *udata);
static bool mainListeningCallback (void *tmaindata, programstate_t programState);
static bool mainConnectingCallback (void *tmaindata, programstate_t programState);
static bool mainHandshakeCallback (void *tmaindata, programstate_t programState);
static void mainStartMarquee (maindata_t *mainData);
static void mainStopMarquee (maindata_t *mainData);
static bool mainStoppingCallback (void *tmaindata, programstate_t programState);
static bool mainStopWaitCallback (void *tmaindata, programstate_t programState);
static bool mainClosingCallback (void *tmaindata, programstate_t programState);
static void mainSendMusicQueueData (maindata_t *mainData, int musicqidx);
static void mainSendMarqueeData (maindata_t *mainData);
static const char * mainSongGetDanceDisplay (maindata_t *mainData, int mqidx, int idx);
static void mainQueueClear (maindata_t *mainData, char *args);
static void mainQueueDance (maindata_t *mainData, char *args);
static void mainQueuePlaylist (maindata_t *mainData, char *plname);
static void mainSigHandler (int sig);
static void mainMusicQueueFill (maindata_t *mainData, int mqidx);
static void mainMusicQueuePrep (maindata_t *mainData, int mqidx);
static void mainMusicqClearPreppedSongs (maindata_t *mainData, int mqidx, int idx);
static void mainMusicqClearPrep (maindata_t *mainData, int mqidx, int idx);
static const char *mainPrepSong (maindata_t *maindata, int flag, int mqidx, song_t *song, const char *sfname, int playlistIdx, int32_t uniqueidx);
static void mainPlaylistClearQueue (maindata_t *mainData, char *args);
static void mainTogglePause (maindata_t *mainData, char *args);
static void mainMusicqMove (maindata_t *mainData, char *args, mainmove_t direction);
static void mainMusicqMoveTop (maindata_t *mainData, char *args);
static void mainMusicqClear (maindata_t *mainData, char *args);
static void mainMusicqRemove (maindata_t *mainData, char *args);
static void mainMusicqSwap (maindata_t *mainData, char *args);
static void mainNextSongPlay (maindata_t *mainData);
static void mainMusicqInsert (maindata_t *mainData, bdjmsgroute_t route, char *args);
static void mainMusicqSetPlayback (maindata_t *mainData, char *args);
static void mainMusicqSendQueueConfig (maindata_t *mainData);
static void mainMusicqSwitch (maindata_t *mainData, int newidx);
static void mainPlaybackBegin (maindata_t *mainData);
static void mainMusicQueuePlay (maindata_t *mainData);
static void mainMusicQueueFinish (maindata_t *mainData, const char *args);
static void mainMusicQueueNext (maindata_t *mainData, const char *args);
static ilistidx_t mainMusicQueueLookup (void *mainData, ilistidx_t idx);
static void mainSendDanceList (maindata_t *mainData, bdjmsgroute_t route);
static void mainSendPlaylistNames (maindata_t *mainData, bdjmsgroute_t route);
static void mainSendPlayerStatus (maindata_t *mainData, char *playerResp);
static void mainSendRemctrlData (maindata_t *mainData);
static void mainSendMusicqStatus (maindata_t *mainData);
static int  mainMusicqIndexNumParse (maindata_t *mainData, char *args, ilistidx_t *b, ilistidx_t *c);
static int  mainParseQueuePlaylist (maindata_t *mainData, char *args, char **b, int *editmode);
static int  mainMusicqIndexParse (maindata_t *mainData, const char *p);
static void mainSendFinished (maindata_t *mainData);
static int32_t mainCalculateSongDuration (maindata_t *mainData, song_t *song, int playlistIdx, int mqidx, int speedCalcFlag);
static playlistitem_t * mainPlaylistItemCache (maindata_t *mainData, playlist_t *pl, int playlistIdx);
static void mainPlaylistItemFree (void *tplitem);
static void mainMusicqSetSuspend (maindata_t *mainData, char *args, bool value);
static void mainMusicQueueMix (maindata_t *mainData, char *args);
static void mainPlaybackFinishStopProcess (maindata_t *mainData, const char *args);
static void mainPlaybackSendSongFinish (maindata_t *mainData, const char *args);
static void mainStatusRequest (maindata_t *mainData, bdjmsgroute_t routefrom);
static void mainAddTemporarySong (maindata_t *mainData, char *args);
static time_t mainCalcStopTime (time_t stopTime);
static void mainSendPlaybackGap (maindata_t *mainData);
static int  mainLocateQueueWithSongs (maindata_t *mainData);
static int32_t mainGetGap (maindata_t *mainData, int mqidx, int index);
static void mainSendPlayerUISwitchQueue (maindata_t *mainData, int mqidx);
static playlist_t * mainNextPlaylist (maindata_t *mainData, int mqidx);
static void mainSetMusicQueuesChanged (maindata_t *mainData);
static bool mainGetAnnounceFlag (maindata_t *mainData, int mqidx, int playlistIdx);
/* routines for testing */
static void mainQueueInfoRequest (maindata_t *mainData, bdjmsgroute_t routefrom, const char *args);
static bool mainCheckMusicQStopTime (maindata_t *mainData, time_t nStopTime, int mqidx);
static void mainChkMusicq (maindata_t *mainData, bdjmsgroute_t routefrom);
static void mainProcessPlayerState (maindata_t *mainData, char *data);

static int32_t globalCounter = 0;
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
  mainData.dispsel = NULL;
  mainData.musicqPlayIdx = MUSICQ_PB_A;
  mainData.musicqDeferredPlayIdx = MAIN_NOT_SET;
  mainData.musicqLookupIdx = MUSICQ_PB_A;
  mainData.playerState = PL_STATE_STOPPED;
  mainData.mobmqUserkey = NULL;
  mainData.switchQueueWhenEmpty = false;
  mainData.finished = false;
  mainData.marqueestarted = false;
  mainData.waitforpbfinish = false;
  /* wait for a stop message, and a playback-finish message */
  mainData.stopwaitcount = 0;
  mainData.ploverridestoptime = 0;
  mainData.songplaysentcount = 0;
  mainData.marqueeChanged = false;
  mainData.inannounce = false;
  mainData.inStartWait = false;
  mainData.lastGapSent = -1;
  for (musicqidx_t i = 0; i < MUSICQ_MAX; ++i) {
    mainData.playlistQueue [i] = NULL;
    mainData.musicqChanged [i] = MAIN_CHG_CLEAR;
    mainData.changeSuspend [i] = false;
    mainData.stopTime [i] = 0;
    mainData.nStopTime [i] = 0;
  }

  procutilInitProcesses (mainData.processes);
  osSetStandardSignals (mainSigHandler);

  mainData.startflags = BDJ4_INIT_ALL;
  bdj4startup (argc, argv, &mainData.musicdb,
      "main", ROUTE_MAIN, &mainData.startflags);
  logProcBegin ();
  mainData.grouping = groupingAlloc (mainData.musicdb);

  mainData.dispsel = dispselAlloc (DISP_SEL_LOAD_MARQUEE);

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

  listenPort = bdjvarsGetNum (BDJVL_PORT_MAIN);
  sockhMainLoop (listenPort, mainProcessMsg, mainProcessing, &mainData);
  connFree (mainData.conn);
  progstateFree (mainData.progstate);
  logProcEnd ("");
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

  logProcBegin ();

  procutilStopAllProcess (mainData->processes, mainData->conn, PROCUTIL_NORM_TERM);
  connDisconnectAll (mainData->conn);
  logProcEnd ("");
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

  logProcBegin ();

  groupingFree (mainData->grouping);
  nlistFree (mainData->playlistCache);
  for (musicqidx_t i = 0; i < MUSICQ_MAX; ++i) {
    if (mainData->playlistQueue [i] != NULL) {
      queueFree (mainData->playlistQueue [i]);
      mainData->playlistQueue [i] = NULL;
    }
  }
  if (mainData->musicQueue != NULL) {
    musicqFree (mainData->musicQueue);
    mainData->musicQueue = NULL;
  }
  slistFree (mainData->announceList);
  dataFree (mainData->mobmqUserkey);
  dispselFree (mainData->dispsel);

  procutilStopAllProcess (mainData->processes, mainData->conn, PROCUTIL_FORCE_TERM);
  procutilFreeAll (mainData->processes);

  bdj4shutdown (ROUTE_MAIN, mainData->musicdb);

  logProcEnd ("");
  return STATE_FINISHED;
}

static int
mainProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  maindata_t  *mainData;

  mainData = (maindata_t *) udata;

  if (msg != MSG_PLAYER_STATUS_DATA) {
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
          mainQueueDance (mainData, args);
          break;
        }
        case MSG_QUEUE_SWITCH_EMPTY: {
          mainData->switchQueueWhenEmpty = atoi (args);
          break;
        }
        case MSG_QUEUE_MIX: {
          mainMusicQueueMix (mainData, args);
          break;
        }
        case MSG_CMD_PLAY: {
          if (mainData->playerState != PL_STATE_PLAYING &&
              mainData->playerState != PL_STATE_IN_FADEOUT) {
            mainMusicQueuePlay (mainData);
          }
          break;
        }
        case MSG_CMD_NEXTSONG_PLAY: {
          mainNextSongPlay (mainData);
          break;
        }
        case MSG_CMD_PLAYPAUSE: {
          if (mainData->playerState == PL_STATE_PLAYING ||
              mainData->playerState == PL_STATE_IN_FADEOUT) {
            connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_PLAY_PAUSE, NULL);
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
        case MSG_PLAYBACK_FINISH_STOP: {
          mainPlaybackFinishStopProcess (mainData, args);
          if (mainData->waitforpbfinish) {
            mainMusicQueuePlay (mainData);
            mainData->waitforpbfinish = false;
          }
          break;
        }
        case MSG_PLAYBACK_FINISH: {
          mainMusicQueueNext (mainData, args);
          if (mainData->waitforpbfinish) {
            mainMusicQueuePlay (mainData);
            mainData->waitforpbfinish = false;
          }
          break;
        }
        case MSG_PL_OVERRIDE_STOP_TIME: {
          /* this message overrides the stop time for the */
          /* next queue-playlist message */
          mainData->ploverridestoptime = atol (args);
          break;
        }
        case MSG_PL_CLEAR_QUEUE: {
          mainPlaylistClearQueue (mainData, args);
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
        case MSG_MUSICQ_SWAP: {
          mainMusicqSwap (mainData, args);
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
        case MSG_MUSICQ_SET_LEN: {
          /* a temporary setting used for the song list editor */
          bdjoptSetNum (OPT_G_PLAYERQLEN, atol (args));
          break;
        }
        case MSG_MUSICQ_DATA_SUSPEND: {
          mainMusicqSetSuspend (mainData, args, true);
          break;
        }
        case MSG_MUSICQ_DATA_RESUME: {
          mainMusicqSetSuspend (mainData, args, false);
          break;
        }
        case MSG_PLAYER_ANN_FINISHED: {
          mainData->inannounce = false;
          break;
        }
        case MSG_PLAYER_STATE: {
          mainProcessPlayerState (mainData, args);
          break;
        }
        case MSG_GET_DANCE_LIST: {
          mainSendDanceList (mainData, routefrom);
          break;
        }
        case MSG_GET_PLAYLIST_LIST: {
          mainSendPlaylistNames (mainData, routefrom);
          break;
        }
        case MSG_PLAYER_STATUS_DATA: {
          mainSendPlayerStatus (mainData, args);
          mainSendRemctrlData (mainData);
          break;
        }
        case MSG_START_MARQUEE: {
          mainStartMarquee (mainData);
          break;
        }
        case MSG_STOP_MARQUEE: {
          mainStopMarquee (mainData);
          break;
        }
        case MSG_DB_ENTRY_UPDATE: {
          dbidx_t   dbidx;

          msgparseDBEntryUpdate (args, &dbidx);
          dbLoadEntry (mainData->musicdb, dbidx);
          groupingRebuild (mainData->grouping, mainData->musicdb);
          mainSetMusicQueuesChanged (mainData);
          break;
        }
        case MSG_DB_ENTRY_REMOVE: {
          dbMarkEntryRemoved (mainData->musicdb, atol (args));
          mainSetMusicQueuesChanged (mainData);
          break;
        }
        case MSG_DB_ENTRY_UNREMOVE: {
          dbClearEntryRemoved (mainData->musicdb, atol (args));
          mainSetMusicQueuesChanged (mainData);
          break;
        }
        case MSG_DATABASE_UPDATE: {
          mainData->musicdb = bdj4ReloadDatabase (mainData->musicdb);
          musicqSetDatabase (mainData->musicQueue, mainData->musicdb);
          mainSetMusicQueuesChanged (mainData);
          break;
        }
        case MSG_MAIN_REQ_STATUS: {
          mainStatusRequest (mainData, routefrom);
          break;
        }
        case MSG_MAIN_REQ_QUEUE_INFO: {
          mainQueueInfoRequest (mainData, routefrom, args);
          break;
        }
        case MSG_DB_ENTRY_TEMP_ADD: {
          mainAddTemporarySong (mainData, args);
          break;
        }
        case MSG_CHK_MAIN_MUSICQ: {
          mainChkMusicq (mainData, routefrom);
          break;
        }
        case MSG_CHK_MAIN_RESET_SENT: {
          mainData->songplaysentcount = 0;
          break;
        }
        case MSG_CHK_MAIN_SET_GAP: {
          bdjoptSetNumPerQueue (OPT_Q_GAP, atoi (args), mainData->musicqPlayIdx);
          mainMusicqSendQueueConfig (mainData);
          break;
        }
        case MSG_CHK_MAIN_SET_MAXPLAYTIME: {
          bdjoptSetNumPerQueue (OPT_Q_MAXPLAYTIME, atol (args), mainData->musicqPlayIdx);
          break;
        }
        case MSG_CHK_MAIN_SET_STOPATTIME: {
          mainData->stopTime [mainData->musicqPlayIdx] = atol (args);
          mainData->nStopTime [mainData->musicqPlayIdx] =
              mainCalcStopTime (mainData->stopTime [mainData->musicqPlayIdx]);
          break;
        }
        case MSG_CHK_MAIN_SET_PLAYANNOUNCE: {
          bdjoptSetNumPerQueue (OPT_Q_PLAY_ANNOUNCE, atoi (args), mainData->musicqPlayIdx);
          break;
        }
        case MSG_CHK_MAIN_SET_QUEUE_ACTIVE: {
          bdjoptSetNumPerQueue (OPT_Q_ACTIVE, atoi (args), mainData->musicqPlayIdx);
          break;
        }
        case MSG_CHK_MAIN_SET_PLAY_WHEN_QUEUED: {
          bdjoptSetNumPerQueue (OPT_Q_PLAY_WHEN_QUEUED, atoi (args), mainData->musicqPlayIdx);
          break;
        }
        case MSG_CHK_MAIN_SET_PAUSE_EACH_SONG: {
          bdjoptSetNumPerQueue (OPT_Q_PAUSE_EACH_SONG, atoi (args), mainData->musicqPlayIdx);
          break;
        }
        case MSG_CHK_MAIN_SET_STARTWAIT: {
          bdjoptSetNumPerQueue (OPT_Q_START_WAIT_TIME, atol (args), mainData->musicqPlayIdx);
          break;
        }
        case MSG_CHK_MAIN_SET_FADEIN: {
          bdjoptSetNumPerQueue (OPT_Q_FADEINTIME, atoi (args), mainData->musicqPlayIdx);
          mainMusicqSendQueueConfig (mainData);
          break;
        }
        case MSG_CHK_MAIN_SET_FADEOUT: {
          bdjoptSetNumPerQueue (OPT_Q_FADEOUTTIME, atoi (args), mainData->musicqPlayIdx);
          mainMusicqSendQueueConfig (mainData);
          break;
        }
        case MSG_CHK_CLEAR_PREP_Q: {
          /* clear any prepped announcements */
          slistFree (mainData->announceList);
          mainData->announceList = slistAlloc ("announcements", LIST_ORDERED, NULL);
          connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_CHK_CLEAR_PREP_Q, NULL);
          break;
        }
        case MSG_CHK_MAIN_SET_DANCESEL_METHOD: {
          int   method;

          /* currently not in use by the test suite */
          method = DANCESEL_METHOD_WINDOWED;
          if (strcmp (args, "windowed") == 0) {
            method = DANCESEL_METHOD_WINDOWED;
          }
          bdjoptSetNum (OPT_G_DANCESEL_METHOD, method);
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
  int         stop = SOCKH_CONTINUE;

  if (! progstateIsRunning (mainData->progstate)) {
    progstateProcess (mainData->progstate);
    if (progstateCurrState (mainData->progstate) == STATE_CLOSED) {
      stop = SOCKH_STOP;
    }
    if (gKillReceived) {
      progstateShutdownProcess (mainData->progstate);
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal (a)");
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

  if (mainData->marqueeChanged) {
    mainSendMarqueeData (mainData);
    mainSendRemctrlData (mainData);
    if (mainData->finished && mainData->playerState == PL_STATE_STOPPED) {
      mainData->finished = false;
    }
  }

  if (mainData->playerState == PL_STATE_STOPPED &&
      mainData->inStartWait) {
    if (mstimeCheck (&mainData->startWaitCheck)) {
      /* music-queue-play turns off in-start-wait */
      mainMusicQueuePlay (mainData);
    }
  }

  if (gKillReceived) {
    progstateShutdownProcess (mainData->progstate);
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal (b)");
  }
  return stop;
}

static bool
mainListeningCallback (void *tmaindata, programstate_t programState)
{
  maindata_t    *mainData = tmaindata;
  int           flags;

  logProcBegin ();

  flags = PROCUTIL_DETACH;
  if ((mainData->startflags & BDJ4_INIT_NO_DETACH) == BDJ4_INIT_NO_DETACH) {
    flags = PROCUTIL_NO_DETACH;
  }

  if ((mainData->startflags & BDJ4_INIT_NO_START) != BDJ4_INIT_NO_START) {
    mainData->processes [ROUTE_PLAYER] = procutilStartProcess (
        ROUTE_PLAYER, "bdj4player", flags, NULL);
    if (bdjoptGetNum (OPT_P_MOBMQ_TYPE) != MOBMQ_TYPE_OFF) {
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

  logProcEnd ("");
  return STATE_FINISHED;
}

static bool
mainConnectingCallback (void *tmaindata, programstate_t programState)
{
  maindata_t    *mainData = tmaindata;
  int           connMax = 0;
  int           connCount = 0;
  bool          rc = STATE_NOT_FINISH;

  logProcBegin ();

  connProcessUnconnected (mainData->conn);

  if ((mainData->startflags & BDJ4_INIT_NO_START) != BDJ4_INIT_NO_START) {
    if (! connIsConnected (mainData->conn, ROUTE_PLAYER)) {
      connConnect (mainData->conn, ROUTE_PLAYER);
    }
    if (bdjoptGetNum (OPT_P_MOBMQ_TYPE) != MOBMQ_TYPE_OFF) {
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
  if (bdjoptGetNum (OPT_P_MOBMQ_TYPE) != MOBMQ_TYPE_OFF) {
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

  logProcEnd ("");
  return rc;
}

static bool
mainHandshakeCallback (void *tmaindata, programstate_t programState)
{
  maindata_t    *mainData = tmaindata;
  bool          rc = STATE_NOT_FINISH;
  int           conn = 0;

  logProcBegin ();

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
    /* send the player the fade-in and fade-out times for the playback queue */
    mainMusicqSendQueueConfig (mainData);
    /* also send the gap */
    mainSendPlaybackGap (mainData);

    connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_MAIN_READY, NULL);
    rc = STATE_FINISHED;
  }

  logProcEnd ("");
  return rc;
}

static void
mainStartMarquee (maindata_t *mainData)
{
  const char  *targv [2];
  int         idx = 0;
  int         flags = 0;

  if (mainData->marqueestarted) {
    return;
  }

  if (bdjoptGetNum (OPT_P_MARQUEE_SHOW) == MARQUEE_SHOW_OFF) {
    return;
  }

#if BDJ4_UI_GTK3 || BDJ4_UI_GTK4
  /* set the theme for the marquee */
  {
    const char *theme;
    theme = bdjoptGetStr (OPT_M_MQ_THEME);
    osSetEnv ("GTK_THEME", theme);
  }
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
mainStopMarquee (maindata_t *mainData)
{
  if (! mainData->marqueestarted) {
    return;
  }

  if (bdjoptGetNum (OPT_P_MARQUEE_SHOW) == MARQUEE_SHOW_OFF) {
    return;
  }

  procutilStopProcess (mainData->processes [ROUTE_MARQUEE],
      mainData->conn, ROUTE_MARQUEE, PROCUTIL_NORM_TERM);
  mainData->marqueestarted = false;
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
  int32_t     uniqueidx;
  int64_t     qDuration;
  char        *p;
  char        *end;


  logProcBegin ();

  musicqLen = musicqGetLen (mainData->musicQueue, musicqidx);

  qDuration = musicqGetDuration (mainData->musicQueue, musicqidx);
  dbidx = musicqGetByIdx (mainData->musicQueue, musicqidx, 0);

  sbuff = mdmalloc (BDJMSG_MAX);
  snprintf (sbuff, BDJMSG_MAX, "%d%c%" PRId64 "%c%" PRId32 "%c",
      musicqidx, MSG_ARGS_RS, qDuration, MSG_ARGS_RS,
      dbidx, MSG_ARGS_RS);
  p = sbuff + strlen (sbuff);
  end = sbuff + BDJMSG_MAX;

  /* main keeps the current song in queue position 0 */
  for (int i = 1; i < musicqLen; ++i) {
    dbidx = musicqGetByIdx (mainData->musicQueue, musicqidx, i);
    song = dbGetByIdx (mainData->musicdb, dbidx);

    if (song == NULL) {
      continue;
    }

    dispidx = musicqGetDispIdx (mainData->musicQueue, musicqidx, i);
    snprintf (tbuff, sizeof (tbuff), "%d%c", dispidx, MSG_ARGS_RS);
    p = stpecpy (p, end, tbuff);
    uniqueidx = musicqGetUniqueIdx (mainData->musicQueue, musicqidx, i);
    snprintf (tbuff, sizeof (tbuff), "%" PRId32 "%c", uniqueidx, MSG_ARGS_RS);
    p = stpecpy (p, end, tbuff);
    snprintf (tbuff, sizeof (tbuff), "%" PRId32 "%c", dbidx, MSG_ARGS_RS);
    p = stpecpy (p, end, tbuff);
    flags = musicqGetFlags (mainData->musicQueue, musicqidx, i);
    pauseind = false;
    if ((flags & MUSICQ_FLAG_PAUSE) == MUSICQ_FLAG_PAUSE) {
      pauseind = true;
    }
    snprintf (tbuff, sizeof (tbuff), "%d%c", pauseind, MSG_ARGS_RS);
    p = stpecpy (p, end, tbuff);
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
  logProcEnd ("");
}

static void
mainSendMarqueeData (maindata_t *mainData)
{
  char        tbuff [300];
  char        *sbuff = NULL;
  const char  *dstr = NULL;
  int         mqLen;
  int         mqidx;
  int         musicqLen;
  time_t      currTime;
  time_t      qdur = 0;
  int         lastmqidx;
  int         marqueeidx;
  int         qoffset;
  char        *jbuff = NULL;
  bool        marqueeactive = false;
  bool        mobmarqueeactive = false;
  char        *jp = NULL;
  char        *jend = NULL;
  char        *sp = NULL;
  char        *send = NULL;

  logProcBegin ();
  mainData->marqueeChanged = false;

  if (bdjoptGetNum (OPT_P_MOBMQ_TYPE) != MOBMQ_TYPE_OFF) {
    mobmarqueeactive = true;
    jbuff = mdmalloc (BDJMSG_MAX);
    jbuff [0] = '\0';
    jp = jbuff;
    jend = jbuff + BDJMSG_MAX;
  }
  if (mainData->marqueestarted) {
    marqueeactive = true;
  }

  if (! marqueeactive && ! mobmarqueeactive) {
    logProcEnd ("not-started");
    return;
  }

  mqLen = bdjoptGetNum (OPT_P_MQQLEN);
  mqidx = mainData->musicqPlayIdx;
  musicqLen = 0;
  if (mainData->musicQueue != NULL) {
    musicqLen = musicqGetLen (mainData->musicQueue, mqidx);
  }

  if (mobmarqueeactive) {
    const char  *title = NULL;

    title = bdjoptGetStr (OPT_P_MOBMQ_TITLE);
    if (title == NULL) {
      title = "";
    }
    jp = stpecpy (jp, jend, "{ ");
    snprintf (tbuff, sizeof (tbuff), "\"mqlen\" : \"%d\", ", mqLen);
    jp = stpecpy (jp, jend, tbuff);
    snprintf (tbuff, sizeof (tbuff), "\"title\" : \"%s\"", title);
    jp = stpecpy (jp, jend, tbuff);
  }

  if (mainData->playerState == PL_STATE_STOPPED &&
      mainData->finished) {
    logMsg (LOG_DBG, LOG_INFO, "sending finished");
    if (marqueeactive) {
      mainSendFinished (mainData);
    }
    if (mobmarqueeactive) {
      /* special case to finalize the mobile marquee display */
      snprintf (tbuff, sizeof (tbuff), "\"current\" : \"%s\"",
          bdjoptGetStr (OPT_P_COMPLETE_MSG));
      jp = stpecpy (jp, jend, ", ");
      jp = stpecpy (jp, jend, tbuff);
      jp = stpecpy (jp, jend, ", ");
      jp = stpecpy (jp, jend, "\"skip\" : \"true\"");
      jp = stpecpy (jp, jend, " }");
      connSendMessage (mainData->conn, ROUTE_MOBILEMQ, MSG_MARQUEE_DATA, jbuff);
      dataFree (jbuff);
    }
    logProcEnd ("finished");
    return;
  }

  if (marqueeactive) {
    sbuff = mdmalloc (BDJMSG_MAX);
    sbuff [0] = '\0';
    sp = sbuff;
    send = sbuff + BDJMSG_MAX;
  }

  currTime = mstime ();
  marqueeidx = 0;
  qoffset = 0;
  lastmqidx = -1;
  while (marqueeidx <= mqLen) {
    dbidx_t   dbidx;
    song_t    *song;
    bool      docheck = false;

    if (mainData->stopTime [mqidx] > 0 &&
        (currTime + qdur) > mainData->nStopTime [mqidx]) {
      /* process stoptime for the marquee display */
      dstr = MSG_ARGS_EMPTY_STR;
      if (lastmqidx != mainData->musicqPlayIdx) {
        docheck = true;
      }
    } else if ((marqueeidx > 0 && mainData->inannounce) ||
        (marqueeidx > 0 && mainData->playerState == PL_STATE_IN_GAP) ||
        (marqueeidx > 1 && mainData->playerState == PL_STATE_IN_FADEOUT)) {
      /* inannounce and marqueeidx > 0 or */
      /* in-gap and marqueeidx > 0 or */
      /* in-fadeout and marqueeidx > 1 */
      dstr = MSG_ARGS_EMPTY_STR;
    } else if (qoffset >= musicqLen) {
      dstr = MSG_ARGS_EMPTY_STR;
      if (lastmqidx != mainData->musicqPlayIdx) {
        docheck = true;
      }
    } else {
      dstr = mainSongGetDanceDisplay (mainData, mqidx, qoffset);
    }

    if (mainData->musicqDeferredPlayIdx != MAIN_NOT_SET) {
      mqidx = mainData->musicqDeferredPlayIdx;
    }

    /* if the queue is empty (or will be empty), and */
    /* switch-queue-when-empty is on */
    /* check the next queue for display */
    if (docheck && mainData->switchQueueWhenEmpty) {
      lastmqidx = mqidx;
      mqidx = musicqNextQueue (mqidx);
      musicqLen = 0;
      if (mainData->musicQueue != NULL) {
        musicqLen = musicqGetLen (mainData->musicQueue, mqidx);
      }
      while (mqidx != mainData->musicqPlayIdx && musicqLen == 0) {
        mqidx = musicqNextQueue (mqidx);
        musicqLen = 0;
        if (mainData->musicQueue != NULL) {
          musicqLen = musicqGetLen (mainData->musicQueue, mqidx);
        }
      }
      if (mqidx != mainData->musicqPlayIdx) {
        /* offset 0 is not active if this is not the queue set for playback */
        qoffset = 1;
        continue;
      }
    }

    /* queue duration data needed for stoptime check */
    dbidx = musicqGetByIdx (mainData->musicQueue, mqidx, qoffset);
    if (dbidx >= 0) {
      int   plidx;

      song = dbGetByIdx (mainData->musicdb, dbidx);
      if (song != NULL) {
        plidx = musicqGetPlaylistIdx (mainData->musicQueue, mqidx, qoffset);
        qdur += mainCalculateSongDuration (mainData, song, plidx, mqidx, MAIN_CALC_WITH_SPEED);
      }
    }

    if (marqueeactive) {
      /* dance display */
      snprintf (tbuff, sizeof (tbuff), "%s%c", dstr, MSG_ARGS_RS);
      sp = stpecpy (sp, send, tbuff);
    }

    if (mobmarqueeactive) {
      if (marqueeidx == 0) {
        snprintf (tbuff, sizeof (tbuff), "\"current\" : \"%s\"", dstr);
      } else {
        if (*dstr == MSG_ARGS_EMPTY) {
          dstr = "";
        }
        snprintf (tbuff, sizeof (tbuff), "\"mq%d\" : \"%s\"", marqueeidx, dstr);
      }
      jp = stpecpy (jp, jend, ", ");
      jp = stpecpy (jp, jend, tbuff);
    }

    ++marqueeidx;
    ++qoffset;
  }

  if (marqueeactive) {
    const char  *tstr;
    slist_t     *sellist;
    slistidx_t  seliteridx;
    int         tagidx;
    int         mqidx;

    mqidx = mainData->musicqPlayIdx;
    sellist = dispselGetList (mainData->dispsel, DISP_SEL_MARQUEE);

    slistStartIterator (sellist, &seliteridx);
    while ((tagidx = slistIterateValueNum (sellist, &seliteridx)) >= 0) {
      tstr = musicqGetData (mainData->musicQueue, mqidx, 0, tagidx);
      if (tstr == NULL || ! *tstr) {
        tstr = MSG_ARGS_EMPTY_STR;
      }
      snprintf (tbuff, sizeof (tbuff), "%s%c", tstr, MSG_ARGS_RS);
      sp = stpecpy (sp, send, tbuff);
    }

    connSendMessage (mainData->conn, ROUTE_MARQUEE, MSG_MARQUEE_DATA, sbuff);
    dataFree (sbuff);
  }

  if (mobmarqueeactive) {
    if (marqueeidx == 0) {
      jp = stpecpy (jp, jend, ", ");
      jp = stpecpy (jp, jend, "\"skip\" : \"true\"");
    }
    jp = stpecpy (jp, jend, " }");
    connSendMessage (mainData->conn, ROUTE_MOBILEMQ, MSG_MARQUEE_DATA, jbuff);
    dataFree (jbuff);
  }

  logProcEnd ("");
}

static const char *
mainSongGetDanceDisplay (maindata_t *mainData, int mqidx, int idx)
{
  const char  *tstr;
  const char  *dstr;
  dbidx_t     dbidx;
  song_t      *song;

  logProcBegin ();

  tstr = NULL;
  dbidx = musicqGetByIdx (mainData->musicQueue, mqidx, idx);
  song = dbGetByIdx (mainData->musicdb, dbidx);
  if (song != NULL) {
    /* if the song has an unknown dance, the marquee display */
    /* will be filled in with the dance name. */
    tstr = songGetStr (song, TAG_MQDISPLAY);
  }
  if (tstr != NULL && *tstr) {
    dstr = tstr;
  } else {
    dstr = musicqGetDance (mainData->musicQueue, mqidx, idx);
  }
  if (dstr == NULL) {
    dstr = MSG_ARGS_EMPTY_STR;
  }

  logProcEnd ("");
  return dstr;
}


/* clears both the playlist and music queues */
static void
mainQueueClear (maindata_t *mainData, char *args)
{
  int   mi;
  char  *p;
  char  *tokstr = NULL;

  logProcBegin ();

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
  mainData->marqueeChanged = true;
  logProcEnd ("");
}

static void
mainQueueDance (maindata_t *mainData, char *args)
{
  playlistitem_t  *plitem;
  playlist_t      *playlist;
  int             mi;
  ilistidx_t      danceIdx;
  int             count;
  ilistidx_t      musicqLen;
  bool            playwhenqueued;

  logProcBegin ();

  /* get the musicq length before any songs are added */
  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);

  mi = mainMusicqIndexNumParse (mainData, args, &danceIdx, &count);

  logMsg (LOG_DBG, LOG_BASIC, "queue dance %d %d %d", mi, danceIdx, count);
  /* CONTEXT: player: the name of the special playlist for queueing a dance */
  if ((playlist = playlistLoad (_("QueueDance"), mainData->musicdb, mainData->grouping)) == NULL) {
    playlist = playlistCreate ("main_queue_dance", PLTYPE_AUTO, mainData->musicdb, mainData->grouping);
  }
  playlistSetConfigNum (playlist, PLAYLIST_STOP_AFTER, count);
  /* clear all dance selected/counts */
  playlistResetAll (playlist);
  /* this will also set 'selected' */
  playlistSetDanceCount (playlist, danceIdx, 1);
  logMsg (LOG_DBG, LOG_BASIC, "Queue Playlist: %s", "queue-dance");
  plitem = mainPlaylistItemCache (mainData, playlist, globalCounter++);
  queuePush (mainData->playlistQueue [mi], plitem);
  logMsg (LOG_DBG, LOG_INFO, "push pl %s", "queue-dance");
  mainMusicQueueFill (mainData, mi);
  mainMusicQueuePrep (mainData, mi);
  mainData->musicqChanged [mi] = MAIN_CHG_START;
  mainSendMusicqStatus (mainData);

  playwhenqueued = bdjoptGetNumPerQueue (OPT_Q_PLAY_WHEN_QUEUED, mainData->musicqPlayIdx);
  if (playwhenqueued &&
      mainData->musicqPlayIdx == mi &&
      mainData->playerState == PL_STATE_STOPPED &&
      musicqLen == 0) {
    mainMusicQueuePlay (mainData);
  }
  logProcEnd ("");
}

static void
mainQueuePlaylist (maindata_t *mainData, char *args)
{
  int             mi = MUSICQ_PB_A;
  playlistitem_t  *plitem = NULL;
  playlist_t      *playlist = NULL;
  char            *plname = NULL;
  ilistidx_t      musicqLen;
  int             editmode;
  bool            playwhenqueued;

  logProcBegin ();

  /* get the musicq length before any songs are added */
  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);

  mi = mainParseQueuePlaylist (mainData, args, &plname, &editmode);

  playlist = playlistLoad (plname, mainData->musicdb, mainData->grouping);
  if (playlist == NULL) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: Queue Playlist failed: %s", plname);
    return;
  }

  if (! playlistCheck (playlist)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "Bad Playlist: %s", plname);
    playlistFree (playlist);
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
  queuePush (mainData->playlistQueue [mi], plitem);
  logMsg (LOG_DBG, LOG_INFO, "push pl %s", plname);
  mainMusicQueueFill (mainData, mi);
  mainMusicQueuePrep (mainData, mi);
  mainData->musicqChanged [mi] = MAIN_CHG_START;

  mainSendMusicqStatus (mainData);

  playwhenqueued = bdjoptGetNumPerQueue (OPT_Q_PLAY_WHEN_QUEUED, mainData->musicqPlayIdx);
  if (playwhenqueued &&
      mainData->musicqPlayIdx == mi &&
      mainData->playerState == PL_STATE_STOPPED &&
      musicqLen == 0) {
    mainMusicQueuePlay (mainData);
  }

  logProcEnd ("");
}

static void
mainSigHandler (int sig)
{
  gKillReceived = 1;
}

static void
mainMusicQueueFill (maindata_t *mainData, int mqidx)
{
  int             playerqLen;
  int             currlen;
  playlistitem_t  *plitem = NULL;
  playlist_t      *playlist = NULL;
  pltype_t        pltype = PLTYPE_SONGLIST;
  bool            stopatflag = false;
  int32_t         dur;
  int             editmode = EDIT_FALSE;
  time_t          stopTime = 0;
  time_t          nStopTime = 0;

  logProcBegin ();

  plitem = queueGetFirst (mainData->playlistQueue [mqidx]);
  playlist = NULL;
  if (plitem != NULL) {
    playlist = plitem->playlist;
  }
  if (playlist != NULL) {
    pltype = (pltype_t) playlistGetConfigNum (playlist, PLAYLIST_TYPE);
    editmode = playlistGetEditMode (playlist);
  }

  playerqLen = bdjoptGetNum (OPT_G_PLAYERQLEN);
  currlen = musicqGetLen (mainData->musicQueue, mqidx);

  /* if this is not the queue selected for playback, then push  */
  /* an empty musicq item on to the head of the queue.          */
  /* this allows the display to work properly for the user      */
  /* when multiple queues are in use.                           */

  if (currlen == 0 &&
      mainData->musicqPlayIdx != mqidx) {
    musicqPushHeadEmpty (mainData->musicQueue, mqidx);
    ++currlen;
  }

  logMsg (LOG_DBG, LOG_BASIC, "fill: %d < %d", currlen, playerqLen);

  stopTime = playlistGetConfigNum (playlist, PLAYLIST_STOP_TIME);
  if (stopTime <= 0) {
    /* playlist stop time is not configured, use the queue stop time */
    stopTime = mainData->stopTime [mqidx];
  }
  if (editmode == EDIT_FALSE && stopTime > 0) {
    nStopTime = mainCalcStopTime (stopTime);
    stopatflag = mainCheckMusicQStopTime (mainData, nStopTime, mqidx);
  }

  /* want current + playerqLen songs */
  while (playlist != NULL && currlen <= playerqLen && stopatflag == false) {
    song_t  *song = NULL;

    mainData->musicqLookupIdx = mqidx;
    song = playlistGetNextSong (playlist, currlen,
        mainMusicQueueLookup, mainData);

    if (song != NULL) {
      logMsg (LOG_DBG, LOG_INFO, "push song to musicq");
      dur = mainCalculateSongDuration (mainData, song,
          plitem->playlistIdx, mqidx, MAIN_CALC_WITH_SPEED);
      musicqPush (mainData->musicQueue, mqidx,
          songGetNum (song, TAG_DBIDX), plitem->playlistIdx, dur);
      mainData->musicqChanged [mqidx] = MAIN_CHG_START;
      mainData->marqueeChanged = true;
      currlen = musicqGetLen (mainData->musicQueue, mqidx);

      if (pltype == PLTYPE_AUTO) {
        plitem = queueGetFirst (mainData->playlistQueue [mqidx]);
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
      stopatflag = mainCheckMusicQStopTime (mainData, nStopTime, mqidx);
    }

    if (song == NULL || stopatflag) {
      logMsg (LOG_DBG, LOG_INFO, "song is null or stop-at");
      playlist = mainNextPlaylist (mainData, mqidx);
      stopatflag = false;
      stopTime = playlistGetConfigNum (playlist, PLAYLIST_STOP_TIME);
      if (stopTime > 0) {
        nStopTime = mainCalcStopTime (stopTime);
      }
      if (editmode == EDIT_FALSE && stopTime > 0) {
        stopatflag = mainCheckMusicQStopTime (mainData, nStopTime, mqidx);
      }
      continue;
    }
  }

  logProcEnd ("");
}

static void
mainMusicQueuePrep (maindata_t *mainData, int mqidx)
{
  bool          announceflag = false;

  logProcBegin ();

  if (mqidx >= MUSICQ_PB_MAX && mqidx != MUSICQ_MNG_PB) {
    logProcEnd ("not-a-pb-q");
    return;
  }

  if (musicqGetLen (mainData->musicQueue, mqidx) <= 0) {
    logProcEnd ("q-empty");
    return;
  }

  /* 5 is the number of songs to prep ahead of time */
  for (int i = 0; i < MAIN_PREP_SIZE; ++i) {
    const char    *sfname = NULL;
    dbidx_t       dbidx;
    song_t        *song = NULL;
    musicqflag_t  flags;
    const char    *annfname = NULL;
    int           playlistIdx;

    dbidx = musicqGetByIdx (mainData->musicQueue, mqidx, i);
    if (dbidx < 0) {
      continue;
    }

    song = dbGetByIdx (mainData->musicdb, dbidx);
    if (song == NULL) {
      continue;
    }
    flags = musicqGetFlags (mainData->musicQueue, mqidx, i);
    playlistIdx = musicqGetPlaylistIdx (mainData->musicQueue, mqidx, i);
    announceflag = mainGetAnnounceFlag (mainData, mqidx, playlistIdx);

    if (song != NULL &&
        (flags & MUSICQ_FLAG_PREP) != MUSICQ_FLAG_PREP) {
      int32_t uniqueidx;

      musicqSetFlag (mainData->musicQueue, mqidx, i, MUSICQ_FLAG_PREP);
      uniqueidx = musicqGetUniqueIdx (mainData->musicQueue, mqidx, i);
      sfname = songGetStr (song, TAG_URI);
      annfname = mainPrepSong (mainData, PREP_SONG, mqidx, song, sfname,
          playlistIdx, uniqueidx);

      if (announceflag == true) {
        if (annfname != NULL && *annfname) {
          musicqSetFlag (mainData->musicQueue, mqidx, i, MUSICQ_FLAG_ANNOUNCE);
          musicqSetAnnounce (mainData->musicQueue, mqidx, i, annfname);
        }
      }
    } /* announcement */
  }

  /* if the queue being prepped is the playback queue, re-send the gap, */
  /* as the songs may have changed */
  /* do not worry about the deferred musicq, the gap is from the current q */
  if (mqidx == mainData->musicqPlayIdx) {
    mainSendPlaybackGap (mainData);
  }

  logProcEnd ("");
}

static void
mainMusicqClearPreppedSongs (maindata_t *mainData, int mqidx, int idx)
{
  for (int i = idx; i < MAIN_PREP_SIZE * 3; ++i) {
    mainMusicqClearPrep (mainData, mqidx, i);
  }
}

static void
mainMusicqClearPrep (maindata_t *mainData, int mqidx, int idx)
{
  dbidx_t       dbidx;
  song_t        *song;
  musicqflag_t  flags;
  int32_t       uniqueidx;


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

      snprintf (tmp, sizeof (tmp), "%" PRId32 "%c%s",
          uniqueidx, MSG_ARGS_RS, songGetStr (song, TAG_URI));
      connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_SONG_CLEAR_PREP, tmp);
    }
  }
}

static const char *
mainPrepSong (maindata_t *mainData, int prepflag, int mqidx,
    song_t *song, const char *sfname, int playlistIdx, int32_t uniqueidx)
{
  char          tbuff [1024];
  ssize_t       dur = 0;
  ssize_t       songstart = 0;
  int           speed = 100;
  double        voladjperc = 0;
  bool          announceflag = false;
  ilistidx_t    danceidx;
  const char    *annfname = NULL;

  logProcBegin ();

  voladjperc = songGetDouble (song, TAG_VOLUMEADJUSTPERC);
  if (isnan (voladjperc)) { voladjperc = 0.0; }
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

    announceflag = mainGetAnnounceFlag (mainData, mqidx, playlistIdx);

    if (announceflag == true) {
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

  snprintf (tbuff, sizeof (tbuff), "%s%c%" PRId64 "%c%" PRId64 "%c%d%c%.1f%c%d%c%" PRId32,
      sfname, MSG_ARGS_RS,
      (int64_t) dur, MSG_ARGS_RS,
      (int64_t) songstart, MSG_ARGS_RS,
      speed, MSG_ARGS_RS,
      voladjperc, MSG_ARGS_RS,
      prepflag, MSG_ARGS_RS,
      uniqueidx);

  logMsg (LOG_DBG, LOG_INFO, "prep song %s", sfname);
  connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_SONG_PREP, tbuff);
  logProcEnd ("");
  return annfname;
}

static void
mainPlaylistClearQueue (maindata_t *mainData, char *args)
{
  int   mi;

  logProcBegin ();

  /* used by the song list editor */
  /* after a song list is created via a sequenced or automatic playlist */
  /* then the playlist queue needs to be reset */

  mi = mainMusicqIndexParse (mainData, args);
  queueClear (mainData->playlistQueue [mi], 0);
  logProcEnd ("");
}

static void
mainTogglePause (maindata_t *mainData, char *args)
{
  int           mi;
  ilistidx_t    idx;
  ilistidx_t    musicqLen;
  musicqflag_t  flags;

  logProcBegin ();

  mi = mainMusicqIndexNumParse (mainData, args, &idx, NULL);
  ++idx;    /* music-q index 0 is reserved for the current song */

  musicqLen = musicqGetLen (mainData->musicQueue, mi);
  if (idx <= 0 || idx > musicqLen) {
    logProcEnd ("bad-idx");
    return;
  }

  flags = musicqGetFlags (mainData->musicQueue, mi, idx);
  if ((flags & MUSICQ_FLAG_PAUSE) == MUSICQ_FLAG_PAUSE) {
    musicqClearFlag (mainData->musicQueue, mi, idx, MUSICQ_FLAG_PAUSE);
  } else {
    musicqSetFlag (mainData->musicQueue, mi, idx, MUSICQ_FLAG_PAUSE);
  }
  mainData->musicqChanged [mi] = MAIN_CHG_START;
  logProcEnd ("");
}

static void
mainMusicqMove (maindata_t *mainData, char *args, mainmove_t direction)
{
  int           mi;
  ilistidx_t    fromidx;
  ilistidx_t    toidx;
  ilistidx_t    musicqLen;

  logProcBegin ();


  mi = mainMusicqIndexNumParse (mainData, args, &fromidx, NULL);
  ++fromidx;   /* music-q index 0 is reserved for the current song */

  musicqLen = musicqGetLen (mainData->musicQueue, mi);

  toidx = fromidx + direction;
  if (fromidx == 1 && toidx == 0 &&
      mainData->playerState != PL_STATE_STOPPED) {
    logProcEnd ("to-0-playing");
    return;
  }

  if (fromidx == 1 && toidx == 0) {
    dbidx_t   dbidx;

    dbidx = musicqGetByIdx (mainData->musicQueue, mi, 0);
    if (dbidx < 0) {
      logProcEnd ("to-0-diff-q");
      return;
    }
  }

  if (toidx > fromidx && fromidx >= musicqLen) {
    logProcEnd ("bad-move");
    return;
  }
  if (toidx < fromidx && fromidx < 1) {
    logProcEnd ("bad-move");
    return;
  }

  musicqMove (mainData->musicQueue, mi, fromidx, toidx);
  mainData->musicqChanged [mi] = MAIN_CHG_START;
  mainData->marqueeChanged = true;
  mainMusicQueuePrep (mainData, mi);

  if (toidx == 0) {
    mainSendMusicqStatus (mainData);
  }
  logProcEnd ("");
}

static void
mainMusicqMoveTop (maindata_t *mainData, char *args)
{
  int         mi;
  ilistidx_t  fromidx;
  ilistidx_t  toidx;
  ilistidx_t  musicqLen;

  logProcBegin ();

  mi = mainMusicqIndexNumParse (mainData, args, &fromidx, NULL);

  musicqLen = musicqGetLen (mainData->musicQueue, mi);

  ++fromidx;    /* music-q index 0 is reserved for the current song */
  if (fromidx < 0 || fromidx >= musicqLen) {
    logProcEnd ("bad-move");
    return;
  }

  toidx = fromidx - 1;
  while (toidx != 0) {
    musicqMove (mainData->musicQueue, mi, fromidx, toidx);
    fromidx--;
    toidx--;
  }
  mainData->musicqChanged [mi] = MAIN_CHG_START;
  mainData->marqueeChanged = true;
  mainMusicQueuePrep (mainData, mi);
  logProcEnd ("");
}

static void
mainMusicqClear (maindata_t *mainData, char *args)
{
  int         mi;
  ilistidx_t  idx;

  logProcBegin ();

  mi = mainMusicqIndexNumParse (mainData, args, &idx, NULL);
  ++idx;    /* music-q index 0 is reserved for the current song */

  mainMusicqClearPreppedSongs (mainData, mi, idx);
  musicqClear (mainData->musicQueue, mi, idx);
  /* there may be other playlists in the playlist queue */
  mainMusicQueueFill (mainData, mi);
  mainMusicQueuePrep (mainData, mi);
  mainData->musicqChanged [mi] = MAIN_CHG_START;
  mainData->marqueeChanged = true;
  logProcEnd ("");
}

static void
mainMusicqRemove (maindata_t *mainData, char *args)
{
  int           mi;
  ilistidx_t    idx;


  logProcBegin ();

  mi = mainMusicqIndexNumParse (mainData, args, &idx, NULL);
  ++idx;    /* music-q index 0 is reserved for the current song */

  mainMusicqClearPrep (mainData, mi, idx);
  musicqRemove (mainData->musicQueue, mi, idx);
  mainMusicQueueFill (mainData, mi);
  mainMusicQueuePrep (mainData, mi);
  mainData->musicqChanged [mi] = MAIN_CHG_START;
  mainData->marqueeChanged = true;
  logProcEnd ("");
}

static void
mainMusicqSwap (maindata_t *mainData, char *args)
{
  int           mi;
  ilistidx_t    fromidx;
  ilistidx_t    toidx;


  logProcBegin ();

  mi = mainMusicqIndexNumParse (mainData, args, &fromidx, &toidx);
  ++fromidx;    /* music-q index 0 is reserved for the current song */
  ++toidx;

  if (fromidx >= 1 && toidx >= 1) {
    musicqSwap (mainData->musicQueue, mi, fromidx, toidx);
    mainMusicQueuePrep (mainData, mi);
    mainData->musicqChanged [mi] = MAIN_CHG_START;
    mainData->marqueeChanged = true;
  }
  logProcEnd ("");
}

static void
mainNextSongPlay (maindata_t *mainData)
{
  int   currlen;

  currlen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);
  if (currlen > 1) {
    connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_PLAY_NEXTSONG, NULL);
  }
  /* the internal manage-playback queue is set to play-when-queued */
  if (mainData->musicqPlayIdx != MUSICQ_MNG_PB &&
      mainData->playerState == PL_STATE_STOPPED) {
    mainMusicQueuePlay (mainData);
  }
  if (mainData->playerState == PL_STATE_PAUSED) {
    mainData->waitforpbfinish = true;
  }
}

static void
mainMusicqInsert (maindata_t *mainData, bdjmsgroute_t routefrom, char *args)
{
  char      *tokstr = NULL;
  char      *p = NULL;
  int       idx;
  dbidx_t   dbidx;
  song_t    *song = NULL;
  int       currlen;
  int       mi;


  logProcBegin ();

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  if (p == NULL) {
    logProcEnd ("parse-fail-a");
    return;
  }
  mi = mainMusicqIndexParse (mainData, p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (p == NULL) {
    logProcEnd ("parse-fail-b");
    return;
  }
  idx = atol (p);
  ++idx;  /* music-q index 0 is reserved for the current song */
  ++idx;  /* want to insert after the current song */

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (p == NULL) {
    logProcEnd ("parse-fail-c");
    return;
  }
  dbidx = atol (p);

  currlen = musicqGetLen (mainData->musicQueue, mi);
  song = dbGetByIdx (mainData->musicdb, dbidx);

  if (currlen == 0 &&
      mainData->musicqPlayIdx != mi) {
    musicqPushHeadEmpty (mainData->musicQueue, mi);
  }

  if (song != NULL) {
    int32_t loc;
    int64_t dur;
    int     musicqLen;
    bool    playwhenqueued;

    dur = mainCalculateSongDuration (mainData, song,
        MUSICQ_PLAYLIST_EMPTY, mi, MAIN_CALC_WITH_SPEED);
    loc = musicqInsert (mainData->musicQueue, mi, idx, dbidx, dur);
    mainData->musicqChanged [mi] = MAIN_CHG_START;
    mainData->marqueeChanged = true;
    mainMusicQueuePrep (mainData, mi);
    mainSendMusicqStatus (mainData);
    musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);

    playwhenqueued = bdjoptGetNumPerQueue (OPT_Q_PLAY_WHEN_QUEUED, mainData->musicqPlayIdx);
    if (mainData->playerState == PL_STATE_STOPPED &&
        playwhenqueued &&
        mainData->musicqPlayIdx == mi &&
        musicqLen == 1) {
      mainMusicQueuePlay (mainData);
    }
    if (loc > 0) {
      char  tbuff [40];

      /* bdj4main is offset by 1 */
      snprintf (tbuff, sizeof (tbuff), "%d%c%" PRId32, mi, MSG_ARGS_RS, loc - 1);
      connSendMessage (mainData->conn, routefrom, MSG_SONG_SELECT, tbuff);
    }
  }

  logProcEnd ("");
}

static void
mainMusicqSetPlayback (maindata_t *mainData, char *args)
{
  int       mqidx;

  logProcBegin ();

  mqidx = atoi (args);
  if (mqidx < 0 || mqidx >= MUSICQ_MAX) {
    logProcEnd ("bad-idx");
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
  mainData->marqueeChanged = true;
  logProcEnd ("");
}

static void
mainMusicqSendQueueConfig (maindata_t *mainData)
{
  char          tmp [40];

  snprintf (tmp, sizeof (tmp), "%" PRId64,
      (int64_t) bdjoptGetNumPerQueue (OPT_Q_FADEINTIME, mainData->musicqPlayIdx));
  connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_SET_PLAYBACK_FADEIN, tmp);
  snprintf (tmp, sizeof (tmp), "%" PRId64,
      (int64_t) bdjoptGetNumPerQueue (OPT_Q_FADEOUTTIME, mainData->musicqPlayIdx));
  connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_SET_PLAYBACK_FADEOUT, tmp);
}

static void
mainMusicqSwitch (maindata_t *mainData, int newMusicqIdx)
{
  dbidx_t       dbidx;
  song_t        *song;
  int           oldplayidx;

  logProcBegin ();

  oldplayidx = mainData->musicqPlayIdx;
  musicqPushHeadEmpty (mainData->musicQueue, oldplayidx);
  /* the prior musicq has changed */
  mainData->musicqChanged [oldplayidx] = MAIN_CHG_START;
  /* send the prior musicq update while the play idx is still set */
  mainSendMusicqStatus (mainData);

  /* now change the play idx */
  mainData->musicqPlayIdx = newMusicqIdx;
  mainData->musicqDeferredPlayIdx = MAIN_NOT_SET;

  /* send the player the fade-in and fade-out times for this queue */
  mainMusicqSendQueueConfig (mainData);

  /* having switched the playback music queue, the songs must be prepped */
  /* the prep will send the playback gap */
  mainMusicQueuePrep (mainData, mainData->musicqPlayIdx);

  dbidx = musicqGetCurrent (mainData->musicQueue, mainData->musicqPlayIdx);
  song = dbGetByIdx (mainData->musicdb, dbidx);
  if (song == NULL) {
    musicqPop (mainData->musicQueue, mainData->musicqPlayIdx);
  }

  /* and now send the musicq update for the new play idx */
  mainSendMusicqStatus (mainData);
  mainData->musicqChanged [mainData->musicqPlayIdx] = MAIN_CHG_START;
  mainData->marqueeChanged = true;
  logProcEnd ("");
}

static void
mainPlaybackBegin (maindata_t *mainData)
{
  musicqflag_t  flags;
  bool          pauseind;

  logProcBegin ();

  /* if the pause flag is on for this queue, */
  /* tell the player to turn on the pause-at-end */
  pauseind = bdjoptGetNumPerQueue (OPT_Q_PAUSE_EACH_SONG, mainData->musicqPlayIdx);
  if (pauseind) {
    connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_PLAY_PAUSEATEND, NULL);
    logProcEnd ("pause-each-song");
    return;
  }

  /* if the pause flag is on for this entry in the music queue, */
  /* tell the player to turn on the pause-at-end */
  flags = musicqGetFlags (mainData->musicQueue, mainData->musicqPlayIdx, 0);
  if ((flags & MUSICQ_FLAG_PAUSE) == MUSICQ_FLAG_PAUSE) {
    connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_PLAY_PAUSEATEND, NULL);
  }
  logProcEnd ("");
}

static void
mainMusicQueuePlay (maindata_t *mainData)
{
  musicqflag_t  flags;
  const char    *sfname;
  dbidx_t       dbidx;
  int32_t       uniqueidx;
  song_t        *song;
  int           currlen;
  time_t        currTime;

  logProcBegin ();

  currTime = mstime ();
  if (mainData->stopTime [mainData->musicqPlayIdx] > 0 &&
      currTime > mainData->nStopTime [mainData->musicqPlayIdx]) {
    logMsg (LOG_DBG, LOG_INFO, "past stop-at time: finished <= true");
    mainData->finished = true;
    mainData->marqueeChanged = true;
    /* reset the stop time so that the player can be re-started */
    mainData->stopTime [mainData->musicqPlayIdx] = 0;
    mainData->nStopTime [mainData->musicqPlayIdx] = 0;
    return;
  }

  logMsg (LOG_DBG, LOG_BASIC, "pl-state: %d/%s",
      mainData->playerState, logPlayerState (mainData->playerState));

  mainData->startwaitTime = bdjoptGetNumPerQueue (OPT_Q_START_WAIT_TIME, mainData->musicqPlayIdx);

  /* main receives the playback-finish message first, and acts on it, */
  /* before the player-state message (stopped) is received */
  /* otherwise some other method of determining when the player is stopped */
  /* would be needed */
  if (! mainData->inStartWait &&
      mainData->playerState == PL_STATE_STOPPED &&
      mainData->startwaitTime > 0) {
    /* if the start-wait-time time is set, */
    /* and the player is stopped, */
    /* don't start playing immediately */
    mstimeset (&mainData->startWaitCheck, mainData->startwaitTime);
    mainData->inStartWait = true;
    return;
  }

  mainData->inStartWait = false;

  if (mainData->playerState != PL_STATE_PAUSED) {
    if (mainData->musicqDeferredPlayIdx != MAIN_NOT_SET) {
      mainMusicqSwitch (mainData, mainData->musicqDeferredPlayIdx);
      mainData->musicqDeferredPlayIdx = MAIN_NOT_SET;
    }

    /* grab a song out of the music queue and start playing */
    logMsg (LOG_DBG, LOG_INFO, "player sent a finish; get song, start");
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
          mainData->inannounce = true;
          snprintf (tmp, sizeof (tmp), "%d%c%s", PL_UNIQUE_ANN, MSG_ARGS_RS, annfname);
          connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_SONG_PLAY, tmp);
        }
      }
      sfname = songGetStr (song, TAG_URI);
      snprintf (tmp, sizeof (tmp), "%" PRId32 "%c%s", uniqueidx, MSG_ARGS_RS, sfname);
      connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_SONG_PLAY, tmp);

      /* set the gap for the upcoming song */
      mainSendPlaybackGap (mainData);

      ++mainData->songplaysentcount;
    }

    currlen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);

    if (song == NULL && currlen == 0) {
      logMsg (LOG_DBG, LOG_INFO, "no songs left in queue");

      if (mainData->switchQueueWhenEmpty) {
        logMsg (LOG_DBG, LOG_INFO, "switch queues");

        mainData->musicqPlayIdx = mainLocateQueueWithSongs (mainData);
        currlen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);

        mainMusicqSendQueueConfig (mainData);

        if (currlen > 0) {
          /* the songs must be prepped */
          mainMusicQueuePrep (mainData, mainData->musicqPlayIdx);
          mainSendPlayerUISwitchQueue (mainData, mainData->musicqPlayIdx);

          /* and start up playback for the new queue */
          /* the next-song flag is always 0 here */
          /* if the state is stopped or paused, music-q-next will start up */
          /* playback */
          mainMusicQueueNext (mainData, "0");
        } else {
          /* there is no music to play; tell the player to clear its display */
          logMsg (LOG_DBG, LOG_INFO, "sqwe:true no music to play: finished <= true");
          mainData->finished = true;
          mainData->marqueeChanged = true;
        }
      } else {
        logMsg (LOG_DBG, LOG_INFO, "no more songs; pl-state: %d/%s; finished <= true",
            mainData->playerState, logPlayerState (mainData->playerState));
        mainData->finished = true;
        mainData->marqueeChanged = true;
      }
    } /* if the song was null and the queue is empty */
  } /* song is not paused */

  /* this handles the user-selected play button when the song is paused */
  if (mainData->playerState == PL_STATE_PAUSED ||
      mainData->playerState == PL_STATE_IN_GAP) {
    logMsg (LOG_DBG, LOG_INFO, "player is paused/gap, send play msg");
    connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_PLAY_PLAY, NULL);
    mainSendPlaybackGap (mainData);
  }

  logProcEnd ("");
}

static void
mainMusicQueueFinish (maindata_t *mainData, const char *args)
{
  playlist_t    *playlist;
  dbidx_t       dbidx;
  song_t        *song;
  int           playlistIdx;

  logProcBegin ();

  mainPlaybackSendSongFinish (mainData, args);

  /* let the playlist know this song has been played */
  dbidx = musicqGetCurrent (mainData->musicQueue, mainData->musicqPlayIdx);
  song = dbGetByIdx (mainData->musicdb, dbidx);
  if (mainData->playerState == PL_STATE_STOPPED) {
    /* this is only needed when the player is stopped */
    mainMusicqClearPrep (mainData, mainData->musicqPlayIdx, 0);
  }
  if (song != NULL) {
    playlistIdx = musicqGetPlaylistIdx (mainData->musicQueue, mainData->musicqPlayIdx, 0);
    if (playlistIdx != MUSICQ_PLAYLIST_EMPTY) {
      playlist = nlistGetData (mainData->playlistCache, playlistIdx);
      if (playlist != NULL && song != NULL) {
        playlistAddPlayed (playlist, song);
      }
    }
  }

  musicqPop (mainData->musicQueue, mainData->musicqPlayIdx);

  mainData->musicqChanged [mainData->musicqPlayIdx] = MAIN_CHG_START;
  mainData->marqueeChanged = true;

  /* if the queue is being stopped, need to check if the playback */
  /* queue needs to be switched */
  if (mainData->musicqDeferredPlayIdx != MAIN_NOT_SET) {
    mainMusicqSwitch (mainData, mainData->musicqDeferredPlayIdx);
    mainData->musicqDeferredPlayIdx = MAIN_NOT_SET;
  }

  mainSendMusicqStatus (mainData);
  logProcEnd ("");
}

static void
mainMusicQueueNext (maindata_t *mainData, const char *args)
{
  int     currlen;

  logProcBegin ();

  mainMusicQueueFinish (mainData, args);
  if (mainData->playerState != PL_STATE_STOPPED &&
      mainData->playerState != PL_STATE_PAUSED) {
    mainMusicQueuePlay (mainData);
  }
  mainMusicQueueFill (mainData, mainData->musicqPlayIdx);
  mainMusicQueuePrep (mainData, mainData->musicqPlayIdx);

  currlen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);

  if (mainData->playerState == PL_STATE_STOPPED ||
      mainData->playerState == PL_STATE_PAUSED) {
    if (currlen == 0) {
      mainData->finished = true;
    }
  }

  /* some extra complications to get the gap from the upcoming */
  /* queue and not the current queue */
  if (mainData->switchQueueWhenEmpty && currlen == 1) {
    int   tplayidx;
    int   tcurrlen;
    int   origMusicqPlayIdx;

    tplayidx = mainLocateQueueWithSongs (mainData);
    tcurrlen = musicqGetLen (mainData->musicQueue, tplayidx);
    if (tcurrlen > 0) {
      origMusicqPlayIdx = mainData->musicqPlayIdx;
      mainData->musicqPlayIdx = tplayidx;
      mainSendPlaybackGap (mainData);
      mainData->musicqPlayIdx = origMusicqPlayIdx;
    }
  }

  logProcEnd ("");
}

static ilistidx_t
mainMusicQueueLookup (void *tmaindata, ilistidx_t idx)
{
  maindata_t    *mainData = tmaindata;
  ilistidx_t    didx = -1;
  dbidx_t       dbidx;
  song_t        *song;

  logProcBegin ();

  if (idx < 0 ||
      idx >= musicqGetLen (mainData->musicQueue, mainData->musicqLookupIdx)) {
    logProcEnd ("bad-idx");
    return -1;
  }

  dbidx = musicqGetByIdx (mainData->musicQueue, mainData->musicqLookupIdx, idx);
  song = dbGetByIdx (mainData->musicdb, dbidx);
  if (song != NULL) {
    didx = songGetNum (song, TAG_DANCE);
  }
  logProcEnd ("");
  return didx;
}

static void
mainSendDanceList (maindata_t *mainData, bdjmsgroute_t route)
{
  dance_t       *dances;
  slist_t       *danceList;
  slistidx_t    idx;
  const char    *dancenm;
  char          tbuff [200];
  char          *rbuff = NULL;
  slistidx_t    iteridx;
  char          *rp;
  char          *rend;

  logProcBegin ();

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  danceList = danceGetDanceList (dances);

  rbuff = mdmalloc (BDJMSG_MAX);
  rbuff [0] = '\0';
  rp = rbuff;
  rend = rbuff + BDJMSG_MAX;
  slistStartIterator (danceList, &iteridx);
  while ((dancenm = slistIterateKey (danceList, &iteridx)) != NULL) {
    idx = slistGetNum (danceList, dancenm);
    snprintf (tbuff, sizeof (tbuff), "%" PRId32 "%c%s%c",
        idx, MSG_ARGS_RS, dancenm, MSG_ARGS_RS);
    rp = stpecpy (rp, rend, tbuff);
  }

  connSendMessage (mainData->conn, route, MSG_DANCE_LIST_DATA, rbuff);
  dataFree (rbuff);
  logProcEnd ("");
}

static void
mainSendPlaylistNames (maindata_t *mainData, bdjmsgroute_t route)
{
  slist_t       *plNames = NULL;
  const char    *plfnm = NULL;
  const char    *plnm = NULL;
  char          tbuff [200];
  char          *rbuff = NULL;
  slistidx_t    iteridx;
  char          *rp;
  char          *rend;

  logProcBegin ();

  plNames = playlistGetPlaylistNames (PL_LIST_NORMAL, NULL);

  rbuff = mdmalloc (BDJMSG_MAX);
  rbuff [0] = '\0';
  rp = rbuff;
  rend = rbuff + BDJMSG_MAX;
  slistStartIterator (plNames, &iteridx);
  while ((plnm = slistIterateKey (plNames, &iteridx)) != NULL) {
    plfnm = slistGetStr (plNames, plnm);
    snprintf (tbuff, sizeof (tbuff), "%s%c%s%c",
        plfnm, MSG_ARGS_RS, plnm, MSG_ARGS_RS);
    rp = stpecpy (rp, rend, tbuff);
  }

  slistFree (plNames);

  connSendMessage (mainData->conn, route, MSG_PLAYLIST_LIST_DATA, rbuff);
  dataFree (rbuff);
  logProcEnd ("");
}

static void
mainSendPlayerStatus (maindata_t *mainData, char *playerResp)
{
  char        tbuff [200];
  char        tbuff2 [40];
  char        *jsbuff = NULL;
  int         jsonflag;
  const char  *p;
  mp_playerstatus_t *ps;
  char        *tp;
  char        *tend;
  char        *jp;
  char        *jend;

  logProcBegin ();

  jsonflag = bdjoptGetNum (OPT_P_REMOTECONTROL);

  ps = msgparsePlayerStatusData (playerResp);

  if (mainData->marqueestarted) {
    char  *timerbuff = NULL;

    /* for marquee */
    timerbuff = mdmalloc (BDJMSG_MAX);
    tp = timerbuff;
    tend = timerbuff + BDJMSG_MAX;

    snprintf (tbuff, sizeof (tbuff), "%" PRIu32 "%c", ps->playedtime, MSG_ARGS_RS);
    tp = stpecpy (tp, tend, tbuff);

    snprintf (tbuff, sizeof (tbuff), "%" PRId32, ps->duration);
    tp = stpecpy (tp, tend, tbuff);

    connSendMessage (mainData->conn, ROUTE_MARQUEE, MSG_MARQUEE_TIMER, timerbuff);
    dataFree (timerbuff);
  }

  if (! jsonflag) {
    msgparsePlayerStatusFree (ps);
    return;
  }

  jsbuff = mdmalloc (BDJMSG_MAX);
  *jsbuff = '\0';
  jp = jsbuff;
  jend = jsbuff + BDJMSG_MAX;

  jp = stpecpy (jp, jend, "{ ");

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
  jp = stpecpy (jp, jend, tbuff);

  snprintf (tbuff, sizeof (tbuff),
      "\"repeat\" : \"%d\"", ps->repeat);
  jp = stpecpy (jp, jend, ", ");
  jp = stpecpy (jp, jend, tbuff);

  snprintf (tbuff, sizeof (tbuff),
      "\"pauseatend\" : \"%d\"", ps->pauseatend);
  jp = stpecpy (jp, jend, ", ");
  jp = stpecpy (jp, jend, tbuff);

  snprintf (tbuff, sizeof (tbuff),
      "\"vol\" : \"%d%%\"", ps->currentVolume);
  jp = stpecpy (jp, jend, ", ");
  jp = stpecpy (jp, jend, tbuff);

  snprintf (tbuff, sizeof (tbuff),
      "\"speed\" : \"%d%%\"", ps->currentSpeed);
  jp = stpecpy (jp, jend, ", ");
  jp = stpecpy (jp, jend, tbuff);

  snprintf (tbuff, sizeof (tbuff),
      "\"playedtime\" : \"%s\"",
      tmutilToMS (ps->playedtime, tbuff2, sizeof (tbuff2)));
  jp = stpecpy (jp, jend, ", ");
  jp = stpecpy (jp, jend, tbuff);

  snprintf (tbuff, sizeof (tbuff),
      "\"duration\" : \"%s\"",
      tmutilToMS (ps->duration, tbuff2, sizeof (tbuff2)));
  jp = stpecpy (jp, jend, ", ");
  jp = stpecpy (jp, jend, tbuff);

  /* the javascript is built as two pieces */
  /* the complete js message will be put together by bdj4rc.c */

  connSendMessage (mainData->conn, ROUTE_REMCTRL, MSG_PLAYER_STATUS_DATA, jsbuff);

  msgparsePlayerStatusFree (ps);
  dataFree (jsbuff);

  logProcEnd ("");
}

static void
mainSendRemctrlData (maindata_t *mainData)
{
  char        *jsbuff = NULL;
  int32_t     musicqLen;
  char        tbuff [200];
  const char  *data;
  dbidx_t     dbidx;
  song_t      *song;
  char        *jp;
  char        *jend;

  if (! bdjoptGetNum (OPT_P_REMOTECONTROL)) {
    return;
  }

  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);

  jsbuff = mdmalloc (BDJMSG_MAX);
  *jsbuff = '\0';
  jp = jsbuff;
  jend = jsbuff + BDJMSG_MAX;

  /* the javascript is built as two pieces */
  /* the complete js message will be put together by bdj4rc.c */

  snprintf (tbuff, sizeof (tbuff),
      "\"qlength\" : \"%" PRId32 "\"", musicqLen);
  jp = stpecpy (jp, jend, ", ");
  jp = stpecpy (jp, jend, tbuff);

  /* dance */
  data = musicqGetDance (mainData->musicQueue, mainData->musicqPlayIdx, 0);
  if (data == NULL) { data = ""; }
  snprintf (tbuff, sizeof (tbuff),
      "\"dance\" : \"%s\"", data);
  jp = stpecpy (jp, jend, ", ");
  jp = stpecpy (jp, jend, tbuff);

  dbidx = musicqGetByIdx (mainData->musicQueue, mainData->musicqPlayIdx, 0);
  song = dbGetByIdx (mainData->musicdb, dbidx);

  /* artist */
  data = songGetStr (song, TAG_ARTIST);
  if (data == NULL) { data = ""; }
  snprintf (tbuff, sizeof (tbuff),
      "\"artist\" : \"%s\"", data);
  jp = stpecpy (jp, jend, ", ");
  jp = stpecpy (jp, jend, tbuff);

  /* title */
  data = songGetStr (song, TAG_TITLE);
  if (data == NULL) { data = ""; }
  snprintf (tbuff, sizeof (tbuff),
      "\"title\" : \"%s\"", data);
  jp = stpecpy (jp, jend, ", ");
  jp = stpecpy (jp, jend, tbuff);

  jp = stpecpy (jp, jend, " }");
  connSendMessage (mainData->conn, ROUTE_REMCTRL, MSG_CURR_SONG_DATA, jsbuff);

  dataFree (jsbuff);

  logProcEnd ("");
}

static void
mainSendMusicqStatus (maindata_t *mainData)
{
  char    tbuff [40];
  dbidx_t dbidx;
  int32_t uniqueidx;

  logProcBegin ();

  dbidx = musicqGetByIdx (mainData->musicQueue, mainData->musicqPlayIdx, 0);
  uniqueidx = musicqGetUniqueIdx (mainData->musicQueue, mainData->musicqPlayIdx, 0);
  msgbuildMusicQStatus (tbuff, sizeof (tbuff), dbidx, uniqueidx);

  connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_MUSICQ_STATUS_DATA, tbuff);
  connSendMessage (mainData->conn, ROUTE_MANAGEUI, MSG_MUSICQ_STATUS_DATA, tbuff);
  logProcEnd ("");
}

/* calls mainMusicqIndexParse, */
static int
mainMusicqIndexNumParse (maindata_t *mainData, char *args, ilistidx_t *b, ilistidx_t *c)
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
static int
mainParseQueuePlaylist (maindata_t *mainData, char *args, char **b, int *editmode)
{
  int   mi = MUSICQ_PB_A;
  char  *p = NULL;
  char  *tokstr = NULL;

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  if (p != NULL) {
    mi = mainMusicqIndexParse (mainData, p);
  }

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (p != NULL) {
    *b = p;
  }

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

static int
mainMusicqIndexParse (maindata_t *mainData, const char *p)
{
  int   mi = -1;

  if (p != NULL) {
    mi = atoi (p);
  }

  return mi;
}

static void
mainSendFinished (maindata_t *mainData)
{
  connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_FINISHED, NULL);
  connSendMessage (mainData->conn, ROUTE_MANAGEUI, MSG_FINISHED, NULL);
  if (mainData->marqueestarted) {
    connSendMessage (mainData->conn, ROUTE_MARQUEE, MSG_FINISHED, NULL);
  }
  if (bdjoptGetNum (OPT_P_MOBMQ_TYPE) != MOBMQ_TYPE_OFF) {
    connSendMessage (mainData->conn, ROUTE_MOBILEMQ, MSG_FINISHED, NULL);
  }
}

static int32_t
mainCalculateSongDuration (maindata_t *mainData, song_t *song,
    int playlistIdx, int mqidx, int speedCalcFlag)
{
  int32_t     maxdur;
  int32_t     dur;
  playlist_t  *playlist = NULL;
  int32_t     pldncmaxdur;
  int32_t     plmaxdur;
  int32_t     songstart;
  int32_t     songend;
  int         speed;
  ilistidx_t  danceidx;


  if (song == NULL) {
    return 0;
  }

  logProcBegin ();

  songstart = 0;
  speed = 100;
  dur = songGetNum (song, TAG_DURATION);
  logMsg (LOG_DBG, LOG_INFO, "song base-dur: %" PRId32, dur);

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
  logMsg (LOG_DBG, LOG_INFO, "dur: %" PRId32 " songstart: %" PRId32 " songend: %" PRId32,
      dur, songstart, songend);
  if (songend >= 10000 && dur > songend) {
    dur = songend;
    logMsg (LOG_DBG, LOG_INFO, "dur-songend: %" PRId32, dur);
  }
  /* adjust the song's duration by the songstart value */
  if (songstart > 0) {
    dur -= songstart;
    logMsg (LOG_DBG, LOG_INFO, "dur-songstart: %" PRId32, dur);
  }

  /* after adjusting the duration by song start/end, then adjust */
  /* the duration by the speed of the song */
  /* this is the real duration for the song */
  if (speedCalcFlag == MAIN_CALC_WITH_SPEED && speed != 100) {
    dur = songutilAdjustPosReal (dur, speed);
    logMsg (LOG_DBG, LOG_INFO, "dur-speed: %" PRId32, dur);
  }

  maxdur = bdjoptGetNumPerQueue (OPT_Q_MAXPLAYTIME, mqidx);
  logMsg (LOG_DBG, LOG_INFO, "dur: %" PRId32, dur);
  logMsg (LOG_DBG, LOG_INFO, "dur-q-maxdur: %" PRId32, maxdur);

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
    logMsg (LOG_DBG, LOG_INFO, "dur-pl-dnc-dur: %" PRId32, dur);
  } else {
    /* the playlist max-play-time overrides the global max-play-time */
    if (plmaxdur >= 5000 && dur > plmaxdur) {
      dur = plmaxdur;
      logMsg (LOG_DBG, LOG_INFO, "dur-pl-dur: %" PRId32, dur);
    } else if (maxdur >= 5000 && dur > maxdur) {
      dur = maxdur;
      logMsg (LOG_DBG, LOG_INFO, "dur-q-dur: %" PRId32, dur);
    }
  }

  logProcEnd ("");
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

  /* the playlist data is owned by the playlistCache and is freed by it */
  dataFree (plitem);
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
  qidx_t        musicqLen;
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
  int           failcount = 0;

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  mqidx = mainMusicqIndexParse (mainData, args);

  danceCounts = nlistAlloc ("mq-mix-counts", LIST_ORDERED, NULL);
  songList = nlistAlloc ("mq-mix-song-list", LIST_ORDERED, NULL);

  musicqLen = musicqGetLen (mainData->musicQueue, mqidx);
  logMsg (LOG_DBG, LOG_BASIC, "mix: mq len: %" PRId32, musicqLen);
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
    if (song == NULL) {
      continue;
    }
    danceIdx = songGetNum (song, TAG_DANCE);
    if (danceIdx >= 0) {
      nlistIncrement (danceCounts, danceIdx);
      ++totcount;
    } else {
      logMsg (LOG_DBG, LOG_BASIC, "mix: unknown dance skipped dbidx:%" PRId32, dbidx);
    }
  }
  logMsg (LOG_DBG, LOG_BASIC, "mix: total count: %d", totcount);
  logMsg (LOG_DBG, LOG_BASIC, "mix: counts len: %" PRId32, nlistGetCount (danceCounts));

  /* for the purposes of a mix, the countlist passed to the dancesel alloc */
  /* and the dance counts used for selection are identical */

  musicqClear (mainData->musicQueue, mqidx, 1);
  /* should already be an empty head on the queue */

  mainData->musicqLookupIdx = mqidx;
  dancesel = danceselAlloc (danceCounts, mainMusicQueueLookup, mainData);
  /* for a mix, grouping is not used */
  songsel = songselAlloc (mainData->musicdb, danceCounts);
  songselInitialize (songsel, songList, NULL);

  currlen = 0;
  while (currlen < totcount && failcount < 100) {
    /* as there is always an empty head on the music queue, */
    /* the prior-index must point at currlen + 1 */
    danceIdx = danceselSelect (dancesel, currlen + 1);
    if (danceIdx < 0) {
      /* selection failed */
      ++failcount;
      continue;
    }

    /* check and see if this dance is used up */
    if (nlistGetNum (danceCounts, danceIdx) <= 0) {
      logMsg (LOG_DBG, LOG_INFO, "mix: dance %d/%s at 0", danceIdx,
          danceGetStr (dances, danceIdx, DANCE_DANCE));
      ++failcount;
      continue;
    }

    song = songselSelect (songsel, danceIdx);
    if (song != NULL) {
      int     plidx;
      int32_t dur;

      songselSelectFinalize (songsel, danceIdx);
      logMsg (LOG_DBG, LOG_BASIC, "mix: (%d) d:%d/%s select: %s",
          currlen, danceIdx,
          danceGetStr (dances, danceIdx, DANCE_DANCE),
          songGetStr (song, TAG_URI));
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
  mainData->marqueeChanged = true;
  if (failcount >= 100) {
    connSendMessage (mainData->conn, ROUTE_MANAGEUI, MSG_PROCESSING_FAIL, NULL);
  } else {
    connSendMessage (mainData->conn, ROUTE_MANAGEUI, MSG_PROCESSING_FINISH, NULL);
  }
}

static void
mainPlaybackFinishStopProcess (maindata_t *mainData, const char *args)
{
  mainMusicQueueFinish (mainData, args);

  /* also check if a switch will occur immediately afterwards */
  /* this handles the case where a pause-at-end was done for the last */
  /* song in an empty queue */
  if (mainData->switchQueueWhenEmpty) {
    int   currlen;

    currlen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);
    if (currlen == 0) {
      int tplayidx;

      tplayidx = mainLocateQueueWithSongs (mainData);
      currlen = musicqGetLen (mainData->musicQueue, tplayidx);
      if (currlen > 0) {
        /* this will send a message back to main and set the deferredplayidx */
        /* and process the music queue switch */
        mainSendPlayerUISwitchQueue (mainData, tplayidx);
      }
    }
  }
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
    snprintf (tmp, sizeof (tmp), "%" PRId32, dbidx);
    connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_SONG_FINISH, tmp);
  }
}

static void
mainStatusRequest (maindata_t *mainData, bdjmsgroute_t routefrom)
{
  char  tmp [40];

  mainSetMusicQueuesChanged (mainData);
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


  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  if (p == NULL) {
    return;
  }
  song = songAlloc ();
  songSetStr (song, TAG_URI, p);

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
  songParse (song, p, tdbidx);
  /* the temporary music-db flag will be set */
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
mainSendPlaybackGap (maindata_t *mainData)
{
  int         mqidx;
  int32_t     gap;
  char        tmp [40];

  mqidx = mainData->musicqPlayIdx;

  /* always get the playlist gap from the next song to be played */
  /* in the current queue, not the upcoming queue */
  /* special cases such as switching queues */
  /* are handled by the caller of this routine */
  gap = mainGetGap (mainData, mqidx, 1);

  if (mainData->lastGapSent == gap) {
    return;
  }

  snprintf (tmp, sizeof (tmp), "%" PRId32, gap);
  connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_SET_PLAYBACK_GAP, tmp);
  mainData->lastGapSent = gap;
}

static int
mainLocateQueueWithSongs (maindata_t *mainData)
{
  int   tplayidx;
  int   tcurrlen;
  int   origMusicqPlayIdx;

  origMusicqPlayIdx = mainData->musicqPlayIdx;
  tplayidx = musicqNextQueue (mainData->musicqPlayIdx);
  tcurrlen = musicqGetLen (mainData->musicQueue, tplayidx);

  /* locate a queue that has songs in it */
  while (tplayidx != origMusicqPlayIdx && tcurrlen == 0) {
    tplayidx = musicqNextQueue (tplayidx);
    tcurrlen = musicqGetLen (mainData->musicQueue, tplayidx);
  }

  return tplayidx;
}

static int32_t
mainGetGap (maindata_t *mainData, int mqidx, int index)
{
  int32_t     gap;
  int         playlistIdx;
  playlist_t  *playlist = NULL;
  int32_t     plgap = -1;

  playlistIdx = musicqGetPlaylistIdx (mainData->musicQueue, mqidx, index);
  if (playlistIdx != MUSICQ_PLAYLIST_EMPTY) {
    playlist = nlistGetData (mainData->playlistCache, playlistIdx);
  }

  gap = bdjoptGetNumPerQueue (OPT_Q_GAP, mqidx);
  /* special case: the manage ui playback queue has no gap */
  /* the manage ui playback queue otherwise inherits from the music q */
  if (mqidx == MUSICQ_MNG_PB) {
    gap = 0;
  }
  plgap = playlistGetConfigNum (playlist, PLAYLIST_GAP);
  /* if the playlist-gap is configured to the default, */
  /* it is a negative number */
  if (plgap >= 0) {
    gap = plgap;
  }

  return gap;
}

static void
mainSendPlayerUISwitchQueue (maindata_t *mainData, int mqidx)
{
  char    tmp [40];

  snprintf (tmp, sizeof (tmp), "%d", mqidx);
  connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_QUEUE_SWITCH, tmp);
}

static playlist_t *
mainNextPlaylist (maindata_t *mainData, int mqidx)
{
  playlistitem_t  *plitem = NULL;
  playlist_t      *playlist = NULL;

  plitem = queuePop (mainData->playlistQueue [mqidx]);
  mainPlaylistItemFree (plitem);
  plitem = queueGetFirst (mainData->playlistQueue [mqidx]);
  playlist = NULL;
  if (plitem != NULL) {
    playlist = plitem->playlist;
  }

  return playlist;
}

static void
mainSetMusicQueuesChanged (maindata_t *mainData)
{
  for (int i = 0; i < MUSICQ_MAX; ++i) {
    int   musicqLen;

    musicqLen = musicqGetLen (mainData->musicQueue, i);
    if (musicqLen <= 0) {
      continue;
    }
    mainData->musicqChanged [i] = MAIN_CHG_START;
    mainData->marqueeChanged = true;
  }
}

static bool
mainGetAnnounceFlag (maindata_t *mainData, int mqidx, int playlistIdx)
{
  bool    announceflag = false;
  int     tval;

  tval = bdjoptGetNumPerQueue (OPT_Q_PLAY_ANNOUNCE, mqidx);
  if (tval >= 0) {
    announceflag = tval;
  }
  if (announceflag != 1) {
    playlist_t  *playlist;

    playlist = nlistGetData (mainData->playlistCache, playlistIdx);
    tval = playlistGetConfigNum (playlist, PLAYLIST_ANNOUNCE);
    if (tval >= 0) {
      announceflag = tval;
    }
  }
  /* announcements should not be played for the song list editor */
  /* or the music manager */
  if (mqidx >= MUSICQ_PB_MAX) {
    announceflag = false;
  }

  return (bool) announceflag;
}

/* routines for testing */

static void
mainQueueInfoRequest (maindata_t *mainData, bdjmsgroute_t routefrom,
    const char *args)
{
  char        tbuff [200];
  char        *sbuff = NULL;
  qidx_t      musicqLen;
  slistidx_t  dbidx;
  song_t      *song;
  int         musicqidx;
  char        *sp;
  char        *send;

  logProcBegin ();

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
  snprintf (sbuff, BDJMSG_MAX, "%" PRId32 "%c", musicqLen, MSG_ARGS_RS);
  sp = sbuff + strlen (sbuff);
  send = sbuff + BDJMSG_MAX;

  for (int i = 0; i <= musicqLen; ++i) {
    int32_t     dur;
    int         plidx;
    int32_t     gap;

    dbidx = musicqGetByIdx (mainData->musicQueue, musicqidx, i);
    song = dbGetByIdx (mainData->musicdb, dbidx);
    if (song == NULL) {
      continue;
    }

    plidx = musicqGetPlaylistIdx (mainData->musicQueue, musicqidx, i);

    snprintf (tbuff, sizeof (tbuff), "%d%c", dbidx, MSG_ARGS_RS);
    sp = stpecpy (sp, send, tbuff);

    dur = mainCalculateSongDuration (mainData, song, plidx, musicqidx, MAIN_CALC_NO_SPEED);
    snprintf (tbuff, sizeof (tbuff), "%" PRId32 "%c", dur, MSG_ARGS_RS);
    sp = stpecpy (sp, send, tbuff);

    gap = mainGetGap (mainData, musicqidx, i);

    snprintf (tbuff, sizeof (tbuff), "%" PRId32 "%c", gap, MSG_ARGS_RS);
    sp = stpecpy (sp, send, tbuff);
  }

  connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_MAIN_QUEUE_INFO, sbuff);
  dataFree (sbuff);
  logProcEnd ("");
}

static bool
mainCheckMusicQStopTime (maindata_t *mainData, time_t nStopTime, int mqidx)
{
  time_t  currTime;
  time_t  qDuration;
  bool    stopatflag = false;

  currTime = mstime ();
  qDuration = musicqGetDuration (mainData->musicQueue, mqidx);
  if (currTime + qDuration > nStopTime) {
    stopatflag = true;
  }

  return stopatflag;
}

static void
mainChkMusicq (maindata_t *mainData, bdjmsgroute_t routefrom)
{
  char        tmp [2000];
  dbidx_t     dbidx;
  dbidx_t     qdbidx;
  dbidx_t     mqdbidx [MUSICQ_PB_MAX][MAIN_TS_DEBUG_MAX];
  const char  *title;
  const char  *dance;
  song_t      *song;
  const char  *songfn;
  int         bpm;

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
    dance = mainSongGetDanceDisplay (mainData, mainData->musicqPlayIdx, 0);
    song = dbGetByIdx (mainData->musicdb, dbidx);
    songfn = songGetStr (song, TAG_URI);
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
      "mqplay%c%d%c"
      "mq0len%c%" PRId32 "%c"
      "mq1len%c%" PRId32 "%c"
      "mq2len%c%" PRId32 "%c"
      "dbidx%c%" PRId32 "%c"
      "bpm%c%d%c"
      "qdbidx%c%" PRId32 "%c"
      "m-songfn%c%s%c"
      "title%c%s%c"
      "dance%c%s%c"
      "mq0idx0%c%" PRId32 "%c"
      "mq0idx1%c%" PRId32 "%c"
      "mq0idx2%c%" PRId32 "%c"
      "mq0idx3%c%" PRId32 "%c"
      "mq0idx4%c%" PRId32 "%c"
      "mq0idx5%c%" PRId32 "%c"
      "mq1idx0%c%" PRId32 "%c"
      "mq1idx1%c%" PRId32 "%c"
      "mq1idx2%c%" PRId32 "%c"
      "mq1idx3%c%" PRId32 "%c"
      "mq1idx4%c%" PRId32 "%c"
      "mq1idx5%c%" PRId32 "%c"
      "mq2idx0%c%" PRId32 "%c"
      "mq2idx1%c%" PRId32 "%c"
      "mq2idx2%c%" PRId32 "%c"
      "mq2idx3%c%" PRId32 "%c"
      "mq2idx4%c%" PRId32 "%c"
      "mq2idx5%c%" PRId32 "%c"
      "pae0%c%d%c"
      "pae1%c%d%c"
      "pae2%c%d%c"
      "pae3%c%d%c"
      "pae4%c%d%c"
      "pae5%c%d%c"
      "songplaysentcount%c%d",
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
      MSG_ARGS_RS, (musicqGetFlags (mainData->musicQueue, mainData->musicqPlayIdx, 0) & MUSICQ_FLAG_PAUSE) ? 1 : 0, MSG_ARGS_RS,
      MSG_ARGS_RS, (musicqGetFlags (mainData->musicQueue, mainData->musicqPlayIdx, 1) & MUSICQ_FLAG_PAUSE) ? 1 : 0, MSG_ARGS_RS,
      MSG_ARGS_RS, (musicqGetFlags (mainData->musicQueue, mainData->musicqPlayIdx, 2) & MUSICQ_FLAG_PAUSE) ? 1 : 0, MSG_ARGS_RS,
      MSG_ARGS_RS, (musicqGetFlags (mainData->musicQueue, mainData->musicqPlayIdx, 3) & MUSICQ_FLAG_PAUSE) ? 1 : 0, MSG_ARGS_RS,
      MSG_ARGS_RS, (musicqGetFlags (mainData->musicQueue, mainData->musicqPlayIdx, 4) & MUSICQ_FLAG_PAUSE) ? 1 : 0, MSG_ARGS_RS,
      MSG_ARGS_RS, (musicqGetFlags (mainData->musicQueue, mainData->musicqPlayIdx, 5) & MUSICQ_FLAG_PAUSE) ? 1 : 0, MSG_ARGS_RS,
      MSG_ARGS_RS, mainData->songplaysentcount);
  connSendMessage (mainData->conn, routefrom, MSG_CHK_MAIN_MUSICQ, tmp);
}

static void
mainProcessPlayerState (maindata_t *mainData, char *data)
{
  mp_playerstate_t *ps;

  ps = msgparsePlayerStateData (data);
  mainData->playerState = ps->playerState;
  logMsg (LOG_DBG, LOG_MSGS, "got: pl-state: %d/%s",
      mainData->playerState, logPlayerState (mainData->playerState));
  mainData->marqueeChanged = true;

  msgparsePlayerStateFree (ps);
}
