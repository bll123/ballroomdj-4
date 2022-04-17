/*
 * bdj4player
 *  Does the actual playback of the music.
 *  Handles volume changes, fades.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <signal.h>

#include "bdj4.h"
#include "bdj4init.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "conn.h"
#include "osutils.h"
#include "pathbld.h"
#include "progstate.h"
#include "fileop.h"
#include "filemanip.h"
#include "lock.h"
#include "log.h"
#include "pathutil.h"
#include "pli.h"
#include "procutil.h"
#include "queue.h"
#include "sock.h"
#include "sockh.h"
#include "songutil.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"
#include "volume.h"

typedef struct {
  char          *songfullpath;
  char          *songname;
  char          *tempname;
  ssize_t       dur;
  ssize_t       songstart;
  ssize_t       speed;
  double        voladjperc;
  ssize_t       gap;
  ssize_t       announce;
} prepqueue_t;

typedef struct {
  conn_t          *conn;
  progstate_t     *progstate;
  char            *locknm;
  long            globalCount;
  pli_t           *pli;
  volume_t        *volume;
  queue_t         *prepRequestQueue;
  queue_t         *prepQueue;
  ssize_t         prepiteridx;
  prepqueue_t     *currentSong;
  queue_t         *playRequest;
  int             originalSystemVolume;
  int             realVolume;
  int             currentVolume;  // real volume + any adjustments
  ssize_t         currentSpeed;
  char            *defaultSink;
  char            *currentSink;
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
  ssize_t         gap;
  mstime_t        gapFinishTime;
  ssize_t         fadeType;
  ssize_t         fadeinTime;
  ssize_t         fadeoutTime;
  long            fadeCount;
  long            fadeSamples;
  mstime_t        fadeTimeNext;
  bool            inFade : 1;
  bool            inFadeIn : 1;
  bool            inFadeOut : 1;
  bool            inGap : 1;
  bool            mute : 1;
  bool            pauseAtEnd : 1;
  bool            repeat : 1;
  bool            stopPlaying : 1;
} playerdata_t;

#define FADEIN_TIMESLICE      100
#define FADEOUT_TIMESLICE     250


static void     playerCheckSystemVolume (playerdata_t *playerData);
static int      playerProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static int      playerProcessing (void *udata);
static bool     playerConnectingCallback (void *tpdata, programstate_t programState);
static bool     playerHandshakeCallback (void *tpdata, programstate_t programState);
static bool     playerStoppingCallback (void *tpdata, programstate_t programState);
static bool     playerClosingCallback (void *tpdata, programstate_t programState);
static void     playerSongPrep (playerdata_t *playerData, char *sfname);
void            playerProcessPrepRequest (playerdata_t *playerData);
static void     playerSongPlay (playerdata_t *playerData, char *sfname);
static prepqueue_t * playerLocatePreppedSong (playerdata_t *playerData, char *sfname);
static void     songMakeTempName (playerdata_t *playerData,
                    char *in, char *out, size_t maxlen);
static void     playerPause (playerdata_t *playerData);
static void     playerPlay (playerdata_t *playerData);
static void     playerNextSong (playerdata_t *playerData);
static void     playerPauseAtEnd (playerdata_t *playerData);
static void     playerSendPauseAtEndState (playerdata_t *playerData);
static void     playerFade (playerdata_t *playerData);
static void     playerSpeed (playerdata_t *playerData, char *trate);
static void     playerSeek (playerdata_t *playerData, ssize_t pos);
static void     playerStop (playerdata_t *playerData);
static void     playerVolumeSet (playerdata_t *playerData, char *tvol);
static void     playerVolumeMute (playerdata_t *playerData);
static void     playerPrepQueueFree (void *);
static void     playerSigHandler (int sig);
static void     playerSetAudioSink (playerdata_t *playerData, char *sinkname);
static void     playerInitSinklist (playerdata_t *playerData);
static void     playerFadeVolSet (playerdata_t *playerData);
static double   calcFadeIndex (playerdata_t *playerData);
static void     playerStartFadeOut (playerdata_t *playerData);
static void     playerSetCheckTimes (playerdata_t *playerData, prepqueue_t *pq);
static void     playerSetPlayerState (playerdata_t *playerData, playerstate_t pstate);
static void     playerSendStatus (playerdata_t *playerData);

static int      gKillReceived = 0;

int
main (int argc, char *argv[])
{
  playerdata_t    playerData;
  uint16_t        listenPort;
  int             flags;

#if _define_SIGHUP
  procutilCatchSignal (playerSigHandler, SIGHUP);
#endif
  procutilCatchSignal (playerSigHandler, SIGINT);
  procutilDefaultSignal (SIGTERM);
#if _define_SIGCHLD
  procutilDefaultSignal (SIGCHLD);
#endif

  playerData.currentSong = NULL;
  playerData.fadeCount = 0;
  playerData.fadeSamples = 1;
  playerData.globalCount = 1;
  playerData.playerState = PL_STATE_STOPPED;
  playerData.lastPlayerState = PL_STATE_UNKNOWN;
  mstimestart (&playerData.playTimeStart);
  playerData.playTimePlayed = 0;
  playerData.playRequest = queueAlloc (free);
  mstimeset (&playerData.statusCheck, 0);
  playerData.pli = NULL;
  playerData.prepQueue = queueAlloc (playerPrepQueueFree);
  playerData.prepRequestQueue = queueAlloc (playerPrepQueueFree);
  playerData.progstate = progstateInit ("player");
  playerData.inFade = false;
  playerData.inFadeIn = false;
  playerData.inFadeOut = false;
  playerData.inGap = false;
  playerData.mute = false;
  playerData.pauseAtEnd = false;
  playerData.repeat = false;
  playerData.stopPlaying = false;

  progstateSetCallback (playerData.progstate, STATE_CONNECTING,
      playerConnectingCallback, &playerData);
  progstateSetCallback (playerData.progstate, STATE_WAIT_HANDSHAKE,
      playerHandshakeCallback, &playerData);
  progstateSetCallback (playerData.progstate, STATE_STOPPING,
      playerStoppingCallback, &playerData);
  progstateSetCallback (playerData.progstate, STATE_CLOSING,
      playerClosingCallback, &playerData);

  flags = BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  bdj4startup (argc, argv, "p", ROUTE_PLAYER, flags);

  playerData.conn = connInit (ROUTE_PLAYER);

  playerData.fadeType = bdjoptGetNum (OPT_P_FADETYPE);
  playerData.fadeinTime = bdjoptGetNum (OPT_P_FADEINTIME);
  playerData.fadeoutTime = bdjoptGetNum (OPT_P_FADEOUTTIME);

  volumeSinklistInit (&playerData.sinklist);
  playerData.defaultSink = "";
  playerData.currentSink = "";
  playerData.currentSpeed = 100;

  playerData.volume = volumeInit ();
  assert (playerData.volume != NULL);

  playerInitSinklist (&playerData);
  /* sets the current sink */
  playerSetAudioSink (&playerData, bdjoptGetStr (OPT_M_AUDIOSINK));

  playerData.originalSystemVolume =
      volumeGet (playerData.volume, playerData.currentSink);
  logMsg (LOG_DBG, LOG_MAIN, "Original system volume: %d", playerData.originalSystemVolume);
  playerData.realVolume = (int) bdjoptGetNum (OPT_P_DEFAULTVOLUME);
  playerData.currentVolume = playerData.realVolume;
  volumeSet (playerData.volume, playerData.currentSink, playerData.currentVolume);
  logMsg (LOG_DBG, LOG_MAIN, "set volume: %d", playerData.currentVolume);

  if (playerData.sinklist.sinklist != NULL) {
    for (size_t i = 0; i < playerData.sinklist.count; ++i) {
      logMsg (LOG_DBG, LOG_BASIC, "%d %3d %s %s",
               playerData.sinklist.sinklist [i].defaultFlag,
               playerData.sinklist.sinklist [i].idxNumber,
               playerData.sinklist.sinklist [i].name,
               playerData.sinklist.sinklist [i].description);
    }
  }

  /* this works for pulse audio */
  if (isLinux () &&
      strcmp (bdjoptGetStr (OPT_M_VOLUME_INTFC), "libvolpa") == 0) {
    osSetEnv ("PULSE_SINK", playerData.currentSink);
  }

  playerData.pli = pliInit (bdjoptGetStr (OPT_M_VOLUME_INTFC),
      playerData.currentSink);

  listenPort = bdjvarsGetNum (BDJVL_PLAYER_PORT);
  sockhMainLoop (listenPort, playerProcessMsg, playerProcessing, &playerData);

  while (progstateShutdownProcess (playerData.progstate) != STATE_CLOSED) {
    ;
  }
  progstateFree (playerData.progstate);
  logEnd ();
  return 0;
}

