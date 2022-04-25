#include "config.h"

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

#if _hdr_fcntl
# include <fcntl.h>
#endif
#if _sys_wait
# include <sys/wait.h>
#endif

#if _hdr_winsock2
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmissing-declarations"
# include <winsock2.h>
# pragma clang diagnostic pop
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdj4.h"
#include "bdjstring.h"
#include "osutils.h"
#include "tmutil.h"

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

/* handles redirection to a file */
pid_t
osProcessStart (char *targv[], int flags, void **handle, char *outfname)
{
  pid_t       pid;

#if _lib_fork
  {
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

      if (outfname != NULL) {
        int fd = open (outfname, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRWXG);
        if (fd < 0) {
          outfname = NULL;
        } else {
          ostdout = dup (STDOUT_FILENO);
          ostderr = dup (STDERR_FILENO);
          dup2 (fd, STDOUT_FILENO);
          dup2 (fd, STDERR_FILENO);
          close (fd);
        }
      }

      rc = execv (targv [0], targv);
      if (rc < 0) {
        fprintf (stderr, "unable to execute %s %d %s\n", targv [0], errno, strerror (errno));
        exit (1);
      }

      exit (0);
    }

    pid = tpid;
    if ((flags & OS_PROC_WAIT) == OS_PROC_WAIT) {
      waitpid (pid, NULL, 0);
    }
  }
#endif

#if _lib_CreateProcess
  {
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
  }
#endif
  return pid;
}

/* creates a pipe for re-direction and grabs the output */
pid_t
osProcessPipe (char *targv[], int flags, char *rbuff, size_t sz)
{
  pid_t   pid;

#if _lib_fork
  {
    int         rc;
    pid_t       tpid;
    int         pipefd [2];
    int         ostdout;
    int         ostderr;
    ssize_t     bytesread;

    if (pipe (pipefd) < 0) {
      return -1;
    }

    /* this may be slower, but it works; speed is not a major issue */
    tpid = fork ();
    if (tpid < 0) {
      return tpid;
    }

    if (tpid == 0) {
      /* child */
      /* close any open file descriptors, but not the pipe write side */
      for (int i = 3; i < 30; ++i) {
        if (pipefd [1] == i) {
          continue;
        }
        close (i);
      }

      ostdout = dup (STDOUT_FILENO);
      ostderr = dup (STDERR_FILENO);
      dup2 (pipefd [1], STDOUT_FILENO);
      dup2 (pipefd [1], STDERR_FILENO);

      rc = execv (targv [0], targv);
      if (rc < 0) {
        fprintf (stderr, "unable to execute %s %d %s\n", targv [0], errno, strerror (errno));
        exit (1);
      }

      exit (0);
    }

    /* write end of pipe is not needed by parent */
    close (pipefd [1]);

    pid = tpid;
    if ((flags & OS_PROC_WAIT) == OS_PROC_WAIT) {
      waitpid (pid, NULL, 0);
    }

    if (rbuff != NULL) {
      /* the application wants all the data in one chunk */
      bytesread = read (pipefd [0], rbuff, sz);
      rbuff [sz - 1] = '\0';
      if (bytesread < (ssize_t) sz) {
        rbuff [bytesread] = '\0';
      }
      close (pipefd [0]);
    } else {
      /* set up so the application can read from stdin */
      close (STDIN_FILENO);
      dup2 (pipefd [0], STDIN_FILENO);
    }
  }
#endif

#if _lib_CreateProcess
  {
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
      fprintf (stderr, "getlasterr: %d\n", err);
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
  }
#endif
  return pid;
}

void *
osToFSFilename (const char *fname)
{
  OS_FS_CHAR_TYPE *tfname = NULL;

#if OS_FS_CHAR_TYPE == wchar_t && _lib_MultiByteToWideChar
  {
    size_t      len;

    /* the documentation lies; len does not include room for the null byte */
    len = MultiByteToWideChar (CP_UTF8, 0, fname, strlen (fname), NULL, 0);
    tfname = malloc ((len + 1) * OS_FS_CHAR_SIZE);
    MultiByteToWideChar (CP_UTF8, 0, fname, strlen (fname), tfname, len);
    tfname [len] = L'\0';
  }
#else
  tfname = strdup (fname);
#endif
  assert (tfname != NULL);
  return tfname;
}

char *
osFromFSFilename (const void *fname)
{
  char        *tfname = NULL;

#if OS_FS_CHAR_TYPE == wchar_t && _lib_WideCharToMultiByte
  {
    size_t      len;
    BOOL        used;
    static char *defChar = "?";

    /* the documentation lies; len does not include room for the null byte */
    len = WideCharToMultiByte (CP_UTF8, 0, fname, -1, NULL, 0, defChar, &used);
    tfname = malloc (len + 1);
    WideCharToMultiByte (CP_UTF8, 0, fname, -1, tfname, len, defChar, &used);
    tfname [len] = '\0';
  }
#else
  tfname = strdup (fname);
#endif
  assert (tfname != NULL);
  return tfname;
}

dirhandle_t *
osDirOpen (const char *dirname)
{
  dirhandle_t   *dirh;

  dirh = malloc (sizeof (dirhandle_t));
  assert (dirh != NULL);
  dirh->dirname = NULL;
  dirh->dh = NULL;

#if _lib_FindFirstFileW
  {
    size_t        len = 0;

    dirh->dhandle = INVALID_HANDLE_VALUE;
    len = strlen (dirname) + 3;
    dirh->dirname = malloc (len);
    strlcpy (dirh->dirname, dirname, len);
    strlcat (dirh->dirname, "\\*", len);
  }
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

    wdirname = osToFSFilename (dirh->dirname);
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
    fname = osFromFSFilename (filedata.cFileName);
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"

int
osSetEnv (const char *name, const char *value)
{
  int     rc;
  char    tbuff [4096];

  /* setenv is better */
#if _lib_setenv
  rc = setenv (name, value, 1);
#else
  snprintf (tbuff, sizeof (tbuff), "%s=%s", name, value);
  rc = putenv (tbuff);
#endif
  return rc;
}

#pragma GCC diagnostic pop
#pragma clang diagnostic pop


char *
osRegistryGet (char *key, char *name)
{
  char    *rval = NULL;

#if _lib_RegOpenKeyEx
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

  rval = strdup (buff);

  RegCloseKey (hkey);
#endif

  return rval;
}
