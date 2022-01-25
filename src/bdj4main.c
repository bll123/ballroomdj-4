/*
 * bdj4main
 *  Main entry point for the player process.
 *  Handles startup of the player, mobile marquee and remote control.
 *  Handles playlists and the music queue.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "bdj4.h"
#include "bdj4init.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "dancesel.h"
#include "pathbld.h"
#include "filedata.h"
#include "lock.h"
#include "log.h"
#include "musicdb.h"
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
#include "webclient.h"

typedef enum {
  MOVE_UP = -1,
  MOVE_DOWN = 1,
} mainmove_t;

#define SOCKOF(idx) (mainData->processconns [idx].sock)

/* playlistCache contains all of the playlists that have been seen */
/* so that playlist lookups can be processed */
typedef struct {
  mstime_t          tm;
  programstate_t    programState;
  process_t         *processes [PROCESS_MAX];
  processconn_t     processconns [PROCESS_MAX];
  slist_t           *playlistCache;
  queue_t           *playlistQueue;
  musicq_t          *musicQueue;
  nlist_t           *danceCounts;
  musicqidx_t       musicqCurrentIdx;
  slist_t           *announceList;
  playerstate_t     playerState;
  ssize_t           gap;
  webclient_t       *webclient;
  char              *mobmqUserkey;
  bool              musicqChanged : 1;
} maindata_t;

static int      mainProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static int      mainProcessing (void *udata);
static void     mainSendMobileMarqueeData (maindata_t *mainData);
static void     mainMobilePostCallback (void *userdata, char *resp, size_t len);
static void     mainConnectProcess (maindata_t *mainData, processconnidx_t idx,
                    bdjmsgroute_t route, uint16_t port);
static void     mainQueueClear (maindata_t *mainData);
static void     mainQueueDance (maindata_t *mainData, ssize_t danceIdx, ssize_t count);
static void     mainQueuePlaylist (maindata_t *mainData, char *plname);
static void     mainSigHandler (int sig);
static void     mainMusicQueueFill (maindata_t *mainData);
static void     mainMusicQueuePrep (maindata_t *mainData);
static char     *mainPrepSong (maindata_t *maindata, song_t *song,
                    char *sfname, char *plname, bdjmsgprep_t flag);
static void     mainTogglePause (maindata_t *mainData, ssize_t idx);
static void     mainMusicqMove (maindata_t *mainData, ssize_t fromidx, mainmove_t direction);
static void     mainMusicqMoveTop (maindata_t *mainData, ssize_t fromidx);
static void     mainMusicqClear (maindata_t *mainData, ssize_t idx);
static void     mainMusicqRemove (maindata_t *mainData, ssize_t idx);
static void     mainMusicqInsert (maindata_t *mainData, char *args);
static void     mainPlaybackBegin (maindata_t *mainData);
static void     mainMusicQueuePlay (maindata_t *mainData);
static void     mainMusicQueueFinish (maindata_t *mainData);
static void     mainMusicQueueNext (maindata_t *mainData);
static bool     mainCheckMusicQueue (song_t *song, void *tdata);
static ssize_t  mainMusicQueueHistory (void *mainData, ssize_t idx);
static void     mainForceStop (maindata_t *mainData, char *lockfn,
                    int flags, processconnidx_t idx);
static int      mainStartProcess (maindata_t *mainData, processconnidx_t idx,
                    char *fname);
static void     mainStopProcess (maindata_t *mainData, processconnidx_t idx,
                    bdjmsgroute_t route, char *lockfn, bool force);
static void     mainSendDanceList (maindata_t *mainData);
static void     mainSendPlaylistList (maindata_t *mainData);
static void     mainSendStatus (maindata_t *mainData, char *playerResp);
static void     mainDanceCountsInit (maindata_t *mainData);

static long globalCounter = 0;
static int gKillReceived = 0;

