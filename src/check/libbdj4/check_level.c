/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
#include "level.h"
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "templateutil.h"

static void
setup (void)
{
  templateFileCopy ("levels.txt", "levels.txt");
  bdjvarsdfloadInit ();
}

static void
teardown (void)
{
  bdjvarsdfloadCleanup ();
}

START_TEST(level_alloc)
{
  level_t   *level = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- level_alloc");
  mdebugSubTag ("level_alloc");

  level = levelAlloc ();
  ck_assert_ptr_nonnull (level);
  levelFree (level);
}
END_TEST

START_TEST(level_iterate)
{
  level_t     *level = NULL;
  const char  *val = NULL;
  slistidx_t  iteridx;
  int         count;
  int         key;
  int         w;
  int         df;
  int         dcount;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- level_iterate");
  mdebugSubTag ("level_iterate");

  level = levelAlloc ();
  levelStartIterator (level, &iteridx);
  count = 0;
  dcount = 0;
  while ((key = levelIterate (level, &iteridx)) >= 0) {
    val = levelGetLevel (level, key);

    w = levelGetWeight (level, key);
    ck_assert_int_ge (w, 0);

    df = levelGetDefault (level, key);
    ck_assert_int_ge (df, 0);
    ck_assert_int_le (df, 1);
    if (df) {
      ck_assert_str_eq (val, levelGetDefaultName (level));
      ck_assert_int_eq (key, levelGetDefaultKey (level));
      ++dcount;
    }
    ++count;
  }
  ck_assert_int_eq (dcount, 1);
  ck_assert_int_eq (count, levelGetCount (level));
  ck_assert_int_ge (levelGetMaxWidth (level), 1);
  levelFree (level);
}
END_TEST

START_TEST(level_conv)
{
  level_t     *level = NULL;
  const char  *val = NULL;
  slistidx_t  iteridx;
  datafileconv_t conv;
  int         count;
  int         key;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- level_conv");
  mdebugSubTag ("level_conv");

  level = levelAlloc ();
  levelStartIterator (level, &iteridx);
  count = 0;
  while ((key = levelIterate (level, &iteridx)) >= 0) {
    val = levelGetLevel (level, key);

    conv.invt = VALUE_STR;
    conv.str = val;
    levelConv (&conv);
    ck_assert_int_eq (conv.num, count);

    conv.invt = VALUE_NUM;
    conv.num = count;
    levelConv (&conv);
    ck_assert_str_eq (conv.str, val);

    ++count;
  }
  levelFree (level);
}
END_TEST

START_TEST(level_save)
{
  level_t     *level = NULL;
  const char  *val = NULL;
  ilistidx_t  giteridx;
  ilistidx_t  iiteridx;
  int         key;
  int         w;
  int         df;
  ilist_t     *tlist;
  int         tkey;
  const char  *tval;
  int         tw;
  int         tdf;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- level_save");
  mdebugSubTag ("level_save");

  /* required for the level conversion function */

  level = levelAlloc ();

  levelStartIterator (level, &giteridx);
  tlist = ilistAlloc ("chk-level", LIST_ORDERED);
  while ((key = levelIterate (level, &giteridx)) >= 0) {
    val = levelGetLevel (level, key);
    w = levelGetWeight (level, key);
    df = levelGetDefault (level, key);
    ilistSetStr (tlist, key, LEVEL_LEVEL, val);
    ilistSetNum (tlist, key, LEVEL_WEIGHT, w);
    ilistSetNum (tlist, key, LEVEL_DEFAULT_FLAG, df);
  }

  levelSave (level, tlist);
  levelFree (level);
  bdjvarsdfloadCleanup ();

  bdjvarsdfloadInit ();
  level = levelAlloc ();

  ck_assert_int_eq (ilistGetCount (tlist), levelGetCount (level));

  levelStartIterator (level, &giteridx);
  ilistStartIterator (tlist, &iiteridx);
  while ((key = levelIterate (level, &giteridx)) >= 0) {
    val = levelGetLevel (level, key);
    w = levelGetWeight (level, key);
    df = levelGetDefault (level, key);

    tkey = ilistIterateKey (tlist, &iiteridx);
    tval = ilistGetStr (tlist, key, LEVEL_LEVEL);
    tw = ilistGetNum (tlist, key, LEVEL_WEIGHT);
    tdf = ilistGetNum (tlist, key, LEVEL_DEFAULT_FLAG);

    ck_assert_int_eq (key, tkey);
    ck_assert_str_eq (val, tval);
    ck_assert_int_eq (w, tw);
    ck_assert_int_eq (df, tdf);
  }

  ilistFree (tlist);
  levelFree (level);
}
END_TEST

START_TEST(level_set)
{
  level_t    *level = NULL;
  const char  *sval = NULL;
  int         oval;
  int         nval;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- level_set");
  mdebugSubTag ("level_set");

  level = levelAlloc ();

  sval = levelGetLevel (level, 0);
  ck_assert_str_ne (sval, "test");
  levelSetLevel (level, 0, "test");
  sval = levelGetLevel (level, 0);
  ck_assert_str_eq (sval, "test");

  oval = levelGetWeight (level, 0);
  levelSetWeight (level, 0, oval + 10);
  nval = levelGetWeight (level, 0);
  ck_assert_int_eq (oval + 10, nval);

  nval = levelGetDefault (level, 0);
  ck_assert_int_ne (nval, 1);
  levelSetDefault (level, 0);
  nval = levelGetDefault (level, 0);
  ck_assert_int_eq (nval, 1);
  levelSetDefault (level, 1);
  nval = levelGetDefault (level, 0);
  ck_assert_int_eq (nval, 0);
  nval = levelGetDefault (level, 1);
  ck_assert_int_eq (nval, 1);

  levelFree (level);
}
END_TEST

START_TEST(level_dellast)
{
  level_t    *level = NULL;
  int         ocount, ncount;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- level_dellast");
  mdebugSubTag ("level_dellast");

  level = levelAlloc ();

  ocount = levelGetCount (level);
  levelDeleteLast (level);
  ncount = levelGetCount (level);
  ck_assert_int_eq (ocount - 1, ncount);

  levelFree (level);
}
END_TEST

Suite *
level_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("level");
  tc = tcase_create ("level");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, level_alloc);
  tcase_add_test (tc, level_iterate);
  tcase_add_test (tc, level_conv);
  tcase_add_test (tc, level_save);
  tcase_add_test (tc, level_set);
  tcase_add_test (tc, level_dellast);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
