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

#include "bdj4.h"
#include "bdj4init.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "conn.h"
#include "filemanip.h"
#include "fileop.h"
#include "fileop.h"
#include "lock.h"
#include "log.h"
#include "ossignal.h"
#include "osutils.h"
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

enum {
  STOP_NEXTSONG = 0,
  STOP_NORMAL = 1,
};

typedef struct {
  char          *songfullpath;
  char          *songname;
  char          *tempname;
  ssize_t       dur;
  ssize_t       songstart;
  double        voladjperc;
  long          uniqueidx;
  int           speed;
  int           announce;     // one of PREP_SONG or PREP_ANNOUNCE
} prepqueue_t;

typedef struct {
  long          uniqueidx;
  char          *songname;
} playrequest_t;

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
  int             realVolume;     // the real volume that is set (+voladjperc).
  int             currentVolume;  // current volume settings, no adjustments.
  int             actualVolume;   // testsuite: the actualvolume that is set
  int             currentSpeed;
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
  long            priorGap;
  long            gap;
  mstime_t        gapFinishTime;
  int             fadeType;
  ssize_t         fadeinTime;
  ssize_t         fadeoutTime;
  int             fadeCount;
  int             fadeSamples;
  time_t          fadeTimeStart;
  mstime_t        fadeTimeNext;
  int             stopNextsongFlag;
  bool            inFade : 1;
  bool            inFadeIn : 1;
  bool            inFadeOut : 1;
  bool            inGap : 1;
  bool            mute : 1;
  bool            pauseAtEnd : 1;
  bool            repeat : 1;
  bool            stopPlaying : 1;
} playerdata_t;

enum {
  FADEIN_TIMESLICE = 50,
  FADEOUT_TIMESLICE = 100,
};

static void     playerCheckSystemVolume (playerdata_t *playerData);
static int      playerProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static int      playerProcessing (void *udata);
static bool     playerConnectingCallback (void *tpdata, programstate_t programState);
static bool     playerHandshakeCallback (void *tpdata, programstate_t programState);
static bool     playerStoppingCallback (void *tpdata, programstate_t programState);
static bool     playerClosingCallback (void *tpdata, programstate_t programState);
static void     playerSongPrep (playerdata_t *playerData, char *sfname);
static void     playerSongClearPrep (playerdata_t *playerData, char *sfname);
void            playerProcessPrepRequest (playerdata_t *playerData);
static void     playerSongPlay (playerdata_t *playerData, char *args);
static prepqueue_t * playerLocatePreppedSong (playerdata_t *playerData, long uniqueidx, const char *sfname);
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
static int      playerLimitVolume (int vol);
static ssize_t  playerCalcPlayedTime (playerdata_t *playerData);
static void     playerSetDefaultVolume (playerdata_t *playerData);
static void     playerFreePlayRequest (void *tpreq);
static void     playerChkPlayerStatus (playerdata_t *playerData, int routefrom);
static void     playerChkPlayerSong (playerdata_t *playerData, int routefrom);

static int  gKillReceived = 0;

int
main (int argc, char *argv[])
{
  playerdata_t    playerData;
  uint16_t        listenPort;
  int             flags;

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
  mstimeset (&playerData.statusCheck, 3600000);
  playerData.priorGap = 2000;
  playerData.gap = 2000;
  playerData.pli = NULL;
  playerData.prepQueue = queueAlloc ("prep-q", playerPrepQueueFree);
  playerData.prepRequestQueue = queueAlloc ("prep-req", playerPrepQueueFree);
  playerData.progstate = progstateInit ("player");
  playerData.inFade = false;
  playerData.inFadeIn = false;
  playerData.inFadeOut = false;
  playerData.inGap = false;
  playerData.mute = false;
  playerData.pauseAtEnd = false;
  playerData.repeat = false;
  playerData.stopNextsongFlag = STOP_NORMAL;
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
  bdj4startup (argc, argv, NULL, "p", ROUTE_PLAYER, flags);

  playerData.conn = connInit (ROUTE_PLAYER);

  playerData.fadeType = bdjoptGetNum (OPT_P_FADETYPE);
  playerData.fadeinTime = bdjoptGetNum (OPT_P_FADEINTIME);
  playerData.fadeoutTime = bdjoptGetNum (OPT_P_FADEOUTTIME);

  playerData.defaultSink = "";
  playerData.currentSink = "";
  volumeSinklistInit (&playerData.sinklist);
  playerData.currentSpeed = 100;

  logMsg (LOG_DBG, LOG_IMPORTANT, "volume interface: %s", bdjoptGetStr (OPT_M_VOLUME_INTFC));
  playerData.volume = volumeInit (bdjoptGetStr (OPT_M_VOLUME_INTFC));
  assert (playerData.volume != NULL);

  playerInitSinklist (&playerData);

  if (playerData.sinklist.sinklist != NULL) {
    logMsg (LOG_DBG, LOG_BASIC, "vol-sinklist");
    for (size_t i = 0; i < playerData.sinklist.count; ++i) {
      logMsg (LOG_DBG, LOG_BASIC, "  %d %3d %s %s",
               playerData.sinklist.sinklist [i].defaultFlag,
               playerData.sinklist.sinklist [i].idxNumber,
               playerData.sinklist.sinklist [i].name,
               playerData.sinklist.sinklist [i].description);
    }
  }

  /* sets the current sink */
  playerSetAudioSink (&playerData, bdjoptGetStr (OPT_MP_AUDIOSINK));

  /* this works for pulse audio */
  if (isLinux () &&
      strcmp (bdjoptGetStr (OPT_M_VOLUME_INTFC), "libvolpa") == 0) {
    osSetEnv ("PULSE_SINK", playerData.currentSink);
  }

  logMsg (LOG_DBG, LOG_IMPORTANT, "player interface: %s", bdjoptGetStr (OPT_M_PLAYER_INTFC));
  logMsg (LOG_DBG, LOG_IMPORTANT, "volume sink: %s", playerData.currentSink);
  playerData.pli = pliInit (bdjoptGetStr (OPT_M_PLAYER_INTFC),
      playerData.currentSink);
  /* some audio device interfaces may not have the audio device enumeration. */
  /* (e.g. I don't know how to do this on macos) */
  /* in this case, retrieve the list of devices from the player if possible. */
  if (! volumeHaveSinkList (playerData.volume)) {
    /* try getting the sink list from the player */
    pliAudioDeviceList (playerData.pli, &playerData.sinklist);
    if (playerData.sinklist.sinklist != NULL) {
      logMsg (LOG_DBG, LOG_BASIC, "vlc-sinklist");
      for (size_t i = 0; i < playerData.sinklist.count; ++i) {
        logMsg (LOG_DBG, LOG_BASIC, "  %d %3d %s %s",
                 playerData.sinklist.sinklist [i].defaultFlag,
                 playerData.sinklist.sinklist [i].idxNumber,
                 playerData.sinklist.sinklist [i].name,
                 playerData.sinklist.sinklist [i].description);
      }
    }
    playerSetAudioSink (&playerData, bdjoptGetStr (OPT_MP_AUDIOSINK));
    /* use the player to set the audio output device */
    pliSetAudioDevice (playerData.pli, playerData.currentSink);
  }

  playerSetDefaultVolume (&playerData);

  listenPort = bdjvarsGetNum (BDJVL_PLAYER_PORT);
  sockhMainLoop (listenPort, playerProcessMsg, playerProcessing, &playerData);
  connFree (playerData.conn);
  progstateFree (playerData.progstate);
  logEnd ();
  return 0;
}