int
main (int argc, char *argv[])
{
  maindata_t    mainData;
  uint16_t      listenPort;


  mstimestart (&mainData.tm);

  mainData.programState = STATE_INITIALIZING;
  mainData.playlistCache = NULL;
  mainData.playlistQueue = NULL;
  mainData.musicQueue = NULL;
  mainData.danceCounts = NULL;
  mainData.musicqCurrentIdx = MUSICQ_A;
  mainData.playerState = PL_STATE_STOPPED;
  mainData.webclient = NULL;
  mainData.mobmqUserkey = NULL;
  for (processconnidx_t i = PROCESS_PLAYER; i < PROCESS_MAX; ++i) {
    mainData.processconns [i].sock = INVALID_SOCKET;
    mainData.processconns [i].started = false;
    mainData.processconns [i].handshake = false;
    mainData.processes [i] = NULL;
  }
  mainData.musicqChanged = false;

#if _define_SIGHUP
  processCatchSignal (mainSigHandler, SIGHUP);
#endif
  processCatchSignal (mainSigHandler, SIGINT);
  processCatchSignal (mainSigHandler, SIGTERM);
#if _define_SIGCHLD
  processIgnoreSignal (SIGCHLD);
#endif

  bdj4startup (argc, argv);
  logProcBegin (LOG_PROC, "main");

  mainData.gap = bdjoptGetNum (OPT_P_GAP);
  mainData.playlistCache = slistAlloc ("playlist-list", LIST_ORDERED,
      free, playlistFree);
  mainData.playlistQueue = queueAlloc (NULL);
  mainData.musicQueue = musicqAlloc ();
  mainDanceCountsInit (&mainData);
  mainData.announceList = slistAlloc ("announcements", LIST_ORDERED,
      free, NULL);

  listenPort = bdjvarsl [BDJVL_MAIN_PORT];
  sockhMainLoop (listenPort, mainProcessMsg, mainProcessing, &mainData);

  logProcEnd (LOG_PROC, "main", "");
  if (mainData.playlistCache != NULL) {
    slistFree (mainData.playlistCache);
  }
  if (mainData.playlistQueue != NULL) {
    queueFree (mainData.playlistQueue);
  }
  if (mainData.musicQueue != NULL) {
    musicqFree (mainData.musicQueue);
  }
  if (mainData.announceList != NULL) {
    slistFree (mainData.announceList);
  }
  if (mainData.webclient != NULL) {
    webclientClose (mainData.webclient);
  }
  if (mainData.mobmqUserkey != NULL) {
    free (mainData.mobmqUserkey);
  }
  if (mainData.danceCounts != NULL) {
    nlistFree (mainData.danceCounts);
  }
  mainData.programState = STATE_NOT_RUNNING;
  bdj4shutdown ();
  /* give the other processes some time to shut down */
  mssleep (100);
  mainStopProcess (&mainData, PROCESS_PLAYER, ROUTE_PLAYER,
      PLAYER_LOCK_FN, true);
  mainStopProcess (&mainData, PROCESS_MOBILEMQ, ROUTE_MOBILEMQ,
      MOBILEMQ_LOCK_FN, true);
  mainStopProcess (&mainData, PROCESS_REMCTRL, ROUTE_REMCTRL,
      REMCTRL_LOCK_FN, true);
  for (processconnidx_t i = 0; i < PROCESS_MAX; ++i) {
    if (mainData.processes [i] != NULL) {
      processFree (mainData.processes [i]);
    }
  }
  logMsg (LOG_SESS, LOG_IMPORTANT, "time-to-end: %ld ms", mstimeend (&mainData.tm));
  logEnd ();
  return 0;
}

/* internal routines */

