/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
/*
 * bdj4player
 *  Does the actual playback of the music.
 *  Handles volume changes, fades.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#if _hdr_pthread
# include <pthread.h>
#endif

#include "audiosrc.h"
#include "bdj4.h"
#include "bdj4init.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "conn.h"
#include "dyintfc.h"
#include "filemanip.h"
#include "fileop.h"
#include "ilist.h"
#include "lock.h"
#include "log.h"
#include "mdebug.h"
#include "msgparse.h"
#include "osenv.h"
#include "ossignal.h"
#include "pathbld.h"
#include "pathutil.h"
#include "player.h"
#include "pli.h"
#include "progstate.h"
#include "queue.h"
#include "sock.h"
#include "sockh.h"
#include "songutil.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"
#include "volreg.h"
#include "volsink.h"
#include "volume.h"

#define DEBUG_PREP_QUEUE 0
/* threads working 2025-7-30 */
#define PLAYER_USE_THREADS 1

enum {
  STOP_NEXTSONG = 0,
  STOP_NORMAL = 1,
  STATUS_NO_FORCE = 0,
  STATUS_FORCE = 1,
  FADEIN_TIMESLICE = 50,
  FADEOUT_TIMESLICE = 100,
  PLAYER_MAX_PREP = 10,
#if PLAYER_USER_THREADS
  /* a large number is needed for downloading via the bdj4/bdj4 connection */
  /* for local playback, the retry generally doesn't take more than a */
  /* couple tries */
  /* 200 (* 10ms) handles a normal length song */
  PLAYER_RETRY_COUNT = 800,
#else
  PLAYER_RETRY_COUNT = 4,
#endif
};

enum {
  PREP_QUEUE_IDENT = 0xbbaa007170657270,
};

typedef struct {
  uint64_t      ident;
  char          *songname;
  char          tempname [MAXPATHLEN];
  int32_t       dur;
  int32_t       plidur;
  listnum_t     songstart;
  double        voladjperc;
  int32_t       uniqueidx;
  int           speed;
  /* one of PREP_SONG or PREP_ANNOUNCE */
  int           announce;
  /* audio-src is not robust, it is only used to determine */
  /* if the song is a file-type */
  int           audiosrc;
} prepqueue_t;

typedef struct {
  int32_t       uniqueidx;
  char          *songname;
} playrequest_t;

typedef struct {
#if _lib_pthread_create
  pthread_t     thread;
#endif
  prepqueue_t   *npq;
  int           idx;
  int           rc;
  _Atomic(bool) finished;
} prepthread_t;

typedef struct {
  conn_t          *conn;
  progstate_t     *progstate;
  char            *locknm;
  long            globalCount;
  pli_t           *pli;
  volume_t        *volume;
  queue_t         *prepRequestQueue;
  queue_t         *prepQueue;
  qidx_t          prepiteridx;
  prepqueue_t     *currentSong;
  queue_t         *playRequest;
  prepthread_t    *prepthread [PLAYER_MAX_PREP];
  int             pliSupported;
  int             originalSystemVolume;
  /* the real volume including adjustments. */
  int             realVolume;
  /* current volume without adjustments. */
  /* this is the volume that the user has set. */
  /* this is what is displayed in the player ui. */
  int             currentVolume;
  /* the base volume is the current volume before any changes. */
  /* it is the volume before any changes have been made to the volume. */
  /* it is set when a song starts playing.  */
  /* the base volume is used by the quick-edit process */
  /* in order to be able to calculate the volume adjustment and reset */
  /* the volume back to the base volume level. */
  int             baseVolume;
  /* testsuite: the actual volume is used by the test suite. */
  /* it closely follows the real volume. */
  int             actualVolume;
  int             currentSpeed;
  const char      *actualSink;
  const char      *currentSink;
  mstime_t        statusCheck;
  volsinklist_t   sinklist;
  playerstate_t   playerState;
  playerstate_t   lastPlayerState;      // used by sendstatus
  mstime_t        playTimeStart;
  ssize_t         playTimePlayed;
  mstime_t        playTimeCheck;
  mstime_t        playEndCheck;
  mstime_t        fadeTimeCheck;
  mstime_t        volumeTimeCheck;
  int             newSpeed;
  long            priorGap;           // used for announcements
  long            gap;
  mstime_t        gapFinishTime;
  int             fadeType;
  listnum_t       fadeinTime;
  listnum_t       fadeoutTime;
  int             fadeCount;
  int             fadeSamples;
  time_t          fadeTimeStart;
  mstime_t        fadeTimeNext;
  int             stopNextsongFlag;
  int             stopwaitcount;
  int             threadcount;
  bool            inFade;
  bool            inFadeIn;
  bool            inFadeOut;
  bool            inGap;
  bool            mute;
  bool            newsong;      // used in the player status msg
  bool            pauseAtEnd;
  bool            repeat;
  bool            speedWaitChg;
  bool            stopPlaying;
} playerdata_t;

static void     playerCheckSystemVolume (playerdata_t *playerData);
static int      playerProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static int      playerProcessing (void *udata);
static bool     playerConnectingCallback (void *tpdata, programstate_t programState);
static bool     playerHandshakeCallback (void *tpdata, programstate_t programState);
static bool     playerStoppingCallback (void *tpdata, programstate_t programState);
static bool     playerStopWaitCallback (void *udata, programstate_t programState);
static bool     playerClosingCallback (void *tpdata, programstate_t programState);
static void     playerSongPrep (playerdata_t *playerData, char *sfname);
static void     playerSongClearPrep (playerdata_t *playerData, char *sfname);
#if _lib_pthread_create && PLAYER_USE_THREADS
static void * playerThreadPrepRequest (void *arg);
#endif
void            playerProcessPrepRequest (playerdata_t *playerData);
static void     playerSongPlay (playerdata_t *playerData, char *args);
static prepqueue_t * playerLocatePreppedSong (playerdata_t *playerData, int32_t uniqueidx, const char *sfname, int externalreq);
static void     playerPause (playerdata_t *playerData);
static void     playerPlay (playerdata_t *playerData);
static void     playerNextSong (playerdata_t *playerData);
static void     playerPauseAtEnd (playerdata_t *playerData);
static void     playerSendPauseAtEndState (playerdata_t *playerData);
static void     playerFade (playerdata_t *playerData);
static void     playerSpeed (playerdata_t *playerData, char *trate);
static void     playerChangeSpeed (playerdata_t *playerData, int speed);
static void     playerSeek (playerdata_t *playerData, ssize_t pos);
static void     playerStop (playerdata_t *playerData);
static void     playerSongBegin (playerdata_t *playerData);
static void     playerVolumeSet (playerdata_t *playerData, char *tvol);
static void     playerCheckVolumeSink (playerdata_t *playerData);
static void     playerVolumeMute (playerdata_t *playerData);
static void     playerPrepQueueFree (void *);
static void     playerSigHandler (int sig);
static int      playerSetAudioSink (playerdata_t *playerData, const char *sinkname);
static void     playerInitSinkList (playerdata_t *playerData);
static void     playerFadeVolSet (playerdata_t *playerData);
static double   calcFadeIndex (playerdata_t *playerData, int fadeType);
static void     playerStartFadeOut (playerdata_t *playerData);
static void     playerSetCheckTimes (playerdata_t *playerData, prepqueue_t *pq);
static void     playerSetPlayerState (playerdata_t *playerData, playerstate_t pstate);
static void     playerSendStatus (playerdata_t *playerData, bool forceFlag);
static int      playerLimitVolume (int vol);
static ssize_t  playerCalcPlayedTime (playerdata_t *playerData);
static void     playerSetDefaultVolume (playerdata_t *playerData);
static void     playerFreePlayRequest (void *tpreq);
static void     playerChkPlayerStatus (playerdata_t *playerData, int routefrom);
static void     playerChkPlayerSong (playerdata_t *playerData, int routefrom);
static void     playerResetVolume (playerdata_t *playerData);
static void     playerSetAudioSinkEnv (playerdata_t *playerData, bool isdefault);
static const char * playerGetAudioInterface (void);
static void playerCheckPrepThreads (playerdata_t *playerData);
#if DEBUG_PREP_QUEUE
/* note that this will reset the queue iterator */
static void playerDumpPrepQueue (playerdata_t *playerData, const char *tag);
#endif

static int      gKillReceived = 0;

