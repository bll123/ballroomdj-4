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
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "conn.h"
#include "log.h"
#include "pathbld.h"
#include "portability.h"
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeclaration-after-statement"

procutil_t *
procutilStart (const char *fn, ssize_t profile, ssize_t loglvl)
{
  procutil_t  * process;
  char        tbuff [MAXPATHLEN];
  char        tmp [50];
  char        sprof [40];
  char        sloglvl [40];
  char        * targv [15];
  int         idx;


  process = malloc (sizeof (procutil_t));
  assert (process != NULL);
  process->pid = 0;
  process->hasHandle = false;
  process->started = false;

  logProcBegin (LOG_PROC, "procutilStart");
  snprintf (sprof, sizeof (sprof), "%zd", profile);
  snprintf (sloglvl, sizeof (sloglvl), "%zd", loglvl);

#if _lib_fork
  pid_t       tpid;
  int         rc;
  char        *extension;

  extension = "";
  if (isWindows ()) {
    extension = ".exe";
  }

  /* this may be slower, but it works; speed is not a major issue */
  tpid = fork ();
  if (tpid < 0) {
    logError ("fork");
    logProcEnd (LOG_PROC, "procutilStart", "fork-fail");
    return NULL;
  }
  if (tpid == 0) {
    /* child */
//    if (setsid () < 0) {
//      logError ("setsid");
//    }
    /* close any open file descriptors */
    for (int i = 3; i < 30; ++i) {
      close (i);
    }
    logEnd ();
    idx = 0;
    targv [idx++] = (char *) fn;
    targv [idx++] = "--profile";
    targv [idx++] = sprof;
    targv [idx++] = "--debug";
    targv [idx++] = sloglvl;
    targv [idx++] = NULL;
    rc = execv (targv [0], targv);
    if (rc < 0) {
      logError ("execv");
      exit (1);
    }
    exit (0);
  }
  process->pid = tpid;
#endif

#if _lib_CreateProcess
  STARTUPINFO         si;
  PROCESS_INFORMATION pi;

  process->processHandle = 0;
  memset (&si, '\0', sizeof (si));
  si.cb = sizeof(si);
  memset (&pi, '\0', sizeof (pi));

  snprintf (tmp, sizeof (tmp), "%s --profile %zd --debug %zd", fn, profile, loglvl);

  // Start the child process.
  if (! CreateProcess (
      fn,             // module name
      tmp,            // command line
      NULL,           // process handle
      NULL,           // hread handle
      FALSE,          // handle inheritance
      0,              // set to DETACHED_PROCESS
      NULL,           // parent's environment
      NULL,           // parent's starting directory
      &si,            // STARTUPINFO structure
      &pi )           // PROCESS_INFORMATION structure
  ) {
    logError ("CreateProcess");
    int err = GetLastError ();
    fprintf (stderr, "getlasterr: %d\n", err);
    return NULL;
  }

  process->pid = pi.dwProcessId;
  process->processHandle = pi.hProcess;
  process->hasHandle = true;

  CloseHandle (pi.hThread);

#endif
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

#pragma GCC diagnostic pop
#pragma clang diagnostic pop

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
procutilStartProcess (bdjmsgroute_t route, char *fname)
{
  char      tbuff [MAXPATHLEN];
  char      *extension = NULL;
  procutil_t *process = NULL;


  logProcBegin (LOG_PROC, "procutilStartProcess");

  extension = "";
  if (isWindows ()) {
    extension = ".exe";
  }
  pathbldMakePath (tbuff, sizeof (tbuff), "",
      fname, extension, PATHBLD_MP_EXECDIR);
  process = procutilStart (tbuff, sysvarsGetNum (SVL_BDJIDX),
      bdjoptGetNum (OPT_G_DEBUGLVL), uselauncher);
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

