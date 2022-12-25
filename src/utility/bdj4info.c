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
#include <assert.h>
#include <getopt.h>
#include <unistd.h>

#include "bdjopt.h"
#include "localeutil.h"
#include "sysvars.h"

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
    { "msys",         no_argument,      NULL,   0 },
    { "theme",        required_argument,NULL,   0 },
    { NULL,         0,                  NULL,   0 }
  };

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

  for (int i = 0; i < SV_MAX; ++i) {
    if (i == SV_TEMP_A || i == SV_TEMP_B) {
      continue;
    }
    fprintf (stdout, " s: %-20s %s (%d)\n", sysvarsDesc (i), sysvarsGetStr (i), i);
  }

  for (int i = 0; i < SVL_MAX; ++i) {
    fprintf (stdout, "sl: %-20s %"PRId64" (%d)\n", sysvarslDesc (i), sysvarsGetNum (i), i);
  }

  bdjoptInit ();
  bdjoptDump ();
  bdjoptCleanup ();
  localeCleanup ();

  return 0;
}
