#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <locale.h>

#pragma clang diagnostic push
#pragma gcc diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "check_bdj.h"
#include "log.h"
#include "osutils.h"
#include "sysvars.h"

int
main (int argc, char *argv [])
{
  int     c = 0;
  int     option_index = 0;
  int     number_failed = 0;
  bool    skiplong = false;

  static struct option coptions [] = {
    { "check_all",  no_argument, NULL, 0 },
    { "bdj4",       no_argument, NULL, 0 },
    { "skiplong",   no_argument, NULL, 's' },
    { NULL,         0,           NULL, 0 }
  };

  setlocale (LC_ALL, "");
  if (isWindows ()) {
    osSetEnv ("LC_ALL", "en-US");
  }

  while ((c = getopt_long_only (argc, argv, "s", coptions, &option_index)) != -1) {
    switch (c) {
      case 's': {
        skiplong = true;
        break;
      }
    }
  }

  sysvarsInit (argv [0]);
  if (chdir (sysvarsGetStr (SV_BDJ4DATATOPDIR)) < 0) {
    fprintf (stderr, "Unable to chdir: %s\n", sysvarsGetStr (SV_BDJ4DATATOPDIR));
    exit (1);
  }

  logStart ("check_all", "ck", LOG_ALL);

  number_failed += check_libcommon (skiplong);
  number_failed += check_libbdj4 (skiplong);

  logEnd ();
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
