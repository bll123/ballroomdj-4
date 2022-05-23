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
#include <signal.h>
#include <locale.h>
#include <stdarg.h>

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

/* identical on linux and mac os */
#if _lib_fork

/* handles redirection to a file */
pid_t
osProcessStart (char *targv[], int flags, void **handle, char *outfname)
{
  pid_t       pid;
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

    rc = execv (targv [0], (char * const *) targv);
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

  return pid;
}

#endif /* if _lib_fork */

/* identical on linux and mac os */
#if _lib_fork

/* creates a pipe for re-direction and grabs the output */
pid_t
osProcessPipe (const char *targv[], int flags, char *rbuff, size_t sz)
{
  pid_t   pid;
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

    rc = execv (targv [0], (char * const *) targv);
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

  return pid;
}

#endif /* if _lib_fork */


/* almost a noop on linux and mac os */
#if ! _lib_MultiByteToWideChar && OS_FS_CHAR_TYPE == char

void *
osToFSFilename (const char *fname)
{
  OS_FS_CHAR_TYPE *tfname = NULL;

  tfname = strdup (fname);
  assert (tfname != NULL);
  return tfname;
}

char *
osFromFSFilename (const void *fname)
{
  char        *tfname = NULL;

  tfname = strdup (fname);
  assert (tfname != NULL);
  return tfname;
}

#endif /* not windows */

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

void
osSetStandardSignals (void (*sigHandler)(int))
{
#if _define_SIGHUP
  osCatchSignal (sigHandler, SIGHUP);
#endif
  osDefaultSignal (SIGTERM);
#if _define_SIGCHLD
  osIgnoreSignal (SIGCHLD);
#endif
}

void
osCatchSignal (void (*sigHandler)(int), int sig)
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
osIgnoreSignal (int sig)
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
osDefaultSignal (int sig)
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

/* windows setlocale() returns the old style long locale name */
char *
osGetLocale (char *buff, size_t sz)
{
#if _lib_GetSystemDefaultLocaleName
  wchar_t locbuff [LOCALE_NAME_MAX_LENGTH];
  char    *tbuff;

  GetSystemDefaultLocaleName (locbuff, LOCALE_NAME_MAX_LENGTH);
  tbuff = osFromFSFilename (locbuff);
  strlcpy (buff, tbuff, sz);
  free (tbuff);
#else
  strlcpy (buff, setlocale (LC_ALL, NULL), sz);
#endif
  return buff;
}

char *
osRunProgram (const char *prog, ...)
{
  char        data [2048];
  char        *arg;
  const char  *targv [10];
  int         targc;
  va_list     valist;

  va_start (valist, prog);

  targc = 0;
  targv [targc++] = prog;
  while ((arg = va_arg (valist, char *)) != NULL) {
    targv [targc++] = arg;
  }
  targv [targc++] = NULL;
  va_end (valist);

  osProcessPipe (targv, OS_PROC_WAIT | OS_PROC_DETACH, data, sizeof (data));
  return strdup (data);
}

#if _lib_symlink

int
osCreateLink (const char *target, const char *linkpath)
{
  int rc;

  rc = symlink (target, linkpath);
  return rc;
}

#endif