/* internal routines */

static bool
playerStoppingCallback (void *tpdata, programstate_t programState)
{
  playerdata_t    *playerData = tpdata;

  connDisconnectAll (playerData->conn);
  return true;
}

static bool
playerClosingCallback (void *tpdata, programstate_t programState)
{
  playerdata_t    *playerData = tpdata;

  bdj4shutdown (ROUTE_PLAYER);

  if (playerData->pli != NULL) {
    pliStop (playerData->pli);
    pliClose (playerData->pli);
    pliFree (playerData->pli);
  }

  volumeSet (playerData->volume, playerData->currentSink, playerData->originalSystemVolume);
  logMsg (LOG_DBG, LOG_MAIN, "set to orig volume: %d", playerData->originalSystemVolume);
  playerData->defaultSink = "";
  playerData->currentSink = "";
  volumeFreeSinkList (&playerData->sinklist);
  volumeFree (playerData->volume);

  if (playerData->prepQueue != NULL) {
    queueFree (playerData->prepQueue);
  }
  if (playerData->prepRequestQueue != NULL) {
    queueFree (playerData->prepRequestQueue);
  }
  if (playerData->playRequest != NULL) {
    queueFree (playerData->playRequest);
  }

  connFree (playerData->conn);

  return true;
}

static int
playerProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  playerdata_t      *playerData;

  logProcBegin (LOG_PROC, "playerProcessMsg");
  playerData = (playerdata_t *) udata;

  logMsg (LOG_DBG, LOG_MSGS, "got: from:%ld/%s route:%ld/%s msg:%ld/%s args:%s",
      routefrom, msgRouteDebugText (routefrom),
      route, msgRouteDebugText (route), msg, msgDebugText (msg), args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_PLAYER: {
      logMsg (LOG_DBG, LOG_MSGS, "got: route-player");
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (playerData->conn, routefrom);
          connConnectResponse (playerData->conn, routefrom);
          break;
        }
        case MSG_SOCKET_CLOSE: {
          connDisconnect (playerData->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          logMsg (LOG_DBG, LOG_MSGS, "got: req-exit");
          progstateShutdownProcess (playerData->progstate);
          logProcEnd (LOG_PROC, "playerProcessMsg", "req-exit");
          return 1;
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
          logMsg (LOG_DBG, LOG_MSGS, "got: repeat", args);
          playerData->repeat = playerData->repeat ? false : true;
          break;
        }
        case MSG_PLAY_PAUSEATEND: {
          logMsg (LOG_DBG, LOG_MSGS, "got: pauseatend", args);
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
          logMsg (LOG_DBG, LOG_MSGS, "got: volmute", args);
          playerVolumeMute (playerData);
          break;
        }
        case MSG_PLAYER_VOLUME: {
          logMsg (LOG_DBG, LOG_MSGS, "got: volume %s", args);
          playerVolumeSet (playerData, args);
          break;
        }
        case MSG_PLAY_STOP: {
          logMsg (LOG_DBG, LOG_MSGS, "got: stop", args);
          playerStop (playerData);
          playerData->pauseAtEnd = false;
          playerSendPauseAtEndState (playerData);
          playerSetPlayerState (playerData, PL_STATE_STOPPED);
          logMsg (LOG_DBG, LOG_BASIC, "pl state: stopped (msg-req)");
          break;
        }
        case MSG_PLAY_SONG_BEGIN: {
          logMsg (LOG_DBG, LOG_MSGS, "got: song begin", args);
          playerSeek (playerData, 0);
          break;
        }
        case MSG_PLAY_SEEK: {
          logMsg (LOG_DBG, LOG_MSGS, "got: seek", args);
          playerSeek (playerData, atol (args));
          break;
        }
        case MSG_SONG_PREP: {
          playerSongPrep (playerData, args);
          break;
        }
        case MSG_SONG_PLAY: {
          playerSongPlay (playerData, args);
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

  logProcEnd (LOG_PROC, "playerProcessMsg", "");
  return 0;
}

static int
playerProcessing (void *udata)
{
  playerdata_t      *playerData = udata;;


  if (! progstateIsRunning (playerData->progstate)) {
    progstateProcess (playerData->progstate);
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    }
    return gKillReceived;
  }

  if (mstimeCheck (&playerData->statusCheck)) {
    /* the playerSendStatus() routine will set the statusCheck var */
    playerSendStatus (playerData);
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
      playerSetPlayerState (playerData, PL_STATE_STOPPED);
      logMsg (LOG_DBG, LOG_BASIC, "pl state: stopped (gap finish)");
    }
  }

  if (playerData->playerState == PL_STATE_STOPPED &&
      ! playerData->inGap &&
      queueGetCount (playerData->playRequest) > 0) {
    prepqueue_t       *pq = NULL;
    char              *request;

    request = queueGetCurrent (playerData->playRequest);

    pq = playerLocatePreppedSong (playerData, request);
    if (pq == NULL) {
      request = queuePop (playerData->playRequest);
      free (request);
      if (gKillReceived) {
        logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      }
      return gKillReceived;
    }

    playerData->currentSong = pq;

    logMsg (LOG_DBG, LOG_BASIC, "play: %s", pq->tempname);
    playerData->currentVolume = playerData->realVolume;
    if (pq->voladjperc != 0.0) {
      double      val;

      val = pq->voladjperc / 100.0;
      val = round ((double) playerData->currentVolume +
          ((double) playerData->currentVolume * val));
      playerData->currentVolume = (int) val;
    }

    if ((pq->announce == PREP_ANNOUNCE ||
        playerData->fadeinTime == 0) &&
        ! playerData->mute) {
      volumeSet (playerData->volume, playerData->currentSink, playerData->currentVolume);
      logMsg (LOG_DBG, LOG_MAIN, "no fade-in set volume: %d", playerData->currentVolume);
    }
    pliMediaSetup (playerData->pli, pq->tempname);
    pliStartPlayback (playerData->pli, pq->songstart, pq->speed);
    playerData->currentSpeed = pq->speed;
    playerSetPlayerState (playerData, PL_STATE_LOADING);
    logMsg (LOG_DBG, LOG_BASIC, "pl state: loading");
  }

  if (playerData->playerState == PL_STATE_LOADING) {
    prepqueue_t       *pq = playerData->currentSong;
    plistate_t        plistate;

    plistate = pliState (playerData->pli);
    if (plistate == PLI_STATE_OPENING ||
        plistate == PLI_STATE_BUFFERING) {
      ;
    } else if (plistate == PLI_STATE_PLAYING) {
      if (pq->dur <= 1) {
        pq->dur = pliGetDuration (playerData->pli);
        logMsg (LOG_DBG, LOG_MAIN, "WARN: Replace duration with player data: %zd", pq->dur);
      }

        /* save for later use */
      playerData->gap = pq->gap;
      playerData->playTimePlayed = 0;
      playerSetCheckTimes (playerData, pq);

      if (pq->announce == PREP_SONG && playerData->fadeinTime > 0) {
        playerData->inFade = true;
        playerData->inFadeIn = true;
        playerData->fadeCount = 1;
        playerData->fadeSamples = playerData->fadeinTime / FADEIN_TIMESLICE + 1;
        playerFadeVolSet (playerData);
      }

      playerSetPlayerState (playerData, PL_STATE_PLAYING);
      logMsg (LOG_DBG, LOG_BASIC, "pl state: playing");

      if (pq->announce == PREP_SONG) {
        connSendMessage (playerData->conn, ROUTE_MAIN,
            MSG_PLAYBACK_BEGIN, NULL);
      }
    } else {
      /* ### FIX: need to process this; stopped/ended/error */
      ;
    }
  }

  if (playerData->playerState == PL_STATE_PLAYING &&
      ! playerData->inFade &&
      ! playerData->inGap &&
      mstimeCheck (&playerData->volumeTimeCheck)) {
    playerCheckSystemVolume (playerData);
    mstimeset (&playerData->volumeTimeCheck, 1000);
  }

  if (playerData->playerState == PL_STATE_PLAYING ||
      playerData->playerState == PL_STATE_IN_FADEOUT) {
    prepqueue_t       *pq = playerData->currentSong;

    if (playerData->fadeoutTime > 0 &&
        ! playerData->inFade &&
        mstimeCheck (&playerData->fadeTimeCheck)) {

        /* before going into the fade, check the system volume */
        /* and see if the user changed it */
      logMsg (LOG_DBG, LOG_MAIN, "check sysvol: before fade");
      playerCheckSystemVolume (playerData);
      playerStartFadeOut (playerData);
    }

    if (playerData->stopPlaying ||
        mstimeCheck (&playerData->playTimeCheck)) {
      plistate_t plistate = pliState (playerData->pli);
      ssize_t plidur = pliGetDuration (playerData->pli);
      ssize_t plitm = pliGetTime (playerData->pli);

      /* for a song with a speed adjustment, vlc returns the current */
      /* timestamp and the real duration, not adjusted values. */
      /* pq->dur is adjusted for the speed. */
      /* plitm cannot be used in conjunction with pq->dur */
      if (plistate == PLI_STATE_STOPPED ||
          plistate == PLI_STATE_ENDED ||
          plistate == PLI_STATE_ERROR ||
          playerData->stopPlaying ||
          plitm >= plidur ||
          mstimeCheck (&playerData->playEndCheck)) {

        /* stop any fade */
        playerData->inFade = false;
        playerData->inFadeOut = false;
        playerData->inFadeIn = false;
        playerData->stopPlaying = false;
        playerData->currentSpeed = 100;

        logMsg (LOG_DBG, LOG_BASIC, "actual play time: %zd", mstimeend (&playerData->playTimeStart) + playerData->playTimePlayed);
        playerStop (playerData);

        if (pq->announce == PREP_SONG) {
          if (playerData->pauseAtEnd) {
            playerData->gap = 0;
            playerData->pauseAtEnd = false;
            playerSendPauseAtEndState (playerData);
            playerSetPlayerState (playerData, PL_STATE_STOPPED);
            if (! playerData->repeat) {
              /* let the main know we're done with this song. */
              logMsg (LOG_DBG, LOG_BASIC, "pl state: stopped (pause at end)");
              connSendMessage (playerData->conn, ROUTE_MAIN,
                  MSG_PLAYBACK_STOP, NULL);
            }
          } else {
            if (! playerData->repeat) {
              /* done, go on to next song */
              connSendMessage (playerData->conn, ROUTE_MAIN,
                  MSG_PLAYBACK_FINISH, NULL);
            }
          }

          if (! playerData->repeat) {
            playerData->currentSong = NULL;
          }
        }

        if (! playerData->repeat || pq->announce == PREP_ANNOUNCE) {
          char          *request;
          prepqueue_t   *tpq;

          request = queuePop (playerData->playRequest);
          free (request);
          if (pq->announce == PREP_SONG) {
            tpq = queueIterateRemoveNode (playerData->prepQueue, &playerData->prepiteridx);
            playerPrepQueueFree (tpq);
          }
        }

        if (playerData->fadeoutTime == 0) {
          logMsg (LOG_DBG, LOG_MAIN, "check sysvol: no-fade-out");
          playerCheckSystemVolume (playerData);
        }

        if (playerData->gap > 0) {
          playerSetPlayerState (playerData, PL_STATE_IN_GAP);
          logMsg (LOG_DBG, LOG_BASIC, "pl state: in gap");
          volumeSet (playerData->volume, playerData->currentSink, 0);
          logMsg (LOG_DBG, LOG_MAIN, "gap set volume: %d", 0);
          playerData->inGap = true;
          mstimeset (&playerData->gapFinishTime, playerData->gap);
        } else {
          playerSetPlayerState (playerData, PL_STATE_STOPPED);
          logMsg (LOG_DBG, LOG_BASIC, "pl state: stopped (no gap)");
        }
      } /* has stopped */
    } /* time to check...*/
  } /* is playing */

    /* only process the prep requests when the player isn't doing much  */
    /* windows must do a physical copy, and this may take a bit of time */
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
  }
  return gKillReceived;
}

