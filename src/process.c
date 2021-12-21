#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#if _hdr_windows
# include <windows.h>
#endif

#include "process.h"

#if _lib_kill

int
processExists (pid_t pid)
{
  return (kill (pid, 0));
}

#endif

#if _lib_OpenProcess

int
processExists (pid_t pid)
{
  HANDLE hProcess;
  DWORD exitCode;

  hProcess = OpenProcess (SYNCHRONIZE, FALSE, pid);
  if (NULL == hProcess) {
    if (GetLastError () == ERROR_INVALID_PARAMETER) {
      return -1;
    }

    return -1;
  }

  if (GetExitCodeProcess (hProcess, &exitCode)) {
    CloseHandle (hProcess);
    return (exitCode == STILL_ACTIVE);
  }

  CloseHandle (hProcess);
  return -1;
}

#endif