static int
mainProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  maindata_t      *mainData;

  logProcBegin (LOG_PROC, "mainProcessMsg");
  mainData = (maindata_t *) udata;

  if (msg != MSG_PLAYER_STATUS_DATA) {
    logMsg (LOG_DBG, LOG_MSGS, "got: from: %ld route: %ld msg:%ld args:%s",
        routefrom, route, msg, args);
  }

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MAIN: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          if (routefrom == ROUTE_PLAYER) {
            mainData->processconns [PROCESS_PLAYER].handshake = true;
          }
          if (routefrom == ROUTE_MOBILEMQ) {
            mainData->processconns [PROCESS_MOBILEMQ].handshake = true;
          }
          if (routefrom == ROUTE_REMCTRL) {
            mainData->processconns [PROCESS_REMCTRL].handshake = true;
          }
          break;
        }
        case MSG_EXIT_REQUEST: {
          mstimestart (&mainData->tm);
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          mainData->programState = STATE_CLOSING;
          mainStopProcess (mainData, PROCESS_PLAYER, ROUTE_PLAYER, NULL, false);
          mainStopProcess (mainData, PROCESS_MOBILEMQ, ROUTE_MOBILEMQ, NULL, false);
          mainStopProcess (mainData, PROCESS_REMCTRL, ROUTE_REMCTRL, NULL, false);
          logProcEnd (LOG_PROC, "mainProcessMsg", "req-exit");
          return 1;
        }
        case MSG_QUEUE_CLEAR: {
          /* clears both the playlist queue and the music queue */
          logMsg (LOG_DBG, LOG_MSGS, "got: queue-clear");
          mainQueueClear (mainData);
          break;
        }
        case MSG_PLAYLIST_CLEARPLAY: {
          mainQueueClear (mainData);
          /* clear out any playing song */
          sockhSendMessage (SOCKOF (PROCESS_PLAYER),
              ROUTE_MAIN, ROUTE_PLAYER, MSG_PLAY_NEXTSONG, NULL);
          mainQueuePlaylist (mainData, args);
          mainMusicQueuePlay (mainData);
          break;
        }
        case MSG_PLAYLIST_QUEUE: {
          logMsg (LOG_DBG, LOG_MSGS, "got: playlist-queue %s", args);
          mainQueuePlaylist (mainData, args);
          mainMusicQueuePlay (mainData);
          break;
        }
        case MSG_DANCE_QUEUE: {
          mainQueueDance (mainData, atol (args), 1);
          break;
        }
        case MSG_DANCE_QUEUE5: {
          mainQueueDance (mainData, atol (args), 5);
          break;
        }
        case MSG_PLAY_PLAY: {
          mainMusicQueuePlay (mainData);
          break;
        }
        case MSG_PLAY_PLAYPAUSE: {
          if (mainData->playerState == PL_STATE_PLAYING ||
              mainData->playerState == PL_STATE_IN_FADEOUT) {
            sockhSendMessage (SOCKOF (PROCESS_PLAYER),
                ROUTE_MAIN, ROUTE_PLAYER, MSG_PLAY_PAUSE, NULL);
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
        case MSG_PLAYBACK_STOP: {
          mainMusicQueueFinish (mainData);
          break;
        }
        case MSG_PLAYBACK_FINISH: {
          mainMusicQueueNext (mainData);
          break;
        }
        case MSG_MUSICQ_TOGGLE_PAUSE: {
          mainTogglePause (mainData, atol (args));
          break;
        }
        case MSG_MUSICQ_MOVE_DOWN: {
          mainMusicqMove (mainData, atol (args), MOVE_DOWN);
          break;
        }
        case MSG_MUSICQ_MOVE_TOP: {
          mainMusicqMoveTop (mainData, atol (args));
          break;
        }
        case MSG_MUSICQ_MOVE_UP: {
          mainMusicqMove (mainData, atol (args), MOVE_UP);
          break;
        }
        case MSG_MUSICQ_REMOVE: {
          mainMusicqRemove (mainData, atol (args));
          break;
        }
        case MSG_MUSICQ_TRUNCATE: {
          mainMusicqClear (mainData, atol (args));
          break;
        }
        case MSG_MUSICQ_INSERT: {
          mainMusicqInsert (mainData, args);
          break;
        }
        case MSG_PLAYER_STATE: {
          mainData->playerState = (playerstate_t) atol (args);
          logMsg (LOG_DBG, LOG_MSGS, "got: player-state: %d", mainData->playerState);
          mainData->musicqChanged = true;
          break;
        }
        case MSG_GET_DANCE_LIST: {
          mainSendDanceList (mainData);
          break;
        }
        case MSG_GET_PLAYLIST_LIST: {
          mainSendPlaylistList (mainData);
          break;
        }
        case MSG_PLAYER_STATUS_DATA: {
          mainSendStatus (mainData, args);
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
      ! mainData->processconns [PROCESS_PLAYER].started) {
    mainStartProcess (mainData, PROCESS_PLAYER, "bdj4player");
    mainStartProcess (mainData, PROCESS_MOBILEMQ, "bdj4mobmq");
    mainStartProcess (mainData, PROCESS_REMCTRL, "bdj4rc");
    mainData->programState = STATE_CONNECTING;
    logMsg (LOG_DBG, LOG_MAIN, "prog: connecting");
  }

  if (mainData->programState == STATE_CONNECTING &&
      mainData->processconns [PROCESS_PLAYER].started &&
      SOCKOF (PROCESS_PLAYER) == INVALID_SOCKET) {
    uint16_t      port;
    int           connMax = 0;
    int           connCount = 0;

    port = bdjvarsl [BDJVL_PLAYER_PORT];
    mainConnectProcess (mainData, PROCESS_PLAYER, ROUTE_PLAYER, port);

    if (SOCKOF (PROCESS_MOBILEMQ) == INVALID_SOCKET &&
        bdjoptGetNum (OPT_P_MOBILEMARQUEE) != MOBILEMQ_OFF) {
      port = bdjvarsl [BDJVL_MOBILEMQ_PORT];
      mainConnectProcess (mainData, PROCESS_MOBILEMQ, ROUTE_MOBILEMQ, port);
    }
    if (SOCKOF (PROCESS_REMCTRL) == INVALID_SOCKET &&
        bdjoptGetNum (OPT_P_REMOTECONTROL)) {
      port = bdjvarsl [BDJVL_REMCONTROL_PORT];
      mainConnectProcess (mainData, PROCESS_REMCTRL, ROUTE_REMCTRL, port);
    }

    ++connMax;
    if (SOCKOF (PROCESS_PLAYER) != INVALID_SOCKET) {
      ++connCount;
    }
    if (bdjoptGetNum (OPT_P_MOBILEMARQUEE) != MOBILEMQ_OFF) {
      ++connMax;
      if (SOCKOF (PROCESS_MOBILEMQ) != INVALID_SOCKET) {
        ++connCount;
      }
    }
    if (bdjoptGetNum (OPT_P_REMOTECONTROL)) {
      ++connMax;
      if (SOCKOF (PROCESS_REMCTRL) != INVALID_SOCKET) {
        ++connCount;
      }
    }
    if (connCount == connMax) {
      mainData->programState = STATE_WAIT_HANDSHAKE;
      logMsg (LOG_DBG, LOG_MAIN, "prog: wait for handshake");
    }

    if (SOCKOF (PROCESS_GUI) == INVALID_SOCKET) {
      port = bdjvarsl [BDJVL_GUI_PORT];
      mainConnectProcess (mainData, PROCESS_GUI, ROUTE_GUI, port);
    }
  }

  if (mainData->programState == STATE_WAIT_HANDSHAKE) {
    if (mainData->processconns [PROCESS_PLAYER].handshake) {
      mainData->programState = STATE_RUNNING;
      logMsg (LOG_DBG, LOG_MAIN, "prog: running");
      logMsg (LOG_SESS, LOG_IMPORTANT, "running: time-to-start: %ld ms", mstimeend (&mainData->tm));
    }
  }

  if (mainData->programState != STATE_RUNNING) {
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    }
    return gKillReceived;
  }

  if (mainData->musicqChanged) {
    mainSendMobileMarqueeData (mainData);
  }

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  return gKillReceived;
}

