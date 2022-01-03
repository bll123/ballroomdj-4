#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <check.h>

#include "check_bdj.h"
#include "sysvars.h"
#include "tagdef.h"
#include "log.h"

int
main (int argc, char *argv [])
{
  int     number_failed;
  Suite   *s;
  SRunner *sr;

  sysvarsInit (argv [0]);
  if (chdir (sysvars [SV_BDJ4DIR]) < 0) {
    fprintf (stderr, "Unable to chdir: %s\n", sysvars [SV_BDJ4DIR]);
    exit (1);
  }

  logStart ("c", LOG_LVL_6);
  s = tmutil_suite();
  sr = srunner_create (s);
  s = process_suite();
  srunner_add_suite (sr, s);
  s = pathutil_suite();
  srunner_add_suite (sr, s);
  s = fileop_suite();
  srunner_add_suite (sr, s);
  s = sock_suite();
  srunner_add_suite (sr, s);
  s = lock_suite();
  srunner_add_suite (sr, s);
  s = list_suite();
  srunner_add_suite (sr, s);
  s = rafile_suite();
  srunner_add_suite (sr, s);
  s = datafile_suite();
  srunner_add_suite (sr, s);
  srunner_run_all (sr, CK_ENV);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  logEnd ();
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