static bool
playerConnectingCallback (void *tpdata, programstate_t programState)
{
  playerdata_t  *playerData = tpdata;
  bool          rc = false;

  if (! connIsConnected (playerData->conn, ROUTE_MAIN)) {
    connConnect (playerData->conn, ROUTE_MAIN);
  }

  if (connIsConnected (playerData->conn, ROUTE_MAIN)) {
    rc = true;
  }

  return rc;
}

static bool
playerHandshakeCallback (void *tpdata, programstate_t programState)
{
  playerdata_t  *playerData = tpdata;
  bool          rc = false;

  if (connHaveHandshake (playerData->conn, ROUTE_MAIN)) {
    rc = true;
  }

  return rc;
}


static void
playerCheckSystemVolume (playerdata_t *playerData)
{
  int         tvol;

  if (playerData->mute) {
    return;
  }
  tvol = volumeGet (playerData->volume, playerData->currentSink);
  logMsg (LOG_DBG, LOG_MAIN, "get volume: %d", tvol);
  if (tvol != playerData->currentVolume) {
    playerData->realVolume = tvol;
    playerData->currentVolume = tvol;
  }
}

static void
playerSongPrep (playerdata_t *playerData, char *args)
{
  prepqueue_t     *npq;
  char            *tokptr = NULL;
  char            *aptr;
  char            stname [MAXPATHLEN];


  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  logProcBegin (LOG_PROC, "playerSongPrep");

  aptr = strtok_r (args, MSG_ARGS_RS_STR, &tokptr);
  if (aptr == NULL) {
    return;
  }

  npq = malloc (sizeof (prepqueue_t));
  assert (npq != NULL);
  npq->songname = strdup (aptr);
  assert (npq->songname != NULL);
  logMsg (LOG_DBG, LOG_BASIC, "prep request: %s", npq->songname);
  npq->songfullpath = songFullFileName (npq->songname);

  aptr = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  npq->dur = atol (aptr);
  logMsg (LOG_DBG, LOG_MAIN, "prep duration: %zd", npq->dur);

  aptr = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  npq->songstart = atol (aptr);
  logMsg (LOG_DBG, LOG_MAIN, "prep songstart: %zd", npq->songstart);

  aptr = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  npq->speed = atol (aptr);
  logMsg (LOG_DBG, LOG_MAIN, "prep speed: %zd", npq->speed);

  aptr = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  npq->voladjperc = atof (aptr);
  logMsg (LOG_DBG, LOG_MAIN, "prep voladjperc: %.1f", npq->voladjperc);

  aptr = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  npq->gap = atol (aptr);
  logMsg (LOG_DBG, LOG_MAIN, "prep gap: %zd", npq->gap);

  aptr = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  npq->announce = atol (aptr);
  logMsg (LOG_DBG, LOG_MAIN, "prep announce: %zd", npq->announce);

  songMakeTempName (playerData, npq->songname, stname, sizeof (stname));
  npq->tempname = strdup (stname);
  assert (npq->tempname != NULL);
  queuePush (playerData->prepRequestQueue, npq);
  logProcEnd (LOG_PROC, "playerSongPrep", "");
}

