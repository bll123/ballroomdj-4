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
} prepqueue_t;

typedef struct {
  Sock_t          mainSock;
  programstate_t  programState;
  mstime_t        tmstart;
  long            globalCount;
  pli_t           *pli;
  volume_t        *volume;
  queue_t         *prepRequestQueue;
  queue_t         *prepQueue;
  prepqueue_t     *currentSong;
  char            *playRequest;
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
  time_t          gapFinishTime;
  ssize_t         gap;
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
  bool            mainHandshake : 1;
} playerdata_t;

static int      playerProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static int      playerProcessing (void *udata);
static void     playerSongPrep (playerdata_t *playerData, char *sfname);
void            playerProcessPrepRequest (playerdata_t *playerData);
static void     playerSongPlay (playerdata_t *playerData, char *sfname);
static void     songMakeTempName (playerdata_t *playerData,
                    char *in, char *out, size_t maxlen);
static void     playerPause (playerdata_t *playerData);
static void     playerPlay (playerdata_t *playerData);
static void     playerStop (playerdata_t *playerData);
static void     playerPrepQueueFree (void *);
static void     playerSigHandler (int sig);
static void     playerSetAudioSink (playerdata_t *playerData, char *sinkname);
static void     playerInitSinklist (playerdata_t *playerData);
static void     playerFadeVolSet (playerdata_t *playerData);
static double   calcFadeIndex (playerdata_t *playerData);

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
  playerData.pli = NULL;
  playerData.prepRequestQueue = queueAlloc (playerPrepQueueFree);
  playerData.prepQueue = queueAlloc (playerPrepQueueFree);
  playerData.playRequest = NULL;
  playerData.globalCount = 1;
  playerData.playerState = PL_STATE_STOPPED;
  playerData.inFade = false;
  playerData.inFadeIn = false;
  playerData.inFadeOut = false;
  playerData.gapFinishTime = 0;
  playerData.inGap = false;
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

  playerData.gap = bdjoptGetNum (OPT_P_GAP);
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
  playerData.realVolume = (int) bdjoptGetNum (OPT_P_DEFAULTVOLUME);
  playerData.currentVolume = playerData.realVolume;
  volumeSet (playerData.volume, playerData.currentSink, playerData.realVolume);

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
          if (playerData->programState == STATE_RUNNING) {
            playerPause (playerData);
          }
          break;
        }
        case MSG_PLAY_PLAY: {
          playerPlay (playerData);
          break;
        }
        case MSG_PLAY_STOP: {
          playerStop (playerData);
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
          playerData->mainSock = INVALID_SOCKET;
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

    mainPort = bdjvarsl [BDJVL_MAIN_PORT];
    playerData->mainSock = sockConnect (mainPort, &err, 1000);
    if (playerData->mainSock != INVALID_SOCKET) {
      sockhSendMessage (playerData->mainSock, ROUTE_PLAYER, ROUTE_MAIN,
          MSG_HANDSHAKE, NULL);
      playerData->programState = STATE_WAIT_HANDSHAKE;
      logMsg (LOG_DBG, LOG_MAIN, "prog: wait for handshake");
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
      playerData->playRequest != NULL) {
    logMsg (LOG_DBG, LOG_BASIC, "play: %s", playerData->playRequest);
    if (playerData->fadeinTime == 0) {
      volumeSet (playerData->volume, playerData->currentSink, playerData->realVolume);
    }
    pliMediaSetup (playerData->pli, playerData->playRequest);
    pliStartPlayback (playerData->pli);
    playerData->playerState = PL_STATE_LOADING;
    free (playerData->playRequest);
    playerData->playRequest = NULL;

    logMsg (LOG_DBG, LOG_BASIC, "pl state: loading");
  }

  if (playerData->playerState == PL_STATE_LOADING) {
    prepqueue_t       *pq = playerData->currentSong;

    plistate_t plistate = pliState (playerData->pli);
    if (plistate == PLI_STATE_OPENING ||
        plistate == PLI_STATE_BUFFERING) {
      ;
    } else if (plistate == PLI_STATE_PLAYING) {
      ssize_t     maxdur = 0;

      if (pq->dur <= 1) {
        pq->dur = pliGetDuration (playerData->pli);
        logMsg (LOG_DBG, LOG_MAIN, "Replace duration with vlc data: %zd", pq->dur);
      }
      maxdur = bdjoptGetNum (OPT_P_MAXPLAYTIME);
      if (pq->dur > maxdur) {
        pq->dur = maxdur;
      }

      playerData->playTimeStart = mstime ();
        /* want to start check for real finish 500 ms before end */
      playerData->playTimeCheck = playerData->playTimeStart;
      playerData->fadeTimeCheck = playerData->playTimeCheck;
      playerData->playTimeCheck += pq->dur - 500;
      playerData->fadeTimeCheck += pq->dur;
      if (playerData->fadeoutTime > 0) {
        playerData->fadeTimeCheck -= playerData->fadeoutTime;
      }

      if (playerData->fadeinTime > 0) {
        playerData->inFade = true;
        playerData->inFadeIn = true;
        playerData->fadeCount = 1;
        playerData->fadeSamples = playerData->fadeinTime / 100 + 1;
        playerFadeVolSet (playerData);
      }

      logMsg (LOG_DBG, LOG_BASIC, "pl state: playing");
      sockhSendMessage (playerData->mainSock, ROUTE_PLAYER, ROUTE_MAIN,
          MSG_PLAYBACK_BEGIN, NULL);

      playerData->playerState = PL_STATE_PLAYING;
    } else {
      /* ### FIX: need to process this; stopped/ended/error */
      ;
    }
  }

  if (playerData->playerState == PL_STATE_PLAYING) {
    int               tvol;
    prepqueue_t       *pq = playerData->currentSong;
    time_t            currTime = mstime ();

    if (playerData->fadeoutTime > 0 &&
        ! playerData->inFade &&
        currTime >= playerData->fadeTimeCheck) {

        /* before going into the fade, check the system volume */
        /* and see if the user changed it */
      tvol = volumeGet (playerData->volume, playerData->currentSink);
      if (tvol != playerData->realVolume) {
        playerData->realVolume = tvol;
        playerData->currentVolume = tvol;
      }

      playerData->inFade = true;
      playerData->inFadeOut = true;
      playerData->fadeSamples = playerData->fadeoutTime / 100 + 1;
      playerData->fadeCount = playerData->fadeSamples;
      playerFadeVolSet (playerData);
      playerData->fadeTimeCheck = 0;
    }

    if (currTime >= playerData->playTimeCheck) {
      plistate_t plistate = pliState (playerData->pli);
      ssize_t plidur = pliGetDuration (playerData->pli);
      ssize_t pltm = pliGetTime (playerData->pli);

      if (plistate == PLI_STATE_STOPPED ||
          plistate == PLI_STATE_ENDED ||
          plistate == PLI_STATE_ERROR ||
          pltm >= plidur ||
          pltm >= pq->dur) {

          /* stop any fade */
        playerData->inFade = false;
        playerData->inFadeOut = false;
        playerData->inFadeIn = false;

        logMsg (LOG_DBG, LOG_BASIC, "actual play time: %zd", mstime() - playerData->playTimeStart);
        playerStop (playerData);
        playerData->playerState = PL_STATE_STOPPED;
        logMsg (LOG_DBG, LOG_BASIC, "pl state: stopped");
        playerPrepQueueFree (playerData->currentSong);
        playerData->currentSong = NULL;
        sockhSendMessage (playerData->mainSock, ROUTE_PLAYER, ROUTE_MAIN,
            MSG_PLAYBACK_FINISH, NULL);

        if (playerData->gap > 0) {
          volumeSet (playerData->volume, playerData->currentSink, 0);
          playerData->inGap = true;
          playerData->gapFinishTime = mstime () + playerData->gap;
        }
      }
    }
  }

    /* only process the prep requests when the player isn't doing much  */
    /* windows must do a physical copy, and this may take a bit of time */
  if ((playerData->playerState == PL_STATE_PLAYING ||
       playerData->playerState == PL_STATE_STOPPED) &&
      ! playerData->inGap &&
      ! playerData->inFade) {
    playerProcessPrepRequest (playerData);
  }

  return gKillReceived;
}

