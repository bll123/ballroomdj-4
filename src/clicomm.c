#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#if _hdr_unistd
# include <unistd.h>
#endif

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
  long            route = ROUTE_NONE;
  long            msg = MSG_NONE;
  int             routeok = 0;
  int             msgok = 0;
  int             argsok = 0;
  int             playerstart = 0;
  char            *args;
  char            *rval;
  int             err;
  Sock_t          mainSock = INVALID_SOCKET;
  Sock_t          playerSock = INVALID_SOCKET;


  sysvarsInit (argv [0]);
  if (chdir (sysvars [SV_BDJ4DIR]) < 0) {
    fprintf (stderr, "Unable to chdir: %s\n", sysvars [SV_BDJ4DIR]);
    exit (1);
  }
  bdjvarsInit ();
  logStartAppend ("clicomm", "cl", LOG_LVL_6);

  uint16_t mainPort = bdjvarsl [BDJVL_MAIN_PORT];
  mainSock = sockConnect (mainPort, &err, 1000);
  while (socketInvalid (mainSock)) {
    msleep (100);
    mainSock = sockConnect (mainPort, &err, 1000);
  }

  while (1) {
    printf ("Route to: ");
    fflush (stdout);
    rval = fgets (buff, sizeof (buff), stdin);
    buff [strlen (buff) - 1] = '\0';
    processBuff (buff);
    routeok = 0;
    argsok = 0;
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
      sockClose (mainSock);
      logEnd ();
      exit (0);
    }

    printf ("  Msg: ");
    fflush (stdout);
    rval = fgets (buff, sizeof (buff), stdin);
    buff [strlen (buff) - 1] = '\0';
    processBuff (buff);
    msgok = 0;
    if (strcmp (buff, "start-player") == 0) {
      msg = MSG_PLAYER_START;
      msgok = 1;
      playerstart = 1;
    }
    if (strcmp (buff, "prep") == 0) {
      msg = MSG_PREP_SONG;
      msgok = 1;
      argsok = 1;
    }
    if (strcmp (buff, "play") == 0) {
      msg = MSG_PLAY_SONG;
      msgok = 1;
      argsok = 1;
    }
    if (strcmp (buff, "exit") == 0) {
      msg = MSG_REQUEST_EXIT;
      msgok = 1;
    }
    if (strcmp (buff, "force-exit") == 0) {
      msg = MSG_FORCE_EXIT;
      msgok = 1;
    }
    if (strcmp (buff, "cliexit") == 0) {
      sockClose (mainSock);
      exit (0);
    }

    args = NULL;
    if (argsok) {
      printf ("  Args: ");
      fflush (stdout);
      rval = fgets (buff, sizeof (buff), stdin);
      buff [strlen (buff) - 1] = '\0';
      processBuff (buff);
      args = buff;
    }

    if (routeok && msgok) {
      if (route == ROUTE_MAIN) {
        sockhSendMessage (mainSock, route, msg, args);
      }
      if (route == ROUTE_PLAYER) {
        sockhSendMessage (playerSock, route, msg, args);
      }
    } else {
      if (! routeok) {
        fprintf (stdout, "    invalid route\n");
        fflush (stdout);
      }
      if (! msgok) {
        fprintf (stdout, "    invalid msg\n");
        fflush (stdout);
      }
    }

    if (playerstart) {
      uint16_t playerPort = bdjvarsl [BDJVL_PLAYER_PORT];
      playerSock = sockConnect (playerPort, &err, 1000);
      while (socketInvalid (playerSock)) {
        msleep (100);
        playerSock = sockConnect (playerPort, &err, 1000);
      }
      playerstart = 0;
    }
  }

  return 0;
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
