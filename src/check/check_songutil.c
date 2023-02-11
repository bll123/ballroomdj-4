/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdjopt.h"
#include "check_bdj.h"
#include "log.h"
#include "mdebug.h"
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
    val = songutilFullFileName (tvalues [i].test);
    ck_assert_str_eq (val, tvalues [i].result);
    mdfree (val);
  }
  bdjoptCleanup ();
}
END_TEST


START_TEST(songutil_pos)
{
  ssize_t   pos;
  ssize_t   npos;
  ssize_t   rpos;
  int       speed;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songutil_pos");

  pos = 40000;
  speed = 100;
  rpos = songutilAdjustPosReal (pos, speed);
  ck_assert_int_eq (rpos, pos);
  npos = songutilNormalizePosition (rpos, speed);
  ck_assert_int_eq (rpos, pos);
  ck_assert_int_eq (npos, pos);

  pos = 40000;
  speed = 120;
  rpos = songutilAdjustPosReal (pos, speed);
  ck_assert_int_lt (rpos, pos);
  npos = songutilNormalizePosition (rpos, speed);
  ck_assert_int_eq (npos, pos);
  ck_assert_int_gt (npos, rpos);

  pos = 40000;
  speed = 70;
  rpos = songutilAdjustPosReal (pos, speed);
  ck_assert_int_gt (rpos, pos);
  npos = songutilNormalizePosition (rpos, speed);
  ck_assert_int_eq (npos, pos);
  ck_assert_int_lt (npos, rpos);
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
  tcase_add_test (tc, songutil_pos);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
