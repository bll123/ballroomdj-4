/*
 * bdj4play
 *  Does the actual playback of the music by calling the api to the
 *  music player.   Handles volume changes, fades.
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

#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "datautil.h"
#include "fileop.h"
#include "list.h"
#include "lock.h"
#include "log.h"
#include "pathutil.h"
#include "pli.h"
#include "portability.h"
#include "process.h"
#include "queue.h"
#include "sock.h"
#include "sockh.h"
#include "sysvars.h"
#include "tmutil.h"
#include "volume.h"

#define PLAYER_LOCK_FN    "player"

typedef struct {
  char          *songfullpath;
  char          *songname;
  char          *tempname;
  ssize_t       dur;
  ssize_t       songstart;
  ssize_t       voladjperc;
  ssize_t       gap;
  ssize_t       announce;
} prepqueue_t;

typedef struct {
  Sock_t          mainSock;
  Sock_t          guiSock;
  programstate_t  programState;
  mstime_t        tmstart;
  long            globalCount;
  pli_t           *pli;
  volume_t        *volume;
  queue_t         *prepRequestQueue;
  queue_t         *prepQueue;
  prepqueue_t     *currentSong;
  queue_t         *playRequest;
  int             originalSystemVolume;
  int             realVolume;
  int             currentVolume;
  char            *defaultSink;
  char            *currentSink;
  volsinklist_t   sinklist;
  playerstate_t   playerState;
  time_t          playTimeStart;
  time_t          playTimeCheck;
  time_t          fadeTimeCheck;
  ssize_t         gap;
  time_t          gapFinishTime;
  ssize_t         fadeType;
  ssize_t         fadeinTime;
  ssize_t         fadeoutTime;
  long            fadeCount;
  long            fadeSamples;
  time_t          fadeTimeNext;
  bool            inFadeOut : 1;
  bool            inFadeIn : 1;
  bool            inFade : 1;
  bool            inGap : 1;
  bool            pauseAtEnd : 1;
  bool            stopPlaying : 1;
  bool            mainHandshake : 1;
} playerdata_t;

#define FADEIN_TIMESLICE      100
#define FADEOUT_TIMESLICE     300


static void     playerCheckSystemVolume (playerdata_t *playerData);
static int      playerProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static int      playerProcessing (void *udata);
static void     playerSongPrep (playerdata_t *playerData, char *sfname);
void            playerProcessPrepRequest (playerdata_t *playerData);
static void     playerSongPlay (playerdata_t *playerData, char *sfname);
static prepqueue_t * playerLocatePreppedSong (playerdata_t *playerData, char *sfname);
static void     songMakeTempName (playerdata_t *playerData,
                    char *in, char *out, size_t maxlen);
static void     playerPause (playerdata_t *playerData);
static void     playerPlay (playerdata_t *playerData);
static void     playerPauseAtEnd (playerdata_t *playerData, char *args);
static void     playerSendPauseAtEndState (playerdata_t *playerData);
static void     playerFade (playerdata_t *playerData);
static void     playerStop (playerdata_t *playerData);
static void     playerPrepQueueFree (void *);
static void     playerSigHandler (int sig);
static void     playerSetAudioSink (playerdata_t *playerData, char *sinkname);
static void     playerInitSinklist (playerdata_t *playerData);
static void     playerFadeVolSet (playerdata_t *playerData);
static double   calcFadeIndex (playerdata_t *playerData);
static void     playerStartFadeOut (playerdata_t *playerData);
static void     playerSetCheckTimes (playerdata_t *playerData, prepqueue_t *pq);
static void     playerSendPlayerState (playerdata_t *playerData);

static int      gKillReceived = 0;

int
main (int argc, char *argv[])
{
  playerdata_t    playerData;
  int             c = 0;
  int             rc = 0;
  int             option_index = 0;
  bdjloglvl_t     loglevel = LOG_IMPORTANT | LOG_MAIN;
  uint16_t        listenPort;

  static struct option bdj_options [] = {
    { "debug",      required_argument,  NULL,   'd' },
    { "profile",    required_argument,  NULL,   'p' },
    { "player",     no_argument,        NULL,   0 },
    { NULL,         0,                  NULL,   0 }
  };

  mstimestart (&playerData.tmstart);

  processCatchSignals (playerSigHandler);
  processSigChildDefault ();

  playerData.programState = STATE_INITIALIZING;
  playerData.mainSock = INVALID_SOCKET;
  playerData.guiSock = INVALID_SOCKET;
  playerData.pli = NULL;
  playerData.prepRequestQueue = queueAlloc (playerPrepQueueFree);
  playerData.prepQueue = queueAlloc (playerPrepQueueFree);
  playerData.playRequest = queueAlloc (free);
  playerData.globalCount = 1;
  playerData.playerState = PL_STATE_STOPPED;
  playerData.inFade = false;
  playerData.inFadeIn = false;
  playerData.inFadeOut = false;
  playerData.gapFinishTime = 0;
  playerData.inGap = false;
  playerData.pauseAtEnd = false;
  playerData.stopPlaying = false;
  playerData.mainHandshake = false;
  playerData.fadeCount = 0;
  playerData.fadeSamples = 1;
  playerData.playTimeCheck = 0;
  playerData.fadeTimeCheck = 0;
  playerData.currentSong = NULL;

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

  logStartAppend ("bdj4player", "p", loglevel);
  logMsg (LOG_SESS, LOG_IMPORTANT, "Using profile %ld", lsysvars [SVL_BDJIDX]);

  rc = lockAcquire (PLAYER_LOCK_FN, DATAUTIL_MP_USEIDX);
  if (rc < 0) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: player: unable to acquire lock: profile: %zd", lsysvars [SVL_BDJIDX]);
    logMsg (LOG_SESS, LOG_IMPORTANT, "ERR: player: unable to acquire lock: profile: %zd", lsysvars [SVL_BDJIDX]);
    logEnd ();
    exit (0);
  }

  bdjvarsInit ();
  bdjoptInit ();

  playerData.fadeType = bdjoptGetNum (OPT_P_FADETYPE);
  playerData.fadeinTime = bdjoptGetNum (OPT_P_FADEINTIME);
  playerData.fadeoutTime = bdjoptGetNum (OPT_P_FADEOUTTIME);

  playerData.sinklist.defname = "";
  playerData.sinklist.count = 0;
  playerData.sinklist.sinklist = NULL;
  playerData.defaultSink = "";
  playerData.currentSink = "";

  playerData.volume = volumeInit ();

  playerInitSinklist (&playerData);
    /* sets the current sink */
  playerSetAudioSink (&playerData, bdjoptGetData (OPT_M_AUDIOSINK));

  playerData.originalSystemVolume =
      volumeGet (playerData.volume, playerData.currentSink);
  logMsg (LOG_DBG, LOG_VOLUME, "Original system volume: %d", playerData.originalSystemVolume);
  playerData.realVolume = (int) bdjoptGetNum (OPT_P_DEFAULTVOLUME);
  playerData.currentVolume = playerData.realVolume;
  volumeSet (playerData.volume, playerData.currentSink, playerData.realVolume);
  logMsg (LOG_DBG, LOG_VOLUME, "set volume: %d", playerData.realVolume);

  if (playerData.sinklist.sinklist != NULL) {
    for (size_t i = 0; i < playerData.sinklist.count; ++i) {
      logMsg (LOG_DBG, LOG_BASIC, "%d %3d %s %s",
               playerData.sinklist.sinklist [i].defaultFlag,
               playerData.sinklist.sinklist [i].idxNumber,
               playerData.sinklist.sinklist [i].name,
               playerData.sinklist.sinklist [i].description);
    }
  }

  playerData.pli = pliInit ();

  listenPort = bdjvarsl [BDJVL_PLAYER_PORT];
  sockhMainLoop (listenPort, playerProcessMsg, playerProcessing, &playerData);

  if (playerData.pli != NULL) {
    pliStop (playerData.pli);
    pliClose (playerData.pli);
    pliFree (playerData.pli);
  }

  volumeSet (playerData.volume, playerData.currentSink, playerData.originalSystemVolume);
  logMsg (LOG_DBG, LOG_VOLUME, "set to orig volume: %d", playerData.originalSystemVolume);
  playerData.defaultSink = "";
  playerData.currentSink = "";
  volumeFreeSinkList (&playerData.sinklist);
  volumeFree (playerData.volume);

  if (playerData.currentSong != NULL) {
    playerPrepQueueFree (playerData.currentSong);
  }
  if (playerData.prepQueue != NULL) {
    queueFree (playerData.prepQueue);
  }
  if (playerData.prepRequestQueue != NULL) {
    queueFree (playerData.prepRequestQueue);
  }
  if (playerData.playRequest != NULL) {
    queueFree (playerData.playRequest);
  }
  logEnd ();
  bdjoptFree ();
  bdjvarsCleanup ();
  playerData.programState = STATE_NOT_RUNNING;
  lockRelease (PLAYER_LOCK_FN, DATAUTIL_MP_USEIDX);
  return 0;
}