int
main (int argc, char *argv[])
{
  playerdata_t    playerData;
  uint16_t        listenPort;
  uint32_t        flags;
  const char      *audiosink;
  const char      *plintfc;
  char            *volintfc;
  int             plidevtype;

#if BDJ4_MEM_DEBUG
  mdebugInit ("play");
#endif
  osSetStandardSignals (playerSigHandler);

  playerData.currentSong = NULL;
  playerData.fadeCount = 0;
  playerData.fadeSamples = 1;
  playerData.globalCount = 1;
  playerData.playerState = PL_STATE_STOPPED;
  playerData.lastPlayerState = PL_STATE_UNKNOWN;
  mstimestart (&playerData.playTimeStart);
  playerData.playTimePlayed = 0;
  playerData.playRequest = queueAlloc ("play-request", playerFreePlayRequest);
  mstimeset (&playerData.statusCheck, TM_TIMER_OFF);
  playerData.priorGap = 2000;
  playerData.gap = 2000;
  playerData.pli = NULL;
  playerData.pliSupported = PLI_SUPPORT_NONE;
  playerData.prepQueue = queueAlloc ("prep-q", playerPrepQueueFree);
  playerData.prepRequestQueue = queueAlloc ("prep-req", playerPrepQueueFree);
  playerData.progstate = progstateInit ("player");
  playerData.stopNextsongFlag = STOP_NORMAL;
  playerData.stopwaitcount = 0;
  playerData.threadcount = 0;
  for (int i = 0; i < PLAYER_MAX_PREP; ++i) {
    playerData.prepthread [i] = NULL;
  }
  playerData.newSpeed = 100;
  playerData.inFade = false;
  playerData.inFadeIn = false;
  playerData.inFadeOut = false;
  playerData.inGap = false;
  playerData.mute = false;
  playerData.newsong = false;
  playerData.pauseAtEnd = false;
  playerData.repeat = false;
  playerData.speedWaitChg = false;
  playerData.stopPlaying = false;

  progstateSetCallback (playerData.progstate, STATE_CONNECTING,
      playerConnectingCallback, &playerData);
  progstateSetCallback (playerData.progstate, STATE_WAIT_HANDSHAKE,
      playerHandshakeCallback, &playerData);
  progstateSetCallback (playerData.progstate, STATE_STOPPING,
      playerStoppingCallback, &playerData);
  progstateSetCallback (playerData.progstate, STATE_STOP_WAIT,
      playerStopWaitCallback, &playerData);
  progstateSetCallback (playerData.progstate, STATE_CLOSING,
      playerClosingCallback, &playerData);

  flags = BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  bdj4startup (argc, argv, NULL, "play", ROUTE_PLAYER, &flags);

  playerData.conn = connInit (ROUTE_PLAYER);

  playerData.fadeType = bdjoptGetNum (OPT_P_FADETYPE);
  playerData.fadeinTime = bdjoptGetNumPerQueue (OPT_Q_FADEINTIME, 0);
  playerData.fadeoutTime = bdjoptGetNumPerQueue (OPT_Q_FADEOUTTIME, 0);

  playerData.currentSink = "";  // default
  playerData.currentSpeed = 100;

  volintfc = volumeCheckInterface (bdjoptGetStr (OPT_M_VOLUME_INTFC));
  bdjoptSetStr (OPT_M_VOLUME_INTFC, volintfc);
  logMsg (LOG_DBG, LOG_IMPORTANT, "volume interface: %s", volintfc);
  mdfree (volintfc);
  playerData.volume = volumeInit (bdjoptGetStr (OPT_M_VOLUME_INTFC));

  volumeInitSinkList (&playerData.sinklist);
  playerInitSinkList (&playerData);

  /* some audio device interfaces may not have the audio device enumeration. */
  /* in this case, retrieve the list of devices from the player if possible. */
  if (! volumeHaveSinkList (playerData.volume)) {
    /* try getting the sink list from the player */
    pliAudioDeviceList (playerData.pli, &playerData.sinklist);
    if (playerData.sinklist.sinklist != NULL) {
      logMsg (LOG_DBG, LOG_BASIC, "vlc-sinklist");
      for (int i = 0; i < playerData.sinklist.count; ++i) {
        logMsg (LOG_DBG, LOG_BASIC, "  %d %3d %s %s",
            playerData.sinklist.sinklist [i].defaultFlag,
            playerData.sinklist.sinklist [i].idxNumber,
            playerData.sinklist.sinklist [i].name,
            playerData.sinklist.sinklist [i].description);
      }
    }
  }

  /* vlc must be checked for a switch in versions */
  plintfc = playerGetAudioInterface ();

  /* sets the current sink */
  audiosink = bdjoptGetStr (OPT_MP_AUDIOSINK);
  plidevtype = playerSetAudioSink (&playerData, audiosink);

  logMsg (LOG_DBG, LOG_IMPORTANT, "player interface: %s", plintfc);
  logMsg (LOG_DBG, LOG_IMPORTANT, "volume sink: %s", playerData.actualSink);
  playerData.pli = pliInit (plintfc, bdjoptGetStr (OPT_M_PLAYER_INTFC_NM));
  playerData.pliSupported = pliSupported (playerData.pli);

  /* vlc needs to have the audio device set */
  pliSetAudioDevice (playerData.pli, playerData.actualSink, plidevtype);

  playerSetDefaultVolume (&playerData);

  listenPort = bdjvarsGetNum (BDJVL_PORT_PLAYER);
  sockhMainLoop (listenPort, playerProcessMsg, playerProcessing, &playerData);
  connFree (playerData.conn);
  progstateFree (playerData.progstate);
  logEnd ();
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return 0;
}

/* internal routines */

static bool
playerStoppingCallback (void *tpdata, programstate_t programState)
{
  playerdata_t    *playerData = tpdata;

  logProcBegin ();
  connDisconnectAll (playerData->conn);
  logProcEnd ("");
  return STATE_FINISHED;
}

static bool
playerStopWaitCallback (void *udata, programstate_t programState)
{
  playerdata_t  *playerData = udata;
  bool          rc;

  rc = connWaitClosed (playerData->conn, &playerData->stopwaitcount);
  return rc;
}

static bool
playerClosingCallback (void *tpdata, programstate_t programState)
{
  playerdata_t  *playerData = tpdata;

  logProcBegin ();

  volumeSet (playerData->volume, playerData->currentSink, 0);

  /* the prep queues need to be freed before audiosrc is cleaned */
  queueFree (playerData->prepQueue);
  queueFree (playerData->prepRequestQueue);
  queueFree (playerData->playRequest);

  if (playerData->currentSong != NULL) {
    if (playerData->currentSong->announce != PREP_ANNOUNCE) {
      playerPrepQueueFree (playerData->currentSong);
      playerData->currentSong = NULL;
    }
  }

  bdj4shutdown (ROUTE_PLAYER, NULL);

  if (playerData->pli != NULL) {
    pliStop (playerData->pli);
    pliClose (playerData->pli);
    pliFree (playerData->pli);
  }

  /* do the volume reset last, give time for the player to stop */

  mssleep (100);
  playerResetVolume (playerData);
  /* note that if there are BDJ4 instances with different sinks */
  /* the bdj4 flag will be improperly cleared */
  volregClearBDJ4Flag ();
  volumeFreeSinkList (&playerData->sinklist);
  volumeFree (playerData->volume);

  logProcEnd ("");
  return STATE_FINISHED;
}

