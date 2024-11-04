/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "fileop.h"
#include "locatebdj3.h"
#include "mdebug.h"
#include "osenv.h"

char *
locatebdj3 (void)
{
  char          *loc;
  char          home [MAXPATHLEN];
  char          tbuff [MAXPATHLEN];
  char          *tp = tbuff;
  char          *tend = tbuff + sizeof (tbuff);


  /* make it possible to specify a location via the environment */
  loc = getenv ("BDJ3_LOCATION");
  if (loc != NULL) {
    if (locationcheck (loc)) {
      return mdstrdup (loc);
    }
  }

  osGetEnv ("HOME", home, sizeof (home));
  if (! *home) {
    /* probably a windows machine */
    osGetEnv ("USERPROFILE", home, sizeof (home));
  }

  if (! *home) {
    return mdstrdup ("");
  }

  /* Linux, old MacOS, recent windows: $HOME/BallroomDJ */
  tp = stpecpy (tp, tend, home);
  tp = stpecpy (tp, tend, "/");
  tp = stpecpy (tp, tend, "BallroomDJ");

  if (locationcheck (tbuff)) {
    return mdstrdup (tbuff);
  }

  /* windows: %USERPROFILE%/Desktop/BallroomDJ */
  *tbuff = '\0';
  tp = tbuff;
  tp = stpecpy (tp, tend, home);
  tp = stpecpy (tp, tend, "/");
  tp = stpecpy (tp, tend, "Desktop");
  tp = stpecpy (tp, tend, "/");
  tp = stpecpy (tp, tend, "BallroomDJ");

  if (locationcheck (tbuff)) {
    return mdstrdup (tbuff);
  }

  /* macos $HOME/Library/Application Support/BallroomDJ */
  /* this is not the main install dir, but the data directory */
  *tbuff = '\0';
  tp = tbuff;
  tp = stpecpy (tp, tend, home);
  tp = stpecpy (tp, tend, "/Library/Application Support/");
  tp = stpecpy (tp, tend, "BallroomDJ");

  if (locationcheck (tbuff)) {
    return mdstrdup (tbuff);
  }

  /* my personal location */
  *tbuff = '\0';
  tp = tbuff;
  tp = stpecpy (tp, tend, home);
  tp = stpecpy (tp, tend, "/");
  tp = stpecpy (tp, tend, "music-local");

  if (locationcheck (tbuff)) {
    return mdstrdup (tbuff);
  }

  return mdstrdup ("");
}

bool
locationcheck (const char *dir)
{
  if (dir == NULL) {
    return false;
  }

  if (fileopIsDirectory (dir)) {
    if (locatedb (dir)) {
      return true;
    }
  }

  return false;
}


bool
locatedb (const char *dir)
{
  char  tdir [MAXPATHLEN];
  char  tbuff [MAXPATHLEN];
  bool  rc = false;


  snprintf (tdir, sizeof (tdir), "%s/data", dir);
  if (! fileopIsDirectory (tdir)) {
    return rc;
  }

  snprintf (tbuff, sizeof (tbuff), "%s/musicdb.txt", tdir);
  if (fileopFileExists (tbuff)) {
    rc = true;
  }

  if (! rc) {
    snprintf (tbuff, sizeof (tbuff), "%s/masterlist.txt", tdir);
    if (fileopFileExists (tbuff)) {
      rc = true;
    }
  }

  return rc;
}
