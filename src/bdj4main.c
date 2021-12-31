#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "bdj4init.h"
#include "sockh.h"
#include "sock.h"
#include "bdjvars.h"
#include "log.h"
#include "process.h"
#include "portability.h"

static void     startPlayer (void);
static void     processMsg (long route, long msg, char *args);
static void     mainProcessing (void);

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
  }
  logProcEnd ("startPlayer", "");
}

static void
processMsg (long route, long msg, char *args)
{
  return;
}

static void
mainProcessing (void)
{
  return;
}