static int
playerProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  playerdata_t      *playerData;

  logProcBegin ();
  playerData = (playerdata_t *) udata;

  logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
      routefrom, msgRouteDebugText (routefrom),
      route, msgRouteDebugText (route), msg, msgDebugText (msg), args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_PLAYER: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (playerData->conn, routefrom);
          /* force a player-status-data message */
          playerData->lastPlayerState = PL_STATE_UNKNOWN;
          break;
        }
        case MSG_SOCKET_CLOSE: {
          connDisconnect (playerData->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          progstateShutdownProcess (playerData->progstate);
          break;
        }
        case MSG_PLAY_PAUSE: {
          logMsg (LOG_DBG, LOG_MSGS, "got: pause:");
          playerPause (playerData);
          break;
        }
        case MSG_PLAY_PLAY: {
          logMsg (LOG_DBG, LOG_MSGS, "got: play");
          playerPlay (playerData);
          break;
        }
        case MSG_PLAY_PLAYPAUSE: {
          logMsg (LOG_DBG, LOG_MSGS, "got: playpause");
          if (playerData->playerState == PL_STATE_PLAYING ||
             playerData->playerState == PL_STATE_IN_FADEOUT) {
            playerPause (playerData);
          } else {
            playerPlay (playerData);
          }
          break;
        }
        case MSG_PLAY_FADE: {
          logMsg (LOG_DBG, LOG_MSGS, "got: fade:");
          playerFade (playerData);
          break;
        }
        case MSG_PLAY_REPEAT: {
          logMsg (LOG_DBG, LOG_MSGS, "got: repeat");
          playerData->repeat = playerData->repeat ? false : true;
          playerSendStatus (playerData, STATUS_FORCE);
          break;
        }
        case MSG_PLAY_PAUSEATEND: {
          logMsg (LOG_DBG, LOG_MSGS, "got: pauseatend");
          playerPauseAtEnd (playerData);
          break;
        }
        case MSG_PLAY_NEXTSONG: {
          playerNextSong (playerData);
          break;
        }
        case MSG_PLAY_SPEED: {
          playerSpeed (playerData, args);
          break;
        }
        case MSG_PLAYER_VOL_MUTE: {
          logMsg (LOG_DBG, LOG_MSGS, "got: volmute");
          playerVolumeMute (playerData);
          break;
        }
        case MSG_PLAYER_VOLUME: {
          logMsg (LOG_DBG, LOG_MSGS, "got: volume %s", args);
          playerVolumeSet (playerData, args);
          break;
        }
        case MSG_PLAYER_SUPPORT: {
          char  tmp [40];

          snprintf (tmp, sizeof (tmp), "%d", playerData->pliSupported);
          connSendMessage (playerData->conn, routefrom, MSG_PLAYER_SUPPORT, tmp);
          break;
        }
        case MSG_PLAY_STOP: {
          logMsg (LOG_DBG, LOG_MSGS, "got: stop");
          playerStop (playerData);
          playerData->pauseAtEnd = false;
          playerSendPauseAtEndState (playerData);
          playerSetPlayerState (playerData, PL_STATE_STOPPED);
          break;
        }
        case MSG_PLAY_SONG_BEGIN: {
          logMsg (LOG_DBG, LOG_MSGS, "got: song begin");
          playerSongBegin (playerData);
          break;
        }
        case MSG_PLAY_SEEK: {
          logMsg (LOG_DBG, LOG_MSGS, "got: seek");
          playerSeek (playerData, atol (args));
          break;
        }
        case MSG_SONG_PREP: {
          playerSongPrep (playerData, args);
          break;
        }
        case MSG_SONG_CLEAR_PREP: {
          playerSongClearPrep (playerData, args);
          break;
        }
        case MSG_SONG_PLAY: {
          playerSongPlay (playerData, args);
          break;
        }
        case MSG_MAIN_READY: {
          mstimeset (&playerData->statusCheck, 0);
          break;
        }
        case MSG_SET_PLAYBACK_GAP: {
          playerData->gap = atol (args);
          playerData->priorGap = playerData->gap;
          break;
        }
        case MSG_SET_PLAYBACK_FADEIN: {
          playerData->fadeinTime = atol (args);
          break;
        }
        case MSG_SET_PLAYBACK_FADEOUT: {
          playerData->fadeoutTime = atol (args);
          break;
        }
        case MSG_PLAY_RESET_VOLUME: {
          playerData->currentVolume = playerData->baseVolume;
          break;
        }
        case MSG_CHK_PLAYER_STATUS: {
          playerChkPlayerStatus (playerData, routefrom);
          break;
        }
        case MSG_CHK_PLAYER_SONG: {
          playerChkPlayerSong (playerData, routefrom);
          break;
        }
        case MSG_CHK_CLEAR_PREP_Q: {
          queueClear (playerData->prepQueue, 0);
          queueRemoveByIdx (playerData->prepQueue, 0);
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

  logProcEnd ("");
  return 0;
}

static int
playerProcessing (void *udata)
{
  playerdata_t  *playerData = udata;
  int           stop = SOCKH_CONTINUE;

  if (! progstateIsRunning (playerData->progstate)) {
    progstateProcess (playerData->progstate);
    if (progstateCurrState (playerData->progstate) == STATE_CLOSED) {
      stop = SOCKH_STOP;
    }
    if (gKillReceived) {
      progstateShutdownProcess (playerData->progstate);
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    }
    return stop;
  }

  connProcessUnconnected (playerData->conn);

  playerCheckPrepThreads (playerData);

  if (mstimeCheck (&playerData->statusCheck)) {
    /* the playerSendStatus() routine will set the statusCheck var */
    playerSendStatus (playerData, STATUS_NO_FORCE);
  }

  if (playerData->inFade) {
    if (mstimeCheck (&playerData->fadeTimeNext)) {
      playerFadeVolSet (playerData);
    }
  }

  if (playerData->playerState == PL_STATE_IN_GAP &&
      playerData->inGap) {
    if (mstimeCheck (&playerData->gapFinishTime)) {
      playerData->inGap = false;
      logMsg (LOG_DBG, LOG_BASIC, "gap finish");
      playerSetPlayerState (playerData, PL_STATE_STOPPED);
    }
  }

  if (playerData->playerState == PL_STATE_STOPPED &&
      ! playerData->inGap &&
      queueGetCount (playerData->playRequest) > 0) {
    prepqueue_t   *pq = NULL;
    playrequest_t *preq = NULL;
    bool          temprepeat = false;
    char          tempffn [MAXPATHLEN];
    int           tspeed;
    int           taudiosrc;


    temprepeat = playerData->repeat;

    pq = playerData->currentSong;
    if (pq != NULL && pq->announce == PREP_ANNOUNCE) {
      connSendMessage (playerData->conn, ROUTE_MAIN,
          MSG_PLAYER_ANN_FINISHED, NULL);
    }

    /* announcements are not repeated */
    if (playerData->repeat) {
      pq = playerData->currentSong;
      if (pq != NULL && pq->announce == PREP_ANNOUNCE) {
        pq = NULL;
        /* The user pressed the repeat toggle while the announcement */
        /* was playing.  Force the code below to activate as if no */
        /* repeat was on.  The repeat flag will be restored. */
        playerData->repeat = false;
      }
    }

    /* the pq == null condition occurs if repeat is turned on before */
    /* playing the first song */
    if (! playerData->repeat || pq == NULL) {
      preq = queueGetFirst (playerData->playRequest);
      pq = playerLocatePreppedSong (playerData, preq->uniqueidx, preq->songname, false);
      if (pq == NULL) {
        preq = queuePop (playerData->playRequest);
        playerFreePlayRequest (preq);
        if (gKillReceived) {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
        }
        return gKillReceived;
      }

      playerData->currentSong = pq;
      /* announcements are not removed from the prep queue */
      if (pq->announce == PREP_SONG) {
        logMsg (LOG_DBG, LOG_BASIC, "prep rm node (no free) %s", pq->songname);
        queueIterateRemoveNode (playerData->prepQueue, &playerData->prepiteridx);
        /* do not free */
      }
    }
    playerData->repeat = temprepeat;

    /* save the prior current volume before playing a song */
    playerData->baseVolume = playerData->currentVolume;
    playerData->realVolume = playerData->currentVolume;
    if (pq->voladjperc != 0.0) {
      double      val;
      double      newvol;

      val = pq->voladjperc / 100.0 + 1.0;
      newvol = round ((double) playerData->currentVolume * val);
      newvol = playerLimitVolume (newvol);
      playerData->realVolume = (int) newvol;
    }

    if ((pq->announce == PREP_ANNOUNCE ||
        playerData->fadeinTime == 0) &&
        ! playerData->mute) {
      playerData->realVolume = playerData->currentVolume;
      volumeSet (playerData->volume, playerData->currentSink, playerData->realVolume);
      playerData->actualVolume = playerData->realVolume;
      logMsg (LOG_DBG, LOG_VOLUME, "no fade-in set volume: %d", playerData->realVolume);
    }

    *tempffn = '\0';
    /* only need to know if this is a file-type */
    if (pq->audiosrc == AUDIOSRC_TYPE_FILE) {
      /* some pli need the full path */
      pathbldMakePath (tempffn, sizeof (tempffn), pq->tempname, "",
          PATHBLD_MP_DIR_DATATOP);
    }


    taudiosrc = pq->audiosrc;
    if (taudiosrc == AUDIOSRC_TYPE_BDJ4) {
      taudiosrc = AUDIOSRC_TYPE_FILE;
    }
    pliMediaSetup (playerData->pli, pq->tempname, tempffn, taudiosrc);
    /* pq->songstart is normalized */

    tspeed = pq->speed;
    if (playerData->repeat) {
      /* if repeating, use the current speed */
      tspeed = playerData->currentSpeed;
    }
    pliStartPlayback (playerData->pli, pq->songstart, tspeed);
    playerData->currentSpeed = tspeed;
    playerSetPlayerState (playerData, PL_STATE_LOADING);
  }

  if (playerData->playerState == PL_STATE_LOADING) {
    prepqueue_t       *pq = playerData->currentSong;
    plistate_t        plistate;

    plistate = pliState (playerData->pli);
    logMsg (LOG_DBG, LOG_BASIC, "loading: pli-state: %d", plistate);
    if (plistate == PLI_STATE_OPENING ||
        plistate == PLI_STATE_BUFFERING) {
      ;
    } else if (plistate == PLI_STATE_PLAYING) {
      if (pq->dur <= 1) {
        int32_t   tdur;

        tdur = pliGetDuration (playerData->pli);
        /* some players may not have duration support */
        if (tdur >= 0) {
          pq->dur = tdur;
        }
        logMsg (LOG_DBG, LOG_INFO, "WARN: Replace duration with player data: %" PRId32, pq->dur);
      }

      /* save for later use */
      playerData->playTimePlayed = 0;
      playerSetCheckTimes (playerData, pq);

      if (pq->announce == PREP_SONG && playerData->fadeinTime > 0) {
        playerData->inFade = true;
        playerData->inFadeIn = true;
        playerData->fadeCount = 1;
        playerData->fadeSamples = playerData->fadeinTime / FADEIN_TIMESLICE + 1;
        playerData->fadeTimeStart = mstime ();
        playerFadeVolSet (playerData);
      }

      playerSetPlayerState (playerData, PL_STATE_PLAYING);

      if (pq->announce == PREP_SONG) {
        connSendMessage (playerData->conn, ROUTE_MAIN,
            MSG_PLAYBACK_BEGIN, NULL);
      }
    } else if (plistate == PLI_STATE_STOPPED) {
      playerSetPlayerState (playerData, PL_STATE_STOPPED);
    }
  }

  if (playerData->playerState == PL_STATE_PLAYING &&
      mstimeCheck (&playerData->volumeTimeCheck)) {
    playerCheckSystemVolume (playerData);
  }

  if (playerData->playerState == PL_STATE_PLAYING ||
      playerData->playerState == PL_STATE_IN_FADEOUT) {
    prepqueue_t       *pq = playerData->currentSong;

    /* announcements have no fade-out */
    if (pq->announce == PREP_SONG &&
        playerData->fadeoutTime > 0 &&
        ! playerData->inFade &&
        mstimeCheck (&playerData->fadeTimeCheck)) {

      /* before going into the fade, check the system volume */
      /* and see if the user changed it */
      logMsg (LOG_DBG, LOG_VOLUME, "check sysvol: before fade");
      playerCheckSystemVolume (playerData);
      playerStartFadeOut (playerData);
    }

    if (playerData->stopPlaying ||
        mstimeCheck (&playerData->playTimeCheck)) {
      int32_t     plitm;
      plistate_t  plistate;

      plistate = pliState (playerData->pli);
      if (pq->plidur < 1) {
        int32_t   tdur;
        tdur = pliGetDuration (playerData->pli);
        if (tdur >= 0) {
          pq->plidur = tdur;
        }
      }
      plitm = pliGetTime (playerData->pli);

      /* for a song with a speed adjustment, vlc returns the current */
      /* timestamp and the real duration, not adjusted values. */
      /* pq->dur is adjusted for the speed. */
      /* pli-time cannot be used in conjunction with pq->dur */
      if (plistate == PLI_STATE_STOPPED ||
          plistate == PLI_STATE_ERROR ||
          playerData->stopPlaying ||
          plitm >= pq->plidur ||
          mstimeCheck (&playerData->playEndCheck)) {
        char  nsflag [20];

        /* done, go on to next song, history is determined by */
        /* the stopnextsongflag */
        snprintf (nsflag, sizeof (nsflag), "%d", playerData->stopNextsongFlag);

        /* stop any fade */
        playerData->inFade = false;
        playerData->inFadeOut = false;
        playerData->inFadeIn = false;

        /* protect the stop-playing flag so that it propagates */
        /* past the announcement */
        if (pq->announce == PREP_SONG) {
          playerData->stopNextsongFlag = STOP_NORMAL;
          playerData->stopPlaying = false;
        }

        /* on repeat, preserve the current speed */
        if (! playerData->repeat) {
          playerData->currentSpeed = 100;
        }

        logMsg (LOG_DBG, LOG_BASIC, "actual play time: %" PRId64,
            (int64_t) mstimeend (&playerData->playTimeStart) + playerData->playTimePlayed);
        playerStop (playerData);

        if (pq->announce == PREP_SONG) {
          if (playerData->pauseAtEnd) {
            playerData->priorGap = playerData->gap;
            playerData->gap = 0;
            playerData->pauseAtEnd = false;
            playerSendPauseAtEndState (playerData);
            logMsg (LOG_DBG, LOG_BASIC, "pause-at-end");

            /* want the newsong flag to be sent with the stop state */
            if (! playerData->repeat) {
              /* let main know we're done with this song. */
              playerData->newsong = true;
              connSendMessage (playerData->conn, ROUTE_MAIN,
                  MSG_PLAYBACK_FINISH_STOP, nsflag);
            }
            playerSetPlayerState (playerData, PL_STATE_STOPPED);

            if (playerData->repeat) {
              playrequest_t *preq;

              /* for a repeat, clear out the current play request */
              /* to make the music stop */
              /* main will send a new play request when the music is started */
              preq = queuePop (playerData->playRequest);
              playerFreePlayRequest (preq);
            }
          } else {
            if (! playerData->repeat) {
              playerData->newsong = true;
              connSendMessage (playerData->conn, ROUTE_MAIN,
                  MSG_PLAYBACK_FINISH, nsflag);
            }
          }
        } /* song, not announcement */

        /* the play request for a song on repeat is left in the queue */
        /* so that the song will be re-played when finished. */
        /* announcements are always cleared */
        if (pq->announce == PREP_ANNOUNCE || ! playerData->repeat) {
          playrequest_t *preq;

          preq = queuePop (playerData->playRequest);
          playerFreePlayRequest (preq);
        }

        if (playerData->fadeoutTime == 0) {
          logMsg (LOG_DBG, LOG_VOLUME, "check sysvol: no-fade-out");
          playerCheckSystemVolume (playerData);
        }

        /* there is no gap after an announcement */
        if (pq->announce == PREP_SONG &&
            playerData->gap > 0) {
          playerSetPlayerState (playerData, PL_STATE_IN_GAP);
          playerData->realVolume = 0;
          volumeSet (playerData->volume, playerData->currentSink, 0);
          playerData->actualVolume = 0;
          logMsg (LOG_DBG, LOG_VOLUME, "gap set volume: %d", 0);
          playerData->inGap = true;
          mstimeset (&playerData->gapFinishTime, playerData->gap);
        } else {
          logMsg (LOG_DBG, LOG_BASIC, "no-gap");
          playerSetPlayerState (playerData, PL_STATE_STOPPED);
        }

        if (pq->announce == PREP_SONG) {
          /* protect the gap reset so it propagates past the announcement */
          playerData->gap = playerData->priorGap;

          if (! playerData->repeat) {
            playerPrepQueueFree (playerData->currentSong);
            playerData->currentSong = NULL;
          }
        }
      } /* has stopped */
    } /* time to check...*/
  } /* is playing */

  /* only process the prep requests when the player isn't doing much  */
  /* windows must do a physical copy, and this may take a bit of time */
  /* as of 4.15.4, the prep is done in a thread */
  if ((playerData->playerState == PL_STATE_PLAYING ||
       playerData->playerState == PL_STATE_STOPPED ||
       playerData->playerState == PL_STATE_PAUSED) &&
      queueGetCount (playerData->prepRequestQueue) > 0 &&
      ! playerData->inGap &&
      ! playerData->inFade) {
    playerProcessPrepRequest (playerData);
  }

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (playerData->progstate);
  }
  return stop;
}

