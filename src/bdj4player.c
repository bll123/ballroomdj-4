#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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

static char *   vlcDefaultOptions [] = {
      "--quiet",
      "--intf=dummy",
      "--audio-filter", "scaletempo",
      "--ignore-config",
      "--no-media-library",
      "--no-playlist-autostart",
      "--no-random",
      "--no-loop",
      "--no-repeat",
      "--play-and-stop",
      "--novideo",
};
#define VLC_DFLT_OPT_SZ (sizeof (vlcDefaultOptions) / sizeof (char *))

static Sock_t   mainSock = INVALID_SOCKET;


int
main (int argc, char *argv[])
{
  vlcData_t *     vlcData;

  sysvarsInit ();
  logStartAppend ("bdj4player", "p");
  bdjvarsInit ();

  vlcData = vlcInit (VLC_DFLT_OPT_SZ, vlcDefaultOptions);
  assert (vlcData != NULL);

  uint16_t listenPort = lbdjvars [BDJVL_PLAYER_PORT];
  sockhMainLoop (listenPort, processMsg, mainProcessing);

  vlcClose (vlcData);
  logEnd ();
  return 0;
}

/* internal routines */

static int
processMsg (long route, long msg, char *args)
{
  logProcBegin ("processMsg");
  switch (route) {
    case ROUTE_NONE: {
      break;
    }
    case ROUTE_PLAYER: {
      logMsg (LOG_DBG, "got: route-player");
      switch (msg) {
        case MSG_REQUEST_EXIT: {
          logMsg (LOG_DBG, "got: req-exit");
          logProcEnd ("processMsg", "req-exit");
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

  logProcEnd ("processMsg", "");
  return 0;
}

static void
mainProcessing (void)
{
  if (mainSock == INVALID_SOCKET) {
    int       err;

    uint16_t mainPort = lbdjvars [BDJVL_MAIN_PORT];
    mainSock = sockConnect (mainPort, &err, 1000);
    logMsg (LOG_DBG, "main-socket: %zd", (size_t) mainSock);
  }

  return;
}
