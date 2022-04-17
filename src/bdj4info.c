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
  bdjoptFree ();

  return 0;
}
