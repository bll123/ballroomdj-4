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
#include "localeutil.h"
#include "log.h"
#include "osrandom.h"
#include "osutils.h"
#include "sysvars.h"

int
main (int argc, char *argv [])
{
  SRunner *sr = NULL;
  int     c = 0;
  int     option_index = 0;
  int     number_failed = 0;
  bool    skipslow = false;

  static struct option coptions [] = {
    { "check_all",  no_argument, NULL, 0 },
    { "bdj4",       no_argument, NULL, 0 },
    { "skipslow",   no_argument, NULL, 's' },
    { NULL,         0,           NULL, 0 }
  };

  while ((c = getopt_long_only (argc, argv, "s", coptions, &option_index)) != -1) {
    switch (c) {
      case 's': {
        skipslow = true;
        break;
      }
    }
  }

  sRandom ();
  sysvarsInit (argv [0]);
  localeInit ();

  if (chdir (sysvarsGetStr (SV_BDJ4DATATOPDIR)) < 0) {
    fprintf (stderr, "Unable to chdir: %s\n", sysvarsGetStr (SV_BDJ4DATATOPDIR));
    exit (1);
  }

  logStart ("check_all", "ck", LOG_ALL);

  sr = srunner_create (NULL);
  check_libcommon (sr);
  check_libbasic (sr);
  check_libbdj4 (sr);
  /* if the durations are needed */
//  srunner_set_xml (sr, "tmp/check.xml");
  srunner_set_log (sr, "tmp/check.log");
  srunner_run_all (sr, CK_ENV);
  number_failed += srunner_ntests_failed (sr);
  srunner_free (sr);

  localeCleanup ();
  logEnd ();
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
