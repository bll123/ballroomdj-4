#include "config.h"

#if __WINNT__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include <winsock2.h>
#include <windows.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "osutils.h"
#include "tmutil.h"

char *
osRegistryGet (char *key, char *name)
{
  char    *rval = NULL;
  DWORD   dwRet;
  HKEY    hkey;
  LSTATUS rc;
  unsigned char buff [512];
  DWORD   len = 512;

  *buff = '\0';

  rc = RegOpenKeyEx (
      HKEY_CURRENT_USER,
      key,
      0,
      KEY_QUERY_VALUE,
      &hkey
      );

  dwRet = RegQueryValueEx (
      hkey,
      name,
      NULL,
      NULL,
      buff,
      &len
      );

  rval = strdup ((char *) buff);

  RegCloseKey (hkey);

  return rval;
}

char *
osGetSystemFont (const char *gsettingspath)
{
  return NULL;
}

int
osCreateLink (const char *target, const char *linkpath)
{
  return -1;
}

pid_t
osProcessStart (char *targv[], int flags, void **handle, char *outfname)
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
    inherit = TRUE;
  }

  buff [0] = '\0';
  idx = 0;
  while (targv [idx] != NULL) {
    /* quote everything on windows. the batch files must be adjusted to suit */
    snprintf (tbuff, sizeof (tbuff), "\"%s\"", targv [idx++]);
    strlcat (buff, tbuff, MAXPATHLEN);
    strlcat (buff, " ", MAXPATHLEN);
  }
  wbuff = osToFSFilename (buff);

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
    WaitForSingleObject (pi.hProcess, INFINITE);
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
  free (wbuff);
  return pid;
}

/* creates a pipe for re-direction and grabs the output */
pid_t
osProcessPipe (const char *targv[], int flags, char *rbuff, size_t sz)
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
  DWORD               bytesRead;

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
  wbuff = osToFSFilename (buff);

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

  if ((flags & OS_PROC_WAIT) == OS_PROC_WAIT) {
    WaitForSingleObject (pi.hProcess, INFINITE);
  }

  CloseHandle (handleStdoutWrite);

  if (rbuff != NULL) {
    /* the application wants all the data in one chunk */
    ReadFile (handleStdoutRead, rbuff, sz, &bytesRead, NULL);
    rbuff [bytesRead] = '\0';

    CloseHandle (handleStdoutRead);
    CloseHandle (pi.hProcess);
  }

  free (wbuff);
  return pid;
}

void *
osToFSFilename (const char *fname)
{
  OS_FS_CHAR_TYPE *tfname = NULL;
  size_t      len;

  /* the documentation lies; len does not include room for the null byte */
  len = MultiByteToWideChar (CP_UTF8, 0, fname, strlen (fname), NULL, 0);
  tfname = malloc ((len + 1) * OS_FS_CHAR_SIZE);
  MultiByteToWideChar (CP_UTF8, 0, fname, strlen (fname), tfname, len);
  tfname [len] = L'\0';
  return tfname;
}

char *
osFromFSFilename (const void *fname)
{
  char        *tfname = NULL;
  size_t      len;

  /* the documentation lies; len does not include room for the null byte */
  len = WideCharToMultiByte (CP_UTF8, 0, fname, -1, NULL, 0, NULL, NULL);
  tfname = malloc (len + 1);
  WideCharToMultiByte (CP_UTF8, 0, fname, -1, tfname, len, NULL, NULL);
  tfname [len] = '\0';
  assert (tfname != NULL);
  return tfname;
}

#endif /* __WINNT__ */
