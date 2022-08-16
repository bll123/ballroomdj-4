#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
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
    /* ignored */
    { "nodetach",     no_argument,      NULL,   0 },
    { "debugself",    no_argument,      NULL,   0 },
    { "msys",         no_argument,      NULL,   0 },
    { "theme",        required_argument,NULL,   0 },
    { NULL,         0,                  NULL,   0 }
  };

  while ((c = getopt_long_only (argc, argv, "g:r:u:", bdj_options, &option_index)) != -1) {
    switch (c) {
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
    fprintf (stdout, " s: %-20s %zd (%d)\n", sysvarslDesc (i), sysvarsGetNum (i), i);
  }

  bdjoptInit ();
  bdjoptDump ();
  bdjoptCleanup ();

  return 0;
}
