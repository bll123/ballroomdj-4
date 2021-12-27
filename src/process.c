#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#if _hdr_windows
# include <windows.h>
#endif

#include "process.h"

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

  hProcess = OpenProcess (PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  if (NULL == hProcess) {
    if (GetLastError () == ERROR_INVALID_PARAMETER) {
      return -1;
    }
    return -1;
  }

  if (GetExitCodeProcess (hProcess, &exitCode)) {
    CloseHandle (hProcess);
    /* return 0 if the process is still active */
    return (exitCode != STILL_ACTIVE);
  }

  CloseHandle (hProcess);
  return -1;
}

#endif