/* internal routines */

static bool
playerStoppingCallback (void *tpdata, programstate_t programState)
{
  playerdata_t    *playerData = tpdata;

  logProcBegin (LOG_PROC, "playerStoppingCallback");
  connDisconnectAll (playerData->conn);
  logProcEnd (LOG_PROC, "playerStoppingCallback", "");
  return STATE_FINISHED;
}

static bool
playerClosingCallback (void *tpdata, programstate_t programState)
{
  playerdata_t  *playerData = tpdata;
  int           origvol;
  int           bdj3flag;

  logProcBegin (LOG_PROC, "playerClosingCallback");

  bdj4shutdown (ROUTE_PLAYER, NULL);

  if (playerData->pli != NULL) {
    pliStop (playerData->pli);
    pliClose (playerData->pli);
    pliFree (playerData->pli);
  }

  if (playerData->currentSong != NULL) {
    playerPrepQueueFree (playerData->currentSong);
    playerData->currentSong = NULL;
  }

  origvol = volregClear (playerData->currentSink);
  bdj3flag = volregCheckBDJ3Flag ();
  if (origvol > 0) {
    /* note that if there are BDJ4 instances with different sinks */
    /* the bdj4 flag will be improperly cleared */
    volregClearBDJ4Flag ();
    if (! bdj3flag) {
      volumeSet (playerData->volume, playerData->currentSink, origvol);
      playerData->actualVolume = origvol;
      logMsg (LOG_DBG, LOG_MAIN, "set to orig volume: (was:%d) %d", playerData->originalSystemVolume, origvol);
    }
  }
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

  logProcEnd (LOG_PROC, "playerClosingCallback", "");
  return STATE_FINISHED;
}