static void
mainSendMobileMarqueeData (maindata_t *mainData)
{
  char        tbuff [200];
  char        tbuffb [200];
  char        qbuff [2048];
  char        jbuff [2048];
  char        *title;
  char        *dstr;
  char        *tstr;
  char        *tag;
  ssize_t     mqLen;
  ssize_t     musicqLen;
  song_t      *song;


  if (bdjoptGetNum (OPT_P_MOBILEMARQUEE) == MOBILEMQ_OFF) {
    return;
  }

  mainData->musicqChanged = false;

  mqLen = bdjoptGetNum (OPT_G_PLAYERQLEN);
  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqCurrentIdx);
  title = bdjoptGetData (OPT_P_MOBILEMQTITLE);
  if (title == NULL) {
    title = "";
  }

  strlcpy (jbuff, "{ ", sizeof (jbuff));

  snprintf (tbuff, sizeof (tbuff), "\"mqlen\" : \"%zd\", ", mqLen);
  strlcat (jbuff, tbuff, sizeof (jbuff));
  snprintf (tbuff, sizeof (tbuff), "\"title\" : \"%s\"", title);
  strlcat (jbuff, tbuff, sizeof (jbuff));

  if (musicqLen > 0) {
    for (ssize_t i = 0; i <= mqLen; ++i) {
      if ((i > 0 && mainData->playerState == PL_STATE_IN_GAP) ||
          (i > 1 && mainData->playerState == PL_STATE_IN_FADEOUT)) {
        dstr = "";
      } else if (i > musicqLen) {
        dstr = "";
      } else {
        song = musicqGetByIdx (mainData->musicQueue, mainData->musicqCurrentIdx, i);
        /* if the song has an unknown dance, the marquee display */
        /* will be filled in with the dance name. */
        tstr = songGetData (song, TAG_MQDISPLAY);
        if (tstr != NULL) {
          dstr = tstr;
        } else {
          dstr = musicqGetDance (mainData->musicQueue, mainData->musicqCurrentIdx, i);
        }
      }

      if (i == 0) {
        snprintf (tbuff, sizeof (tbuff), "\"current\" : \"%s\"", dstr);
      } else {
        snprintf (tbuff, sizeof (tbuff), "\"mq%zd\" : \"%s\"", i, dstr);
      }
      strlcat (jbuff, ", ", sizeof (jbuff));
      strlcat (jbuff, tbuff, sizeof (jbuff));
    }
  } else {
    strlcat (jbuff, ", ", sizeof (jbuff));
    strlcat (jbuff, "\"skip\" : \"true\"", sizeof (jbuff));
  }
  strlcat (jbuff, " }", sizeof (jbuff));

  sockhSendMessage (SOCKOF (PROCESS_MOBILEMQ),
      ROUTE_MAIN, ROUTE_MOBILEMQ, MSG_MARQUEE_DATA, jbuff);

  if (bdjoptGetNum (OPT_P_MOBILEMARQUEE) == MOBILEMQ_LOCAL) {
    return;
  }

  /* internet mode from here on */

  tag = bdjoptGetData (OPT_P_MOBILEMQTAG);
  if (tag != NULL) {
    if (mainData->mobmqUserkey == NULL) {
      pathbldMakePath (tbuff, sizeof (tbuff), "",
          "mmq", ".key", PATHBLD_MP_USEIDX);
      mainData->mobmqUserkey = filedataReadAll (tbuff);
    }

    snprintf (tbuff, sizeof (tbuff),
        "https://%s/marquee4.php", sysvars [SV_MOBMQ_HOST]);
    snprintf (qbuff, sizeof (qbuff), "v=2&mqdata=%s&key=%s&tag=%s",
        jbuff, "93457645", tag);
    if (mainData->mobmqUserkey != NULL) {
      snprintf (tbuffb, sizeof (tbuffb), "&userkey=%s", mainData->mobmqUserkey);
      strlcat (qbuff, tbuffb, MAXPATHLEN);
    }
    mainData->webclient = webclientPost (mainData->webclient,
        tbuff, qbuff, mainData, mainMobilePostCallback);
  }
}

static void
mainMobilePostCallback (void *userdata, char *resp, size_t len)
{
  maindata_t    *mainData = userdata;
  char          tbuff [MAXPATHLEN];

  if (strncmp (resp, "OK", 2) == 0) {
    ;
  } else if (strncmp (resp, "NG", 2) == 0) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: unable to post mobmq data: %.*s", (int) len, resp);
  } else {
    FILE    *fh;

    /* this should be the user key */
    strlcpy (tbuff, resp, 3);
    tbuff [2] = '\0';
    mainData->mobmqUserkey = strdup (tbuff);
    /* need to save this for future use */
    pathbldMakePath (tbuff, sizeof (tbuff), "",
        "mmq", ".key", PATHBLD_MP_USEIDX);
    fh = fopen (tbuff, "w");
    fprintf (fh, "%.*s", (int) len, resp);
    fclose (fh);
  }
}


static void
mainConnectProcess (maindata_t *mainData, processconnidx_t idx,
    bdjmsgroute_t route, uint16_t port)
{
  int       err;


  SOCKOF (idx) = sockConnect (port, &err, 1000);
  if (SOCKOF (idx) != INVALID_SOCKET) {
    sockhSendMessage (SOCKOF (idx),
        ROUTE_MAIN, route, MSG_HANDSHAKE, NULL);
  }
}

/* clears both the playlist and music queues */
static void
mainQueueClear (maindata_t *mainData)
{
  int startIdx;

  logMsg (LOG_DBG, LOG_BASIC, "clear music queue");
  queueClear (mainData->playlistQueue, 0);
  startIdx = mainData->musicqCurrentIdx == MUSICQ_A ? 1 : 0;
  musicqClear (mainData->musicQueue, MUSICQ_A, startIdx);
  startIdx = mainData->musicqCurrentIdx == MUSICQ_B ? 1 : 0;
  musicqClear (mainData->musicQueue, MUSICQ_B, startIdx);
  mainData->musicqChanged = true;
}

