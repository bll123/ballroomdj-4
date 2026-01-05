/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "check_bdj.h"
#include "colorutils.h"
#include "log.h"
#include "mdebug.h"
#include "osrandom.h"

static void
setup (void)
{
  sRandom ();
}

START_TEST(colorutils_chk)
{
  char    c [200];
  char    cb [200];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- colorutils_chk");
  mdebugSubTag ("colorutils_chk");

  createRandomColor (c, sizeof (c));
  ck_assert_int_eq (strlen (c), 7);
  createRandomColor (cb, sizeof (cb));
  ck_assert_int_eq (strlen (cb), 7);
  ck_assert_str_ne (c, cb);
}
END_TEST


Suite *
colorutils_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("colorutils");
  tc = tcase_create ("colorutils");
  tcase_set_tags (tc, "libcommon");
  tcase_add_unchecked_fixture (tc, setup, NULL);
  tcase_add_test (tc, colorutils_chk);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