static int
playerProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  playerdata_t      *playerData;

  logProcBegin (LOG_PROC, "playerProcessMsg");
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
        case MSG_CHK_PLAYER_STATUS: {
          playerChkPlayerStatus (playerData, routefrom);
          break;
        }
        case MSG_CHK_PLAYER_SONG: {
          playerChkPlayerSong (playerData, routefrom);
          break;
        }
        case MSG_CHK_PLAYER_FADEIN: {
          playerData->fadeinTime = atol (args);
          break;
        }
        case MSG_CHK_PLAYER_FADEOUT: {
          playerData->fadeoutTime = atol (args);
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
  playerdata_t  *playerData = udata;
  int           stop = false;

  if (! progstateIsRunning (playerData->progstate)) {
    progstateProcess (playerData->progstate);
    if (progstateCurrState (playerData->progstate) == STATE_CLOSED) {
      stop = true;
    }
    if (gKillReceived) {
      progstateShutdownProcess (playerData->progstate);
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    }
    return stop;
  }

  connProcessUnconnected (playerData->conn);

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
      logMsg (LOG_DBG, LOG_BASIC, "gap finish");
      playerSetPlayerState (playerData, PL_STATE_STOPPED);
    }
  }

  if (playerData->playerState == PL_STATE_STOPPED &&
      ! playerData->inGap &&
      queueGetCount (playerData->playRequest) > 0) {
    prepqueue_t   *pq = NULL;
    prepqueue_t   *tpq = NULL;
    playrequest_t *preq = NULL;

    if (! playerData->repeat) {
      preq = queueGetCurrent (playerData->playRequest);
      pq = playerLocatePreppedSong (playerData, preq->uniqueidx, preq->songname);
      if (pq == NULL) {
        preq = queuePop (playerData->playRequest);
        playerFreePlayRequest (preq);
        if (gKillReceived) {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
        }
        return gKillReceived;
      }

      playerData->currentSong = pq;
      tpq = queueIterateRemoveNode (playerData->prepQueue, &playerData->prepiteridx);
      /* do not free */
    }

    if (playerData->repeat) {
      pq = playerData->currentSong;
    }

    logMsg (LOG_DBG, LOG_BASIC, "play: %s", pq->tempname);
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
      logMsg (LOG_DBG, LOG_MAIN, "no fade-in set volume: %d", playerData->currentVolume);
    }
    pliMediaSetup (playerData->pli, pq->tempname);
    pliStartPlayback (playerData->pli, pq->songstart, pq->speed);
    playerData->currentSpeed = pq->speed;
    playerSetPlayerState (playerData, PL_STATE_LOADING);
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
    } else {
      /* ### FIX: need to process this; stopped/ended/error */
      ;
    }
  }

  if (playerData->playerState == PL_STATE_PLAYING &&
      mstimeCheck (&playerData->volumeTimeCheck)) {
    playerCheckSystemVolume (playerData);
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
      /* pli-time cannot be used in conjunction with pq->dur */
      if (plistate == PLI_STATE_STOPPED ||
          plistate == PLI_STATE_ENDED ||
          plistate == PLI_STATE_ERROR ||
          playerData->stopPlaying ||
          plitm >= plidur ||
          mstimeCheck (&playerData->playEndCheck)) {
        char  nsflag [20];

        /* done, go on to next song, history is determined by */
        /* the stopnextsongflag */
        snprintf (nsflag, sizeof (nsflag), "%d", playerData->stopNextsongFlag);

        /* stop any fade */
        playerData->inFade = false;
        playerData->inFadeOut = false;
        playerData->inFadeIn = false;
        playerData->stopNextsongFlag = STOP_NORMAL;
        playerData->stopPlaying = false;
        playerData->currentSpeed = 100;

        logMsg (LOG_DBG, LOG_BASIC, "actual play time: %zd", mstimeend (&playerData->playTimeStart) + playerData->playTimePlayed);
        playerStop (playerData);

        if (pq->announce == PREP_SONG) {
          if (playerData->pauseAtEnd) {
            playerData->priorGap = playerData->gap;
            playerData->gap = 0;
            playerData->pauseAtEnd = false;
            playerSendPauseAtEndState (playerData);
            logMsg (LOG_DBG, LOG_BASIC, "pause-at-end");
            playerSetPlayerState (playerData, PL_STATE_STOPPED);
            if (! playerData->repeat) {
              /* let main know we're done with this song. */
              connSendMessage (playerData->conn, ROUTE_MAIN,
                  MSG_PLAYBACK_FINISH_STOP, nsflag);
            }
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
              connSendMessage (playerData->conn, ROUTE_MAIN,
                  MSG_PLAYBACK_FINISH, nsflag);
            }
          }

          if (! playerData->repeat) {
            playerPrepQueueFree (playerData->currentSong);
            playerData->currentSong = NULL;
          }
        }

        /* the play request for a song on repeat is left in the queue */
        /* so that the song will be re-played when finished. */
        if (! playerData->repeat || pq->announce == PREP_ANNOUNCE) {
          playrequest_t *preq;

          preq = queuePop (playerData->playRequest);
          playerFreePlayRequest (preq);
        }

        if (playerData->fadeoutTime == 0) {
          logMsg (LOG_DBG, LOG_MAIN, "check sysvol: no-fade-out");
          playerCheckSystemVolume (playerData);
        }

        if (playerData->gap > 0) {
          playerSetPlayerState (playerData, PL_STATE_IN_GAP);
          playerData->realVolume = 0;
          volumeSet (playerData->volume, playerData->currentSink, 0);
          playerData->actualVolume = 0;
          logMsg (LOG_DBG, LOG_MAIN, "gap set volume: %d", 0);
          playerData->inGap = true;
          mstimeset (&playerData->gapFinishTime, playerData->gap);
        } else {
          logMsg (LOG_DBG, LOG_BASIC, "no-gap");
          playerSetPlayerState (playerData, PL_STATE_STOPPED);
        }
        playerData->gap = playerData->priorGap;
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
    progstateShutdownProcess (playerData->progstate);
  }
  return stop;
}

static bool
playerConnectingCallback (void *tpdata, programstate_t programState)
{
  playerdata_t  *playerData = tpdata;
  bool          rc = STATE_NOT_FINISH;

  logProcBegin (LOG_PROC, "playerConnectingCallback");

  connProcessUnconnected (playerData->conn);

  if (! connIsConnected (playerData->conn, ROUTE_MAIN)) {
    connConnect (playerData->conn, ROUTE_MAIN);
  }

  if (connIsConnected (playerData->conn, ROUTE_MAIN)) {
    rc = STATE_FINISHED;
  }

  logProcEnd (LOG_PROC, "playerConnectingCallback", "");
  return rc;
}