void
playerProcessPrepRequest (playerdata_t *playerData)
{
  prepqueue_t     *npq;

  npq = queuePop (playerData->prepRequestQueue);
  if (npq == NULL) {
    return;
  }

  logProcBegin (LOG_PROC, "playerProcessPrepRequest");
  /* FIX: TODO: if we are in client/server mode, then need to request the song
   * from the server and save it
   */
  logMsg (LOG_DBG, LOG_BASIC, "prep: %s : %s", npq->songname, npq->tempname);

  /* VLC still cannot handle internationalized names.
   * I wonder how they handle them internally.
   * Symlinks work on Linux/Mac OS.
   */
  fileopDelete (npq->tempname);
  filemanipLinkCopy (npq->songfullpath, npq->tempname);
  if (! fileopFileExists (npq->tempname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: file copy failed: %s", npq->tempname);
    logProcEnd (LOG_PROC, "playerSongPrep", "copy-failed");
    playerPrepQueueFree (npq);
    logProcEnd (LOG_PROC, "playerProcessPrepRequest", "copy-fail");
    return;
  }

  queuePush (playerData->prepQueue, npq);
  logProcEnd (LOG_PROC, "playerSongPrep", "");
}

static void
playerSongPlay (playerdata_t *playerData, char *sfname)
{
  prepqueue_t       *pq = NULL;

  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  logProcBegin (LOG_PROC, "playerSongPlay");
  logMsg (LOG_DBG, LOG_BASIC, "play request: %s", sfname);
  pq = playerLocatePreppedSong (playerData, sfname);
  if (pq == NULL) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: not prepped: %s", sfname);
    logProcEnd (LOG_PROC, "playerSongPlay", "not-prepped");
    return;
  }
  if (! fileopFileExists (pq->tempname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: no file: %s", pq->tempname);
    logProcEnd (LOG_PROC, "playerSongPlay", "no-file");
    return;
  }

  queuePush (playerData->playRequest, strdup (sfname));
  logProcEnd (LOG_PROC, "playerSongPlay", "");
}

static prepqueue_t *
playerLocatePreppedSong (playerdata_t *playerData, char *sfname)
{
  prepqueue_t       *pq = NULL;
  int               found = 0;
  int               count = 0;

  found = 0;
  count = 0;
  while (! found && count < 2) {
    queueStartIterator (playerData->prepQueue, &playerData->prepiteridx);
    pq = queueIterateData (playerData->prepQueue, &playerData->prepiteridx);
    while (pq != NULL) {
      if (strcmp (sfname, pq->songname) == 0) {
        found = 1;
        break;
      }
      pq = queueIterateData (playerData->prepQueue, &playerData->prepiteridx);
    }
    if (! found) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: song %s not prepped; processing queue", sfname);
      playerProcessPrepRequest (playerData);
      ++count;
    }
  }
  if (! found) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: unable to locate song %s", sfname);
    logProcEnd (LOG_PROC, "playerSongPlay", "not-found");
    return NULL;
  }
  return pq;
}


