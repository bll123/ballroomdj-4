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
#include "log.h"

void
check_libbasic (SRunner *sr)
{
  Suite   *s;

  /*
   * libbasic:
   *  istring     complete
   *  list
   *  nlist       partial
   *  ilist       partial
   *  slist       partial
   *  datafile    partial
   *  bdjopt      complete 2023-7-18
   *  procutil    partial
   *  lock        complete
   *  localeutil
   *  rafile      complete
   *  dirlist     complete
   *  progstate   complete (no log checks)
   */

  logMsg (LOG_DBG, LOG_IMPORTANT, "==chk== libbasic");

  s = istring_suite();
  srunner_add_suite (sr, s);

  /* list */

  s = nlist_suite();
  srunner_add_suite (sr, s);

  s = slist_suite();
  srunner_add_suite (sr, s);

  s = ilist_suite();
  srunner_add_suite (sr, s);

  s = datafile_suite();
  srunner_add_suite (sr, s);

  s = bdjopt_suite();
  srunner_add_suite (sr, s);

  s = procutil_suite();
  srunner_add_suite (sr, s);

  s = lock_suite();
  srunner_add_suite (sr, s);

  /* localeutil */

  s = rafile_suite();
  srunner_add_suite (sr, s);

  s = dirlist_suite();
  srunner_add_suite (sr, s);

  s = progstate_suite();
  srunner_add_suite (sr, s);
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