static bool
playerHandshakeCallback (void *tpdata, programstate_t programState)
{
  playerdata_t  *playerData = tpdata;
  bool          rc = STATE_NOT_FINISH;

  logProcBegin (LOG_PROC, "playerHandshakeCallback");

  connProcessUnconnected (playerData->conn);

  if (connHaveHandshake (playerData->conn, ROUTE_MAIN)) {
    rc = STATE_FINISHED;
  }

  logProcEnd (LOG_PROC, "playerHandshakeCallback", "");
  return rc;
}


static void
playerCheckSystemVolume (playerdata_t *playerData)
{
  int         tvol;
  int         voldiff;

  logProcBegin (LOG_PROC, "playerCheckSystemVolume");
  mstimeset (&playerData->volumeTimeCheck, 1000);

  if (playerData->inFade || playerData->inGap || playerData->mute) {
    logProcEnd (LOG_PROC, "playerCheckSystemVolume", "in-fade-gap-mute");
    return;
  }

  tvol = volumeGet (playerData->volume, playerData->currentSink);
  tvol = playerLimitVolume (tvol);
  // logMsg (LOG_DBG, LOG_MAIN, "get volume: %d", tvol);
  voldiff = tvol - playerData->realVolume;
  if (tvol != playerData->realVolume) {
    playerData->realVolume += voldiff;
    playerData->realVolume = playerLimitVolume (playerData->realVolume);
    playerData->currentVolume += voldiff;
    playerData->currentVolume = playerLimitVolume (playerData->currentVolume);
  }
  logProcEnd (LOG_PROC, "playerCheckSystemVolume", "");
}

static void
playerSongPrep (playerdata_t *playerData, char *args)
{
  prepqueue_t     *npq;
  char            *tokptr = NULL;
  char            *p;
  char            stname [MAXPATHLEN];

  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  logProcBegin (LOG_PROC, "playerSongPrep");

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokptr);
  if (p == NULL) {
    logProcEnd (LOG_PROC, "playerSongPrep", "no-data");
    return;
  }

  npq = malloc (sizeof (prepqueue_t));
  assert (npq != NULL);
  npq->songname = strdup (p);
  assert (npq->songname != NULL);
  logMsg (LOG_DBG, LOG_BASIC, "prep request: %s", npq->songname);
  npq->songfullpath = songFullFileName (npq->songname);

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  npq->dur = atol (p);
  logMsg (LOG_DBG, LOG_MAIN, "     duration: %zd", npq->dur);

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  npq->songstart = atol (p);
  logMsg (LOG_DBG, LOG_MAIN, "     songstart: %zd", npq->songstart);

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  npq->speed = atoi (p);
  logMsg (LOG_DBG, LOG_MAIN, "     speed: %zd", npq->speed);

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  npq->voladjperc = atof (p);
  logMsg (LOG_DBG, LOG_MAIN, "     voladjperc: %.1f", npq->voladjperc);

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  npq->announce = atoi (p);
  logMsg (LOG_DBG, LOG_MAIN, "     announce: %d", npq->announce);

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  npq->uniqueidx = atol (p);
  logMsg (LOG_DBG, LOG_MAIN, "     uniqueidx: %ld", npq->uniqueidx);

  songMakeTempName (playerData, npq->songname, stname, sizeof (stname));
  npq->tempname = strdup (stname);
  assert (npq->tempname != NULL);
  queuePush (playerData->prepRequestQueue, npq);
  logProcEnd (LOG_PROC, "playerSongPrep", "");
}

static void
playerSongClearPrep (playerdata_t *playerData, char *args)
{
  prepqueue_t     *tpq;
  char            *tokptr = NULL;
  char            *p;
  long            uniqueidx;

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokptr);
  if (p == NULL) {
    return;
  }
  uniqueidx = atol (p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  if (p == NULL) {
    return;
  }

  tpq = playerLocatePreppedSong (playerData, uniqueidx, p);
  if (tpq != NULL) {
    tpq = queueIterateRemoveNode (playerData->prepQueue, &playerData->prepiteridx);
    playerPrepQueueFree (tpq);
  }
}

void
playerProcessPrepRequest (playerdata_t *playerData)
{
  prepqueue_t     *npq;
  FILE            *fh;
  ssize_t         sz;
  char            *buff;
  mstime_t        mstm;
  time_t          tm;

  logProcBegin (LOG_PROC, "playerProcessPrepRequest");

  npq = queuePop (playerData->prepRequestQueue);
  if (npq == NULL) {
    logProcEnd (LOG_PROC, "playerProcessPrepRequest", "no-song");
    return;
  }

  mstimestart (&mstm);
  logMsg (LOG_DBG, LOG_BASIC, "prep: %ld %s : %s", npq->uniqueidx, npq->songname, npq->tempname);

  /* VLC still cannot handle internationalized names.
   * I wonder how they handle them internally.
   * Symlinks work on Linux/Mac OS.
   */
  fileopDelete (npq->tempname);
  filemanipLinkCopy (npq->songfullpath, npq->tempname);
  if (! fileopFileExists (npq->tempname)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: file copy failed: %s", npq->tempname);
    logProcEnd (LOG_PROC, "playerSongPrep", "copy-failed");
    playerPrepQueueFree (npq);
    logProcEnd (LOG_PROC, "playerProcessPrepRequest", "copy-fail");
    return;
  }

  /* read the entire file in order to get it into the operating system's */
  /* filesystem cache */
  sz = fileopSize (npq->songfullpath);
  buff = malloc (sz);
  fh = fileopOpen (npq->songfullpath, "rb");
  fread (buff, sz, 1, fh);
  fclose (fh);
  free (buff);

  queuePush (playerData->prepQueue, npq);
  tm = mstimeend (&mstm);
  logMsg (LOG_DBG, LOG_BASIC, "prep time %zd %s", tm, npq->songname);
  logProcEnd (LOG_PROC, "playerProcessPrepRequest", "");
}

