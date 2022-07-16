#include "config.h"

#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <check.h>
#include <sys/types.h>
#include <time.h>

#include "tmutil.h"
#include "check_bdj.h"


START_TEST(msleep_chk)
{
  time_t      tm_s;
  time_t      tm_e;

  tm_s = time (NULL);
  mssleep (2000);
  tm_e = time (NULL);
  ck_assert_int_ge (tm_e - tm_s, 2);
}
END_TEST

START_TEST(tmutil_start_end)
{
  time_t      tm_s;
  time_t      tm_e;
  mstime_t    mi;
  time_t      m, d;

  tm_s = time (NULL);
  mstimestart (&mi);
  mssleep (2000);
  m = mstimeend (&mi);
  tm_e = time (NULL);
  d = (long) tm_e - (long) tm_s;
  if (d >= 3) {
    --d;
  }
  d *= 1000;
  d -= (long) m;
  if (d < 0) {
    d = - d;
  }
  ck_assert_int_lt (d, 30);
}
END_TEST

START_TEST(tmutil_set)
{
  mstime_t    tmstart;
  mstime_t    tmset;
  time_t      diffa;
  time_t      diffb;

  mstimeset (&tmset, 2000);
  mssleep (2000);
  mstimestart (&tmstart);
  diffa = mstimeend (&tmstart);
  diffb = mstimeend (&tmset);
  diffa -= diffb;
  if (diffa < 0) {
    diffa = - diffa;
  }
  ck_assert_int_lt (diffa, 50);
}
END_TEST

START_TEST(tmutil_check)
{
  mstime_t    tmset;
  bool        rc;

  mstimeset (&tmset, 2000);
  rc = mstimeCheck (&tmset);
  ck_assert_int_eq (rc, false);
  mssleep (1000);
  rc = mstimeCheck (&tmset);
  ck_assert_int_eq (rc, false);
  mssleep (1000);
  rc = mstimeCheck (&tmset);
  ck_assert_int_eq (rc, true);
}
END_TEST

START_TEST(tmutilDstamp_chk)
{
  char        buff [80];

  tmutilDstamp (buff, sizeof (buff));
  ck_assert_int_eq (strlen (buff), 10);
}
END_TEST

START_TEST(tmutilTstamp_chk)
{
  char        buff [80];

  tmutilTstamp (buff, sizeof (buff));
  ck_assert_int_eq (strlen (buff), 12);
}
END_TEST

START_TEST(tmutilToMS_chk)
{
  char        buff [80];

  tmutilToMS (0, buff, sizeof (buff));
  ck_assert_str_eq (buff, " 0:00");
  tmutilToMS (59000, buff, sizeof (buff));
  ck_assert_str_eq (buff, " 0:59");
  tmutilToMS (60000, buff, sizeof (buff));
  ck_assert_str_eq (buff, " 1:00");
  tmutilToMS (119000, buff, sizeof (buff));
  ck_assert_str_eq (buff, " 1:59");
}
END_TEST

START_TEST(tmutil_strtohm)
{
  long  value;

  value = tmutilStrToHM ("0:00");
  ck_assert_int_eq (value, 0);
  value = tmutilStrToHM ("12:00");
  ck_assert_int_eq (value, 720000);
  value = tmutilStrToHM ("12:00a");
  ck_assert_int_eq (value, 1440000);
  value = tmutilStrToHM ("12a");
  ck_assert_int_eq (value, 1440000);
  value = tmutilStrToHM ("1:45");
  ck_assert_int_eq (value, 105000);
  value = tmutilStrToHM ("12p");
  ck_assert_int_eq (value, 720000);
  value = tmutilStrToHM ("12:30p");
  ck_assert_int_eq (value, 750000);
  value = tmutilStrToHM ("1:30p");
  ck_assert_int_eq (value, 810000);
  value = tmutilStrToHM ("12:00p");
  ck_assert_int_eq (value, 720000);
}
END_TEST

Suite *
tmutil_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("Time Utils Suite");
  tc = tcase_create ("Time Utils");
  tcase_add_test (tc, msleep_chk);
  tcase_add_test (tc, tmutil_start_end);
  tcase_add_test (tc, tmutil_set);
  tcase_add_test (tc, tmutil_check);
  tcase_add_test (tc, tmutilDstamp_chk);
  tcase_add_test (tc, tmutilTstamp_chk);
  tcase_add_test (tc, tmutilToMS_chk);
  tcase_add_test (tc, tmutil_strtohm);
  suite_add_tcase (s, tc);
  return s;
}

