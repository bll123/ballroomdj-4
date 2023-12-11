/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
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

#include "bdj4.h"
#include "bdjstring.h"
#include "mdebug.h"
#include "osprocess.h"
#include "tmutil.h"

#if _define_WIFEXITED

static int osProcessWaitStatus (int wstatus);

#endif

/* identical on linux and mac os */
#if _lib_fork

/* handles redirection to a file */
pid_t
osProcessStart (const char *targv[], int flags, void **handle, char *outfname)
{
  pid_t       pid;
  pid_t       tpid;
  int         rc;

# if 0
    {
      int   k = 0;
      fprintf (stderr, "== start: ");
      while (targv [k] != NULL) {
        fprintf (stderr, "%s ", targv [k]);
        ++k;
      }
      fprintf (stderr, "\n");
    }
#endif

  /* this may be slower, but it works; speed is not a major issue */
  tpid = fork ();
  if (tpid < 0) {
    fprintf (stderr, "ERR: fork: %d %s\n", errno, strerror (errno));
    return tpid;
  }

  if (tpid == 0) {
    /* child */
    if ((flags & OS_PROC_DETACH) == OS_PROC_DETACH) {
      setsid ();
    }

    if (outfname != NULL) {
      int fd = open (outfname, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRWXG);
      mdextopen (fd);
      if (fd < 0) {
        outfname = NULL;
      } else {
        dup2 (fd, STDOUT_FILENO);
        dup2 (fd, STDERR_FILENO);
        mdextclose (fd);
        close (fd);
      }
    }

    rc = execv (targv [0], (char * const *) targv);
    if (rc < 0) {
      fprintf (stderr, "ERR: unable to execute %s %d %s\n", targv [0], errno, strerror (errno));
      exit (1);
    }

    exit (0);
  }

  pid = tpid;
  if ((flags & OS_PROC_WAIT) == OS_PROC_WAIT) {
    int   rc, wstatus;

    if (waitpid (pid, &wstatus, 0) < 0) {
      rc = 0;
      // fprintf (stderr, "waitpid: errno %d %s\n", errno, strerror (errno));
    } else {
      rc = osProcessWaitStatus (wstatus);
    }
    return rc;
  }

  return pid;
}

#endif /* if _lib_fork */

/* identical on linux and mac os */
#if _lib_fork

/* creates a pipe for re-direction and grabs the output */
int
osProcessPipe (const char *targv[], int flags, char *rbuff, size_t sz, size_t *retsz)
{
  pid_t   pid;
  int     rc = 0;
  pid_t   tpid;
  int     pipefd [2];

  flags |= OS_PROC_WAIT;      // required

  if (rbuff != NULL) {
    *rbuff = '\0';
  }
  if (retsz != NULL) {
    *retsz = 0;
  }

  if (pipe (pipefd) < 0) {
    return -1;
  }
  mdextopen (pipefd [0]);
  mdextopen (pipefd [1]);

# if 0
    {
      int   k = 0;
      fprintf (stderr, "== pipe: ");
      while (targv [k] != NULL) {
        fprintf (stderr, "%s ", targv [k]);
        ++k;
      }
      fprintf (stderr, "\n");
    }
#endif

  /* this may be slower, but it works; speed is not a major issue */
  tpid = fork ();
  if (tpid < 0) {
    fprintf (stderr, "ERR: %d %s\n", errno, strerror (errno));
    return tpid;
  }

  if (tpid == 0) {
    /* child */
    /* close the pipe read side */
    mdextclose (pipefd [0]);
    close (pipefd [0]);

    /* send both stdout and stderr to the same fd */
    dup2 (pipefd [1], STDOUT_FILENO);
    if ((flags & OS_PROC_NOSTDERR) != OS_PROC_NOSTDERR) {
      dup2 (pipefd [1], STDERR_FILENO);
    }
    mdextclose (pipefd [1]);
    close (pipefd [1]);

    rc = execv (targv [0], (char * const *) targv);
    if (rc < 0) {
      fprintf (stderr, "unable to execute %s %d %s\n", targv [0], errno, strerror (errno));
      exit (1);
    }

    exit (0);
  }

  /* write end of pipe is not needed by parent */
  mdextclose (pipefd [1]);
  close (pipefd [1]);

  pid = tpid;

  {
    bool    wait = true;
    int     wstatus;
    ssize_t bytesread = 0;

    if (rbuff != NULL) {
      rbuff [sz - 1] = '\0';
    }

    wait = true;
    while (wait) {
      if (rbuff != NULL) {
        size_t    rbytes;

        rbytes = read (pipefd [0], rbuff + bytesread, sz - bytesread);
        bytesread += rbytes;
        if (bytesread < (ssize_t) sz) {
          rbuff [bytesread] = '\0';
        }
        if (retsz != NULL) {
          *retsz = bytesread;
        }
      }
      if ((flags & OS_PROC_WAIT) == OS_PROC_WAIT) {
        rc = waitpid (pid, &wstatus, WNOHANG);
        if (rc < 0) {
          rc = 0;
          wait = false;
          // fprintf (stderr, "waitpid: errno %d %s\n", errno, strerror (errno));
        } else if (rc != 0) {
          rc = osProcessWaitStatus (wstatus);
          wait = false;
        }
        if (wait) {
          mssleep (2);
        }
      }
    }
    mdextclose (pipefd [0]);
    close (pipefd [0]);
  }

  return rc;
}

#endif /* if _lib_fork */

char *
osRunProgram (const char *prog, ...)
{
  char        data [4096];
  char        *arg;
  const char  *targv [30];
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

  *data = '\0';
  osProcessPipe (targv, OS_PROC_WAIT | OS_PROC_DETACH, data, sizeof (data), NULL);
  return mdstrdup (data);
}

#if _define_WIFEXITED

static int
osProcessWaitStatus (int wstatus)
{
  int rc = 0;

  if (WIFEXITED (wstatus)) {
    rc = WEXITSTATUS (wstatus);
  } else if (WIFSIGNALED (wstatus)) {
    rc = WTERMSIG (wstatus);
  }
  return rc;
}

#endif