static void
playerSongPlay (playerdata_t *playerData, char *args)
{
  prepqueue_t   *pq = NULL;
  playrequest_t *preq = NULL;
  char          *p;
  char          *tokstr = NULL;
  long          uniqueidx;

  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  logProcBegin (LOG_PROC, "playerSongPlay");

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  if (p == NULL) {
    logProcEnd (LOG_PROC, "playerSongPlay", "bad-msg-a");
    return;
  }
  uniqueidx = atol (p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (p == NULL) {
    logProcEnd (LOG_PROC, "playerSongPlay", "bad-msg-b");
    return;
  }

  logMsg (LOG_DBG, LOG_BASIC, "play request: %ld %s", uniqueidx, p);
  pq = playerLocatePreppedSong (playerData, uniqueidx, p);
  if (pq == NULL) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: not prepped: %s", p);
    logProcEnd (LOG_PROC, "playerSongPlay", "not-prepped");
    return;
  }
  if (! fileopFileExists (pq->tempname)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: no file: %s", pq->tempname);
    logProcEnd (LOG_PROC, "playerSongPlay", "no-file");
    return;
  }

  preq = malloc (sizeof (playrequest_t));
  preq->uniqueidx = uniqueidx;
  preq->songname = strdup (p);
  queuePush (playerData->playRequest, preq);
  logProcEnd (LOG_PROC, "playerSongPlay", "");
}

static prepqueue_t *
playerLocatePreppedSong (playerdata_t *playerData, long uniqueidx, const char *sfname)
{
  prepqueue_t       *pq = NULL;
  bool              found = false;
  int               count = 0;

  logProcBegin (LOG_PROC, "playerLocatePreppedSong");

  found = false;
  count = 0;
  if (playerData->repeat) {
    pq = playerData->currentSong;
    if (uniqueidx != PL_UNIQUE_ANN && uniqueidx == pq->uniqueidx) {
      logMsg (LOG_DBG, LOG_BASIC, "song-play %s found as repeat", sfname);
      found = true;
    }
  }

  while (! found && count < 2) {
    queueStartIterator (playerData->prepQueue, &playerData->prepiteridx);
    pq = queueIterateData (playerData->prepQueue, &playerData->prepiteridx);
    while (pq != NULL) {
      if (uniqueidx != PL_UNIQUE_ANN && uniqueidx == pq->uniqueidx) {
        found = true;
        break;
      }
      if (uniqueidx == PL_UNIQUE_ANN && uniqueidx == pq->uniqueidx &&
          strcmp (sfname, pq->songname) == 0) {
        found = true;
        break;
      }
      pq = queueIterateData (playerData->prepQueue, &playerData->prepiteridx);
    }
    if (! found) {
      /* this usually happens when a song is prepped and then immediately */
      /* played before it has had time to be prepped (e.g. during tests) */
      logMsg (LOG_ERR, LOG_IMPORTANT, "WARN: song %s not prepped; processing queue", sfname);
      playerProcessPrepRequest (playerData);
      ++count;
    }
  }
  if (! found) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: unable to locate song %s", sfname);
    logProcEnd (LOG_PROC, "playerSongPlay", "not-found");
    return NULL;
  }
  logProcEnd (LOG_PROC, "playerLocatePreppedSong", "");
  return pq;
}


static void
songMakeTempName (playerdata_t *playerData, char *in, char *out, size_t maxlen)
{
  char        tnm [MAXPATHLEN];
  size_t      idx;
  pathinfo_t  *pi;

  logProcBegin (LOG_PROC, "songMakeTempName");

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
  logProcEnd (LOG_PROC, "songMakeTempName", "");
}

/* internal routines - player handling */

static void
playerPause (playerdata_t *playerData)
{
  plistate_t plistate = pliState (playerData->pli);

  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  logProcBegin (LOG_PROC, "playerPause");

  if (playerData->inFadeOut) {
    playerData->pauseAtEnd = true;
  } else if (plistate == PLI_STATE_PLAYING) {
    playerSetPlayerState (playerData, PL_STATE_PAUSED);
    pliPause (playerData->pli);
    /* set the play time after restarting the player */
    playerData->playTimePlayed += mstimeend (&playerData->playTimeStart);
    if (playerData->inFadeIn) {
      playerData->inFade = false;
      playerData->inFadeIn = false;
      if (! playerData->mute) {
        volumeSet (playerData->volume, playerData->currentSink, playerData->realVolume);
        playerData->actualVolume = playerData->realVolume;
      }
    }
  }
  logProcEnd (LOG_PROC, "playerPause", "");
}

static void
playerPlay (playerdata_t *playerData)
{
  plistate_t plistate = pliState (playerData->pli);

  logProcBegin (LOG_PROC, "playerPlay");

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
  logProcEnd (LOG_PROC, "playerPlay", "");
}

