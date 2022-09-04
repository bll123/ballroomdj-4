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
check_libbasic (SRunner *sr)
{
  Suite   *s;

  /* libbasic:
   *  istring     complete
   *  list
   *  nlist       partial
   *  ilist       partial
   *  slist       partial
   *  datafile    partial
   *  bdjopt
   *  procutil    partial
   *  lock        complete
   *  rafile      complete
   *  localeutil
   *  dirlist     complete
   *  progstate   complete (no log checks)
   */

  logMsg (LOG_DBG, LOG_IMPORTANT, "==chk== libbasic");

  s = istring_suite();
  srunner_add_suite (sr, s);

  s = nlist_suite();
  srunner_add_suite (sr, s);

  s = slist_suite();
  srunner_add_suite (sr, s);

  s = ilist_suite();
  srunner_add_suite (sr, s);

  s = datafile_suite();
  srunner_add_suite (sr, s);

  s = procutil_suite();
  srunner_add_suite (sr, s);

  s = lock_suite();
  srunner_add_suite (sr, s);

  s = rafile_suite();
  srunner_add_suite (sr, s);

  s = dirlist_suite();
  srunner_add_suite (sr, s);

  s = progstate_suite();
  srunner_add_suite (sr, s);
}
