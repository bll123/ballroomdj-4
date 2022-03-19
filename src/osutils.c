#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#if _hdr_fcntl
# include <fcntl.h>
#endif
#if _sys_wait
# include <sys/wait.h>
#endif

#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdj4.h"
#include "bdjstring.h"
#include "osutils.h"

double
dRandom (void)
{
  double      dval;

#if _lib_drand48
  dval = drand48 ();
#endif
#if ! _lib_drand48 && _lib_random
  long    lval;

  lval = random ();
  dval = (double) ival / (double) LONG_MAX;
#endif
#if ! _lib_drand48 && ! _lib_random && _lib_rand
  int       ival;

  ival = rand ();
  dval = (double) ival / (double) RAND_MAX;
#endif
  return dval;
}

void
sRandom (void)
{
  long  pid = (long) getpid ();
  long  seed = (ssize_t) time (NULL) ^ (pid + (pid << 15));

#if _lib_srand48
  srand48 (seed);
#endif
#if ! _lib_srand48 && _lib_random
  srandom ((unsigned int) seed);
#endif
#if ! _lib_srand48 && ! _lib_random && _lib_srand
  srand ((unsigned int) seed);
#endif
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeclaration-after-statement"

pid_t
osProcessStart (char *targv[], int flags, void **handle, char *outfname)
{
  pid_t       pid;

#if _lib_fork
  pid_t       tpid;
  int         rc;
  int         ostdout;
  int         ostderr;

  /* this may be slower, but it works; speed is not a major issue */
  tpid = fork ();
  if (tpid < 0) {
    return tpid;
  }

  if (tpid == 0) {
    /* child */
    if ((flags & OS_PROC_DETACH) == OS_PROC_DETACH) {
      setsid ();
    }
    /* close any open file descriptors */
    for (int i = 3; i < 30; ++i) {
      close (i);
    }

    if (outfname != NULL) {
      int fd = open (outfname, O_CREAT | O_WRONLY | O_TRUNC);
      ostdout = dup (STDOUT_FILENO);
      ostderr = dup (STDERR_FILENO);
      dup2 (fd, STDOUT_FILENO);
      dup2 (fd, STDERR_FILENO);
      close (fd);
    }

    rc = execv (targv [0], targv);
    if (rc < 0) {
      fprintf (stderr, "unable to execute %s %d %s\n", targv [0], errno, strerror (errno));
      exit (1);
    }
    if (outfname != NULL) {
      close (STDOUT_FILENO);
      close (STDERR_FILENO);
      dup2 (ostdout, STDOUT_FILENO);
      dup2 (ostderr, STDERR_FILENO);
      close (ostdout);
      close (ostderr);
    }

    exit (0);
  }

  pid = tpid;
  if ((flags & OS_PROC_WAIT) == OS_PROC_WAIT) {
    waitpid (pid, NULL, 0);
  }

#endif

#if _lib_CreateProcess
  STARTUPINFOW        si;
  PROCESS_INFORMATION pi;
  char                tbuff [MAXPATHLEN];
  char                buff [MAXPATHLEN];
  int                 idx;
  int                 val;
  int                 inherit = FALSE;
  wchar_t             *wbuff;
  FILE                *fh;
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
  wbuff = osToWideString (buff);

  val = 0;
  if ((flags & OS_PROC_DETACH) == OS_PROC_DETACH) {
    val |= DETACHED_PROCESS;
  }
  val |= CREATE_NO_WINDOW;

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
    fprintf (stderr, "getlasterr: %d\n", err);
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
    CloseHandle (outhandle);
  }
  free (wbuff);

#endif
  return pid;
}

#pragma GCC diagnostic pop
#pragma clang diagnostic pop

#if _lib_MultiByteToWideChar

wchar_t *
osToWideString (const char *fname)
{
  size_t      len;
  wchar_t     *tfname = NULL;

  /* the documentation lies; len does not include room for the null byte */
  len = MultiByteToWideChar (CP_UTF8, 0, fname, strlen (fname), NULL, 0);
  tfname = malloc ((len + 1) * sizeof (wchar_t));
  assert (tfname != NULL);
  MultiByteToWideChar (CP_UTF8, 0, fname, strlen (fname), tfname, len);
  tfname [len] = L'\0';
  return tfname;
}

char *
osToUTF8String (const wchar_t *fname)
{
  size_t      len;
  char        *tfname = NULL;
  char        *defChar = "?";
  BOOL        used;

  /* the documentation lies; len does not include room for the null byte */
  len = WideCharToMultiByte (CP_UTF8, 0, fname, -1, NULL, 0, defChar, &used);
  tfname = malloc ((len + 1) * sizeof (wchar_t));
  assert (tfname != NULL);
  WideCharToMultiByte (CP_UTF8, 0, fname, -1, tfname, len, defChar, &used);
  tfname [len] = '\0';
  return tfname;
}

#endif


dirhandle_t *
osDirOpen (const char *dirname)
{
  dirhandle_t   *dirh;
  size_t        len = 0;

  dirh = malloc (sizeof (dirhandle_t));
  assert (dirh != NULL);
  dirh->dirname = NULL;
  dirh->dh = NULL;

#if _lib_FindFirstFileW
  dirh->dhandle = INVALID_HANDLE_VALUE;
  len = strlen (dirname) + 3;
  dirh->dirname = malloc (len);
  strlcpy (dirh->dirname, dirname, len);
  strlcat (dirh->dirname, "\\*", len);
#else
  dirh->dh = opendir (dirname);
#endif

  return dirh;
}

char *
osDirIterate (dirhandle_t *dirh)
{
  char      *fname;

#if _lib_FindFirstFileW
  WIN32_FIND_DATAW filedata;
  BOOL             rc;

  if (dirh->dhandle == INVALID_HANDLE_VALUE) {
    wchar_t         *wdirname;

    wdirname = osToWideString (dirh->dirname);
    dirh->dhandle = FindFirstFileW (wdirname, &filedata);
    rc = 0;
    if (dirh->dhandle != INVALID_HANDLE_VALUE) {
      rc = 1;
    }
    free (wdirname);
  } else {
    rc = FindNextFileW (dirh->dhandle, &filedata);
  }

  fname = NULL;
  if (rc != 0) {
    fname = osToUTF8String (filedata.cFileName);
  }
#else
  struct dirent   *dirent;

  dirent = readdir (dirh->dh);
  fname = NULL;
  if (dirent != NULL) {
    fname = strdup (dirent->d_name);
  }
#endif

  return fname;
}


void
osDirClose (dirhandle_t *dirh)
{
#if _lib_FindFirstFileW
  if (dirh->dhandle != INVALID_HANDLE_VALUE) {
    FindClose (dirh->dhandle);
  }
#else
  closedir (dirh->dh);
#endif
  if (dirh->dirname != NULL) {
    free (dirh->dirname);
    dirh->dirname = NULL;
  }
  free (dirh);
}
