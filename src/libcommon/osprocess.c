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
#include "osprocess.h"

static int osProcessWaitStatus (int wstatus);

/* identical on linux and mac os */
#if _lib_fork

/* handles redirection to a file */
pid_t
osProcessStart (const char *targv[], int flags, void **handle, char *outfname)
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
    int   rc, wstatus;

    waitpid (pid, &wstatus, 0);
    rc = osProcessWaitStatus (wstatus);
    return rc;
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
    int   rc, wstatus;

    waitpid (pid, &wstatus, 0);
    rc = osProcessWaitStatus (wstatus);
    /* more processing to do, so overload the pid return */
    pid = rc;
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

char *
osRunProgram (const char *prog, ...)
{
  char        data [4096];
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

static int
osProcessWaitStatus (int wstatus)
{
  int rc = 0;

  if (WIFEXITED (wstatus)) {
    rc = WEXITSTATUS (wstatus);
  }
  if (WIFSIGNALED (wstatus)) {
    rc = WTERMSIG (wstatus);
  }
  return rc;
}
