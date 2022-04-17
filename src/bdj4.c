#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>

#if BDJ4_GUI_LAUNCHER
# include <gtk/gtk.h>
#endif

#if _hdr_winsock2
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmissing-declarations"
# include <winsock2.h>
# pragma clang diagnostic pop
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdj4.h"
#include "bdjstring.h"
#include "fileop.h"
#include "osutils.h"
#include "pathbld.h"
#include "pathutil.h"
#include "sysvars.h"

#define BDJ4_LAUNCHER_MAX_ARGS 30

int
main (int argc, char * argv[])
{
  char      buff [MAXPATHLEN];
  int       validargs = 0;
  int       c = 0;
  int       option_index = 0;
  char      *prog;
  char      *extension;
  bool      debugself = false;
  bool      nodetach = false;
  bool      forcenodetach = false;
  bool      isinstaller = false;
  bool      usemsys = false;
  int       flags;
  char      *targv [BDJ4_LAUNCHER_MAX_ARGS];
  int       targc;
  bool      havetheme = false;
  FILE      *fh = NULL;

  static struct option bdj_options [] = {
    { "check_all",      no_argument,        NULL,   1 },
    { "bdj4cli",        no_argument,        NULL,   2 },
    { "cli",            no_argument,        NULL,   2 },
    { "bdj4configui",   no_argument,        NULL,   3 },
    { "bdj4main",       no_argument,        NULL,   5 },
    { "main",           no_argument,        NULL,   5 },
    { "bdj4manageui",   no_argument,        NULL,   6 },
    { "bdj4marquee",    no_argument,        NULL,   7 },
    { "bdj4mobilemq",   no_argument,        NULL,   8 },
    { "bdj4player",     no_argument,        NULL,   9 },
    { "bdj4playerui",   no_argument,        NULL,   10 },
    { "playerui",       no_argument,        NULL,   10 },
    { "bdj4remctrl",    no_argument,        NULL,   11 },
    { "bdj4installer",  no_argument,        NULL,   12 },
    { "installer",      no_argument,        NULL,   12 },
    { "bdj4info",       no_argument,        NULL,   13 },
    { "bdj4starterui",  no_argument,        NULL,   14 },
    { "bdj4dbupdate",   no_argument,        NULL,   15 },
    /* cli */
    { "forcestop",      no_argument,        NULL,   0 },
    /* used by installer */
    { "unpackdir",      required_argument,  NULL,   'u' },
    { "reinstall",      no_argument,        NULL,   'r' },
    { "guidisabled",no_argument,            NULL,   'g' },
    /* standard stuff */
    { "profile",        required_argument,  NULL,   'p' },
    { "debug",          required_argument,  NULL,   'd' },
    { "theme",          required_argument,  NULL,   't' },
    { "ignorelock",     no_argument,        NULL,   0 },
    { "startlog",       no_argument,        NULL,   0 },
    { "nostart",        no_argument,        NULL,   0 },
    /* this process */
    { "debugself",      no_argument,        NULL,   'D' },
    { "nodetach",       no_argument,        NULL,   'N' },
    { "msys",           no_argument,        NULL,   'M' },
    /* dbupdate options */
    { "rebuild",        no_argument,        NULL,   0 },
    { "checknew",       no_argument,        NULL,   0 },
    { NULL,             0,                  NULL,   0 }
  };

#if BDJ4_GUI_LAUNCHER
  /* for macos; turns the launcher into a gui program, then the icon */
  /* shows up in the dock */
  gtk_init (&argc, NULL);
#endif

  prog = "bdj4starterui";  // default

  sysvarsInit (argv [0]);

  if (getenv ("GTK_THEME") != NULL) {
    havetheme = true;
  }

  while ((c = getopt_long_only (argc, argv, "p:d:t:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 1: {
        prog = "check_all";
        ++validargs;
        nodetach = true;
        break;
      }
      case 2: {
        prog = "bdj4cli";
        ++validargs;
        nodetach = true;
        break;
      }
      case 3: {
        prog = "bdj4configui";
        ++validargs;
        break;
      }
      case 4: {
        prog = "bdj4converter";
        ++validargs;
        break;
      }
      case 5: {
        prog = "bdj4main";
        ++validargs;
        break;
      }
      case 6: {
        prog = "bdj4manageui";
        ++validargs;
        break;
      }
      case 7: {
        prog = "bdj4marquee";
        ++validargs;
        break;
      }
      case 8: {
        prog = "bdj4mobmq";
        ++validargs;
        break;
      }
      case 9: {
        prog = "player";
        ++validargs;
        break;
      }
      case 10: {
        prog = "bdj4playerui";
        ++validargs;
        break;
      }
      case 11: {
        prog = "bdj4remctrl";
        ++validargs;
        break;
      }
      case 12: {
        prog = "bdj4installer";
        nodetach = true;
        isinstaller = true;
        ++validargs;
        break;
      }
      case 13: {
        prog = "bdj4info";
        nodetach = true;
        ++validargs;
        break;
      }
      case 14: {
        prog = "bdj4starterui";
        ++validargs;
        break;
      }
      case 15: {
        prog = "bdj4dbupdate";
        ++validargs;
        break;
      }
      case 'd': {
        break;
      }
      case 'D': {
        debugself = true;
        break;
      }
      case 'N': {
        forcenodetach = true;
        break;
      }
      case 'M': {
        usemsys = true;
        break;
      }
      case 'p': {
        if (optarg) {
          sysvarsSetNum (SVL_BDJIDX, atol (optarg));
        }
        break;
      }
      case 't': {
        osSetEnv ("GTK_THEME", optarg);
        havetheme = true;
        break;
      }
      case 'r': {
        break;
      }
      case 'u': {
        break;
      }
      case 'g': {
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

  if (isinstaller == false) {
    if (chdir (sysvarsGetStr (SV_BDJ4DATATOPDIR)) < 0) {
      fprintf (stderr, "Unable to set working dir: %s\n", sysvarsGetStr (SV_BDJ4DATATOPDIR));
      exit (1);
    }
  }

  osSetEnv ("GTK_CSD", "0");
  osSetEnv ("PYTHONIOENCODING", "utf-8");

  if (isMacOS ()) {
    char      *path = NULL;
    char      *npath = NULL;
    size_t    sz = 4096;


    npath = malloc (sz);
    assert (npath != NULL);

    path = getenv ("PATH");
    *npath = '\0';
    strlcat (npath, path, sz);
    strlcat (npath, ":/opt/local/bin", sz);
    osSetEnv ("PATH", npath);

    osSetEnv ("DYLD_FALLBACK_LIBRARY_PATH",
        "/Applications/VLC.app/Contents/MacOS/lib/");
    osSetEnv ("VLC_PLUGIN_PATH",
        "/Applications/VLC.app/Contents/MacOS/plugins");
    free (npath);

    osSetEnv ("G_FILENAME_ENCODING", "UTF8-MAC");
  }

  if (isWindows ()) {
    char      * pbuff;
    char      * tbuff;
    char      * tmp;
    char      * path;
    char      * p;
    char      * tokstr;
    size_t    sz = 4096;

    pbuff = malloc (sz);
    assert (pbuff != NULL);
    tbuff = malloc (sz);
    assert (tbuff != NULL);
    tmp = malloc (sz);
    assert (tmp != NULL);
    path = malloc (sz);
    assert (path != NULL);

    strlcpy (tbuff, getenv ("PATH"), sz);
    p = strtok_r (tbuff, ";", &tokstr);
    *path = '\0';
    while (p != NULL) {
      snprintf (tmp, sz, "%s\\libgtk-3-0.dll", p);
      if (usemsys || ! fileopFileExists (tmp)) {
        if (debugself) {
          fprintf (stderr, "path ok: %s\n", p);
        }
        strlcat (path, p, sz);
        strlcat (path, ";", sz);
      } else {
        if (debugself) {
          fprintf (stderr, "found dir with libgtk: %s\n", p);
        }
      }
      p = strtok_r (NULL, ";", &tokstr);
    }

    strlcpy (pbuff, sysvarsGetStr (SV_BDJ4EXECDIR), sz);
    pathWinPath (pbuff, sz);
    strlcat (path, pbuff, sz);

    if (debugself) {
      fprintf (stderr, "execdir: %s\n", pbuff);
      fprintf (stderr, "path: %s\n", path);
    }

    strlcat (path, ";", sz);
    snprintf (pbuff, sz, "%s/../plocal/bin", sysvarsGetStr (SV_BDJ4EXECDIR));
    pathRealPath (tbuff, pbuff, sz);
    strlcat (path, tbuff, sz);

    strlcat (path, ";", sz);
    /* do not use double quotes w/environment var */
    strlcat (path, "C:\\Program Files\\VideoLAN\\VLC", sz);
    osSetEnv ("PATH", path);

    if (debugself) {
      fprintf (stderr, "final PATH=%s\n", getenv ("PATH"));
    }

    osSetEnv ("PANGOCAIRO_BACKEND", "fc");

    free (pbuff);
    free (tbuff);
    free (path);
  }

  if (! havetheme) {
    pathbldMakePath (buff, sizeof (buff), "",
        "theme", ".txt", PATHBLD_MP_NONE);
    if (fileopFileExists (buff)) {
      fh = fopen (buff, "r");
      fgets (buff, sizeof (buff), fh);
      stringTrim (buff);
      fclose (fh);
      osSetEnv ("GTK_THEME", buff);
    }
  }

  /* launch the program */

  if (debugself) {
    fprintf (stderr, "GTK_THEME=%s\n", getenv ("GTK_THEME"));
    fprintf (stderr, "GTK_CSD=%s\n", getenv ("GTK_CSD"));
    fprintf (stderr, "PANGOCAIRO_BACKEND=%s\n", getenv ("PANGOCAIRO_BACKEND"));
  }

  extension = "";
  if (isWindows()) {
    extension = ".exe";
  }

  targc = 0;
  for (int i = 0; i < argc; ++i) {
    if (targc >= BDJ4_LAUNCHER_MAX_ARGS) {
      fprintf (stderr, "too many arguments\n");
      exit (1);
    }
    targv [targc++] = argv [i];
  }
  if (targc >= (BDJ4_LAUNCHER_MAX_ARGS - 2)) {
    fprintf (stderr, "too many arguments\n");
    exit (1);
  }
  targv [targc++] = "--bdj4";
  targv [targc++] = NULL;

  pathbldMakePath (buff, sizeof (buff), "",
      prog, extension, PATHBLD_MP_EXECDIR);
  /* this is necessary on mac os, as otherwise it will use the path     */
  /* from the start of this launcher, and the executable path can not   */
  /* be determined, as we've done a chdir().                            */
  targv [0] = buff;
  if (debugself) {
    fprintf (stderr, "cmd: %s\n", buff);
  }

  flags = OS_PROC_DETACH;
  if (forcenodetach || nodetach) {
    flags = OS_PROC_NONE;
  }
  osProcessStart (targv, flags, NULL, NULL);
  return 0;
}

