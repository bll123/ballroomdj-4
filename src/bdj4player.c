/*
 * bdj4play
 *  Does the actual playback of the music by calling the api to the
 *  music player.   Handles volume changes, fades.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <ctype.h>
#include <unistd.h>

#include "bdjmsg.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "fileop.h"
#include "list.h"
#include "log.h"
#include "sock.h"
#include "sockh.h"
#include "sysvars.h"
#include "portability.h"
#include "pathutil.h"
#include "queue.h"
#include "pli.h"
#include "process.h"
#include "volume.h"
#include "bdjopt.h"
#include "tmutil.h"

typedef struct {
  char          *songname;
  char          *tempname;
} prepqueue_t;

typedef struct {
  mtime_t         tmstart;
  programstate_t  programState;
  Sock_t          mainSock;
  plidata_t       *pliData;
  queue_t         *prepQueue;
  long            globalCount;
  int             originalSystemVolume;
  int             realVolume;
  int             currentVolume;
  char            *defaultSink;
  char            *currentSink;
  volsinklist_t   sinklist;
} playerdata_t;

static int      playerProcessMsg (long routefrom, long route, long msg,
                    char *args, void *udata);
static int      playerProcessing (void *udata);
static void     songPrep (playerdata_t *playerData, char *sfname);
static void     songPlay (playerdata_t *playerData, char *sfname);
static void     songMakeTempName (playerdata_t *playerData,
                    char *in, char *out, size_t maxlen);
static void     playerPause (playerdata_t *playerData);
static void     playerPlay (playerdata_t *playerData);
static void     playerStop (playerdata_t *playerData);
static void     playerClose (playerdata_t *playerData);
static void     playerPrepQueueFree (void *);
static void     playerCleanPrepSongs (playerdata_t *playerData);
static void     playerSigHandler (int sig);
static void     playerSetAudioSink (playerdata_t *playerData, char *sinkname);
static void     playerInitSinklist (playerdata_t *playerData);

static int      gKillReceived = 0;

int
main (int argc, char *argv[])
{
  playerdata_t    playerData;
  int             c = 0;
  int             option_index = 0;
  int             loglevel = LOG_IMPORTANT | LOG_MAIN;

  static struct option bdj_options [] = {
    { "debug",     no_argument,         NULL,   'd' },
    { "player",     no_argument,        NULL,   0 },
    { "profile",    required_argument,  NULL,   'p' },
    { NULL,         0,                  NULL,   0 }
  };

  mtimestart (&playerData.tmstart);

  playerData.programState = STATE_INITIALIZING;
  playerData.mainSock = INVALID_SOCKET;
  playerData.pliData = NULL;
  playerData.prepQueue = queueAlloc (playerPrepQueueFree);
  playerData.globalCount = 1;

  processCatchSignals (playerSigHandler);

  sysvarsInit (argv[0]);

  while ((c = getopt_long (argc, argv, "", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'd': {
        if (optarg) {
          loglevel = atol (optarg);
        }
        break;
      }
      case 'p': {
        if (optarg) {
          sysvarSetLong (SVL_BDJIDX, atol (optarg));
        }
        break;
      }
      default: {
        break;
      }
    }
  }

  bdjvarsInit ();

  logStartAppend ("bdj4player", "p", loglevel);
  logMsg (LOG_SESS, LOG_IMPORTANT, "Using profile %ld", lsysvars [SVL_BDJIDX]);

  bdjoptInit ();

  playerData.sinklist.defname = "";
  playerData.sinklist.count = 0;
  playerData.sinklist.sinklist = NULL;
  playerData.defaultSink = "";
  playerData.currentSink = "";

  playerInitSinklist (&playerData);
    /* sets the current sink */
  playerSetAudioSink (&playerData, bdjoptGetData (OPT_M_AUDIOSINK));

  playerData.originalSystemVolume = volumeGet (playerData.currentSink);
  playerData.realVolume = (int) bdjoptGetLong (OPT_P_DEFAULTVOLUME);
  volumeSet (playerData.currentSink, playerData.realVolume);

  if (playerData.sinklist.sinklist != NULL) {
    for (size_t i = 0; i < playerData.sinklist.count; ++i) {
      logMsg (LOG_DBG, LOG_BASIC, "%d %3d %s %s\n",
               playerData.sinklist.sinklist [i].defaultFlag,
               playerData.sinklist.sinklist [i].idxNumber,
               playerData.sinklist.sinklist [i].name,
               playerData.sinklist.sinklist [i].description);
    }
  }

  playerData.pliData = pliInit ();
  assert (playerData.pliData != NULL);

  uint16_t listenPort = bdjvarsl [BDJVL_PLAYER_PORT];
  sockhMainLoop (listenPort, playerProcessMsg, playerProcessing, &playerData);

  if (playerData.pliData != NULL) {
    playerStop (&playerData);
    playerClose (&playerData);
    pliFree (playerData.pliData);
  }

  volumeSet (playerData.currentSink, playerData.originalSystemVolume);
  playerData.defaultSink = "";
  playerData.currentSink = "";
  volumeFreeSinkList (&playerData.sinklist);

  if (playerData.prepQueue != NULL) {
    playerCleanPrepSongs (&playerData);
    queueFree (playerData.prepQueue);
  }
  logEnd ();
  bdjoptFree ();
  bdjvarsCleanup ();
  playerData.programState = STATE_NOT_RUNNING;
  return 0;
}

