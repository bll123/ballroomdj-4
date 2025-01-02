/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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

#include "bdjstring.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "osenv.h"
#include "sysvars.h"

#define ENAME "ABC"
#define EDATA "def123"
#define WTEST "abcÜÄÑÖ내가 제일 잘 나가ははは夕陽伴我歸Ne_Русский_Шторм"

START_TEST(osenv_getenv)
{
  char  *str = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- osenv_getenv");
  mdebugSubTag ("osenv_getenv");

  str = getenv ("PATH");
  ck_assert_ptr_nonnull (str);

  str = getenv ("XYZZY");
  ck_assert_ptr_null (str);
}
END_TEST

START_TEST(osenv_setenv)
{
  char  *str = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- osenv_setenv");
  mdebugSubTag ("osenv_setenv");

  str = getenv (ENAME);
  ck_assert_ptr_null (str);

  osSetEnv (ENAME, EDATA);
  str = getenv (ENAME);
  ck_assert_ptr_nonnull (str);
  ck_assert_str_eq (str, EDATA);

  osSetEnv (ENAME, "");
  str = getenv (ENAME);
  ck_assert_ptr_null (str);

  osSetEnv (ENAME, EDATA);
  str = getenv (ENAME);
  ck_assert_ptr_nonnull (str);
  ck_assert_str_eq (str, EDATA);
}
END_TEST


Suite *
osenv_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("osenv");
  tc = tcase_create ("osenv");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, osenv_getenv);
  tcase_add_test (tc, osenv_setenv);
  suite_add_tcase (s, tc);

  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
