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

int
check_libcommon (bool skiplong)
{
  int     number_failed = 0;
  Suite   *s;
  SRunner *sr;

  /* libcommon:
   *  osutils
   *  fileop      complete
   *  bdjstring   needs istringCompare (test w/de_DE, sv_SE)
   *  osprocess
   *  filedata    done
   *  osnetutils
   *  pathutil    done
   *  sysvars
   *  tmutil      complete (check)
   *  osdir
   *  dirop       complete
   *  filemanip   need renameall test
   *  fileutil
   *  pathbld     complete
   *  log
   *  nlist       partial
   *  ilist       partial
   *  slist       partial
   *  bdjmsg      complete
   *  sock        mostly
   *  datafile    partial
   *  bdjvars
   *  sockh
   *  bdjopt
   *  conn
   *  procutil
   *  lock        partial
   *  rafile      partial
   *  localeutil
   *  osrandom
   *  queue
   *  dirlist     needs utf-8 basic/recursive
   *  misc
   *  ossignal
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

  s = pathbld_suite();
  srunner_add_suite (sr, s);

  s = filemanip_suite();
  srunner_add_suite (sr, s);

  s = nlist_suite();
  srunner_add_suite (sr, s);

  s = slist_suite();
  srunner_add_suite (sr, s);

  s = ilist_suite();
  srunner_add_suite (sr, s);

  s = bdjmsg_suite();
  srunner_add_suite (sr, s);

  if (! skiplong) {
    s = sock_suite();
    srunner_add_suite (sr, s);
  }

  s = datafile_suite();
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

  s = dirlist_suite();
  srunner_add_suite (sr, s);

  srunner_run_all (sr, CK_ENV);
  number_failed += srunner_ntests_failed (sr);
  srunner_free (sr);

  return number_failed;
}