static void
mainQueueDance (maindata_t *mainData, ssize_t danceIdx, ssize_t count)
{
  playlist_t      *playlist;
  char            plname [40];

  logMsg (LOG_DBG, LOG_BASIC, "queue dance %ld %ld", danceIdx, count);
  snprintf (plname, sizeof (plname), "_main_dance_%ld_%ld",
      danceIdx, globalCounter++);
  playlist = playlistCreate (plname, PLTYPE_AUTO, NULL);
  playlistSetConfigNum (playlist, PLAYLIST_STOP_AFTER, count);
  /* this will also set 'selected' */
  playlistSetDanceCount (playlist, danceIdx, 1);
  logMsg (LOG_DBG, LOG_BASIC, "Queue Playlist: %s", plname);
  slistSetData (mainData->playlistCache, plname, playlist);
  queuePush (mainData->playlistQueue, playlist);
  logMsg (LOG_DBG, LOG_MAIN, "push pl %s", plname);
  mainMusicQueueFill (mainData);
  mainMusicQueuePrep (mainData);
  mainMusicQueuePlay (mainData);
}

static void
mainQueuePlaylist (maindata_t *mainData, char *plname)
{
  playlist_t      *playlist;


  logProcBegin (LOG_PROC, "mainQueuePlaylist");
  playlist = playlistLoad (plname);
  if (playlist != NULL) {
    logMsg (LOG_DBG, LOG_BASIC, "Queue Playlist: %s", plname);
    slistSetData (mainData->playlistCache, plname, playlist);
    queuePush (mainData->playlistQueue, playlist);
    logMsg (LOG_DBG, LOG_MAIN, "push pl %s", plname);
    mainMusicQueueFill (mainData);
    mainMusicQueuePrep (mainData);
  } else {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Queue Playlist failed: %s", plname);
  }
  logProcEnd (LOG_PROC, "mainQueuePlaylist", "");
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
  pltype_t    pltype;


  logProcBegin (LOG_PROC, "mainMusicQueueFill");
  playlist = queueGetCurrent (mainData->playlistQueue);
  pltype = (pltype_t) playlistGetConfigNum (playlist, PLAYLIST_TYPE);

  mqLen = bdjoptGetNum (OPT_G_PLAYERQLEN);
  currlen = musicqGetLen (mainData->musicQueue, mainData->musicqCurrentIdx);
  logMsg (LOG_DBG, LOG_BASIC, "fill: %ld < %ld", currlen, mqLen);
    /* want current + mqLen songs */
  while (playlist != NULL && currlen <= mqLen) {
    song = playlistGetNextSong (playlist, mainData->danceCounts,
        currlen, mainCheckMusicQueue, mainMusicQueueHistory, mainData);
    if (song == NULL) {
      queuePop (mainData->playlistQueue);
      playlist = queueGetCurrent (mainData->playlistQueue);
      continue;
    }
    musicqPush (mainData->musicQueue, mainData->musicqCurrentIdx, song, playlistGetName (playlist));
    mainData->musicqChanged = true;
    currlen = musicqGetLen (mainData->musicQueue, mainData->musicqCurrentIdx);

    if (pltype == PLTYPE_AUTO) {
      playlist = queueGetCurrent (mainData->playlistQueue);
      if (playlist != NULL && song != NULL) {
        playlistAddCount (playlist, song);
      }
    }
  }

  logProcEnd (LOG_PROC, "mainMusicQueueFill", "");
}

static void
mainMusicQueuePrep (maindata_t *mainData)
{
  playlist_t    *playlist;
  ssize_t       plannounce;


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
      musicqSetFlag (mainData->musicQueue, mainData->musicqCurrentIdx,
          i, MUSICQ_FLAG_PREP);

      plannounce = false;
      if (plname != NULL) {
        playlist = slistGetData (mainData->playlistCache, plname);
        plannounce = playlistGetConfigNum (playlist, PLAYLIST_ANNOUNCE);
      }
      annfname = mainPrepSong (mainData, song, sfname, plname, PREP_SONG);

      if (plannounce == 1) {
        if (annfname != NULL && strcmp (annfname, "") != 0 ) {
          musicqSetFlag (mainData->musicQueue, mainData->musicqCurrentIdx,
              i, MUSICQ_FLAG_ANNOUNCE);
          musicqSetAnnounce (mainData->musicQueue, mainData->musicqCurrentIdx,
              i, annfname);
        }
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
  ssize_t       speed = 100;
  double        voladjperc = 0;
  ssize_t       gap = 0;
  ssize_t       plannounce = 0;
  ilistidx_t    danceidx;
  char          *annfname = NULL;


  sfname = songGetData (song, TAG_FILE);
  dur = songGetNum (song, TAG_DURATION);
  voladjperc = songGetDouble (song, TAG_VOLUMEADJUSTPERC);
  if (voladjperc < 0.0) {
    voladjperc = 0.0;
  }
  gap = 0;
  songstart = 0;
  speed = 100;

    /* announcements don't need any of the following... */
  if (flag != PREP_ANNOUNCE) {
    maxdur = bdjoptGetNum (OPT_P_MAXPLAYTIME);
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

    pldur = LIST_VALUE_INVALID;
    if (plname != NULL) {
      playlist = slistGetData (mainData->playlistCache, plname);
      pldur = playlistGetConfigNum (playlist, PLAYLIST_MAX_PLAY_TIME);
    }
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

    /* after adjusting the duration by song start/end, then adjust */
    /* the duration by the speed of the song */
    /* this is the real duration for the song */
    if (speed != 100) {
      double      drate;
      double      ddur;

      drate = (double) speed / 100.0;
      ddur = (double) dur / drate;
      dur = (ssize_t) ddur;
    }

    /* if the playlist has a maximum play time specified for a dance */
    /* it overrides any of the other max play times */
    danceidx = songGetNum (song, TAG_DANCE);
    plddur = playlistGetDanceNum (playlist, danceidx, PLDANCE_MAXPLAYTIME);
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
    if (plannounce == 1) {
      dance_t       *dances;
      song_t        *tsong;

      danceidx = songGetNum (song, TAG_DANCE);
      dances = bdjvarsdf [BDJVDF_DANCES];
      annfname = danceGetData (dances, danceidx, DANCE_ANNOUNCE);
      if (annfname != NULL) {
        ssize_t   tval;

        tsong = dbGetByName (annfname);
        if (tsong != NULL) {
          tval = slistGetNum (mainData->announceList, annfname);
          if (tval == LIST_VALUE_INVALID) {
            /* only prep the announcement if it has not been prepped before */
            mainPrepSong (mainData, tsong, annfname, plname, PREP_ANNOUNCE);
          }
          slistSetNum (mainData->announceList, annfname, 1);
        } else {
          annfname = NULL;
        }
      } /* found an announcement for the dance */
    } /* announcements are on in the playlist */
  } /* if this is a normal song */

  snprintf (tbuff, MAXPATHLEN, "%s%c%zd%c%zd%c%zd%c%.1f%c%zd%c%d", sfname,
      MSG_ARGS_RS, dur, MSG_ARGS_RS, songstart, MSG_ARGS_RS, speed,
      MSG_ARGS_RS, voladjperc, MSG_ARGS_RS, gap, MSG_ARGS_RS, flag);

  logMsg (LOG_DBG, LOG_MAIN, "prep song %s", sfname);
  sockhSendMessage (SOCKOF (PROCESS_PLAYER),
      ROUTE_MAIN, ROUTE_PLAYER, MSG_SONG_PREP, tbuff);
  return annfname;
}

static void
mainTogglePause (maindata_t *mainData, ssize_t idx)
{
  ssize_t       musicqLen;
  musicqflag_t  flags;

  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqCurrentIdx);
  if (idx <= 0 || idx > musicqLen) {
    return;
  }

  flags = musicqGetFlags (mainData->musicQueue, mainData->musicqCurrentIdx, idx);
  if ((flags & MUSICQ_FLAG_PAUSE) == MUSICQ_FLAG_PAUSE) {
    musicqClearFlag (mainData->musicQueue, mainData->musicqCurrentIdx, idx, MUSICQ_FLAG_PAUSE);
  } else {
    musicqSetFlag (mainData->musicQueue, mainData->musicqCurrentIdx, idx, MUSICQ_FLAG_PAUSE);
  }
}

