/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
#include "validate.h"
#include "check_bdj.h"
#include "mdebug.h"


START_TEST(validate_empty)
{
  bool    val;
  char    tbuff [200];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- validate_empty");
  mdebugSubTag ("validate_empty");

  val = validate (tbuff, sizeof (tbuff), "empty", NULL, VAL_NOT_EMPTY);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "empty", "", VAL_NOT_EMPTY);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "empty", "stuff", VAL_NOT_EMPTY);
  ck_assert_int_eq (val, true);
}
END_TEST

START_TEST(validate_nospace)
{
  bool    val;
  char    tbuff [200];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- validate_nospace");
  mdebugSubTag ("validate_nospace");

  val = validate (tbuff, sizeof (tbuff), "nospace", "stuff", VAL_NO_SPACES);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "nospace", "more stuff", VAL_NO_SPACES);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "nospace", " stuff", VAL_NO_SPACES);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "nospace", "stuff ", VAL_NO_SPACES);
  ck_assert_int_eq (val, false);
}
END_TEST

START_TEST(validate_noslash)
{
  bool    val;
  char    tbuff [200];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- validate_noslash");
  mdebugSubTag ("validate_noslash");

  val = validate (tbuff, sizeof (tbuff), "noslash", "stuff", VAL_NO_SLASHES);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "noslash", "stuff/", VAL_NO_SLASHES);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "noslash", "stuff\\", VAL_NO_SLASHES);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "noslash", "st/uff", VAL_NO_SLASHES);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "noslash", "st\\uff", VAL_NO_SLASHES);
  ck_assert_int_eq (val, false);
}
END_TEST

START_TEST(validate_nowinchar)
{
  bool    val;
  char    tbuff [200];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- validate_nowinchar");
  mdebugSubTag ("validate_nowinchar");

  val = validate (tbuff, sizeof (tbuff), "nowinchar", "stuff", VAL_NO_WINCHARS);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "nowinchar", "stuff:", VAL_NO_WINCHARS);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "nowinchar", "stuff*", VAL_NO_WINCHARS);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "nowinchar", "stuff|", VAL_NO_WINCHARS);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "nowinchar", "stuff<", VAL_NO_WINCHARS);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "nowinchar", "stuff>", VAL_NO_WINCHARS);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "nowinchar", "stuff'", VAL_NO_WINCHARS);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "nowinchar", "stuff\"", VAL_NO_WINCHARS);
  ck_assert_int_eq (val, false);
}
END_TEST

START_TEST(validate_numeric)
{
  bool    val;
  char    tbuff [200];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- validate_numeric");
  mdebugSubTag ("validate_numeric");

  val = validate (tbuff, sizeof (tbuff), "num", "1234", VAL_NUMERIC);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "num", "1234x", VAL_NUMERIC);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "num", "  1234", VAL_NUMERIC);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "num", "x1234", VAL_NUMERIC);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "num", "1x234", VAL_NUMERIC);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "num", "1234 ", VAL_NUMERIC);
  ck_assert_int_eq (val, true);
}
END_TEST

START_TEST(validate_float)
{
  bool    val;
  char    tbuff [200];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- validate_float");
  mdebugSubTag ("validate_float");

  val = validate (tbuff, sizeof (tbuff), "float", "1234", VAL_FLOAT);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "float", "123.4", VAL_FLOAT);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "float", "1.234", VAL_FLOAT);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "float", ".234", VAL_FLOAT);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "float", "1.", VAL_FLOAT);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "float", "0.1", VAL_FLOAT);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "float", "1.0", VAL_FLOAT);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "float", " 1.0", VAL_FLOAT);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "float", "1.0 ", VAL_FLOAT);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "float", "1.0x", VAL_FLOAT);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "float", "1.0.3", VAL_FLOAT);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "float", "x1.0", VAL_FLOAT);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "float", "1.x0", VAL_FLOAT);
  ck_assert_int_eq (val, false);
}
END_TEST

START_TEST(validate_hour_min)
{
  bool    val;
  char    tbuff [200];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- validate_hour_min");
  mdebugSubTag ("validate_hour_min");

  val = validate (tbuff, sizeof (tbuff), "hourmin", "1:00", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "1:29", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "1:59", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "12:34", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "120:34", VAL_HOUR_MIN);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "1:62", VAL_HOUR_MIN);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "24:00", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "0:00", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "12:00am", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "12:00a.m.", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "12:00pm", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "12:00p.m.", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "12:00p", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "12:00a", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "12:00a", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "x24:00", VAL_HOUR_MIN);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "34:00", VAL_HOUR_MIN);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "25:00", VAL_HOUR_MIN);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "12:00x", VAL_HOUR_MIN);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "12:00m", VAL_HOUR_MIN);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "12:00AM", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "12:00PM", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "12:00P", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "12:00A", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "12am", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "12a", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "12 am", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hourmin", "12 a", VAL_HOUR_MIN);
  ck_assert_int_eq (val, true);
}
END_TEST

