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

#include "bdjstring.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "istring.h"
#include "log.h"
#include "sysvars.h"

/* note that to-lower will not work on localized strings */
START_TEST(bdjstring_string_to_lower)
{
  char  buff [20];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- bdjstring_string_to_lower");
  mdebugSubTag ("bdjstring_string_to_lower");

  strcpy (buff, "ABCD");
  stringAsciiToLower (buff);
  ck_assert_str_eq (buff, "abcd");
}
END_TEST

/* note that to-upper will not work on localized strings */
START_TEST(bdjstring_string_to_upper)
{
  char  buff [20];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- bdjstring_string_to_upper");
  mdebugSubTag ("bdjstring_string_to_upper");

  strcpy (buff, "abcd");
  stringAsciiToUpper (buff);
  ck_assert_str_eq (buff, "ABCD");
}
END_TEST

START_TEST(bdjstring_string_trim)
{
  char  buff [20];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- bdjstring_string_trim");
  mdebugSubTag ("bdjstring_string_trim");

  strcpy (buff, "abcd\n");
  stringTrim (buff);
  ck_assert_str_eq (buff, "abcd");

  strcpy (buff, "abcd\r");
  stringTrim (buff);
  ck_assert_str_eq (buff, "abcd");

  strcpy (buff, "abcd\r\n");
  stringTrim (buff);
  ck_assert_str_eq (buff, "abcd");

  strcpy (buff, "abcd\r\n\r\n");
  stringTrim (buff);
  ck_assert_str_eq (buff, "abcd");

  strcpy (buff, "abcd\r\r");
  stringTrim (buff);
  ck_assert_str_eq (buff, "abcd");

  strcpy (buff, "abcd\n\n");
  stringTrim (buff);
  ck_assert_str_eq (buff, "abcd");
}
END_TEST

START_TEST(bdjstring_string_trim_char)
{
  char  buff [20];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- bdjstring_string_trim_char");
  mdebugSubTag ("bdjstring_string_trim_char");

  strcpy (buff, "abcdz");
  stringTrimChar (buff, 'z');
  ck_assert_str_eq (buff, "abcd");

  strcpy (buff, "abcdzzzz");
  stringTrimChar (buff, 'z');
  ck_assert_str_eq (buff, "abcd");
}
END_TEST

#if 0
START_TEST(bdjstring_version_compare)
{
  int   rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- bdjstring_version_compare");
  mdebugSubTag ("bdjstring_version_compare");

  rc = versionCompare ("1", "1");
  ck_assert_int_eq (rc, 0);
  rc = versionCompare ("1", "2");
  ck_assert_int_eq (rc, -1);
  rc = versionCompare ("2", "1");
  ck_assert_int_eq (rc, 1);

  rc = versionCompare ("1.0", "1.0");
  ck_assert_int_eq (rc, 0);
  rc = versionCompare ("1.1", "1.2");
  ck_assert_int_eq (rc, -1);
  rc = versionCompare ("1.2", "1.1");
  ck_assert_int_eq (rc, 1);

  rc = versionCompare ("1.0.0", "1.0.0");
  ck_assert_int_eq (rc, 0);
  rc = versionCompare ("1.0.1", "1.0.2");
  ck_assert_int_eq (rc, -1);
  rc = versionCompare ("1.0.2", "1.0.1");
  ck_assert_int_eq (rc, 1);

  rc = versionCompare ("1.0", "1.0.0");
  ck_assert_int_eq (rc, 0);
  rc = versionCompare ("1.0", "1.0.1");
  ck_assert_int_eq (rc, -1);
  rc = versionCompare ("1.0.1", "1.0");
  ck_assert_int_eq (rc, 1);
  rc = versionCompare ("1.0.1.1", "1.0.1");
  ck_assert_int_eq (rc, 1);
  rc = versionCompare ("1.0.1a", "1.0a");
  ck_assert_int_eq (rc, 1);
}
END_TEST
#endif

Suite *
bdjstring_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("bdjstring");
  tc = tcase_create ("bdjstring");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, bdjstring_string_to_lower);
  tcase_add_test (tc, bdjstring_string_to_upper);
  tcase_add_test (tc, bdjstring_string_trim);
  tcase_add_test (tc, bdjstring_string_trim_char);
  // tcase_add_test (tc, bdjstring_version_compare);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
