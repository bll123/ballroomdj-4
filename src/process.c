#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "process.h"
#include "log.h"

/* returns 0 if process exists */
int
processExists (pid_t pid)
{
#if _lib_kill
  return (kill (pid, 0));
#endif
#if _lib_OpenProcess
    /* this doesn't seem to be very stable. */
    /* it can get invalid parameter errors. */

  HANDLE hProcess;
  DWORD exitCode;

  logProcBegin (LOG_PROC, "processExists");
  hProcess = OpenProcess (PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  if (NULL == hProcess) {
    int err = GetLastError ();
    if (err == ERROR_INVALID_PARAMETER) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "openprocess: %d", err);
      logProcEnd (LOG_PROC, "openprocess", "fail-a");
      return -1;
    }
    logMsg (LOG_DBG, LOG_IMPORTANT, "openprocess: %d", err);
    logProcEnd (LOG_PROC, "openprocess", "fail-b");
    return -1;
  }

  if (GetExitCodeProcess (hProcess, &exitCode)) {
    CloseHandle (hProcess);
    logMsg (LOG_DBG, LOG_PROCESS, "found: %lld", exitCode);
    /* return 0 if the process is still active */
    logProcEnd (LOG_PROC, "processExists", "ok");
    return (exitCode != STILL_ACTIVE);
  }
  logMsg (LOG_DBG, LOG_IMPORTANT, "getexitcodeprocess: %d", GetLastError());

  CloseHandle (hProcess);
  logProcEnd (LOG_PROC, "processExists", "");
  return -1;
#endif
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeclaration-after-statement"

int
processStart (const char *fn, pid_t *pid, ssize_t profile, ssize_t loglvl)
{
  char        tmp [100];
  char        tmp2 [40];


  logProcBegin (LOG_PROC, "processStart");
  snprintf (tmp, sizeof (tmp), "%zd", profile);
  snprintf (tmp2, sizeof (tmp2), "%zd", loglvl);

#if _lib_fork
  pid_t       tpid;

  /* this may be slower, but it works; speed is not a major issue */
  tpid = fork ();
  if (tpid < 0) {
    logError ("fork");
    logProcEnd (LOG_PROC, "processStart", "fork-fail");
    return -1;
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
    int rc = execl (fn, fn, "--profile", tmp, "--debug", tmp2, NULL);
    if (rc < 0) {
      logError ("execl");
      exit (1);
    }
    exit (0);
  }
  *pid = tpid;
#endif

#if _lib_CreateProcess
  STARTUPINFO         si;
  PROCESS_INFORMATION pi;

  ZeroMemory (&si, sizeof (si));
  si.cb = sizeof(si);
  ZeroMemory (&pi, sizeof (pi));

  snprintf (tmp, sizeof (tmp), "%s --profile %zd --debug %zd", fn, profile, loglvl);

  // Start the child process.
  if (! CreateProcess (
      fn,             // module name
      tmp,            // command line
      NULL,           // process handle
      NULL,           // thread handle
      FALSE,          // handle inheritance
      0,              // set to DETACHED_PROCESS
      NULL,           // parent's environment
      NULL,           // parent's starting directory
      &si,            // STARTUPINFO structure
      &pi )           // PROCESS_INFORMATION structure
  ) {
    logError ("CreateProcess");
    return -1;
  }


  // Close process and thread handles.
  CloseHandle (pi.hProcess);
  CloseHandle (pi.hThread);

  /* ### FIX would like process id back */

#endif
  logProcEnd (LOG_PROC, "processStart", "");
  return 0;
}

#pragma GCC diagnostic pop
#pragma clang diagnostic pop

int
processKill (pid_t pid)
{
#if _lib_kill
  return (kill (pid, SIGTERM));
#endif
#if _lib_TerminateProcess
  HANDLE hProcess;
  DWORD exitCode;

  logProcBegin (LOG_PROC, "processKill");
  hProcess = OpenProcess (PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  if (NULL == hProcess) {
    int err = GetLastError ();
    if (err == ERROR_INVALID_PARAMETER) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "openprocess: %d", err);
      logProcEnd (LOG_PROC, "openprocess", "fail-a");
      return -1;
    }
    logMsg (LOG_DBG, LOG_IMPORTANT, "openprocess: %d", err);
    logProcEnd (LOG_PROC, "openprocess", "fail-b");
    return -1;
  }

  if (TerminateProcess (hProcess, 0)) {
    CloseHandle (hProcess);
    logMsg (LOG_DBG, LOG_PROCESS, "terminated: %lld", exitCode);
    return 0;
  }

  CloseHandle (hProcess);
  return -1;
#endif
}

void
processCatchSignal (void (*sigHandler)(int), int sig)
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
processIgnoreSignal (int sig)
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
processDefaultSignal (int sig)
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

