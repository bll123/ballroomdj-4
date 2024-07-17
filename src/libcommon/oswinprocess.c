/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#if _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "mdebug.h"
#include "osprocess.h"
#include "osutils.h"
#include "tmutil.h"


pid_t
osProcessStart (const char *targv[], int flags, void **handle, char *outfname)
{
  pid_t               pid;
  STARTUPINFOW        si;
  PROCESS_INFORMATION pi;
  char                tbuff [MAXPATHLEN];
  char                buff [MAXPATHLEN];
  int                 idx;
  DWORD               val;
  BOOL                inherit = FALSE;
  wchar_t             *wbuff = NULL;
  wchar_t             *woutfname = NULL;
  HANDLE              outhandle = INVALID_HANDLE_VALUE;

  memset (&si, '\0', sizeof (si));
  si.cb = sizeof (si);
  memset (&pi, '\0', sizeof (pi));

  if (outfname != NULL && *outfname) {
    SECURITY_ATTRIBUTES sao;

    sao.nLength = sizeof (SECURITY_ATTRIBUTES);
    sao.lpSecurityDescriptor = NULL;
    sao.bInheritHandle = 1;

    woutfname = osToWideChar (outfname);
    outhandle = CreateFileW (woutfname,
      GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      &sao,
      CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL,
      NULL);
    si.hStdOutput = outhandle;
    si.hStdError = outhandle;
    si.hStdInput = GetStdHandle (STD_INPUT_HANDLE);
    si.dwFlags |= STARTF_USESTDHANDLES;
    inherit = true;
  }

  buff [0] = '\0';
  idx = 0;
  while (targv [idx] != NULL) {
    /* quote everything on windows. the batch files must be adjusted to suit */
    snprintf (tbuff, sizeof (tbuff), "\"%s\"", targv [idx++]);
    strlcat (buff, tbuff, MAXPATHLEN);
    strlcat (buff, " ", MAXPATHLEN);
  }
  wbuff = osToWideChar (buff);

  val = 0;
  if ((flags & OS_PROC_DETACH) == OS_PROC_DETACH) {
    val |= DETACHED_PROCESS;
    if ((flags & OS_PROC_WINDOW_OK) != OS_PROC_WINDOW_OK) {
      val |= CREATE_NO_WINDOW;
    }
  }

  /* windows and its stupid space-in-name and quoting issues */
  /* leave the module name as null, as otherwise it would have to be */
  /* a non-quoted version.  the command in the command line must be quoted */
  if (! CreateProcessW (
      NULL,           // module name
      wbuff,           // command line
      NULL,           // process handle
      NULL,           // hread handle
      inherit,        // handle inheritance
      val,            // set to DETACHED_PROCESS
      NULL,           // parent's environment
      NULL,           // parent's starting directory
      &si,            // STARTUPINFO structure
      &pi )           // PROCESS_INFORMATION structure
  ) {
    int err = GetLastError ();
    fprintf (stderr, "getlasterr: %d %s\n", err, buff);
    return -1;
  }

  pid = pi.dwProcessId;
  if (handle != NULL) {
    *handle = pi.hProcess;
  }
  CloseHandle (pi.hThread);

  if ((flags & OS_PROC_WAIT) == OS_PROC_WAIT) {
    DWORD rc;

    WaitForSingleObject (pi.hProcess, INFINITE);
    GetExitCodeProcess (pi.hProcess, &rc);
    pid = rc;
  }

  if (outfname != NULL && *outfname) {
    bool        rc;
    int         count;
    struct __stat64  statbuf;

    CloseHandle (outhandle);
    rc = _wstat64 (woutfname, &statbuf);

    /* windows is mucked up; wait for the redirected output to appear */
    count = 0;
    while (rc == 0 && statbuf.st_size == 0 && count < 60) {
      mssleep (5);
      rc = _wstat64 (woutfname, &statbuf);
      ++count;
    }

    mdfree (woutfname);
  }
  mdfree (wbuff);
  return pid;
}

