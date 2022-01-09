/*
 * bdj4main
 *  Main entry point for the player process.
 *  Handles startup of the player, playlists and the music queue.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "bdj4init.h"
#include "sockh.h"
#include "sock.h"
#include "bdjvars.h"
#include "bdjmsg.h"
#include "log.h"
#include "process.h"
#include "bdjstring.h"
#include "sysvars.h"
#include "portability.h"
#include "playlist.h"
#include "queue.h"
#include "musicq.h"
#include "plq.h"
#include "tmutil.h"
#include "bdjopt.h"
#include "tagdef.h"

typedef struct {
  mstime_t          tmstart;
  programstate_t    programState;
  int               playerStarted;
  Sock_t            playerSock;
  plq_t             *playlistQueue;
  void              *currentPlaylist;
  musicq_t          *musicQueue;
  musicqidx_t       musicqCurrent;
  playerstate_t     playerState;
  bool              playerHandshake : 1;
} maindata_t;

static void     mainStartPlayer (maindata_t *mainData);
static void     mainStopPlayer (maindata_t *mainData);
static int      mainProcessMsg (long routefrom, long route, long msg,
                    char *args, void *udata);
static int      mainProcessing (void *udata);
static void     mainPlaylistQueue (maindata_t *mainData, char *plname);
static void     mainSigHandler (int sig);
static void     mainMusicQueueFill (maindata_t *mainData);
static void     mainMusicQueuePrep (maindata_t *mainData);
static void     mainMusicQueuePlay (maindata_t *mainData);
static void     mainMusicQueueNext (maindata_t *mainData);

static int gKillReceived = 0;

int
main (int argc, char *argv[])
{
  maindata_t    mainData;

  mstimestart (&mainData.tmstart);
  mainData.programState = STATE_INITIALIZING;
  mainData.playerStarted = 0;
  mainData.playerSock = INVALID_SOCKET;
  mainData.playlistQueue = NULL;
  mainData.musicQueue = NULL;
  mainData.musicqCurrent = MUSICQ_A;
  mainData.playerState = PL_STATE_STOPPED;
  mainData.playerHandshake = false;

  processCatchSignals (mainSigHandler);

  bdj4startup (argc, argv);
  logProcBegin (LOG_MAIN, "main");

  mainData.playlistQueue = plqAlloc ();
  mainData.musicQueue = musicqAlloc ();

  uint16_t listenPort = bdjvarsl [BDJVL_MAIN_PORT];
  sockhMainLoop (listenPort, mainProcessMsg, mainProcessing, &mainData);

  logProcEnd (LOG_MAIN, "main", "");
  bdj4shutdown ();
  if (mainData.playlistQueue != NULL) {
    plqFree (mainData.playlistQueue);
  }
  if (mainData.musicQueue != NULL) {
    musicqFree (mainData.musicQueue);
  }
  mainData.programState = STATE_NOT_RUNNING;
  return 0;
}

/* internal routines */

static void
mainStartPlayer (maindata_t *mainData)
{
  char      tbuff [MAXPATHLEN];
  pid_t     pid;

  logProcBegin (LOG_BASIC, "mainStartPlayer");
  if (mainData->playerStarted) {
    logProcEnd (LOG_BASIC, "mainStartPlayer", "already");
    return;
  }

  snprintf (tbuff, MAXPATHLEN, "%s/bdj4player", sysvars [SV_BDJ4EXECDIR]);
  if (isWindows ()) {
    strlcat (tbuff, ".exe", MAXPATHLEN);
  }
  int rc = processStart (tbuff, &pid, lsysvars [SVL_BDJIDX],
      bdjoptGetLong (OPT_G_DEBUGLVL));
  if (rc < 0) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "player %s failed to start", tbuff);
  } else {
    logMsg (LOG_DBG, LOG_BASIC, "player started pid: %zd", (ssize_t) pid);
    mainData->playerStarted = 1;
  }
  logProcEnd (LOG_MAIN, "mainStartPlayer", "");
}