static void
songMakeTempName (playerdata_t *playerData, char *in, char *out, size_t maxlen)
{
  char        tnm [MAXPATHLEN];
  size_t      idx;
  pathinfo_t  *pi;

  pi = pathInfo (in);

  idx = 0;
  for (const char *p = pi->filename; *p && idx < maxlen && idx < pi->flen; ++p) {
    if ((isascii (*p) && isalnum (*p)) ||
        *p == '.' || *p == '-' || *p == '_') {
      tnm [idx++] = *p;
    }
  }
  tnm [idx] = '\0';
  pathInfoFree (pi);

    /* the profile index so we don't stomp on other bdj instances   */
    /* the global count so we don't stomp on ourselves              */
  snprintf (out, maxlen, "tmp/%02zd-%03ld-%s", sysvarsGetNum (SVL_BDJIDX),
      playerData->globalCount, tnm);
  ++playerData->globalCount;
}

/* internal routines - player handling */

static void
playerPause (playerdata_t *playerData)
{
  plistate_t plistate = pliState (playerData->pli);

  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  if (playerData->inFadeOut) {
    playerData->pauseAtEnd = true;
  } else if (plistate == PLI_STATE_PLAYING) {
    playerSetPlayerState (playerData, PL_STATE_PAUSED);
    pliPause (playerData->pli);
    /* set the play time after restarting the player */
    playerData->playTimePlayed += mstimeend (&playerData->playTimeStart);
    logMsg (LOG_DBG, LOG_BASIC, "pl state: paused");
    if (playerData->inFadeIn) {
      playerData->inFade = false;
      playerData->inFadeIn = false;
      if (! playerData->mute) {
        volumeSet (playerData->volume, playerData->currentSink, playerData->currentVolume);
      }
    }
  }
}

