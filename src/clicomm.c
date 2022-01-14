#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>

#include "sockh.h"
#include "bdjmsg.h"
#include "sysvars.h"
#include "bdjvars.h"
#include "log.h"
#include "tmutil.h"

static void processBuff (char *buff);

int
main (int argc, char *argv[])
{
  char            buff [80];
  bdjmsgroute_t   route = ROUTE_NONE;
  bdjmsgmsg_t     msg = MSG_NONE;
  int             routeok = 0;
  int             msgok = 0;
  int             argsok = 0;
  char            *args;
  char            *rval;
  int             err;
  Sock_t          mainSock = INVALID_SOCKET;
  Sock_t          playerSock = INVALID_SOCKET;
  int             c;
  int             option_index = 0;
  uint16_t        mainPort;
  uint16_t        playerPort;


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
      if (strcmp (buff, "gui") == 0) {
        route = ROUTE_GUI;
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
        logEnd ();
        exit (0);
      }
      if (! routeok) {
        fprintf (stdout, "    invalid route\n");
        fflush (stdout);
      }
    }

    msgok = 0;
    argsok = 0;
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
      if (strcmp (buff, "prep-song") == 0) {
        msg = MSG_SONG_PREP;
        msgok = 1;
        argsok = 1;
      }
      if (strcmp (buff, "play-song") == 0) {
        msg = MSG_SONG_PLAY;
        msgok = 1;
        argsok = 1;
      }
      if (strcmp (buff, "pause") == 0) {
        msg = MSG_PLAY_PAUSE;
        msgok = 1;
      }
      if (strcmp (buff, "play") == 0) {
        msg = MSG_PLAY_PLAY;
        msgok = 1;
      }
      if (strcmp (buff, "stop") == 0) {
        msg = MSG_PLAY_STOP;
        msgok = 1;
      }
      if (strcmp (buff, "playlist-q") == 0) {
        msg = MSG_PLAYLIST_QUEUE;
        msgok = 1;
        argsok = 1;
      }
      if (strcmp (buff, "debug") == 0) {
        msg = MSG_SET_DEBUG_LVL;
        msgok = 1;
        argsok = 1;
      }
      if (strcmp (buff, "exit") == 0) {
        msg = MSG_EXIT_REQUEST;
        msgok = 1;
      }
      if (strcmp (buff, "force-exit") == 0) {
        msg = MSG_EXIT_FORCE;
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
        exit (0);
      }
      if (! msgok) {
        fprintf (stdout, "    invalid msg\n");
        fflush (stdout);
      }
    }

    args = NULL;
    if (argsok) {
      printf ("  %s args: ", buff);
      fflush (stdout);
      rval = fgets (buff, sizeof (buff), stdin);
      buff [strlen (buff) - 1] = '\0';
      processBuff (buff);
      args = buff;
    }

    if (routeok && msgok) {
      if (route == ROUTE_MAIN) {
        sockhSendMessage (mainSock, ROUTE_CLICOMM, route, msg, args);
      }
      if (route == ROUTE_PLAYER) {
        sockhSendMessage (playerSock, ROUTE_CLICOMM, route, msg, args);
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
