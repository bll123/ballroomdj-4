#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>

#pragma clang diagnostic push
#pragma gcc diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdjopt.h"
#include "check_bdj.h"
#include "log.h"
#include "songutil.h"

typedef struct {
  char  *test;
  char  *result;
} chk_su_t;

static chk_su_t tvalues [] = {
  { "abc123", "/testpath/abc123" },
  { "/stuff", "/stuff" },
  { "C:/there", "C:/there" },
  { "d:/here", "d:/here" }
};
enum {
  tvaluesz = sizeof (tvalues) / sizeof (chk_su_t),
};

START_TEST(songutil_chk)
{
  char  *val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songutil_chk");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "/testpath");
  for (int i = 0; i < tvaluesz; ++i) {
    val = songFullFileName (tvalues [i].test);
    ck_assert_str_eq (val, tvalues [i].result);
    free (val);
  }
  bdjoptCleanup ();
}
END_TEST


Suite *
songutil_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("songutil");
  tc = tcase_create ("songutil");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_test (tc, songutil_chk);
  suite_add_tcase (s, tc);
  return s;
}