static bool
playerConnectingCallback (void *tpdata, programstate_t programState)
{
  playerdata_t  *playerData = tpdata;
  bool          rc = STATE_NOT_FINISH;

  logProcBegin ();

  connProcessUnconnected (playerData->conn);

  if (! connIsConnected (playerData->conn, ROUTE_MAIN)) {
    connConnect (playerData->conn, ROUTE_MAIN);
  }

  if (connIsConnected (playerData->conn, ROUTE_MAIN)) {
    rc = STATE_FINISHED;
  }

  logProcEnd ("");
  return rc;
}

static bool
playerHandshakeCallback (void *tpdata, programstate_t programState)
{
  playerdata_t  *playerData = tpdata;
  bool          rc = STATE_NOT_FINISH;

  logProcBegin ();

  connProcessUnconnected (playerData->conn);

  if (connHaveHandshake (playerData->conn, ROUTE_MAIN)) {
    rc = STATE_FINISHED;
  }

  logProcEnd ("");
  return rc;
}


static void
playerCheckSystemVolume (playerdata_t *playerData)
{
  int         tvol;
  int         voldiff;

  logProcBegin ();
  mstimeset (&playerData->volumeTimeCheck, 1000);

  if (playerData->inFade || playerData->inGap || playerData->mute) {
    logProcEnd ("in-fade-gap-mute");
    return;
  }

  playerCheckVolumeSink (playerData);
  tvol = volumeGet (playerData->volume, playerData->currentSink);
  tvol = playerLimitVolume (tvol);
  // logMsg (LOG_DBG, LOG_VOLUME, "get volume: %d", tvol);
  voldiff = tvol - playerData->realVolume;
  if (tvol != playerData->realVolume) {
    playerData->realVolume += voldiff;
    playerData->realVolume = playerLimitVolume (playerData->realVolume);
    playerData->currentVolume += voldiff;
    playerData->currentVolume = playerLimitVolume (playerData->currentVolume);
  }
  logProcEnd ("");
}

static void
playerSongPrep (playerdata_t *playerData, char *args)
{
  prepqueue_t     *npq;
  char            *tokptr = NULL;
  char            *p;

  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  logProcBegin ();

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokptr);
  if (p == NULL) {
    logProcEnd ("no-data");
    return;
  }

  npq = mdmalloc (sizeof (prepqueue_t));
  npq->ident = PREP_QUEUE_IDENT;
  *npq->tempname = '\0';
  npq->songname = NULL;
  npq->audiosrc = AUDIOSRC_TYPE_FILE;

  npq->songname = mdstrdup (p);
  logMsg (LOG_DBG, LOG_BASIC, "prep request: %s", npq->songname);

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  npq->dur = atol (p);
  npq->plidur = 0;
  logMsg (LOG_DBG, LOG_INFO, "     duration: %" PRId32, npq->dur);

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  npq->songstart = atoll (p);
  logMsg (LOG_DBG, LOG_INFO, "     songstart: %" PRId64, npq->songstart);

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  npq->speed = atoi (p);
  logMsg (LOG_DBG, LOG_INFO, "     speed: %d", npq->speed);

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  npq->voladjperc = atof (p);
  logMsg (LOG_DBG, LOG_INFO, "     voladjperc: %.1f", npq->voladjperc);

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  npq->announce = atoi (p);
  logMsg (LOG_DBG, LOG_INFO, "     announce: %d", npq->announce);

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  npq->uniqueidx = atol (p);
  logMsg (LOG_DBG, LOG_INFO, "     uniqueidx: %" PRId32, npq->uniqueidx);

  queuePush (playerData->prepRequestQueue, npq);
  logMsg (LOG_DBG, LOG_INFO, "prep-req-add: %" PRId32 " %s r:%d p:%" PRId32, npq->uniqueidx, npq->songname, queueGetCount (playerData->prepRequestQueue), queueGetCount (playerData->prepQueue));
  logProcEnd ("");
}

static void
playerSongClearPrep (playerdata_t *playerData, char *args)
{
  prepqueue_t     *tpq;
  char            *tokptr = NULL;
  char            *p;
  int32_t         uniqueidx;

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokptr);
  if (p == NULL) {
    return;
  }
  uniqueidx = atol (p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  if (p == NULL) {
    return;
  }

  tpq = playerLocatePreppedSong (playerData, uniqueidx, p, true);
  if (tpq != NULL) {
    tpq = queueIterateRemoveNode (playerData->prepQueue, &playerData->prepiteridx);
    /* prevent any issues by checking the uniqueidx again */
    if (tpq != NULL && tpq->uniqueidx == uniqueidx) {
      logMsg (LOG_DBG, LOG_INFO, "prep-clear: %" PRId32 " %s r:%d p:%" PRId32, tpq->uniqueidx, tpq->songname, queueGetCount (playerData->prepRequestQueue), queueGetCount (playerData->prepQueue));
      playerPrepQueueFree (tpq);
    }
  } else {
    logMsg (LOG_DBG, LOG_INFO, "prep-clear: not located: %" PRId32 " %s", uniqueidx, p);
  }
}

#if _lib_pthread_create && PLAYER_USE_THREADS
static void *
playerThreadPrepRequest (void *arg)
{
  prepthread_t    *prepthread = (prepthread_t *) arg;
  prepqueue_t     *npq;

  npq = prepthread->npq;
  prepthread->rc = audiosrcPrep (npq->songname, npq->tempname,
      sizeof (npq->tempname));

  prepthread->finished = true;
  pthread_exit (NULL);
  return NULL;
}
#endif

void
playerProcessPrepRequest (playerdata_t *playerData)
{
  prepqueue_t     *npq;
#if _lib_pthread_create && PLAYER_USE_THREADS
  prepthread_t    *prepthread;
#else
  int             rc;
#endif

  logProcBegin ();

#if _lib_pthread_create && PLAYER_USE_THREADS
  if (playerData->threadcount >= PLAYER_MAX_PREP) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "out of prep space");
    logProcEnd ("no-space");
    return;
  }
#endif

  npq = queuePop (playerData->prepRequestQueue);
  if (npq == NULL) {
    logProcEnd ("no-song");
    return;
  }

  npq->audiosrc = audiosrcGetType (npq->songname);

#if _lib_pthread_create && PLAYER_USE_THREADS
  prepthread = mdmalloc (sizeof (prepthread_t));
  prepthread->npq = npq;
  prepthread->finished = false;
  prepthread->idx = playerData->threadcount;
  prepthread->rc = false;

  playerData->prepthread [playerData->threadcount] = prepthread;
  ++playerData->threadcount;

  pthread_create (&prepthread->thread, NULL, playerThreadPrepRequest, prepthread);
