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

#define SOCKOF(idx) (mainData->processes [idx].sock)

  /* playlistList contains all of the playlists that have been seen */
  /* so that playlist lookups can be processed */
typedef struct {
  mstime_t          tm;
  programstate_t    programState;
  processconn_t     processes [PROCESS_MAX];
  slist_t           *playlistList;
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
static void     mainPlaylistQueue (maindata_t *mainData, char *plname);
static void     mainSigHandler (int sig);
static void     mainMusicQueueFill (maindata_t *mainData);
static void     mainMusicQueuePrep (maindata_t *mainData);
static char     *mainPrepSong (maindata_t *maindata, song_t *song,
                    char *sfname, char *plname, bdjmsgprep_t flag);
static void     mainTogglePause (maindata_t *mainData, ssize_t idx);
static void     mainMusicqMove (maindata_t *mainData, ssize_t fromidx, ssize_t direction);
static void     mainMusicqMoveTop (maindata_t *mainData, ssize_t fromidx);
static void     mainPlaybackBegin (maindata_t *mainData);
static void     mainMusicQueuePlay (maindata_t *mainData);
static void     mainMusicQueueFinish (maindata_t *mainData);
static void     mainMusicQueueNext (maindata_t *mainData);
static bool     mainCheckMusicQueue (song_t *song, void *tdata);
static void     mainForceStop (char *lockfn, datautil_mp_t flags);
static int      mainStartProcess (maindata_t *mainData, processconnidx_t idx,
                    char *fname);
static void     mainStopProcess (maindata_t *mainData, processconnidx_t idx,
                    bdjmsgroute_t route, char *lockfn, bool force);
static void     mainSendDanceList (maindata_t *mainData);
static void     mainSendPlaylistList (maindata_t *mainData);
static void     mainSendStatusResp (maindata_t *mainData, char *playerResp);
static void     mainDanceCountsInit (maindata_t *mainData);

static int gKillReceived = 0;

int
main (int argc, char *argv[])
{
  maindata_t    mainData;
  uint16_t      listenPort;


  mstimestart (&mainData.tm);

  mainData.programState = STATE_INITIALIZING;
  mainData.playlistList = NULL;
  mainData.playlistQueue = NULL;
  mainData.musicQueue = NULL;
  mainData.danceCounts = NULL;
  mainData.musicqCurrentIdx = MUSICQ_A;
  mainData.playerState = PL_STATE_STOPPED;
  mainData.webclient = NULL;
  mainData.mobmqUserkey = NULL;
  for (processconnidx_t i = PROCESS_PLAYER; i < PROCESS_MAX; ++i) {
    mainData.processes [i].sock = INVALID_SOCKET;
    mainData.processes [i].started = false;
    mainData.processes [i].handshake = false;
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
  mainData.playlistList = slistAlloc ("playlist-list", LIST_UNORDERED,
      free, playlistFree);
  mainData.playlistQueue = queueAlloc (NULL);
  mainData.musicQueue = musicqAlloc ();
  mainDanceCountsInit (&mainData);
  mainData.announceList = slistAlloc ("announcements", LIST_ORDERED,
      free, NULL);

  listenPort = bdjvarsl [BDJVL_MAIN_PORT];
  sockhMainLoop (listenPort, mainProcessMsg, mainProcessing, &mainData);

  logProcEnd (LOG_PROC, "main", "");
  if (mainData.playlistList != NULL) {
    slistFree (mainData.playlistList);
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

  logMsg (LOG_DBG, LOG_MSGS, "got: from: %ld route: %ld msg:%ld args:%s",
      routefrom, route, msg, args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MAIN: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          if (routefrom == ROUTE_PLAYER) {
            mainData->processes [PROCESS_PLAYER].handshake = true;
          }
          if (routefrom == ROUTE_MOBILEMQ) {
            mainData->processes [PROCESS_MOBILEMQ].handshake = true;
          }
          if (routefrom == ROUTE_REMCTRL) {
            mainData->processes [PROCESS_REMCTRL].handshake = true;
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
          mainPlaylistQueue (mainData, args);
          break;
        }
        case MSG_PLAYLIST_QUEUE: {
          logMsg (LOG_DBG, LOG_MSGS, "got: playlist-queue %s", args);
          mainPlaylistQueue (mainData, args);
          break;
        }
        case MSG_DANCE_QUEUE: {
          break;
        }
        case MSG_DANCE_QUEUE5: {
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
          mainMusicqMove (mainData, atol (args), 1);
          break;
        }
        case MSG_MUSICQ_MOVE_TOP: {
          mainMusicqMoveTop (mainData, atol (args));
          break;
        }
        case MSG_MUSICQ_MOVE_UP: {
          mainMusicqMove (mainData, atol (args), -1);
          break;
        }
        case MSG_PLAYER_STATE: {
          mainData->playerState = atol (args);
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
        case MSG_GET_STATUS: {
          sockhSendMessage (SOCKOF (PROCESS_PLAYER),
              ROUTE_MAIN, ROUTE_PLAYER, MSG_GET_STATUS, NULL);
          break;
        }
        case MSG_PLAYER_STATUS_DATA: {
          mainSendStatusResp (mainData, args);
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
      ! mainData->processes [PROCESS_PLAYER].started) {
    mainStartProcess (mainData, PROCESS_PLAYER, "bdj4player");
    mainStartProcess (mainData, PROCESS_MOBILEMQ, "mobilemarquee");
    mainStartProcess (mainData, PROCESS_REMCTRL, "remotecontrol");
    mainData->programState = STATE_CONNECTING;
    logMsg (LOG_DBG, LOG_MAIN, "prog: connecting");
  }

  if (mainData->programState == STATE_CONNECTING &&
      mainData->processes [PROCESS_PLAYER].started &&
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
    if (mainData->processes [PROCESS_PLAYER].handshake) {
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
  char        *tag;
  ssize_t     mqLen;
  ssize_t     musicqLen;


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

  sockhSendMessage (SOCKOF (PROCESS_MOBILEMQ),
      ROUTE_MAIN, ROUTE_MOBILEMQ, MSG_MARQUEE_DATA, jbuff);

  if (bdjoptGetNum (OPT_P_MOBILEMARQUEE) == MOBILEMQ_LOCAL) {
    return;
  }

  /* internet mode from here on */

  if (mainData->mobmqUserkey == NULL) {
    pathbldMakePath (tbuff, sizeof (tbuff), "",
        "mmq", ".key", PATHBLD_MP_USEIDX);
    mainData->mobmqUserkey = filedataReadAll (tbuff);
  }

  tag = bdjoptGetData (OPT_P_MOBILEMQTAG);
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

static void
mainMobilePostCallback (void *userdata, char *resp, size_t len)
{
  maindata_t    *mainData = userdata;
  char          tbuff [MAXPATHLEN];

  if (strncmp (resp, "OK", 2) == 0) {
    ;
  } else if (strncmp (resp, "NG", 2) == 0) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: unable to post mobmq data: %.*s\n", (int) len, resp);
  } else {
    FILE    *fh;

    /* this should be the user key */
    mainData->mobmqUserkey = strndup (resp, len);
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

  queueClear (mainData->playlistQueue, 0);
  startIdx = mainData->musicqCurrentIdx == MUSICQ_A ? 1 : 0;
  musicqClear (mainData->musicQueue, MUSICQ_A, startIdx);
  startIdx = mainData->musicqCurrentIdx == MUSICQ_B ? 1 : 0;
  musicqClear (mainData->musicQueue, MUSICQ_B, startIdx);
  mainData->musicqChanged = true;
}

static void
mainPlaylistQueue (maindata_t *mainData, char *plname)
{
  playlist_t      *playlist;


  logProcBegin (LOG_PROC, "mainPlaylistQueue");
  playlist = playlistLoad (plname);
  if (playlist != NULL) {
    logMsg (LOG_DBG, LOG_BASIC, "Queue Playlist: %s", plname);
    slistSetData (mainData->playlistList, plname, playlist);
    queuePush (mainData->playlistQueue, playlist);
    logMsg (LOG_DBG, LOG_MAIN, "push pl %s", plname);
    mainMusicQueueFill (mainData);
    mainMusicQueuePrep (mainData);
  } else {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Queue Playlist failed: %s", plname);
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
  nlist_t     *musicqList = NULL;
  pltype_t    pltype;


  playlist = queueGetCurrent (mainData->playlistQueue);

  pltype = (pltype_t) playlistGetConfigNum (playlist, PLAYLIST_TYPE);
  if (pltype == PLTYPE_AUTO) {
    ssize_t     len;
    ilistidx_t  danceIdx;

    len = musicqGetLen (mainData->musicQueue, mainData->musicqCurrentIdx);
    musicqList = nlistAlloc ("musicqlist", LIST_UNORDERED, NULL);
    nlistSetSize (musicqList, len);
    /* traverse in reverse order */
    for (ssize_t i = len - 1; i >= 0; --i) {
      song = musicqGetByIdx (mainData->musicQueue, mainData->musicqCurrentIdx, i);
      danceIdx = songGetNum (song, TAG_DANCE);
      nlistSetNum (musicqList, danceIdx, 0);
    }
  }

  logProcBegin (LOG_PROC, "mainMusicQueueFill");
  mqLen = bdjoptGetNum (OPT_G_PLAYERQLEN);
  currlen = musicqGetLen (mainData->musicQueue, mainData->musicqCurrentIdx);
  logMsg (LOG_DBG, LOG_BASIC, "fill: %ld < %ld", currlen, mqLen);
    /* want current + mqLen songs */
  while (playlist != NULL && currlen <= mqLen) {
    song = playlistGetNextSong (playlist, mainData->danceCounts,
        musicqList, mainCheckMusicQueue, mainData);
    if (song == NULL) {
      queuePop (mainData->playlistQueue);
      playlist = queueGetCurrent (mainData->playlistQueue);
      continue;
    }
    musicqPush (mainData->musicQueue, mainData->musicqCurrentIdx, song, playlistGetName (playlist));
    mainData->musicqChanged = true;
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
      musicqSetFlag (mainData->musicQueue, mainData->musicqCurrentIdx,
          i, MUSICQ_FLAG_PREP);
      annfname = mainPrepSong (mainData, song, sfname, plname, PREP_SONG);
      if (annfname != NULL && strcmp (annfname, "") != 0 ) {
        musicqSetFlag (mainData->musicQueue, mainData->musicqCurrentIdx,
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
    playlist = slistGetData (mainData->playlistList, plname);
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

  snprintf (tbuff, MAXPATHLEN, "%s%c%zd%c%zd%c%zd%c%zd%c%d", sfname,
      MSG_ARGS_RS, dur, MSG_ARGS_RS, songstart,
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
mainMusicqMove (maindata_t *mainData, ssize_t fromidx, ssize_t direction)
{
  ssize_t       toidx;
  ssize_t       musicqLen;

  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqCurrentIdx);

  if (direction == 0) {
    toidx = 1;
  } else {
    toidx = fromidx + direction;
  }
  if (mainData->playerState == PL_STATE_STOPPED &&
      fromidx == 1 && toidx == 0) {
    /* moving up into the to-be-played position is a special case */
  }

  if (toidx > fromidx && fromidx >= musicqLen) {
    return;
  }
  if (toidx < fromidx && fromidx <= 1) {
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
mainPlaybackBegin (maindata_t *mainData)
{
  musicqflag_t  flags;
  playlist_t    *playlist = NULL;
  song_t        *song = NULL;
  ilistidx_t    danceIdx;

  /* if the pause flag is on for this entry in the music queue, */
  /* tell the player to turn on the pause-at-end */
  flags = musicqGetFlags (mainData->musicQueue, mainData->musicqCurrentIdx, 0);
  if ((flags & MUSICQ_FLAG_PAUSE) == MUSICQ_FLAG_PAUSE) {
    sockhSendMessage (SOCKOF (PROCESS_PLAYER),
        ROUTE_MAIN, ROUTE_PLAYER, MSG_PLAY_PAUSEATEND, NULL);
  }

  /* let the playlist know this song has been played */
  playlist = queueGetCurrent (mainData->playlistQueue);
  song = musicqGetCurrent (mainData->musicQueue, mainData->musicqCurrentIdx);
  playlistAddPlayed (playlist, song);
  danceIdx = songGetNum (song, TAG_DANCE);
  if (danceIdx >= 0) {
    nlistDecrement (mainData->danceCounts, danceIdx);
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
  logProcBegin (LOG_PROC, "mainMusicQueueFinish");
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

static void
mainForceStop (char *lockfn, datautil_mp_t flags)
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
    if (! processExists (pid)) {
      logMsg (LOG_DBG, LOG_MAIN, "%s exited", lockfn);
      exists = 0;
      break;
    }
    ++count;
    mssleep (100);
  }

  if (exists) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "force-terminating %s", lockfn);
    processKill (pid);
  }
}

static int
mainStartProcess (maindata_t *mainData, processconnidx_t idx, char *fname)
{
  char      tbuff [MAXPATHLEN];
  pid_t     pid;
  char      *extension = NULL;
  int       rc = -1;

  logProcBegin (LOG_PROC, "mainStartProcess");
  mainData->processes [idx].name = fname;
  if (mainData->processes [idx].started) {
    logProcEnd (LOG_PROC, "mainStartProcess", "already");
    return -1;
  }

  extension = "";
  if (isWindows ()) {
    extension = ".exe";
  }
  pathbldMakePath (tbuff, sizeof (tbuff), "",
      fname, extension, PATHBLD_MP_EXECDIR);
  rc = processStart (tbuff, &pid, lsysvars [SVL_BDJIDX],
      bdjoptGetNum (OPT_G_DEBUGLVL));
  if (rc < 0) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "%s %s failed to start", fname, tbuff);
  } else {
    logMsg (LOG_DBG, LOG_BASIC, "%s started pid: %zd", fname, (ssize_t) pid);
    mainData->processes [idx].started = true;
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
    if (! mainData->processes [idx].started) {
      logProcEnd (LOG_PROC, "mainStopProcess", "not-started");
      return;
    }
    sockhSendMessage (SOCKOF (idx), ROUTE_MAIN, route,
        MSG_EXIT_REQUEST, NULL);
    sockhSendMessage (SOCKOF (idx), ROUTE_MAIN, route,
        MSG_SOCKET_CLOSE, NULL);
    sockClose (mainData->processes [idx].sock);
    mainData->processes [idx].sock = INVALID_SOCKET;
    mainData->processes [idx].started = false;
    mainData->processes [idx].handshake = false;
  }
  if (force) {
    mainForceStop (lockfn, PATHBLD_MP_USEIDX);
  }
  logProcEnd (LOG_PROC, "mainStopProcess", "");
}

static void
mainSendDanceList (maindata_t *mainData)
{
  dance_t       *dances;
  slist_t       *danceList;
  listidx_t     idx;
  char          *dancenm;
  char          tbuff [200];
  char          rbuff [3096];

  dances = bdjvarsdf [BDJVDF_DANCES];
  danceList = danceGetDanceList (dances);

  rbuff [0] = '\0';
  slistStartIterator (danceList);
  while ((dancenm = slistIterateKey (danceList)) != NULL) {
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
  slist_t       *playlistList;
  char          *plfnm;
  char          *plnm;
  char          tbuff [200];
  char          rbuff [3096];

    /* note that maindata->playlistlist is something else */
  playlistList = playlistGetPlaylistList ();

  rbuff [0] = '\0';
  slistStartIterator (playlistList);
  while ((plnm = slistIterateKey (playlistList)) != NULL) {
    plfnm = listGetData (playlistList, plnm);
    snprintf (tbuff, sizeof (tbuff), "<option value=\"%s\">%s</option>\n", plfnm, plnm);
    strlcat (rbuff, tbuff, sizeof (rbuff));
  }

  slistFree (playlistList);

  sockhSendMessage (SOCKOF (PROCESS_REMCTRL),
      ROUTE_MAIN, ROUTE_REMCTRL, MSG_PLAYLIST_LIST_DATA, rbuff);
}


static void
mainSendStatusResp (maindata_t *mainData, char *playerResp)
{
  char    tbuff [200];
  char    rbuff [3096];
  int     musicqLen;
  char    *data;


  strlcpy (rbuff, "{ ", sizeof (rbuff));
  strlcat (rbuff, playerResp, sizeof (rbuff));

  musicqLen = musicqGetLen (mainData->musicQueue, mainData->musicqCurrentIdx);
  snprintf (tbuff, sizeof (tbuff),
      "\"qlength\" : \"%d\"", musicqLen);
  strlcat (rbuff, ", ", sizeof (rbuff));
  strlcat (rbuff, tbuff, sizeof (rbuff));

  if (bdjoptGetNum (OPT_P_REMCONTROLSHOWDANCE)) {
    data = musicqGetDance (mainData->musicQueue, mainData->musicqCurrentIdx, 0);
    if (data == NULL) { data = ""; }
    snprintf (tbuff, sizeof (tbuff),
        "\"dance\" : \"%s\"", data);
    strlcat (rbuff, ", ", sizeof (rbuff));
    strlcat (rbuff, tbuff, sizeof (rbuff));
  }

  if (bdjoptGetNum (OPT_P_REMCONTROLSHOWSONG)) {
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
  }

  strlcat (rbuff, " }", sizeof (rbuff));

  sockhSendMessage (SOCKOF (PROCESS_REMCTRL),
      ROUTE_MAIN, ROUTE_REMCTRL, MSG_STATUS_DATA, rbuff);
}

static void
mainDanceCountsInit (maindata_t *mainData)
{
  nlist_t     *dc;
  ilistidx_t  didx;

  dc = dbGetDanceCounts ();

  if (mainData->danceCounts != NULL) {
    nlistFree (mainData->danceCounts);
  }

  mainData->danceCounts = nlistAlloc ("main-dancecounts", LIST_ORDERED, NULL);
  nlistSetSize (mainData->danceCounts, nlistGetCount (dc));

  nlistStartIterator (dc);
  while ((didx = nlistIterateKey (dc)) >= 0) {
    nlistSetNum (mainData->danceCounts, didx, nlistGetNum (dc, didx));
  }
}
