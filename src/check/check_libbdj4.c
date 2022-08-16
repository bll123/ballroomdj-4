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
check_libbdj4 (bool skiplong)
{
  int     number_failed = 0;
  Suite   *s;
  SRunner *sr;

  /* libbdj4
   *  bdjvarsdf
   *    needed by tests
   *  bdjvarsdfload
   *    needed by tests
   *  dnctypes              complete
   *  templateutil
   *    needed by tests
   *  dance                 complete
   *  genre                 complete
   *  level                 complete
   *  rating                complete
   *  songfav               complete
   *  status                complete
   *  songutil              complete
   *  tagdef                complete
   *  song
   *  musicdb
   *  songlist
   *  autosel
   *  songfilter
   *  dancesel
   *  sequence
   *  songsel
   *  playlist
   *  sortopt
   *  dispsel
   *  orgutil
   *  validate
   *  webclient
   *  audiotag
   *  m3u
   *  orgopt
   *  bdj4init
   *  msgparse
   *  songdb
   *  volreg
   *  musicq
   */

  fprintf (stdout, "=== libbdj4\n");

  s = dnctypes_suite();
  sr = srunner_create (s);

  s = dance_suite();
  srunner_add_suite (sr, s);

  s = genre_suite();
  srunner_add_suite (sr, s);

  s = rating_suite();
  srunner_add_suite (sr, s);

  s = level_suite();
  srunner_add_suite (sr, s);

  s = songfav_suite();
  srunner_add_suite (sr, s);

  s = status_suite();
  srunner_add_suite (sr, s);

  s = songutil_suite();
  srunner_add_suite (sr, s);

  s = tagdef_suite();
  srunner_add_suite (sr, s);

  s = orgutil_suite();
  srunner_add_suite (sr, s);

  s = validate_suite();
  srunner_add_suite (sr, s);

  srunner_run_all (sr, CK_ENV);
  number_failed += srunner_ntests_failed (sr);
  srunner_free (sr);

  return number_failed;
}
