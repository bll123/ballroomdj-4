/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#if ! defined (BDJ4_MEM_DEBUG)
# define BDJ4_MEM_DEBUG
#endif

#include "check_bdj.h"
#include "log.h"
#include "mdebug.h"

START_TEST(mdebug_valid)
{
  void  *data;

  logMsg (LOG_DBG, LOG_IMPORTANT, "valid");
  mdebugCleanup ();
  mdebugInit ("chk-md-v");
  mdebugSetNoOutput ();
  data = mdmalloc (10);
  mdfree (data);
  ck_assert_int_eq (mdebugCount (), 0);
  ck_assert_int_eq (mdebugErrors (), 0);
  mdebugCleanup ();
}
END_TEST

START_TEST(mdebug_valid_b)
{
  void  *data [10];

  logMsg (LOG_DBG, LOG_IMPORTANT, "valid-b");
  mdebugCleanup ();
  mdebugInit ("chk-md-vb");
  mdebugSetNoOutput ();
  data [0] = mdmalloc (10);
  data [1] = mdmalloc (10);
  mdfree (data [0]);
  mdfree (data [1]);
  ck_assert_int_eq (mdebugCount (), 0);
  ck_assert_int_eq (mdebugErrors (), 0);

  data [0] = mdmalloc (10);
  data [1] = mdmalloc (10);
  mdfree (data [0]);
  mdfree (data [1]);
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
    data [i] = mdmalloc (20);
  }
  for (int i = 9; i >= 0; --i) {
    mdfree (data [i]);
  }
  ck_assert_int_eq (mdebugCount (), 0);
  ck_assert_int_eq (mdebugErrors (), 0);
  mdebugCleanup ();
}
END_TEST

START_TEST(mdebug_valid_ext_a)
{
  void  *data;

  logMsg (LOG_DBG, LOG_IMPORTANT, "valid");
  mdebugCleanup ();
  mdebugInit ("chk-md-v");
  mdebugSetNoOutput ();
  data = malloc (10);
  mdextalloc (data);
  mdextfree (data);
  ck_assert_int_eq (mdebugCount (), 0);
  ck_assert_int_eq (mdebugErrors (), 0);
  mdebugCleanup ();
}
END_TEST

START_TEST(mdebug_valid_ext_b)
{
  void  *data;

  logMsg (LOG_DBG, LOG_IMPORTANT, "valid");
  mdebugCleanup ();
  mdebugInit ("chk-md-v");
  mdebugSetNoOutput ();
  data = malloc (10);
  mdextalloc (data);
  mdfree (data);
  ck_assert_int_eq (mdebugCount (), 0);
  ck_assert_int_eq (mdebugErrors (), 0);
  mdebugCleanup ();
}
END_TEST

START_TEST(mdebug_no_free)
{
  void  *data;

  logMsg (LOG_DBG, LOG_IMPORTANT, "no-free");
  mdebugCleanup ();
  mdebugInit ("chk-md-nf");
  mdebugSetNoOutput ();
  data = mdmalloc (10);
  ck_assert_int_eq (mdebugCount (), 1);
  mdebugReport ();
  ck_assert_int_eq (mdebugErrors (), 1);
  mdfree (data);
  mdebugCleanup ();
}
END_TEST

START_TEST(mdebug_null_free)
{
  void  *data = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "null-free");
  mdebugCleanup ();
  mdebugInit ("chk-md-nullf");
  mdebugSetNoOutput ();
  mdfree (data);
  ck_assert_int_eq (mdebugCount (), 0);
  ck_assert_int_eq (mdebugErrors (), 1);
  mdebugCleanup ();
}
END_TEST

START_TEST(mdebug_unknown_free)
{
  void  *data;

  logMsg (LOG_DBG, LOG_IMPORTANT, "unknown-free");
  mdebugCleanup ();
  mdebugInit ("chk-md-uf");
  mdebugSetNoOutput ();
  data = malloc (10);
  mdfree (data);
  ck_assert_int_eq (mdebugCount (), 0);
  ck_assert_int_eq (mdebugErrors (), 1);
  mdebugCleanup ();
}
END_TEST

START_TEST(mdebug_bad_realloc)
{
  void  *data;

  logMsg (LOG_DBG, LOG_IMPORTANT, "bad-realloc");
  mdebugCleanup ();
  mdebugInit ("chk-md-br");
  mdebugSetNoOutput ();
  data = malloc (55);
  data = mdrealloc (data, 95);
  mdfree (data);
  ck_assert_int_eq (mdebugCount (), 0);
  ck_assert_int_eq (mdebugErrors (), 1);
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
  tcase_add_test (tc, mdebug_valid_ext_a);
  tcase_add_test (tc, mdebug_valid_ext_b);
  tcase_add_test (tc, mdebug_no_free);
  tcase_add_test (tc, mdebug_null_free);
  tcase_add_test (tc, mdebug_unknown_free);
  tcase_add_test (tc, mdebug_bad_realloc);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
