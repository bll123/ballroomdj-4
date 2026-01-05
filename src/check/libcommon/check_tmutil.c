/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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

#include "log.h"
#include "tmutil.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "sysvars.h"

START_TEST(mstime_chk)
{
  time_t      tm_s;
  time_t      tm_chk;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mstime_chk");
  mdebugSubTag ("mstime_chk");

  tm_s = time (NULL);
  tm_s *= 1000;
  tm_chk = mstime ();
  /* the granularity of the time() call is high */
  ck_assert_int_lt (tm_chk - tm_s, 1005);
  tm_s = mstime ();
  tm_chk = mstime ();
  ck_assert_int_lt (tm_chk - tm_s, 10);
}
END_TEST

START_TEST(mssleep_sec)
{
  time_t      tm_s;
  time_t      tm_e;
  int         val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mssleep_sec");
  mdebugSubTag ("mssleep_sec");

  tm_s = mstime ();
  mssleep (2000);
  tm_e = mstime ();
  val = 30;
  /* no idea why windows is slow for this particular test */
  if (isWindows ()) {
    val += 60;
  }
  if (sysvarsGetNum (SVL_IS_VM)) {
    val += 1000;
  }
  ck_assert_int_lt (abs ((int) (tm_e - tm_s - 2000)), val);
}
END_TEST

START_TEST(mssleep_ms)
{
  time_t      tm_s;
  time_t      tm_e;
  int         val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mssleep_ms");
  mdebugSubTag ("mssleep_ms");

  tm_s = mstime ();
  mssleep (200);
  tm_e = mstime ();
  val = 30;
  /* running on a VM is slower */
  /* windows is quite bad on a VM */
  /* the opensuse VM is quite bad */
  if (sysvarsGetNum (SVL_IS_VM)) {
    val += 200;
  }
  ck_assert_int_lt (abs ((int) (tm_e - tm_s - 200)), val);
}
END_TEST

START_TEST(mssleep_ms_b)
{
  time_t      tm_s;
  time_t      tm_e;
  int         val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mssleep_ms");
  mdebugSubTag ("mssleep_ms");

  tm_s = mstime ();
  for (int i = 0; i < 10; ++i) {
    mssleep (40);
  }
  tm_e = mstime ();
  /* this works fine on linux */
  val = 20;
  /* macos is off quite a bit */
  if (isMacOS ()) {
    val = 110;
  }
  /* windows is currently unknown, this should be fixed someday */
  if (isWindows ()) {
    val = 100;
  }

  /* When running on a VM, this timer loop is way off */
  /* 500ms on a Linux VM, 650ms on a Windows VM */
  if (sysvarsGetNum (SVL_IS_VM)) {
    val += 700;
  }
  ck_assert_int_lt (abs ((int) (tm_e - tm_s - 400)), val);
}
END_TEST

START_TEST(mstimestartofday_chk)
{
  time_t    tm_s;
  time_t    tm_chk;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mstimestartofday_chk");
  mdebugSubTag ("mstimestartofday_chk");

  tm_s = mstime ();
  tm_chk = mstimestartofday ();
  /* this needs a test */
  ck_assert_int_ge (tm_s, tm_chk);
}
END_TEST

START_TEST(mstime_start_end)
{
  time_t      tm_s;
  time_t      tm_e;
  mstime_t    mi;
  time_t      m, d;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mstime_start_end");
  mdebugSubTag ("mstime_start_end");

  tm_s = mstime ();
  mstimestart (&mi);
  mssleep (2000);
  m = mstimeend (&mi);
  tm_e = mstime ();
  d = tm_e - tm_s;
  d -= m;
  if (d < 0) {
    d = - d;
  }
  ck_assert_int_lt (d, 30);
}
END_TEST

START_TEST(mstime_set)
{
  mstime_t    tmset;
  time_t      tm_s;
  time_t      tm_e;
  time_t      m, d;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mstime_set");
  mdebugSubTag ("mstime_set");

  tm_s = mstime ();
  mstimeset (&tmset, 2000);
  mssleep (2000);
  m = mstimeend (&tmset);
  tm_e = mstime ();
  d = tm_e - tm_s;
  d -= m;
  d -= 2000;
  if (d < 0) {
    d = - d;
  }
  ck_assert_int_lt (d, 30);
}
END_TEST

START_TEST(mstime_set_tm)
{
  mstime_t    tmset;
  time_t      m, s, d;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mstime_set_tm");
  mdebugSubTag ("mstime_set_tm");

  s = mstime ();
  /* sets tmset to the value supplied */
  mstimesettm (&tmset, 2000);
  m = mstimeend (&tmset);
  d = s - m - 2000;
  if (d < 0) {
    d = -d;
  }
  ck_assert_int_lt (d, 30);
}
END_TEST

