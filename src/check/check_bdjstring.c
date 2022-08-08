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
#include "check_bdj.h"

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

/* note that this does not work within the C locale */
START_TEST(bdjstring_istrlen)
{
  char    buff [20];

  strcpy (buff, "\xc2\xbf");
  ck_assert_int_eq (strlen (buff), 2);
  ck_assert_int_eq (istrlen (buff), 1);

  strcpy (buff, "ab" "\xc2\xbf" "cd");
  ck_assert_int_eq (strlen (buff), 6);
  ck_assert_int_eq (istrlen (buff), 5);

  strcpy (buff, "ab" "\xF0\x9F\x92\x94" "cd");
  ck_assert_int_eq (strlen (buff), 8);
  ck_assert_int_eq (istrlen (buff), 5);

  strcpy (buff, "ab" "\xE2\x99\xa5" "cd");
  ck_assert_int_eq (strlen (buff), 7);
  ck_assert_int_eq (istrlen (buff), 5);

  strcpy (buff, "ab" "\xe2\x87\x92" "cd");
  ck_assert_int_eq (strlen (buff), 7);
  ck_assert_int_eq (istrlen (buff), 5);
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

  s = suite_create ("string Suite");
  tc = tcase_create ("string");
  tcase_add_test (tc, bdjstring_string_to_lower);
  tcase_add_test (tc, bdjstring_string_to_upper);
  tcase_add_test (tc, bdjstring_string_trim);
  tcase_add_test (tc, bdjstring_string_trim_char);
  tcase_add_test (tc, bdjstring_istrlen);
  tcase_add_test (tc, bdjstring_version_compare);
  suite_add_tcase (s, tc);
  return s;
}

