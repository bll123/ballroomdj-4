#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "sysvars.h"
#include "tmutil.h"
#include "lock.h"
#include "fileop.h"
#include "pathbld.h"
#include "procutil.h"

static char *locknames [ROUTE_MAX] = {
  [ROUTE_NONE] = "none",
  [ROUTE_MAIN] = "main",
  [ROUTE_PLAYERUI] = "playerui",
  [ROUTE_CONFIGUI] = "configui",
  [ROUTE_MANAGEUI] = "manageui",
  [ROUTE_PLAYER] = "player",
  [ROUTE_MOBILEMQ] = "mobilemq",
  [ROUTE_REMCTRL] = "remctrl",
  [ROUTE_STARTERUI] = "starterui",
  [ROUTE_DBUPDATE] = "dbupdate",
  [ROUTE_DBTAG] = "dbtag",
  [ROUTE_MARQUEE] = "marquee",
  [ROUTE_HELPERUI] = "helperui",
  [ROUTE_BPM_COUNTER] = "bpmcounter",
};

static int    lockAcquirePid (char *fn, pid_t pid, int flags);
static int    lockReleasePid (char *fn, pid_t pid, int flags);
static pid_t  getPidFromFile (char *fn);

inline char *
lockName (bdjmsgroute_t route)
{
  return locknames [route];
}

/* returns PID if the process exists */
/* returns 0 if no lock exists or the lock belongs to this process, */
/* or if no process exists for this lock */
pid_t
lockExists (char *fn, int flags)
{
  procutil_t process;
  char      tfn [MAXPATHLEN];
  pid_t     fpid = 0;

  pathbldMakePath (tfn, sizeof (tfn), fn, BDJ4_LOCK_EXT,
      flags | PATHBLD_MP_TMPDIR);
  fpid = getPidFromFile (tfn);
  process.pid = fpid;
  process.hasHandle = false;
  if (fpid == -1 || procutilExists (&process) != 0) {
    fileopDelete (tfn);
    fpid = 0;
  }
  if ((flags & LOCK_TEST_SKIP_SELF) != LOCK_TEST_SKIP_SELF) {
    if (fpid == getpid ()) {
      fpid = 0;
    }
  }
  return fpid;
}

int
lockAcquire (char *fn, int flags)
{
  int rc = lockAcquirePid (fn, getpid(), flags);
  return rc;
}

int
lockRelease (char *fn, int flags)
{
  return lockReleasePid (fn, getpid(), flags);
}

/* internal routines */

static int
lockAcquirePid (char *fn, pid_t pid, int flags)
{
  int       fd;
  size_t    len;
  ssize_t   rwlen;
  int       rc;
  int       count;
  char      pidstr [16];
  char      tfn [MAXPATHLEN];
  procutil_t process;

  if ((flags & LOCK_TEST_OTHER_PID) == LOCK_TEST_OTHER_PID) {
    pid = 5;
  }

  if ((flags & PATHBLD_LOCK_FFN) == PATHBLD_LOCK_FFN) {
    strlcpy (tfn, fn, sizeof (tfn));
  } else {
    pathbldMakePath (tfn, sizeof (tfn), fn, BDJ4_LOCK_EXT,
        flags | PATHBLD_MP_TMPDIR);
  }

  fd = open (tfn, O_CREAT | O_EXCL | O_RDWR, 0600);
  count = 0;
  while (fd < 0 && count < 30) {
    /* check for detached lock file */
    pid_t fpid = getPidFromFile (tfn);

    if ((flags & LOCK_TEST_SKIP_SELF) != LOCK_TEST_SKIP_SELF) {
      if (fpid == pid) {
        /* our own lock, this is ok */
        return 0;
      }
    }
    if (fpid > 0) {
      process.pid = fpid;
      process.hasHandle = false;
      rc = procutilExists (&process);
      if (rc < 0) {
        /* process does not exist */
        fileopDelete (tfn);
      }
    }
    mssleep (20);
    ++count;
    fd = open (tfn, O_CREAT | O_EXCL | O_RDWR, 0600);
  }

  if (fd >= 0) {
    snprintf (pidstr, sizeof (pidstr), "%zd", (size_t) pid);
    len = strnlen (pidstr, sizeof (pidstr));
    rwlen = write (fd, pidstr, len);
    assert (rwlen == (ssize_t) len);
    close (fd);
  }
  return fd;
}

static int
lockReleasePid (char *fn, pid_t pid, int flags)
{
  char      tfn [MAXPATHLEN];
  int       rc;
  pid_t     fpid;

  if ((flags & LOCK_TEST_OTHER_PID) == LOCK_TEST_OTHER_PID) {
    pid = 5;
  }

  if ((flags & PATHBLD_LOCK_FFN) == PATHBLD_LOCK_FFN) {
    strlcpy (tfn, fn, sizeof (tfn));
  } else {
    pathbldMakePath (tfn, sizeof (tfn), fn, BDJ4_LOCK_EXT,
        flags | PATHBLD_MP_TMPDIR);
  }

  rc = -1;
  fpid = getPidFromFile (tfn);
  if (fpid == pid) {
    fileopDelete (tfn);
    rc = 0;
  }
  return rc;
}

static pid_t
getPidFromFile (char *fn)
{
  FILE      *fh;
  size_t    temp;

  pid_t pid = -1;
  fh = fopen (fn, "r");
  if (fh != NULL) {
    int rc = fscanf (fh, "%zd", &temp);
    pid = (pid_t) temp;
    if (rc != 1) {
      pid = -1;
    }
    fclose (fh);
  }
  return pid;
}
