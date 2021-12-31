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

int
main (int argc, char *argv[])
{
  sysvarsInit ();

/* ### FIX need to get the executable's directory, and change dir to one up,
   if we're not already there */

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
#if _lib_SetDllDirectory
    char *    buff;
    char *    tbuff;
    size_t    sz = 4096;

    buff = malloc (sz);
    assert (buff != NULL);
    tbuff = malloc (sz);
    assert (tbuff != NULL);

    if (getcwd (tbuff, sz) == NULL) {
      fprintf (stderr, "main: getcwd: %d %s\n", errno, strerror (errno));
    }
    snprintf (buff, sz, "%s", tbuff);
    snprintf (tbuff, "%s\\bin", buff);  /* our package */

    /* cannot get the .dll working with the PATH environment variable */
    SetDllDirectory (tbuff);
    SetDllDirectory ("C:\\Program Files\\VideoLAN\\VLC");
    snprintf (tbuff, "%s\\plocal\\bin", buff); /* other packages */
    SetDllDirectory (tbuff);

    free (buff);
    free (tbuff);
#endif
  }

  /* launch the program */

  execv ("bin/bdj4main", argv);
  return 0;
}