static void
playerPlay (playerdata_t *playerData)
{
  plistate_t plistate = pliState (playerData->pli);

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
      logMsg (LOG_DBG, LOG_BASIC, "pl state: playing (msg-req)");
    }
  }
}

static void
playerNextSong (playerdata_t *playerData)
{
  logMsg (LOG_DBG, LOG_BASIC, "next song");

  playerData->repeat = false;
  playerData->gap = 0;

  if (playerData->playerState == PL_STATE_LOADING ||
      playerData->playerState == PL_STATE_PLAYING ||
      playerData->playerState == PL_STATE_IN_FADEOUT) {
    /* the song will stop playing, and the normal logic will move */
    /* to the next song and continue playing */
    playerData->stopPlaying = true;
  } else {
    if (playerData->playerState == PL_STATE_PAUSED) {
      char  *request;

      /* this song's play request must be removed from the play request q */
      request = queuePop (playerData->playRequest);
      free (request);

      /* tell vlc to stop */
      pliStop (playerData->pli);
      playerSetPlayerState (playerData, PL_STATE_STOPPED);
      logMsg (LOG_DBG, LOG_BASIC, "pl state: stopped (was paused; next-song)");
      /* and have main advance to the next song */
      connSendMessage (playerData->conn, ROUTE_MAIN,
          MSG_PLAYBACK_STOP, NULL);
    } else {
      /* tell main to go to the next song */
      connSendMessage (playerData->conn, ROUTE_MAIN,
          MSG_PLAYBACK_FINISH, NULL);
    }
  }
}

static void
playerPauseAtEnd (playerdata_t *playerData)
{
  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  playerData->pauseAtEnd = playerData->pauseAtEnd ? false : true;
  playerSendPauseAtEndState (playerData);
}

static void
playerSendPauseAtEndState (playerdata_t *playerData)
{
  char    tbuff [20];

  snprintf (tbuff, sizeof (tbuff), "%d", playerData->pauseAtEnd);
  connSendMessage (playerData->conn, ROUTE_PLAYERUI,
      MSG_PLAY_PAUSEATEND_STATE, tbuff);
}

static void
playerFade (playerdata_t *playerData)
{
  plistate_t plistate = pliState (playerData->pli);

  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  if (playerData->inFadeOut) {
    return;
  }

  if (plistate == PLI_STATE_PLAYING) {
    logMsg (LOG_DBG, LOG_BASIC, "fade");
    if (! playerData->inFade) {
      logMsg (LOG_DBG, LOG_MAIN, "check sysvol: before fade request");
      playerCheckSystemVolume (playerData);
    }
    mstimeset (&playerData->playTimeCheck, playerData->fadeoutTime - 500);
    mstimeset (&playerData->playEndCheck, playerData->fadeoutTime);
    playerStartFadeOut (playerData);
  }
}

static void
playerSpeed (playerdata_t *playerData, char *trate)
{
  double rate;

  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  if (playerData->playerState == PL_STATE_PLAYING) {
    rate = atof (trate);
    pliRate (playerData->pli, (ssize_t) rate);
    playerData->currentSpeed = (ssize_t) rate;
  }
}

