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

int
main (int argc, char *argv[])
{
  char    buff [80];
  long    route = ROUTE_NONE;
  long    msg = MSG_NONE;
  int     routeok = 0;
  int     msgok = 0;
  char    *rval;
  int     err;

  sysvarsInit (argv [0]);
  chdir (sysvars [SV_BDJ4DIR]);
  bdjvarsInit ();
  logStartAppend ("clicomm", "cl");
  uint16_t mainPort = lbdjvars [BDJVL_MAIN_PORT];
  Sock_t mainSock = sockConnect (mainPort, &err, 1000);
  while (socketInvalid (mainSock)) {
    msleep (100);
    mainSock = sockConnect (mainPort, &err, 1000);
  }

  while (1) {
    printf ("Route to: ");
    fflush (stdout);
    rval = fgets (buff, sizeof (buff), stdin);
    buff [strlen (buff) - 1] = '\0';
    routeok = 0;
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
      exit (0);
    }
    printf ("  Msg: ");
    fflush (stdout);
    rval = fgets (buff, sizeof (buff), stdin);
    buff [strlen (buff) - 1] = '\0';
    msgok = 0;
    if (strcmp (buff, "start-player") == 0) {
      msg = MSG_PLAYER_START;
      msgok = 1;
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
    if (routeok && msgok) {
      sockhSendMessage (mainSock, route, msg, NULL);
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
  }
  return 0;
}
