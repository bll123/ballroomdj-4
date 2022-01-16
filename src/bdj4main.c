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
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "datautil.h"
#include "log.h"
#include "musicq.h"
#include "playlist.h"
#include "portability.h"
#include "process.h"
#include "queue.h"
#include "sock.h"
#include "sockh.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"

  /* playlistList contains all of the playlists that have been seen */
  /* so that playlist lookups can be processed */
typedef struct {
  mstime_t          tmstart;
  programstate_t    programState;
  Sock_t            playerSock;
  Sock_t            mobilemqSock;
  Sock_t            guiSock;
  list_t            *playlistList;
  queue_t           *playlistQueue;
  musicq_t          *musicQueue;
  musicqidx_t       musicqCurrentIdx;
  list_t            *announceList;
  playerstate_t     playerState;
  ssize_t           gap;
  bool              playerHandshake : 1;
  bool              playerStarted : 1;
  bool              mobilemqHandshake : 1;
  bool              mobilemqStarted : 1;
  bool              marqueeChanged : 1;
} maindata_t;

static void     mainStartPlayer (maindata_t *mainData);
static void     mainStopPlayer (maindata_t *mainData);
static void     mainStartMobileMq (maindata_t *mainData);
static void     mainStopMobileMq (maindata_t *mainData);
static int      mainProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static int      mainProcessing (void *udata);
static void     mainConnectMobileMq (maindata_t *mainData);
static void     mainPlaylistQueue (maindata_t *mainData, char *plname);
static void     mainSigHandler (int sig);
static void     mainMusicQueueFill (maindata_t *mainData);
static void     mainMusicQueuePrep (maindata_t *mainData);
static char     *mainPrepSong (maindata_t *maindata, song_t *song,
                    char *sfname, char *plname, bdjmsgprep_t flag);
static void     mainMusicQueuePlay (maindata_t *mainData);
static void     mainMusicQueueFinish (maindata_t *mainData);
static void     mainMusicQueueNext (maindata_t *mainData);
static bool     mainCheckMusicQueue (song_t *song, void *tdata);

static int gKillReceived = 0;

int
main (int argc, char *argv[])
{
  maindata_t    mainData;
  uint16_t      listenPort;


  mstimestart (&mainData.tmstart);

  mainData.programState = STATE_INITIALIZING;
  mainData.playerSock = INVALID_SOCKET;
  mainData.mobilemqSock = INVALID_SOCKET;
  mainData.mobilemqStarted = false;
  mainData.playlistList = NULL;
  mainData.playlistQueue = NULL;
  mainData.musicQueue = NULL;
  mainData.musicqCurrentIdx = MUSICQ_A;
  mainData.playerState = PL_STATE_STOPPED;
  mainData.playerStarted = false;
  mainData.playerHandshake = false;
  mainData.mobilemqStarted = false;
  mainData.mobilemqHandshake = false;
  mainData.marqueeChanged = false;

  processCatchSignals (mainSigHandler);
  processSigChildIgnore ();

  bdj4startup (argc, argv);
  logProcBegin (LOG_PROC, "main");

  mainData.gap = bdjoptGetNum (OPT_P_GAP);
  mainData.playlistList = listAlloc ("playlist-list", LIST_UNORDERED,
      istringCompare, free, playlistFree);
  mainData.playlistQueue = queueAlloc (NULL);
  mainData.musicQueue = musicqAlloc ();
  mainData.announceList = listAlloc ("announcements", LIST_ORDERED,
      istringCompare, free, NULL);

  listenPort = bdjvarsl [BDJVL_MAIN_PORT];
  sockhMainLoop (listenPort, mainProcessMsg, mainProcessing, &mainData);

  logProcEnd (LOG_PROC, "main", "");
  bdj4shutdown ();
  if (mainData.playlistList != NULL) {
    listFree (mainData.playlistList);
  }
  if (mainData.playlistQueue != NULL) {
    queueFree (mainData.playlistQueue);
  }
  if (mainData.musicQueue != NULL) {
    musicqFree (mainData.musicQueue);
  }
  if (mainData.announceList != NULL) {
    listFree (mainData.announceList);
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
  char      *extension = "";
  int       rc;


  logProcBegin (LOG_PROC, "mainStartPlayer");
  if (mainData->playerStarted) {
    logProcEnd (LOG_PROC, "mainStartPlayer", "already");
    return;
  }

  extension = "";
  if (isWindows ()) {
    extension = ".exe";
  }
  datautilMakePath (tbuff, sizeof (tbuff), "",
      "bdj4player", extension, DATAUTIL_MP_EXECDIR);
  rc = processStart (tbuff, &pid, lsysvars [SVL_BDJIDX],
      bdjoptGetNum (OPT_G_DEBUGLVL));
  if (rc < 0) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "player %s failed to start", tbuff);
  } else {
    logMsg (LOG_DBG, LOG_BASIC, "player started pid: %zd", (ssize_t) pid);
    mainData->playerStarted = true;
  }
  logProcEnd (LOG_PROC, "mainStartPlayer", "");
}