/* creates a pipe for re-direction and grabs the output */
int
osProcessPipe (const char *targv[], int flags, char *rbuff, size_t sz, size_t *retsz)
{
  STARTUPINFOW        si;
  PROCESS_INFORMATION pi;
  char                tbuff [MAXPATHLEN];
  char                buff [MAXPATHLEN];
  int                 idx;
  DWORD               val;
  wchar_t             *wbuff;
  HANDLE              handleStdoutRead = INVALID_HANDLE_VALUE;
  HANDLE              handleStdoutWrite = INVALID_HANDLE_VALUE;
  HANDLE              handleStdinRead = INVALID_HANDLE_VALUE;
  HANDLE              handleStdinWrite = INVALID_HANDLE_VALUE;
  SECURITY_ATTRIBUTES sao;
  DWORD               rbytes;
  DWORD               rc = 0;

  flags |= OS_PROC_WAIT;      // required

  memset (&si, '\0', sizeof (si));
  si.cb = sizeof (si);
  memset (&pi, '\0', sizeof (pi));

  sao.nLength = sizeof (SECURITY_ATTRIBUTES);
  sao.lpSecurityDescriptor = NULL;
  sao.bInheritHandle = TRUE;

  if ( ! CreatePipe (&handleStdoutRead, &handleStdoutWrite, &sao, 0) ) {
    int err = GetLastError ();
    fprintf (stderr, "createpipe: getlasterr: %d\n", err);
    return -1;
  }
  if ( ! CreatePipe (&handleStdinRead, &handleStdinWrite, &sao, 0) ) {
    int err = GetLastError ();
    fprintf (stderr, "createpipe: getlasterr: %d\n", err);
    return -1;
  }
  CloseHandle (handleStdinWrite);

  si.hStdOutput = handleStdoutWrite;
  if ((flags & OS_PROC_NOSTDERR) != OS_PROC_NOSTDERR) {
    si.hStdError = handleStdoutWrite;
  }
  if (rbuff != NULL) {
    si.hStdInput = GetStdHandle (STD_INPUT_HANDLE);
  } else {
    si.hStdInput = handleStdinRead;
  }
  si.dwFlags |= STARTF_USESTDHANDLES;

  buff [0] = '\0';
  idx = 0;
  while (targv [idx] != NULL) {
    /* quote everything on windows. the batch files must be adjusted to suit */
    snprintf (tbuff, sizeof (tbuff), "\"%s\"", targv [idx++]);
    strlcat (buff, tbuff, MAXPATHLEN);
    strlcat (buff, " ", MAXPATHLEN);
  }
  wbuff = osToWideChar (buff);

  val = 0;
  if ((flags & OS_PROC_DETACH) == OS_PROC_DETACH) {
    val |= DETACHED_PROCESS;
    val |= CREATE_NO_WINDOW;
  }

  /* windows and its stupid space-in-name and quoting issues */
  /* leave the module name as null, as otherwise it would have to be */
  /* a non-quoted version.  the command in the command line must be quoted */
  if (! CreateProcessW (
      NULL,           // module name
      wbuff,           // command line
      NULL,           // process handle
      NULL,           // hread handle
      TRUE,           // handle inheritance
      val,            // set to DETACHED_PROCESS
      NULL,           // parent's environment
      NULL,           // parent's starting directory
      &si,            // STARTUPINFO structure
      &pi )           // PROCESS_INFORMATION structure
  ) {
    int err = GetLastError ();
    fprintf (stderr, "getlasterr: %d %s\n", err, buff);
    return -1;
  }

  CloseHandle (pi.hThread);

  CloseHandle (handleStdoutWrite);

  {
    ssize_t bytesread = 0;
    bool    wait = true;

    if (rbuff != NULL) {
      rbuff [sz - 1] = '\0';
    }

    while (1) {
      if (rbuff != NULL) {
        ReadFile (handleStdoutRead, rbuff + bytesread, sz - bytesread, &rbytes, NULL);
        bytesread += rbytes;
        if (bytesread < (ssize_t) sz) {
          rbuff [bytesread] = '\0';
        }
        if (retsz != NULL) {
          *retsz = bytesread;
        }
      }
      if (! wait) {
        break;
      }
      if ((flags & OS_PROC_WAIT) == OS_PROC_WAIT) {
        if (WaitForSingleObject (pi.hProcess, 2) != WAIT_TIMEOUT) {
          GetExitCodeProcess (pi.hProcess, &rc);
          wait = false;
        }
      }
      if (wait) {
        Sleep (2);
      }
    }
    CloseHandle (handleStdoutRead);
    CloseHandle (pi.hProcess);
  }

  mdfree (wbuff);
  return rc;
}

#endif /* _WIN32 */
