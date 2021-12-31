#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "vlci.h"
#include "sysvars.h"
#include "bdjvars.h"
#include "bdjmsg.h"
#include "sockh.h"
#include "sock.h"
#include "log.h"
#include "bdjstring.h"

static int      processMsg (long route, long msg, char *args);
static void     mainProcessing (void);

static Sock_t   mainSock = INVALID_SOCKET;

int
main (int argc, char *argv[])
{
  vlcData_t *     vlcData;

  sysvarsInit ();
  logStartAppend ("bdj4player", "p");

  vlcData = vlcInit (0, NULL);
  assert (vlcData != NULL);

  uint16_t listenPort = lbdjvars [BDJVL_PLAYER_PORT];
  sockhMainLoop (listenPort, processMsg, mainProcessing);

  vlcClose (vlcData);
  return 0;
}

/* internal routines */

static int
processMsg (long route, long msg, char *args)
{
  switch (route) {
    case ROUTE_PLAYER: {
      switch (msg) {
        case MSG_REQUEST_EXIT: {
          return 1;
        }
        default: {
          break;
        }
      }
      break;
    }
    case ROUTE_MAIN: {
      switch (msg) {
        default: {
          break;
        }
      }
      break;
    }
    case ROUTE_GUI: {
      switch (msg) {
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

  return 0;
}

static void
mainProcessing (void)
{
  if (mainSock == INVALID_SOCKET) {
    int       err;

    uint16_t mainPort = lbdjvars [BDJVL_MAIN_PORT];
    mainSock = sockConnect (mainPort, &err);
  }

  return;
}
