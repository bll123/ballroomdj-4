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

#include "bdj4.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "log.h"
#include "osnetutils.h"

START_TEST(osnetutils_chk)
{
  char        tbuff [BDJ4_PATH_MAX];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- osnetutils_chk");
  mdebugSubTag ("osnetutils_chk");

  *tbuff = '\0';
  getHostname (tbuff, sizeof (tbuff));
  ck_assert_int_ge (strlen (tbuff), 3);
  ck_assert_int_ne (*tbuff, 0);
}
END_TEST

Suite *
osnetutils_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("osnetutils");
  tc = tcase_create ("osnetutils");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, osnetutils_chk);
  suite_add_tcase (s, tc);

  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
