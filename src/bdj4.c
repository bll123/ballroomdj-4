#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#if _hdr_unistd
# include <unistd.h>
#endif

#include "sysvars.h"
#include "bdjstring.h"
#include "portability.h"
#include "fileutil.h"

int
main (int argc, char *argv[])
{
  char      buff [MAXPATHLEN];

  sysvarsInit (argv [0]);

  chdir (sysvars [SV_BDJ4DIR]);

  if (isMacOS()) {
    char  *   npath;
    size_t    sz = 4096;

    npath = malloc (sz);
    assert (npath != NULL);

    char  *path = getenv ("PATH");
    strlcpy (npath, "PATH=", sz);
    strlcat (npath, path, sz);
    strlcat (npath, ":/opt/local/bin", sz);
    putenv (npath);

    putenv ("DYLD_FALLBACK_LIBRARY_PATH="
        "/Applications/VLC.app/Contents/MacOS/lib/");

    putenv ("VLC_PLUGIN_PATH=/Applications/VLC.app/Contents/MacOS/plugins");
    free (npath);
  }

  if (isWindows()) {
    char *    buff;
    char *    tbuff;
    char *    path;
    size_t    sz = 4096;

    buff = malloc (sz);
    assert (buff != NULL);
    tbuff = malloc (sz);
    assert (tbuff != NULL);
    path = malloc (sz);
    assert (path != NULL);

    fileToWinPath (sysvars [SV_BDJ4EXECDIR], buff, sz);
    strlcpy (path, "PATH=", sz);
    strlcat (path, getenv ("PATH"), sz);
    strlcat (path, ";", sz);
    strlcat (path, buff, sz);

    strlcat (path, ";", sz);
    snprintf (buff, sz, "%s\\..\\plocal\\bin", sysvars [SV_BDJ4EXECDIR]);
    fileRealPath (buff, tbuff);
    strlcat (path, tbuff, sz);

    strlcat (path, ";", sz);
    /* do not use double quotes w/putenv */
    strlcat (path, "C:\\Program Files\\VideoLAN\\VLC", sz);
    putenv (path);

    free (buff);
    free (tbuff);
    free (path);
  }

  /* launch the program */

  snprintf (buff, MAXPATHLEN, "%s/bdj4main", sysvars [SV_BDJ4EXECDIR]);
  if (isWindows()) {
    strlcat (buff, ".exe", MAXPATHLEN);
  }
  execv (buff, argv);
  return 0;
}
