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
#include "nlist.h"
#include "songutil.h"
#include "tagdef.h"

static void
setup (void)
{
  bdjoptInit ();
}

static void
teardown (void)
{
  bdjoptCleanup ();
}

START_TEST(songutil_pos)
{
  ssize_t   pos;
  ssize_t   npos;
  ssize_t   rpos;
  int       speed;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songutil_pos");
  mdebugSubTag ("songutil_pos");

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

START_TEST(songutil_title)
{
  const char  *tval;
  nlist_t     *entry;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songutil_title");
  mdebugSubTag ("songutil_title");

  bdjoptSetNum (OPT_G_USE_WORK_MOVEMENT, true);

  entry = nlistAlloc ("chk-songutil-title", LIST_ORDERED, NULL);
  nlistSetStr (entry, TAG_TITLE, "none");
  nlistSetStr (entry, TAG_WORK, "work");
  nlistSetStr (entry, TAG_MOVEMENTNAME, "m");
  nlistSetNum (entry, TAG_MOVEMENTNUM, 1);
  songutilTitleFromWorkMovement (entry);
  tval = nlistGetStr (entry, TAG_TITLE);
  ck_assert_str_eq (tval, "work: I. m");
  nlistFree (entry);

  entry = nlistAlloc ("chk-songutil-title", LIST_ORDERED, NULL);
  nlistSetStr (entry, TAG_TITLE, "none");
  nlistSetStr (entry, TAG_WORK, "work");
  nlistSetNum (entry, TAG_MOVEMENTNUM, 1);
  songutilTitleFromWorkMovement (entry);
  tval = nlistGetStr (entry, TAG_TITLE);
  ck_assert_str_eq (tval, "work");
  nlistFree (entry);

  entry = nlistAlloc ("chk-songutil-title", LIST_ORDERED, NULL);
  nlistSetStr (entry, TAG_TITLE, "none");
  nlistSetNum (entry, TAG_MOVEMENTNUM, 1);
  nlistSetStr (entry, TAG_MOVEMENTNAME, "m");
  songutilTitleFromWorkMovement (entry);
  tval = nlistGetStr (entry, TAG_TITLE);
  ck_assert_str_eq (tval, "none");
  nlistFree (entry);

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
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, songutil_pos);
  tcase_add_test (tc, songutil_title);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
