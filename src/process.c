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

#include "process.h"
#include "log.h"

/* returns 0 if process exists */
int
processExists (process_t *process)
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
    hProcess = processGetProcessHandle (process->pid);
  }

  if (GetExitCodeProcess (hProcess, &exitCode)) {
    logMsg (LOG_DBG, LOG_PROCESS, "found: %lld", exitCode);
    /* return 0 if the process is still active */
    logProcEnd (LOG_PROC, "processExists", "ok");
    if (! process->hasHandle) {
      CloseHandle (hProcess);
    }
    return (exitCode != STILL_ACTIVE);
  }
  logMsg (LOG_DBG, LOG_IMPORTANT, "getexitcodeprocess: %d", GetLastError());

  if (! process->hasHandle) {
    CloseHandle (hProcess);
  }
  logProcEnd (LOG_PROC, "processExists", "");
  return -1;
#endif
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeclaration-after-statement"

process_t *
processStart (const char *fn, ssize_t profile, ssize_t loglvl)
{
  process_t   *process;
  char        tmp [100];
  char        tmp2 [40];


  process = malloc (sizeof (process_t));
  assert (process != NULL);
  process->pid = 0;
  process->hasHandle = false;
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
    int rc = execl (fn, fn, "--profile", tmp, "--debug", tmp2, NULL);
    if (rc < 0) {
      logError ("execl");
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
  logProcEnd (LOG_PROC, "processStart", "");
  return process;
}

void
processFree (process_t *process)
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
processKill (process_t *process)
{
#if _lib_kill
  return (kill (process->pid, SIGTERM));
#endif
#if _lib_TerminateProcess
  HANDLE hProcess = process->processHandle;
  DWORD exitCode = 0;

  if (! process->hasHandle) {
    return -1;
  }

  logProcBegin (LOG_PROC, "processKill");
  if (TerminateProcess (hProcess, 0)) {
    logMsg (LOG_DBG, LOG_PROCESS, "terminated: %lld", exitCode);
    return 0;
  }

  return -1;
#endif
}

#if _typ_HANDLE

HANDLE
processGetProcessHandle (pid_t pid)
{
  HANDLE  hProcess = NULL;

  hProcess = OpenProcess (PROCESS_QUERY_LIMITED_INFORMATION,
      FALSE, pid);
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

#endif

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