static void
mainStopPlayer (maindata_t *mainData)
{
  logProcBegin (LOG_PROC, "mainStopPlayer");
  if (! mainData->playerStarted) {
    logProcEnd (LOG_PROC, "mainStopPlayer", "not-started");
    return;
  }
  sockhSendMessage (mainData->playerSock, ROUTE_MAIN, ROUTE_PLAYER,
      MSG_EXIT_REQUEST, NULL);
  sockhSendMessage (mainData->playerSock, ROUTE_MAIN, ROUTE_PLAYER,
      MSG_SOCKET_CLOSE, NULL);
  sockClose (mainData->playerSock);
  mainData->playerSock = INVALID_SOCKET;
  mainData->playerStarted = false;
  logProcEnd (LOG_PROC, "mainStopPlayer", "");
}

static void
mainStartMobileMq (maindata_t *mainData)
{
  char      tbuff [MAXPATHLEN];
  pid_t     pid;
  char      *extension = "";
  int       rc;

  if (bdjoptGetNum (OPT_P_MOBILEMARQUEE) == MOBILEMQ_OFF) {
    return;
  }
  if (mainData->mobilemqStarted) {
    return;
  }

  extension = "";
  if (isWindows ()) {
    extension = ".exe";
  }
  datautilMakePath (tbuff, sizeof (tbuff), "",
      "mobilemarquee", extension, DATAUTIL_MP_EXECDIR);
  rc = processStart (tbuff, &pid, lsysvars [SVL_BDJIDX],
      bdjoptGetNum (OPT_G_DEBUGLVL));
  if (rc < 0) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "mobilemarquee %s failed to start", tbuff);
  } else {
    logMsg (LOG_DBG, LOG_BASIC, "mobilemarquee started pid: %zd", (ssize_t) pid);
    mainData->mobilemqStarted = true;
  }
}

static void
mainStopMobileMq (maindata_t *mainData)
{
  if (bdjoptGetNum (OPT_P_MOBILEMARQUEE) == MOBILEMQ_OFF) {
    return;
  }
  if (! mainData->mobilemqStarted) {
    return;
  }

  sockhSendMessage (mainData->mobilemqSock, ROUTE_MAIN, ROUTE_MOBILEMQ,
      MSG_EXIT_REQUEST, NULL);
  sockhSendMessage (mainData->mobilemqSock, ROUTE_MAIN, ROUTE_MOBILEMQ,
      MSG_SOCKET_CLOSE, NULL);
  sockClose (mainData->mobilemqSock);
  mainData->mobilemqSock = INVALID_SOCKET;
  mainData->mobilemqStarted = false;
}

