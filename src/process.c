#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#if _hdr_unistd
# include <unistd.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "process.h"
#include "log.h"

#if _lib_kill

/* boolean */
int
processExists (pid_t pid)
{
  return (kill (pid, 0));
}

#endif

#if _lib_OpenProcess

/* this doesn't seem to be very stable. */
/* it can get invalid parameter errors. */

int
processExists (pid_t pid)
{
  HANDLE hProcess;
  DWORD exitCode;

  logProcBegin ("processExists");
  hProcess = OpenProcess (PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  if (NULL == hProcess) {
    int err = GetLastError ();
    if (err == ERROR_INVALID_PARAMETER) {
      logMsg (LOG_DBG, "openprocess: %d", err);
      logProcEnd ("processExists", "fail-a");
      return -1;
    }
    logMsg (LOG_DBG, "openprocess: %d", err);
    logProcEnd ("processExists", "fail-b");
    return -1;
  }

  if (GetExitCodeProcess (hProcess, &exitCode)) {
    CloseHandle (hProcess);
    logMsg (LOG_DBG, "found: %lld", exitCode);
    /* return 0 if the process is still active */
    logProcEnd ("processExists", "ok");
    return (exitCode != STILL_ACTIVE);
  }
  logMsg (LOG_DBG, "getexitcodeprocess: %d", GetLastError());

  CloseHandle (hProcess);
  logProcEnd ("processExists", "");
  return -1;
}

#endif

#if _lib_fork

int
processStart (const char *fn, pid_t *pid)
{
  pid_t       tpid;

  logProcBegin ("processStart");
  tpid = fork ();
  if (tpid < 0) {
    logError ("fork");
    logProcEnd ("processStart", "fork-fail");
    return -1;
  }
  if (tpid == 0) {
    logEnd ();
    int rc = execl (fn, fn, NULL);
    if (rc < 0) {
      logError ("execl");
      exit (1);
    }
    exit (0);
  }
  *pid = tpid;
  logProcEnd ("processStart", "");
  return 0;
}

#endif

#if _lib_CreateProcess

int
processStart (const char *fn, pid_t *pid)
{
  STARTUPINFO         si;
  PROCESS_INFORMATION pi;

  ZeroMemory (&si, sizeof (si));
  si.cb = sizeof(si);
  ZeroMemory (&pi, sizeof (pi));

  // Start the child process.
  if (!CreateProcess (
    fn,             // module name
    NULL,           // command line
    NULL,           // process handle
    NULL,           // thread handle
    FALSE,          // handle inheritance
    0,              // creation flags
    NULL,           // parent's environment
    NULL,           // parent's starting directory
    &si,            // STARTUPINFO structure
    &pi )           // PROCESS_INFORMATION structure
  ) {
    printf ("CreateProcess failed (%ld).\n", GetLastError());
    return -1;
  }


  // Close process and thread handles.
  CloseHandle (pi.hProcess);
  CloseHandle (pi.hThread);

  /* ### FIX would like process id back */

  return 0;
}

#endif
