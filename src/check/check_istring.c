#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>

#pragma clang diagnostic push
#pragma gcc diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "check_bdj.h"
#include "istring.h"
#include "localeutil.h"
#include "osutils.h"
#include "sysvars.h"

/* note that this does not work within the C locale */
START_TEST(istring_istrlen)
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
  if (isWindows ()) {
    /* windows requires two characters to hold this glyph */
    /* and istrlen() on windows is not quite correct */
    ck_assert_int_eq (istrlen (buff), 6);
  } else {
    ck_assert_int_eq (istrlen (buff), 5);
  }

  strcpy (buff, "ab" "\xE2\x99\xa5" "cd");
  ck_assert_int_eq (strlen (buff), 7);
  ck_assert_int_eq (istrlen (buff), 5);

  strcpy (buff, "ab" "\xe2\x87\x92" "cd");
  ck_assert_int_eq (strlen (buff), 7);
  ck_assert_int_eq (istrlen (buff), 5);
}
END_TEST

START_TEST(istring_istr_comp)
{
  int     rc;

  /* at this time, I don't know how to get windows to change its sort */
  /* order. */

  setlocale (LC_ALL, "de_DE.UTF-8");

  rc = istringCompare ("ÄÄÄÄ", "ÖÖÖÖ");
  ck_assert_int_lt (rc, 0);

  rc = istringCompare ("ZZZZ", "ÖÖÖÖ");
  if (! isWindows()) {
    ck_assert_int_gt (rc, 0);
  }

  setlocale (LC_ALL, "sv_SE.UTF-8");

  rc = istringCompare ("ÄÄÄÄ", "ÖÖÖÖ");
  ck_assert_int_lt (rc, 0);

  rc = istringCompare ("ZZZZ", "ÖÖÖÖ");
  if (! isWindows()) {
    ck_assert_int_lt (rc, 0);
  }
}

Suite *
istring_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("istring");
  tc = tcase_create ("istring");
  tcase_add_test (tc, istring_istrlen);
  tcase_add_test (tc, istring_istr_comp);
  suite_add_tcase (s, tc);
  return s;
}

