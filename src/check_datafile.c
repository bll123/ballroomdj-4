#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "datafile.h"
#include "check_bdj.h"

START_TEST(parse_init_free)
{
  parseinfo_t     *pi;

  pi = parseInit ();
  ck_assert_ptr_nonnull (pi);
  ck_assert_int_eq (pi->allocCount, 0);
  ck_assert_ptr_null (pi->strdata);
  parseFree (pi);
}
END_TEST

START_TEST(parse_basic)
{
  parseinfo_t     *pi;

  char *tstr = strdup ("A\n..a\nB\n..b\nC\n..c\nD\n..d\nE\n..e\nF\n..f");
  pi = parseInit ();
  ck_assert_ptr_nonnull (pi);
  size_t count = parseKeyValue (pi, tstr);
  ck_assert_int_eq (count, 12);
  ck_assert_int_ge (pi->allocCount, 12);
  char **strdata = parseGetData (pi);
  ck_assert_ptr_eq (pi->strdata, strdata);
  ck_assert_str_eq (strdata[0], "A");
  ck_assert_str_eq (strdata[1], "a");
  ck_assert_str_eq (strdata[2], "B");
  ck_assert_str_eq (strdata[3], "b");
  ck_assert_str_eq (strdata[4], "C");
  ck_assert_str_eq (strdata[5], "c");
  ck_assert_str_eq (strdata[6], "D");
  ck_assert_str_eq (strdata[7], "d");
  ck_assert_str_eq (strdata[8], "E");
  ck_assert_str_eq (strdata[9], "e");
  ck_assert_str_eq (strdata[10], "F");
  ck_assert_str_eq (strdata[11], "f");
  parseFree (pi);
  free (tstr);
}
END_TEST

START_TEST(parse_with_comments)
{
  parseinfo_t     *pi;

  char *tstr = strdup ("# comment\nA\n..a\n# comment\nB\n..b\nC\n..c\nD\n# comment\n..d\nE\n..e\nF\n..f");
  pi = parseInit ();
  ck_assert_ptr_nonnull (pi);
  size_t count = parseKeyValue (pi, tstr);
  ck_assert_int_eq (count, 12);
  ck_assert_int_ge (pi->allocCount, 12);
  char **strdata = parseGetData (pi);
  ck_assert_ptr_eq (pi->strdata, strdata);
  ck_assert_str_eq (strdata[0], "A");
  ck_assert_str_eq (strdata[1], "a");
  ck_assert_str_eq (strdata[2], "B");
  ck_assert_str_eq (strdata[3], "b");
  ck_assert_str_eq (strdata[4], "C");
  ck_assert_str_eq (strdata[5], "c");
  ck_assert_str_eq (strdata[6], "D");
  ck_assert_str_eq (strdata[7], "d");
  ck_assert_str_eq (strdata[8], "E");
  ck_assert_str_eq (strdata[9], "e");
  ck_assert_str_eq (strdata[10], "F");
  ck_assert_str_eq (strdata[11], "f");
  parseFree (pi);
  free (tstr);
}
END_TEST

START_TEST(parse_simple)
{
  parseinfo_t     *pi;

  char *tstr = strdup ("# comment\nA\na\n# comment\nB\nb\nC\nc\nD\n# comment\nd\nE\ne\nF\nf");
  pi = parseInit ();
  ck_assert_ptr_nonnull (pi);
  size_t count = parseSimple (pi, tstr);
  ck_assert_int_eq (count, 12);
  ck_assert_int_ge (pi->allocCount, 12);
  char **strdata = parseGetData (pi);
  ck_assert_ptr_eq (pi->strdata, strdata);
  ck_assert_str_eq (strdata[0], "A");
  ck_assert_str_eq (strdata[1], "a");
  ck_assert_str_eq (strdata[2], "B");
  ck_assert_str_eq (strdata[3], "b");
  ck_assert_str_eq (strdata[4], "C");
  ck_assert_str_eq (strdata[5], "c");
  ck_assert_str_eq (strdata[6], "D");
  ck_assert_str_eq (strdata[7], "d");
  ck_assert_str_eq (strdata[8], "E");
  ck_assert_str_eq (strdata[9], "e");
  ck_assert_str_eq (strdata[10], "F");
  ck_assert_str_eq (strdata[11], "f");
  parseFree (pi);
  free (tstr);
}
END_TEST

Suite *
datafile_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("Datafile Suite");
  tc = tcase_create ("Datafile");
  tcase_add_test (tc, parse_init_free);
  tcase_add_test (tc, parse_basic);
  tcase_add_test (tc, parse_with_comments);
  tcase_add_test (tc, parse_simple);
  suite_add_tcase (s, tc);
  return s;
}