static int
mainProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  maindata_t      *mainData;

  logProcBegin (LOG_PROC, "mainProcessMsg");
  mainData = (maindata_t *) udata;

  logMsg (LOG_DBG, LOG_MSGS, "got: from: %ld route: %ld msg:%ld args:%s",
      routefrom, route, msg, args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MAIN: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          if (routefrom == ROUTE_PLAYER) {
            mainData->playerHandshake = true;
          }
          if (routefrom == ROUTE_MOBILEMQ) {
            mainData->mobilemqHandshake = true;
          }
          break;
        }
        case MSG_SET_DEBUG_LVL: {
          break;
        }
        case MSG_PLAY_PLAY: {
          mainMusicQueuePlay (mainData);
          break;
        }
        case MSG_PLAYBACK_STOP: {
          mainMusicQueueFinish (mainData);
          break;
        }
        case MSG_PLAYBACK_FINISH: {
          mainMusicQueueNext (mainData);
          break;
        }
        case MSG_PLAYER_STATE: {
          mainData->playerState = atol (args);
          logMsg (LOG_DBG, LOG_MSGS, "got: player-state: %d", mainData->playerState);
          mainData->marqueeChanged = true;
          break;
        }
        case MSG_PLAYLIST_QUEUE: {
          logMsg (LOG_DBG, LOG_MSGS, "got: playlist-queue");
          mainPlaylistQueue (mainData, args);
          break;
        }
        case MSG_EXIT_REQUEST: {
          gKillReceived = 0;
          mainData->programState = STATE_CLOSING;
          mainStopPlayer (mainData);
          mainStopMobileMq (mainData);
          logProcEnd (LOG_PROC, "mainProcessMsg", "req-exit");
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

  logProcEnd (LOG_PROC, "mainProcessMsg", "");
  return 0;
}

static int
mainProcessing (void *udata)
{
  maindata_t      *mainData = NULL;


  mainData = (maindata_t *) udata;
  if (mainData->programState == STATE_INITIALIZING) {
    mainData->programState = STATE_LISTENING;
    logMsg (LOG_DBG, LOG_MAIN, "prog: listening");
  }

  if (mainData->programState == STATE_LISTENING &&
      ! mainData->playerStarted) {
    mainStartPlayer (mainData);
    mainStartMobileMq (mainData);
    mainData->programState = STATE_CONNECTING;
    logMsg (LOG_DBG, LOG_MAIN, "prog: connecting");
  }

  if (mainData->programState == STATE_CONNECTING &&
      mainData->playerStarted &&
      mainData->playerSock == INVALID_SOCKET) {
    int           err;
    uint16_t      port;

    port = bdjvarsl [BDJVL_PLAYER_PORT];
    mainData->playerSock = sockConnect (port, &err, 1000);
    if (mainData->playerSock != INVALID_SOCKET) {
      sockhSendMessage (mainData->playerSock, ROUTE_MAIN, ROUTE_PLAYER,
          MSG_HANDSHAKE, NULL);
      mainData->programState = STATE_WAIT_HANDSHAKE;
      logMsg (LOG_DBG, LOG_MAIN, "prog: wait for handshake");
    }

    mainConnectMobileMq (mainData);

    port = bdjvarsl [BDJVL_GUI_PORT];
    mainData->guiSock = sockConnect (port, &err, 1000);
    if (mainData->guiSock != INVALID_SOCKET) {
      sockhSendMessage (mainData->guiSock, ROUTE_MAIN, ROUTE_GUI,
          MSG_HANDSHAKE, NULL);
    }
  }

  if (mainData->programState == STATE_WAIT_HANDSHAKE) {
    if (mainData->playerHandshake) {
      mainData->programState = STATE_RUNNING;
      logMsg (LOG_DBG, LOG_MAIN, "prog: running");
      logMsg (LOG_SESS, LOG_IMPORTANT, "running: time-to-start: %ld ms", mstimeend (&mainData->tmstart));
    }
  }

  if (mainData->programState != STATE_RUNNING) {
    return gKillReceived;
  }

  if (mainData->marqueeChanged) {
    char        tbuff [200];
    char        jbuff [2048];
    char        *title;
    char        *dstr;
    ssize_t     mqLen;
    ssize_t     musicqLen;


    mainData->marqueeChanged = false;

    mqLen = bdjoptGetNum (OPT_G_PLAYERQLEN);
    musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqCurrentIdx);
    title = bdjoptGetData (OPT_P_MOBILEMQTITLE);
    if (title == NULL) {
      title = "";
    }

    dstr = "";
    if (musicqLen > 0) {
      dstr = musicqGetDance (mainData->musicQueue, mainData->musicqCurrentIdx, 0);
      if (dstr == NULL) {
        dstr = "";
      }
    }

    strlcpy (jbuff, "{ ", sizeof (jbuff));

    snprintf (tbuff, sizeof (tbuff), "\"mqlen\" : \"%zd\", ", mqLen);
    strlcat (jbuff, tbuff, sizeof (jbuff));
    snprintf (tbuff, sizeof (tbuff), "\"title\" : \"%s\", ", title);
    strlcat (jbuff, tbuff, sizeof (jbuff));

    snprintf (tbuff, sizeof (tbuff), "\"current\" : \"%s\"", dstr);
    strlcat (jbuff, tbuff, sizeof (jbuff));

    if (musicqGetLen (mainData->musicQueue, mainData->musicqCurrentIdx) > 0) {
      for (ssize_t i = 1; i <= mqLen; ++i) {
        if (mainData->playerState == PL_STATE_IN_GAP ||
            (mainData->playerState == PL_STATE_IN_FADEOUT && i > 1)) {
          dstr = "";
        } else {
          dstr = musicqGetDance (mainData->musicQueue, mainData->musicqCurrentIdx, i);
          if (dstr == NULL) {
            dstr = "";
          }
        }
        snprintf (tbuff, sizeof (tbuff), "\"mq%zd\" : \"%s\"", i, dstr);
        strlcat (jbuff, ", ", sizeof (jbuff));
        strlcat (jbuff, tbuff, sizeof (jbuff));
      }
    } else {
      strlcat (jbuff, ", ", sizeof (jbuff));
      strlcat (jbuff, "\"skip\" : \"true\"", sizeof (jbuff));
    }
    strlcat (jbuff, " }", sizeof (jbuff));

    sockhSendMessage (mainData->mobilemqSock, ROUTE_MAIN, ROUTE_MOBILEMQ,
        MSG_MARQUEE_DATA, jbuff);
  }

  return gKillReceived;
}

