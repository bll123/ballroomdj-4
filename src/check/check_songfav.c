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
#pragma gcc diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "datafile.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "songfav.h"
#include "templateutil.h"

static void
setup (void)
{
  templateFileCopy ("favorites.txt", "favorites.txt");
}

START_TEST(songfav_alloc)
{
  songfav_t   *songfav = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfav_alloc");

  songfav = songFavoriteAlloc ();
  ck_assert_ptr_nonnull (songfav);
  songFavoriteFree (songfav);
}
END_TEST

START_TEST(songfav_count)
{
  songfav_t     *songfav = NULL;
  int           count;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfav_count");
  songfav = songFavoriteAlloc ();
  count = songFavoriteGetCount (songfav);
  ck_assert_int_ge (count, 1);
  songFavoriteFree (songfav);
}
END_TEST

START_TEST(songfav_next_value)
{
  songfav_t     *songfav = NULL;
  int           value = 0;
  int           tvalue = 0;
  int           count;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfav_next_value");
  songfav = songFavoriteAlloc ();
  count = songFavoriteGetCount (songfav);
  value = songFavoriteGetNextValue (songfav, value);
  while (value != 0) {
    ++tvalue;
    ck_assert_int_eq (value, tvalue);
    ck_assert_int_lt (value, count);
    value = songFavoriteGetNextValue (songfav, value);
  }
  ck_assert_int_eq (value, 0);
  ck_assert_int_lt (value, count);
  songFavoriteFree (songfav);
}
END_TEST

START_TEST(songfav_get_str)
{
  songfav_t     *songfav = NULL;
  const char    *str;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfav_get-str");
  songfav = songFavoriteAlloc ();

  str = songFavoriteGetStr (songfav, 1, SONGFAV_COLOR);
  ck_assert_ptr_nonnull (str);
  str = songFavoriteGetStr (songfav, 1, SONGFAV_NAME);
  ck_assert_ptr_nonnull (str);
  str = songFavoriteGetStr (songfav, 1, SONGFAV_DISPLAY);
  ck_assert_ptr_nonnull (str);

  str = songFavoriteGetSpanStr (songfav, 1);
  ck_assert_ptr_nonnull (str);

  songFavoriteFree (songfav);
}
END_TEST

START_TEST(songfav_conv)
{
  songfav_t     *songfav = NULL;
  datafileconv_t conv;
  int           count;

  bdjvarsdfloadInit ();

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfav_conv");

  songfav = songFavoriteAlloc ();

  conv.allocated = false;
  conv.valuetype = VALUE_STR;
  conv.str = "bluestar";
  songFavoriteConv (&conv);
  ck_assert_int_ge (conv.num, 0);

  songFavoriteConv (&conv);
  ck_assert_str_eq (conv.str, "bluestar");
  if (conv.allocated) {
    mdfree (conv.str);
  }

  count = songFavoriteGetCount (songfav);
  conv.allocated = false;
  conv.valuetype = VALUE_STR;
  conv.str = "imported";
  songFavoriteConv (&conv);
  ck_assert_int_ge (conv.num, count);

  songFavoriteFree (songfav);
  bdjvarsdfloadCleanup ();
}
END_TEST

Suite *
songfav_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("songfav");
  tc = tcase_create ("songfav");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, NULL);
  tcase_add_test (tc, songfav_alloc);
  tcase_add_test (tc, songfav_count);
  tcase_add_test (tc, songfav_next_value);
  tcase_add_test (tc, songfav_get_str);
  tcase_add_test (tc, songfav_conv);
  suite_add_tcase (s, tc);
  return s;
}