static void
mainMusicqMove (maindata_t *mainData, ssize_t fromidx, mainmove_t direction)
{
  ssize_t       toidx;
  ssize_t       musicqLen;

  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqCurrentIdx);

  toidx = fromidx + direction;
  if (fromidx == 1 && toidx == 0 &&
      mainData->playerState != PL_STATE_STOPPED) {
    return;
  }

  if (toidx > fromidx && fromidx >= musicqLen) {
    return;
  }
  if (toidx < fromidx && fromidx < 1) {
    return;
  }

  musicqMove (mainData->musicQueue, mainData->musicqCurrentIdx, fromidx, toidx);
  mainData->musicqChanged = true;
}

static void
mainMusicqMoveTop (maindata_t *mainData, ssize_t fromidx)
{
  ssize_t       toidx;
  ssize_t       musicqLen;

  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqCurrentIdx);

  if (fromidx >= musicqLen || fromidx <= 1) {
    return;
  }

  toidx = fromidx - 1;
  while (toidx != 0) {
    musicqMove (mainData->musicQueue, mainData->musicqCurrentIdx, fromidx, toidx);
    fromidx--;
    toidx--;
  }
  mainData->musicqChanged = true;
}

static void
mainMusicqClear (maindata_t *mainData, ssize_t idx)
{
  musicqClear (mainData->musicQueue, mainData->musicqCurrentIdx, idx);
  /* there may be other playlists in the playlist queue */
  mainMusicQueueFill (mainData);
  mainMusicQueuePrep (mainData);
  mainData->musicqChanged = true;
}

static void
mainMusicqRemove (maindata_t *mainData, ssize_t idx)
{
  musicqRemove (mainData->musicQueue, mainData->musicqCurrentIdx, idx);
  mainMusicQueueFill (mainData);
  mainMusicQueuePrep (mainData);
  mainData->musicqChanged = true;
}

