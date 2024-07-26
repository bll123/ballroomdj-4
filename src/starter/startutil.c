/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "bdj4.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "conn.h"
#include "fileop.h"
#include "lock.h"
#include "osprocess.h"
#include "osutils.h"
#include "pathbld.h"
#include "procutil.h"
#include "startutil.h"
#include "sysvars.h"
#include "tmutil.h"
#include "volreg.h"

static int starterCountProcesses (void);
static void starterQuickConnect (conn_t *conn, bdjmsgroute_t route);
static int starterTerminateProcesses (int termtype);

void
starterStopAllProcesses (conn_t *conn)
{
  bdjmsgroute_t route;
  char          *locknm;
  pid_t         pid;
  int           count;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_INFO, "stop-all-processes");

  count = starterCountProcesses ();
  if (count == 0) {
    logProcEnd ("begin-none");
    return;
  }

  /* send the standard exit request to the main controlling processes first */
  logMsg (LOG_DBG, LOG_IMPORTANT, "send exit request to ui");
  starterQuickConnect (conn, ROUTE_PLAYERUI);
  connSendMessage (conn, ROUTE_PLAYERUI, MSG_EXIT_REQUEST, NULL);
  starterQuickConnect (conn, ROUTE_MANAGEUI);
  connSendMessage (conn, ROUTE_MANAGEUI, MSG_EXIT_REQUEST, NULL);
  starterQuickConnect (conn, ROUTE_CONFIGUI);
  connSendMessage (conn, ROUTE_CONFIGUI, MSG_EXIT_REQUEST, NULL);

  logMsg (LOG_DBG, LOG_IMPORTANT, "after-ui sleeping");
  mssleep (1000);

  count = starterCountProcesses ();
  if (count == 0) {
    logProcEnd ("after-ui");
    return;
  }

  if (isWindows ()) {
    /* windows is slow */
    mssleep (1500);
  }

  /* send the exit request to main */
  starterQuickConnect (conn, ROUTE_MAIN);
  logMsg (LOG_DBG, LOG_IMPORTANT, "send exit request to main");
  connSendMessage (conn, ROUTE_MAIN, MSG_EXIT_REQUEST, NULL);
  logMsg (LOG_DBG, LOG_IMPORTANT, "after-main sleeping");
  mssleep (1500);

  count = starterCountProcesses ();
  if (count == 0) {
    logProcEnd ("after-main");
    return;
  }

  /* see which lock files exist, and send exit requests to them */
  count = 0;
  for (route = ROUTE_NONE + 1; route < ROUTE_MAX; ++route) {
    if (route == ROUTE_STARTERUI) {
      continue;
    }

    locknm = lockName (route);
    pid = lockExists (locknm, PATHBLD_MP_USEIDX);
    if (pid > 0) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "route %d %s exists; send exit request",
          route, msgRouteDebugText (route));
      starterQuickConnect (conn, route);
      connSendMessage (conn, route, MSG_EXIT_REQUEST, NULL);
      ++count;
    }
  }

  if (count <= 0) {
    logProcEnd ("after-exit-all");
    return;
  }

  logMsg (LOG_DBG, LOG_IMPORTANT, "after-exit-all sleeping");
  mssleep (1500);

  /* see which lock files still exist and kill the processes */
  count = starterTerminateProcesses (PROCUTIL_NORM_TERM);
  if (count <= 0) {
    logProcEnd ("after-term");
    return;
  }

  logMsg (LOG_DBG, LOG_IMPORTANT, "after-term sleeping");
  mssleep (1500);

  /* see which lock files still exist and kill the processes with */
  /* a signal that is not caught; remove the lock file */
  starterTerminateProcesses (PROCUTIL_FORCE_TERM);
  starterRemoveAllLocks ();
  starterCleanVolumeReg ();

  logProcEnd ("");
  return;
}

void
starterRemoveAllLocks (void)
{
  bdjmsgroute_t route;
  char          *locknm;
  pid_t         pid;

  /* see which lock files still exist and kill the processes with */
  /* a signal that is not caught; remove the lock file */
  for (route = ROUTE_NONE + 1; route < ROUTE_MAX; ++route) {
    if (route == ROUTE_STARTERUI) {
      continue;
    }

    locknm = lockName (route);
    pid = lockExists (locknm, PATHBLD_MP_USEIDX);
    if (pid > 0) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "removing lock %d %s",
          route, msgRouteDebugText (route));
      fileopDelete (locknm);
    }
  }
}

void
starterCleanVolumeReg (void)
{
  volregClean ();
  volregClearBDJ4Flag ();
}

void
starterPlayerStartup (void)
{
  const char    *script;

  osSuspendSleep ();

  script = bdjoptGetStr (OPT_M_STARTUP_SCRIPT);
  if (script != NULL &&
      *script &&
      fileopFileExists (script)) {
    const char  *targv [2];

    /* the startup script can be an absolute path or a relative path */
    /* if it is a relative path, it is assumed to be from the main dir */
    /* which is the current location, so no special processing is needed */
    targv [0] = script;
    targv [1] = NULL;
    osProcessStart (targv, OS_PROC_DETACH, NULL, NULL);
  }
}

void
starterPlayerShutdown (void)
{
  const char    *script;

  osResumeSleep ();

  script = bdjoptGetStr (OPT_M_SHUTDOWN_SCRIPT);
  if (script != NULL &&
      *script &&
      fileopFileExists (script)) {
    const char  *targv [2];

    /* the shutdown script can be an absolute path or a relative path */
    /* if it is a relative path, it is assumed to be from the main dir */
    /* which is the current location, so no special processing is needed */
    targv [0] = script;
    targv [1] = NULL;
    osProcessStart (targv, OS_PROC_DETACH, NULL, NULL);
  }
}

/* internal routines */

static int
starterCountProcesses (void)
{
  bdjmsgroute_t route;
  char          *locknm;
  pid_t         pid;
  int           count;

  logProcBegin ();
  count = 0;
  for (route = ROUTE_NONE + 1; route < ROUTE_MAX; ++route) {
    if (route == ROUTE_STARTERUI) {
      continue;
    }

    locknm = lockName (route);
    pid = lockExists (locknm, PATHBLD_MP_USEIDX);
    if (pid > 0) {
      ++count;
    }
  }

  logProcEnd ("");
  return count;
}

static void
starterQuickConnect (conn_t *conn, bdjmsgroute_t route)
{
  int   count;

  count = 0;
  while (! connIsConnected (conn, route)) {
    connConnect (conn, route);
    if (count > 5) {
      break;
    }
    mssleep (10);
    ++count;
  }
}

static int
starterTerminateProcesses (int termtype)
{
  bdjmsgroute_t route;
  char          *locknm;
  pid_t         pid;
  int           count = 0;

  for (route = ROUTE_NONE + 1; route < ROUTE_MAX; ++route) {
    if (route == ROUTE_STARTERUI) {
      continue;
    }

    locknm = lockName (route);
    pid = lockExists (locknm, PATHBLD_MP_USEIDX);
    if (pid > 0) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "lock %d %s exists; terminate %d",
          route, msgRouteDebugText (route), termtype);
      procutilTerminate (pid, termtype);
      ++count;
    }
  }

  return count;
}

