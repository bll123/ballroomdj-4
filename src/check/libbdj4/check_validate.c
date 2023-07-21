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

#include "log.h"
#include "validate.h"
#include "check_bdj.h"
#include "mdebug.h"


START_TEST(validate_empty)
{
  const char *valstr;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- validate_empty");
  mdebugSubTag ("validate_empty");

  valstr = validate (NULL, VAL_NOT_EMPTY);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("", VAL_NOT_EMPTY);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("stuff", VAL_NOT_EMPTY);
  ck_assert_ptr_null (valstr);
}
END_TEST

START_TEST(validate_nospace)
{
  const char *valstr;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- validate_nospace");
  mdebugSubTag ("validate_nospace");

  valstr = validate ("stuff", VAL_NO_SPACES);
  ck_assert_ptr_null (valstr);
  valstr = validate ("more stuff", VAL_NO_SPACES);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate (" stuff", VAL_NO_SPACES);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("stuff ", VAL_NO_SPACES);
  ck_assert_ptr_nonnull (valstr);
}
END_TEST

START_TEST(validate_noslash)
{
  const char *valstr;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- validate_noslash");
  mdebugSubTag ("validate_noslash");

  valstr = validate ("stuff", VAL_NO_SLASHES);
  ck_assert_ptr_null (valstr);
  valstr = validate ("stuff/", VAL_NO_SLASHES);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("stuff\\", VAL_NO_SLASHES);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("st/uff", VAL_NO_SLASHES);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("st\\uff", VAL_NO_SLASHES);
  ck_assert_ptr_nonnull (valstr);
}
END_TEST

START_TEST(validate_numeric)
{
  const char *valstr;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- validate_numeric");
  mdebugSubTag ("validate_numeric");

  valstr = validate ("1234", VAL_NUMERIC);
  ck_assert_ptr_null (valstr);
  valstr = validate ("1234x", VAL_NUMERIC);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("  1234", VAL_NUMERIC);
  ck_assert_ptr_null (valstr);
  valstr = validate ("x1234", VAL_NUMERIC);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("1x234", VAL_NUMERIC);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("1234 ", VAL_NUMERIC);
  ck_assert_ptr_null (valstr);
}
END_TEST

START_TEST(validate_float)
{
  const char *valstr;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- validate_float");
  mdebugSubTag ("validate_float");

  valstr = validate ("1234", VAL_FLOAT);
  ck_assert_ptr_null (valstr);
  valstr = validate ("123.4", VAL_FLOAT);
  ck_assert_ptr_null (valstr);
  valstr = validate ("1.234", VAL_FLOAT);
  ck_assert_ptr_null (valstr);
  valstr = validate (".234", VAL_FLOAT);
  ck_assert_ptr_null (valstr);
  valstr = validate ("1.", VAL_FLOAT);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("0.1", VAL_FLOAT);
  ck_assert_ptr_null (valstr);
  valstr = validate ("1.0", VAL_FLOAT);
  ck_assert_ptr_null (valstr);
  valstr = validate (" 1.0", VAL_FLOAT);
  ck_assert_ptr_null (valstr);
  valstr = validate ("1.0 ", VAL_FLOAT);
  ck_assert_ptr_null (valstr);
  valstr = validate ("1.0x", VAL_FLOAT);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("1.0.3", VAL_FLOAT);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("x1.0", VAL_FLOAT);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("1.x0", VAL_FLOAT);
  ck_assert_ptr_nonnull (valstr);
}
END_TEST

START_TEST(validate_hour_min)
{
  const char *valstr;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- validate_hour_min");
  mdebugSubTag ("validate_hour_min");

  valstr = validate ("1:00", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
  valstr = validate ("1:29", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
  valstr = validate ("1:59", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
  valstr = validate ("12:34", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
  valstr = validate ("120:34", VAL_HOUR_MIN);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("1:62", VAL_HOUR_MIN);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("24:00", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
  valstr = validate ("0:00", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
  valstr = validate ("12:00am", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
  valstr = validate ("12:00a.m.", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
  valstr = validate ("12:00pm", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
  valstr = validate ("12:00p.m.", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
  valstr = validate ("12:00p", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
  valstr = validate ("12:00a", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
  valstr = validate ("12:00a", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
  valstr = validate ("x24:00", VAL_HOUR_MIN);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("34:00", VAL_HOUR_MIN);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("25:00", VAL_HOUR_MIN);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("12:00x", VAL_HOUR_MIN);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("12:00m", VAL_HOUR_MIN);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("12:00AM", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
  valstr = validate ("12:00PM", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
  valstr = validate ("12:00P", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
  valstr = validate ("12:00A", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
  valstr = validate ("12am", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
  valstr = validate ("12a", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
  valstr = validate ("12 am", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
  valstr = validate ("12 a", VAL_HOUR_MIN);
  ck_assert_ptr_null (valstr);
}
END_TEST

START_TEST(validate_min_sec)
{
  const char *valstr;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- validate_min_sec");
  mdebugSubTag ("validate_min_sec");

  valstr = validate ("1:00", VAL_MIN_SEC);
  ck_assert_ptr_null (valstr);
  valstr = validate ("1:29", VAL_MIN_SEC);
  ck_assert_ptr_null (valstr);
  valstr = validate ("1:59", VAL_MIN_SEC);
  ck_assert_ptr_null (valstr);
  valstr = validate ("12:34", VAL_MIN_SEC);
  ck_assert_ptr_null (valstr);
  valstr = validate ("120:34", VAL_MIN_SEC);
  ck_assert_ptr_null (valstr);
  valstr = validate ("1:62", VAL_MIN_SEC);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("24:00", VAL_MIN_SEC);
  ck_assert_ptr_null (valstr);
  valstr = validate ("0:00", VAL_MIN_SEC);
  ck_assert_ptr_null (valstr);
  valstr = validate ("x24:00", VAL_MIN_SEC);
  ck_assert_ptr_nonnull (valstr);
  valstr = validate ("12:00x", VAL_MIN_SEC);
  ck_assert_ptr_nonnull (valstr);
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
  tcase_add_test (tc, validate_numeric);
  tcase_add_test (tc, validate_float);
  tcase_add_test (tc, validate_hour_min);
  tcase_add_test (tc, validate_min_sec);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