static void
playerSeek (playerdata_t *playerData, ssize_t reqpos)
{
  double        drate;
  double        dpos;
  ssize_t       nreqpos;
  ssize_t       seekpos;
  prepqueue_t   *pq = playerData->currentSong;

  /* if duration is adjusted for speed, so is reqpos                */
  /* need to change it back to something the player will understand */
  /* the 'playTimeStart' position is already relative to songstart  */
  nreqpos = reqpos;
  seekpos = reqpos + pq->songstart;
  if (pq->speed != 100) {
    drate = (double) pq->speed / 100.0;
    dpos = (double) seekpos * drate;
    seekpos = (ssize_t) dpos;
  }
  pliSeek (playerData->pli, seekpos);
  /* newpos is from vlc; don't use it */
  if (pq->speed != 100) {
    drate = (double) pq->speed / 100.0;
    dpos = (double) nreqpos * drate;
    nreqpos = (ssize_t) dpos;
  }
  playerData->playTimePlayed = nreqpos;
  playerSetCheckTimes (playerData, pq);
}

static void
playerStop (playerdata_t *playerData)
{
  plistate_t plistate = pliState (playerData->pli);

  if (plistate == PLI_STATE_PLAYING ||
      plistate == PLI_STATE_PAUSED) {
    pliStop (playerData->pli);
  }
}

static void
playerVolumeSet (playerdata_t *playerData, char *tvol)
{
  int       newvol;

  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  newvol = (int) atol (tvol);
  volumeSet (playerData->volume, playerData->currentSink, newvol);
  playerData->realVolume = newvol;
  playerData->currentVolume = newvol;
}

static void
playerVolumeMute (playerdata_t *playerData)
{
  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  playerData->mute = playerData->mute ? false : true;
  if (playerData->mute) {
    volumeSet (playerData->volume, playerData->currentSink, 0);
  } else {
    volumeSet (playerData->volume, playerData->currentSink, playerData->currentVolume);
  }
}

static void
playerPrepQueueFree (void *data)
{
  prepqueue_t     *pq = data;

  if (pq != NULL) {
    if (pq->songfullpath != NULL) {
      free (pq->songfullpath);
    }
    if (pq->songname != NULL) {
      free (pq->songname);
    }
    if (pq->tempname != NULL) {
      fileopDelete (pq->tempname);
      free (pq->tempname);
    }
    free (pq);
  }
}

static void
playerSigHandler (int sig)
{
  gKillReceived = 1;
}

static void
playerSetAudioSink (playerdata_t *playerData, char *sinkname)
{
  int           found = 0;
  int           idx = -1;

  logProcBegin (LOG_PROC, "playerSetAudioSink");
  /* the sink list is not ordered */
  found = 0;
  for (size_t i = 0; i < playerData->sinklist.count; ++i) {
    if (strcmp (sinkname, playerData->sinklist.sinklist [i].name) == 0) {
      found = 1;
      idx = (int) i;
      break;
    }
  }

  if (found && idx >= 0) {
    playerData->currentSink = playerData->sinklist.sinklist [idx].name;
    logMsg (LOG_DBG, LOG_IMPORTANT, "audio sink set to %s", playerData->sinklist.sinklist [idx].description);
  } else {
    playerData->currentSink = playerData->sinklist.defname;
    logMsg (LOG_DBG, LOG_IMPORTANT, "audio sink set to default");
  }
  logProcEnd (LOG_PROC, "playerSetAudioSink", "");
}

static void
playerInitSinklist (playerdata_t *playerData)
{
  int count;

  volumeFreeSinkList (&playerData->sinklist);

  logProcBegin (LOG_PROC, "playerInitSinklist");
  playerData->sinklist.defname = "";
  playerData->sinklist.count = 0;
  playerData->sinklist.sinklist = NULL;
  playerData->defaultSink = "";
  playerData->currentSink = "";

  count = 0;
  while (volumeGetSinkList (playerData->volume, "", &playerData->sinklist) != 0 &&
      count < 20) {
    mssleep (100);
    ++count;
  }
  playerData->defaultSink = playerData->sinklist.defname;
  logProcEnd (LOG_PROC, "playerInitSinklist", "");
}

static void
playerFadeVolSet (playerdata_t *playerData)
{
  double  findex = calcFadeIndex (playerData);
  int     newvol;

  newvol = (int) round ((double) playerData->currentVolume * findex);

  if (newvol > playerData->currentVolume) {
    newvol = playerData->currentVolume;
  }
  if (! playerData->mute) {
    volumeSet (playerData->volume, playerData->currentSink, newvol);
  }
  logMsg (LOG_DBG, LOG_MAIN, "fade set volume: %d count:%ld time %zd",
      newvol, playerData->fadeCount, mstimeend (&playerData->playEndCheck));
  if (playerData->inFadeIn) {
    ++playerData->fadeCount;
  }
  if (playerData->inFadeOut) {
    --playerData->fadeCount;
  }
  mstimeset (&playerData->fadeTimeNext,
      (playerData->inFadeOut ? FADEOUT_TIMESLICE : FADEIN_TIMESLICE));
  if (playerData->inFadeIn &&
      newvol >= playerData->currentVolume) {
    playerData->inFade = false;
    playerData->inFadeIn = false;
  }
  if (playerData->inFadeOut && newvol <= 0) {
      /* leave inFade set to prevent race conditions in the main loop */
      /* the player stop condition will reset the inFade flag */
    playerData->inFadeOut = false;
    volumeSet (playerData->volume, playerData->currentSink, 0);
    logMsg (LOG_DBG, LOG_MAIN, "fade-out done volume: %d time: %zd", 0, mstimeend (&playerData->playEndCheck));
  }
}

