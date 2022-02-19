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
#include "bdjvarsdf.h"
#include "bdjvars.h"
#include "conn.h"
#include "dance.h"
#include "dancesel.h"
#include "filedata.h"
#include "lock.h"
#include "log.h"
#include "musicdb.h"
#include "musicq.h"
#include "pathbld.h"
#include "playlist.h"
#include "procutil.h"
#include "progstate.h"
#include "queue.h"
#include "slist.h"
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

#define PREP_SIZE     5
#define MAIN_NOT_SET  -1

/* playlistCache contains all of the playlists that have been seen */
/* so that playlist lookups can be processed */
typedef struct {
  progstate_t       *progstate;
  procutil_t         *processes [ROUTE_MAX];
  conn_t            *conn;
  slist_t           *playlistCache;
  queue_t           *playlistQueue [MUSICQ_MAX];
  musicq_t          *musicQueue;
  nlist_t           *danceCounts;
  musicqidx_t       musicqPlayIdx;
  musicqidx_t       musicqManageIdx;
  int               musicqDeferredPlayIdx;
  slist_t           *announceList;
  playerstate_t     playerState;
  ssize_t           gap;
  webclient_t       *webclient;
  char              *mobmqUserkey;
  bool              musicqChanged [MUSICQ_MAX];
  bool              marqueeChanged [MUSICQ_MAX];
  bool              playWhenQueued : 1;
  bool              switchQueueWhenEmpty : 1;
} maindata_t;

static int      mainProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static int      mainProcessing (void *udata);
static bool     mainListeningCallback (void *tmaindata, programstate_t programState);
static bool     mainConnectingCallback (void *tmaindata, programstate_t programState);
static bool     mainHandshakeCallback (void *tmaindata, programstate_t programState);
static bool     mainStoppingCallback (void *tmaindata, programstate_t programState);
static bool     mainClosingCallback (void *tmaindata, programstate_t programState);
static void     mainSendMusicQueueData (maindata_t *mainData, int musicqidx);
static void     mainSendMarqueeData (maindata_t *mainData);
static void     mainSendMobileMarqueeData (maindata_t *mainData);
static void     mainMobilePostCallback (void *userdata, char *resp, size_t len);
static void     mainQueueClear (maindata_t *mainData, char *args);
static void     mainQueueDance (maindata_t *mainData, char *args, ssize_t count);
static void     mainQueuePlaylist (maindata_t *mainData, char *plname);
static void     mainSigHandler (int sig);
static void     mainMusicQueueFill (maindata_t *mainData);
static void     mainMusicQueuePrep (maindata_t *mainData);
static char     *mainPrepSong (maindata_t *maindata, song_t *song,
                    char *sfname, char *plname, bdjmsgprep_t flag);
static void     mainTogglePause (maindata_t *mainData, char *args);
static void     mainMusicqMove (maindata_t *mainData, char *args, mainmove_t direction);
static void     mainMusicqMoveTop (maindata_t *mainData, char *args);
static void     mainMusicqClear (maindata_t *mainData, char *args);
static void     mainMusicqRemove (maindata_t *mainData, char *args);
static void     mainMusicqInsert (maindata_t *mainData, char *args);
static void     mainMusicqSetPlayback (maindata_t *mainData, char *args);
static void     mainMusicqSwitch (maindata_t *mainData, musicqidx_t newidx);
static void     mainPlaybackBegin (maindata_t *mainData);
static void     mainMusicQueuePlay (maindata_t *mainData);
static void     mainMusicQueueFinish (maindata_t *mainData);
static void     mainMusicQueueNext (maindata_t *mainData);
static bool     mainCheckMusicQueue (song_t *song, void *tdata);
static ssize_t  mainMusicQueueHistory (void *mainData, ssize_t idx);
static void     mainSendDanceList (maindata_t *mainData, bdjmsgroute_t route);
static void     mainSendPlaylistList (maindata_t *mainData, bdjmsgroute_t route);
static void     mainSendPlayerStatus (maindata_t *mainData, char *playerResp);
static void     mainSendMusicqStatus (maindata_t *mainData, char *rbuff, size_t siz);
static void     mainDanceCountsInit (maindata_t *mainData);
static void     mainParseIntNum (char *args, int *a, ssize_t *b);
static void     mainParseIntStr (char *args, int *a, char **b);

static long globalCounter = 0;
static int  gKillReceived = 0;

int
main (int argc, char *argv[])
{
  maindata_t    mainData;
  uint16_t      listenPort;


  mainData.progstate = progstateInit ("main");
  progstateSetCallback (mainData.progstate, STATE_LISTENING,
      mainListeningCallback, &mainData);
  progstateSetCallback (mainData.progstate, STATE_CONNECTING,
      mainConnectingCallback, &mainData);
  progstateSetCallback (mainData.progstate, STATE_WAIT_HANDSHAKE,
      mainHandshakeCallback, &mainData);
  progstateSetCallback (mainData.progstate, STATE_STOPPING,
      mainStoppingCallback, &mainData);
  progstateSetCallback (mainData.progstate, STATE_CLOSING,
      mainClosingCallback, &mainData);
  mainData.playlistCache = NULL;
  mainData.musicQueue = NULL;
  mainData.danceCounts = NULL;
  mainData.musicqPlayIdx = MUSICQ_A;
  mainData.musicqManageIdx = MUSICQ_A;
  mainData.musicqDeferredPlayIdx = MAIN_NOT_SET;
  mainData.playerState = PL_STATE_STOPPED;
  mainData.webclient = NULL;
  mainData.mobmqUserkey = NULL;
  mainData.playWhenQueued = true;
  mainData.switchQueueWhenEmpty = true;
  for (int i = 0; i < MUSICQ_MAX; ++i) {
    mainData.playlistQueue [i] = NULL;
    mainData.musicqChanged [i] = false;
    mainData.marqueeChanged [i] = false;
  }
  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    mainData.processes [i] = NULL;
  }

