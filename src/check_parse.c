#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "parse.h"
#include "check_bdj.h"

START_TEST(parse_init_free)
{
  parseinfo_t     *pi;

  pi = parseInit ();
  ck_assert (pi != NULL);
  ck_assert (pi->allocCount == 0);
  ck_assert (pi->strdata == NULL);
  parseFree (pi);
}
END_TEST

START_TEST(parse_basic)
{
  parseinfo_t     *pi;

  char *tstr = strdup ("A\n..a\nB\n..b\nC\n..c\nD\n..d\nE\n..e\nF\n..f");
  pi = parseInit ();
  ck_assert (pi != NULL);
  int count = parse (pi, tstr);
  ck_assert (count == 12);
  ck_assert (pi->allocCount >= 12);
  char **strdata = parseGetData (pi);
  ck_assert (pi->strdata == strdata);
  ck_assert (strcmp (strdata[0], "A") == 0);
  ck_assert (strcmp (strdata[1], "a") == 0);
  ck_assert (strcmp (strdata[2], "B") == 0);
  ck_assert (strcmp (strdata[3], "b") == 0);
  ck_assert (strcmp (strdata[4], "C") == 0);
  ck_assert (strcmp (strdata[5], "c") == 0);
  ck_assert (strcmp (strdata[6], "D") == 0);
  ck_assert (strcmp (strdata[7], "d") == 0);
  ck_assert (strcmp (strdata[8], "E") == 0);
  ck_assert (strcmp (strdata[9], "e") == 0);
  ck_assert (strcmp (strdata[10], "F") == 0);
  ck_assert (strcmp (strdata[11], "f") == 0);
  parseFree (pi);
  free (tstr);
}
END_TEST

START_TEST(parse_with_comments)
{
  parseinfo_t     *pi;

  char *tstr = strdup ("# comment\nA\n..a\n# comment\nB\n..b\nC\n..c\nD\n# comment\n..d\nE\n..e\nF\n..f");
  pi = parseInit ();
  ck_assert (pi != NULL);
  int count = parse (pi, tstr);
  ck_assert (count == 12);
  ck_assert (pi->allocCount >= 12);
  char **strdata = parseGetData (pi);
  ck_assert (pi->strdata == strdata);
  ck_assert (strcmp (strdata[0], "A") == 0);
  ck_assert (strcmp (strdata[1], "a") == 0);
  ck_assert (strcmp (strdata[2], "B") == 0);
  ck_assert (strcmp (strdata[3], "b") == 0);
  ck_assert (strcmp (strdata[4], "C") == 0);
  ck_assert (strcmp (strdata[5], "c") == 0);
  ck_assert (strcmp (strdata[6], "D") == 0);
  ck_assert (strcmp (strdata[7], "d") == 0);
  ck_assert (strcmp (strdata[8], "E") == 0);
  ck_assert (strcmp (strdata[9], "e") == 0);
  ck_assert (strcmp (strdata[10], "F") == 0);
  ck_assert (strcmp (strdata[11], "f") == 0);
  parseFree (pi);
  free (tstr);
}
END_TEST


Suite *
parse_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("Parse Suite");
  tc = tcase_create ("Parse");
  tcase_add_test (tc, parse_init_free);
  tcase_add_test (tc, parse_basic);
  tcase_add_test (tc, parse_with_comments);
  suite_add_tcase (s, tc);
  return s;
}