#else
  rc = audiosrcPrep (npq->songname, npq->tempname, sizeof (npq->tempname));
#endif

#if ! _lib_pthread_create || ! PLAYER_USE_THREADS
  if (! rc) {
    logProcEnd ("unable-to-prep");
  }

  queuePush (playerData->prepQueue, npq);
  logMsg (LOG_DBG, LOG_INFO, "prep-do: %" PRId32 " %s r:%d p:%" PRId32, npq->uniqueidx, npq->songname, queueGetCount (playerData->prepRequestQueue), queueGetCount (playerData->prepQueue));
  logMsg (LOG_DBG, LOG_INFO, "prep-tempname: %s", npq->tempname);
#endif
  logProcEnd ("");
}

static void
playerSongPlay (playerdata_t *playerData, char *args)
{
  prepqueue_t   *pq = NULL;
  playrequest_t *preq = NULL;
  char          *p;
  char          *tokstr = NULL;
  int32_t       uniqueidx;
  int           count;

  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  logProcBegin ();

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  if (p == NULL) {
    logProcEnd ("bad-msg-a");
    return;
  }
  uniqueidx = atol (p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (p == NULL) {
    logProcEnd ("bad-msg-b");
    return;
  }

  logMsg (LOG_DBG, LOG_BASIC, "play request: %" PRId32 " %s", uniqueidx, p);
  pq = playerLocatePreppedSong (playerData, uniqueidx, p, false);
  count = 0;
  while (pq == NULL && count < 5) {
    mssleep (10);
    playerCheckPrepThreads (playerData);
    pq = playerLocatePreppedSong (playerData, uniqueidx, p, false);
    ++count;
  }
  if (pq == NULL) {
    connSendMessage (playerData->conn, ROUTE_MAIN, MSG_PLAYBACK_FINISH, "0");
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: not prepped: %s", p);
    logProcEnd ("not-prepped");
    return;
  }

  if (pq->audiosrc == AUDIOSRC_TYPE_FILE &&
      ! fileopFileExists (pq->tempname)) {
    /* no history */
    connSendMessage (playerData->conn, ROUTE_MAIN, MSG_PLAYBACK_FINISH, "0");
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: no file: %s", pq->tempname);
    logProcEnd ("no-file");
    return;
  }

  preq = mdmalloc (sizeof (playrequest_t));
  preq->uniqueidx = uniqueidx;
  preq->songname = mdstrdup (p);
  queuePush (playerData->playRequest, preq);

  logProcEnd ("");
}

static prepqueue_t *
playerLocatePreppedSong (playerdata_t *playerData, int32_t uniqueidx,
    const char *sfname, int externalreq)
{
  prepqueue_t       *pq = NULL;
  bool              found = false;
  int               count = 0;

  logProcBegin ();

  /* do a pre-check to finish off any threads */
  playerCheckPrepThreads (playerData);

#if DEBUG_PREP_QUEUE
  fprintf (stderr, "pq: looking for: %d %s\n", uniqueidx, sfname);
  playerDumpPrepQueue (playerData, "locate");
#endif
  found = false;
  count = 0;
  if (playerData->repeat) {
    pq = playerData->currentSong;
    if (pq != NULL && pq->announce == PREP_SONG &&
        uniqueidx != PL_UNIQUE_ANN && uniqueidx == pq->uniqueidx) {
      logMsg (LOG_DBG, LOG_BASIC, "locate found %" PRId32 " as repeat %s", uniqueidx, sfname);
      found = true;
    }
  }

  /* the prep queue is generally quite short, a brute force search is fine */
  /* the maximum could be potentially ~twenty announcements + five songs */
  /* with no announcements, five songs only */
  while (! found && count < PLAYER_RETRY_COUNT) {
    queueStartIterator (playerData->prepQueue, &playerData->prepiteridx);
    pq = queueIterateData (playerData->prepQueue, &playerData->prepiteridx);
    while (pq != NULL) {
      if (uniqueidx != PL_UNIQUE_ANN && uniqueidx == pq->uniqueidx) {
        logMsg (LOG_DBG, LOG_BASIC, "locate found %" PRId32 " %s (%s)", uniqueidx, sfname, pq->tempname);
        if (count > 0) {
          logMsg (LOG_DBG, LOG_IMPORTANT, "song was not prepped; retry count %d", count);
        }
        found = true;
        break;
      }
      if (uniqueidx == PL_UNIQUE_ANN && uniqueidx == pq->uniqueidx &&
          strcmp (sfname, pq->songname) == 0) {
        logMsg (LOG_DBG, LOG_BASIC, "locate found %" PRId32 " ann %s", uniqueidx, sfname);
        if (count > 0) {
          logMsg (LOG_DBG, LOG_IMPORTANT, "song was not prepped; retry count %d", count);
        }
        found = true;
        break;
      }
      pq = queueIterateData (playerData->prepQueue, &playerData->prepiteridx);
    }
    if (! found) {
      /* this usually happens when a song is prepped and then immediately */
      /* played before it has had time to be prepped */
      /* the test suite does this, */
      /* also playing a song directly in the music manager */
      /* when using a bdj4/bdj4 connection with the download, this can */
      /* take a long time, thus the retry count is set to 200 */
      playerProcessPrepRequest (playerData);
      playerCheckPrepThreads (playerData);
      ++count;
      mssleep (10);
    }
  }

  if (! found && ! externalreq) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: unable to locate song %s", sfname);
    logProcEnd ("not-found");
    return NULL;
  }

  if (pq != NULL) {
    logMsg (LOG_DBG, LOG_BASIC, "  %" PRId32 " %s", pq->uniqueidx, pq->songname);
  }
  logProcEnd ("");
  return pq;
}

/* internal routines - player handling */

static void
playerPause (playerdata_t *playerData)
{
  prepqueue_t *pq = NULL;
  plistate_t  plistate;

  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  logProcBegin ();

  pq = playerData->currentSong;
  if (pq != NULL && pq->announce == PREP_ANNOUNCE) {
    logProcEnd ("play-announce");
    return;
  }

  plistate = pliState (playerData->pli);

  if (playerData->inFadeOut) {
    playerData->pauseAtEnd = true;
  } else if (plistate == PLI_STATE_PLAYING) {
    /* record the playtime played */
    playerData->playTimePlayed += mstimeend (&playerData->playTimeStart);
    pliPause (playerData->pli);
    playerSetPlayerState (playerData, PL_STATE_PAUSED);
    if (playerData->inFadeIn) {
      playerData->inFade = false;
      playerData->inFadeIn = false;
      if (! playerData->mute) {
        volumeSet (playerData->volume, playerData->currentSink, playerData->realVolume);
        logMsg (LOG_DBG, LOG_VOLUME, "play after pause: in-fade: set volume: %d", playerData->realVolume);
        playerData->actualVolume = playerData->realVolume;
      }
    }
  }
  logProcEnd ("");
}

static void
playerPlay (playerdata_t *playerData)
{
  plistate_t plistate = pliState (playerData->pli);

  logProcBegin ();

  if (plistate != PLI_STATE_PLAYING) {
    if (playerData->playerState == PL_STATE_IN_GAP &&
        playerData->inGap) {
      /* cancel the gap */
      mstimestart (&playerData->gapFinishTime);
    }

    pliPlay (playerData->pli);

    if (playerData->playerState == PL_STATE_PAUSED) {
      prepqueue_t       *pq = playerData->currentSong;

      /* all of the check times must be reset */
      /* set the times after restarting the player */
      /* there are some subtleties about when to set the check times */
      playerSetCheckTimes (playerData, pq);
      /* set the state and send the status last */
      playerSetPlayerState (playerData, PL_STATE_PLAYING);
    }
  }

  logProcEnd ("");
}

static void
playerNextSong (playerdata_t *playerData)
{
  logMsg (LOG_DBG, LOG_BASIC, "next song");

  logProcBegin ();

  playerData->repeat = false;
  playerData->priorGap = playerData->gap;
  playerData->gap = 0;

  if (playerData->playerState == PL_STATE_LOADING ||
      playerData->playerState == PL_STATE_PLAYING ||
      playerData->playerState == PL_STATE_IN_GAP ||
      playerData->playerState == PL_STATE_IN_FADEOUT) {
    /* the song will stop playing, and the normal logic will move */
    /* to the next song and continue playing */
    playerData->stopNextsongFlag = STOP_NEXTSONG;
    playerData->stopPlaying = true;
    /* cancel any gap */
    mstimestart (&playerData->gapFinishTime);
  } else {
    if (playerData->playerState == PL_STATE_PAUSED) {
      playrequest_t   *preq;

      /* this song's play request must be removed from the play request q */
      preq = queuePop (playerData->playRequest);
      playerFreePlayRequest (preq);

      /* tell vlc to stop */
      pliStop (playerData->pli);
      logMsg (LOG_DBG, LOG_BASIC, "was paused; next-song");
      playerSetPlayerState (playerData, PL_STATE_STOPPED);

      if (playerData->currentSong != NULL) {
        /* cannot be an announcement, but be safe anyways */
        if (playerData->currentSong->announce != PREP_ANNOUNCE) {
          playerPrepQueueFree (playerData->currentSong);
          playerData->currentSong = NULL;
        }
      }
    } else {
      /* stopped */
      /* the main process will send a clear-prep back to the player */
      playerData->gap = playerData->priorGap;
    }

    /* tell main to go to the next song, no history */
    connSendMessage (playerData->conn, ROUTE_MAIN, MSG_PLAYBACK_FINISH, "0");
  }
  logProcEnd ("");
}

static void
playerPauseAtEnd (playerdata_t *playerData)
{
  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  logProcBegin ();

  playerData->pauseAtEnd = playerData->pauseAtEnd ? false : true;
  playerSendPauseAtEndState (playerData);
  logProcEnd ("");
}

