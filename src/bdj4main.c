#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "bdj4init.h"
#include "sockh.h"
#include "sock.h"
#include "bdjvars.h"
#include "bdjmsg.h"
#include "log.h"
#include "process.h"
#include "portability.h"
#include "bdjstring.h"
#include "sysvars.h"

static void     startPlayer (void);
static void     stopPlayer (void);
static int      processMsg (long route, long msg, char *args);
static void     mainProcessing (void);

static int      playerStarted = 0;
static Sock_t   playerSocket = INVALID_SOCKET;

int
main (int argc, char *argv[])
{
  bdj4startup (argc, argv);

  uint16_t listenPort = lbdjvars [BDJVL_MAIN_PORT];
  sockhMainLoop (listenPort, processMsg, mainProcessing);
  bdj4shutdown ();
  return 0;
}

/* internal routines */

static void
startPlayer (void)
{
  char      tbuff [MAXPATHLEN];
  pid_t     pid;

  if (playerStarted) {
    return;
  }

  logProcBegin ("startPlayer");
  strlcpy (tbuff, "bin/bdj4player", MAXPATHLEN);
  if (isWindows()) {
    strlcat (tbuff, ".exe", MAXPATHLEN);
  }
  int rc = processStart (tbuff, &pid);
  if (rc < 0) {
    logMsg (LOG_DBG, "player %s failed to start", tbuff);
  } else {
    logMsg (LOG_DBG, "player started pid: %zd", (ssize_t) pid);
    playerStarted = 1;
  }
  logProcEnd ("startPlayer", "");
}

static void
stopPlayer (void)
{
  if (! playerStarted) {
    return;
  }
  sockhSendMessage (playerSocket, ROUTE_PLAYER, MSG_REQUEST_EXIT, NULL);
}

static int
processMsg (long route, long msg, char *args)
{
  switch (route) {
    case ROUTE_PLAYER: {
      switch (msg) {
        case MSG_PLAYER_START: {
          startPlayer ();
          break;
        }
        default: {
          break;
        }
      }
      break;
    }
    case ROUTE_MAIN: {
      switch (msg) {
        case MSG_REQUEST_EXIT: {
          stopPlayer ();
          return 1;
        }
        default: {
          break;
        }
      }
      break;
    }
    case ROUTE_GUI: {
      switch (msg) {
        case MSG_PLAYER_START: {
          startPlayer ();
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

  return 0;
}

static void
mainProcessing (void)
{
  if (playerStarted && playerSocket == INVALID_SOCKET) {
    int       err;

    uint16_t playerPort = lbdjvars [BDJVL_PLAYER_PORT];
    playerSocket = sockConnect (playerPort, &err);
  }

  return;
}