START_TEST(mstime_check)
{
  mstime_t    tmset;
  bool        rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mstime_check");
  mdebugSubTag ("mstime_check");

  mstimeset (&tmset, 2000);
  rc = mstimeCheck (&tmset);
  ck_assert_int_eq (rc, false);
  mssleep (1000);
  rc = mstimeCheck (&tmset);
  ck_assert_int_eq (rc, false);
  mssleep (2000);
  rc = mstimeCheck (&tmset);
  ck_assert_int_eq (rc, true);
}
END_TEST

START_TEST(tmutildstamp_chk)
{
  char        buff [80];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- tmutildstamp_chk");
  mdebugSubTag ("tmutildstamp_chk");

  tmutilDstamp (buff, sizeof (buff));
  ck_assert_int_eq (strlen (buff), 10);
}
END_TEST

START_TEST(tmutildisp_chk)
{
  char        buff [80];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- tmutildisp_chk");
  mdebugSubTag ("tmutildisp_chk");

  tmutilDisp (buff, sizeof (buff), TM_CLOCK_ISO);
  ck_assert_int_ge (strlen (buff), 16);
  tmutilDisp (buff, sizeof (buff), TM_CLOCK_LOCAL);
  ck_assert_int_ge (strlen (buff), 16);
  tmutilDisp (buff, sizeof (buff), TM_CLOCK_TIME_12);
  ck_assert_int_ge (strlen (buff), 6);
  tmutilDisp (buff, sizeof (buff), TM_CLOCK_TIME_24);
  ck_assert_int_eq (strlen (buff), 5);
  tmutilDisp (buff, sizeof (buff), TM_CLOCK_OFF);
  ck_assert_int_eq (strlen (buff), 0);
}
END_TEST

START_TEST(tmutiltstamp_chk)
{
  char        buff [80];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- tmutiltstamp_chk");
  mdebugSubTag ("tmutiltstamp_chk");

  tmutilTstamp (buff, sizeof (buff));
  ck_assert_int_eq (strlen (buff), 12);
}
END_TEST

START_TEST(tmutilshorttstamp_chk)
{
  char        buff [80];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- tmutilshorttstamp_chk");
  mdebugSubTag ("tmutilshorttstamp_chk");

  tmutilShortTstamp (buff, sizeof (buff));
  ck_assert_int_eq (strlen (buff), 6);
}
END_TEST

