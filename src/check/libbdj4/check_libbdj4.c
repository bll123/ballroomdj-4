/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <locale.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "check_bdj.h"
#include "mdebug.h"
#include "log.h"

void
check_libbdj4 (SRunner *sr)
{
  Suite   *s;

  /* libbdj4
   *  bdjvarsdf             complete // needed by tests
   *  dnctypes              complete
   *  dance                 complete
   *  genre                 complete
   *  level                 complete
   *  rating                complete
   *  songfav               complete
   *  songutil              complete
   *  status                complete
   *  tagdef                complete
   *  song                  complete
   *  musicdb               complete
   *  songlist              complete 2023-7-18
   *  autosel               complete
   *  songfilter            complete
   *  dancesel              complete
   *  sequence              complete 2023-7-18
   *  songsel
   *  playlist              complete 2023-7-21
   *      add-count, add-played, set-filter are not tested at this time.
   *  validate              complete
   *  audiotag
   *  sortopt               complete
   *  dispsel               complete
   *  orgutil               partial
   *  webclient             complete 2022-12-27
   *  songdb
   *  samesong              complete
   *  audioadjust
   *  templateutil          complete // needed by tests; needs localized tests
   *  bdjvarsdfload         complete // needed by tests; uses templateutil
   *  msgparse              complete 2022-12-27
   *  orgopt                complete
   *  volreg                complete 2022-12-27 (missing lock tests)
   *  m3u
   *  songlistutil
   *  itunes
   *  bdj4init
   *  instutil
   *  support
   *  mp3exp
   *  musicq
   *  expimpbdj4
   */

  logMsg (LOG_DBG, LOG_IMPORTANT, "==chk== libbdj4");

  s = bdjvarsdf_suite();
  srunner_add_suite (sr, s);

  s = dnctypes_suite();
  srunner_add_suite (sr, s);

  s = dance_suite();
  srunner_add_suite (sr, s);

  s = genre_suite();
  srunner_add_suite (sr, s);

  s = level_suite();
  srunner_add_suite (sr, s);

  s = rating_suite();
  srunner_add_suite (sr, s);

  s = songfav_suite();
  srunner_add_suite (sr, s);

  s = songutil_suite();
  srunner_add_suite (sr, s);

  s = status_suite();
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

  /* songsel */

  s = playlist_suite();
  srunner_add_suite (sr, s);

  s = validate_suite();
  srunner_add_suite (sr, s);

  /* audiotag */

  s = sortopt_suite();
  srunner_add_suite (sr, s);

  s = dispsel_suite();
  srunner_add_suite (sr, s);

  s = orgutil_suite();
  srunner_add_suite (sr, s);

  s = webclient_suite();
  srunner_add_suite (sr, s);

  /* songdb */

  s = samesong_suite();
  srunner_add_suite (sr, s);

  /* audioadjust */

  s = templateutil_suite();
  srunner_add_suite (sr, s);

  s = bdjvarsdfload_suite();
  srunner_add_suite (sr, s);

  s = msgparse_suite();
  srunner_add_suite (sr, s);

  s = orgopt_suite();
  srunner_add_suite (sr, s);

  s = volreg_suite();
  srunner_add_suite (sr, s);

  /* m3u */

  /* songlistutil */

  /* itunes */

  /* bdj4init */

  /* instutil */

  /* support */

  /* mp3exp */

  s = musicq_suite();
  srunner_add_suite (sr, s);

  /* expimpbdj4 */
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