static void
playerNextSong (playerdata_t *playerData)
{
  logMsg (LOG_DBG, LOG_BASIC, "next song");

  logProcBegin (LOG_PROC, "playerNextSong");

  playerData->repeat = false;
  playerData->priorGap = playerData->gap;
  playerData->gap = 0;

  if (playerData->playerState == PL_STATE_LOADING ||
      playerData->playerState == PL_STATE_PLAYING ||
      playerData->playerState == PL_STATE_IN_FADEOUT) {
    /* the song will stop playing, and the normal logic will move */
    /* to the next song and continue playing */
    playerData->stopNextsongFlag = STOP_NEXTSONG;
    playerData->stopPlaying = true;
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
    } else {
      playerData->gap = playerData->priorGap;
    }

    /* tell main to go to the next song, no history */
    connSendMessage (playerData->conn, ROUTE_MAIN, MSG_PLAYBACK_FINISH, "0");
  }
  logProcEnd (LOG_PROC, "playerNextSong", "");
}

static void
playerPauseAtEnd (playerdata_t *playerData)
{
  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  logProcBegin (LOG_PROC, "playerPauseAtEnd");

  playerData->pauseAtEnd = playerData->pauseAtEnd ? false : true;
  playerSendPauseAtEndState (playerData);
  logProcEnd (LOG_PROC, "playerPauseAtEnd", "");
}

static void
playerSendPauseAtEndState (playerdata_t *playerData)
{
  char    tbuff [20];

  logProcBegin (LOG_PROC, "playerSendPauseAtEndState");

  snprintf (tbuff, sizeof (tbuff), "%d", playerData->pauseAtEnd);
  connSendMessage (playerData->conn, ROUTE_PLAYERUI,
      MSG_PLAY_PAUSEATEND_STATE, tbuff);
  connSendMessage (playerData->conn, ROUTE_MANAGEUI,
      MSG_PLAY_PAUSEATEND_STATE, tbuff);
  logProcEnd (LOG_PROC, "playerSendPauseAtEndState", "");
}

static void
playerFade (playerdata_t *playerData)
{
  plistate_t plistate = pliState (playerData->pli);

  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  logProcBegin (LOG_PROC, "playerFade");

  if (playerData->inFadeOut) {
    logProcEnd (LOG_PROC, "playerFade", "in-fade-out");
    return;
  }

  if (playerData->fadeoutTime == 0) {
    playerCheckSystemVolume (playerData);
    playerData->stopPlaying = true;
  }

  if (playerData->fadeoutTime > 0 &&
      plistate == PLI_STATE_PLAYING) {
    logMsg (LOG_DBG, LOG_BASIC, "fade");
    playerCheckSystemVolume (playerData);
    playerStartFadeOut (playerData);
    mstimeset (&playerData->playTimeCheck, playerData->fadeoutTime - 500);
    mstimeset (&playerData->playEndCheck, playerData->fadeoutTime);
  }
  logProcEnd (LOG_PROC, "playerFade", "");
}

static void
playerSpeed (playerdata_t *playerData, char *trate)
{
  double rate;

  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  logProcBegin (LOG_PROC, "playerSpeed");

  if (playerData->playerState == PL_STATE_PLAYING) {
    rate = atof (trate);
    pliRate (playerData->pli, (ssize_t) rate);
    playerData->currentSpeed = (ssize_t) rate;
  }
  logProcEnd (LOG_PROC, "playerSpeed", "");
}

static void
playerSeek (playerdata_t *playerData, ssize_t reqpos)
{
  double        drate;
  double        dpos;
  ssize_t       nreqpos;
  ssize_t       seekpos;
  prepqueue_t   *pq = playerData->currentSong;

  if (pq == NULL) {
    return;
  }

  logProcBegin (LOG_PROC, "playerSeek");

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
  logProcEnd (LOG_PROC, "playerSeek", "");
}

static void
playerStop (playerdata_t *playerData)
{
  plistate_t plistate = pliState (playerData->pli);

  logProcBegin (LOG_PROC, "playerStop");

  if (plistate == PLI_STATE_PLAYING ||
      plistate == PLI_STATE_PAUSED) {
    pliStop (playerData->pli);
  }
  logProcEnd (LOG_PROC, "playerStop", "");
}

static void
playerVolumeSet (playerdata_t *playerData, char *tvol)
{
  int       newvol;
  int       voldiff;


  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  logProcBegin (LOG_PROC, "playerVolumeSet");

  newvol = (int) atol (tvol);
  newvol = playerLimitVolume (newvol);
  voldiff = newvol - playerData->currentVolume;
  playerData->realVolume += voldiff;
  playerData->realVolume = playerLimitVolume (playerData->realVolume);
  playerData->currentVolume += voldiff;
  playerData->currentVolume = playerLimitVolume (playerData->currentVolume);
  volumeSet (playerData->volume, playerData->currentSink, playerData->realVolume);
  playerData->actualVolume = playerData->realVolume;
  logProcEnd (LOG_PROC, "playerVolumeSet", "");
}

static void
playerVolumeMute (playerdata_t *playerData)
{
  if (! progstateIsRunning (playerData->progstate)) {
    return;
  }

  logProcBegin (LOG_PROC, "playerVolumeMute");

  playerData->mute = playerData->mute ? false : true;
  if (playerData->mute) {
    volumeSet (playerData->volume, playerData->currentSink, 0);
    playerData->actualVolume = 0;
  } else {
    volumeSet (playerData->volume, playerData->currentSink, playerData->realVolume);
    playerData->actualVolume = playerData->realVolume;
  }
  logProcEnd (LOG_PROC, "playerVolumeMute", "");
}