static void
mainStopPlayer (maindata_t *mainData)
{
  logProcBegin (LOG_BASIC, "mainStopPlayer");
  if (! mainData->playerStarted) {
    logProcEnd (LOG_BASIC, "mainStopPlayer", "not-started");
    return;
  }
  sockhSendMessage (mainData->playerSock, ROUTE_MAIN, ROUTE_PLAYER,
      MSG_EXIT_REQUEST, NULL);
  sockhSendMessage (mainData->playerSock, ROUTE_MAIN, ROUTE_PLAYER,
      MSG_SOCKET_CLOSE, NULL);
  sockClose (mainData->playerSock);
  mainData->playerSock = INVALID_SOCKET;
  mainData->playerStarted = 0;
  logProcEnd (LOG_BASIC, "mainStopPlayer", "");
}

static int
mainProcessMsg (long routefrom, long route, long msg, char *args, void *udata)
{
  maindata_t      *mainData;

  logProcBegin (LOG_MAIN, "mainProcessMsg");
  mainData = (maindata_t *) udata;

  logMsg (LOG_DBG, LOG_MAIN, "got: from: %ld route: %ld msg:%ld args:%s",
      routefrom, route, msg, args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MAIN: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          if (routefrom == ROUTE_PLAYER) {
            mainData->playerHandshake = true;
          }
          break;
        }
        case MSG_SET_DEBUG_LVL: {
          break;
        }
        case MSG_PLAY_FADE: {
          break;
        }
        case MSG_PLAY_PLAY: {
          mainMusicQueuePlay (mainData);
          break;
        }
        case MSG_PLAY_PAUSE: {
          break;
        }
        case MSG_PLAY_STOP: {
          break;
        }
        case MSG_PLAYBACK_BEGIN: {
          mainData->playerState = PL_STATE_PLAYING;
          break;
        }
        case MSG_PLAYBACK_FINISH: {
          mainMusicQueueNext (mainData);
          break;
        }
        case MSG_PLAYLIST_QUEUE: {
          logMsg (LOG_DBG, LOG_MAIN, "got: playlist-queue");
          mainPlaylistQueue (mainData, args);
          break;
        }
        case MSG_EXIT_REQUEST: {
          gKillReceived = 0;
          mainData->programState = STATE_CLOSING;
          mainStopPlayer (mainData);
          logProcEnd (LOG_MAIN, "mainProcessMsg", "req-exit");
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

  logProcEnd (LOG_MAIN, "mainProcessMsg", "");
  return 0;
}

static int
mainProcessing (void *udata)
{
  maindata_t      *mainData;

  mainData = (maindata_t *) udata;
  if (mainData->programState == STATE_INITIALIZING) {
    mainData->programState = STATE_LISTENING;
    logMsg (LOG_DBG, LOG_MAIN, "prog: listening");
  }

  if (mainData->programState == STATE_LISTENING &&
      ! mainData->playerStarted) {
    mainStartPlayer (mainData);
    mainData->programState = STATE_CONNECTING;
    logMsg (LOG_DBG, LOG_MAIN, "prog: connecting");
  }

  if (mainData->programState == STATE_CONNECTING &&
      mainData->playerStarted &&
      mainData->playerSock == INVALID_SOCKET) {
    int       err;

    uint16_t playerPort = bdjvarsl [BDJVL_PLAYER_PORT];
    mainData->playerSock = sockConnect (playerPort, &err, 1000);
    if (mainData->playerSock != INVALID_SOCKET) {
      sockhSendMessage (mainData->playerSock, ROUTE_MAIN, ROUTE_PLAYER,
          MSG_HANDSHAKE, NULL);
      mainData->programState = STATE_WAIT_HANDSHAKE;
      logMsg (LOG_DBG, LOG_MAIN, "prog: wait for handshake");
    }
  }

  if (mainData->programState == STATE_WAIT_HANDSHAKE) {
    if (mainData->playerHandshake) {
      mainData->programState = STATE_RUNNING;
      logMsg (LOG_DBG, LOG_MAIN, "prog: running");
      logMsg (LOG_SESS, LOG_IMPORTANT, "running: time-to-start: %ld ms", mstimeend (&mainData->tmstart));
    }
  }

  return gKillReceived;
}

static void
mainPlaylistQueue (maindata_t *mainData, char *plname)
{
  logProcBegin (LOG_BASIC, "mainPlaylistQueue");
  plqPush (mainData->playlistQueue, plname);
  logMsg (LOG_DBG, LOG_MAIN, "push pl %s", plname);
  mainMusicQueueFill (mainData);
  mainMusicQueuePrep (mainData);
  logProcEnd (LOG_BASIC, "mainPlaylistQueue", "");
}

static void
mainSigHandler (int sig)
{
  gKillReceived = 1;
}

static void
mainMusicQueueFill (maindata_t *mainData)
{
  long        maxlen;
  long        currlen;
  song_t      *song;
  playlist_t  *playlist;

  logProcBegin (LOG_BASIC, "mainMusicQueueFill");
  maxlen = bdjoptGetLong (OPT_G_PLAYERQLEN);
  playlist = plqGetCurrent (mainData->playlistQueue);
  currlen = musicqGetLen (mainData->musicQueue, mainData->musicqCurrent);
  logMsg (LOG_DBG, LOG_BASIC, "fill: %ld < %ld", currlen, maxlen);
  while (currlen < maxlen) {
    song = playlistGetNextSong (playlist);
    if (song == NULL) {
      /* ### FIX: pop playlist; if there are no more, return */
      break;
    }
    musicqPush (mainData->musicQueue, mainData->musicqCurrent, song);
  }
  logProcEnd (LOG_BASIC, "mainMusicQueueFill", "");
}

static void
mainMusicQueuePrep (maindata_t *mainData)
{
  char tbuff [MAXPATHLEN];

  logProcBegin (LOG_BASIC, "mainMusicQueuePrep");
    /* 5 is number of songs to prep ahead of time */
  for (size_t i = 0; i < 5; ++i) {
    song_t    *song;
    char      *sfname;
    ssize_t   dur;


    song = musicqGetByIdx (mainData->musicQueue, mainData->musicqCurrent, i);
    if (song != NULL) {
      sfname = songGetData (song, TAG_KEY_FILE);
      dur = songGetLong (song, TAG_KEY_DURATION);

      snprintf (tbuff, MAXPATHLEN, "%s%c%zd", sfname, MSG_ARGS_RS, dur);

      logMsg (LOG_DBG, LOG_MAIN, "prep song %s", sfname);
      sockhSendMessage (mainData->playerSock, ROUTE_MAIN, ROUTE_PLAYER,
          MSG_SONG_PREP, tbuff);
    }
  }
  logProcEnd (LOG_BASIC, "mainMusicQueuePrep", "");
}


static void
mainMusicQueuePlay (maindata_t *mainData)
{
  logProcBegin (LOG_BASIC, "mainMusicQueuePlay");
  if (mainData->playerState == PL_STATE_STOPPED) {
      /* grab a song out of the music queue and start playing */
    song_t *song = musicqGetCurrent (mainData->musicQueue, mainData->musicqCurrent);
    char *sfname = songGetData (song, TAG_KEY_FILE);
    sockhSendMessage (mainData->playerSock, ROUTE_MAIN, ROUTE_PLAYER,
        MSG_SONG_PLAY, sfname);
  }
  if (mainData->playerState == PL_STATE_PAUSED) {
    sockhSendMessage (mainData->playerSock, ROUTE_MAIN, ROUTE_PLAYER,
        MSG_PLAY_PLAY, NULL);
  }
  logProcEnd (LOG_BASIC, "mainMusicQueuePlay", "");
}

static void
mainMusicQueueNext (maindata_t *mainData)
{
  logProcBegin (LOG_BASIC, "mainMusicQueueNext");
  mainData->playerState = PL_STATE_STOPPED;
  musicqPop (mainData->musicQueue, mainData->musicqCurrent);
  mainMusicQueuePlay (mainData);
  mainMusicQueueFill (mainData);
  mainMusicQueuePrep (mainData);
  logProcEnd (LOG_BASIC, "mainMusicQueueNext", "");
}