static void
mainConnectMobileMq (maindata_t *mainData)
{
  uint16_t    port;
  int         err;

  port = bdjvarsl [BDJVL_MOBILEMQ_PORT];
  mainData->mobilemqSock = sockConnect (port, &err, 1000);
  if (mainData->mobilemqSock != INVALID_SOCKET) {
    sockhSendMessage (mainData->mobilemqSock, ROUTE_MAIN, ROUTE_MOBILEMQ,
        MSG_HANDSHAKE, NULL);
  }
}


static void
mainPlaylistQueue (maindata_t *mainData, char *plname)
{
  playlist_t      *playlist;


  logProcBegin (LOG_PROC, "mainPlaylistQueue");
  playlist = playlistAlloc (plname);
  if (playlist != NULL) {
    listSetData (mainData->playlistList, plname, playlist);
    queuePush (mainData->playlistQueue, playlist);
    logMsg (LOG_DBG, LOG_MAIN, "push pl %s", plname);
    mainMusicQueueFill (mainData);
    mainMusicQueuePrep (mainData);
  }
  logProcEnd (LOG_PROC, "mainPlaylistQueue", "");
}

static void
mainSigHandler (int sig)
{
  gKillReceived = 1;
}

static void
mainMusicQueueFill (maindata_t *mainData)
{
  ssize_t     mqLen;
  ssize_t     currlen;
  song_t      *song = NULL;
  playlist_t  *playlist = NULL;


  logProcBegin (LOG_PROC, "mainMusicQueueFill");
  mqLen = bdjoptGetNum (OPT_G_PLAYERQLEN);
  playlist = queueGetCurrent (mainData->playlistQueue);
  currlen = musicqGetLen (mainData->musicQueue, mainData->musicqCurrentIdx);
  logMsg (LOG_DBG, LOG_BASIC, "fill: %ld < %ld", currlen, mqLen);
    /* want current + mqLen songs */
  while (playlist != NULL && currlen <= mqLen) {
    song = playlistGetNextSong (playlist, mainCheckMusicQueue, mainData);
    if (song == NULL) {
      queuePop (mainData->playlistQueue);
      playlist = queueGetCurrent (mainData->playlistQueue);
      continue;
    }
    musicqPush (mainData->musicQueue, mainData->musicqCurrentIdx, song, playlistGetName (playlist));
    mainData->marqueeChanged = true;
    currlen = musicqGetLen (mainData->musicQueue, mainData->musicqCurrentIdx);
  }
  logProcEnd (LOG_PROC, "mainMusicQueueFill", "");
}