#if _define_SIGHUP
  procutilCatchSignal (mainSigHandler, SIGHUP);
#endif
  procutilCatchSignal (mainSigHandler, SIGINT);
  procutilCatchSignal (mainSigHandler, SIGTERM);
#if _define_SIGCHLD
  procutilIgnoreSignal (SIGCHLD);
#endif

  bdj4startup (argc, argv, "m", ROUTE_MAIN);
  logProcBegin (LOG_PROC, "main");

  mainData.conn = connInit (ROUTE_MAIN);
  mainData.gap = bdjoptGetNum (OPT_P_GAP);
  mainData.playlistCache = slistAlloc ("playlist-list", LIST_ORDERED,
      free, playlistFree);
  mainData.playlistQueue [MUSICQ_A] = queueAlloc (NULL);
  mainData.playlistQueue [MUSICQ_B] = queueAlloc (NULL);
  mainData.musicQueue = musicqAlloc ();
  mainDanceCountsInit (&mainData);
  mainData.announceList = slistAlloc ("announcements", LIST_ORDERED,
      free, NULL);

  listenPort = bdjvarsGetNum (BDJVL_MAIN_PORT);
  sockhMainLoop (listenPort, mainProcessMsg, mainProcessing, &mainData);

  logProcEnd (LOG_PROC, "main", "");
  while (progstateShutdownProcess (mainData.progstate) != STATE_CLOSED) {
    ;
  }
  progstateFree (mainData.progstate);
  logEnd ();
  return 0;
}

/* internal routines */

static bool
mainStoppingCallback (void *tmaindata, programstate_t programState)
{
  maindata_t    *mainData = tmaindata;

  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    if (mainData->processes [i] != NULL) {
      procutilStopProcess (mainData->processes [i], mainData->conn, i, false);
    }
  }
  mssleep (200);

  return true;
}

static bool
mainClosingCallback (void *tmaindata, programstate_t programState)
{
  maindata_t    *mainData = tmaindata;

  connFree (mainData->conn);

  if (mainData->playlistCache != NULL) {
    slistFree (mainData->playlistCache);
  }
  if (mainData->playlistQueue [MUSICQ_A] != NULL) {
    queueFree (mainData->playlistQueue [MUSICQ_A]);
  }
  if (mainData->playlistQueue [MUSICQ_B] != NULL) {
    queueFree (mainData->playlistQueue [MUSICQ_B]);
  }
  if (mainData->musicQueue != NULL) {
    musicqFree (mainData->musicQueue);
  }
  if (mainData->announceList != NULL) {
    slistFree (mainData->announceList);
  }
  if (mainData->webclient != NULL) {
    webclientClose (mainData->webclient);
  }
  if (mainData->mobmqUserkey != NULL) {
    free (mainData->mobmqUserkey);
  }
  if (mainData->danceCounts != NULL) {
    nlistFree (mainData->danceCounts);
  }
  bdj4shutdown (ROUTE_MAIN);
  /* give the other processes some time to shut down */
  mssleep (200);
  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    if (mainData->processes [i] != NULL) {
      procutilStopProcess (mainData->processes [i], mainData->conn, i, true);
      procutilFree (mainData->processes [i]);
    }
  }
  return true;
}