START_TEST(validate_min_sec)
{
  bool    val;
  char    tbuff [200];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- validate_min_sec");
  mdebugSubTag ("validate_min_sec");

  val = validate (tbuff, sizeof (tbuff), "minsec", "1:00", VAL_MIN_SEC);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "minsec", "1:29", VAL_MIN_SEC);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "minsec", "1:59", VAL_MIN_SEC);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "minsec", "12:34", VAL_MIN_SEC);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "minsec", "120:34", VAL_MIN_SEC);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "minsec", "1:62", VAL_MIN_SEC);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "minsec", "24:00", VAL_MIN_SEC);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "minsec", "0:00", VAL_MIN_SEC);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "minsec", "x24:00", VAL_MIN_SEC);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "minsec", "12:00x", VAL_MIN_SEC);
  ck_assert_int_eq (val, false);
}
END_TEST


START_TEST(validate_hms)
{
  bool    val;
  char    tbuff [200];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- validate_hms");
  mdebugSubTag ("validate_hms");

  val = validate (tbuff, sizeof (tbuff), "hms", "1:00", VAL_HMS);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms", "1:29", VAL_HMS);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms", "1:59", VAL_HMS);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms", "12:34", VAL_HMS);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms", "120:34", VAL_HMS);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "hms", "1:62", VAL_HMS);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "hms", "24:00", VAL_HMS);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms", "0:00", VAL_HMS);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms", "x24:00", VAL_HMS);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "hms", "34:00", VAL_HMS);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms", "25:00", VAL_HMS);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms", "12:00x", VAL_HMS);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "hms", "12:00m", VAL_HMS);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "hms", "1:12:00", VAL_HMS);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms", "2:59:59", VAL_HMS);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms", "2:60:59", VAL_HMS);
  ck_assert_int_eq (val, false);
}
END_TEST

START_TEST(validate_hms_precise)
{
  bool    val;
  char    tbuff [200];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- validate_hms_precise");
  mdebugSubTag ("validate_hms_precise");

  val = validate (tbuff, sizeof (tbuff), "hms_p", "1:00", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms_p", "1:29", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms_p", "1:59", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms_p", "12:34", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms_p", "120:34", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "hms_p", "1:62", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "hms_p", "24:00", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms_p", "0:00", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms_p", "x24:00", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "hms_p", "34:00", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms_p", "25:00", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms_p", "12:00x", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "hms_p", "12:00m", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "hms_p", "1:12:00", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms_p", "2:59:59", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms_p", "2:60:59", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "hms_p", "2.59.59", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms_p", "2.60.59", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "hms_p", "12:20.1", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms_p", "12:20.01", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "hms_p", "12:20.65", VAL_HMS_PRECISE);
  ck_assert_int_eq (val, true);
}
END_TEST

START_TEST(validate_base_uri)
{
  bool    val;
  char    tbuff [200];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- validate_base_uri");
  mdebugSubTag ("validate_base_uri");

  val = validate (tbuff, sizeof (tbuff), "baseuri", "stuff", VAL_BASE_URI);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "baseuri", "stuff.", VAL_BASE_URI);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "baseuri", "a.com", VAL_BASE_URI);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "baseuri", "bll-mac.local", VAL_BASE_URI);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "baseuri", "www.a.com", VAL_BASE_URI);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "baseuri", "www..a.com", VAL_BASE_URI);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "baseuri", "www a.com", VAL_BASE_URI);
  ck_assert_int_eq (val, false);
}
END_TEST

START_TEST(validate_full_uri)
{
  bool    val;
  char    tbuff [200];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- validate_full_uri");
  mdebugSubTag ("validate_full_uri");

  val = validate (tbuff, sizeof (tbuff), "fulluri", "stuff", VAL_FULL_URI);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "fulluri", "stuff.", VAL_FULL_URI);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "fulluri", "http://a.com", VAL_FULL_URI);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "fulluri", "https://bll-mac.local", VAL_FULL_URI);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "fulluri", "https://www.a.com", VAL_FULL_URI);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "fulluri", "https://www..a.com", VAL_FULL_URI);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "fulluri", "https://www a.com", VAL_FULL_URI);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "fulluri", "http://a.com/stuff", VAL_FULL_URI);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "fulluri", "http://a.com/", VAL_FULL_URI);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "fulluri", " http://a.com/", VAL_FULL_URI);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "fulluri", "http://a.com/a z", VAL_FULL_URI);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "fulluri", "http://a.com:9011/stuff", VAL_FULL_URI);
  ck_assert_int_eq (val, true);
  val = validate (tbuff, sizeof (tbuff), "fulluri", "https://", VAL_FULL_URI);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "fulluri", "https://a", VAL_FULL_URI);
  ck_assert_int_eq (val, false);
  val = validate (tbuff, sizeof (tbuff), "fulluri", "https", VAL_FULL_URI);
  ck_assert_int_eq (val, false);
}
END_TEST

Suite *
validate_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("validate");
  tc = tcase_create ("validate");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_test (tc, validate_empty);
  tcase_add_test (tc, validate_nospace);
  tcase_add_test (tc, validate_noslash);
  tcase_add_test (tc, validate_nowinchar);
  tcase_add_test (tc, validate_numeric);
  tcase_add_test (tc, validate_float);
  tcase_add_test (tc, validate_hour_min);
  tcase_add_test (tc, validate_min_sec);
  tcase_add_test (tc, validate_hms);
  tcase_add_test (tc, validate_hms_precise);
  tcase_add_test (tc, validate_base_uri);
  tcase_add_test (tc, validate_full_uri);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
