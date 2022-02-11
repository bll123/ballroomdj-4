#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "bdjstring.h"
#include "locatebdj3.h"
#include "portability.h"

char *
locatebdj3 (void)
{
  char          *loc;
  char          *home;
  struct stat   statbuf;
  char          tbuff [MAXPATHLEN];
  int           grc;
  int           rc;
  bool          iswin = false;


  /* make it possible to specify a location via the environment */
  loc = getenv ("BDJ3_LOCATION");
  if (loc != NULL) {
    //logMsg (LOG_INSTALL, LOG_IMPORTANT, "from-env: %s", loc);
    if (locationcheck (loc)) {
      return strdup (loc);
    }
  }

  home = getenv ("HOME");
  if (home == NULL) {
    /* probably a windows machine */
    iswin = true;
    home = getenv ("USERPROFILE");
  }
  //logMsg (LOG_INSTALL, LOG_IMPORTANT, "home: %s", loc);

  if (home == NULL) {
    //logMsg (LOG_INSTALL, LOG_IMPORTANT, "err: no home env", loc);
    return "";
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
    return strdup (tbuff);
  }

  /* windows: %USERPROFILE%/Desktop/BallroomDJ */
  strlcpy (tbuff, home, MAXPATHLEN);
  strlcat (tbuff, "/", MAXPATHLEN);
  strlcat (tbuff, "Desktop", MAXPATHLEN);
  strlcat (tbuff, "/", MAXPATHLEN);
  strlcat (tbuff, "BallroomDJ", MAXPATHLEN);

  if (locationcheck (tbuff)) {
    return strdup (tbuff);
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
    return strdup (tbuff);
  }

  /* my personal location */
  strlcpy (tbuff, home, MAXPATHLEN);
  strlcat (tbuff, "/", MAXPATHLEN);
  strlcat (tbuff, "music-local", MAXPATHLEN);

  if (locationcheck (tbuff)) {
    return strdup (tbuff);
  }

  return "";
}

bool
locationcheck (const char *dir)
{
  char          tbuff [MAXPATHLEN];
  struct stat   statbuf;
  int           rc;

  if (dir == NULL) {
    return false;
  }

  strlcpy (tbuff, dir, MAXPATHLEN);

  rc = stat (tbuff, &statbuf);
  //logMsg (LOG_INSTALL, LOG_IMPORTANT, "try: %d %s", rc, tbuff);
  if (rc == 0) {
    if ((statbuf.st_mode & S_IFDIR) == S_IFDIR) {
      if (locatedb (tbuff)) {
        return true;
      }
    }
  }

  return false;
}


bool
locatedb (const char *dir)
{
  char          tbuff [MAXPATHLEN];
  struct stat   statbuf;
  int           rc;

  strlcpy (tbuff, dir, MAXPATHLEN);
  strlcat (tbuff, "/", MAXPATHLEN);
  strlcat (tbuff, "data", MAXPATHLEN);

  rc = stat (tbuff, &statbuf);
  //logMsg (LOG_INSTALL, LOG_IMPORTANT, "try: %d %s", rc, tbuff);
  if (rc != 0) {
    return false;
  }
  if ((statbuf.st_mode & S_IFDIR) != S_IFDIR) {
    return false;
  }

  strlcat (tbuff, "/", MAXPATHLEN);
  strlcat (tbuff, "musicdb.txt", MAXPATHLEN);

  rc = stat (tbuff, &statbuf);
  //logMsg (LOG_INSTALL, LOG_IMPORTANT, "try: %d %s", rc, tbuff);
  if (rc != 0) {
    return false;
  }
  if ((statbuf.st_mode & S_IFREG) != S_IFREG) {
    return false;
  }

  return true;
}