static int
mainProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  maindata_t      *mainData;

  logProcBegin (LOG_PROC, "mainProcessMsg");
  mainData = (maindata_t *) udata;

  /* this just reduces the amount of stuff in the log */
  if (msg != MSG_MUSICQ_STATUS_DATA && msg != MSG_PLAYER_STATUS_DATA) {
    logMsg (LOG_DBG, LOG_MSGS, "got: from: %ld route: %ld msg:%ld args:%s",
        routefrom, route, msg, args);
  }

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MAIN: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (mainData->conn, routefrom);
          connConnectResponse (mainData->conn, routefrom);
          break;
        }
        case MSG_SOCKET_CLOSE: {
          connDisconnect (mainData->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          progstateShutdownProcess (mainData->progstate);
          logProcEnd (LOG_PROC, "mainProcessMsg", "req-exit");
          return 1;
        }
        case MSG_PLAYLIST_CLEARPLAY: {
          mainQueueClear (mainData, args);
          /* clear out any playing song */
          connSendMessage (mainData->conn, ROUTE_PLAYER,
              MSG_PLAY_NEXTSONG, NULL);
          mainQueuePlaylist (mainData, args);
          break;
        }
        case MSG_QUEUE_CLEAR: {
          /* clears both the playlist queue and the music queue */
          logMsg (LOG_DBG, LOG_MSGS, "got: queue-clear");
          mainQueueClear (mainData, args);
          break;
        }
        case MSG_QUEUE_PLAYLIST: {
          logMsg (LOG_DBG, LOG_MSGS, "got: playlist-queue %s", args);
          mainQueuePlaylist (mainData, args);
          break;
        }
        case MSG_QUEUE_DANCE: {
          mainQueueDance (mainData, args, 1);
          break;
        }
        case MSG_QUEUE_DANCE_5: {
          mainQueueDance (mainData, args, 5);
          break;
        }
        case MSG_QUEUE_PLAY_ON_ADD: {
          mainData->playWhenQueued = atoi (args);
          break;
        }
        case MSG_QUEUE_SWITCH_EMPTY: {
          mainData->switchQueueWhenEmpty = atoi (args);
          break;
        }
        case MSG_PLAY_PLAY: {
          mainMusicQueuePlay (mainData);
          break;
        }
        case MSG_PLAY_PLAYPAUSE: {
          if (mainData->playerState == PL_STATE_PLAYING ||
              mainData->playerState == PL_STATE_IN_FADEOUT) {
            connSendMessage (mainData->conn, ROUTE_PLAYER,
                MSG_PLAY_PAUSE, NULL);
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
          mainTogglePause (mainData, args);
          break;
        }
        case MSG_MUSICQ_MOVE_DOWN: {
          mainMusicqMove (mainData, args, MOVE_DOWN);
          break;
        }
        case MSG_MUSICQ_MOVE_TOP: {
          mainMusicqMoveTop (mainData, args);
          break;
        }
        case MSG_MUSICQ_MOVE_UP: {
          mainMusicqMove (mainData, args, MOVE_UP);
          break;
        }
        case MSG_MUSICQ_REMOVE: {
          mainMusicqRemove (mainData, args);
          break;
        }
        case MSG_MUSICQ_TRUNCATE: {
          mainMusicqClear (mainData, args);
          break;
        }
        case MSG_MUSICQ_INSERT: {
          mainMusicqInsert (mainData, args);
          break;
        }
        case MSG_MUSICQ_SET_PLAYBACK: {
          mainMusicqSetPlayback (mainData, args);
          break;
        }
        case MSG_PLAYER_STATE: {
          mainData->playerState = (playerstate_t) atol (args);
          logMsg (LOG_DBG, LOG_MSGS, "got: player-state: %d", mainData->playerState);
          mainData->marqueeChanged [mainData->musicqPlayIdx] = true;
          break;
        }
        case MSG_GET_DANCE_LIST: {
          mainSendDanceList (mainData, routefrom);
          break;
        }
        case MSG_GET_PLAYLIST_LIST: {
          mainSendPlaylistList (mainData, routefrom);
          break;
        }
        case MSG_PLAYER_STATUS_DATA: {
          mainSendPlayerStatus (mainData, args);
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

  if (! progstateIsRunning (mainData->progstate)) {
    progstateProcess (mainData->progstate);
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    }
    return gKillReceived;
  }

  for (int i = 0; i < MUSICQ_MAX; ++i) {
    if (mainData->musicqChanged [i]) {
      mainSendMusicQueueData (mainData, i);
      mainData->musicqChanged [i] = false;
    }
  }
  /* for the marquee, only the currently selected playback queue is */
  /* of interest */
  if (mainData->marqueeChanged [mainData->musicqPlayIdx]) {
    mainSendMarqueeData (mainData);
    mainSendMobileMarqueeData (mainData);
    mainData->marqueeChanged [mainData->musicqPlayIdx] = false;
  }

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  return gKillReceived;
}

static bool
mainListeningCallback (void *tmaindata, programstate_t programState)
{
  maindata_t    *mainData = tmaindata;

  mainData->processes [ROUTE_PLAYER] = procutilStartProcess (
      ROUTE_PLAYER, "bdj4player");
  if (bdjoptGetNum (OPT_P_MOBILEMARQUEE) != MOBILEMQ_OFF) {
    mainData->processes [ROUTE_MOBILEMQ] = procutilStartProcess (
        ROUTE_MOBILEMQ, "bdj4mobmq");
  }
  if (bdjoptGetNum (OPT_P_REMOTECONTROL)) {
    mainData->processes [ROUTE_REMCTRL] = procutilStartProcess (
        ROUTE_REMCTRL, "bdj4rc");
  }
  mainData->processes [ROUTE_MARQUEE] = procutilStartProcess (
      ROUTE_MARQUEE, "bdj4marquee");
  return true;
}

static bool
mainConnectingCallback (void *tmaindata, programstate_t programState)
{
  maindata_t    *mainData = tmaindata;
  int           connMax = 0;
  int           connCount = 0;
  bool          rc = false;

  if (! connIsConnected (mainData->conn, ROUTE_PLAYER)) {
    connConnect (mainData->conn, ROUTE_PLAYER);
  }
  if (bdjoptGetNum (OPT_P_MOBILEMARQUEE) != MOBILEMQ_OFF) {
    if (! connIsConnected (mainData->conn, ROUTE_MOBILEMQ)) {
      connConnect (mainData->conn, ROUTE_MOBILEMQ);
    }
  }
  if (bdjoptGetNum (OPT_P_REMOTECONTROL)) {
    if (! connIsConnected (mainData->conn, ROUTE_REMCTRL)) {
      connConnect (mainData->conn, ROUTE_REMCTRL);
    }
  }
  if (! connIsConnected (mainData->conn, ROUTE_MARQUEE)) {
    connConnect (mainData->conn, ROUTE_MARQUEE);
  }

  ++connMax;
  if (connIsConnected (mainData->conn, ROUTE_PLAYER)) {
    ++connCount;
  }
  if (bdjoptGetNum (OPT_P_MOBILEMARQUEE) != MOBILEMQ_OFF) {
    ++connMax;
    if (connIsConnected (mainData->conn, ROUTE_MOBILEMQ)) {
      ++connCount;
    }
  }
  if (bdjoptGetNum (OPT_P_REMOTECONTROL)) {
    ++connMax;
    if (connIsConnected (mainData->conn, ROUTE_REMCTRL)) {
      ++connCount;
    }
  }
  ++connMax;
  if (connIsConnected (mainData->conn, ROUTE_MARQUEE)) {
    ++connCount;
  }

  if (connCount == connMax) {
    rc = true;
  }

  return rc;
}

static bool
mainHandshakeCallback (void *tmaindata, programstate_t programState)
{
  maindata_t    *mainData = tmaindata;
  bool          rc = false;

  if (connHaveHandshake (mainData->conn, ROUTE_PLAYER)) {
    rc = true;
  }
  return rc;
}

static void
mainSendMusicQueueData (maindata_t *mainData, int musicqidx)
{
  char        tbuff [200];
  char        sbuff [4100];
  ssize_t     musicqLen;
  slistidx_t  dbidx;
  song_t      *song;
  int         flags;
  int         pflag;
  int         dispidx;
  int         uniqueidx;


  musicqLen = musicqGetLen (mainData->musicQueue, musicqidx);

  sbuff [0] = '\0';
  snprintf (sbuff, sizeof (sbuff), "%d%c", musicqidx, MSG_ARGS_RS);

  for (ssize_t i = 1; i <= musicqLen; ++i) {
    song = musicqGetByIdx (mainData->musicQueue, musicqidx, i);
    if (song != NULL) {
      dispidx = musicqGetDispIdx (mainData->musicQueue, musicqidx, i);
      snprintf (tbuff, sizeof (tbuff), "%d%c", dispidx, MSG_ARGS_RS);
      strlcat (sbuff, tbuff, sizeof (sbuff));
      uniqueidx = musicqGetUniqueIdx (mainData->musicQueue, musicqidx, i);
      snprintf (tbuff, sizeof (tbuff), "%d%c", uniqueidx, MSG_ARGS_RS);
      strlcat (sbuff, tbuff, sizeof (sbuff));
      dbidx = songGetNum (song, TAG_DBIDX);
      snprintf (tbuff, sizeof (tbuff), "%zd%c", dbidx, MSG_ARGS_RS);
      strlcat (sbuff, tbuff, sizeof (sbuff));
      flags = musicqGetFlags (mainData->musicQueue, musicqidx, i);
      pflag = false;
      if ((flags & MUSICQ_FLAG_PAUSE) == MUSICQ_FLAG_PAUSE) {
        pflag = true;
      }
      snprintf (tbuff, sizeof (tbuff), "%d%c", pflag, MSG_ARGS_RS);
      strlcat (sbuff, tbuff, sizeof (sbuff));
    }
  }

  connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_MUSIC_QUEUE_DATA, sbuff);
}

