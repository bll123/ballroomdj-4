/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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
#include "log.h"
#include "osrandom.h"

START_TEST(osrandom_chk)
{
  double    dval, dvalb;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- osrandom_chk");

  sRandom ();
  dval = dRandom ();
  dvalb = dRandom ();
  ck_assert_float_ne (dval, 1.0);
  ck_assert_float_ne (dvalb, 1.0);
  ck_assert_float_ne (dval, dvalb);
}
END_TEST


Suite *
osrandom_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("osrandom");
  tc = tcase_create ("osrandom");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, osrandom_chk);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
