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

void
check_libcommon (SRunner *sr)
{
  Suite   *s;

  /* libcommon:
   *  osutils
   *  fileop      complete
   *  bdjstring   needs istringCompare (test w/de_DE, sv_SE)
   *  osprocess
   *  filedata    complete
   *  osnetutils
   *  pathutil    complete
   *  sysvars
   *  tmutil      complete
   *  osdir
   *  dirop       complete
   *  filemanip   need renameall test
   *  fileutil                        // open/write/close shared
   *  pathbld     complete
   *  log
   *  nlist       partial
   *  ilist       partial
   *  slist       partial
   *  bdjmsg      complete
   *  sock        partial
   *  datafile    partial
   *  bdjvars     complete
   *  sockh
   *  bdjopt
   *  conn
   *  procutil    partial
   *  lock        complete
   *  rafile      partial
   *  localeutil
   *  osrandom
   *  queue       complete
   *  dirlist     needs utf-8 basic/recursive
   *  colorutils
   *  ossignal
   *  progstate   complete (no log checks)
   */

  s = bdjstring_suite();
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

  s = nlist_suite();
  srunner_add_suite (sr, s);

  s = slist_suite();
  srunner_add_suite (sr, s);

  s = ilist_suite();
  srunner_add_suite (sr, s);

  s = bdjmsg_suite();
  srunner_add_suite (sr, s);

  s = sock_suite();
  srunner_add_suite (sr, s);

  s = datafile_suite();
  srunner_add_suite (sr, s);

  s = bdjvars_suite();
  srunner_add_suite (sr, s);

  s = procutil_suite();
  srunner_add_suite (sr, s);

  s = lock_suite();
  srunner_add_suite (sr, s);

  s = rafile_suite();
  srunner_add_suite (sr, s);

  s = queue_suite();
  srunner_add_suite (sr, s);

  s = dirop_suite();
  srunner_add_suite (sr, s);

  s = dirlist_suite();
  srunner_add_suite (sr, s);

  s = progstate_suite();
  srunner_add_suite (sr, s);
}
