#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma gcc diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdjstring.h"
#include "istring.h"
#include "check_bdj.h"
#include "sysvars.h"

START_TEST(bdjstring_string_to_lower)
{
  char  buff [20];

  strcpy (buff, "ABCD");
  stringToLower (buff);
  ck_assert_str_eq (buff, "abcd");
}
END_TEST

START_TEST(bdjstring_string_to_upper)
{
  char  buff [20];

  strcpy (buff, "abcd");
  stringToUpper (buff);
  ck_assert_str_eq (buff, "ABCD");
}
END_TEST

START_TEST(bdjstring_string_trim)
{
  char  buff [20];

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

  strcpy (buff, "abcdz");
  stringTrimChar (buff, 'z');
  ck_assert_str_eq (buff, "abcd");

  strcpy (buff, "abcdzzzz");
  stringTrimChar (buff, 'z');
  ck_assert_str_eq (buff, "abcd");
}
END_TEST

START_TEST(bdjstring_version_compare)
{
  int   rc;

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

Suite *
bdjstring_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("bdjstring");
  tc = tcase_create ("bdjstring");
  tcase_add_test (tc, bdjstring_string_to_lower);
  tcase_add_test (tc, bdjstring_string_to_upper);
  tcase_add_test (tc, bdjstring_string_trim);
  tcase_add_test (tc, bdjstring_string_trim_char);
  tcase_add_test (tc, bdjstring_version_compare);
  suite_add_tcase (s, tc);
  return s;
}

