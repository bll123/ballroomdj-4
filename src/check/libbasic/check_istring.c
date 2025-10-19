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
#include <locale.h>
#if __has_include (<uchar.h>)
# include <uchar.h>
#else
/* macos does not have uchar.h */
typedef uint_least16_t char16_t;
#endif

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
#include "osutils.h"
#include "sysvars.h"

START_TEST(istring_check)
{
  bool    rc;

  istringCleanup ();
  istringInit ("en_US");

  rc = istringCheck ();
  ck_assert_int_eq (rc, 1);

  istringCleanup ();
  istringInit (sysvarsGetStr (SV_LOCALE));
}
END_TEST

/* note that this does not work within the C locale */
START_TEST(istring_istrlen)
{
  char    buff [20];

  istringCleanup ();
  istringInit ("de_DE");

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- istring_istrlen");
  mdebugSubTag ("istring_istrlen");

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

  istringCleanup ();
  istringInit (sysvarsGetStr (SV_LOCALE));
}
END_TEST

START_TEST(istring_comp)
{
  int     rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- istring_comp");
  mdebugSubTag ("istring_comp");

  istringCleanup ();
  istringInit ("de_DE");

  /* the unicode routines do not handle nulls properly */
  /* istringCompare is modified to handle the nulls */
  rc = istringCompare ("aaaa", NULL);
  ck_assert_int_eq (rc, 1);

  rc = istringCompare (NULL, "aaaa");
  ck_assert_int_eq (rc, -1);

  rc = istringCompare (NULL, NULL);
  ck_assert_int_eq (rc, 0);

  /* de_DE */

  rc = istringCompare ("ÄÄÄÄ", "ÖÖÖÖ");
  ck_assert_int_lt (rc, 0);

  rc = istringCompare ("ZZZZ", "ÖÖÖÖ");
  /* note that this is greater than */
  ck_assert_int_gt (rc, 0);

  istringCleanup ();
  istringInit ("sv_SE");

  rc = istringCompare ("ÄÄÄÄ", "ÖÖÖÖ");
  ck_assert_int_lt (rc, 0);

  rc = istringCompare ("ZZZZ", "ÖÖÖÖ");
  /* note that this is less than */
  ck_assert_int_lt (rc, 0);

  istringCleanup ();
  istringInit (sysvarsGetStr (SV_LOCALE));
}
END_TEST

typedef struct {
  char  *u;
  char  *l;
} chk_text_t;

static chk_text_t tvalues [] = {
  { "ÄÄÄÄ", "ääää", },
  { "ÖÖÖÖ", "öööö", },
  { "ZZZZ", "zzzz" },
  { "내가제일잘나가", "내가제일잘나가" },
  { "ははは", "ははは" },
  { "夕陽伴我歸", "夕陽伴我歸" },
  { "NE_РУССКИЙ_ШТОРМ", "ne_русский_шторм" },
};
enum {
  tvaluesz = sizeof (tvalues) / sizeof (chk_text_t),
};

START_TEST(istring_tolower)
{
  int     rc;
  char    tbuff [200];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- istring_tolower");
  mdebugSubTag ("istring_tolower");

  istringCleanup ();
  istringInit ("de_DE");

  for (int i = 0; i < tvaluesz; ++i) {
    stpecpy (tbuff, tbuff + sizeof (tbuff), tvalues [i].u);
    istringToLower (tbuff);
    rc = strcmp (tbuff, tvalues [i].l);
    ck_assert_int_eq (rc, 0);
  }

  istringCleanup ();
  istringInit (sysvarsGetStr (SV_LOCALE));
}
END_TEST

START_TEST(istring_toutf8)
{
  char16_t  *inbuff;
  char      *tbuff;
  char      *tbuffb;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- istring_toutf8");
  mdebugSubTag ("istring_toutf8");

  tbuff = "내가제일잘나가";
  inbuff = u"내가제일잘나가";
  tbuffb = istring16ToUTF8 ((const unsigned char *) inbuff);
  ck_assert_ptr_nonnull (tbuffb);

  ck_assert_str_eq (tbuff, tbuffb);
  mdfree (tbuffb);
}
END_TEST

Suite *
istring_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("istring");
  tc = tcase_create ("istring");
  tcase_set_tags (tc, "libbasic");
  tcase_add_test (tc, istring_check);
  tcase_add_test (tc, istring_istrlen);
  tcase_add_test (tc, istring_comp);
  tcase_add_test (tc, istring_tolower);
  tcase_add_test (tc, istring_toutf8);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
