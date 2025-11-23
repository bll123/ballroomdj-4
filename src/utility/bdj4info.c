/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#define __STDC_WANT_LIB_EXT1__ 1
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <limits.h>

#if BDJ4_UI_GTK3
# include <gtk/gtk.h>
#endif

#include "bdj4.h"
#include "bdj4arg.h"
#include "bdjopt.h"
#include "bdjvars.h"
#include "fileop.h"
#include "localeutil.h"
#include "mdebug.h"
#include "osenv.h"
#include "sysvars.h"

static const char *envitems [] = {
  "DESKTOP_SESSION",
  "DYLD_FALLBACK_LIBRARY_PATH",
  "GDK_BACKEND",
  "GDK_SCALE",
  "G_FILENAME_ENCODING",
  "GTK_BACKEND",
  "GTK_CSD",
  "GTK_THEME",
  "HOME",
  "LC_ALL",
  "NUMBER_OF_PROCESSORS",
  "PANGOCAIRO_BACKEND",
  "PATH",
  "TEMP",
  "TMP",
  "TMPDIR",
  "USER",
  "USERNAME",
  "USERPROFILE",
  "VLC_PLUGIN_PATH",
  "XDG_CACHE_HOME",
  "XDG_CONFIG_HOME",
  "XDG_CURRENT_DESKTOP",
  "XDG_SESSION_DESKTOP",
  "XDG_SESSION_TYPE",
};
enum {
  ENV_MAX = sizeof (envitems) / sizeof (const char *),
};

