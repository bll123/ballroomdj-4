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

void
check_libbdj4 (SRunner *sr)
{
  Suite   *s;

  /* libbdj4
   *  bdjvarsdf             complete // needed by tests
   *  templateutil          complete // needed by tests; needs localized tests
   *  bdjvarsdfload         complete // needed by tests; uses templateutil
   *  dnctypes              complete
   *  dance                 complete
   *  genre                 complete
   *  level                 complete
   *  rating                complete
   *  songfav               complete
   *  status                complete
   *  songutil              complete
   *  tagdef                complete
   *  song                  complete
   *  musicdb               complete
   *  songlist              complete
   *  autosel               complete
   *  songfilter            complete
   *  dancesel              complete
   *  sequence              complete
   *  songsel
   *  playlist
   *  sortopt               complete
   *  dispsel               complete
   *  samesong              complete
   *  orgutil               partial
   *  validate              complete
   *  webclient
   *  audiotag
   *  m3u
   *  orgopt                complete
   *  bdj4init
   *  msgparse
   *  songdb
   *  volreg
   *  musicq
   */

  logMsg (LOG_DBG, LOG_IMPORTANT, "==chk== libbdj4");

  s = bdjvarsdf_suite();
  srunner_add_suite (sr, s);

  s = templateutil_suite();
  srunner_add_suite (sr, s);

  s = bdjvarsdfload_suite();
  srunner_add_suite (sr, s);

  s = dnctypes_suite();
  srunner_add_suite (sr, s);

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

  s = song_suite();
  srunner_add_suite (sr, s);

  s = musicdb_suite();
  srunner_add_suite (sr, s);

  s = songlist_suite();
  srunner_add_suite (sr, s);

  s = autosel_suite();
  srunner_add_suite (sr, s);

  s = songfilter_suite();
  srunner_add_suite (sr, s);

  s = dancesel_suite();
  srunner_add_suite (sr, s);

  s = sequence_suite();
  srunner_add_suite (sr, s);

  s = sortopt_suite();
  srunner_add_suite (sr, s);

  s = dispsel_suite();
  srunner_add_suite (sr, s);

  s = samesong_suite();
  srunner_add_suite (sr, s);

  s = orgutil_suite();
  srunner_add_suite (sr, s);

  s = validate_suite();
  srunner_add_suite (sr, s);

  s = orgopt_suite();
  srunner_add_suite (sr, s);
}
