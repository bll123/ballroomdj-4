/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>

#if BDJ4_GUI_LAUNCHER && BDJ4_UI_GTK3
# include <gtk/gtk.h>
#endif

#include "bdj4.h"
#include "bdj4arg.h"
#include "bdjstring.h"
#include "fileop.h"
#include "mdebug.h"
#include "osdirutil.h"
#include "osenv.h"
#include "osprocess.h"
#include "pathbld.h"
#include "pathdisp.h"
#include "pathutil.h"
#include "sysvars.h"

enum {
  BDJ4_LAUNCHER_MAX_ARGS = 40,
};

int
main (int argc, char * argv[])
{
  char        buff [MAXPATHLEN];
  char        origcwd [MAXPATHLEN];
  int         validargs = 0;
  int         c = 0;
  int         option_index = 0;
  char        *prog;
  bool        debugself = false;
  bool        nodetach = false;
  bool        wait = false;
  bool        forcenodetach = false;
  bool        forcewait = false;
  bool        isinstaller = false;
  int         flags;
  const char  *targv [BDJ4_LAUNCHER_MAX_ARGS];
  int         targc;
  bool        havetheme = false;
  bool        havescale = false;
  FILE        *fh = NULL;
  int         rc;
  bdj4arg_t   *bdj4arg;
  const char  *targ;
  const char  *vlctag = "VLC";
  char        *p;
  char        *end;

  static struct option bdj_options [] = {
    { "aesed",          no_argument,        NULL,   1 },
    { "bdj4altinst",    no_argument,        NULL,   2 },
    { "bdj4cleantmp",   no_argument,        NULL,   3 },
    { "bdj4dbupdate",   no_argument,        NULL,   4 },
    { "bdj4info",       no_argument,        NULL,   5 },
    { "bdj4installer",  no_argument,        NULL,   6 },
    { "bdj4podcastupd", no_argument,        NULL,   20 },
    { "bdj4server",     no_argument,        NULL,   7 },
    { "bdj4tags",       no_argument,        NULL,   8 },
    { "bdj4updater",    no_argument,        NULL,   9 },
    { "check_all",      no_argument,        NULL,   10 },
    { "dmkmfromdb",     no_argument,        NULL,   11 },
    { "tdbcompare",     no_argument,        NULL,   12 },
    { "tdbsetval",      no_argument,        NULL,   13 },
    { "testsuite",      no_argument,        NULL,   14 },
    { "tmusicsetup",    no_argument,        NULL,   15 },
    { "ttagdbchk",      no_argument,        NULL,   16 },
    { "uitest",         no_argument,        NULL,   17 },
    { "vlcsinklist",    no_argument,        NULL,   18 },
    { "vlcversion",     no_argument,        NULL,   19 },
    /* used by installer */
    { "bdj3dir",        required_argument,  NULL,   0 },
    { "noclean",        no_argument,        NULL,   0 },
    { "readonly",       no_argument,        NULL,   0 },
    { "testregistration", no_argument,      NULL,   0 },
    { "unpackdir",      required_argument,  NULL,   0 },
    /* used by installer, altinst */
    { "locale",         required_argument,  NULL,   0 },
    { "reinstall",      no_argument,        NULL,   0 },
    { "targetdir",      required_argument,  NULL,   0 },
    { "unattended",     no_argument,        NULL,   0 },
    /* altinst */
    { "name",           required_argument,  NULL,   0 },
    /* starter */
    { "nopodcastupd",   no_argument,        NULL,   0 },
    /* updater */
    { "convert",        no_argument,        NULL,   0 },
    { "newinstall",     no_argument,        NULL,   0 },
    /* standard stuff */
    { "debug",          required_argument,  NULL,   0 },
    { "profile",        required_argument,  NULL,   'p' },
    { "scale",          required_argument,  NULL,   'S' },
    { "theme",          required_argument,  NULL,   'T' },
    /* this process */
    { "debugself",      no_argument,        NULL,   'D' },
    { "nodetach",       no_argument,        NULL,   'N' },
    { "nostart",        no_argument,        NULL,   0 },
    { "pli",            required_argument,  NULL,   'P' },
    { "wait",           no_argument,        NULL,   'w' },
    /* dbupdate options */
    { "checknew",       no_argument,        NULL,   0 },
    { "compact",        no_argument,        NULL,   0 },
    { "musicdir",       required_argument,  NULL,   0 },
    { "rebuild",        no_argument,        NULL,   0 },
    { "reorganize",     no_argument,        NULL,   0 },
    { "updfromtags",    no_argument,        NULL,   0 },
    { "writetags",      no_argument,        NULL,   0 },
    /* bdjtags */
    { "cleantags",      no_argument,        NULL,   0 },
    { "copy",           no_argument,        NULL,   0 },
    /* test suite options */
    { "runsection",     required_argument,  NULL,   0 },
    { "runtest",        required_argument,  NULL,   0 },
    { "starttest",      required_argument,  NULL,   0 },
    /* tmusicsetup */
    { "seconddir",      required_argument,  NULL,   0 },
    { "emptydb",        no_argument,        NULL,   0 },
    { "infile",         required_argument,  NULL,   0 },
    { "keepmusic",      no_argument,        NULL,   0 },
    { "outfile",        required_argument,  NULL,   0 },
    /* tdbcompare */
    { "noloclockchk",   no_argument,        NULL,   0 },
    /* ttagdbchk */
    { "ignoremissing",  no_argument,        NULL,   0 },
    /* general options */
    { "cli",            no_argument,        NULL,   'c' },
    { "progress",       no_argument,        NULL,   0 },
    { "quiet",          no_argument,        NULL,   0 },
    { "verbose",        no_argument,        NULL,   0 },
    { NULL,             0,                  NULL,   0 }
  };

#if BDJ4_MEM_DEBUG
  mdebugInit ("lnch");
#endif

  bdj4arg = bdj4argInit (argc, argv);

#if BDJ4_GUI_LAUNCHER && (BDJ4_UI_GTK3 || BDJ4_UI_GTK4)
  /* for macos; turns the launcher into a gui program, then the icon */
  /* shows up in the dock */
  gtk_init (&argc, NULL);
#endif

  prog = "bdj4starterui";  // default

  targ = bdj4argGet (bdj4arg, 0, argv [0]);
  sysvarsInit (targ, SYSVARS_FLAG_BASIC);
#if BDJ4_UI_GTK3 || BDJ4_UI_GTK4
  if (getenv ("GTK_THEME") != NULL) {
    havetheme = true;
  }
  if (getenv ("GDK_SCALE") != NULL) {
    havescale = true;
  }
#endif

  while ((c = getopt_long_only (argc, bdj4argGetArgv (bdj4arg),
      "p:d:t:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 1: {
        prog = "aesed";
        nodetach = true;
        wait = true;
        ++validargs;
        break;
      }
      case 2: {
        prog = "bdj4altinst";
        nodetach = true;
        wait = true;
        isinstaller = true;
        ++validargs;
        break;
      }
      case 3: {
        prog = "bdj4cleantmp";
        nodetach = true;
        wait = true;
        ++validargs;
        break;
      }
      case 4: {
        prog = "bdj4dbupdate";
        ++validargs;
        break;
      }
      case 5: {
        prog = "bdj4info";
        nodetach = true;
        wait = true;
        ++validargs;
        break;
      }
      case 6: {
        prog = "bdj4installer";
        nodetach = true;
        wait = true;
        isinstaller = true;
        ++validargs;
        break;
      }
      case 7: {
        prog = "bdj4server";
        ++validargs;
        break;
      }
      case 8: {
        prog = "bdj4tags";
        nodetach = true;
        wait = true;
        ++validargs;
        break;
      }
      case 9: {
        prog = "bdj4updater";
        nodetach = true;
        wait = true;
        ++validargs;
        break;
      }
      case 10: {
        prog = "check_all";
        ++validargs;
        nodetach = true;
        wait = true;
        break;
      }
      case 11: {
        prog = "dmkmfromdb";
        nodetach = true;
        wait = true;
        ++validargs;
        break;
      }
      case 12: {
        prog = "tdbcompare";
        nodetach = true;
        wait = true;
        ++validargs;
        break;
      }
      case 13: {
        prog = "tdbsetval";
        nodetach = true;
        wait = true;
        ++validargs;
        break;
      }
      case 14: {
        prog = "testsuite";
        nodetach = true;
        wait = true;
        ++validargs;
        break;
      }
      case 15: {
        prog = "tmusicsetup";
        nodetach = true;
        wait = true;
        ++validargs;
        break;
      }
      case 16: {
        prog = "ttagdbchk";
        nodetach = true;
        wait = true;
        ++validargs;
        break;
      }
      case 17: {
        prog = "uitest";
        nodetach = true;
        wait = true;
        ++validargs;
        break;
      }
      case 18: {
        prog = "vlcsinklist";
        nodetach = true;
        wait = true;
        ++validargs;
        break;
      }
      case 19: {
        prog = "vlcversion";
        nodetach = false;
        wait = true;
        ++validargs;
        break;
      }
      case 20: {
        prog = "bdj4podcastupd";
        ++validargs;
        break;
      }
      case 'c': {
        forcenodetach = true;
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
      case 'p': {
        if (optarg != NULL) {
          sysvarsSetNum (SVL_PROFILE_IDX, atoi (optarg));
        }
        break;
      }
      case 'T': {
        if (optarg != NULL) {
          targ = bdj4argGet (bdj4arg, optind - 1, optarg);
#if BDJ4_UI_GTK3 || BDJ4_UI_GTK4
          osSetEnv ("GTK_THEME", targ);
#endif
        }
        havetheme = true;
        break;
      }
      case 'S': {
#if BDJ4_UI_GTK3 || BDJ4_UI_GTK4
        osSetEnv ("GDK_SCALE", optarg);
#endif
        havescale = true;
        break;
      }
      case 'P': {
        vlctag = bdj4argGet (bdj4arg, optind - 1, optarg);
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

  osGetCurrentDir (origcwd, sizeof (origcwd));
  pathNormalizePath (origcwd, sizeof (origcwd));

  if (sysvarsGetNum (SVL_DATAPATH) == SYSVARS_DATAPATH_UNKNOWN) {
    prog = "bdj4altinst";
    nodetach = true;
    wait = true;
    isinstaller = true;
    if (osChangeDir (sysvarsGetStr (SV_BDJ4_DIR_MAIN)) < 0) {
      fprintf (stderr, "Unable to set working dir: %s\n", sysvarsGetStr (SV_BDJ4_DIR_MAIN));
      exit (1);
    }
  }

  if (isinstaller == false) {
    if (osChangeDir (sysvarsGetStr (SV_BDJ4_DIR_DATATOP)) < 0) {
      fprintf (stderr, "Unable to set working dir: %s\n", sysvarsGetStr (SV_BDJ4_DIR_DATATOP));
      exit (1);
    }
  }

#if BDJ4_UI_GTK3 || BDJ4_UI_GTK4
  osSetEnv ("GTK_CSD", "0");
#endif

  if (isMacOS () || isLinux ()) {
    char      tbuff [MAXPATHLEN];
    char      pbuff [MAXPATHLEN];
    char      *path = NULL;
    char      *npath = NULL;
    size_t    sz = 4096;

    npath = mdmalloc (sz);

    path = getenv ("PATH");
    *npath = '\0';
    p = npath;
    end = npath + sz;
    if (isMacOS ()) {
      p = stpecpy (p, end, "/opt/local/bin:");
    }
    p = stpecpy (p, end, path);
    snprintf (pbuff, sizeof (pbuff), "%s/../plocal/bin",
        sysvarsGetStr (SV_BDJ4_DIR_EXEC));
    pathRealPath (tbuff, pbuff, sizeof (tbuff));
    p = stpecpy (p, end, ":");
    p = stpecpy (p, end, tbuff);
    osSetEnv ("PATH", npath);
    mdfree (npath);
  }

  if (isMacOS ()) {
    char    pbuff [MAXPATHLEN];
    char    tbuff [MAXPATHLEN];
    bool    foundvlc = false;

    /* players other than vlc may need a different setup */

    /* determine the location of VLC, /Applications or $HOME/Applications */
    snprintf (tbuff, sizeof (tbuff),
          "/Applications/%s.app", vlctag);
    if (fileopIsDirectory (tbuff)) {
      foundvlc = true;
    } else {
      snprintf (tbuff, sizeof (tbuff),
          "%s/Applications/%s.app", sysvarsGetStr (SV_HOME), vlctag);
      if (fileopIsDirectory (tbuff)) {
        foundvlc = true;
      }
    }

    if (foundvlc) {
      /* note that 'tbuff' currently has the main path */

      stpecpy (pbuff, pbuff + sizeof (pbuff), tbuff);

      if (sysvarsGetNum (SVL_VLC_VERSION) == 3) {
        /* VLC 3 */
        snprintf (tbuff, sizeof (tbuff), "%s/Contents/MacOS/lib/", pbuff);
        osSetEnv ("DYLD_FALLBACK_LIBRARY_PATH", tbuff);
        snprintf (tbuff, sizeof (tbuff), "%s/Contents/MacOS/plugins", pbuff);
        osSetEnv ("VLC_PLUGIN_PATH", tbuff);
      }

      if (sysvarsGetNum (SVL_VLC_VERSION) == 4) {
        /* VLC 4 */
        snprintf (tbuff, sizeof (tbuff), "%s/Contents/Frameworks", pbuff);
        osSetEnv ("DYLD_FALLBACK_LIBRARY_PATH", tbuff);
        snprintf (tbuff, sizeof (tbuff),
            "%s/Contents/Frameworks/plugins", pbuff);
        osSetEnv ("VLC_PLUGIN_PATH", tbuff);
      }
    }

    osSetEnv ("G_FILENAME_ENCODING", "UTF8-MAC");
  }

  if (isWindows ()) {
    char      * pbuff = NULL;
    char      * tbuff = NULL;
    char      * path = NULL;
    size_t    sz = 16384;

    pbuff = mdmalloc (sz);
    tbuff = mdmalloc (sz);
    path = mdmalloc (sz);

    *path = '\0';
    p = path;
    end = path + sz;

    stpecpy (pbuff, pbuff + sz, sysvarsGetStr (SV_BDJ4_DIR_EXEC));
    pathDisplayPath (pbuff, sz);
    p = stpecpy (p, end, pbuff);
    p = stpecpy (p, end, ";");

    snprintf (pbuff, sz, "%s/../plocal/bin", sysvarsGetStr (SV_BDJ4_DIR_EXEC));
    pathRealPath (tbuff, pbuff, sz);
    p = stpecpy (p, end, tbuff);
    p = stpecpy (p, end, ";");

    if (debugself) {
      fprintf (stderr, "base path: %s\n", path);
    }

    /* do not use double quotes w/environment var */
    snprintf (tbuff, sz, "C:\\Program Files\\VideoLAN\\%s", vlctag);
    p = stpecpy (p, end, tbuff);
    p = stpecpy (p, end, ";");
    osGetEnv ("PATH", tbuff, sz);
    if (debugself) {
      fprintf (stderr, "sys path: %s\n", tbuff);
    }
    p = stpecpy (p, end, tbuff);

    osSetEnv ("PATH", path);

    if (debugself) {
      osGetEnv ("PATH", tbuff, sz);
      fprintf (stderr, "final PATH=%s\n", tbuff);
    }

#if BDJ4_UI_GTK3 || BDJ4_UI_GTK4
    osSetEnv ("PANGOCAIRO_BACKEND", "fc");
#endif

    dataFree (pbuff);
    dataFree (tbuff);
    dataFree (path);
  }

  if (! havetheme) {
    pathbldMakePath (buff, sizeof (buff),
        "theme", BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
    if (fileopFileExists (buff)) {
      fh = fileopOpen (buff, "r");
      (void) ! fgets (buff, sizeof (buff), fh);
      mdextfclose (fh);
      fclose (fh);
      stringTrim (buff);
#if BDJ4_UI_GTK3 || BDJ4_UI_GTK4
      osSetEnv ("GTK_THEME", buff);
#endif
      havetheme = true;
    }
  }

  /* 4.0.10 2023-1-27 : set the GDK_SCALE env var */
  /* not an issue if not set -- defaults to 1 */
  if (! havescale) {
    pathbldMakePath (buff, sizeof (buff),
        "scale", BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
    if (fileopFileExists (buff)) {
      fh = fileopOpen (buff, "r");
      (void) ! fgets (buff, sizeof (buff), fh);
      mdextfclose (fh);
      fclose (fh);
      stringTrim (buff);
#if BDJ4_UI_GTK3 || BDJ4_UI_GTK4
      osSetEnv ("GDK_SCALE", buff);
#endif
      havescale = true;
    }
  }

#if BDJ4_UI_GTK3 || BDJ4_UI_GTK4
  if (! havetheme && isWindows ()) {
    osSetEnv ("GTK_THEME", "Windows-10-Dark");
  }
  if (! havetheme && isMacOS ()) {
    osSetEnv ("GTK_THEME", "Mojave-dark-solid");
  }
#endif

  /* launch the program */

  if (debugself) {
#if BDJ4_UI_GTK3 || BDJ4_UI_GTK4
    fprintf (stderr, "GTK_THEME=%s\n", getenv ("GTK_THEME"));
    fprintf (stderr, "GTK_CSD=%s\n", getenv ("GTK_CSD"));
    fprintf (stderr, "PANGOCAIRO_BACKEND=%s\n", getenv ("PANGOCAIRO_BACKEND"));
#endif
    fprintf (stderr, "LC_ALL=%s\n", getenv ("LC_ALL"));
  }

  targc = 0;
  for (int i = 0; i < argc; ++i) {
    if (targc >= BDJ4_LAUNCHER_MAX_ARGS) {
      fprintf (stderr, "ERR: too many arguments\n");
      exit (1);
    }
    targ = bdj4argGet (bdj4arg, i, argv [i]);
    targv [targc++] = targ;
  }
  if (sysvarsGetNum (SVL_DATAPATH) == SYSVARS_DATAPATH_ALT) {
    targv [targc++] = "--datatopdir";
    targv [targc++] = sysvarsGetStr (SV_BDJ4_DIR_DATATOP);
  }
  if (targc >= (BDJ4_LAUNCHER_MAX_ARGS - 4)) {
    fprintf (stderr, "ERR: too many arguments\n");
    exit (1);
  }
  targv [targc++] = "--bdj4";
  targv [targc++] = "--origcwd";
  targv [targc++] = origcwd;
  targv [targc++] = NULL;

  pathbldMakePath (buff, sizeof (buff),
      prog, sysvarsGetStr (SV_OS_EXEC_EXT), PATHBLD_MP_DIR_EXEC);
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

#if BDJ4_MEM_DEBUG
  /* report is generally not needed unless source is changed */
  //mdebugReport ();
  mdebugCleanup ();
#endif

  bdj4argCleanup (bdj4arg);
  return rc;
}