/* internal routines */

static int
playerProcessMsg (long routefrom, long route, long msg, char *args, void *udata)
{
  playerdata_t      *playerData;

  logProcBegin (LOG_MAIN, "playerProcessMsg");
  playerData = (playerdata_t *) udata;

  logMsg (LOG_DBG, LOG_MAIN, "got: from %ld route: %ld msg:%ld args:%s",
      routefrom, route, msg, args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_PLAYER: {
      logMsg (LOG_DBG, LOG_BASIC, "got: route-player");
      switch (msg) {
        case MSG_ECHO_RESP: {
          if (routefrom == ROUTE_MAIN) {
            playerData->programState = STATE_RUNNING;
            logMsg (LOG_SESS, LOG_IMPORTANT, "running: time-to-start: %ld ms", mtimeend (&playerData->tmstart));
          }
          break;
        }
        case MSG_ECHO: {
          Sock_t      tsock;

          tsock = playerData->mainSock;
          sockhSendMessage (tsock, ROUTE_PLAYER, routefrom,
              MSG_ECHO_RESP, NULL);
          break;
        }
        case MSG_PLAYER_PAUSE: {
          playerPause (playerData);
          break;
        }
        case MSG_PLAYER_PLAY: {
          playerPlay (playerData);
          break;
        }
        case MSG_PLAYER_STOP: {
          playerStop (playerData);
          break;
        }
        case MSG_PLAYER_VOLSINK_SET: {
          playerSetAudioSink (playerData, args);
          break;
        }
        case MSG_SONG_PREP: {
          songPrep (playerData, args);
          break;
        }
        case MSG_SONG_PLAY: {
          songPlay (playerData, args);
          break;
        }
        case MSG_EXIT_REQUEST: {
          gKillReceived = 0;
          logMsg (LOG_DBG, LOG_BASIC, "got: req-exit");
          playerData->programState = STATE_CLOSING;
          sockhSendMessage (playerData->mainSock, ROUTE_PLAYER, ROUTE_MAIN,
              MSG_SOCKET_CLOSE, NULL);
          sockClose (playerData->mainSock);
          playerData->mainSock = INVALID_SOCKET;
          logProcEnd (LOG_BASIC, "playerProcessMsg", "req-exit");
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

  logProcEnd (LOG_BASIC, "playerProcessMsg", "");
  return 0;
}

static int
playerProcessing (void *udata)
{
  playerdata_t      *playerData;

  playerData = (playerdata_t *) udata;

  if (playerData->programState == STATE_INITIALIZING) {
    playerData->programState = STATE_CONNECTING;
    logMsg (LOG_DBG, LOG_MAIN, "listening/connecting");
  }

  if (playerData->programState == STATE_CONNECTING &&
      playerData->mainSock == INVALID_SOCKET) {
    int       err;

    uint16_t mainPort = bdjvarsl [BDJVL_MAIN_PORT];
    playerData->mainSock = sockConnect (mainPort, &err, 1000);
    if (playerData->mainSock != INVALID_SOCKET) {
      sockhSendMessage (playerData->mainSock, ROUTE_PLAYER, ROUTE_MAIN,
          MSG_ECHO, NULL);
      playerData->programState = STATE_WAIT_HANDSHAKE;
      logMsg (LOG_DBG, LOG_MAIN, "wait for handshake");
    }
  }

  return gKillReceived;
}

/* internal routines - song handling */

static void
songPrep (playerdata_t *playerData, char *sfname)
{
  char            tsfname [MAXPATHLEN];  // testing: remove later
  char            stname [MAXPATHLEN];
  prepqueue_t     *pq;

  logProcBegin (LOG_BASIC, "songPrep");
  if (! fileopExists (sfname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: no file: %s\n", sfname);
    logProcEnd (LOG_BASIC, "songPrep", "no-file");
    return;
  }

  pq = malloc (sizeof (prepqueue_t));
  assert (pq != NULL);
  pq->songname = strdup (sfname);
  pq->tempname = NULL;

  /* TODO: if we are in client/server mode, then need to request the song
   * from the server and save it
   */
  songMakeTempName (playerData, sfname, stname, sizeof (stname));
  if (*sfname != '/') {
    /* ### FIX: remove later: for testing */
    (void) ! getcwd (tsfname, MAXPATHLEN);
    strlcat (tsfname, "/", MAXPATHLEN);
    strlcat (tsfname, sfname, MAXPATHLEN);
  }

  /* VLC still cannot handle internationalized names.
   * I wonder how they handle them internally.
   * Symlinks work on Linux/Mac OS.
   */
  fileopDelete (stname);
  fileopLinkCopy (tsfname, stname);
  if (! fileopExists (stname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: file copy failed: %s\n", stname);
    logProcEnd (LOG_BASIC, "songPrep", "copy-failed");
    playerPrepQueueFree (pq);
    return;
  }
  pq->tempname = strdup (stname);
  queuePush (playerData->prepQueue, pq);

  // TODO : add the name to a list of prepared files.
  logProcEnd (LOG_BASIC, "songPrep", "");
}

static void
songPlay (playerdata_t *playerData, char *sfname)
{
  char              stname [MAXPATHLEN];
  prepqueue_t       *pq;
  int               found = 0;

  logProcBegin (LOG_BASIC, "songPlay");
  found = 0;
  queueStartIterator (playerData->prepQueue);
  pq = queueIterateData (playerData->prepQueue);
  while (pq != NULL) {
    if (strcmp (sfname, pq->songname) == 0) {
      strlcpy (stname, pq->tempname, MAXPATHLEN);
      queueIterateRemoveNode (playerData->prepQueue);
      found = 1;
      break;
    }
    pq = queueIterateData (playerData->prepQueue);
  }
  if (! found) {
    logProcEnd (LOG_BASIC, "songPlay", "not-found");
    return;
  }

  if (! fileopExists (stname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: no file: %s\n", stname);
    logProcEnd (LOG_BASIC, "songPlay", "no-file");
    return;
  }

  pliMediaSetup (playerData->pliData, stname);
  pliStartPlayback (playerData->pliData);
  logProcEnd (LOG_BASIC, "songPlay", "");
}

static void
songMakeTempName (playerdata_t *playerData, char *in, char *out, size_t maxlen)
{
  char        tnm [MAXPATHLEN];
  size_t      idx;
  pathinfo_t  *pi;

  pi = pathInfo (in);

  idx = 0;
  for (char *p = pi->filename; *p && idx < maxlen; ++p) {
    if ((isascii (*p) && isalnum (*p)) || *p == '.' ) {
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
  pliPause (playerData->pliData);
}

static void
playerPlay (playerdata_t *playerData)
{
  pliPlay (playerData->pliData);
}

static void
playerStop (playerdata_t *playerData)
{
  pliStop (playerData->pliData);
}

static void
playerClose (playerdata_t *playerData)
{
  pliClose (playerData->pliData);
}

static void
playerPrepQueueFree (void *data)
{
  prepqueue_t     *pq = data;

  if (pq != NULL) {
    if (pq->songname != NULL) {
      free (pq->songname);
    }
    if (pq->tempname != NULL) {
      free (pq->tempname);
    }
    free (pq);
  }
}

static void
playerCleanPrepSongs (playerdata_t *playerData)
{
  prepqueue_t     *pq = NULL;

  if (playerData->prepQueue != NULL) {
    queueStartIterator (playerData->prepQueue);
    pq = queueIterateData (playerData->prepQueue);
    while (pq != NULL) {
      if (pq->tempname != NULL) {
        fileopDelete (pq->tempname);
      }
      pq = queueIterateData (playerData->prepQueue);
    }
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

    /* the sink list is not ordered */
  found = 0;
  for (size_t i = 0; i < playerData->sinklist.count; ++i) {
    if (strcmp (sinkname, playerData->sinklist.sinklist [i].name) == 0) {
      found = 1;
      idx = i;
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
}

static void
playerInitSinklist (playerdata_t *playerData)
{
  volumeFreeSinkList (&playerData->sinklist);

  playerData->sinklist.defname = "";
  playerData->sinklist.count = 0;
  playerData->sinklist.sinklist = NULL;
  playerData->defaultSink = "";
  playerData->currentSink = "";

  volumeGetSinkList ("", &playerData->sinklist);
  playerData->defaultSink = playerData->sinklist.defname;
}
