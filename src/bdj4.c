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
#include "fileop.h"
#include "pathbld.h"
#include "pathutil.h"
#include "portability.h"
#include "sysvars.h"

#if _lib_CreateProcess
static void winCreateProcess (char *cmd, char *argv []);
#endif

int
main (int argc, char * argv[])
{
  char      buff [MAXPATHLEN];
  char      unpackdir [MAXPATHLEN];
  int       validargs = 0;
  int       c = 0;
  int       option_index = 0;
  char      *prog;
  char      *extension;
  bool      debugself = false;
  bool      havetheme = false;
  bool      isinstaller = false;

  static struct option bdj_options [] = {
    { "check_all",  no_argument,        NULL,   1 },
    { "cli",        no_argument,        NULL,   2 },
    { "configui",   no_argument,        NULL,   3 },
    { "converter",  no_argument,        NULL,   4 },
    { "main",       no_argument,        NULL,   5 },
    { "manageui",   no_argument,        NULL,   6 },
    { "marquee",    no_argument,        NULL,   7 },
    { "mobilemq",   no_argument,        NULL,   8 },
    { "player",     no_argument,        NULL,   9 },
    { "playerui",   no_argument,        NULL,   10 },
    { "remctrl",    no_argument,        NULL,   11 },
    { "installer",  no_argument,        NULL,   12 },
    { "locale",     no_argument,        NULL,   13 },
    { "forcestop",  no_argument,        NULL,   0 },
    { "unpackdir",  required_argument,  NULL,   'u' },
    { "reinstall",  no_argument,        NULL,   'r' },
    { "guienabled", no_argument,        NULL,   'g' },
    { "profile",    required_argument,  NULL,   'p' },
    { "debug",      required_argument,  NULL,   'd' },
    { "theme",      required_argument,  NULL,   't' },
    { "debugself",  no_argument,        NULL,   'D' },
    { NULL,         0,                  NULL,   0 }
  };

  prog = "bdj4playerui";

  sysvarsInit (argv [0]);

  while ((c = getopt_long_only (argc, argv, "p:d:t:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 1: {
        prog = "check_all";
        ++validargs;
        break;
      }
      case 2: {
        prog = "bdj4cli";
        ++validargs;
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
        isinstaller = true;
        ++validargs;
        break;
      }
      case 13: {
        prog = "bdj4locale";
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
      case 'p': {
        if (optarg) {
          sysvarSetNum (SVL_BDJIDX, atol (optarg));
        }
        break;
      }
      case 't': {
        snprintf (buff, sizeof (buff), "GTK_THEME=%s", optarg);
        putenv (buff);
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

  if (chdir (sysvarsGetStr (SV_BDJ4DATATOPDIR)) < 0) {
    if (! isinstaller) {
      fprintf (stderr, "Unable to set working dir: %s\n", sysvarsGetStr (SV_BDJ4DATATOPDIR));
      exit (1);
    }
  }

  fileopMakeDir ("tmp");

  putenv ("GTK_OVERLAY_SCROLLING=0");
  putenv ("GTK_CSD=0");

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

    putenv ("G_FILENAME_ENCODING=UTF8-MAC");
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

    pathToWinPath (pbuff, sysvarsGetStr (SV_BDJ4EXECDIR), sz);
    if (debugself) {
      fprintf (stderr, "execdir: %s\n", pbuff);
    }
    strlcpy (path, "PATH=", sz);
    strlcat (path, getenv ("PATH"), sz);
    strlcat (path, ";", sz);
    strlcat (path, pbuff, sz);

    strlcat (path, ";", sz);
    snprintf (pbuff, sz, "%s\\..\\plocal\\bin", sysvarsGetStr (SV_BDJ4EXECDIR));
    pathRealPath (tbuff, pbuff);
    strlcat (path, tbuff, sz);

    strlcat (path, ";", sz);
    /* do not use double quotes w/putenv */
    strlcat (path, "C:\\Program Files\\VideoLAN\\VLC", sz);
    putenv (path);

    if (debugself) {
      fprintf (stderr, "PATH=%s\n", getenv ("PATH"));
    }

    free (pbuff);
    free (tbuff);
    free (path);
  }

  /* launch the program */

  if (debugself) {
    fprintf (stderr, "GTK_THEME=%s\n", getenv ("GTK_THEME"));
    fprintf (stderr, "GTK_CSD=%s\n", getenv ("GTK_CSD"));
    fprintf (stderr, "GTK_OVERLAY_SCROLLING=%s\n", getenv ("GTK_OVERLAY_SCROLLING"));
  }

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
  if (debugself) {
    fprintf (stderr, "cmd: %s\n", buff);
  }

#if _lib_CreateProcess
  winCreateProcess (buff, argv, isinstaller);
#else
  if (execv (buff, argv) < 0) {
    fprintf (stderr, "Unable to start %s %d %s\n", buff, errno, strerror (errno));
  }
#endif
  return 0;
}

#if _lib_CreateProcess

static void
winCreateProcess (char *cmd, char *argv [], bool isinstaller)
{
  STARTUPINFO         si;
  PROCESS_INFORMATION pi;
  char                tmp [MAXPATHLEN];
  int                 i;
  int                 val;

  memset (&si, '\0', sizeof (si));
  si.cb = sizeof(si);
  memset (&pi, '\0', sizeof (pi));

  tmp [0] = '\0';
  i = 0;
  while (argv [i] != NULL) {
    strlcat (tmp, argv [i], MAXPATHLEN);
    strlcat (tmp, " ", MAXPATHLEN);
    ++i;
  }

  val = DETACHED_PROCESS;
  if (isinstaller) {
    val = 0;
  }
  CreateProcess (
      cmd,            // module name
      tmp,            // command line
      NULL,           // process handle
      NULL,           // hread handle
      FALSE,          // handle inheritance
      val,            // set to DETACHED_PROCESS
      NULL,           // parent's environment
      NULL,           // parent's starting directory
      &si,            // STARTUPINFO structure
      &pi );          // PROCESS_INFORMATION structure
}

#endif
