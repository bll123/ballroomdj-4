#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#if _hdr_winsock2
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmissing-declarations"
# include <winsock2.h>
# pragma clang diagnostic pop
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdj4.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "conn.h"
#include "log.h"
#include "osutils.h"
#include "pathbld.h"
#include "procutil.h"
#include "sysvars.h"
#include "tmutil.h"

#if _typ_HANDLE
static HANDLE procutilGetProcessHandle (pid_t pid, DWORD procaccess);
#endif

/* returns 0 if process exists */
int
procutilExists (procutil_t *process)
{
#if _lib_kill
  return (kill (process->pid, 0));
#endif
#if _lib_OpenProcess
    /* this doesn't seem to be very stable. */
    /* it can get invalid parameter errors. */

  HANDLE hProcess = process->processHandle;
  DWORD exitCode;

  if (! process->hasHandle) {
    hProcess = procutilGetProcessHandle (process->pid, PROCESS_QUERY_LIMITED_INFORMATION);
  }

  if (GetExitCodeProcess (hProcess, &exitCode)) {
    logMsg (LOG_DBG, LOG_PROCESS, "found: %lld", exitCode);
    /* return 0 if the process is still active */
    logProcEnd (LOG_PROC, "procutilExists", "ok");
    if (! process->hasHandle) {
      CloseHandle (hProcess);
    }
    return (exitCode != STILL_ACTIVE);
  }
  logMsg (LOG_DBG, LOG_IMPORTANT, "getexitcodeprocess: %d", GetLastError());

  if (! process->hasHandle) {
    CloseHandle (hProcess);
  }
  logProcEnd (LOG_PROC, "procutilExists", "");
  return -1;
#endif
}

procutil_t *
procutilStart (const char *fn, ssize_t profile, ssize_t loglvl,
    int procutilflag, char *aargs [])
{
  procutil_t  * process;
  char        sprof [40];
  char        sloglvl [40];
  char        * targv [30];
  int         idx;
  int         flags;


  process = malloc (sizeof (procutil_t));
  assert (process != NULL);
  process->pid = 0;
  process->hasHandle = false;
  process->started = false;

  logProcBegin (LOG_PROC, "procutilStart");
  snprintf (sprof, sizeof (sprof), "%zd", profile);
  snprintf (sloglvl, sizeof (sloglvl), "%zd", loglvl);

  idx = 0;
  targv [idx++] = (char *) fn;
  targv [idx++] = "--profile";
  targv [idx++] = sprof;
  targv [idx++] = "--debug";
  targv [idx++] = sloglvl;
  targv [idx++] = "--bdj4";

  if (aargs != NULL) {
    char    *p;
    int     aidx;

    aidx = 0;
    while ((p = aargs [aidx++]) != NULL) {
      targv [idx++] = p;
    }
  }

  targv [idx++] = NULL;

  process->processHandle = NULL;
  flags = OS_PROC_DETACH;
  if ((procutilflag & PROCUTIL_NO_DETACH) != PROCUTIL_NO_DETACH) {
    flags = OS_PROC_NONE;
  }
  process->pid = osProcessStart (targv, flags, &process->processHandle, NULL);
  if (process->processHandle != NULL) {
    process->hasHandle = true;
  }

  logProcEnd (LOG_PROC, "procutilStart", "");
  process->started = true;
  return process;
}

void
procutilFree (procutil_t *process)
{
  if (process != NULL) {
    if (process->hasHandle) {
#if _typ_HANDLE
      CloseHandle (process->processHandle);
#endif
    }
    free (process);
  }
}

int
procutilKill (procutil_t *process, bool force)
{
#if _lib_kill
  int         sig = SIGTERM;

  if (force) {
    sig = SIGABRT;
  }

  return (kill (process->pid, sig));
#endif
#if _lib_TerminateProcess
  HANDLE hProcess = process->processHandle;

  logProcBegin (LOG_PROC, "processKill");
  if (! process->hasHandle) {
    /* need PROCESS_TERMINATE */
    hProcess = procutilGetProcessHandle (process->pid, PROCESS_TERMINATE);
  }

  if (hProcess != NULL) {
    if (TerminateProcess (hProcess, 0)) {
      logMsg (LOG_DBG, LOG_PROCESS, "terminated");
      logProcEnd (LOG_PROC, "processKill", "ran-terminate");
      return 0;
    }
  }

  logProcEnd (LOG_PROC, "procutilKill", "fail");
  return -1;
#endif
}

