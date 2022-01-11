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

#include "sysvars.h"
#include "process.h"
#include "tmutil.h"
#include "lock.h"
#include "fileop.h"
#include "datautil.h"
#include "portability.h"

static pid_t   getPidFromFile (char *);

int
lockAcquire (char *fn, datautil_mp_t flags)
{
  int rc = lockAcquirePid (fn, getpid(), flags);
  return rc;
}

int
lockAcquirePid (char *fn, pid_t pid, datautil_mp_t flags)
{
  int       fd;
  size_t    len;
  ssize_t   rwlen;
  int       rc;
  int       count;
  char      pidstr [16];
  char      tfn [MAXPATHLEN];


  datautilMakePath (tfn, sizeof (tfn), "", fn, ".lck",
      flags | DATAUTIL_MP_TMPPREFIX);

  fd = open (tfn, O_CREAT | O_EXCL | O_RDWR, 0600);
  count = 0;
  while (fd < 0 && count < 30) {
    /* check for detached lock file */
    pid_t fpid = getPidFromFile (tfn);
    if (fpid > 0) {
      rc = processExists (fpid);
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


int
lockRelease (char *fn, datautil_mp_t flags)
{
  return lockReleasePid (fn, getpid(), flags);
}

int
lockReleasePid (char *fn, pid_t pid, datautil_mp_t flags)
{
  char      tfn [MAXPATHLEN];

  datautilMakePath (tfn, sizeof (tfn), "", fn, ".lck",
      flags | DATAUTIL_MP_TMPPREFIX);
  int rc = -1;
  pid_t fpid = getPidFromFile (tfn);
  if (fpid == pid) {
    fileopDelete (tfn);
    rc = 0;
  }
  return rc;
}

/* internal routines */

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