static void
playerSendPauseAtEndState (playerdata_t *playerData)
{
  char    tbuff [20];

  logProcBegin ();

  snprintf (tbuff, sizeof (tbuff), "%d", playerData->pauseAtEnd);
  connSendMessage (playerData->conn, ROUTE_PLAYERUI,
      MSG_PLAY_PAUSEATEND_STATE, tbuff);
  connSendMessage (playerData->conn, ROUTE_MANAGEUI,
      MSG_PLAY_PAUSEATEND_STATE, tbuff);
  logProcEnd ("");
}

static void
playerFade (playerdata_t *playerData)
{
  plistate_t  plistate;
  prepqueue_t *pq = NULL;

  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  logProcBegin ();

  if (playerData->inFadeOut) {
    logProcEnd ("in-fade-out");
    return;
  }

  pq = playerData->currentSong;
  if (pq != NULL && pq->announce == PREP_ANNOUNCE) {
    logProcEnd ("play-announce");
    return;
  }

  if (playerData->fadeoutTime == 0) {
    playerCheckSystemVolume (playerData);
    playerData->stopPlaying = true;
  }

  plistate = pliState (playerData->pli);

  if (playerData->fadeoutTime > 0 &&
      plistate == PLI_STATE_PLAYING) {
    logMsg (LOG_DBG, LOG_BASIC, "fade");
    playerCheckSystemVolume (playerData);
    playerStartFadeOut (playerData);
    mstimeset (&playerData->playTimeCheck, playerData->fadeoutTime - 500);
    mstimeset (&playerData->playEndCheck, playerData->fadeoutTime);
  }
  logProcEnd ("");
}

static void
playerSpeed (playerdata_t *playerData, char *trate)
{
  double rate;

  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  logProcBegin ();

  if (! pliCheckSupport (playerData->pliSupported, PLI_SUPPORT_SPEED)) {
    logProcEnd ("not-supported");
    return;
  }

  if (playerData->playerState == PL_STATE_PLAYING) {
    rate = atof (trate);
    playerChangeSpeed (playerData, (int) rate);
  }
  if (playerData->playerState == PL_STATE_PAUSED ||
      playerData->playerState == PL_STATE_STOPPED) {
    playerData->speedWaitChg = true;
    playerData->newSpeed = (int) atof (trate);
  }

  logProcEnd ("");
}

static void
playerChangeSpeed (playerdata_t *playerData, int speed)
{
  pliRate (playerData->pli, (ssize_t) speed);
  playerData->currentSpeed = (ssize_t) speed;
  playerData->speedWaitChg = false;
}

static void
playerSeek (playerdata_t *playerData, ssize_t reqpos)
{
  ssize_t       seekpos;
  prepqueue_t   *pq = playerData->currentSong;

  if (pq == NULL) {
    return;
  }

  logProcBegin ();

  if (! pliCheckSupport (playerData->pliSupported, PLI_SUPPORT_SEEK)) {
    logProcEnd ("not-supported");
    return;
  }

  /* the requested position is adjusted for the speed, as the position */
  /* slider is based on the speed adjusted duration. */
  /* need to change it back to something the player will understand */
  /* songstart is already normalized */
  seekpos = reqpos;
  seekpos = songutilNormalizePosition (seekpos, pq->speed);
  seekpos += pq->songstart;
  pliSeek (playerData->pli, seekpos);
  playerData->playTimePlayed = reqpos;
  playerSetCheckTimes (playerData, pq);
  logProcEnd ("");
}

static void
playerStop (playerdata_t *playerData)
{
  plistate_t plistate = pliState (playerData->pli);

  logProcBegin ();

  if (plistate == PLI_STATE_PLAYING ||
      plistate == PLI_STATE_PAUSED) {
    pliStop (playerData->pli);
  }
  logProcEnd ("");
}

static void
playerSongBegin (playerdata_t *playerData)
{
  prepqueue_t   *pq = NULL;

  if (playerData->playerState != PL_STATE_PLAYING &&
      playerData->playerState != PL_STATE_PAUSED) {
    return;
  }

  pq = playerData->currentSong;
  if (pq != NULL && pq->announce == PREP_ANNOUNCE) {
    return;
  }

  playerSeek (playerData, 0);
  /* there is a change in position */
  playerSendStatus (playerData, STATUS_FORCE);
}

static void
playerVolumeSet (playerdata_t *playerData, char *tvol)
{
  int       newvol;
  int       voldiff;


  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  logProcBegin ();

  newvol = (int) atol (tvol);
  newvol = playerLimitVolume (newvol);
  voldiff = newvol - playerData->currentVolume;
  playerData->realVolume += voldiff;
  playerData->realVolume = playerLimitVolume (playerData->realVolume);
  playerData->currentVolume += voldiff;
  playerData->currentVolume = playerLimitVolume (playerData->currentVolume);
  playerCheckVolumeSink (playerData);
  volumeSet (playerData->volume, playerData->currentSink, playerData->realVolume);
  logMsg (LOG_DBG, LOG_VOLUME, "volume-set: %d", playerData->realVolume);
  playerData->actualVolume = playerData->realVolume;
  logProcEnd ("");
}

static void
playerCheckVolumeSink (playerdata_t *playerData)
{
  if (*playerData->currentSink == '\0') {
    if (volumeCheckSink (playerData->volume, playerData->currentSink)) {
      int   currvol;
      int   realvol;
      int   plidevtype;

      /* preserve the volume */
      currvol = playerData->currentVolume;
      realvol = playerData->realVolume;

      /* clean up the prior audio sink */
      playerResetVolume (playerData);

      playerInitSinkList (playerData);
      plidevtype = playerSetAudioSink (playerData, playerData->actualSink);
      pliSetAudioDevice (playerData->pli, playerData->actualSink, plidevtype);

      playerSetDefaultVolume (playerData);
      playerData->currentVolume = currvol;
      playerData->realVolume = realvol;
      /* set the volume on the newly selected audio sink to the same volume */
      volumeSet (playerData->volume, playerData->currentSink, playerData->realVolume);
    }
  }
}

static void
playerVolumeMute (playerdata_t *playerData)
{
  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  logProcBegin ();

  playerData->mute = playerData->mute ? false : true;
  if (playerData->mute) {
    volumeSet (playerData->volume, playerData->currentSink, 0);
    playerData->actualVolume = 0;
  } else {
    volumeSet (playerData->volume, playerData->currentSink, playerData->realVolume);
    playerData->actualVolume = playerData->realVolume;
  }
  logProcEnd ("");
}

static void
playerPrepQueueFree (void *data)
{
  prepqueue_t     *pq = data;

  logProcBegin ();

  if (pq == NULL) {
    return;
  }

  logMsg (LOG_DBG, LOG_INFO, "prep-free: %" PRId32 " %s", pq->uniqueidx, pq->songname);

  if (pq->ident != PREP_QUEUE_IDENT) {
    logMsg (LOG_DBG, LOG_ERR, "ERR: double free of prep queue");
    fprintf (stderr, "ERR: double free of prep queue\n");
    return;
  }

  audiosrcPrepClean (pq->songname, pq->tempname);
  dataFree (pq->songname);
  *pq->tempname = '\0';
  mdfree (pq);

  logProcEnd ("");
}

static void
playerSigHandler (int sig)
{
  gKillReceived = 1;
}

static int
playerSetAudioSink (playerdata_t *playerData, const char *sinkname)
{
  bool        found = false;
  int         idx = -1;
  bool        isdefault = false;
  const char  *confsink;
  int         rc;

  confsink = bdjoptGetStr (OPT_MP_AUDIOSINK);
  if (strcmp (confsink, VOL_DEFAULT_NAME) == 0) {
    isdefault = true;
  }

  logProcBegin ();
  if (sinkname != NULL) {
    /* the sink list is not ordered */
    for (int i = 0; i < playerData->sinklist.count; ++i) {
      if (strcmp (sinkname, playerData->sinklist.sinklist [i].name) == 0) {
        found = true;
        idx = (int) i;
        break;
      }
    }
  }

  if (found && idx >= 0 && ! isdefault) {
    playerData->currentSink = playerData->sinklist.sinklist [idx].name;
    playerData->actualSink = playerData->currentSink;
    rc = PLI_SELECTED_DEV;
    logMsg (LOG_DBG, LOG_IMPORTANT, "audio sink set to %s", playerData->sinklist.sinklist [idx].description);
  } else {
    playerData->currentSink = "";
    playerData->actualSink = playerData->sinklist.defname;
    rc = PLI_DEFAULT_DEV;
    logMsg (LOG_DBG, LOG_IMPORTANT, "audio sink set to default %s", playerData->sinklist.defname);
  }

  playerSetAudioSinkEnv (playerData, isdefault);

  logProcEnd ("");
  return rc;
}

static void
playerInitSinkList (playerdata_t *playerData)
{
  int count;

  logProcBegin ();

  if (playerData == NULL) {
    return;
  }

  volumeFreeSinkList (&playerData->sinklist);
  volumeInitSinkList (&playerData->sinklist);

  if (volumeHaveSinkList (playerData->volume)) {
    count = 0;
    while (volumeGetSinkList (playerData->volume, "", &playerData->sinklist) != 0 &&
        count < 20) {
      mssleep (100);
      ++count;
    }
  }

  if (playerData->sinklist.defname != NULL &&
      *playerData->sinklist.defname) {
    playerData->actualSink = playerData->sinklist.defname;
  } else {
    playerData->actualSink = VOL_DEFAULT_NAME;
  }

  if (playerData->sinklist.sinklist != NULL) {
    logMsg (LOG_DBG, LOG_BASIC, "vol-sinklist");
    for (int i = 0; i < playerData->sinklist.count; ++i) {
      logMsg (LOG_DBG, LOG_BASIC, "  %d %3d %s %s",
               playerData->sinklist.sinklist [i].defaultFlag,
               playerData->sinklist.sinklist [i].idxNumber,
               playerData->sinklist.sinklist [i].name,
               playerData->sinklist.sinklist [i].description);
    }
  }

  logProcEnd ("");
}

