/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <limits.h>

#if BDJ4_USE_GTK3
# include <gtk/gtk.h>
#endif

#include "bdj4.h"
#include "bdjopt.h"
#include "fileop.h"
#include "localeutil.h"
#include "mdebug.h"
#include "osutils.h"
#include "sysvars.h"

const char *envitems [] = {
  "DESKTOP_SESSION",
  "GDK_SCALE",
  "GTK_CSD",
  "GTK_THEME",
  "HOME",
  "LC_ALL",
  "NUMBER_OF_PROCESSORS",
  "PANGOCAIRO_BACKEND",
  "PATH",
  "PYTHONIOENCODING",
  "TEMP",
  "TMP",
  "TMPDIR",
  "USER",
  "USERNAME",
  "USERPROFILE",
  "XDG_CONFIG_HOME",
  "XDG_CURRENT_DESKTOP",
  "XDG_SESSION_DESKTOP",
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

  static struct option bdj_options [] = {
    { "bdj4info",   no_argument,        NULL,   0 },
    { "bdj4",       no_argument,        NULL,   'B' },
    { "datatopdir", required_argument,  NULL,   't' },
    /* ignored */
    { "nodetach",     no_argument,      NULL,   0 },
    { "debugself",    no_argument,      NULL,   0 },
    { "scale",        required_argument,NULL,   0 },
    { "theme",        required_argument,NULL,   0 },
    { "origcwd",      required_argument,  NULL,   0 },
    { NULL,         0,                  NULL,   0 }
  };

#if BDJ4_MEM_DEBUG
  mdebugInit ("info");
#endif

  while ((c = getopt_long_only (argc, argv, "Bt:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 't': {
        if (fileopIsDirectory (optarg)) {
          sysvarsSetStr (SV_BDJ4_DIR_DATATOP, optarg);
          sysvarsSetNum (SVL_DATAPATH, SYSVARS_DATAPATH_ALT);
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

  sysvarsInit (argv [0]);
  localeInit ();

  fprintf (stdout, " i: bool   %d\n", (int) sizeof (bool));
  fprintf (stdout, " i: short  %d\n", (int) sizeof (short));
  fprintf (stdout, " i: int    %d %ld\n", (int) sizeof (int), (long) INT_MAX);
  fprintf (stdout, " i: long   %d %ld\n", (int) sizeof (long), (long) LONG_MAX);
  fprintf (stdout, " i: pid_t  %d\n", (int) sizeof (pid_t));
  fprintf (stdout, " i: size_t %d\n", (int) sizeof (size_t));
  fprintf (stdout, " i: time_t %d\n", (int) sizeof (time_t));
  fprintf (stdout, " i: uint32_t %d %ld\n", (int) sizeof (uint32_t), (long) INT32_MAX);
  fprintf (stdout, " i: uint64_t %d\n", (int) sizeof (uint64_t));
#if BDJ4_USE_GTK3
  fprintf (stdout, " i: gboolean %d\n", (int) sizeof (gboolean));
  fprintf (stdout, " i: gint  %d\n", (int) sizeof (gint));
  fprintf (stdout, " i: glong %d\n", (int) sizeof (glong));
  fprintf (stdout, "ui: GTK3\n");
#endif
#if BDJ4_USE_NULLUI
  fprintf (stdout, "ui: null\n");
#endif

  for (int i = 0; i < ENV_MAX; ++i) {
    char  tmp [MAXPATHLEN];

    osGetEnv (envitems [i], tmp, sizeof (tmp));
    if (*tmp) {
      fprintf (stdout, " e: %-20s %s\n", envitems [i], tmp);
    }
  }

  for (int i = 0; i < SV_MAX; ++i) {
    if (i == SV_TEMP_A || i == SV_TEMP_B) {
      continue;
    }
    fprintf (stdout, " s: %-20s %s (%d)\n", sysvarsDesc (i), sysvarsGetStr (i), i);
  }

  for (int i = 0; i < SVL_MAX; ++i) {
    fprintf (stdout, "sl: %-20s %" PRId64 " (%d)\n", sysvarslDesc (i), sysvarsGetNum (i), i);
  }

  bdjoptInit ();
  bdjoptDump ();
  bdjoptCleanup ();
  localeCleanup ();

#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return 0;
}
