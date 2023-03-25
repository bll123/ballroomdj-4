/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#if __WINNT__

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

#include <winsock2.h>
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
  int                 val;
  int                 inherit = FALSE;
  wchar_t             *wbuff;
  HANDLE              outhandle = INVALID_HANDLE_VALUE;

  memset (&si, '\0', sizeof (si));
  si.cb = sizeof (si);
  memset (&pi, '\0', sizeof (pi));

  if (outfname != NULL) {
    SECURITY_ATTRIBUTES sao;

    sao.nLength = sizeof (SECURITY_ATTRIBUTES);
    sao.lpSecurityDescriptor = NULL;
    sao.bInheritHandle = 1;

    outhandle = CreateFile (outfname,
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
      inherit,        // handle inheritance
      val,            // set to DETACHED_PROCESS
      NULL,           // parent's environment
      NULL,           // parent's starting directory
      &si,            // STARTUPINFO structure
      &pi )           // PROCESS_INFORMATION structure
  ) {
    int err = GetLastError ();
    fprintf (stderr, "getlasterr: %d %S\n", err, wbuff);
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

  if (outfname != NULL) {
    int         rc;
    int         count;
    struct stat statbuf;

    CloseHandle (outhandle);
    rc = stat (outfname, &statbuf);

    /* windows is mucked up; wait for the redirected output to appear */
    count = 0;
    while (rc == 0 && statbuf.st_size == 0 && count < 60) {
      mssleep (5);
      rc = stat (outfname, &statbuf);
      ++count;
    }
  }
  mdfree (wbuff);
  return pid;
}

/* creates a pipe for re-direction and grabs the output */
int
osProcessPipe (const char *targv[], int flags, char *rbuff, size_t sz, size_t *retsz)
{
  pid_t   pid;
  STARTUPINFOW        si;
  PROCESS_INFORMATION pi;
  char                tbuff [MAXPATHLEN];
  char                buff [MAXPATHLEN];
  int                 idx;
  int                 val;
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
  si.hStdError = handleStdoutWrite;
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
    fprintf (stderr, "getlasterr: %d %S\n", err, wbuff);
    return -1;
  }

  pid = pi.dwProcessId;
  CloseHandle (pi.hThread);

  CloseHandle (handleStdoutWrite);

  if (rbuff != NULL) {
    ssize_t bytesread = 0;
    bool    wait = true;

    rbuff [sz - 1] = '\0';

    while (1) {
      ReadFile (handleStdoutRead, rbuff + bytesread, sz - bytesread, &rbytes, NULL);
      bytesread += rbytes;
      if (bytesread < (ssize_t) sz) {
        rbuff [bytesread] = '\0';
      }
      if (retsz != NULL) {
        *retsz = bytesread;
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

#endif /* __WINNT__ */
