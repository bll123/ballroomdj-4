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

#include "autosel.h"
#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "log.h"
#include "templateutil.h"

static void
setup (void)
{
  templateFileCopy ("autoselection.txt", "autoselection.txt");
  bdjvarsdfloadInit ();
}

static void
teardown (void)
{
  bdjvarsdfloadCleanup ();
}

START_TEST(autosel_alloc)
{
  autosel_t     *autosel;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- autosel_alloc");
  mdebugSubTag ("autosel_alloc");

  autosel = autoselAlloc ();
  ck_assert_ptr_nonnull (autosel);
  autoselFree (autosel);
}
END_TEST

START_TEST(autosel_get)
{
  autosel_t     *autosel;
  int           val;
  double        dval;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- autosel_get");
  mdebugSubTag ("autosel_get");

  /* check for the default values from the template */
  autosel = autoselAlloc ();

  dval = autoselGetDouble (autosel, AUTOSEL_FAST_BOTH);
  ck_assert_float_eq (dval, 1000.0);
  dval = autoselGetDouble (autosel, AUTOSEL_FAST_PRIOR);
  ck_assert_float_eq (dval, 100.0);
  val = autoselGetNum (autosel, AUTOSEL_HIST_DISTANCE);
  ck_assert_int_eq (val, 5);

  dval = autoselGetDouble (autosel, AUTOSEL_PREV_TAGMATCH);
  ck_assert_float_eq (dval, 80.0);
  dval = autoselGetDouble (autosel, AUTOSEL_TYPE_MATCH);
  ck_assert_float_eq (dval, 50.0);
  dval = autoselGetDouble (autosel, AUTOSEL_TAGMATCH);
  ck_assert_float_eq (dval, 20.0);
  dval = autoselGetDouble (autosel, AUTOSEL_PRIOR_EXP);
  ck_assert_float_eq (dval, 1.3);

  val = autoselGetNum (autosel, AUTOSEL_BEG_COUNT);
  ck_assert_int_eq (val, 3);
  dval = autoselGetDouble (autosel, AUTOSEL_BEG_FAST);
  ck_assert_float_eq (dval, 30000.0);

  dval = autoselGetDouble (autosel, AUTOSEL_RATING_WEIGHT);
  ck_assert_float_eq (dval, 0.8);
  dval = autoselGetDouble (autosel, AUTOSEL_LEVEL_WEIGHT);
  ck_assert_float_eq (dval, 0.1);
  dval = autoselGetDouble (autosel, AUTOSEL_TAG_WEIGHT);
  ck_assert_float_eq (dval, 0.1);
  dval = autoselGetDouble (autosel, AUTOSEL_RATING_WEIGHT) +
      autoselGetDouble (autosel, AUTOSEL_LEVEL_WEIGHT) +
      autoselGetDouble (autosel, AUTOSEL_TAG_WEIGHT);
  ck_assert_float_eq (dval, 1.0);

  dval = autoselGetDouble (autosel, AUTOSEL_WINDOWED_DIFF_A);
  ck_assert_float_eq (dval, 0.5);
  dval = autoselGetDouble (autosel, AUTOSEL_WINDOWED_DIFF_B);
  ck_assert_float_eq (dval, 0.25);
  dval = autoselGetDouble (autosel, AUTOSEL_WINDOWED_DIFF_C);
  ck_assert_float_eq (dval, 0.1);

  autoselFree (autosel);
}
END_TEST


Suite *
autosel_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("autosel");
  tc = tcase_create ("autosel");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, autosel_alloc);
  tcase_add_test (tc, autosel_get);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