void
procutilTerminate (pid_t pid, bool force)
{
  procutil_t     process;

  process.hasHandle = false;
  process.pid = pid;
  procutilKill (&process, force);
}

void
procutilCatchSignal (void (*sigHandler)(int), int sig)
{
#if _lib_sigaction
  struct sigaction    sigact;
  struct sigaction    oldact;

  memset (&sigact, '\0', sizeof (sigact));
  sigact.sa_handler = sigHandler;
  sigaction (sig, &sigact, &oldact);
#endif
#if ! _lib_sigaction && _lib_signal
  signal (sig, sigHandler);
#endif
}

void
procutilIgnoreSignal (int sig)
{
#if _lib_sigaction
  struct sigaction    sigact;
  struct sigaction    oldact;

  memset (&sigact, '\0', sizeof (sigact));
  sigact.sa_handler = SIG_IGN;
  sigaction (sig, &sigact, &oldact);
#endif
#if _lib_signal
  signal (sig, SIG_IGN);
#endif
}

void
procutilDefaultSignal (int sig)
{
#if _lib_sigaction
  struct sigaction    sigact;
  struct sigaction    oldact;

  memset (&sigact, '\0', sizeof (sigact));
  sigact.sa_handler = SIG_DFL;
  sigaction (sig, &sigact, &oldact);
#endif
#if _lib_signal
  signal (sig, SIG_DFL);
#endif
}

/* these next three routines are for management of processes */

procutil_t *
procutilStartProcess (bdjmsgroute_t route, char *fname, int detachflag,
    char *aargs [])
{
  char      tbuff [MAXPATHLEN];
  procutil_t *process = NULL;


  logProcBegin (LOG_PROC, "procutilStartProcess");

  pathbldMakePath (tbuff, sizeof (tbuff),
      fname, sysvarsGetStr (SV_OS_EXEC_EXT), PATHBLD_MP_EXECDIR);
  process = procutilStart (tbuff, sysvarsGetNum (SVL_BDJIDX),
      bdjoptGetNum (OPT_G_DEBUGLVL), detachflag, aargs);
  if (isWindows ()) {
    logMsg (LOG_DBG, LOG_BASIC, "PATH=%s\n", getenv ("PATH"));
  }
  if (process == NULL) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "%s %s failed to start", fname, tbuff);
  } else {
    logMsg (LOG_DBG, LOG_BASIC, "%s started pid: %zd", fname,
        (ssize_t) process->pid);
    process->started = true;
  }
  logProcEnd (LOG_PROC, "procutilStartProcess", "");

  return process;
}

void
procutilStopProcess (procutil_t *process, conn_t *conn,
    bdjmsgroute_t route, bool force)
{
  logProcBegin (LOG_PROC, "procutilStopProcess");
  if (! force) {
    if (! process->started) {
      logProcEnd (LOG_PROC, "procutilStopProcess", "not-started");
      return;
    }
    connSendMessage (conn, route, MSG_EXIT_REQUEST, NULL);
    connDisconnect (conn, route);
    process->started = false;
  }
  if (force) {
    procutilForceStop (process, PATHBLD_MP_USEIDX, route);
  }
  logProcEnd (LOG_PROC, "procutilStopProcess", "");
}

void
procutilForceStop (procutil_t *process, int flags, bdjmsgroute_t route)
{
  int   count = 0;
  int   exists = 1;

  while (count < 10) {
    if (procutilExists (process) != 0) {
      logMsg (LOG_DBG, LOG_MAIN, "%d exited", route);
      exists = 0;
      break;
    }
    ++count;
    mssleep (100);
  }

  if (exists) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "force-terminating %d", route);
    procutilKill (process, false);
  }
}

/* internal routines */

#if _typ_HANDLE

static HANDLE
procutilGetProcessHandle (pid_t pid, DWORD procaccess)
{
  HANDLE  hProcess = NULL;

  hProcess = OpenProcess (procaccess, FALSE, pid);
  if (NULL == hProcess) {
    int err = GetLastError ();
    if (err == ERROR_INVALID_PARAMETER) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "openprocess: %d", err);
      logProcEnd (LOG_PROC, "openprocess", "fail-a");
      return NULL;
    }
    logMsg (LOG_DBG, LOG_IMPORTANT, "openprocess: %d", err);
    logProcEnd (LOG_PROC, "openprocess", "fail-b");
    return NULL;
  }

  return hProcess;
}

#endif /* _typ_HANDLE */