static void
mainMusicQueuePrep (maindata_t *mainData)
{
  logProcBegin (LOG_PROC, "mainMusicQueuePrep");
    /* 5 is the number of songs to prep ahead of time */
  for (ssize_t i = 0; i < 5; ++i) {
    char          *sfname = NULL;
    song_t        *song = NULL;
    musicqflag_t  flags;
    char          *plname = NULL;
    char          *annfname = NULL;

    song = musicqGetByIdx (mainData->musicQueue, mainData->musicqCurrentIdx, i);
    flags = musicqGetFlags (mainData->musicQueue, mainData->musicqCurrentIdx, i);
    plname = musicqGetPlaylistName (mainData->musicQueue, mainData->musicqCurrentIdx, i);

    if (song != NULL &&
        (flags & MUSICQ_FLAG_PREP) != MUSICQ_FLAG_PREP) {
      musicqSetFlags (mainData->musicQueue, mainData->musicqCurrentIdx,
          i, MUSICQ_FLAG_PREP);
      annfname = mainPrepSong (mainData, song, sfname, plname, PREP_SONG);
      if (annfname != NULL && strcmp (annfname, "") != 0 ) {
        musicqSetFlags (mainData->musicQueue, mainData->musicqCurrentIdx,
            i, MUSICQ_FLAG_ANNOUNCE);
        musicqSetAnnounce (mainData->musicQueue, mainData->musicqCurrentIdx,
            i, annfname);
      } else {
        annfname = NULL;
      }
    }
  }
  logProcEnd (LOG_PROC, "mainMusicQueuePrep", "");
}


static char *
mainPrepSong (maindata_t *mainData, song_t *song,
    char *sfname, char *plname, bdjmsgprep_t flag)
{
  char          tbuff [MAXPATHLEN];
  playlist_t    *playlist = NULL;
  ssize_t       dur = 0;
  ssize_t       maxdur = -1;
  ssize_t       pldur = -1;
  ssize_t       plddur = -1;
  ssize_t       plgap = -1;
  ssize_t       songstart = 0;
  ssize_t       songend = -1;
  ssize_t       voladjperc = 0;
  ssize_t       gap = 0;
  ssize_t       plannounce = 0;
  dancekey_t    dancekey;
  char          *annfname = NULL;


  sfname = songGetData (song, TAG_FILE);
  dur = songGetNum (song, TAG_DURATION);
  voladjperc = songGetNum (song, TAG_VOLUMEADJUSTPERC);
  gap = 0;
  songstart = 0;

    /* announcements don't need any of the following... */
  if (flag != PREP_ANNOUNCE) {
    maxdur = bdjoptGetNum (OPT_P_MAXPLAYTIME);
    songstart = songGetNum (song, TAG_SONGSTART);
    songend = songGetNum (song, TAG_SONGEND);
    playlist = listGetData (mainData->playlistList, plname);
    pldur = playlistGetConfigNum (playlist, PLAYLIST_MAX_PLAY_TIME);
      /* apply songend if set to a reasonable value */
    logMsg (LOG_DBG, LOG_MAIN, "dur: %zd songstart: %zd songend: %zd",
        dur, songstart, songend);
    if (songend >= 10000 && dur > songend) {
      dur = songend;
      logMsg (LOG_DBG, LOG_MAIN, "dur-songend: %zd", dur);
    }
      /* adjust the song's duration by the songstart value */
    if (songstart > 0) {
      dur -= songstart;
      logMsg (LOG_DBG, LOG_MAIN, "dur-songstart: %zd", dur);
    }
      /* if the playlist has a maximum play time specified for a dance */
      /* it overrides any of the other max play times */
    dancekey = songGetNum (song, TAG_DANCE);
    plddur = playlistGetDanceNum (playlist, dancekey, PLDANCE_MAXPLAYTIME);
    if (plddur >= 5000) {
      dur = plddur;
      logMsg (LOG_DBG, LOG_MAIN, "dur-pldur: %zd", dur);
    } else {
        /* the playlist max-play-time overrides the global max-play-time */
      if (pldur >= 5000) {
        if (dur > pldur) {
          dur = pldur;
          logMsg (LOG_DBG, LOG_MAIN, "dur-pldur: %zd", dur);
        }
      } else if (dur > maxdur) {
        dur = maxdur;
        logMsg (LOG_DBG, LOG_MAIN, "dur-maxdur: %zd", dur);
      }
    }

    gap = mainData->gap;
    plgap = playlistGetConfigNum (playlist, PLAYLIST_GAP);
    if (plgap >= 0) {
      gap = plgap;
    }

    plannounce = playlistGetConfigNum (playlist, PLAYLIST_ANNOUNCE);
    if (plannounce) {
      dance_t       *dances;
      song_t        *tsong;

      dancekey = songGetNum (song, TAG_DANCE);
      dances = bdjvarsdf [BDJVDF_DANCES];
      annfname = danceGetData (dances, dancekey, DANCE_ANNOUNCE);
      if (annfname != NULL) {
        ssize_t   tval;

        tsong = dbGetByName (annfname);
        if (tsong != NULL) {
          tval = listGetNum (mainData->announceList, annfname);
          if (tval == LIST_VALUE_INVALID) {
            /* only prep the announcement if it has not been prepped before */
            mainPrepSong (mainData, tsong, annfname, plname, PREP_ANNOUNCE);
          }
          listSetNum (mainData->announceList, annfname, 1);
        } else {
          annfname = NULL;
        }
      } /* found an announcement for the dance */
    } /* announcements are on in the playlist */
  } /* if this is a normal song */

  snprintf (tbuff, MAXPATHLEN, "%s%c%zd%c%zd%c%zd%c%zd%c%d", sfname,
      MSG_ARGS_RS, dur, MSG_ARGS_RS, songstart,
      MSG_ARGS_RS, voladjperc, MSG_ARGS_RS, gap, MSG_ARGS_RS, flag);

  logMsg (LOG_DBG, LOG_MAIN, "prep song %s", sfname);
  sockhSendMessage (mainData->playerSock, ROUTE_MAIN, ROUTE_PLAYER,
      MSG_SONG_PREP, tbuff);
  return annfname;
}


