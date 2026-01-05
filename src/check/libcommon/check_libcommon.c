/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
check_libcommon (SRunner *sr)
{
  Suite   *s;

  /* libcommon:
   *  stpecpy     --
   *  tmutil      complete
   *  fileop      complete
   *  osenv       complete 2022-12-27
   *  bdjstring   complete
   *  osprocess   complete                // uses procutil, pathbld, ossignal
   *  osdirutil   complete 2024-5-21
   *  filedata    complete
   *  osnetutils  complete 2022-12-27
   *  pathutil    complete
   *  mdebug      complete
   *  sysvars
   *  osdir       complete 2022-12-27     // test uses dirop
   *  pathdisp    complete
   *  osutils     complete 2022-12-27
   *  pathbld     complete
   *  dirop       complete
   *  fileop_dir  complete 2023-7-31
   *  filemanip   complete 2022-11-1
   *  fileshared  complete 2025-2-8 // uses procutil, pathbld, ossignal
   *  pathinfo    complete
   *  log
   *  bdjmsg      complete
   *  sock        partial                 // uses ossignal
   *  bdjvars     complete
   *  sockh
   *  queue       complete 2022-11-1
   *  conn
   *  callback    complete 2023-3-4
   *  osrandom    complete
   *  bdjregex
   *  colorutils  complete
   *  vsencdec    complete
   *  oslocale
   *  ossignal    complete
   *  bdj4arg
   *  roman       complete 2024-11-20
   */

  logMsg (LOG_DBG, LOG_IMPORTANT, "==chk== libcommon");

  s = tmutil_suite();
  srunner_add_suite (sr, s);

  s = fileop_suite();
  srunner_add_suite (sr, s);

  s = osenv_suite();
  srunner_add_suite (sr, s);

  s = bdjstring_suite();
  srunner_add_suite (sr, s);

  s = osprocess_suite();
  srunner_add_suite (sr, s);

  s = osdirutil_suite();
  srunner_add_suite (sr, s);

  s = filedata_suite();
  srunner_add_suite (sr, s);

  s = osnetutils_suite();
  srunner_add_suite (sr, s);

  s = pathutil_suite();
  srunner_add_suite (sr, s);

  s = mdebug_suite();
  srunner_add_suite (sr, s);

  /* sysvars */

  s = osdir_suite();
  srunner_add_suite (sr, s);

  s = pathdisp_suite();
  srunner_add_suite (sr, s);

  s = osutils_suite();
  srunner_add_suite (sr, s);

  s = pathbld_suite();
  srunner_add_suite (sr, s);

  s = dirop_suite();
  srunner_add_suite (sr, s);

  s = fileop_dir_suite();
  srunner_add_suite (sr, s);

  s = filemanip_suite();
  srunner_add_suite (sr, s);

  s = fileshared_suite();
  srunner_add_suite (sr, s);

  s = pathinfo_suite();
  srunner_add_suite (sr, s);

  /* log */

  s = bdjmsg_suite();
  srunner_add_suite (sr, s);

  s = sock_suite();
  srunner_add_suite (sr, s);

  s = bdjvars_suite();
  srunner_add_suite (sr, s);

  /* sockh */

  s = queue_suite();
  srunner_add_suite (sr, s);

  /* conn */

  s = callback_suite();
  srunner_add_suite (sr, s);

  s = osrandom_suite();
  srunner_add_suite (sr, s);

  /* bdjregex */

  s = colorutils_suite();
  srunner_add_suite (sr, s);

  s = vsencdec_suite();
  srunner_add_suite (sr, s);

  /* oslocale */

  s = ossignal_suite();
  srunner_add_suite (sr, s);

  /* bdj4arg */

  s = roman_suite();
  srunner_add_suite (sr, s);
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
