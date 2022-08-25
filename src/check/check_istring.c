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

#include "bdjstring.h"
#include "check_bdj.h"
#include "istring.h"
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
  ck_assert_int_eq (istrlen (buff), 5);

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

  istringCleanup ();
  istringInit ("de_DE");

  rc = istringCompare ("ÄÄÄÄ", "ÖÖÖÖ");
  ck_assert_int_lt (rc, 0);

  rc = istringCompare ("ZZZZ", "ÖÖÖÖ");
  ck_assert_int_gt (rc, 0);

  istringCleanup ();
  istringInit ("sv_SE");

  rc = istringCompare ("ÄÄÄÄ", "ÖÖÖÖ");
  ck_assert_int_lt (rc, 0);

  rc = istringCompare ("ZZZZ", "ÖÖÖÖ");
  ck_assert_int_lt (rc, 0);
}
END_TEST

typedef struct {
  char  *u;
  char  *l;
} chk_istr_text_t;

static chk_istr_text_t tvalues [] = {
  { "ÄÄÄÄ", "ääää", },
  { "ÖÖÖÖ", "öööö", },
  { "ZZZZ", "zzzz" },
  { "내가제일잘나가", "내가제일잘나가" },
  { "ははは", "ははは" },
  { "夕陽伴我歸", "夕陽伴我歸" },
  { "Ne_Русский_Шторм", "Ne_Русский_Шторм" }
};
enum {
  tvaluesz = sizeof (tvalues) / sizeof (chk_istr_text_t),
};

START_TEST(istring_istr_tolower)
{
  int     rc;
  char    tbuff [200];

  for (int i = 0; i < tvaluesz; ++i) {
    strlcpy (tbuff, tvalues [i].u, sizeof (tbuff));
    istringToLower (tbuff);
    rc = strcmp (tbuff, tvalues [i].l);
    ck_assert_int_eq (rc, 0);
  }
}

START_TEST(istring_istr_toupper)
{
  int     rc;
  char    tbuff [200];

  for (int i = 0; i < tvaluesz; ++i) {
    strlcpy (tbuff, tvalues [i].l, sizeof (tbuff));
    istringToUpper (tbuff);
    rc = strcmp (tbuff, tvalues [i].u);
    ck_assert_int_eq (rc, 0);
  }
}


Suite *
istring_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("istring");
  tc = tcase_create ("istring");
  tcase_set_tags (tc, "libbasic");
  tcase_add_test (tc, istring_istrlen);
  tcase_add_test (tc, istring_istr_comp);
  tcase_add_test (tc, istring_istr_tolower);
  tcase_add_test (tc, istring_istr_toupper);
  suite_add_tcase (s, tc);
  return s;
}

