/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "bdj4.h"
#include "bdjmsg.h"
#include "bdjstring.h"
#include "dirop.h"
#include "fileop.h"
#include "lock.h"
#include "mdebug.h"
#include "pathbld.h"
#include "procutil.h"
#include "sysvars.h"
#include "tmutil.h"

static char *locknames [ROUTE_MAX] = {
  [ROUTE_BPM_COUNTER] = "bpmcounter",
  [ROUTE_CONFIGUI] = "configui",
  [ROUTE_DBUPDATE] = "dbupdate",
  [ROUTE_HELPERUI] = "helperui",
  [ROUTE_MAIN] = "main",
  [ROUTE_MANAGEUI] = "manageui",
  [ROUTE_MARQUEE] = "marquee",
  [ROUTE_MOBILEMQ] = "mobilemq",
  [ROUTE_NONE] = "none",
  [ROUTE_PLAYER] = "player",
  [ROUTE_PLAYERUI] = "playerui",
  [ROUTE_REMCTRL] = "remctrl",
  [ROUTE_STARTERUI] = "starterui",
  [ROUTE_TEST_SUITE] = "testsuite",
  [ROUTE_ALTINST] = "altinst",
  [ROUTE_SERVER] = "server",
};

static void   lockCheckLockDir (void);
static int    lockAcquirePid (char *fn, pid_t pid, int flags);
static int    lockReleasePid (char *fn, pid_t pid, int flags);
static pid_t  getPidFromFile (char *fn);

static bool   lockdirchecked = false;

char *
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

  lockCheckLockDir ();
  pathbldMakePath (tfn, sizeof (tfn), fn, BDJ4_LOCK_EXT,
      flags | PATHBLD_MP_DIR_LOCK);
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

static void
lockCheckLockDir (void)
{
  char  tdir [MAXPATHLEN];

  if (lockdirchecked) {
    return;
  }

  pathbldMakePath (tdir, sizeof (tdir), "", "", PATHBLD_MP_DIR_LOCK);
  if (! fileopIsDirectory (tdir)) {
    diropMakeDir (tdir);
  }
  lockdirchecked = true;
}

static int
lockAcquirePid (char *fn, pid_t pid, int flags)
{
  int       fd;
  size_t    len;
  int       rc;
  int       count;
  char      pidstr [16];
  char      tfn [MAXPATHLEN];
  procutil_t process;

  lockCheckLockDir ();

  if ((flags & LOCK_TEST_OTHER_PID) == LOCK_TEST_OTHER_PID) {
    pid = 5;
  }

  if ((flags & PATHBLD_LOCK_FFN) == PATHBLD_LOCK_FFN) {
    stpecpy (tfn, tfn + sizeof (tfn), fn);
  } else {
    pathbldMakePath (tfn, sizeof (tfn), fn, BDJ4_LOCK_EXT,
        flags | PATHBLD_MP_DIR_LOCK);
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
    mdextopen (fd);
    snprintf (pidstr, sizeof (pidstr), "%" PRId64, (int64_t) pid);
    len = strnlen (pidstr, sizeof (pidstr));
    (void) ! write (fd, pidstr, len);
    mdextclose (fd);
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

  lockCheckLockDir ();

  if ((flags & LOCK_TEST_OTHER_PID) == LOCK_TEST_OTHER_PID) {
    pid = 5;
  }

  if ((flags & PATHBLD_LOCK_FFN) == PATHBLD_LOCK_FFN) {
    stpecpy (tfn, tfn + sizeof (tfn), fn);
  } else {
    pathbldMakePath (tfn, sizeof (tfn), fn, BDJ4_LOCK_EXT,
        flags | PATHBLD_MP_DIR_LOCK);
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
  int64_t   temp;

  pid_t pid = -1;
  fh = fileopOpen (fn, "r");
  if (fh != NULL) {
    int rc = fscanf (fh, "%" PRId64, &temp);
    pid = (pid_t) temp;
    if (rc != 1) {
      pid = -1;
    }
    mdextfclose (fh);
    fclose (fh);
  }
  return pid;
}