int
main (int argc, char *argv [])
{
  int     c = 0;
  int     option_index = 0;
  bool    isbdj4 = false;
  bdj4arg_t   *bdj4arg;
  const char  *targ;

  static struct option bdj_options [] = {
    { "bdj4info",     no_argument,        NULL,   0 },
    { "bdj4",         no_argument,        NULL,   'B' },
    { "datatopdir",   required_argument,  NULL,   't' },
    { "profile",      required_argument,  NULL,   'p' },
    { "locale",       required_argument,  NULL,   'L' },
    /* ignored */
    { "debugself",    no_argument,        NULL,   0 },
    { "nodetach",     no_argument,        NULL,   0 },
    { "origcwd",      required_argument,  NULL,   0 },
    { "scale",        required_argument,  NULL,   0 },
    { "theme",        required_argument,  NULL,   0 },
    { "pli",          required_argument,  NULL,   0 },
    { "wait",         no_argument,        NULL,   0 },
    { NULL,           0,                  NULL,   0 }
  };

#if BDJ4_MEM_DEBUG
  mdebugInit ("info");
#endif

  bdj4arg = bdj4argInit (argc, argv);

  targ = bdj4argGet (bdj4arg, 0, argv [0]);
  sysvarsInit (targ, SYSVARS_FLAG_ALL);
  localeInit ();
  bdjvarsInit ();

  while ((c = getopt_long_only (argc, bdj4argGetArgv (bdj4arg),
      "Bt:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 't': {
        if (optarg != NULL) {
          targ = bdj4argGet (bdj4arg, optind - 1, optarg);
          if (fileopIsDirectory (targ)) {
            sysvarsSetStr (SV_BDJ4_DIR_DATATOP, targ);
            sysvarsSetNum (SVL_DATAPATH, SYSVARS_DATAPATH_ALT);
          }
        }
        break;
      }
      case 'p': {
        if (optarg != NULL) {
          sysvarsSetNum (SVL_PROFILE_IDX, atoi (optarg));
        }
        break;
      }
      case 'L': {
        if (optarg != NULL) {
          char    tbuff [40];

          sysvarsSetStr (SV_LOCALE, optarg);
          snprintf (tbuff, sizeof (tbuff), "%.2s", optarg);
          sysvarsSetStr (SV_LOCALE_SHORT, tbuff);
          sysvarsSetNum (SVL_LOCALE_SET, 1);
          localeSetup ();
        }
        break;
      }
      case 'B': {
        isbdj4 = true;
        break;
      }
      default: {
        break;
      }
    }
  }

  if (! isbdj4) {
    fprintf (stderr, "not started with launcher\n");
    exit (1);
  }

  bdjvarsUpdateData ();

  /* C language: integer sizes */

  fprintf (stdout, "  i: bool   %d\n", (int) sizeof (bool));
  fprintf (stdout, "  i: short  %d %d\n", (int) sizeof (short), SHRT_MAX);
  fprintf (stdout, "  i: int    %d %ld\n", (int) sizeof (int), (long) INT_MAX);
  fprintf (stdout, "  i: long   %d %ld\n", (int) sizeof (long), (long) LONG_MAX);
  fprintf (stdout, "  i: uint32_t %d %" PRIu32 "\n", (int) sizeof (uint32_t), UINT32_MAX);
  fprintf (stdout, "  i: uint64_t %d %" PRIu64 "\n", (int) sizeof (uint64_t), UINT64_MAX);
  fprintf (stdout, "  i: pid_t  %d\n", (int) sizeof (pid_t));
  fprintf (stdout, "  i: size_t %d\n", (int) sizeof (size_t));
  fprintf (stdout, "  i: time_t %d\n", (int) sizeof (time_t));

  /* C language */

#if _POSIX_C_SOURCE
  fprintf (stdout, "  c: _POSIX_C_SOURCE %ld\n", _POSIX_C_SOURCE);
#endif

  c = 0;
#if defined (__STDC_LIB_EXT1__)
  c = 1;
#endif
  fprintf (stdout, "  c: __STDC_LIB_EXT1__ %d\n", c);

  c = 0;
#if defined(__STDC_NO_ATOMICS__)
  c = 1;
#endif
  fprintf (stdout, "  c: __STDC_NO_ATOMICS__ %d\n", c);

  fprintf (stdout, "  c: __STDC_VERSION__ %ld\n", __STDC_VERSION__);

  /* UI */
#if BDJ4_UI_GTK3
  fprintf (stdout, " ui: GTK3\n");
#endif
#if BDJ4_UI_GTK4
  fprintf (stdout, " ui: GTK4\n");
#endif
#if BDJ4_UI_GTK3 || BDJ4_UI_GTK4
  fprintf (stdout, "  i: gboolean %d\n", (int) sizeof (gboolean));
  fprintf (stdout, "  i: gint  %d\n", (int) sizeof (gint));
  fprintf (stdout, "  i: glong %d\n", (int) sizeof (glong));
#endif
#if BDJ4_UI_NULL
  fprintf (stdout, "ui: null\n");
#endif
#if BDJ4_UI_MACOS
  fprintf (stdout, "ui: macos\n");
#endif

  /* environment */

  for (int i = 0; i < ENV_MAX; ++i) {
    char  tmp [MAXPATHLEN];

    osGetEnv (envitems [i], tmp, sizeof (tmp));
    if (*tmp) {
      fprintf (stdout, "  e: %-20s %s\n", envitems [i], tmp);
    }
  }

  /* other stuff */

  if (fileopIsDirectory ("/opt/local/bin")) {
    fprintf (stdout, "pkg: MacOS: MacPorts\n");
  }
  if (fileopIsDirectory ("/opt/pkg/bin")) {
    fprintf (stdout, "pkg: MacOS: pkgsrc\n");
  }
  if (fileopIsDirectory ("/opt/homebrew/bin")) {
    fprintf (stdout, "pkg: MacOS: Homebrew - /opt/homebrew\n");
  }
  if (fileopIsDirectory ("/usr/local/Homebrew")) {
    fprintf (stdout, "pkg: MacOS: Homebrew - /usr/local\n");
  }
  if (fileopFileExists ("data/macos.homebrew")) {
    fprintf (stdout, "pkg: MacOS: Homebrew forced\n");
  }
  if (fileopFileExists ("data/macos.pkgsrc")) {
    fprintf (stdout, "pkg: MacOS: pkgsrc forced\n");
  }

  /* sysvars */

  for (int i = 0; i < SV_MAX; ++i) {
    if (i == SV_TEMP_A || i == SV_TEMP_B) {
      continue;
    }
    fprintf (stdout, "  s: %-20s %s (%d)\n", sysvarsDesc (i), sysvarsGetStr (i), i);
  }

  /* sysvars numeric */

  for (int i = 0; i < SVL_MAX; ++i) {
    fprintf (stdout, " sl: %-20s %" PRId64 " (%d)\n", sysvarslDesc (i), sysvarsGetNum (i), i);
  }

  for (int i = 0; i < BDJV_MAX; ++i) {
    fprintf (stdout, "  v: %-20s %s (%d)\n", bdjvarsDesc (i), bdjvarsGetStr (i), i);
  }
  for (int i = 0; i < BDJVL_MAX; ++i) {
    fprintf (stdout, " vl: %-20s %" PRId64 " (%d)\n", bdjvarslDesc (i), bdjvarsGetNum (i), i);
  }

  bdjoptInit ();
  bdjoptDump ();

  bdjvarsCleanup ();
  bdjoptCleanup ();
  localeCleanup ();
  bdj4argCleanup (bdj4arg);
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return 0;
}