static void
mainMusicqInsert (maindata_t *mainData, char *args)
{
  char      *tokstr = NULL;
  char      *p = NULL;
  char      *sfname = NULL;
  ssize_t   idx;
  song_t    *song = NULL;

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  idx = atol (p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  sfname = p;

  song = dbGetByName (sfname);

  if (song != NULL) {
    musicqInsert (mainData->musicQueue, mainData->musicqCurrentIdx, idx, song);
    mainData->musicqChanged = true;
  }
}

static void
mainPlaybackBegin (maindata_t *mainData)
{
  musicqflag_t  flags;

  /* if the pause flag is on for this entry in the music queue, */
  /* tell the player to turn on the pause-at-end */
  flags = musicqGetFlags (mainData->musicQueue, mainData->musicqCurrentIdx, 0);
  if ((flags & MUSICQ_FLAG_PAUSE) == MUSICQ_FLAG_PAUSE) {
    sockhSendMessage (SOCKOF (PROCESS_PLAYER),
        ROUTE_MAIN, ROUTE_PLAYER, MSG_PLAY_PAUSEATEND, NULL);
  }
}

static void
mainMusicQueuePlay (maindata_t *mainData)
{
  musicqflag_t      flags;
  char              *sfname;
  song_t            *song;

  logProcBegin (LOG_PROC, "mainMusicQueuePlay");
  logMsg (LOG_DBG, LOG_BASIC, "playerState: %d", mainData->playerState);
  if (mainData->playerState == PL_STATE_STOPPED) {
      /* grab a song out of the music queue and start playing */
    logMsg (LOG_DBG, LOG_MAIN, "player is stopped, get song, start");
    song = musicqGetCurrent (mainData->musicQueue, mainData->musicqCurrentIdx);
    if (song != NULL) {
      flags = musicqGetFlags (mainData->musicQueue, mainData->musicqCurrentIdx, 0);
      if ((flags & MUSICQ_FLAG_ANNOUNCE) == MUSICQ_FLAG_ANNOUNCE) {
        char      *annfname;

        annfname = musicqGetAnnounce (mainData->musicQueue, mainData->musicqCurrentIdx, 0);
        if (annfname != NULL) {
          sockhSendMessage (SOCKOF (PROCESS_PLAYER),
              ROUTE_MAIN, ROUTE_PLAYER, MSG_SONG_PLAY, annfname);
        }
      }
      sfname = songGetData (song, TAG_FILE);
      sockhSendMessage (SOCKOF (PROCESS_PLAYER),
          ROUTE_MAIN, ROUTE_PLAYER, MSG_SONG_PLAY, sfname);
    }
  }
  if (mainData->playerState == PL_STATE_IN_GAP ||
      mainData->playerState == PL_STATE_PAUSED) {
    logMsg (LOG_DBG, LOG_MAIN, "player is paused/in gap, send play msg");
    sockhSendMessage (SOCKOF (PROCESS_PLAYER),
        ROUTE_MAIN, ROUTE_PLAYER, MSG_PLAY_PLAY, NULL);
  }
  logProcEnd (LOG_PROC, "mainMusicQueuePlay", "");
}

static void
mainMusicQueueFinish (maindata_t *mainData)
{
  playlist_t    *playlist;
  song_t        *song;
  ilistidx_t    danceIdx;
  char          *plname;

  logProcBegin (LOG_PROC, "mainMusicQueueFinish");

  /* let the playlist know this song has been played */
  song = musicqGetCurrent (mainData->musicQueue, mainData->musicqCurrentIdx);
  plname = musicqGetPlaylistName (mainData->musicQueue, mainData->musicqCurrentIdx, 0);
  if (plname != NULL) {
    playlist = slistGetData (mainData->playlistCache, plname);
    if (playlist != NULL && song != NULL) {
      playlistAddPlayed (playlist, song);
    }
  }
  /* update the dance counts */
  if (song != NULL) {
    danceIdx = songGetNum (song, TAG_DANCE);
    if (danceIdx >= 0) {
      nlistDecrement (mainData->danceCounts, danceIdx);
    }
  }

  mainData->playerState = PL_STATE_STOPPED;
  musicqPop (mainData->musicQueue, mainData->musicqCurrentIdx);
  mainData->musicqChanged = true;
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

static ssize_t
mainMusicQueueHistory (void *tmaindata, ssize_t idx)
{
  maindata_t    *mainData = tmaindata;
  ilistidx_t    didx;
  song_t        *song;

  if (idx < 0 ||
      idx >= musicqGetLen (mainData->musicQueue, mainData->musicqCurrentIdx)) {
    return -1;
  }

  song = musicqGetByIdx (mainData->musicQueue, mainData->musicqCurrentIdx, idx);
  didx = songGetNum (song, TAG_DANCE);
  return didx;
}

static void
mainForceStop (maindata_t *mainData, char *lockfn,
    int flags, processconnidx_t idx)
{
  pid_t pid;
  int   count = 0;
  int   exists = 0;

  pid = lockExists (lockfn, flags);
  if (pid != 0) {
    logMsg (LOG_DBG, LOG_MAIN, "%s still extant; wait", lockfn);
  }
  while (pid != 0 && count < 10) {
    exists = 1;
    if (! processExists (mainData->processes [idx])) {
      logMsg (LOG_DBG, LOG_MAIN, "%s exited", lockfn);
      exists = 0;
      break;
    }
    ++count;
    mssleep (100);
  }

  if (exists) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "force-terminating %s", lockfn);
    processKill (mainData->processes [idx]);
  }
}

static int
mainStartProcess (maindata_t *mainData, processconnidx_t idx, char *fname)
{
  char      tbuff [MAXPATHLEN];
  char      *extension = NULL;
  int       rc = -1;

  logProcBegin (LOG_PROC, "mainStartProcess");
  mainData->processconns [idx].name = fname;
  if (mainData->processconns [idx].started) {
    logProcEnd (LOG_PROC, "mainStartProcess", "already");
    return -1;
  }

  extension = "";
  if (isWindows ()) {
    extension = ".exe";
  }
  pathbldMakePath (tbuff, sizeof (tbuff), "",
      fname, extension, PATHBLD_MP_EXECDIR);
  mainData->processes [idx] = processStart (tbuff, lsysvars [SVL_BDJIDX],
      bdjoptGetNum (OPT_G_DEBUGLVL));
  if (rc < 0) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "%s %s failed to start", fname, tbuff);
  } else {
    logMsg (LOG_DBG, LOG_BASIC, "%s started pid: %zd", fname,
        (ssize_t) mainData->processes [idx]->pid);
    mainData->processconns [idx].started = true;
  }
  logProcEnd (LOG_PROC, "mainStartProcess", "");
  return rc;
}

