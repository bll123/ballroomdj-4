#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>

#include "bdjmsg.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "conn.h"
#include "fileop.h"
#include "lock.h"
#include "log.h"
#include "pathbld.h"
#include "process.h"
#include "sockh.h"
#include "sysvars.h"
#include "tmutil.h"

static bdjmsgroute_t cvtRoute (char *routename);
static bdjmsgmsg_t cvtMsg (char *msgname, int *argcount);
static void sendMsg (conn_t *conn,
    bdjmsgroute_t route, bdjmsgmsg_t msg, char *buff);
static void exitAll (conn_t *conn);

int
main (int argc, char *argv[])
{
  conn_t          *conn;
  bdjmsgroute_t   route = ROUTE_NONE;
  bdjmsgmsg_t     msg = MSG_NULL;
  int             argcount = 0;
  int             c = 0;
  int             option_index = 0;
  uint16_t        port;
  char            buff [1024];
  bool            forceexit = false;


  static struct option bdj_options [] = {
    { "route",      required_argument,  NULL,   'r' },
    { "msg",        required_argument,  NULL,   'm' },
    { "forceexit",  no_argument,        NULL,   'f' },
    { "clicomm",    no_argument,        NULL,   0 },
    { "debug",      required_argument,  NULL,   'd' },
    { "profile",    required_argument,  NULL,   'p' },
    { NULL,         0,                  NULL,   0 }
  };

  sysvarsInit (argv [0]);

  while ((c = getopt_long_only (argc, argv, "p:d:r:m:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'd': {
        break;
      }
      case 'p': {
        if (optarg) {
          sysvarSetNum (SVL_BDJIDX, atol (optarg));
        }
        break;
      }
      case 'f': {
        forceexit = true;
        break;
      }
      case 'r': {
        route = cvtRoute (optarg);
        break;
      }
      case 'm': {
        msg = cvtMsg (optarg, &argcount);
        break;
      }
      default: {
        break;
      }
    }
  }

  if (chdir (sysvars [SV_BDJ4DIR]) < 0) {
    fprintf (stderr, "Unable to chdir: %s\n", sysvars [SV_BDJ4DIR]);
    exit (1);
  }

  bdjvarsInit ();
  logStartAppend ("clicomm", "cl", LOG_IMPORTANT);

  conn = connInit (ROUTE_CLICOMM);

  if (forceexit) {
    exitAll (conn);
    logEnd ();
    exit (0);
  }

  if (argc - optind < argcount) {
    fprintf (stderr, "incorrect number of arguments: %d < %d\n", argc - optind, argcount);
    exit (1);
  }

  if (route != ROUTE_NONE && msg != MSG_NULL) {
    /* only used to get port numbers */

    buff [0] = '\0';
    for (int i = 0; i < argcount; ++i) {
      strlcat (buff, argv [optind++], sizeof (buff));
      strlcat (buff, MSG_ARGS_RS_STR, sizeof (buff));
    }

    sendMsg (conn, route, msg, buff);
  }

  bdjvarsCleanup ();
  logEnd ();
  connFree (conn);
  exit (0);
}


static bdjmsgroute_t
cvtRoute (char *routename)
{
  bdjmsgroute_t route = ROUTE_NONE;

  if (strcmp (routename, "main") == 0) {
    route = ROUTE_MAIN;
  }
  if (strcmp (routename, "player") == 0) {
    route = ROUTE_PLAYER;
  }
  if (strcmp (routename, "pui") == 0) {
    route = ROUTE_PLAYERUI;
  }
  if (strcmp (routename, "cui") == 0) {
    route = ROUTE_CONFIGUI;
  }
  if (strcmp (routename, "mui") == 0) {
    route = ROUTE_MANAGEUI;
  }
  if (strcmp (routename, "mm") == 0) {
    route = ROUTE_MOBILEMQ;
  }
  if (strcmp (routename, "rc") == 0) {
    route = ROUTE_REMCTRL;
  }
  if (strcmp (routename, "mq") == 0) {
    route = ROUTE_MARQUEE;
  }

  return route;
}

