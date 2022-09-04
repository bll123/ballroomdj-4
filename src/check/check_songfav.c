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
#include "songfav.h"
#include "slist.h"

typedef struct {
  char  *value;
  int   key;
} sf_test_t;

static sf_test_t tvalues [] = {
  { "bluestar", SONG_FAVORITE_BLUE },
  { "unknown", SONG_FAVORITE_NONE },
  { "redstar", SONG_FAVORITE_RED },
  { "", SONG_FAVORITE_NONE },
  { "purplestar", SONG_FAVORITE_PURPLE },
  };
enum {
  tvaluesz = sizeof (tvalues) / sizeof (sf_test_t),
};

START_TEST(songfav_init)
{
  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfav_init");

  songFavoriteInit ();
  songFavoriteInit ();
  songFavoriteCleanup ();
  songFavoriteCleanup ();
  songFavoriteInit ();
  songFavoriteCleanup ();
}
END_TEST

START_TEST(songfav_conv)
{
  datafileconv_t    conv;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfav_conv");

  for (int i = 0; i < tvaluesz; ++i) {
    conv.allocated = false;
    conv.valuetype = VALUE_STR;
    conv.str = tvalues [i].value;
    songFavoriteConv (&conv);
    ck_assert_int_eq (conv.num, tvalues [i].key);

    songFavoriteConv (&conv);
    if (tvalues [i].key == SONG_FAVORITE_NONE) {
      ck_assert_str_eq (conv.str, "");
    } else {
      ck_assert_str_eq (conv.str, tvalues [i].value);
    }
  }
}
END_TEST

START_TEST(songfav_get)
{
  songfavoriteinfo_t  *sfi;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfav_get");

  for (int i = 0; i < tvaluesz; ++i) {
    sfi = songFavoriteGet (tvalues [i].key);
    ck_assert_ptr_null (sfi->spanStr);
  }
  songFavoriteInit ();
  for (int i = 0; i < tvaluesz; ++i) {
    sfi = songFavoriteGet (tvalues [i].key);
    ck_assert_int_eq (sfi->idx, tvalues [i].key);
    ck_assert_ptr_nonnull (sfi->spanStr);
  }
  songFavoriteCleanup ();
  for (int i = 0; i < tvaluesz; ++i) {
    sfi = songFavoriteGet (tvalues [i].key);
    ck_assert_ptr_null (sfi->spanStr);
  }
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
  tcase_add_test (tc, songfav_init);
  tcase_add_test (tc, songfav_conv);
  tcase_add_test (tc, songfav_get);
  suite_add_tcase (s, tc);
  return s;
}
