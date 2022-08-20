#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>

#pragma clang diagnostic push
#pragma gcc diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "datafile.h"
#include "ilist.h"
#include "rating.h"
#include "slist.h"
#include "templateutil.h"

static void
setup (void)
{
  templateFileCopy ("ratings.txt", "ratings.txt");
}

START_TEST(rating_alloc)
{
  rating_t   *rating = NULL;

  rating = ratingAlloc ();
  ck_assert_ptr_nonnull (rating);
  ratingFree (rating);
}
END_TEST

START_TEST(rating_iterate)
{
  rating_t     *rating = NULL;
  char        *val = NULL;
  slistidx_t  iteridx;
  int         count;
  int         key;
  int         w;

  rating = ratingAlloc ();
  ratingStartIterator (rating, &iteridx);
  count = 0;
  while ((key = ratingIterate (rating, &iteridx)) >= 0) {
    val = ratingGetRating (rating, key);
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
  char        *val = NULL;
  slistidx_t  iteridx;
  datafileconv_t conv;
  int         count;
  int         key;

  bdjvarsdfloadInit ();

  rating = ratingAlloc ();
  ratingStartIterator (rating, &iteridx);
  count = 0;
  while ((key = ratingIterate (rating, &iteridx)) >= 0) {
    val = ratingGetRating (rating, key);

    conv.allocated = false;
    conv.valuetype = VALUE_STR;
    conv.str = val;
    ratingConv (&conv);
    ck_assert_int_eq (conv.num, count);

    conv.allocated = false;
    conv.valuetype = VALUE_NUM;
    conv.num = count;
    ratingConv (&conv);
    ck_assert_str_eq (conv.str, val);
    if (conv.allocated) {
      free (conv.str);
    }

    ++count;
  }
  ratingFree (rating);
  bdjvarsdfloadCleanup ();
}
END_TEST

START_TEST(rating_save)
{
  rating_t     *rating = NULL;
  char        *val = NULL;
  ilistidx_t  giteridx;
  ilistidx_t  iiteridx;
  int         key;
  int         w;
  ilist_t     *tlist;
  int         tkey;
  char        *tval;
  int         tw;

  /* required for the rating conversion function */
  bdjvarsdfloadInit ();

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
  bdjvarsdfloadCleanup ();
}
END_TEST

Suite *
rating_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("rating");
  tc = tcase_create ("rating");
  tcase_add_unchecked_fixture (tc, setup, NULL);
  tcase_add_test (tc, rating_alloc);
  tcase_add_test (tc, rating_iterate);
  tcase_add_test (tc, rating_conv);
  tcase_add_test (tc, rating_save);
  suite_add_tcase (s, tc);
  return s;
}