static void
playerFadeVolSet (playerdata_t *playerData)
{
  double  findex;
  int     newvol;
  int     ts;
  int     fadeType;

  logProcBegin ();

  fadeType = playerData->fadeType;
  if (playerData->inFadeIn) {
    fadeType = FADETYPE_TRIANGLE;
  }
  findex = calcFadeIndex (playerData, fadeType);

  newvol = (int) round ((double) playerData->realVolume * findex);

  if (newvol > playerData->realVolume) {
    newvol = playerData->realVolume;
  }
  if (! playerData->mute) {
    volumeSet (playerData->volume, playerData->currentSink, newvol);
    playerData->actualVolume = newvol;
  }
  if (playerData->inFade) {
    logMsg (LOG_DBG, LOG_VOLUME, "fade set volume: %d count:%d",
        newvol, playerData->fadeCount);
  }
  if (playerData->inFadeOut) {
    logMsg (LOG_DBG, LOG_VOLUME, "   time %" PRId64,
        (int64_t) mstimeend (&playerData->playEndCheck));
  }
  if (playerData->inFadeIn) {
    ++playerData->fadeCount;
  }
  if (playerData->inFadeOut) {
    --playerData->fadeCount;
    if (playerData->fadeCount <= 0) {
      /* leave inFade set to prevent race conditions in the main loop */
      /* the player stop condition will reset the inFade flag */
      playerData->inFadeOut = false;
      volumeSet (playerData->volume, playerData->currentSink, 0);
      playerData->actualVolume = 0;
      logMsg (LOG_DBG, LOG_VOLUME, "fade-out done volume: %d time: %" PRId64,
          0, (int64_t) mstimeend (&playerData->playEndCheck));
    }
  }

  ts = playerData->inFadeOut ? FADEOUT_TIMESLICE : FADEIN_TIMESLICE;
  /* incrementing fade-time start by the timeslice each interval will */
  /* give us the next end-time */
  playerData->fadeTimeStart += ts;
  mstimesettm (&playerData->fadeTimeNext, playerData->fadeTimeStart);

  if (playerData->inFadeIn &&
      newvol >= playerData->realVolume) {
    playerData->inFade = false;
    playerData->inFadeIn = false;
  }
  logProcEnd ("");
}

static double
calcFadeIndex (playerdata_t *playerData, int fadeType)
{
  double findex = 0.0;
  double index = (double) playerData->fadeCount;
  double range = (double) playerData->fadeSamples;

  logProcBegin ();

  findex = fmax(0.0, fmin (1.0, index / range));

  switch (fadeType) {
    case FADETYPE_EXPONENTIAL_SINE: {
      findex = 1.0 - cos (M_PI / 4.0 * (pow (2.0 * findex - 1, 3) + 1));
      break;
    }
    case FADETYPE_HALF_SINE: {
      findex = 1.0 - cos (findex * M_PI) / 2.0;
      break;
    }
    case FADETYPE_INVERTED_PARABOLA: {
      findex = 1.0 - (1.0 - findex) * (1.0 - findex);
      break;
    }
    case FADETYPE_QUADRATIC: {
      findex = findex * findex;
      break;
    }
    case FADETYPE_QUARTER_SINE: {
      findex = sin (findex * M_PI / 2.0);
      break;
    }
    case FADETYPE_TRIANGLE: {
      break;
    }
  }
  logProcEnd ("");
  return findex;
}

static void
playerStartFadeOut (playerdata_t *playerData)
{
  ssize_t       tm;
  prepqueue_t   *pq = playerData->currentSong;

  logProcBegin ();
  playerData->inFade = true;
  playerData->inFadeOut = true;
  tm = pq->dur - playerCalcPlayedTime (playerData);
  tm = tm < playerData->fadeoutTime ? tm : playerData->fadeoutTime;
  playerData->fadeSamples = tm / FADEOUT_TIMESLICE;
  playerData->fadeCount = playerData->fadeSamples;
  logMsg (LOG_DBG, LOG_VOLUME, "fade: samples: %d", playerData->fadeCount);
  logMsg (LOG_DBG, LOG_VOLUME, "fade: timeslice: %d", FADEOUT_TIMESLICE);
  playerData->fadeTimeStart = mstime ();
  playerFadeVolSet (playerData);
  playerSetPlayerState (playerData, PL_STATE_IN_FADEOUT);
  logProcEnd ("");
}

static void
playerSetCheckTimes (playerdata_t *playerData, prepqueue_t *pq)
{
  int32_t     newdur;

  logProcBegin ();

  /* pq->dur is adjusted for speed.  */
  /* plitm is not; it cannot be combined with pq->dur */
  newdur = pq->dur - playerData->playTimePlayed;
  /* want to start check for real finish 500 ms before end */
  mstimestart (&playerData->playTimeStart);
  mstimeset (&playerData->playEndCheck, newdur);
  mstimeset (&playerData->playTimeCheck, newdur - 500);
  mstimeset (&playerData->fadeTimeCheck, newdur);
  mstimeset (&playerData->volumeTimeCheck, 1000);
  if (pq->announce == PREP_SONG && playerData->fadeoutTime > 0) {
    mstimeset (&playerData->fadeTimeCheck, newdur - playerData->fadeoutTime);
  }
  logMsg (LOG_DBG, LOG_INFO, "pq->dur: %" PRId32, pq->dur);
  logMsg (LOG_DBG, LOG_INFO, "newdur: %" PRId32, newdur);
  logMsg (LOG_DBG, LOG_INFO, "playTimeStart: %" PRId64, (int64_t) mstimeend (&playerData->playTimeStart));
  logMsg (LOG_DBG, LOG_INFO, "playEndCheck: %" PRId64, (int64_t) mstimeend (&playerData->playEndCheck));
  logMsg (LOG_DBG, LOG_INFO, "playTimeCheck: %" PRId64, (int64_t) mstimeend (&playerData->playTimeCheck));
  logMsg (LOG_DBG, LOG_INFO, "fadeTimeCheck: %" PRId64, (int64_t) mstimeend (&playerData->fadeTimeCheck));
  logProcEnd ("");
}

static void
playerSetPlayerState (playerdata_t *playerData, playerstate_t pstate)
{
  char    tbuff [20];

  logProcBegin ();

  if (playerData->playerState != pstate) {
    playerData->playerState = pstate;
    logMsg (LOG_DBG, LOG_BASIC, "pl-state: %d/%s",
        playerData->playerState, logPlayerState (playerData->playerState));
    msgbuildPlayerState (tbuff, sizeof (tbuff),
        playerData->playerState, playerData->newsong);
    connSendMessage (playerData->conn, ROUTE_MAIN, MSG_PLAYER_STATE, tbuff);
    connSendMessage (playerData->conn, ROUTE_PLAYERUI, MSG_PLAYER_STATE, tbuff);
    connSendMessage (playerData->conn, ROUTE_MANAGEUI, MSG_PLAYER_STATE, tbuff);

    if (playerData->speedWaitChg &&
        playerData->playerState == PL_STATE_PLAYING) {
      playerChangeSpeed (playerData, playerData->newSpeed);
    }

    /* any time there is a change of player state, send the status */
    playerSendStatus (playerData, STATUS_NO_FORCE);
    /* reset the new-song flag after it has been sent */
    playerData->newsong = false;
  }
  logProcEnd ("");
}


static void
playerSendStatus (playerdata_t *playerData, bool forceFlag)
{
  char        *rbuff;
  prepqueue_t *pq = playerData->currentSong;
  int32_t     tm;
  int32_t     dur;

  logProcBegin ();

  if (forceFlag == STATUS_NO_FORCE &&
      playerData->playerState == playerData->lastPlayerState &&
      playerData->playerState != PL_STATE_PLAYING &&
      playerData->playerState != PL_STATE_IN_FADEOUT) {
    logProcEnd ("no-state-chg");
    return;
  }

  playerData->lastPlayerState = playerData->playerState;

  rbuff = mdmalloc (BDJMSG_MAX);
  rbuff [0] = '\0';

  dur = 0;
  if (pq != NULL) {
    dur = pq->dur;
  }

  if (! progstateIsRunning (playerData->progstate)) {
    dur = 0;
  } else {
    switch (playerData->playerState) {
      case PL_STATE_UNKNOWN:
      case PL_STATE_STOPPED:
      case PL_STATE_IN_GAP: {
        dur = 0;
        break;
      }
      default: {
        break;
      }
    }
  }

  tm = playerCalcPlayedTime (playerData);

  msgbuildPlayerStatus (rbuff, BDJMSG_MAX,
      playerData->repeat, playerData->pauseAtEnd, playerData->currentVolume,
      playerData->currentSpeed, playerData->baseVolume, tm, dur);

  /* 4.4.4 send the playerui and manageui the messages from here, */
  /* avoid some latency by not routing through main */
  connSendMessage (playerData->conn, ROUTE_PLAYERUI,
      MSG_PLAYER_STATUS_DATA, rbuff);
  connSendMessage (playerData->conn, ROUTE_MANAGEUI,
      MSG_PLAYER_STATUS_DATA, rbuff);

  /* main will parse the message and pass on the timer information to */
  /* the marquee.  main also reformats the message into json for */
  /* the remote control. */
  connSendMessage (playerData->conn, ROUTE_MAIN,
      MSG_PLAYER_STATUS_DATA, rbuff);

  dataFree (rbuff);

  /* four times a second... */
  mstimeset (&playerData->statusCheck, 250);
  logProcEnd ("");
}

static int
playerLimitVolume (int vol)
{
  if (vol > 100) {
    vol = 100;
  }
  if (vol < 0) {
    vol = 0;
  }
  return vol;
}

static ssize_t
playerCalcPlayedTime (playerdata_t *playerData)
{
  ssize_t   tm;

  tm = 0;
  if (playerData->playerState == PL_STATE_PAUSED) {
    tm = playerData->playTimePlayed;
  } else if (playerData->playerState == PL_STATE_PLAYING ||
      playerData->playerState == PL_STATE_IN_FADEOUT) {
    tm = playerData->playTimePlayed + mstimeend (&playerData->playTimeStart);
  } else {
    tm = 0;
  }

  return tm;
}