/* internal routines */

static int
playerProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  playerdata_t      *playerData;

  logProcBegin (LOG_PROC, "playerProcessMsg");
  playerData = (playerdata_t *) udata;

  logMsg (LOG_DBG, LOG_MSGS, "got: from %d route: %d msg:%d args:%s",
      routefrom, route, msg, args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_PLAYER: {
      logMsg (LOG_DBG, LOG_MSGS, "got: route-player");
      switch (msg) {
        case MSG_HANDSHAKE: {
          playerData->mainHandshake = true;
          break;
        }
        case MSG_PLAY_PAUSE: {
          logMsg (LOG_DBG, LOG_MSGS, "got: pause: progstate: %d", playerData->programState);
          if (playerData->programState == STATE_RUNNING) {
            playerPause (playerData);
          }
          break;
        }
        case MSG_PLAY_PLAY: {
          logMsg (LOG_DBG, LOG_MSGS, "got: play");
          playerPlay (playerData);
          break;
        }
        case MSG_PLAY_FADE: {
          logMsg (LOG_DBG, LOG_MSGS, "got: fade: progstate: %d", playerData->programState);
          if (playerData->programState == STATE_RUNNING) {
            playerFade (playerData);
          }
          break;
        }
        case MSG_PLAY_PAUSEATEND: {
          logMsg (LOG_DBG, LOG_MSGS, "got: pauseatend args:%s", args);
          playerPauseAtEnd (playerData, args);
          break;
        }
        case MSG_PLAY_STOP: {
          playerStop (playerData);
          playerData->playerState = PL_STATE_STOPPED;
          playerData->pauseAtEnd = false;
          playerSendPauseAtEndState (playerData);
          playerSendPlayerState (playerData);
          break;
        }
        case MSG_PLAYER_VOLSINK_SET: {
          playerSetAudioSink (playerData, args);
          break;
        }
        case MSG_SONG_PREP: {
          if (playerData->programState == STATE_RUNNING) {
            playerSongPrep (playerData, args);
          }
          break;
        }
        case MSG_SONG_PLAY: {
          if (playerData->programState == STATE_RUNNING) {
            playerSongPlay (playerData, args);
          }
          break;
        }
        case MSG_EXIT_REQUEST: {
          gKillReceived = 0;
          logMsg (LOG_DBG, LOG_MSGS, "got: req-exit");
          playerData->programState = STATE_CLOSING;
          sockhSendMessage (playerData->mainSock, ROUTE_PLAYER, ROUTE_MAIN,
              MSG_SOCKET_CLOSE, NULL);
          sockClose (playerData->mainSock);
          sockClose (playerData->guiSock);
          playerData->mainSock = INVALID_SOCKET;
          playerData->guiSock = INVALID_SOCKET;
          logProcEnd (LOG_PROC, "playerProcessMsg", "req-exit");
          return 1;
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
  playerdata_t      *playerData;

  playerData = (playerdata_t *) udata;

  if (playerData->programState == STATE_INITIALIZING) {
    playerData->programState = STATE_CONNECTING;
    logMsg (LOG_DBG, LOG_MAIN, "prog listening/connecting");
  }

  if (playerData->programState == STATE_CONNECTING &&
      playerData->mainSock == INVALID_SOCKET) {
    int       err;
    uint16_t  mainPort;
    uint16_t  guiPort;

    mainPort = bdjvarsl [BDJVL_MAIN_PORT];
    playerData->mainSock = sockConnect (mainPort, &err, 1000);
    if (playerData->mainSock != INVALID_SOCKET) {
      sockhSendMessage (playerData->mainSock, ROUTE_PLAYER, ROUTE_MAIN,
          MSG_HANDSHAKE, NULL);
      playerData->programState = STATE_WAIT_HANDSHAKE;
      logMsg (LOG_DBG, LOG_MAIN, "prog: wait for handshake");
    }

    guiPort = bdjvarsl [BDJVL_GUI_PORT];
    playerData->guiSock = sockConnect (guiPort, &err, 1000);
    if (playerData->guiSock != INVALID_SOCKET) {
      sockhSendMessage (playerData->guiSock, ROUTE_PLAYER, ROUTE_GUI,
          MSG_HANDSHAKE, NULL);
    }
  }

  if (playerData->programState == STATE_WAIT_HANDSHAKE) {
    if (playerData->mainHandshake) {
      playerData->programState = STATE_RUNNING;
      logMsg (LOG_DBG, LOG_MAIN, "prog: running");
      logMsg (LOG_SESS, LOG_IMPORTANT, "running: time-to-start: %ld ms", mstimeend (&playerData->tmstart));
    }
  }

  if (playerData->programState != STATE_RUNNING) {
      /* all of the processing that follows requires a running state */
    return gKillReceived;
  }

  if (playerData->inFade) {
    time_t            currTime = mstime ();

    if (currTime >= playerData->fadeTimeNext) {
      playerFadeVolSet (playerData);
    }
  }

  if (playerData->playerState == PL_STATE_STOPPED &&
      playerData->inGap) {
    time_t            currTime = mstime ();

    if (currTime >= playerData->gapFinishTime) {
      playerData->inGap = false;
      playerData->gapFinishTime = 0;
    }
  }

  if (playerData->playerState == PL_STATE_STOPPED &&
      ! playerData->inGap &&
      queueGetCount (playerData->playRequest) > 0) {
    prepqueue_t       *pq = NULL;
    char              *request;


    request = queuePop (playerData->playRequest);
    pq = playerLocatePreppedSong (playerData, request);
    if (pq == NULL) {
      return gKillReceived;
    }

    playerData->currentSong = pq;
    if (pq->announce == PREP_SONG) {
      queueIterateRemoveNode (playerData->prepQueue);
    }

    logMsg (LOG_DBG, LOG_BASIC, "play: %s", pq->tempname);
    if (pq->announce == PREP_ANNOUNCE || playerData->fadeinTime == 0) {
      volumeSet (playerData->volume, playerData->currentSink, playerData->realVolume);
      logMsg (LOG_DBG, LOG_VOLUME, "no fade-in set volume: %d", playerData->realVolume);
    }
    pliMediaSetup (playerData->pli, pq->tempname);
    pliStartPlayback (playerData->pli, pq->songstart);
    playerData->playerState = PL_STATE_LOADING;
    playerSendPlayerState (playerData);
    free (request);

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
      playerSetCheckTimes (playerData, pq);

      if (pq->announce == PREP_SONG && playerData->fadeinTime > 0) {
        playerData->inFade = true;
        playerData->inFadeIn = true;
        playerData->fadeCount = 1;
        playerData->fadeSamples = playerData->fadeinTime / FADEIN_TIMESLICE + 1;
        playerFadeVolSet (playerData);
      }

      logMsg (LOG_DBG, LOG_BASIC, "pl state: playing");
      playerData->playerState = PL_STATE_PLAYING;
      playerSendPlayerState (playerData);
    } else {
      /* ### FIX: need to process this; stopped/ended/error */
      ;
    }
  }

  if (playerData->playerState == PL_STATE_PLAYING) {
    prepqueue_t       *pq = playerData->currentSong;
    time_t            currTime = mstime ();

    if (playerData->fadeoutTime > 0 &&
        ! playerData->inFade &&
        currTime >= playerData->fadeTimeCheck) {

        /* before going into the fade, check the system volume */
        /* and see if the user changed it */
      logMsg (LOG_DBG, LOG_VOLUME, "check sysvol: before fade");
      playerCheckSystemVolume (playerData);
      playerStartFadeOut (playerData);
    }

    if (playerData->stopPlaying ||
        currTime >= playerData->playTimeCheck) {
      plistate_t plistate = pliState (playerData->pli);
      ssize_t plidur = pliGetDuration (playerData->pli);
      ssize_t plitm = pliGetTime (playerData->pli);

      if (plistate == PLI_STATE_STOPPED ||
          plistate == PLI_STATE_ENDED ||
          plistate == PLI_STATE_ERROR ||
          playerData->stopPlaying ||
          plitm >= plidur ||
          plitm >= pq->dur) {

          /* stop any fade */
        playerData->inFade = false;
        playerData->inFadeOut = false;
        playerData->inFadeIn = false;
        playerData->stopPlaying = false;

        logMsg (LOG_DBG, LOG_BASIC, "actual play time: %zd", mstime() - playerData->playTimeStart);
        playerStop (playerData);
        playerData->playerState = PL_STATE_STOPPED;
        playerSendPlayerState (playerData);
        logMsg (LOG_DBG, LOG_BASIC, "pl state: stopped");
        if (pq->announce == PREP_SONG) {
          if (playerData->pauseAtEnd) {
            playerData->pauseAtEnd = false;
            playerSendPauseAtEndState (playerData);
              /* let the main know we're done with this song. */
            sockhSendMessage (playerData->mainSock, ROUTE_PLAYER, ROUTE_MAIN,
                MSG_PLAYBACK_STOP, NULL);
          } else {
              /* done, go on to next song */
            sockhSendMessage (playerData->mainSock, ROUTE_PLAYER, ROUTE_MAIN,
                MSG_PLAYBACK_FINISH, NULL);
          }
        }
        if (pq->announce == PREP_SONG) {
          playerPrepQueueFree (playerData->currentSong);
          playerData->currentSong = NULL;
        }

        if (playerData->fadeoutTime == 0) {
          logMsg (LOG_DBG, LOG_VOLUME, "check sysvol: no-fade-out");
          playerCheckSystemVolume (playerData);
        }

        if (playerData->gap > 0) {
          volumeSet (playerData->volume, playerData->currentSink, 0);
          logMsg (LOG_DBG, LOG_VOLUME, "gap set volume: %d", 0);
          playerData->inGap = true;
          playerData->gapFinishTime = mstime () + playerData->gap;
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

  return gKillReceived;
}

/* internal routines - song handling */

static void
playerCheckSystemVolume (playerdata_t *playerData)
{
  int         tvol;

  tvol = volumeGet (playerData->volume, playerData->currentSink);
  logMsg (LOG_DBG, LOG_VOLUME, "get volume: %d", tvol);
  if (tvol != playerData->realVolume) {
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
  npq->voladjperc = atol (aptr);
  logMsg (LOG_DBG, LOG_MAIN, "prep voladjperc: %zd", npq->voladjperc);

  aptr = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  npq->gap = atol (aptr);
  logMsg (LOG_DBG, LOG_MAIN, "prep gap: %zd", npq->gap);

  aptr = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  npq->announce= atol (aptr);
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
  fileopLinkCopy (npq->songfullpath, npq->tempname);
  if (! fileopExists (npq->tempname)) {
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


  logProcBegin (LOG_PROC, "playerSongPlay");
  logMsg (LOG_DBG, LOG_BASIC, "play request: %s", sfname);
  pq = playerLocatePreppedSong (playerData, sfname);
  if (pq == NULL) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: not prepped: %s", sfname);
    logProcEnd (LOG_PROC, "playerSongPlay", "not-prepped");
    return;
  }
  if (! fileopExists (pq->tempname)) {
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
    queueStartIterator (playerData->prepQueue);
    pq = queueIterateData (playerData->prepQueue);
    while (pq != NULL) {
      if (strcmp (sfname, pq->songname) == 0) {
        found = 1;
        break;
      }
      pq = queueIterateData (playerData->prepQueue);
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
  for (char *p = pi->filename; *p && idx < maxlen && idx < pi->flen; ++p) {
    if ((isascii (*p) && isalnum (*p)) ||
        *p == '.' || *p == '-' || *p == '_') {
      tnm [idx++] = *p;
    }
  }
  tnm [idx] = '\0';
  pathInfoFree (pi);

    /* the profile index so we don't stomp on other bdj instances   */
    /* the global count so we don't stomp on ourselves              */
  snprintf (out, maxlen, "tmp/%02ld-%03ld-%s", lsysvars [SVL_BDJIDX],
      playerData->globalCount, tnm);
  ++playerData->globalCount;
}

/* internal routines - player handling */

static void
playerPause (playerdata_t *playerData)
{
  plistate_t plistate = pliState (playerData->pli);

  if (playerData->inFadeOut) {
    playerData->pauseAtEnd = true;
  } else if (plistate == PLI_STATE_PLAYING) {
    playerData->playerState = PL_STATE_PAUSED;
    playerSendPlayerState (playerData);
    pliPause (playerData->pli);
    if (playerData->inFadeIn) {
      playerData->inFade = false;
      playerData->inFadeIn = false;
      volumeSet (playerData->volume, playerData->currentSink, playerData->realVolume);
    }
  }
}

static void
playerPlay (playerdata_t *playerData)
{
  plistate_t plistate = pliState (playerData->pli);

  if (plistate != PLI_STATE_PLAYING) {
    if (playerData->playerState == PL_STATE_PAUSED) {
        /* all of the check times must be reset */
      prepqueue_t       *pq = playerData->currentSong;
      playerSetCheckTimes (playerData, pq);
      playerData->playerState = PL_STATE_PLAYING;
      playerSendPlayerState (playerData);
    }
    pliPlay (playerData->pli);
  }
}

static void
playerPauseAtEnd (playerdata_t *playerData, char *args)
{
  if (playerData->programState != STATE_RUNNING) {
    return;
  }

  playerData->pauseAtEnd = (atol (args) > 0);
  playerSendPauseAtEndState (playerData);
}

static void
playerSendPauseAtEndState (playerdata_t *playerData)
{
  char    tbuff [20];

  snprintf (tbuff, sizeof (tbuff), "%d", playerData->pauseAtEnd);
  sockhSendMessage (playerData->guiSock, ROUTE_PLAYER, ROUTE_GUI,
      MSG_PLAY_PAUSEATEND_STATE, tbuff);
}

static void
playerFade (playerdata_t *playerData)
{
  plistate_t plistate = pliState (playerData->pli);

  if (playerData->inFadeOut) {
    return;
  }

  if (plistate == PLI_STATE_PLAYING) {
    if (! playerData->inFade) {
      logMsg (LOG_DBG, LOG_VOLUME, "check sysvol: before fade request");
      playerCheckSystemVolume (playerData);
    }
    playerStartFadeOut (playerData);
  }
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
  volumeFreeSinkList (&playerData->sinklist);

  logProcBegin (LOG_PROC, "playerInitSinklist");
  playerData->sinklist.defname = "";
  playerData->sinklist.count = 0;
  playerData->sinklist.sinklist = NULL;
  playerData->defaultSink = "";
  playerData->currentSink = "";

  volumeGetSinkList (playerData->volume, "", &playerData->sinklist);
  playerData->defaultSink = playerData->sinklist.defname;
  logProcEnd (LOG_PROC, "playerInitSinklist", "");
}

static void
playerFadeVolSet (playerdata_t *playerData)
{
  double  findex = calcFadeIndex (playerData);
  int     newvol;

  newvol = (int) round ((double) playerData->realVolume * findex);

  if (newvol > playerData->realVolume) {
    newvol = playerData->realVolume;
  }
  volumeSet (playerData->volume, playerData->currentSink, newvol);
  logMsg (LOG_DBG, LOG_VOLUME, "fade set volume: %d", newvol);
  if (playerData->inFadeIn) {
    ++playerData->fadeCount;
  }
  if (playerData->inFadeOut) {
    --playerData->fadeCount;
  }
  playerData->fadeTimeNext = mstime() +
      (playerData->inFadeOut ? FADEOUT_TIMESLICE : FADEIN_TIMESLICE);
  if (playerData->inFadeIn &&
      newvol >= playerData->realVolume) {
    playerData->inFade = false;
    playerData->inFadeIn = false;
  }
  if (playerData->inFadeOut && newvol <= 0) {
      /* leave inFade set to prevent race conditions in the main loop */
      /* the player stop condition will reset the inFade flag */
    playerData->inFadeOut = false;
    playerData->stopPlaying = true;
    volumeSet (playerData->volume, playerData->currentSink, 0);
    logMsg (LOG_DBG, LOG_VOLUME, "fade-out done volume: %d", 0);
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
  ssize_t       plidur;
  ssize_t       plitm;
  ssize_t       tm;

  playerData->inFade = true;
  playerData->inFadeOut = true;
  plidur = pliGetDuration (playerData->pli);
  plitm = pliGetTime (playerData->pli);
  tm = plidur - plitm;
  if (tm > playerData->fadeoutTime) {
    tm = playerData->fadeoutTime;
  }
  playerData->fadeSamples = tm / FADEOUT_TIMESLICE - 2;
  playerData->fadeCount = playerData->fadeSamples;
  playerFadeVolSet (playerData);
}


static void
playerSetCheckTimes (playerdata_t *playerData, prepqueue_t *pq)
{
  ssize_t plitm;
  ssize_t newdur;

  newdur = pq->dur;
  plitm = pliGetTime (playerData->pli);
  newdur = pq->dur - plitm;
  playerData->playTimeStart = mstime ();
  playerData->fadeTimeCheck = playerData->playTimeStart;
    /* want to start check for real finish 500 ms before end */
  playerData->playTimeCheck += newdur - 500;
  playerData->fadeTimeCheck += newdur;
  if (pq->announce == PREP_SONG && playerData->fadeoutTime > 0) {
    playerData->fadeTimeCheck -= playerData->fadeoutTime;
  }
}

static void
playerSendPlayerState (playerdata_t *playerData)
{
  char    tbuff [20];

  snprintf (tbuff, sizeof (tbuff), "%d", playerData->playerState);
  sockhSendMessage (playerData->mainSock, ROUTE_PLAYER, ROUTE_MAIN,
      MSG_PLAYER_STATE, tbuff);
  sockhSendMessage (playerData->guiSock, ROUTE_PLAYER, ROUTE_GUI,
      MSG_PLAYER_STATE, tbuff);
}
