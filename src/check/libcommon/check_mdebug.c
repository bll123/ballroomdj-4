/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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

#if defined (BDJ4_MEM_DEBUG)
# define BDJ4_MEM_DEBUG_ALREADY
#endif
#if ! defined (BDJ4_MEM_DEBUG)
# define BDJ4_MEM_DEBUG
#endif

#include "check_bdj.h"
#include "log.h"
#include "mdebug.h"

START_TEST(mdebug_valid)
{
  void  *data;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mdebug_valid");
#ifdef BDJ4_MEM_DEBUG_ALREADY
  logMsg (LOG_DBG, LOG_IMPORTANT, "  not run");
  return;
#endif
  mdebugCleanup ();
  mdebugInit ("chk-md-v");
  mdebugSubTag ("mdebug_valid");
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

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mdebug_valid_b");
#ifdef BDJ4_MEM_DEBUG_ALREADY
  logMsg (LOG_DBG, LOG_IMPORTANT, "  not run");
  return;
#endif
  mdebugCleanup ();
  mdebugInit ("chk-md-vb");
  mdebugSubTag ("mdebug_valid_b");
  mdebugSetNoOutput ();
  data [0] = mdmalloc (10);
  data [1] = mdmalloc (10);
  mdfree (data [1]);
  mdfree (data [0]);
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

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mdebug_valid_ext_a");
#ifdef BDJ4_MEM_DEBUG_ALREADY
  logMsg (LOG_DBG, LOG_IMPORTANT, "  not run");
  return;
#endif
  mdebugCleanup ();
  mdebugInit ("chk-md-ext-a");
  mdebugSubTag ("mdebug_valid_ext_a");
  mdebugSetNoOutput ();
  data = malloc (10);
  mdextalloc (data);
  mdextfree (data);
  free (data);
  ck_assert_int_eq (mdebugCount (), 0);
  ck_assert_int_eq (mdebugErrors (), 0);
  mdebugCleanup ();
}
END_TEST

START_TEST(mdebug_valid_ext_b)
{
  void  *data;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mdebug_valid_ext_b");
#ifdef BDJ4_MEM_DEBUG_ALREADY
  logMsg (LOG_DBG, LOG_IMPORTANT, "  not run");
  return;
#endif
  mdebugCleanup ();
  mdebugInit ("chk-md-ext-b");
  mdebugSubTag ("mdebug_valid_ext_b");
  mdebugSetNoOutput ();
  data = malloc (10);
  mdextalloc (data);
  mdfree (data);
  ck_assert_int_eq (mdebugCount (), 0);
  ck_assert_int_eq (mdebugErrors (), 0);
  mdebugCleanup ();
}
END_TEST

START_TEST(mdebug_valid_open_close)
{
  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mdebug_valid_open_close");
#ifdef BDJ4_MEM_DEBUG_ALREADY
  logMsg (LOG_DBG, LOG_IMPORTANT, "  not run");
  return;
#endif
  mdebugCleanup ();
  mdebugInit ("chk-md-o-c");
  mdebugSubTag ("mdebug_valid_open_close");
  mdebugSetNoOutput ();
  mdextopen (1);
  mdextclose (1);
  ck_assert_int_eq (mdebugCount (), 0);
  ck_assert_int_eq (mdebugErrors (), 0);
  mdebugCleanup ();
}
END_TEST

START_TEST(mdebug_valid_fopen_fclose)
{
  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mdebug_valid_fopen_fclose");
#ifdef BDJ4_MEM_DEBUG_ALREADY
  logMsg (LOG_DBG, LOG_IMPORTANT, "  not run");
  return;
#endif
  mdebugCleanup ();
  mdebugInit ("chk-md-o-c");
  mdebugSubTag ("mdebug_valid_fopen_fclose");
  mdebugSetNoOutput ();
  mdextfopen (stdin);
  mdextfclose (stdin);
  ck_assert_int_eq (mdebugCount (), 0);
  ck_assert_int_eq (mdebugErrors (), 0);
  mdebugCleanup ();
}
END_TEST