static void
playerSetDefaultVolume (playerdata_t *playerData)
{
  int   count;
  bool  bdj3flag;

  volregCreateBDJ4Flag ();
  bdj3flag = volregCheckBDJ3Flag ();

  playerData->originalSystemVolume =
      volumeGet (playerData->volume, playerData->currentSink);
  playerData->originalSystemVolume = playerLimitVolume (playerData->originalSystemVolume);
  logMsg (LOG_DBG, LOG_INFO, "Original system volume: %d", playerData->originalSystemVolume);

  count = volregSave (playerData->actualSink, playerData->originalSystemVolume);
  if (count > 1 || bdj3flag) {
    playerData->currentVolume = playerData->originalSystemVolume;
    logMsg (LOG_DBG, LOG_INFO, "volume set-default: use orig");
  } else {
    playerData->currentVolume = (int) bdjoptGetNum (OPT_P_DEFAULTVOLUME);
    logMsg (LOG_DBG, LOG_INFO, "volume set-default: use dflt");
  }
  playerData->baseVolume = playerData->currentVolume;
  playerData->realVolume = playerData->currentVolume;

  volumeSet (playerData->volume, playerData->currentSink, playerData->realVolume);
  playerData->actualVolume = playerData->realVolume;
  logMsg (LOG_DBG, LOG_INFO, "set-default volume: %d", playerData->realVolume);
}

static void
playerFreePlayRequest (void *tpreq)
{
  playrequest_t *preq = tpreq;

  if (preq != NULL) {
    dataFree (preq->songname);
    mdfree (preq);
  }
}

static void
playerChkPlayerStatus (playerdata_t *playerData, int routefrom)
{
  char  tmp [2000];

  snprintf (tmp, sizeof (tmp),
      "playstate%c%s%c"
      "plistate%c%s%c"
      "currvolume%c%d%c"
      "realvolume%c%d%c"
      "actualvolume%c%d%c"
      "plivolume%c%d%c"
      "speed%c%d%c"
      "playtimeplayed%c%" PRIu64 "%c"
      "pauseatend%c%d%c"
      "repeat%c%d%c"
      "prepqueuecount%c%" PRId32 "%c"
      "currentsink%c%s",
      MSG_ARGS_RS, logPlayerState (playerData->playerState), MSG_ARGS_RS,
      MSG_ARGS_RS, pliStateText (playerData->pli), MSG_ARGS_RS,
      MSG_ARGS_RS, playerData->currentVolume, MSG_ARGS_RS,
      MSG_ARGS_RS, playerData->realVolume, MSG_ARGS_RS,
      MSG_ARGS_RS, playerData->actualVolume, MSG_ARGS_RS,
      MSG_ARGS_RS, pliGetVolume (playerData->pli), MSG_ARGS_RS,
      MSG_ARGS_RS, playerData->currentSpeed, MSG_ARGS_RS,
      MSG_ARGS_RS, (uint64_t) playerCalcPlayedTime (playerData), MSG_ARGS_RS,
      MSG_ARGS_RS, playerData->pauseAtEnd, MSG_ARGS_RS,
      MSG_ARGS_RS, playerData->repeat, MSG_ARGS_RS,
      MSG_ARGS_RS, queueGetCount (playerData->prepQueue), MSG_ARGS_RS,
      /* current sink can be empty, just put it last for now */
      MSG_ARGS_RS, playerData->currentSink);
  connSendMessage (playerData->conn, routefrom, MSG_CHK_PLAYER_STATUS, tmp);
}

static void
playerChkPlayerSong (playerdata_t *playerData, int routefrom)
{
  prepqueue_t *pq = playerData->currentSong;
  char        tmp [2000];
  const char  *sn = MSG_ARGS_EMPTY_STR;
  int64_t     dur;


  dur = 0;
  if (pq != NULL &&
      (playerData->playerState == PL_STATE_LOADING ||
      playerData->playerState == PL_STATE_PLAYING ||
      playerData->playerState == PL_STATE_IN_FADEOUT)) {
    dur = pq->dur;
    sn = pq->songname;
  }

  snprintf (tmp, sizeof (tmp),
      "p-duration%c%" PRId64 "%c"
      "p-songfn%c%s",
      MSG_ARGS_RS, dur, MSG_ARGS_RS,
      MSG_ARGS_RS, sn);
  connSendMessage (playerData->conn, routefrom, MSG_CHK_PLAYER_SONG, tmp);
}

static void
playerResetVolume (playerdata_t *playerData)
{
  int   origvol;
  int   bdj3flag;

  origvol = volregClear (playerData->actualSink);
  bdj3flag = volregCheckBDJ3Flag ();
  if (origvol > 0) {
    if (! bdj3flag) {
      volumeSet (playerData->volume, playerData->actualSink, origvol);
      playerData->actualVolume = origvol;
      logMsg (LOG_DBG, LOG_INFO, "set to orig volume: (was:%d) %d", playerData->originalSystemVolume, origvol);
    }
  }
}

static void
playerSetAudioSinkEnv (playerdata_t *playerData, bool isdefault)
{
  /* this is needed for pulse audio, */
  /* otherwise vlc always chooses the default, */
  /* despite having the audio sink set. */
  if (isLinux () &&
      strcmp (bdjoptGetStr (OPT_M_VOLUME_INTFC), "libvolpa") == 0) {
    /* this works for pulse-audio and pipewire */
    osSetEnv ("PULSE_SINK", playerData->actualSink);
  }
}

static const char *
playerGetAudioInterface (void)
{
  const char  *currplintfc;
  bool        currvlc3 = false;
  bool        currvlc4 = false;
  bool        havevlc3 = false;
  bool        havevlc4 = false;
  ilist_t     *interfaces;
  ilistidx_t  iter;
  ilistidx_t  key;
  ilistidx_t  vlc3key = -1;
  ilistidx_t  vlc4key = -1;
  const char  *intfc;
  const char  *newnm = NULL;
  bool        chg = false;

  currplintfc = bdjoptGetStr (OPT_M_PLAYER_INTFC);
  if (currplintfc == NULL || strstr (currplintfc, "vlc") == NULL) {
    return currplintfc;
  }

  /* 4.10.3 if the configuration is set to use a VLC player, */
  /* use the player interface list to check and see which */
  /* versions of VLC are available */
  /* if version 3 is configured, and version 4 is available, switch */
  /* if version 4 is configured, and version 3 is available, switch */

  if (strcmp (currplintfc, "libplivlc") == 0) {
    currvlc3 = true;
  }
  if (strcmp (currplintfc, "libplivlc4") == 0) {
    currvlc4 = true;
  }

  interfaces = pliInterfaceList ();
  ilistStartIterator (interfaces, &iter);
  while ((key = ilistIterateKey (interfaces, &iter)) >= 0) {
    intfc = ilistGetStr (interfaces, key, DYI_LIB);
    if (intfc != NULL && strcmp (intfc, "libplivlc") == 0) {
      havevlc3 = true;
      vlc3key = key;
    }
    if (intfc != NULL && strcmp (intfc, "libplivlc4") == 0) {
      havevlc4 = true;
      vlc4key = key;
    }
  }

  if (currvlc3 && ! havevlc3 && havevlc4) {
    intfc = ilistGetStr (interfaces, vlc4key, DYI_LIB);
    newnm = ilistGetStr (interfaces, vlc4key, DYI_DESC);
    chg = true;
  }
  if (currvlc4 && ! havevlc4 && havevlc3) {
    intfc = ilistGetStr (interfaces, vlc3key, DYI_LIB);
    newnm = ilistGetStr (interfaces, vlc3key, DYI_DESC);
    chg = true;
  }

  if (chg) {
    bdjoptSetStr (OPT_M_PLAYER_INTFC, intfc);
    bdjoptSetStr (OPT_M_PLAYER_INTFC_NM, newnm);
    bdjoptSave ();
  }

  ilistFree (interfaces);

  return bdjoptGetStr (OPT_M_PLAYER_INTFC);
}

static void
playerCheckPrepThreads (playerdata_t *playerData)
{
#if _lib_pthread_create && PLAYER_USE_THREADS
  if (playerData->threadcount > 0) {
    bool  procflag = false;

    for (int i = 0; i < playerData->threadcount; ++i) {
      prepthread_t  *prepthread;
      prepqueue_t   *npq;

      prepthread = playerData->prepthread [i];
      if (prepthread == NULL) {
        continue;
      }

      if (prepthread->finished == false) {
        continue;
      }

      npq = prepthread->npq;

      if (prepthread->rc) {
        queuePush (playerData->prepQueue, npq);
        logMsg (LOG_DBG, LOG_INFO, "prep-do: %" PRId32 " %s r:%d p:%" PRId32, npq->uniqueidx, npq->songname, queueGetCount (playerData->prepRequestQueue), queueGetCount (playerData->prepQueue));
        logMsg (LOG_DBG, LOG_INFO, "prep-tempname: %s", npq->tempname);
      } else {
        logMsg (LOG_DBG, LOG_IMPORTANT, "unable-to-prep %s", npq->songname);
      }

      mdfree (prepthread);
      playerData->prepthread [i] = NULL;
      procflag = true;
    }
    if (procflag) {
      playerData->threadcount = 0;
      for (int i = PLAYER_MAX_PREP - 1; i >= 0; --i) {
        if (playerData->prepthread [i] != NULL) {
          playerData->threadcount = i + 1;
          break;
        }
      }
    }
  }
#endif
}

#if DEBUG_PREP_QUEUE
static void
playerDumpPrepQueue (playerdata_t *playerData, const char *tag)  /* TESTING */
{
  prepqueue_t       *pq = NULL;
  int               count;

  count = 0;
  queueStartIterator (playerData->prepQueue, &playerData->prepiteridx);
  while ((pq = queueIterateData (playerData->prepQueue, &playerData->prepiteridx)) != NULL) {
    fprintf (stderr, "  pq: /%s/ %d %d %s\n", tag, count, pq->uniqueidx, pq->songname);
    ++count;
  }
}
#endif

