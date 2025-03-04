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

#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "datafile.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "rating.h"
#include "slist.h"
#include "templateutil.h"

static void
setup (void)
{
  templateFileCopy ("ratings.txt", "ratings.txt");
  bdjvarsdfloadInit ();
}

static void
teardown (void)
{
  bdjvarsdfloadCleanup ();
}

START_TEST(rating_alloc)
{
  rating_t   *rating = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- rating_alloc");
  mdebugSubTag ("rating_alloc");

  rating = ratingAlloc ();
  ck_assert_ptr_nonnull (rating);
  ratingFree (rating);
}
END_TEST

START_TEST(rating_iterate)
{
  rating_t     *rating = NULL;
  slistidx_t  iteridx;
  int         count;
  int         key;
  int         w;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- rating_iterate");
  mdebugSubTag ("rating_iterate");

  rating = ratingAlloc ();
  ratingStartIterator (rating, &iteridx);
  count = 0;
  while ((key = ratingIterate (rating, &iteridx)) >= 0) {
    ratingGetRating (rating, key);
    w = ratingGetWeight (rating, key);
    ck_assert_int_ge (w, 0);
    ++count;
  }
  ck_assert_int_eq (count, ratingGetCount (rating));
  ck_assert_int_ge (ratingGetMaxWidth (rating), 1);
  ratingFree (rating);
}
END_TEST

START_TEST(rating_conv)
{
  rating_t     *rating = NULL;
  const char  *val = NULL;
  slistidx_t  iteridx;
  datafileconv_t conv;
  int         count;
  int         key;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- rating_conv");
  mdebugSubTag ("rating_conv");

  rating = ratingAlloc ();
  ratingStartIterator (rating, &iteridx);
  count = 0;
  while ((key = ratingIterate (rating, &iteridx)) >= 0) {
    val = ratingGetRating (rating, key);

    conv.invt = VALUE_STR;
    conv.str = val;
    ratingConv (&conv);
    ck_assert_int_eq (conv.num, count);

    conv.invt = VALUE_NUM;
    conv.num = count;
    ratingConv (&conv);
    ck_assert_str_eq (conv.str, val);

    ++count;
  }
  ratingFree (rating);
}
END_TEST

START_TEST(rating_save)
{
  rating_t     *rating = NULL;
  const char  *val = NULL;
  ilistidx_t  giteridx;
  ilistidx_t  iiteridx;
  int         key;
  int         w;
  ilist_t     *tlist;
  int         tkey;
  const char  *tval;
  int         tw;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- rating_save");
  mdebugSubTag ("rating_save");

  rating = ratingAlloc ();

  ratingStartIterator (rating, &giteridx);
  tlist = ilistAlloc ("chk-rating", LIST_ORDERED);
  while ((key = ratingIterate (rating, &giteridx)) >= 0) {
    val = ratingGetRating (rating, key);
    w = ratingGetWeight (rating, key);
    ilistSetStr (tlist, key, RATING_RATING, val);
    ilistSetNum (tlist, key, RATING_WEIGHT, w);
  }

  ratingSave (rating, tlist);
  ratingFree (rating);
  bdjvarsdfloadCleanup ();

  bdjvarsdfloadInit ();
  rating = ratingAlloc ();

  ck_assert_int_eq (ilistGetCount (tlist), ratingGetCount (rating));

  ratingStartIterator (rating, &giteridx);
  ilistStartIterator (tlist, &iiteridx);
  while ((key = ratingIterate (rating, &giteridx)) >= 0) {
    val = ratingGetRating (rating, key);
    w = ratingGetWeight (rating, key);

    tkey = ilistIterateKey (tlist, &iiteridx);
    tval = ilistGetStr (tlist, key, RATING_RATING);
    tw = ilistGetNum (tlist, key, RATING_WEIGHT);

    ck_assert_int_eq (key, tkey);
    ck_assert_str_eq (val, tval);
    ck_assert_int_eq (w, tw);
  }

  ilistFree (tlist);
  ratingFree (rating);
}
END_TEST

START_TEST(rating_set)
{
  rating_t    *rating = NULL;
  const char  *sval = NULL;
  int         oval;
  int         nval;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- rating_set");
  mdebugSubTag ("rating_set");

  rating = ratingAlloc ();

  sval = ratingGetRating (rating, 0);
  ck_assert_str_ne (sval, "test");
  ratingSetRating (rating, 0, "test");
  sval = ratingGetRating (rating, 0);
  ck_assert_str_eq (sval, "test");

  oval = ratingGetWeight (rating, 0);
  ratingSetWeight (rating, 0, oval + 10);
  nval = ratingGetWeight (rating, 0);
  ck_assert_int_eq (oval + 10, nval);

  ratingFree (rating);
}
END_TEST

START_TEST(rating_dellast)
{
  rating_t    *rating = NULL;
  int         ocount, ncount;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- rating_dellast");
  mdebugSubTag ("rating_dellast");

  rating = ratingAlloc ();

  ocount = ratingGetCount (rating);
  ratingDeleteLast (rating);
  ncount = ratingGetCount (rating);
  ck_assert_int_eq (ocount - 1, ncount);

  ratingFree (rating);
}
END_TEST

Suite *
rating_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("rating");
  tc = tcase_create ("rating");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, rating_alloc);
  tcase_add_test (tc, rating_iterate);
  tcase_add_test (tc, rating_conv);
  tcase_add_test (tc, rating_save);
  tcase_add_test (tc, rating_set);
  tcase_add_test (tc, rating_dellast);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
