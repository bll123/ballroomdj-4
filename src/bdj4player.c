#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "sock.h"
#include "sysvars.h"
#include "bdjvars.h"
#include "bdjmsg.h"
#include "sockh.h"
#include "log.h"
#include "bdjstring.h"
#include "vlci.h"

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

static programstate_t   programState = STATE_NOT_RUNNING;
static Sock_t           mainSock = INVALID_SOCKET;

int
main (int argc, char *argv[])
{
  vlcData_t *     vlcData;

  programState = STATE_INITIALIZING;
  sysvarsInit (argv[0]);
  logStartAppend ("bdj4player", "p", LOG_LVL_1);
  bdjvarsInit ();

  vlcData = vlcInit (VLC_DFLT_OPT_SZ, vlcDefaultOptions);
  assert (vlcData != NULL);

  programState = STATE_RUNNING;

  uint16_t listenPort = lbdjvars [BDJVL_PLAYER_PORT];
  sockhMainLoop (listenPort, processMsg, mainProcessing);

  vlcClose (vlcData);
  logEnd ();
  programState = STATE_NOT_RUNNING;
  return 0;
}

/* internal routines */

static int
processMsg (long route, long msg, char *args)
{
  logProcBegin (LOG_LVL_4, "processMsg");
  logMsg (LOG_DBG, LOG_LVL_4, "got: route: %ld msg:%ld", route, msg);
  switch (route) {
    case ROUTE_NONE: {
      break;
    }
    case ROUTE_PLAYER: {
      logMsg (LOG_DBG, LOG_LVL_4, "got: route-player");
      switch (msg) {
        case MSG_REQUEST_EXIT: {
          logMsg (LOG_DBG, LOG_LVL_4, "got: req-exit");
          programState = STATE_CLOSING;
          sockhSendMessage (mainSock, ROUTE_MAIN, MSG_CLOSE_SOCKET, NULL);
          sockClose (mainSock);
          mainSock = INVALID_SOCKET;
          logProcEnd (LOG_LVL_4, "processMsg", "req-exit");
          return 1;
        }
        default: {
          break;
        }
      }
      break;
    }
    case ROUTE_MAIN: {
      break;
    }
    case ROUTE_GUI: {
      break;
    }
    default: {
      break;
    }
  }

  logProcEnd (LOG_LVL_4, "processMsg", "");
  return 0;
}

static void
mainProcessing (void)
{
  if (programState == STATE_RUNNING && mainSock == INVALID_SOCKET) {
    int       err;

    uint16_t mainPort = lbdjvars [BDJVL_MAIN_PORT];
    mainSock = sockConnect (mainPort, &err, 1000);
    logMsg (LOG_DBG, LOG_LVL_4, "main-socket: %zd", (size_t) mainSock);
  }

  return;
}