static void
playerPrepQueueFree (void *data)
{
  prepqueue_t     *pq = data;

  logProcBegin (LOG_PROC, "playerPrepQueueFree");

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
  logProcEnd (LOG_PROC, "playerPrepQueueFree", "");
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
  if (sinkname != NULL) {
    /* the sink list is not ordered */
    found = 0;
    for (size_t i = 0; i < playerData->sinklist.count; ++i) {
      if (strcmp (sinkname, playerData->sinklist.sinklist [i].name) == 0) {
        found = 1;
        idx = (int) i;
        break;
      }
    }
  }

  if (found && idx >= 0) {
    playerData->currentSink = playerData->sinklist.sinklist [idx].name;
    logMsg (LOG_DBG, LOG_IMPORTANT, "audio sink set to %s", playerData->sinklist.sinklist [idx].description);
  } else {
    playerData->currentSink = playerData->defaultSink;
    logMsg (LOG_DBG, LOG_IMPORTANT, "audio sink set to default");
  }
  logProcEnd (LOG_PROC, "playerSetAudioSink", "");
}

static void
playerInitSinklist (playerdata_t *playerData)
{
  int count;

  logProcBegin (LOG_PROC, "playerInitSinklist");

  volumeFreeSinkList (&playerData->sinklist);

  playerData->sinklist.defname = "";
  playerData->sinklist.count = 0;
  playerData->sinklist.sinklist = NULL;
  playerData->defaultSink = "";
  playerData->currentSink = "";

  if (volumeHaveSinkList (playerData->volume)) {
    count = 0;
    while (volumeGetSinkList (playerData->volume, "", &playerData->sinklist) != 0 &&
        count < 20) {
      mssleep (100);
      ++count;
    }
  }
  if (*playerData->sinklist.defname) {
    playerData->defaultSink = playerData->sinklist.defname;
  } else {
    playerData->defaultSink = "default";
  }
  logProcEnd (LOG_PROC, "playerInitSinklist", "");
}