/* internal routines - song handling */

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
  char              stname [MAXPATHLEN];
  prepqueue_t       *pq = NULL;
  int               found = 0;
  int               count = 0;


  logProcBegin (LOG_PROC, "playerSongPlay");
  logMsg (LOG_DBG, LOG_BASIC, "play request: %s", sfname);
  found = 0;
  count = 0;
  while (! found && count < 2) {
    queueStartIterator (playerData->prepQueue);
    pq = queueIterateData (playerData->prepQueue);
    while (pq != NULL) {
      if (strcmp (sfname, pq->songname) == 0) {
        strlcpy (stname, pq->tempname, MAXPATHLEN);
        playerData->currentSong = pq;
        queueIterateRemoveNode (playerData->prepQueue);
        found = 1;
        break;
      }
      pq = queueIterateData (playerData->prepQueue);
    }
    if (! found) {
        /* the song must be prepped before playing */
        /* try again. */
      logMsg (LOG_DBG, LOG_IMPORTANT, "WARN: song %s not prepped; processing queue", sfname);
      playerSongPrep (playerData, sfname);
      playerProcessPrepRequest (playerData);
      ++count;
    }
  }
  if (! found) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: unable to locate song %s", sfname);
    logProcEnd (LOG_PROC, "playerSongPlay", "not-found");
    return;
  }

  if (! fileopExists (stname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: no file: %s", stname);
    logProcEnd (LOG_PROC, "playerSongPlay", "no-file");
    return;
  }

  playerData->playRequest = strdup (stname);
  logProcEnd (LOG_PROC, "playerSongPlay", "");
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
  snprintf (out, maxlen, "tmp/%ld-%ld-%s", lsysvars [SVL_BDJIDX],
      playerData->globalCount, tnm);
  ++playerData->globalCount;
}

/* internal routines - player handling */

static void
playerPause (playerdata_t *playerData)
{
  pliPause (playerData->pli);
}

static void
playerPlay (playerdata_t *playerData)
{
  pliPlay (playerData->pli);
}

static void
playerStop (playerdata_t *playerData)
{
  pliStop (playerData->pli);
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
  if (playerData->inFadeIn) {
    ++playerData->fadeCount;
  }
  if (playerData->inFadeOut) {
    --playerData->fadeCount;
  }
  playerData->fadeTimeNext = mstime() + 100;
  if (playerData->inFadeIn &&
      newvol >= playerData->realVolume) {
    playerData->inFade = false;
    playerData->inFadeIn = false;
  }
  if (playerData->inFadeOut && newvol <= 0) {
    playerData->inFade = false;
    playerData->inFadeOut = false;
    volumeSet (playerData->volume, playerData->currentSink, 0);
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
