#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>

#if _hdr_windows
# include <windows.h>
#endif

#include "bdjstring.h"
#include "pathbld.h"
#include "fileop.h"
#include "pathutil.h"
#include "portability.h"
#include "sysvars.h"

#if _lib__execv
# define execv _execv
#endif

int
main (int argc, char *argv[])
{
  char      buff [MAXPATHLEN];
  int       validargs = 0;
  int       c = 0;
  int       option_index = 0;
  char      *prog;
  char      *extension;

  static struct option bdj_options [] = {
    { "check_all",  no_argument,        NULL,   1 },
    { "main",       no_argument,        NULL,   2 },
    { "playerui",   no_argument,        NULL,   3 },
    { "configui",   no_argument,        NULL,   4 },
    { "manageui",   no_argument,        NULL,   5 },
    { "player",     no_argument,        NULL,   6 },
    { "cli",        no_argument,        NULL,   7 },
    { "mobilemq",   no_argument,        NULL,   8 },
    { "remctrl",    no_argument,        NULL,   9 },
    { "marquee",    no_argument,        NULL,   10 },
    { "profile",    required_argument,  NULL,   'p' },
    { "debug",      required_argument,  NULL,   'd' },
    { "theme",      required_argument,  NULL,   't' },
    { NULL,         0,                  NULL,   0 }
  };

  prog = "bdj4main";

  sysvarsInit (argv [0]);

  while ((c = getopt_long_only (argc, argv, "p:d:t:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 1: {
        prog = "check_all";
        ++validargs;
        break;
      }
      case 2: {
        prog = "bdj4main";
        ++validargs;
        break;
      }
      case 3: {
        prog = "bdj4playerui";
        ++validargs;
        break;
      }
      case 4: {
        prog = "bdj4configui";
        ++validargs;
        break;
      }
      case 5: {
        prog = "bdj4manageui";
        ++validargs;
        break;
      }
      case 6: {
        prog = "player";
        ++validargs;
        break;
      }
      case 7: {
        prog = "bdj4cli";
        ++validargs;
        break;
      }
      case 8: {
        prog = "bdj4mobmq";
        ++validargs;
        break;
      }
      case 9: {
        prog = "bdj4remctrl";
        ++validargs;
        break;
      }
      case 10: {
        prog = "marquee";
        ++validargs;
        break;
      }
      case 'd': {
        break;
      }
      case 'p': {
        if (optarg) {
          sysvarSetNum (SVL_BDJIDX, atol (optarg));
        }
        break;
      }
      case 't': {
        snprintf (buff, sizeof (buff), "GTK_THEME=%s", optarg);
        putenv (buff);
        break;
      }
      default: {
        break;
      }
    }
  }

  if (validargs > 1) {
    fprintf (stderr, "Invalid arguments\n");
    exit (1);
  }

  if (chdir (sysvars [SV_BDJ4DIR]) < 0) {
    fprintf (stderr, "Unable to chdir: %s\n", sysvars [SV_BDJ4DIR]);
    exit (1);
  }

  fileopMakeDir ("tmp");

  putenv ("GTK_OVERLAY_SCROLLING=0");

  if (isMacOS()) {
    char      *path = NULL;
    char      *npath = NULL;
    size_t    sz = 4096;


    npath = malloc (sz);
    assert (npath != NULL);

    path = getenv ("PATH");
    strlcpy (npath, "PATH=", sz);
    strlcat (npath, path, sz);
    strlcat (npath, ":/opt/local/bin", sz);
    putenv (npath);

    putenv ("DYLD_FALLBACK_LIBRARY_PATH="
        "/Applications/VLC.app/Contents/MacOS/lib/");
    putenv ("VLC_PLUGIN_PATH=/Applications/VLC.app/Contents/MacOS/plugins");
    free (npath);
  }

  if (isWindows ()) {
    char *    pbuff;
    char *    tbuff;
    char *    path;
    size_t    sz = 4096;

    pbuff = malloc (sz);
    assert (pbuff != NULL);
    tbuff = malloc (sz);
    assert (tbuff != NULL);
    path = malloc (sz);
    assert (path != NULL);

    pathToWinPath (sysvars [SV_BDJ4EXECDIR], pbuff, sz);
    strlcpy (path, "PATH=", sz);
    strlcat (path, getenv ("PATH"), sz);
    strlcat (path, ";", sz);
    strlcat (path, pbuff, sz);

    strlcat (path, ";", sz);
    snprintf (pbuff, sz, "%s\\..\\plocal\\bin", sysvars [SV_BDJ4EXECDIR]);
    pathRealPath (pbuff, tbuff);
    strlcat (path, tbuff, sz);

    strlcat (path, ";", sz);
    /* do not use double quotes w/putenv */
    strlcat (path, "C:\\Program Files\\VideoLAN\\VLC", sz);
    putenv (path);

    free (pbuff);
    free (tbuff);
    free (path);
  }

  /* launch the program */

  extension = "";
  if (isWindows()) {
    extension = ".exe";
  }
  pathbldMakePath (buff, sizeof (buff), "",
      prog, extension, PATHBLD_MP_EXECDIR);

  /* this is necessary on mac os, as otherwise it will use the path     */
  /* from the start of this launcher, and the executable path can not   */
  /* be determined, as we've done a chdir().                            */
  argv [0] = buff;
  if (execv (buff, argv) < 0) {
    fprintf (stderr, "Unable to start %s %d %s\n", buff, errno, strerror (errno));
  }
  return 0;
}