static void
playerFadeVolSet (playerdata_t *playerData)
{
  double  findex = calcFadeIndex (playerData);
  int     newvol;
  int     ts;

  logProcBegin (LOG_PROC, "playerFadeVolSet");

  newvol = (int) round ((double) playerData->realVolume * findex);

  if (newvol > playerData->realVolume) {
    newvol = playerData->realVolume;
  }
  if (! playerData->mute) {
    volumeSet (playerData->volume, playerData->currentSink, newvol);
    playerData->actualVolume = newvol;
  }
  logMsg (LOG_DBG, LOG_MAIN, "fade set volume: %d count:%d",
      newvol, playerData->fadeCount);
  if (playerData->inFadeOut) {
    logMsg (LOG_DBG, LOG_MAIN, "   time %d",
        mstimeend (&playerData->playEndCheck));
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
      logMsg (LOG_DBG, LOG_MAIN, "fade-out done volume: %d time: %zd", 0, mstimeend (&playerData->playEndCheck));
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
  logProcEnd (LOG_PROC, "playerFadeVolSet", "");
}

static double
calcFadeIndex (playerdata_t *playerData)
{
  double findex = 0.0;
  double index = (double) playerData->fadeCount;
  double range = (double) playerData->fadeSamples;

  logProcBegin (LOG_PROC, "calcFadeIndex");

  findex = fmax(0.0, fmin (1.0, index / range));

  switch (playerData->fadeType) {
    case FADETYPE_QUARTER_SINE: {
      findex = sin (findex * M_PI / 2.0);
      break;
    }
    case FADETYPE_TRIANGLE: {
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
  logProcEnd (LOG_PROC, "calcFadeIndex", "");
  return findex;
}

static void
playerStartFadeOut (playerdata_t *playerData)
{
  ssize_t       tm;
  prepqueue_t   *pq = playerData->currentSong;

  logProcBegin (LOG_PROC, "playerStartFadeOut");
  playerData->inFade = true;
  playerData->inFadeOut = true;
  tm = pq->dur - playerCalcPlayedTime (playerData);
  tm = tm < playerData->fadeoutTime ? tm : playerData->fadeoutTime;
  playerData->fadeSamples = tm / FADEOUT_TIMESLICE;
  playerData->fadeCount = playerData->fadeSamples;
  logMsg (LOG_DBG, LOG_MAIN, "fade: samples: %d", playerData->fadeCount);
  playerData->fadeTimeStart = mstime ();
  playerFadeVolSet (playerData);
  playerSetPlayerState (playerData, PL_STATE_IN_FADEOUT);
  logProcEnd (LOG_PROC, "playerStartFadeOut", "");
}

static void
playerSetCheckTimes (playerdata_t *playerData, prepqueue_t *pq)
{
  ssize_t newdur;

  logProcBegin (LOG_PROC, "playerSetCheckTimes");

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
  logProcEnd (LOG_PROC, "playerSetCheckTimes", "");
}

static void
playerSetPlayerState (playerdata_t *playerData, playerstate_t pstate)
{
  char    tbuff [20];

  logProcBegin (LOG_PROC, "playerSetPlayerState");

  playerData->playerState = pstate;
  logMsg (LOG_DBG, LOG_BASIC, "pl-state: %d/%s",
      playerData->playerState, plstateDebugText (playerData->playerState));
  snprintf (tbuff, sizeof (tbuff), "%d", playerData->playerState);
  connSendMessage (playerData->conn, ROUTE_MAIN,
      MSG_PLAYER_STATE, tbuff);
  connSendMessage (playerData->conn, ROUTE_PLAYERUI,
      MSG_PLAYER_STATE, tbuff);
  connSendMessage (playerData->conn, ROUTE_MANAGEUI,
      MSG_PLAYER_STATE, tbuff);
  /* any time there is a change of player state, send the status */
  playerSendStatus (playerData);
  logProcEnd (LOG_PROC, "playerSetPlayerState", "");
}


static void
playerSendStatus (playerdata_t *playerData)
{
  char        rbuff [3096];
  prepqueue_t *pq = playerData->currentSong;
  ssize_t     tm;
  ssize_t     dur;

  logProcBegin (LOG_PROC, "playerSendStatus");

  if (playerData->playerState == playerData->lastPlayerState &&
      playerData->playerState != PL_STATE_PLAYING &&
      playerData->playerState != PL_STATE_IN_FADEOUT) {
    logProcEnd (LOG_PROC, "playerSendStatus", "no-state-chg");
    return;
  }

  playerData->lastPlayerState = playerData->playerState;

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

  snprintf (rbuff, sizeof (rbuff), "%d%c%d%c%d%c%d%c%zd%c%zd",
      playerData->repeat, MSG_ARGS_RS,
      playerData->pauseAtEnd, MSG_ARGS_RS,
      playerData->currentVolume, MSG_ARGS_RS,
      playerData->currentSpeed, MSG_ARGS_RS,
      tm, MSG_ARGS_RS,
      dur);

  connSendMessage (playerData->conn, ROUTE_MAIN,
      MSG_PLAYER_STATUS_DATA, rbuff);
  /* four times a second... */
  mstimeset (&playerData->statusCheck, 250);
  logProcEnd (LOG_PROC, "playerSendStatus", "");
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
  if (playerData->playerState == PL_STATE_PAUSED ||
      playerData->playerState == PL_STATE_PLAYING ||
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
  logMsg (LOG_DBG, LOG_MAIN, "Original system volume: %d", playerData->originalSystemVolume);

  count = volregSave (playerData->currentSink, playerData->originalSystemVolume);
  if (count > 1 || bdj3flag) {
    playerData->currentVolume = playerData->originalSystemVolume;
  } else {
    playerData->currentVolume = (int) bdjoptGetNum (OPT_P_DEFAULTVOLUME);
  }
  playerData->realVolume = playerData->currentVolume;

  volumeSet (playerData->volume, playerData->currentSink, playerData->realVolume);
  playerData->actualVolume = playerData->realVolume;
  logMsg (LOG_DBG, LOG_MAIN, "set volume: %d", playerData->realVolume);
}

static void
playerFreePlayRequest (void *tpreq)
{
  playrequest_t *preq = tpreq;

  if (preq != NULL) {
    if (preq->songname != NULL) {
      free (preq->songname);
    }
    free (preq);
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
      "speed%c%d%c"
      "playtimeplayed%c%zd%c"
      "pauseatend%c%d%c"
      "repeat%c%d%c"
      "defaultsink%c%s%c"
      "currentsink%c%s%c"
      "prepqueuecount%c%ld",
      MSG_ARGS_RS, plstateDebugText (playerData->playerState), MSG_ARGS_RS,
      MSG_ARGS_RS, plistateTxt [pliState (playerData->pli)], MSG_ARGS_RS,
      MSG_ARGS_RS, playerData->currentVolume, MSG_ARGS_RS,
      MSG_ARGS_RS, playerData->realVolume, MSG_ARGS_RS,
      MSG_ARGS_RS, playerData->actualVolume, MSG_ARGS_RS,
      MSG_ARGS_RS, playerData->currentSpeed, MSG_ARGS_RS,
      MSG_ARGS_RS, playerCalcPlayedTime (playerData), MSG_ARGS_RS,
      MSG_ARGS_RS, playerData->pauseAtEnd, MSG_ARGS_RS,
      MSG_ARGS_RS, playerData->repeat, MSG_ARGS_RS,
      MSG_ARGS_RS, playerData->defaultSink, MSG_ARGS_RS,
      MSG_ARGS_RS, playerData->currentSink, MSG_ARGS_RS,
      MSG_ARGS_RS, queueGetCount (playerData->prepQueue));
  connSendMessage (playerData->conn, routefrom, MSG_CHK_PLAYER_STATUS, tmp);
}

static void
playerChkPlayerSong (playerdata_t *playerData, int routefrom)
{
  prepqueue_t *pq = playerData->currentSong;
  char        tmp [2000];
  char        *sn = MSG_ARGS_EMPTY_STR;
  ssize_t     dur;


  dur = 0;
  if (pq != NULL &&
      (playerData->playerState == PL_STATE_LOADING ||
      playerData->playerState == PL_STATE_PLAYING ||
      playerData->playerState == PL_STATE_IN_FADEOUT)) {
    dur = pq->dur;
    sn = pq->songname;
  }

  snprintf (tmp, sizeof (tmp),
      "p-duration%c%ld%c"
      "p-songfn%c%s",
      MSG_ARGS_RS, dur, MSG_ARGS_RS,
      MSG_ARGS_RS, sn);
  connSendMessage (playerData->conn, routefrom, MSG_CHK_PLAYER_SONG, tmp);
}
