/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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
  bool          iswin = false;


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
    iswin = true;
    osGetEnv ("USERPROFILE", home, sizeof (home));
  }

  if (! *home) {
    return mdstrdup ("");
  }

  /* Linux, old MacOS, recent windows: $HOME/BallroomDJ */
  strlcpy (tbuff, home, MAXPATHLEN);
  if (iswin) {
    strlcat (tbuff, "\\", MAXPATHLEN);
  } else {
    strlcat (tbuff, "/", MAXPATHLEN);
  }
  strlcat (tbuff, "BallroomDJ", MAXPATHLEN);

  if (locationcheck (tbuff)) {
    return mdstrdup (tbuff);
  }

  /* windows: %USERPROFILE%/Desktop/BallroomDJ */
  strlcpy (tbuff, home, MAXPATHLEN);
  strlcat (tbuff, "/", MAXPATHLEN);
  strlcat (tbuff, "Desktop", MAXPATHLEN);
  strlcat (tbuff, "/", MAXPATHLEN);
  strlcat (tbuff, "BallroomDJ", MAXPATHLEN);

  if (locationcheck (tbuff)) {
    return mdstrdup (tbuff);
  }

  /* macos $HOME/Library/Application Support/BallroomDJ */
  /* this is not the main install dir, but the data directory */
  strlcpy (tbuff, home, MAXPATHLEN);
  strlcat (tbuff, "/", MAXPATHLEN);
  strlcat (tbuff, "Library", MAXPATHLEN);
  strlcat (tbuff, "/", MAXPATHLEN);
  strlcat (tbuff, "Application Support", MAXPATHLEN);
  strlcat (tbuff, "/", MAXPATHLEN);
  strlcat (tbuff, "BallroomDJ", MAXPATHLEN);

  if (locationcheck (tbuff)) {
    return mdstrdup (tbuff);
  }

  /* my personal location */
  strlcpy (tbuff, home, MAXPATHLEN);
  strlcat (tbuff, "/", MAXPATHLEN);
  strlcat (tbuff, "music-local", MAXPATHLEN);

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
