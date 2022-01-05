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
#include "vlci.h"

typedef struct {
  char          *songname;
  char          *tempname;
} prepqueue_t;

typedef struct {
  programstate_t  programState;
  Sock_t          mainSock;
  vlcData_t       *vlcData;
  queue_t         *prepQueue;
  long            globalCount;
} playerData_t;

static int      processMsg (long route, long msg, char *args, void *udata);
static void     mainProcessing (void *udata);
static void     songPrep (playerData_t *playerData, char *sfname);
static void     songPlay (playerData_t *playerData, char *sfname);
static void     songMakeTempName (playerData_t *playerData,
                    char *in, char *out, size_t maxlen);
static void     playerPause (playerData_t *playerData);
static void     playerPlay (playerData_t *playerData);
static void     playerStop (playerData_t *playerData);
static void     playerClose (playerData_t *playerData);
static void     prepQueueFree (void *);

static char *   vlcDefaultOptions [] = {
      "--quiet",
      "--intf=dummy",
      "--audio-filter", "scaletempo",
      "--ignore-config",
      "--no-media-library",
      "--no-playlist-autostart",
      "--no-random",
      "--no-loop",
      "--no-repeat",
      "--play-and-stop",
      "--novideo",
};
#define VLC_DFLT_OPT_SZ (sizeof (vlcDefaultOptions) / sizeof (char *))

int
main (int argc, char *argv[])
{
  playerData_t    playerData;
  int             c = 0;
  int             option_index = 0;

  static struct option bdj_options [] = {
    { "player",     no_argument,        NULL,   0 },
    { "profile",    required_argument,  NULL,   'p' },
    { NULL,         0,                  NULL,   0 }
  };

  playerData.programState = STATE_INITIALIZING;
  playerData.mainSock = INVALID_SOCKET;
  playerData.vlcData = NULL;
  playerData.prepQueue = queueAlloc (prepQueueFree);
  playerData.globalCount = 1;

  sysvarsInit (argv[0]);

  while ((c = getopt_long (argc, argv, "", bdj_options, &option_index)) != -1) {
    switch (c) {
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
  logStartAppend ("bdj4player", "p", LOG_LVL_5);

  logMsg (LOG_SESS, LOG_LVL_1, "Using profile %ld", lsysvars [SVL_BDJIDX]);

  playerData.vlcData = vlcInit (VLC_DFLT_OPT_SZ, vlcDefaultOptions);
  assert (playerData.vlcData != NULL);

  playerData.programState = STATE_RUNNING;

  uint16_t listenPort = bdjvarsl [BDJVL_PLAYER_PORT];
  sockhMainLoop (listenPort, processMsg, mainProcessing, &playerData);

  if (playerData.vlcData != NULL) {
    playerStop (&playerData);
    playerClose (&playerData);
  }
  if (playerData.prepQueue != NULL) {
    queueFree (playerData.prepQueue);
  }
  logEnd ();
  playerData.programState = STATE_NOT_RUNNING;
  return 0;
}

/* internal routines */

static int
processMsg (long route, long msg, char *args, void *udata)
{
  playerData_t      *playerData;

  logProcBegin (LOG_LVL_4, "processMsg");
  playerData = (playerData_t *) udata;

  logMsg (LOG_DBG, LOG_LVL_4, "got: route: %ld msg:%ld args:%s", route, msg, args);
  switch (route) {
    case ROUTE_NONE:
    case ROUTE_PLAYER: {
      logMsg (LOG_DBG, LOG_LVL_4, "got: route-player");
      switch (msg) {
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
        case MSG_SONG_PREP: {
          songPrep (playerData, args);
          break;
        }
        case MSG_SONG_PLAY: {
          songPlay (playerData, args);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_DBG, LOG_LVL_4, "got: req-exit");
          playerData->programState = STATE_CLOSING;
          sockhSendMessage (playerData->mainSock, ROUTE_MAIN, MSG_SOCKET_CLOSE, NULL);
          sockClose (playerData->mainSock);
          playerData->mainSock = INVALID_SOCKET;
          logProcEnd (LOG_LVL_4, "processMsg", "req-exit");
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

  logProcEnd (LOG_LVL_4, "processMsg", "");
  return 0;
}

static void
mainProcessing (void *udata)
{
  playerData_t      *playerData;

  playerData = (playerData_t *) udata;

  if (playerData->programState == STATE_RUNNING &&
      playerData->mainSock == INVALID_SOCKET) {
    int       err;

    uint16_t mainPort = bdjvarsl [BDJVL_MAIN_PORT];
    playerData->mainSock = sockConnect (mainPort, &err, 1000);
    logMsg (LOG_DBG, LOG_LVL_4, "main-socket: %zd", (size_t) playerData->mainSock);
  }

  return;
}

/* internal routines - song handling */

static void
songPrep (playerData_t *playerData, char *sfname)
{
  char            tsfname [MAXPATHLEN];  // testing: remove later
  char            stname [MAXPATHLEN];
  prepqueue_t     *pq;

  logProcBegin (LOG_LVL_2, "songPrep");
  if (! fileopExists (sfname)) {
    logMsg (LOG_DBG, LOG_LVL_1, "ERR: no file: %s\n", sfname);
    logProcEnd (LOG_LVL_2, "songPrep", "no-file");
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
  fileopCopyLink (tsfname, stname);
  if (! fileopExists (stname)) {
    logMsg (LOG_DBG, LOG_LVL_1, "ERR: file copy failed: %s\n", stname);
    logProcEnd (LOG_LVL_2, "songPrep", "copy-failed");
    prepQueueFree (pq);
    return;
  }
  pq->tempname = strdup (stname);
  queuePush (playerData->prepQueue, pq);

  // TODO : add the name to a list of prepared files.
  logProcEnd (LOG_LVL_2, "songPrep", "");
}

static void
songPlay (playerData_t *playerData, char *sfname)
{
  char              stname [MAXPATHLEN];
  prepqueue_t       *pq;
  int               found = 0;

  logProcBegin (LOG_LVL_2, "songPlay");
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
    logProcEnd (LOG_LVL_2, "songPlay", "not-found");
    return;
  }

  if (! fileopExists (stname)) {
    logMsg (LOG_DBG, LOG_LVL_1, "ERR: no file: %s\n", stname);
    logProcEnd (LOG_LVL_2, "songPlay", "no-file");
    return;
  }

  vlcMedia (playerData->vlcData, stname);
  vlcPlay (playerData->vlcData);
  logProcEnd (LOG_LVL_2, "songPlay", "");
}

static void
songMakeTempName (playerData_t *playerData, char *in, char *out, size_t maxlen)
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
playerPause (playerData_t *playerData)
{
  vlcPause (playerData->vlcData);
}

static void
playerPlay (playerData_t *playerData)
{
  vlcPlay (playerData->vlcData);
}

static void
playerStop (playerData_t *playerData)
{
  vlcStop (playerData->vlcData);
}

static void
playerClose (playerData_t *playerData)
{
  vlcClose (playerData->vlcData);
}

static void
prepQueueFree (void *data)
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