START_TEST(mdebug_no_free)
{
  void  *data;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mdebug_no_free");
#ifdef BDJ4_MEM_DEBUG_ALREADY
  logMsg (LOG_DBG, LOG_IMPORTANT, "  not run");
  return;
#endif
  mdebugCleanup ();
  mdebugInit ("chk-md-nf");
  mdebugSubTag ("mdebug_no_free");
  mdebugSetNoOutput ();
  data = mdmalloc (10);
  mdebugReport ();    // needed for no-free count
  ck_assert_int_eq (mdebugCount (), 1);
  ck_assert_int_eq (mdebugErrors (), 1);
  mdfree (data);
  mdebugCleanup ();
}
END_TEST

START_TEST(mdebug_null_free)
{
  void  *data = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mdebug_null_free");
#ifdef BDJ4_MEM_DEBUG_ALREADY
  logMsg (LOG_DBG, LOG_IMPORTANT, "  not run");
  return;
#endif
  mdebugCleanup ();
  mdebugInit ("chk-md-nullf");
  mdebugSubTag ("mdebug_null_free");
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

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mdebug_unknown_free");
#ifdef BDJ4_MEM_DEBUG_ALREADY
  logMsg (LOG_DBG, LOG_IMPORTANT, "  not run");
  return;
#endif
  mdebugCleanup ();
  mdebugInit ("chk-md-uf");
  mdebugSubTag ("mdebug_unknown_free");
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

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mdebug_bad_realloc");
#ifdef BDJ4_MEM_DEBUG_ALREADY
  logMsg (LOG_DBG, LOG_IMPORTANT, "  not run");
  return;
#endif
  mdebugCleanup ();
  mdebugInit ("chk-md-br");
  mdebugSubTag ("mdebug_bad_realloc");
  mdebugSetNoOutput ();
  data = malloc (55);
  data = mdrealloc (data, 95);
  mdfree (data);
  ck_assert_int_eq (mdebugCount (), 0);
  ck_assert_int_eq (mdebugErrors (), 1);
  mdebugCleanup ();
}
END_TEST

START_TEST(mdebug_no_close)
{
  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mdebug_no_close");
#ifdef BDJ4_MEM_DEBUG_ALREADY
  logMsg (LOG_DBG, LOG_IMPORTANT, "  not run");
  return;
#endif
  mdebugCleanup ();
  mdebugInit ("chk-md-nc");
  mdebugSubTag ("mdebug_no_close");
  mdebugSetNoOutput ();
  mdextopen (1);
  ck_assert_int_eq (mdebugCount (), 1);
  mdebugReport ();    // needed for no-free count
  ck_assert_int_eq (mdebugErrors (), 1);
  mdebugCleanup ();
}
END_TEST

START_TEST(mdebug_no_fclose)
{
  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- mdebug_no_fclose");
#ifdef BDJ4_MEM_DEBUG_ALREADY
  logMsg (LOG_DBG, LOG_IMPORTANT, "  not run");
  return;
#endif
  mdebugCleanup ();
  mdebugInit ("chk-md-nfc");
  mdebugSubTag ("mdebug_no_fclose");
  mdebugSetNoOutput ();
  mdextfopen (stdin);
  ck_assert_int_eq (mdebugCount (), 1);
  mdebugReport ();    // needed for no-free count
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
  tcase_add_test (tc, mdebug_valid_open_close);
  tcase_add_test (tc, mdebug_valid_fopen_fclose);
  tcase_add_test (tc, mdebug_no_free);
  tcase_add_test (tc, mdebug_null_free);
  tcase_add_test (tc, mdebug_unknown_free);
  tcase_add_test (tc, mdebug_bad_realloc);
  tcase_add_test (tc, mdebug_no_close);
  tcase_add_test (tc, mdebug_no_fclose);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
