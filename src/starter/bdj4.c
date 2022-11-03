#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>

#if BDJ4_GUI_LAUNCHER && BDJ4_USE_GTK
# include <gtk/gtk.h>
#endif

#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdj4.h"
#include "bdjstring.h"
#include "fileop.h"
#include "osprocess.h"
#include "osutils.h"
#include "pathbld.h"
#include "pathutil.h"
#include "sysvars.h"

enum {
  BDJ4_LAUNCHER_MAX_ARGS = 30,
};

int
main (int argc, char * argv[])
{
  char      buff [MAXPATHLEN];
  int       validargs = 0;
  int       c = 0;
  int       option_index = 0;
  char      *prog;
  bool      debugself = false;
  bool      nodetach = false;
  bool      wait = false;
  bool      forcenodetach = false;
  bool      forcewait = false;
  bool      isinstaller = false;
  bool      usemsys = false;
  int       flags;
  const char *targv [BDJ4_LAUNCHER_MAX_ARGS];
  int       targc;
  bool      havetheme = false;
  FILE      *fh = NULL;
  int       rc;

  static struct option bdj_options [] = {
    { "bdj4altsetup",   no_argument,        NULL,   20 },
    { "bdj4bpmcounter", no_argument,        NULL,   19 },
    { "bdj4configui",   no_argument,        NULL,   3 },
    { "bdj4dbupdate",   no_argument,        NULL,   15 },
    { "bdj4helperui",   no_argument,        NULL,   18 },
    { "bdj4info",       no_argument,        NULL,   13 },
    { "bdj4installer",  no_argument,        NULL,   12 },
    { "bdj4main",       no_argument,        NULL,   5 },
    { "bdj4manageui",   no_argument,        NULL,   6 },
    { "bdj4marquee",    no_argument,        NULL,   7 },
    { "bdj4mobilemq",   no_argument,        NULL,   8 },
    { "bdj4player",     no_argument,        NULL,   9 },
    { "bdj4playerui",   no_argument,        NULL,   10 },
    { "bdj4remctrl",    no_argument,        NULL,   11 },
    { "bdj4starterui",  no_argument,        NULL,   14 },
    { "bdj4tags",       no_argument,        NULL,   17 },
    { "bdj4updater",    no_argument,        NULL,   16 },
    { "check_all",      no_argument,        NULL,   1 },
    { "tdbcompare",     no_argument,        NULL,   23 },
    { "tdbdump",        no_argument,        NULL,   24 },
    { "testsuite",      no_argument,        NULL,   22 },
    { "tmusicsetup",    no_argument,        NULL,   21 },
    /* bdj4updater */
    { "newinstall",     no_argument,        NULL,   0 },
    { "converted",      no_argument,        NULL,   0 },
    { "musicdir",       required_argument,  NULL,   0 },
    /* used by installer */
    { "unpackdir",      required_argument,  NULL,   'u' },
    { "reinstall",      no_argument,        NULL,   'r' },
    /* standard stuff */
    { "profile",        required_argument,  NULL,   'p' },
    { "debug",          required_argument,  NULL,   'd' },
    { "theme",          required_argument,  NULL,   't' },
    { "ignorelock",     no_argument,        NULL,   0 },
    { "startlog",       no_argument,        NULL,   0 },
    { "logstderr",      no_argument,        NULL,   0 },
    { "nostart",        no_argument,        NULL,   0 },
    /* this process */
    { "debugself",      no_argument,        NULL,   'D' },
    { "nodetach",       no_argument,        NULL,   'N' },
    { "wait",           no_argument,        NULL,   'w' },
    { "msys",           no_argument,        NULL,   'M' },
    /* dbupdate options */
    { "rebuild",        no_argument,        NULL,   0 },
    { "checknew",       no_argument,        NULL,   0 },
    { "progress",       no_argument,        NULL,   0 },
    { "updfromtags",    no_argument,        NULL,   0 },
    { "writetags",      no_argument,        NULL,   0 },
    { "dbtopdir",       required_argument,  NULL,   0 },
    /* bdjtags */
    { "rawdata",        no_argument,        NULL,   0 },
    { "bdj3tags",       no_argument,        NULL,   0 },
    { "cleantags",      no_argument,        NULL,   0 },
    /* test suite options */
    { "runsection",     required_argument,  NULL,   0 },
    { "runtest",        required_argument,  NULL,   0 },
    { "starttest",      required_argument,  NULL,   0 },
    /* general options */
    { "cli",            no_argument,        NULL,   'c' },
    { "verbose",        no_argument,        NULL,   0 },
    { "quiet",          no_argument,        NULL,   0 },
    { "outfile",        required_argument,  NULL,   0 },
    { "infile",         required_argument,  NULL,   0 },
    { NULL,             0,                  NULL,   0 }
  };

#if BDJ4_GUI_LAUNCHER && BDJ4_USE_GTK
  /* for macos; turns the launcher into a gui program, then the icon */
  /* shows up in the dock */
  gtk_init (&argc, NULL);
#endif

  prog = "bdj4starterui";  // default

  sysvarsInit (argv [0]);

#if BDJ4_USE_GTK
  if (getenv ("GTK_THEME") != NULL) {
    havetheme = true;
  }
#endif

  while ((c = getopt_long_only (argc, argv, "p:d:t:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 1: {
        prog = "check_all";
        ++validargs;
        nodetach = true;
        wait = true;
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
        wait = true;
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
      case 16: {
        prog = "bdj4updater";
        nodetach = true;
        wait = true;
        ++validargs;
        break;
      }
      case 17: {
        prog = "bdj4tags";
        nodetach = true;
        wait = true;
        ++validargs;
        break;
      }
      case 18: {
        prog = "bdj4helperui";
        ++validargs;
        break;
      }
      case 19: {
        prog = "bdj4bpmcounter";
        ++validargs;
        break;
      }
      case 20: {
        prog = "bdj4altsetup";
        ++validargs;
        break;
      }
      case 21: {
        prog = "tmusicsetup";
        nodetach = true;
        wait = true;
        ++validargs;
        break;
      }
      case 22: {
        prog = "testsuite";
        nodetach = true;
        wait = true;
        ++validargs;
        break;
      }
      case 23: {
        prog = "tdbcompare";
        nodetach = true;
        wait = true;
        ++validargs;
        break;
      }
      case 24: {
        prog = "tdbdump";
        nodetach = true;
        wait = true;
        ++validargs;
        break;
      }
      case 'c': {
        forcenodetach = true;
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
      case 'w': {
        forcewait = true;
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
#if BDJ4_USE_GTK
        osSetEnv ("GTK_THEME", optarg);
#endif
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

#if BDJ4_USE_GTK
  osSetEnv ("GTK_CSD", "0");
#endif
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

#if BDJ4_USE_GTK
    osSetEnv ("PANGOCAIRO_BACKEND", "fc");
#endif

    free (pbuff);
    free (tbuff);
    free (path);
  }

  if (! havetheme) {
    pathbldMakePath (buff, sizeof (buff),
        "theme", BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
    if (fileopFileExists (buff)) {
      fh = fopen (buff, "r");
      (void) ! fgets (buff, sizeof (buff), fh);
      fclose (fh);
      stringTrim (buff);
#if BDJ4_USE_GTK
      osSetEnv ("GTK_THEME", buff);
#endif
      havetheme = true;
    }
  }

  if (! havetheme && isWindows ()) {
#if BDJ4_USE_GTK
    osSetEnv ("GTK_THEME", "Windows-10-Dark");
#endif
  }
  if (! havetheme && isMacOS ()) {
#if BDJ4_USE_GTK
    osSetEnv ("GTK_THEME", "macOS-Mojave-dark");
#endif
  }

  /* launch the program */

  if (debugself) {
#if BDJ4_USE_GTK
    fprintf (stderr, "GTK_THEME=%s\n", getenv ("GTK_THEME"));
    fprintf (stderr, "GTK_CSD=%s\n", getenv ("GTK_CSD"));
    fprintf (stderr, "PANGOCAIRO_BACKEND=%s\n", getenv ("PANGOCAIRO_BACKEND"));
#endif
    fprintf (stderr, "LC_ALL=%s\n", getenv ("LC_ALL"));
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

  pathbldMakePath (buff, sizeof (buff),
      prog, sysvarsGetStr (SV_OS_EXEC_EXT), PATHBLD_MP_EXECDIR);
  /* this is necessary on mac os, as otherwise it will use the path     */
  /* from the start of this launcher, and the executable path can not   */
  /* be determined, as we've done a chdir().                            */
  targv [0] = buff;
  if (debugself) {
    fprintf (stderr, "cmd: %s\n", buff);
    for (int i = 1; i < targc; ++i) {
      fprintf (stderr, "  arg: %s\n", targv [i]);
    }
  }

  flags = OS_PROC_NONE;
  if (! forcenodetach && ! nodetach) {
    flags |= OS_PROC_DETACH;
  }
  if (forcewait || wait) {
    flags |= OS_PROC_WAIT;
    flags &= ~OS_PROC_DETACH;
  }
  rc = osProcessStart (targv, flags, NULL, NULL);
  if (forcewait || wait) {
    return rc;
  }
  return 0;
}