static bdjmsgmsg_t
cvtMsg (char *msgname, int *argcount)
{
  bdjmsgmsg_t msg = MSG_NULL;

  *argcount = 0;

  if (strcmp (msgname, "moveup") == 0) {
    msg = MSG_MUSICQ_MOVE_UP;
    *argcount = 2;
  }
  if (strcmp (msgname, "movetop") == 0) {
    msg = MSG_MUSICQ_MOVE_TOP;
    *argcount = 2;
  }
  if (strcmp (msgname, "movedown") == 0) {
    msg = MSG_MUSICQ_MOVE_DOWN;
    *argcount = 2;
  }
  if (strcmp (msgname, "mremove") == 0) {
    msg = MSG_MUSICQ_REMOVE;
    *argcount = 2;
  }
  if (strcmp (msgname, "mtruncate") == 0) {
    msg = MSG_MUSICQ_TRUNCATE;
    *argcount = 2;
  }
  if (strcmp (msgname, "minsert") == 0) {
    msg = MSG_MUSICQ_INSERT;
    *argcount = 3;
  }
  if (strcmp (msgname, "tpause") == 0) {
    msg = MSG_MUSICQ_TOGGLE_PAUSE;
    *argcount = 2;
  }
  if (strcmp (msgname, "prep-song") == 0) {
    msg = MSG_SONG_PREP;
    *argcount = 1;
  }
  if (strcmp (msgname, "play-song") == 0) {
    msg = MSG_SONG_PLAY;
    *argcount = 1;
  }
  if (strcmp (msgname, "pause") == 0) {
    msg = MSG_PLAY_PAUSE;
  }
  if (strcmp (msgname, "play") == 0) {
    msg = MSG_PLAY_PLAY;
  }
  if (strcmp (msgname, "playpause") == 0) {
    msg = MSG_PLAY_PLAYPAUSE;
  }
  if (strcmp (msgname, "fade") == 0) {
    msg = MSG_PLAY_FADE;
  }
  if (strcmp (msgname, "pauseatend") == 0) {
    msg = MSG_PLAY_PAUSEATEND;
  }
  if (strcmp (msgname, "nextsong") == 0) {
    msg = MSG_PLAY_NEXTSONG;
  }
  if (strcmp (msgname, "volmute") == 0) {
    msg = MSG_PLAYER_VOL_MUTE;
  }
  if (strcmp (msgname, "vol") == 0) {
    msg = MSG_PLAYER_VOLUME;
    *argcount = 1;
  }
  if (strcmp (msgname, "stop") == 0) {
    msg = MSG_PLAY_STOP;
  }
  if (strcmp (msgname, "playlist-q") == 0) {
    msg = MSG_QUEUE_PLAYLIST;
    *argcount = 2;
  }
  if (strcmp (msgname, "mqshow") == 0) {
    msg = MSG_MARQUEE_SHOW;
  }
  if (strcmp (msgname, "exit") == 0) {
    msg = MSG_EXIT_REQUEST;
  }

  return msg;
}

static void
sendMsg (conn_t *conn, bdjmsgroute_t route, bdjmsgmsg_t msg, char *buff)
{
  int       err = 0;
  int       count = 0;
  Sock_t    sock = INVALID_SOCKET;
  uint16_t  port = connPort (conn, route);

  sock = sockConnect (port, &err, 1000);
  count = 0;
  while (socketInvalid (sock) && count < 3) {
    mssleep (100);
    sock = sockConnect (port, &err, 1000);
    ++count;
  }
  if (sock != INVALID_SOCKET) {
    sockhSendMessage (sock, ROUTE_CLICOMM, route, msg, buff);
    sockhSendMessage (sock, ROUTE_CLICOMM, route, MSG_SOCKET_CLOSE, NULL);
    sockClose (sock);
  }
}


static void
exitAll (conn_t *conn)
{
  bdjmsgroute_t     route;
  char              *locknm;
  pid_t             pid;


  /* send the standard exit request to the controlling processes first */
  sendMsg (conn, ROUTE_PLAYERUI, MSG_EXIT_REQUEST, NULL);
  mssleep (200);
  sendMsg (conn, ROUTE_MAIN, MSG_EXIT_REQUEST, NULL);
  mssleep (300);

  /* see which lock files exist, and send exit requests to them */
  for (route = ROUTE_MAIN; route < ROUTE_MAX; ++route) {
    locknm = lockName (route);
    pid = lockExists (locknm, PATHBLD_MP_USEIDX);
    if (pid != 0) {
      sendMsg (conn, route, MSG_EXIT_REQUEST, NULL);
    }
  }
  mssleep (400);

  /* see which lock files still exist and kill the processes */
  for (route = ROUTE_MAIN; route < ROUTE_MAX; ++route) {
    locknm = lockName (route);
    pid = lockExists (locknm, PATHBLD_MP_USEIDX);
    if (pid != 0) {
#if _lib_kill
      kill (pid, SIGTERM);
#endif
    }
  }

  /* see which lock files still exist and kill the processes with
  /* a signal that is not caught; remove the lock file */
  for (route = ROUTE_MAIN; route < ROUTE_MAX; ++route) {
    locknm = lockName (route);
    pid = lockExists (locknm, PATHBLD_MP_USEIDX);
    if (pid != 0) {
#if _lib_kill
      kill (pid, SIGABRT);
#endif
    }
    fileopDelete (locknm);
  }
}