static void
mainSendMarqueeData (maindata_t *mainData)
{
  char        tbuff [200];
  char        sbuff [3096];
  char        *dstr;
  char        *tstr;
  ssize_t     mqLen;
  ssize_t     musicqLen;
  song_t      *song;


  mqLen = bdjoptGetNum (OPT_P_MQQLEN);
  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);

  /* artist/title, dance(s) */

  sbuff [0] = '\0';

  dstr = musicqGetData (mainData->musicQueue, mainData->musicqPlayIdx, 0, TAG_ARTIST);
  if (dstr == NULL || *dstr == '\0') { dstr = MSG_ARGS_EMPTY_STR; }
  tstr = musicqGetData (mainData->musicQueue, mainData->musicqPlayIdx, 0, TAG_TITLE);
  if (tstr == NULL || *tstr == '\0') { tstr = MSG_ARGS_EMPTY_STR; }
  snprintf (tbuff, sizeof (tbuff), "%s%c%s%c", dstr, MSG_ARGS_RS, tstr, MSG_ARGS_RS);
  strlcat (sbuff, tbuff, sizeof (sbuff));

  if (musicqLen > 0) {
    for (ssize_t i = 0; i <= mqLen; ++i) {
      if ((i > 0 && mainData->playerState == PL_STATE_IN_GAP) ||
          (i > 1 && mainData->playerState == PL_STATE_IN_FADEOUT)) {
        dstr = MSG_ARGS_EMPTY_STR;
      } else if (i > musicqLen) {
        dstr = MSG_ARGS_EMPTY_STR;
      } else {
        tstr = NULL;
        song = musicqGetByIdx (mainData->musicQueue, mainData->musicqPlayIdx, i);
        if (song != NULL) {
          /* if the song has an unknown dance, the marquee display */
          /* will be filled in with the dance name. */
          tstr = songGetData (song, TAG_MQDISPLAY);
        }
        if (tstr != NULL && *tstr) {
          dstr = tstr;
        } else {
          dstr = musicqGetDance (mainData->musicQueue, mainData->musicqPlayIdx, i);
        }
        if (dstr == NULL) {
          dstr = MSG_ARGS_EMPTY_STR;
        }
      }

      snprintf (tbuff, sizeof (tbuff), "%s%c", dstr, MSG_ARGS_RS);
      strlcat (sbuff, tbuff, sizeof (sbuff));
    }
  }

  connSendMessage (mainData->conn, ROUTE_MARQUEE,
      MSG_MARQUEE_DATA, sbuff);
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

  mqLen = bdjoptGetNum (OPT_P_MQQLEN);
  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);
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
        tstr = NULL;
        song = musicqGetByIdx (mainData->musicQueue, mainData->musicqPlayIdx, i);
        if (song != NULL) {
          /* if the song has an unknown dance, the marquee display */
          /* will be filled in with the dance name. */
          tstr = songGetData (song, TAG_MQDISPLAY);
        }
        if (tstr != NULL) {
          dstr = tstr;
        } else {
          dstr = musicqGetDance (mainData->musicQueue, mainData->musicqPlayIdx, i);
        }
        if (dstr == NULL) {
          dstr = "";
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

  connSendMessage (mainData->conn, ROUTE_MOBILEMQ,
      MSG_MARQUEE_DATA, jbuff);

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
        "https://%s/marquee4.php", sysvarsGetStr (SV_MOBMQ_HOST));
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