static void
mainMusicQueuePlay (maindata_t *mainData)
{
  musicqflag_t      flags;
  char              *sfname;
  song_t            *song;

  logProcBegin (LOG_PROC, "mainMusicQueuePlay");
  if (mainData->playerState == PL_STATE_STOPPED) {
      /* grab a song out of the music queue and start playing */
    logMsg (LOG_DBG, LOG_MAIN, "player is stopped, get song, start");
    song = musicqGetCurrent (mainData->musicQueue, mainData->musicqCurrentIdx);
    flags = musicqGetFlags (mainData->musicQueue, mainData->musicqCurrentIdx, 0);
    if ((flags & MUSICQ_FLAG_ANNOUNCE) == MUSICQ_FLAG_ANNOUNCE) {
      char      *annfname;

      annfname = musicqGetAnnounce (mainData->musicQueue, mainData->musicqCurrentIdx, 0);
      if (annfname != NULL) {
        sockhSendMessage (mainData->playerSock, ROUTE_MAIN, ROUTE_PLAYER,
            MSG_SONG_PLAY, annfname);
      }
    }
    sfname = songGetData (song, TAG_FILE);
    sockhSendMessage (mainData->playerSock, ROUTE_MAIN, ROUTE_PLAYER,
        MSG_SONG_PLAY, sfname);
  }
  if (mainData->playerState == PL_STATE_PAUSED) {
    logMsg (LOG_DBG, LOG_MAIN, "player is paused, send play msg");
    sockhSendMessage (mainData->playerSock, ROUTE_MAIN, ROUTE_PLAYER,
        MSG_PLAY_PLAY, NULL);
  }
  logProcEnd (LOG_PROC, "mainMusicQueuePlay", "");
}

static void
mainMusicQueueFinish (maindata_t *mainData)
{
  logProcBegin (LOG_PROC, "mainMusicQueueFinish");
  mainData->playerState = PL_STATE_STOPPED;
  musicqPop (mainData->musicQueue, mainData->musicqCurrentIdx);
  mainData->marqueeChanged = true;
  logProcEnd (LOG_PROC, "mainMusicQueueFinish", "");
}

static void
mainMusicQueueNext (maindata_t *mainData)
{
  logProcBegin (LOG_PROC, "mainMusicQueueNext");
  mainMusicQueueFinish (mainData);
  mainMusicQueuePlay (mainData);
  mainMusicQueueFill (mainData);
  mainMusicQueuePrep (mainData);
  logProcEnd (LOG_PROC, "mainMusicQueueNext", "");
}

static bool
mainCheckMusicQueue (song_t *song, void *tdata)
{
//  maindata_t  *mainData = tdata;

  return true;
}
