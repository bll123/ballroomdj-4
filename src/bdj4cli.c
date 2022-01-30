#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <assert.h>

#include "sockh.h"
#include "bdjmsg.h"
#include "bdjstring.h"
#include "sysvars.h"
#include "bdjvars.h"
#include "log.h"
#include "tmutil.h"

static void processBuff (char *buff);

int
main (int argc, char *argv[])
{
  char            mbuff [1024];
  char            buff [200];
  bdjmsgroute_t   route = ROUTE_NONE;
  bdjmsgmsg_t     msg = MSG_NULL;
  int             routeok = 0;
  int             msgok = 0;
  int             argcount = 0;
  char            *rval;
  int             err;
  Sock_t          mainSock = INVALID_SOCKET;
  Sock_t          playerSock = INVALID_SOCKET;
  Sock_t          marqueeSock = INVALID_SOCKET;
  int             c = 0;
  int             option_index = 0;
  uint16_t        mainPort;
  uint16_t        playerPort;
  uint16_t        marqueePort;


  static struct option bdj_options [] = {
    { "clicomm",    no_argument,        NULL,   0 },
    { "debug",      required_argument,  NULL,   'd' },
    { "profile",    required_argument,  NULL,   'p' },
    { NULL,         0,                  NULL,   0 }
  };

  sysvarsInit (argv [0]);

  while ((c = getopt_long (argc, argv, "", bdj_options, &option_index)) != -1) {
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

  mainPort = bdjvarsl [BDJVL_MAIN_PORT];
  mainSock = sockConnect (mainPort, &err, 1000);
  while (socketInvalid (mainSock)) {
    mssleep (100);
    mainSock = sockConnect (mainPort, &err, 1000);
  }

  playerPort = bdjvarsl [BDJVL_PLAYER_PORT];
  playerSock = sockConnect (playerPort, &err, 1000);
  while (socketInvalid (playerSock)) {
    mssleep (100);
    playerSock = sockConnect (playerPort, &err, 1000);
  }

  marqueePort = bdjvarsl [BDJVL_MARQUEE_PORT];
  marqueeSock = sockConnect (marqueePort, &err, 1000);
  while (socketInvalid (marqueeSock)) {
    mssleep (100);
    marqueeSock = sockConnect (marqueePort, &err, 1000);
  }

  while (1) {
    routeok = 0;
    while (routeok == 0) {
      printf ("Route to: ");
      fflush (stdout);
      rval = fgets (buff, sizeof (buff), stdin);
      buff [strlen (buff) - 1] = '\0';
      processBuff (buff);
      if (strcmp (buff, "") == 0) {
        break;
      }
      if (strcmp (buff, "main") == 0) {
        route = ROUTE_MAIN;
        routeok = 1;
      }
      if (strcmp (buff, "player") == 0) {
        route = ROUTE_PLAYER;
        routeok = 1;
      }
      if (strcmp (buff, "pui") == 0) {
        route = ROUTE_PLAYERUI;
        routeok = 1;
      }
      if (strcmp (buff, "cui") == 0) {
        route = ROUTE_CONFIGUI;
        routeok = 1;
      }
      if (strcmp (buff, "mui") == 0) {
        route = ROUTE_MANAGEUI;
        routeok = 1;
      }
      if (strcmp (buff, "mm") == 0) {
        route = ROUTE_MOBILEMQ;
        routeok = 1;
      }
      if (strcmp (buff, "rc") == 0) {
        route = ROUTE_REMCTRL;
        routeok = 1;
      }
      if (strcmp (buff, "mq") == 0) {
        route = ROUTE_MARQUEE;
        routeok = 1;
      }
      if (strcmp (buff, "cliexit") == 0) {
        if (mainSock != INVALID_SOCKET) {
          sockhSendMessage (mainSock, ROUTE_CLICOMM, ROUTE_MAIN, MSG_SOCKET_CLOSE, NULL);
          sockClose (mainSock);
        }
        if (playerSock != INVALID_SOCKET) {
          sockhSendMessage (playerSock, ROUTE_CLICOMM, ROUTE_PLAYER, MSG_SOCKET_CLOSE, NULL);
          sockClose (playerSock);
        }
        if (marqueeSock != INVALID_SOCKET) {
          sockhSendMessage (marqueeSock, ROUTE_CLICOMM, ROUTE_MARQUEE, MSG_SOCKET_CLOSE, NULL);
          sockClose (marqueeSock);
        }
        logEnd ();
        exit (0);
      }
      if (! routeok) {
        fprintf (stdout, "    invalid route\n");
        fflush (stdout);
      }
    }

    msgok = 0;
    argcount = 0;
    while (msgok == 0) {
      printf ("  Msg: ");
      fflush (stdout);
      rval = fgets (buff, sizeof (buff), stdin);
      buff [strlen (buff) - 1] = '\0';
      processBuff (buff);
      msgok = 0;
      if (strcmp (buff, "") == 0) {
        break;
      }
      if (strcmp (buff, "moveup") == 0) {
        msg = MSG_MUSICQ_MOVE_UP;
        msgok = 1;
        argcount = 1;
      }
      if (strcmp (buff, "movetop") == 0) {
        msg = MSG_MUSICQ_MOVE_TOP;
        msgok = 1;
        argcount = 1;
      }
      if (strcmp (buff, "movedown") == 0) {
        msg = MSG_MUSICQ_MOVE_DOWN;
        msgok = 1;
        argcount = 1;
      }
      if (strcmp (buff, "mremove") == 0) {
        msg = MSG_MUSICQ_REMOVE;
        msgok = 1;
        argcount = 1;
      }
      if (strcmp (buff, "mtruncate") == 0) {
        msg = MSG_MUSICQ_TRUNCATE;
        msgok = 1;
        argcount = 1;
      }
      if (strcmp (buff, "minsert") == 0) {
        msg = MSG_MUSICQ_INSERT;
        msgok = 1;
        argcount = 2;
      }
      if (strcmp (buff, "tpause") == 0) {
        msg = MSG_MUSICQ_TOGGLE_PAUSE;
        msgok = 1;
        argcount = 1;
      }
      if (strcmp (buff, "prep-song") == 0) {
        msg = MSG_SONG_PREP;
        msgok = 1;
        argcount = 1;
      }
      if (strcmp (buff, "play-song") == 0) {
        msg = MSG_SONG_PLAY;
        msgok = 1;
        argcount = 1;
      }
      if (strcmp (buff, "pause") == 0) {
        msg = MSG_PLAY_PAUSE;
        msgok = 1;
      }
      if (strcmp (buff, "play") == 0) {
        msg = MSG_PLAY_PLAY;
        msgok = 1;
      }
      if (strcmp (buff, "playpause") == 0) {
        msg = MSG_PLAY_PLAYPAUSE;
        msgok = 1;
      }
      if (strcmp (buff, "fade") == 0) {
        msg = MSG_PLAY_FADE;
        msgok = 1;
      }
      if (strcmp (buff, "pauseatend") == 0) {
        msg = MSG_PLAY_PAUSEATEND;
        msgok = 1;
      }
      if (strcmp (buff, "nextsong") == 0) {
        msg = MSG_PLAY_NEXTSONG;
        msgok = 1;
      }
      if (strcmp (buff, "volmute") == 0) {
        msg = MSG_PLAYER_VOL_MUTE;
        msgok = 1;
      }
      if (strcmp (buff, "vol") == 0) {
        msg = MSG_PLAYER_VOLUME;
        msgok = 1;
        argcount = 1;
      }
      if (strcmp (buff, "stop") == 0) {
        msg = MSG_PLAY_STOP;
        msgok = 1;
      }
      if (strcmp (buff, "playlist-q") == 0) {
        msg = MSG_PLAYLIST_QUEUE;
        msgok = 1;
        argcount = 1;
      }
      if (strcmp (buff, "mqshow") == 0) {
        msg = MSG_MARQUEE_SHOW;
        msgok = 1;
      }
      if (strcmp (buff, "exit") == 0) {
        msg = MSG_EXIT_REQUEST;
        msgok = 1;
      }
      if (strcmp (buff, "cliexit") == 0) {
        if (mainSock != INVALID_SOCKET) {
          sockhSendMessage (mainSock, ROUTE_CLICOMM, ROUTE_MAIN, MSG_SOCKET_CLOSE, NULL);
          sockClose (mainSock);
        }
        if (playerSock != INVALID_SOCKET) {
          sockhSendMessage (playerSock, ROUTE_CLICOMM, ROUTE_PLAYER, MSG_SOCKET_CLOSE, NULL);
          sockClose (playerSock);
        }
        if (marqueeSock != INVALID_SOCKET) {
          sockhSendMessage (marqueeSock, ROUTE_CLICOMM, ROUTE_MARQUEE, MSG_SOCKET_CLOSE, NULL);
          sockClose (marqueeSock);
        }
        exit (0);
      }
      if (! msgok) {
        fprintf (stdout, "    invalid msg\n");
        fflush (stdout);
      }
    }

    mbuff [0] = '\0';
    while (argcount) {
      printf ("  %s args: ", buff);
      fflush (stdout);
      rval = fgets (buff, sizeof (buff), stdin);
      buff [strlen (buff) - 1] = '\0';
      processBuff (buff);
      if (mbuff [0] != '\0') {
        strlcat (mbuff, MSG_ARGS_RS_STR, sizeof (mbuff));
      }
      strlcat (mbuff, buff, sizeof (mbuff));
      --argcount;
    }

    if (routeok && msgok) {
      if (route == ROUTE_MAIN) {
        sockhSendMessage (mainSock, ROUTE_CLICOMM, route, msg, mbuff);
      }
      if (route == ROUTE_PLAYER) {
        sockhSendMessage (playerSock, ROUTE_CLICOMM, route, msg, mbuff);
      }
      if (route == ROUTE_MARQUEE) {
        sockhSendMessage (marqueeSock, ROUTE_CLICOMM, route, msg, mbuff);
      }
    }
  }
}


static void
processBuff (char *buff)
{
  size_t    idx = 0;

  for (char *p = buff; *p; ++p) {
    if (*p == 0x15) {
      idx = 0;
      continue;
    }
    if (*p != 0x8 && *p != 0x7F) {
      buff [idx] = *p;
      ++idx;
    }
  }
  buff [idx] = '\0';
}
