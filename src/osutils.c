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
osProcessStart (char *targv[], int flags, void **handle)
{
  pid_t       pid;

#if _lib_fork
  pid_t       tpid;
  int         rc;

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
    rc = execv (targv [0], targv);
    if (rc < 0) {
      fprintf (stderr, "unable to execute %s %d %s\n", targv [0], errno, strerror (errno));
      exit (1);
    }
    exit (0);
  }
  pid = tpid;
#endif

#if _lib_CreateProcess
  STARTUPINFO         si;
  PROCESS_INFORMATION pi;
  char                tbuff [MAXPATHLEN];
  char                buff [MAXPATHLEN];
  int                 idx;
  int                 val;

  memset (&si, '\0', sizeof (si));
  si.cb = sizeof(si);
  memset (&pi, '\0', sizeof (pi));

  buff [0] = '\0';
  idx = 0;
  while (targv [idx] != NULL) {
    /* quote everything on windows. the batch files must be adjusted to suit */
    snprintf (tbuff, sizeof (tbuff), "\"%s\"", targv [idx++]);
    strlcat (buff, tbuff, MAXPATHLEN);
    strlcat (buff, " ", MAXPATHLEN);
  }

  val = 0;
  if ((flags & OS_PROC_DETACH) == OS_PROC_DETACH) {
    val = DETACHED_PROCESS;
  }

  /* windows and its stupid space-in-name and quoting issues */
  /* leave the module name as null, as otherwise it would have to be */
  /* a non-quoted version.  the command in the command line must be quoted */
  if (! CreateProcess (
      NULL,           // module name
      buff,           // command line
      NULL,           // process handle
      NULL,           // hread handle
      FALSE,          // handle inheritance
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

#endif
  return pid;
}

#pragma GCC diagnostic pop
#pragma clang diagnostic pop