/* clears both the playlist and music queues */
static void
mainQueueClear (maindata_t *mainData, char *args)
{
  int   mi;
  int   startpos = 0;
  char  *p;
  char  *tokstr = NULL;


  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  mi = atoi (p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  startpos = atoi (p);

  mainData->musicqManageIdx = mi;

  logMsg (LOG_DBG, LOG_BASIC, "clear music queue");
  queueClear (mainData->playlistQueue [mi], 0);
  musicqClear (mainData->musicQueue, mi, startpos);
  mainData->musicqChanged [mi] = true;
  mainData->marqueeChanged [mi] = true;
}

static void
mainQueueDance (maindata_t *mainData, char *args, ssize_t count)
{
  playlist_t      *playlist;
  char            plname [40];
  int             mi;
  ssize_t         danceIdx;


  mainParseIntNum (args, &mi, &danceIdx);
  mainData->musicqManageIdx = mi;

  logMsg (LOG_DBG, LOG_BASIC, "queue dance %d %zd %zd", mi, danceIdx, count);
  snprintf (plname, sizeof (plname), "_main_dance_%zd_%ld",
      danceIdx, globalCounter++);
  playlist = playlistCreate (plname, PLTYPE_AUTO, NULL);
  playlistSetConfigNum (playlist, PLAYLIST_STOP_AFTER, count);
  /* this will also set 'selected' */
  playlistSetDanceCount (playlist, danceIdx, 1);
  logMsg (LOG_DBG, LOG_BASIC, "Queue Playlist: %s", plname);
  slistSetData (mainData->playlistCache, plname, playlist);
  queuePush (mainData->playlistQueue [mi], playlist);
  logMsg (LOG_DBG, LOG_MAIN, "push pl %s", plname);
  mainMusicQueueFill (mainData);
  mainMusicQueuePrep (mainData);
  mainData->marqueeChanged [mi] = true;
  mainData->musicqChanged [mi] = true;
  mainSendMusicqStatus (mainData, NULL, 0);
  if (mainData->playWhenQueued &&
      mainData->musicqPlayIdx == (musicqidx_t) mi) {
    mainMusicQueuePlay (mainData);
  }
}

static void
mainQueuePlaylist (maindata_t *mainData, char *args)
{
  int             mi;
  playlist_t      *playlist;
  char            *plname;


  mainParseIntStr (args, &mi, &plname);
  mainData->musicqManageIdx = mi;

  logProcBegin (LOG_PROC, "mainQueuePlaylist");
  playlist = playlistLoad (plname);
  if (playlist != NULL) {
    logMsg (LOG_DBG, LOG_BASIC, "Queue Playlist: %d %s", mi, plname);
    slistSetData (mainData->playlistCache, plname, playlist);
    queuePush (mainData->playlistQueue [mainData->musicqManageIdx], playlist);
    logMsg (LOG_DBG, LOG_MAIN, "push pl %s", plname);
    mainMusicQueueFill (mainData);
    mainMusicQueuePrep (mainData);
    mainData->marqueeChanged [mi] = true;
    mainData->musicqChanged [mi] = true;

    mainSendMusicqStatus (mainData, NULL, 0);
    if (mainData->playWhenQueued &&
        mainData->musicqPlayIdx == (musicqidx_t) mi) {
      mainMusicQueuePlay (mainData);
    }
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
  playlist = queueGetCurrent (mainData->playlistQueue [mainData->musicqManageIdx]);
  pltype = (pltype_t) playlistGetConfigNum (playlist, PLAYLIST_TYPE);

  mqLen = bdjoptGetNum (OPT_G_PLAYERQLEN);
  currlen = musicqGetLen (mainData->musicQueue, mainData->musicqManageIdx);

  /* if this is not the queue selected for playback, then push  */
  /* an empty musicq item on to the head of the queue.          */
  /* this allows the display to work properly for the user      */
  /* when multiple queues are in use.                           */

  if (currlen == 0 &&
      mainData->musicqPlayIdx != mainData->musicqManageIdx) {
    musicqPushHeadEmpty (mainData->musicQueue, mainData->musicqManageIdx);
    ++currlen;
  }

  logMsg (LOG_DBG, LOG_BASIC, "fill: %ld < %ld", currlen, mqLen);
  /* want current + mqLen songs */
  while (playlist != NULL && currlen <= mqLen) {
    song = playlistGetNextSong (playlist, mainData->danceCounts,
        currlen, mainCheckMusicQueue, mainMusicQueueHistory, mainData);
    if (song == NULL) {
      queuePop (mainData->playlistQueue [mainData->musicqManageIdx]);
      playlist = queueGetCurrent (mainData->playlistQueue [mainData->musicqManageIdx]);
      continue;
    }
    musicqPush (mainData->musicQueue, mainData->musicqManageIdx, song, playlistGetName (playlist));
    mainData->musicqChanged [mainData->musicqManageIdx] = true;
    mainData->marqueeChanged [mainData->musicqManageIdx] = true;
    currlen = musicqGetLen (mainData->musicQueue, mainData->musicqManageIdx);

    if (pltype == PLTYPE_AUTO) {
      playlist = queueGetCurrent (mainData->playlistQueue [mainData->musicqManageIdx]);
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
  for (ssize_t i = 0; i < PREP_SIZE; ++i) {
    char          *sfname = NULL;
    song_t        *song = NULL;
    musicqflag_t  flags;
    char          *plname = NULL;
    char          *annfname = NULL;

    song = musicqGetByIdx (mainData->musicQueue, mainData->musicqManageIdx, i);
    flags = musicqGetFlags (mainData->musicQueue, mainData->musicqManageIdx, i);
    plname = musicqGetPlaylistName (mainData->musicQueue, mainData->musicqManageIdx, i);

    if (song != NULL &&
        (flags & MUSICQ_FLAG_PREP) != MUSICQ_FLAG_PREP) {
      musicqSetFlag (mainData->musicQueue, mainData->musicqManageIdx,
          i, MUSICQ_FLAG_PREP);

      plannounce = false;
      if (plname != NULL) {
        playlist = slistGetData (mainData->playlistCache, plname);
        plannounce = playlistGetConfigNum (playlist, PLAYLIST_ANNOUNCE);
      }
      annfname = mainPrepSong (mainData, song, sfname, plname, PREP_SONG);

      if (plannounce == 1) {
        if (annfname != NULL && strcmp (annfname, "") != 0 ) {
          musicqSetFlag (mainData->musicQueue, mainData->musicqManageIdx,
              i, MUSICQ_FLAG_ANNOUNCE);
          musicqSetAnnounce (mainData->musicQueue, mainData->musicqManageIdx,
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
      dances = bdjvarsdfGet (BDJVDF_DANCES);
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
  connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_SONG_PREP, tbuff);
  return annfname;
}

static void
mainTogglePause (maindata_t *mainData, char *args)
{
  int           mi;
  ssize_t       idx;
  ssize_t       musicqLen;
  musicqflag_t  flags;

  mainParseIntNum (args, &mi, &idx);
  mainData->musicqManageIdx = mi;

  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqManageIdx);
  if (idx <= 0 || idx > musicqLen) {
    return;
  }

  flags = musicqGetFlags (mainData->musicQueue, mainData->musicqManageIdx, idx);
  if ((flags & MUSICQ_FLAG_PAUSE) == MUSICQ_FLAG_PAUSE) {
    musicqClearFlag (mainData->musicQueue, mainData->musicqManageIdx, idx, MUSICQ_FLAG_PAUSE);
  } else {
    musicqSetFlag (mainData->musicQueue, mainData->musicqManageIdx, idx, MUSICQ_FLAG_PAUSE);
  }
  mainData->musicqChanged [mi] = true;
}

static void
mainMusicqMove (maindata_t *mainData, char *args, mainmove_t direction)
{
  int           mi;
  ssize_t       fromidx;
  ssize_t       toidx;
  ssize_t       musicqLen;


  mainParseIntNum (args, &mi, &fromidx);
  mainData->musicqManageIdx = mi;

  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqManageIdx);

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

  musicqMove (mainData->musicQueue, mainData->musicqManageIdx, fromidx, toidx);
  mainData->musicqChanged [mi] = true;
  mainData->marqueeChanged [mi] = true;
  mainMusicQueuePrep (mainData);

  if (toidx == 0) {
    mainSendMusicqStatus (mainData, NULL, 0);
  }
}

static void
mainMusicqMoveTop (maindata_t *mainData, char *args)
{
  int     mi;
  ssize_t fromidx;
  ssize_t toidx;
  ssize_t musicqLen;

  mainParseIntNum (args, &mi, &fromidx);
  mainData->musicqManageIdx = mi;

  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqManageIdx);

  if (fromidx >= musicqLen || fromidx <= 1) {
    return;
  }

  toidx = fromidx - 1;
  while (toidx != 0) {
    musicqMove (mainData->musicQueue, mainData->musicqManageIdx, fromidx, toidx);
    fromidx--;
    toidx--;
  }
  mainData->musicqChanged [mi] = true;
  mainData->marqueeChanged [mi] = true;
  mainMusicQueuePrep (mainData);
}

static void
mainMusicqClear (maindata_t *mainData, char *args)
{
  int     mi;
  ssize_t idx;

  mainParseIntNum (args, &mi, &idx);
  mainData->musicqManageIdx = mi;


  musicqClear (mainData->musicQueue, mainData->musicqManageIdx, idx);
  /* there may be other playlists in the playlist queue */
  mainMusicQueueFill (mainData);
  mainMusicQueuePrep (mainData);
  mainData->musicqChanged [mi] = true;
  mainData->marqueeChanged [mi] = true;
}

static void
mainMusicqRemove (maindata_t *mainData, char *args)
{
  int     mi;
  ssize_t idx;


  mainParseIntNum (args, &mi, &idx);
  mainData->musicqManageIdx = mi;

  musicqRemove (mainData->musicQueue, mainData->musicqManageIdx, idx);
  mainMusicQueueFill (mainData);
  mainMusicQueuePrep (mainData);
  mainData->musicqChanged [mi] = true;
  mainData->marqueeChanged [mi] = true;
}

static void
mainMusicqInsert (maindata_t *mainData, char *args)
{
  int       mi;
  char      *tokstr = NULL;
  char      *p = NULL;
  ssize_t   idx;
  dbidx_t   dbidx;
  song_t    *song = NULL;

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  mi = atoi (p);
  if (mi != MUSICQ_CURRENT) {
    mainData->musicqManageIdx = mi;
  }
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  idx = atol (p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  dbidx = atol (p);

  song = dbGetByIdx (dbidx);

  if (song != NULL) {
    musicqInsert (mainData->musicQueue, mainData->musicqManageIdx, idx, song);
    mainData->musicqChanged [mainData->musicqManageIdx] = true;
    mainData->marqueeChanged [mainData->musicqManageIdx] = true;
    mainMusicQueuePrep (mainData);
  }
}

static void
mainMusicqSetPlayback (maindata_t *mainData, char *args)
{
  int         qidx;

  qidx = atoi (args);
  if (qidx < 0 || qidx >= MUSICQ_MAX) {
    return;
  }

  if ((musicqidx_t) qidx == mainData->musicqPlayIdx) {
    mainData->musicqDeferredPlayIdx = MAIN_NOT_SET;
  } else if (mainData->playerState != PL_STATE_STOPPED) {
    /* if the player is playing something, the musicq index cannot */
    /* be changed as yet */
    mainData->musicqDeferredPlayIdx = qidx;
  } else {
    mainMusicqSwitch (mainData, qidx);
  }
}

static void
mainMusicqSwitch (maindata_t *mainData, musicqidx_t newMusicqIdx)
{
  song_t        *song;


  mainData->musicqManageIdx = mainData->musicqPlayIdx;
  mainData->musicqPlayIdx = newMusicqIdx;
  /* manage idx is pointing to the previous musicq play idx */
  musicqPushHeadEmpty (mainData->musicQueue, mainData->musicqManageIdx);
  /* the prior musicq has changed */
  mainData->musicqChanged [mainData->musicqManageIdx] = true;

  mainData->musicqManageIdx = mainData->musicqPlayIdx;
  mainData->musicqDeferredPlayIdx = MAIN_NOT_SET;

  song = musicqGetCurrent (mainData->musicQueue, mainData->musicqPlayIdx);
  if (song == NULL) {
    musicqPop (mainData->musicQueue, mainData->musicqPlayIdx);
  }

  /* and the new musicq has changed */
  mainSendMusicqStatus (mainData, NULL, 0);
  mainData->musicqChanged [mainData->musicqPlayIdx] = true;
  mainData->marqueeChanged [mainData->musicqPlayIdx] = true;
}

static void
mainPlaybackBegin (maindata_t *mainData)
{
  musicqflag_t  flags;

  /* if the pause flag is on for this entry in the music queue, */
  /* tell the player to turn on the pause-at-end */
  flags = musicqGetFlags (mainData->musicQueue, mainData->musicqPlayIdx, 0);
  if ((flags & MUSICQ_FLAG_PAUSE) == MUSICQ_FLAG_PAUSE) {
    connSendMessage (mainData->conn, ROUTE_PLAYER,
        MSG_PLAY_PAUSEATEND, NULL);
  }
}

static void
mainMusicQueuePlay (maindata_t *mainData)
{
  musicqflag_t      flags;
  char              *sfname;
  song_t            *song;
  ssize_t           currlen;

  if (mainData->musicqDeferredPlayIdx != MAIN_NOT_SET) {
    mainMusicqSwitch (mainData, mainData->musicqDeferredPlayIdx);
    mainData->musicqDeferredPlayIdx = MAIN_NOT_SET;
  }

  mainData->musicqManageIdx = mainData->musicqPlayIdx;

  logProcBegin (LOG_PROC, "mainMusicQueuePlay");
  logMsg (LOG_DBG, LOG_BASIC, "playerState: %d", mainData->playerState);
  if (mainData->playerState == PL_STATE_STOPPED) {
    /* grab a song out of the music queue and start playing */
    logMsg (LOG_DBG, LOG_MAIN, "player is stopped, get song, start");
    song = musicqGetCurrent (mainData->musicQueue, mainData->musicqPlayIdx);
    if (song != NULL) {
      flags = musicqGetFlags (mainData->musicQueue, mainData->musicqPlayIdx, 0);
      if ((flags & MUSICQ_FLAG_ANNOUNCE) == MUSICQ_FLAG_ANNOUNCE) {
        char      *annfname;

        annfname = musicqGetAnnounce (mainData->musicQueue, mainData->musicqPlayIdx, 0);
        if (annfname != NULL) {
          connSendMessage (mainData->conn, ROUTE_PLAYER,
              MSG_SONG_PLAY, annfname);
        }
      }
      sfname = songGetData (song, TAG_FILE);
      connSendMessage (mainData->conn, ROUTE_PLAYER,
          MSG_SONG_PLAY, sfname);
    }

    currlen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);
    if (song == NULL && currlen == 0) {
      if (mainData->switchQueueWhenEmpty) {
        char    tmp [40];

        mainData->musicqPlayIdx = musicqNextQueue (mainData->musicqPlayIdx);
        mainData->musicqManageIdx = mainData->musicqPlayIdx;
        /* only process this flag once */
        /* do not want this program to bounce back and forth between queues */
        mainData->switchQueueWhenEmpty = false;
        snprintf (tmp, sizeof (tmp), "%d", mainData->musicqPlayIdx);
        connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_QUEUE_SWITCH, tmp);
        /* and start up playback for the new queue */
        mainMusicQueueNext (mainData);
        /* since the player state is stopped, must re-start playback */
        mainMusicQueuePlay (mainData);
      }
    }
  }
  if (mainData->playerState == PL_STATE_IN_GAP ||
      mainData->playerState == PL_STATE_PAUSED) {
    logMsg (LOG_DBG, LOG_MAIN, "player is paused/in gap, send play msg");
    connSendMessage (mainData->conn, ROUTE_PLAYER, MSG_PLAY_PLAY, NULL);
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

  mainData->musicqManageIdx = mainData->musicqPlayIdx;

  /* let the playlist know this song has been played */
  song = musicqGetCurrent (mainData->musicQueue, mainData->musicqPlayIdx);
  plname = musicqGetPlaylistName (mainData->musicQueue, mainData->musicqPlayIdx, 0);
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
  musicqPop (mainData->musicQueue, mainData->musicqPlayIdx);
  mainData->musicqChanged [mainData->musicqPlayIdx] = true;
  mainData->marqueeChanged [mainData->musicqPlayIdx] = true;
  mainSendMusicqStatus (mainData, NULL, 0);
  logProcEnd (LOG_PROC, "mainMusicQueueFinish", "");
}

static void
mainMusicQueueNext (maindata_t *mainData)
{
  playerstate_t     tplaystate;

  mainData->musicqManageIdx = mainData->musicqPlayIdx;

  logProcBegin (LOG_PROC, "mainMusicQueueNext");
  tplaystate = mainData->playerState;
  mainMusicQueueFinish (mainData);
  if (tplaystate != PL_STATE_STOPPED &&
      tplaystate != PL_STATE_PAUSED) {
    mainMusicQueuePlay (mainData);
  }
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
      idx >= musicqGetLen (mainData->musicQueue, mainData->musicqManageIdx)) {
    return -1;
  }

  song = musicqGetByIdx (mainData->musicQueue, mainData->musicqManageIdx, idx);
  didx = songGetNum (song, TAG_DANCE);
  return didx;
}

static void
mainSendDanceList (maindata_t *mainData, bdjmsgroute_t route)
{
  dance_t       *dances;
  slist_t       *danceList;
  slistidx_t     idx;
  char          *dancenm;
  char          tbuff [200];
  char          rbuff [3096];
  slistidx_t    iteridx;

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  danceList = danceGetDanceList (dances);

  rbuff [0] = '\0';
  slistStartIterator (danceList, &iteridx);
  while ((dancenm = slistIterateKey (danceList, &iteridx)) != NULL) {
    idx = slistGetNum (danceList, dancenm);
    snprintf (tbuff, sizeof (tbuff), "%zd%c%s%c",
        idx, MSG_ARGS_RS, dancenm, MSG_ARGS_RS);
    strlcat (rbuff, tbuff, sizeof (rbuff));
  }

  connSendMessage (mainData->conn, route, MSG_DANCE_LIST_DATA, rbuff);
}

static void
mainSendPlaylistList (maindata_t *mainData, bdjmsgroute_t route)
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
    snprintf (tbuff, sizeof (tbuff), "%s%c%s%c",
        plfnm, MSG_ARGS_RS, plnm, MSG_ARGS_RS);
    strlcat (rbuff, tbuff, sizeof (rbuff));
  }

  slistFree (plList);

  connSendMessage (mainData->conn, route, MSG_PLAYLIST_LIST_DATA, rbuff);
}

static void
mainSendPlayerStatus (maindata_t *mainData, char *playerResp)
{
  char    tbuff [200];
  char    tbuff2 [40];
  char    rbuff [3096];
  char    timerbuff [1024];
  char    statusbuff [1024];
  char    *tokstr = NULL;
  char    *p;

  statusbuff [0] = '\0';

  strlcpy (rbuff, "{ ", sizeof (rbuff));

  p = strtok_r (playerResp, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"playstate\" : \"%s\"", p);
  strlcat (rbuff, tbuff, sizeof (rbuff));

  snprintf (tbuff, sizeof (tbuff), "%s%c", p, MSG_ARGS_RS);
  strlcat (statusbuff, tbuff, sizeof (statusbuff));

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"repeat\" : \"%s\"", p);
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  snprintf (tbuff, sizeof (tbuff), "%s%c", p, MSG_ARGS_RS);
  strlcat (statusbuff, tbuff, sizeof (statusbuff));

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"pauseatend\" : \"%s\"", p);
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  snprintf (tbuff, sizeof (tbuff), "%s%c", p, MSG_ARGS_RS);
  strlcat (statusbuff, tbuff, sizeof (statusbuff));

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"vol\" : \"%s%%\"", p);
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  snprintf (tbuff, sizeof (tbuff), "%s%c", p, MSG_ARGS_RS);
  strlcat (statusbuff, tbuff, sizeof (statusbuff));

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"speed\" : \"%s%%\"", p);
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  snprintf (tbuff, sizeof (tbuff), "%s%c", p, MSG_ARGS_RS);
  strlcat (statusbuff, tbuff, sizeof (statusbuff));

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"playedtime\" : \"%s\"", tmutilToMS (atol (p), tbuff2, sizeof (tbuff2)));
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  /* for marquee */
  snprintf (tbuff, sizeof (tbuff), "%s%c", p, MSG_ARGS_RS);
  strlcpy (timerbuff, tbuff, sizeof (timerbuff));

  snprintf (tbuff, sizeof (tbuff), "%s%c", p, MSG_ARGS_RS);
  strlcat (statusbuff, tbuff, sizeof (statusbuff));

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  snprintf (tbuff, sizeof (tbuff),
      "\"duration\" : \"%s\"", tmutilToMS (atol (p), tbuff2, sizeof (tbuff2)));
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  /* for marquee */
  snprintf (tbuff, sizeof (tbuff), "%s", p);
  strlcat (timerbuff, tbuff, sizeof (timerbuff));

  snprintf (tbuff, sizeof (tbuff), "%s%c", p, MSG_ARGS_RS);
  strlcat (statusbuff, tbuff, sizeof (statusbuff));

  mainSendMusicqStatus (mainData, rbuff, sizeof (rbuff));

  strlcat (rbuff, " }", sizeof (rbuff));

  connSendMessage (mainData->conn, ROUTE_REMCTRL, MSG_PLAYER_STATUS_DATA, rbuff);
  connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_PLAYER_STATUS_DATA, statusbuff);
  connSendMessage (mainData->conn, ROUTE_MARQUEE, MSG_MARQUEE_TIMER, timerbuff);
}