static double
calcFadeIndex (playerdata_t *playerData)
{
  double      findex;

  double index = (double) playerData->fadeCount;
  double range = (double) playerData->fadeSamples;

  findex = fmax(0.0, fmin (1.0, index / range));
  switch (playerData->fadeType) {
    case FADETYPE_TRIANGLE: {
      break;
    }
    case FADETYPE_QUARTER_SINE: {
      findex = sin (findex * M_PI / 2.0);
      break;
    }
    case FADETYPE_HALF_SINE: {
      findex = 1.0 - cos (findex * M_PI) / 2.0;
      break;
    }
    case FADETYPE_LOGARITHMIC: {
      findex = pow (0.1, (1.0 - findex) * 5.0);
      break;
    }
    case FADETYPE_INVERTED_PARABOLA: {
      findex = 1.0 - (1.0 - findex) * (1.0 - findex);
      break;
    }
  }
  return findex;
}

static void
playerStartFadeOut (playerdata_t *playerData)
{
  playerData->inFade = true;
  playerData->inFadeOut = true;
  playerData->fadeSamples = playerData->fadeoutTime / FADEOUT_TIMESLICE;
  playerData->fadeCount = playerData->fadeSamples;
  logMsg (LOG_DBG, LOG_MAIN, "fade: samples: %ld", playerData->fadeCount);
  playerFadeVolSet (playerData);
  playerSetPlayerState (playerData, PL_STATE_IN_FADEOUT);
  logMsg (LOG_DBG, LOG_BASIC, "pl state: in fadeout");
}


static void
playerSetCheckTimes (playerdata_t *playerData, prepqueue_t *pq)
{
  ssize_t newdur;

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
  logMsg (LOG_DBG, LOG_MAIN, "pq->dur: %zd", pq->dur);
  logMsg (LOG_DBG, LOG_MAIN, "newdur: %zd", newdur);
  logMsg (LOG_DBG, LOG_MAIN, "playTimeStart: %zd", mstimeend (&playerData->playTimeStart));
  logMsg (LOG_DBG, LOG_MAIN, "playEndCheck: %zd", mstimeend (&playerData->playEndCheck));
  logMsg (LOG_DBG, LOG_MAIN, "playTimeCheck: %zd", mstimeend (&playerData->playTimeCheck));
  logMsg (LOG_DBG, LOG_MAIN, "fadeTimeCheck: %zd", mstimeend (&playerData->fadeTimeCheck));
}

static void
playerSetPlayerState (playerdata_t *playerData, playerstate_t pstate)
{
  char    tbuff [20];

  playerData->playerState = pstate;
  snprintf (tbuff, sizeof (tbuff), "%d", playerData->playerState);
  connSendMessage (playerData->conn, ROUTE_MAIN,
      MSG_PLAYER_STATE, tbuff);
  connSendMessage (playerData->conn, ROUTE_PLAYERUI,
      MSG_PLAYER_STATE, tbuff);
  /* any time there is a change of player state, send the status */
  playerSendStatus (playerData);
}


static void
playerSendStatus (playerdata_t *playerData)
{
  char        rbuff [3096];
  char        *playstate;
  prepqueue_t *pq = playerData->currentSong;
  ssize_t     tm;
  ssize_t     dur;

  if (playerData->playerState == playerData->lastPlayerState &&
      playerData->playerState != PL_STATE_PLAYING &&
      playerData->playerState != PL_STATE_IN_FADEOUT) {
    return;
  }

  playerData->lastPlayerState = playerData->playerState;

  rbuff [0] = '\0';

  dur = 0;
  if (pq != NULL) {
    dur = pq->dur;
  }

  if (! progstateIsRunning (playerData->progstate)) {
    playstate = "stop";
    dur = 0;
  } else {
    switch (playerData->playerState) {
      case PL_STATE_UNKNOWN:
      case PL_STATE_STOPPED: {
        playstate = "stop";
        dur = 0;
        break;
      }
      case PL_STATE_IN_GAP: {
        playstate = "play";
        dur = 0;
        break;
      }
      case PL_STATE_IN_FADEOUT:
      case PL_STATE_LOADING:
      case PL_STATE_PLAYING: {
        playstate = "play";
        break;
      }
      case PL_STATE_PAUSED: {
        playstate = "pause";
        break;
      }
    }
  }

  tm = 0;
  if (playerData->playerState == PL_STATE_PAUSED) {
    tm = playerData->playTimePlayed;
  } else if (playerData->playerState == PL_STATE_PLAYING ||
      playerData->playerState == PL_STATE_IN_FADEOUT) {
    tm = playerData->playTimePlayed + mstimeend (&playerData->playTimeStart);
  } else {
    tm = 0;
  }

  snprintf (rbuff, sizeof (rbuff), "%s%c%d%c%d%c%d%c%zd%c%zd%c%zd",
      playstate, MSG_ARGS_RS, playerData->repeat, MSG_ARGS_RS,
      playerData->pauseAtEnd, MSG_ARGS_RS,
      playerData->currentVolume, MSG_ARGS_RS,
      playerData->currentSpeed, MSG_ARGS_RS,
      tm, MSG_ARGS_RS, dur);

  connSendMessage (playerData->conn, ROUTE_MAIN,
      MSG_PLAYER_STATUS_DATA, rbuff);
  /* four times a second... */
  mstimeset (&playerData->statusCheck, 250);
}
