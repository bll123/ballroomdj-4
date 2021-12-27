#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#if _hdr_unistd
# include <unistd.h>
#endif

#include "sysvars.h"
#include "bdjstring.h"
#include "portability.h"

int
main (int argc, char *argv[])
{
  sysvarsInit ();

  if (isMacOS()) {
    char  npath [MAXPATHLEN];

    char  *path = getenv ("PATH");
    strlcpy (npath, "PATH=", MAXPATHLEN);
    strlcat (npath, path, MAXPATHLEN);
    strlcat (npath, ":/opt/local/bin", MAXPATHLEN);
    putenv (npath);
    putenv ("DYLD_FALLBACK_LIBRARY_PATH="
        "/Applications/VLC.app/Contents/MacOS/lib/");
  }

  if (isWindows()) {
    char  npath [MAXPATHLEN];
    char  buff [MAXPATHLEN];
    char  cwd [MAXPATHLEN];
    char  *path;

    path = getenv ("PATH");
    strlcpy (npath, "PATH=", MAXPATHLEN);
    strlcat (npath, path, MAXPATHLEN);
    /* ### FIX need to know correct drive letter/path */
    snprintf (buff, MAXPATHLEN, "C:\\Program Files\\VideoLAN\\VLC");
    strlcat (npath, ";", MAXPATHLEN);
    strlcat (npath, buff, MAXPATHLEN);

    path = getenv ("PATH");
    strlcpy (npath, "PATH=", MAXPATHLEN);
    strlcat (npath, path, MAXPATHLEN);
    if (getcwd (cwd, MAXPATHLEN) == NULL) {
      fprintf (stderr, "main: getcwd: %d %s\n", errno, strerror (errno));
    }
    snprintf (buff, MAXPATHLEN, ";%s", cwd);
    strlcat (npath, buff, MAXPATHLEN);
    /* ### FIX Need to know what drive letter we are on */
    snprintf (buff, MAXPATHLEN, ";C:%s\\..\\plocal\\lib", cwd);
    strlcat (npath, buff, MAXPATHLEN);
    putenv (npath);
  }

  /* launch the program */

  execv ("bin/bdj4main", argv);
  return 0;
}
