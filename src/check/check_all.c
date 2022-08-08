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
#include "sysvars.h"
#include "log.h"

int
main (int argc, char *argv [])
{
  int     c = 0;
  int     option_index = 0;
  int     number_failed = 0;
  bool    skiplong = false;
  Suite   *s;
  SRunner *sr;

  static struct option coptions [] = {
    { "check_all",  no_argument, NULL, 0 },
    { "bdj4",       no_argument, NULL, 0 },
    { "skiplong",   no_argument, NULL, 's' },
    { NULL,             0,                  NULL,   0 }
  };

  setlocale (LC_ALL, "");

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

  /* libcommon:
   *  bdjstring
   *  osutils
   *  fileop
   *  filedata
   *  osnetutils
   *  pathutil
   *  sysvars
   *  tmutil
   *  pathbld
   *  filemanip
   *  fileutil
   *  log
   *  nlist
   *  ilist
   *  slist
   *  bdjmsg
   *  sock
   *  datafile
   *  bdjvars
   *  sockh
   *  bdjopt
   *  conn
   *  procutil
   *  lock
   *  rafile
   *  queue
   *  dirop
   *  misc
   *  localeutil
   *  progstate
   */

  fprintf (stdout, "=== libcommon\n");

  s = bdjstring_suite();
  sr = srunner_create (s);

  s = fileop_suite();
  srunner_add_suite (sr, s);

  s = filedata_suite();
  srunner_add_suite (sr, s);

  s = pathutil_suite();
  srunner_add_suite (sr, s);

  if (! skiplong) {
    s = tmutil_suite();
    srunner_add_suite (sr, s);
  }

  s = filemanip_suite();
  srunner_add_suite (sr, s);

  if (! skiplong) {
    s = sock_suite();
    srunner_add_suite (sr, s);
  }

  s = datafile_suite();
  srunner_add_suite (sr, s);

  s = nlist_suite();
  srunner_add_suite (sr, s);

  s = slist_suite();
  srunner_add_suite (sr, s);

  s = ilist_suite();
  srunner_add_suite (sr, s);

  if (! skiplong) {
    s = procutil_suite();
    srunner_add_suite (sr, s);
  }

  if (! skiplong) {
    s = lock_suite();
    srunner_add_suite (sr, s);
  }

  s = rafile_suite();
  srunner_add_suite (sr, s);

  s = queue_suite();
  srunner_add_suite (sr, s);

  s = dirop_suite();
  srunner_add_suite (sr, s);

  srunner_run_all (sr, CK_ENV);
  number_failed += srunner_ntests_failed (sr);
  srunner_free (sr);

  /* libbdj4
   */

  fprintf (stdout, "=== libbdj4\n");

  s = orgutil_suite();
  sr = srunner_create (s);

  s = validate_suite();
  srunner_add_suite (sr, s);

  srunner_run_all (sr, CK_ENV);
  number_failed += srunner_ntests_failed (sr);
  srunner_free (sr);

  logEnd ();
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