static void
mainStopProcess (maindata_t *mainData, processconnidx_t idx,
    bdjmsgroute_t route, char *lockfn, bool force)
{
  logProcBegin (LOG_PROC, "mainStopProcess");
  if (! force) {
    if (! mainData->processconns [idx].started) {
      logProcEnd (LOG_PROC, "mainStopProcess", "not-started");
      return;
    }
    sockhSendMessage (SOCKOF (idx), ROUTE_MAIN, route,
        MSG_EXIT_REQUEST, NULL);
    sockhSendMessage (SOCKOF (idx), ROUTE_MAIN, route,
        MSG_SOCKET_CLOSE, NULL);
    sockClose (mainData->processconns [idx].sock);
    mainData->processconns [idx].sock = INVALID_SOCKET;
    mainData->processconns [idx].started = false;
    mainData->processconns [idx].handshake = false;
  }
  if (force) {
    mainForceStop (mainData, lockfn, PATHBLD_MP_USEIDX, idx);
  }
  logProcEnd (LOG_PROC, "mainStopProcess", "");
}

static void
mainSendDanceList (maindata_t *mainData)
{
  dance_t       *dances;
  slist_t       *danceList;
  slistidx_t     idx;
  char          *dancenm;
  char          tbuff [200];
  char          rbuff [3096];
  slistidx_t    iteridx;

  dances = bdjvarsdf [BDJVDF_DANCES];
  danceList = danceGetDanceList (dances);

  rbuff [0] = '\0';
  slistStartIterator (danceList, &iteridx);
  while ((dancenm = slistIterateKey (danceList, &iteridx)) != NULL) {
    idx = slistGetNum (danceList, dancenm);
    snprintf (tbuff, sizeof (tbuff), "<option value=\"%zd\">%s</option>\n", idx, dancenm);
    strlcat (rbuff, tbuff, sizeof (rbuff));
  }

  sockhSendMessage (SOCKOF (PROCESS_REMCTRL),
      ROUTE_MAIN, ROUTE_REMCTRL, MSG_DANCE_LIST_DATA, rbuff);
}

static void
mainSendPlaylistList (maindata_t *mainData)
{
  slist_t       *plList;
  char          *plfnm;
  char          *plnm;
  char          tbuff [200];
  char          rbuff [3096];
  slistidx_t    iteridx;

  plList = playlistGetPlaylistList ();

  rbuff [0] = '\0';
  slistStartIterator (plList, &iteridx);
  while ((plnm = slistIterateKey (plList, &iteridx)) != NULL) {
    plfnm = listGetData (plList, plnm);
    snprintf (tbuff, sizeof (tbuff), "<option value=\"%s\">%s</option>\n", plfnm, plnm);
    strlcat (rbuff, tbuff, sizeof (rbuff));
  }

  slistFree (plList);

  sockhSendMessage (SOCKOF (PROCESS_REMCTRL),
      ROUTE_MAIN, ROUTE_REMCTRL, MSG_PLAYLIST_LIST_DATA, rbuff);
}


static void
mainSendStatus (maindata_t *mainData, char *playerResp)
{
  char    tbuff [200];
  char    tbuff2 [40];
  char    rbuff [3096];
  ssize_t musicqLen;
  char    *data = NULL;
  char    *tokstr = NULL;
  char    *p;

  strlcpy (rbuff, "{ ", sizeof (rbuff));

  p = strtok_r (playerResp, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"playstate\" : \"%s\"", p);
  strlcat (rbuff, tbuff, sizeof (rbuff));

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"repeat\" : \"%s\"", p);
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"pauseatend\" : \"%s\"", p);
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"vol\" : \"%s%%\"", p);
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"speed\" : \"%s%%\"", p);
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"playedtime\" : \"%s\"", tmutilToMS (atol (p), tbuff2, sizeof (tbuff2)));
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"duration\" : \"%s\"", tmutilToMS (atol (p), tbuff2, sizeof (tbuff2)));
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqCurrentIdx);
  snprintf (tbuff, sizeof (tbuff),
      "\"qlength\" : \"%zd\"", musicqLen);
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  data = musicqGetDance (mainData->musicQueue, mainData->musicqCurrentIdx, 0);
  if (data == NULL) { data = ""; }
  snprintf (tbuff, sizeof (tbuff),
      "\"dance\" : \"%s\"", data);
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  data = musicqGetData (mainData->musicQueue, mainData->musicqCurrentIdx, 0, TAG_ARTIST);
  if (data == NULL) { data = ""; }
  snprintf (tbuff, sizeof (tbuff),
      "\"artist\" : \"%s\"", data);
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));
  data = musicqGetData (mainData->musicQueue, mainData->musicqCurrentIdx, 0, TAG_TITLE);
  if (data == NULL) { data = ""; }
  snprintf (tbuff, sizeof (tbuff),
      "\"title\" : \"%s\"", data);
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  strlcat (rbuff, " }", sizeof (rbuff));

  sockhSendMessage (SOCKOF (PROCESS_REMCTRL),
      ROUTE_MAIN, ROUTE_REMCTRL, MSG_STATUS_DATA, rbuff);

  /* build a message and send it to the gui */

}

static void
mainDanceCountsInit (maindata_t *mainData)
{
  nlist_t     *dc;
  ilistidx_t  didx;
  nlistidx_t  iteridx;

  dc = dbGetDanceCounts ();

  if (mainData->danceCounts != NULL) {
    nlistFree (mainData->danceCounts);
  }

  mainData->danceCounts = nlistAlloc ("main-dancecounts", LIST_ORDERED, NULL);
  nlistSetSize (mainData->danceCounts, nlistGetCount (dc));

  nlistStartIterator (dc, &iteridx);
  while ((didx = nlistIterateKey (dc, &iteridx)) >= 0) {
    nlistSetNum (mainData->danceCounts, didx, nlistGetNum (dc, didx));
  }
}