START_TEST(tmutiltoms_chk)
{
  char        buff [80];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- tmutiltoms_chk");
  mdebugSubTag ("tmutiltoms_chk");

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

START_TEST(tmutiltomsd_chk)
{
  char        buff [80];
  char        tmp [80];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- tmutiltomsd_chk");
  mdebugSubTag ("tmutiltomsd_chk");

  tmutilToMSD (0, buff, sizeof (buff), 3);
  snprintf (tmp, sizeof (tmp), "0:00%s000", sysvarsGetStr (SV_LOCALE_RADIX));
  ck_assert_str_eq (buff, tmp);
  tmutilToMSD (59001, buff, sizeof (buff), 3);
  snprintf (tmp, sizeof (tmp), "0:59%s001", sysvarsGetStr (SV_LOCALE_RADIX));
  ck_assert_str_eq (buff, tmp);
  tmutilToMSD (60002, buff, sizeof (buff), 3);
  snprintf (tmp, sizeof (tmp), "1:00%s002", sysvarsGetStr (SV_LOCALE_RADIX));
  ck_assert_str_eq (buff, tmp);
  tmutilToMSD (119003, buff, sizeof (buff), 3);
  snprintf (tmp, sizeof (tmp), "1:59%s003", sysvarsGetStr (SV_LOCALE_RADIX));
  ck_assert_str_eq (buff, tmp);

  tmutilToMSD (0, buff, sizeof (buff), 1);
  snprintf (tmp, sizeof (tmp), "0:00%s0", sysvarsGetStr (SV_LOCALE_RADIX));
  ck_assert_str_eq (buff, tmp);
  tmutilToMSD (59100, buff, sizeof (buff), 1);
  snprintf (tmp, sizeof (tmp), "0:59%s1", sysvarsGetStr (SV_LOCALE_RADIX));
  ck_assert_str_eq (buff, tmp);
  tmutilToMSD (60200, buff, sizeof (buff), 1);
  snprintf (tmp, sizeof (tmp), "1:00%s2", sysvarsGetStr (SV_LOCALE_RADIX));
  ck_assert_str_eq (buff, tmp);
  tmutilToMSD (119300, buff, sizeof (buff), 1);
  snprintf (tmp, sizeof (tmp), "1:59%s3", sysvarsGetStr (SV_LOCALE_RADIX));
  ck_assert_str_eq (buff, tmp);
}
END_TEST

START_TEST(tmutiltodatehm_chk)
{
  char        buff [80];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- tmutiltodatehm_chk");
  mdebugSubTag ("tmutiltodatehm_chk");

  tmutilToDateHM (mstime(), buff, sizeof (buff));
  ck_assert_int_ge (strlen (buff), 14);
}
END_TEST

START_TEST(tmutil_strtoms)
{
  long  value;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- tmutil_strtoms");
  mdebugSubTag ("tmutil_strtoms");

  value = tmutilStrToMS ("0:00");
  ck_assert_int_eq (value, 0);
  value = tmutilStrToMS ("1:00");
  ck_assert_int_eq (value, 60000);
  value = tmutilStrToMS ("2:00");
  ck_assert_int_eq (value, 120000);
  value = tmutilStrToMS ("1:59");
  ck_assert_int_eq (value, 119000);
  value = tmutilStrToMS ("1:01.005");
  ck_assert_int_eq (value, 61005);
  value = tmutilStrToMS ("1:02,005");
  ck_assert_int_eq (value, 62005);

  value = tmutilStrToMS ("1:03.1");
  ck_assert_int_eq (value, 63100);
  value = tmutilStrToMS ("1:04,02");
  ck_assert_int_eq (value, 64020);

  value = tmutilStrToMS ("1:00:00");
  ck_assert_int_eq (value, 3600000);
  value = tmutilStrToMS ("2:00:00");
  ck_assert_int_eq (value, 7200000);
  value = tmutilStrToMS ("2:01:04.02");
  ck_assert_int_eq (value, 7264020);
}
END_TEST

START_TEST(tmutil_strtohm)
{
  long  value;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- tmutil_strtohm");
  mdebugSubTag ("tmutil_strtohm");

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

START_TEST(tmutil_strtoutc)
{
  const char  *str = "2023-01-03T18:34:58Z";
  time_t      val;
  time_t      a;

  val = tmutilStringToUTC (str, "%FT%TZ");
  a = 1672770898;
  if (val - a == 3600) {
    val -= 3600;
  }
  ck_assert_int_eq (val, a);

  str = "2016-4-26";
  val = tmutilStringToUTC (str, "%F");
  a = 1461672000;
  if (val - a == 3600) {
    val -= 3600;
  }
  ck_assert_int_eq (val, a);

  str = "2023-05-27 10:51:58";
  val = tmutilStringToUTC (str, "%F %T");
  a = 1685184718;
  if (val - a == 3600) {
    val -= 3600;
  }
  ck_assert_int_eq (val, a);

  str = "2025-04-23 17:00:00";
  val = tmutilStringToUTC (str, "%F %T");
  a = 1745427600;
  if (val - a == 3600) {
    val -= 3600;
  }
  ck_assert_int_eq (val, a);

  str = "Wed, 23 Apr 2025 17:00:00 -0700";
  val = tmutilStringToUTC (str, "%a, %d %h %Y %T %z");
  a = 1745427600;
  if (val - a == 3600) {
    val -= 3600;
  }
  ck_assert_int_eq (val, a);
}

Suite *
tmutil_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("tmutil");
  tc = tcase_create ("tmutil-base");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, mstime_chk);
  suite_add_tcase (s, tc);
  tc = tcase_create ("tmutil-timers");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, mssleep_sec);
  tcase_add_test (tc, mssleep_ms);
  tcase_add_test (tc, mssleep_ms_b);
  tcase_add_test (tc, mstimestartofday_chk);
  tcase_add_test (tc, mstime_start_end);
  tcase_add_test (tc, mstime_set);
  tcase_add_test (tc, mstime_set_tm);
  tcase_add_test (tc, mstime_check);
  suite_add_tcase (s, tc);

  tc = tcase_create ("tmutil-disp");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, tmutildstamp_chk);
  tcase_add_test (tc, tmutildisp_chk);
  tcase_add_test (tc, tmutiltstamp_chk);
  tcase_add_test (tc, tmutilshorttstamp_chk);
  tcase_add_test (tc, tmutiltoms_chk);
  tcase_add_test (tc, tmutiltomsd_chk);
  tcase_add_test (tc, tmutiltodatehm_chk);
  tcase_add_test (tc, tmutil_strtoms);
  tcase_add_test (tc, tmutil_strtohm);
  suite_add_tcase (s, tc);

  tc = tcase_create ("tmutil-conv");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, tmutil_strtoutc);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
