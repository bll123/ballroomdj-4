/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#pragma clang diagnostic push
#pragma gcc diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#define BDJ4_MEM_DEBUG 1

#include "check_bdj.h"
#include "mdebug.h"

START_TEST(mdebug_valid)
{
  void *data;

  mdebugInit ("chk");
  data = mdmalloc (10);
  mdfree (data);
  ck_assert_int_eq (mdebugCount (), 0);
  ck_assert_int_eq (mdebugErrors (), 0);
  mdebugReport ();
  mdebugCleanup ();
}
END_TEST

START_TEST(mdebug_valid_b)
{
  void *data [10];

  mdebugInit ("chk");
  data[0] = mdmalloc (10);
  data[1] = mdmalloc (10);
  mdfree (data[0]);
  mdfree (data[1]);
  ck_assert_int_eq (mdebugCount (), 0);
  ck_assert_int_eq (mdebugErrors (), 0);

  data[0] = mdmalloc (10);
  data[1] = mdmalloc (10);
  mdfree (data[0]);
  mdfree (data[1]);
  ck_assert_int_eq (mdebugCount (), 0);
  ck_assert_int_eq (mdebugErrors (), 0);

  for (int i = 0; i < 10; ++i) {
    data [i] = mdmalloc (10);
  }
  for (int i = 0; i < 10; ++i) {
    mdfree (data [i]);
  }
  ck_assert_int_eq (mdebugCount (), 0);
  ck_assert_int_eq (mdebugErrors (), 0);

  for (int i = 0; i < 10; ++i) {
    data [i] = mdmalloc (10);
  }
  for (int i = 9; i >= 0; --i) {
    mdfree (data [i]);
  }
  ck_assert_int_eq (mdebugCount (), 0);
  ck_assert_int_eq (mdebugErrors (), 0);

  mdebugReport ();
  mdebugCleanup ();
}
END_TEST

START_TEST(mdebug_no_free)
{
  void *data;

  mdebugInit ("chk");
  data = mdmalloc (10);
  ck_assert_int_eq (mdebugCount (), 1);
  mdebugReport ();
  ck_assert_int_eq (mdebugErrors (), 1);
  mdfree (data);
  mdebugCleanup ();
}
END_TEST

START_TEST(mdebug_unknown_free)
{
  void *data;

  mdebugInit ("chk");
  data = mdmalloc (10);
  mdfree (data);
  ck_assert_int_eq (mdebugCount (), 0);
  ck_assert_int_eq (mdebugErrors (), 1);
  mdebugReport ();
  mdebugCleanup ();
}
END_TEST

START_TEST(mdebug_bad_realloc)
{
  void *data;

  mdebugInit ("chk");
  data = mdmalloc (10);
  data = mdrealloc (data, 20);
  mdfree (data);
  ck_assert_int_eq (mdebugCount (), 0);
  ck_assert_int_eq (mdebugErrors (), 1);
  mdebugReport ();
  mdebugCleanup ();
}
END_TEST

Suite *
mdebug_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("mdebug");
  tc = tcase_create ("mdebug");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, mdebug_valid);
  tcase_add_test (tc, mdebug_valid_b);
  tcase_add_test (tc, mdebug_no_free);
  tcase_add_test (tc, mdebug_unknown_free);
  tcase_add_test (tc, mdebug_bad_realloc);
  suite_add_tcase (s, tc);
  return s;
}

