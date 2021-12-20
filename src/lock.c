#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

#include "lock.h"
#include "utility.h"

static int    getPidFromFile (char *);

int
lockAcquire (char *fn)
{
  int rc = lockAcquirePid (fn, getpid());
  return rc;
}

int
lockAcquirePid (char *fn, pid_t pid)
{
  int       fd;
  size_t    len;
  ssize_t   rwlen;
  int       rc;
  int       count;
  char      pidstr [16];

  fd = open (fn, O_CREAT | O_EXCL | O_RDWR, 0600);
  count = 0;
  while (fd < 0 && count < 30) {
    /* check for detached lock file */
    int fpid = getPidFromFile (fn);
    if (fpid > 0) {
      rc = kill (fpid, 0);
      if (rc < 0) {
        /* process does not exist */
        unlink (fn);
      }
    }
    msleep (20);
    ++count;
    fd = open (fn, O_CREAT | O_EXCL | O_RDWR, 0600);
  }

  if (fd >= 0) {
    sprintf (pidstr, "%d", pid);
    len = strlen (pidstr);
    rwlen = write (fd, pidstr, len);
    assert (rwlen == (ssize_t) len);
    close (fd);
  }
  return fd;
}


int
lockRelease (char *fn)
{
  return lockReleasePid (fn, getpid());
}

int
lockReleasePid (char *fn, pid_t pid)
{
  int rc = -1;
  int fpid = getPidFromFile (fn);
  if (fpid == pid) {
    unlink (fn);
    rc = 0;
  }
  return rc;
}

/* internal routines */

static int
getPidFromFile (char *fn)
{
  FILE      *fh;

  int pid = -1;
  fh = fopen (fn, "r");
  if (fh != NULL) {
    int rc = fscanf (fh, "%d", &pid);
    if (rc != 1) {
      pid = -1;
    }
    fclose (fh);
  }
  return pid;
}