static void
mainSendMusicqStatus (maindata_t *mainData, char *rbuff, size_t siz)
{
  char    tbuff [200];
  char    statusbuff [40];
  ssize_t musicqLen;
  char    *data = NULL;
  bool    jsonflag = true;
  dbidx_t dbidx;
  song_t  *song;

  if (rbuff == NULL) {
    jsonflag = false;
  }

  statusbuff [0] = '\0';

  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqPlayIdx);
  if (jsonflag) {
    snprintf (tbuff, sizeof (tbuff),
        "\"qlength\" : \"%zd\"", musicqLen);
    strlcat (rbuff, ", ", siz);
    strlcat (rbuff, tbuff, siz);
  }

  song = musicqGetByIdx (mainData->musicQueue, mainData->musicqPlayIdx, 0);

  /* dance */
  data = musicqGetDance (mainData->musicQueue, mainData->musicqPlayIdx, 0);
  if (data == NULL) { data = ""; }
  snprintf (tbuff, sizeof (tbuff),
      "\"dance\" : \"%s\"", data);
  if (jsonflag) {
    strlcat (rbuff, ", ", siz);
    strlcat (rbuff, tbuff, siz);
  }

  /* artist */
  data = songGetData (song, TAG_ARTIST);
  if (data == NULL) { data = ""; }
  snprintf (tbuff, sizeof (tbuff),
      "\"artist\" : \"%s\"", data);
  if (jsonflag) {
    strlcat (rbuff, ", ", siz);
    strlcat (rbuff, tbuff, siz);
  }

  /* title */
  data = songGetData (song, TAG_TITLE);
  if (data == NULL) { data = ""; }
  snprintf (tbuff, sizeof (tbuff),
      "\"title\" : \"%s\"", data);
  if (jsonflag) {
    strlcat (rbuff, ", ", siz);
    strlcat (rbuff, tbuff, siz);
  }

  dbidx = songGetNum (song, TAG_DBIDX);
  snprintf (tbuff, sizeof (tbuff), "%zd%c", dbidx, MSG_ARGS_RS);
  strlcpy (statusbuff, tbuff, sizeof (statusbuff));

  connSendMessage (mainData->conn, ROUTE_PLAYERUI, MSG_MUSICQ_STATUS_DATA, statusbuff);
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

static void
mainParseIntNum (char *args, int *a, ssize_t *b)
{
  char            *p;
  char            *tokstr;

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  *a = atoi (p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  *b = atol (p);
}

static void
mainParseIntStr (char *args, int *a, char **b)
{
  char            *p;
  char            *tokstr;

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  *a = atoi (p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  *b = p;
}


#if 0
#endif
