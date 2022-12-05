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
#pragma gcc diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "check_bdj.h"
#include "log.h"


void
check_libcommon (SRunner *sr)
{
  Suite   *s;

  /* libcommon:
   *  fileop      complete
   *  bdjstring   complete
   *  osprocess   complete    // uses procutil
   *  filedata    complete
   *  osnetutils
   *  pathutil    complete
   *  sysvars
   *  tmutil      complete
   *  osdir
   *  osutils
   *  dirop       complete
   *  filemanip   complete 2022-11-1
   *  fileshared                // open/write/close shared
   *  pathbld     complete
   *  log
   *  bdjmsg      complete
   *  sock        partial
   *  bdjvars     complete
   *  sockh
   *  conn
   *  osrandom    complete
   *  queue       complete 2022-11-1
   *  dirlist     complete
   *  colorutils  complete
   *  ossignal    complete
   */

  logMsg (LOG_DBG, LOG_IMPORTANT, "==chk== libcommon");

  s = bdjstring_suite();
  srunner_add_suite (sr, s);

  s = osprocess_suite();
  srunner_add_suite (sr, s);

  s = fileop_suite();
  srunner_add_suite (sr, s);

  s = filedata_suite();
  srunner_add_suite (sr, s);

  s = pathutil_suite();
  srunner_add_suite (sr, s);

  s = tmutil_suite();
  srunner_add_suite (sr, s);

  s = pathbld_suite();
  srunner_add_suite (sr, s);

  s = filemanip_suite();
  srunner_add_suite (sr, s);

  s = bdjmsg_suite();
  srunner_add_suite (sr, s);

  s = sock_suite();
  srunner_add_suite (sr, s);

  s = bdjvars_suite();
  srunner_add_suite (sr, s);

  s = osrandom_suite();
  srunner_add_suite (sr, s);

  s = queue_suite();
  srunner_add_suite (sr, s);

  s = dirop_suite();
  srunner_add_suite (sr, s);

  s = colorutils_suite();
  srunner_add_suite (sr, s);

  s = ossignal_suite();
  srunner_add_suite (sr, s);
}
