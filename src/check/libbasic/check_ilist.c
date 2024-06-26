/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdjstring.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "ilist.h"
#include "log.h"

START_TEST(ilist_create_free)
{
  ilist_t    *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- ilist_create_free");
  mdebugSubTag ("ilist_create_free");

  list = ilistAlloc ("chk-a", LIST_ORDERED);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (ilistGetCount (list), 0);
  ck_assert_int_eq (ilistGetAllocCount (list), 0);
  ck_assert_int_eq (ilistGetOrdering (list), LIST_ORDERED);
  ilistFree (list);
}
END_TEST

START_TEST(ilist_get_data_str)
{
  ilist_t        *list;
  const char     *value;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- ilist_get_data_str");
  mdebugSubTag ("ilist_get_data_str");

  list = ilistAlloc ("chk-b", LIST_ORDERED);
  ilistSetSize (list, 7);
  ck_assert_int_eq (ilistGetCount (list), 0);
  ck_assert_int_eq (ilistGetAllocCount (list), 7);
  ilistSetStr (list, 6, 0, "0L");
  ilistSetStr (list, 26, 0, "1L");
  ilistSetStr (list, 18, 0, "2L");
  ilistSetStr (list, 11, 0, "3L");
  ilistSetStr (list, 3, 0, "4L");
  ilistSetStr (list, 1, 0, "5L");
  ilistSetStr (list, 2, 0, "6L");
  ck_assert_int_eq (ilistGetCount (list), 7);
  value = ilistGetStr (list, 3, 0);
  ck_assert_ptr_nonnull (value);
  ck_assert_str_eq (value, "4L");
  ilistFree (list);
}
END_TEST

START_TEST(ilist_get_data_str_null)
{
  ilist_t        *list;
  const char     *value;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- ilist_get_data_str_null");
  mdebugSubTag ("ilist_get_data_str_null");

  list = ilistAlloc ("chk-c", LIST_ORDERED);
  ilistSetStr (list, 6, 0, "0L");
  ilistSetStr (list, 26, 0, NULL);
  value = ilistGetStr (list, 6, 0);
  ck_assert_str_eq (value, "0L");
  value = ilistGetStr (list, 26, 0);
  ck_assert_ptr_null (value);
  ilistFree (list);
}
END_TEST

START_TEST(ilist_get_data_str_sub)
{
  ilist_t        *list;
  const char     *value;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- ilist_get_data_str_sub");
  mdebugSubTag ("ilist_get_data_str_sub");

  list = ilistAlloc ("chk-c", LIST_ORDERED);
  ilistSetSize (list, 7);
  ck_assert_int_eq (ilistGetCount (list), 0);
  ck_assert_int_eq (ilistGetAllocCount (list), 7);
  ilistSetStr (list, 6, 0, "0L");
  ilistSetStr (list, 6, 1, "8L");
  ck_assert_int_eq (ilistGetCount (list), 1);
  ilistSetStr (list, 26, 0, "1L");
  ilistSetStr (list, 26, 1, "9L");
  ilistSetStr (list, 18, 0, "2L");
  ilistSetStr (list, 18, 1, "10L");
  ilistSetStr (list, 11, 0, "3L");
  ilistSetStr (list, 11, 1, "11L");
  ck_assert_int_eq (ilistGetCount (list), 4);
  ilistSetStr (list, 3, 0, "4L");
  ilistSetStr (list, 3, 1, "12L");
  ilistSetStr (list, 1, 0, "5L");
  ilistSetStr (list, 1, 1, "13L");
  ilistSetStr (list, 2, 0, "6L");
  ilistSetStr (list, 2, 1, "14L");
  ck_assert_int_eq (ilistGetCount (list), 7);
  value = ilistGetStr (list, 3, 0);
  ck_assert_ptr_nonnull (value);
  ck_assert_str_eq (value, "4L");
  value = ilistGetStr (list, 26, 1);
  ck_assert_ptr_nonnull (value);
  ck_assert_str_eq (value, "9L");
  ilistFree (list);
}
END_TEST

START_TEST(ilist_iterate)
{
  ilist_t *     list;
  const char    *value;
  int           nval;
  ilistidx_t    key;
  ilistidx_t    iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- ilist_iterate");
  mdebugSubTag ("ilist_iterate");

  list = ilistAlloc ("chk-d", LIST_ORDERED);
  ck_assert_ptr_nonnull (list);
  ilistSetStr (list, 6, 0, "555");
  ilistSetNum (list, 6, 1, 6);
  ilistSetStr (list, 3, 0, "222");
  ilistSetNum (list, 3, 1, 3);
  ilistSetStr (list, 5, 0, "444");
  ilistSetNum (list, 5, 1, 5);
  ilistSetStr (list, 4, 0, "333");
  ilistSetNum (list, 4, 1, 4);
  ilistSetStr (list, 1, 0, "000");
  ilistSetNum (list, 1, 1, 1);
  ilistSetStr (list, 2, 0, "111");
  ilistSetNum (list, 2, 1, 2);

  ilistStartIterator (list, &iteridx);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "000");
  nval = ilistGetNum (list, key, 1);
  ck_assert_int_eq (nval, 1);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2);
  value = ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "111");
  nval = ilistGetNum (list, key, 1);
  ck_assert_int_eq (nval, 2);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 3);
  value = ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "222");
  nval = ilistGetNum (list, key, 1);
  ck_assert_int_eq (nval, 3);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 4);
  value = ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "333");
  nval = ilistGetNum (list, key, 1);
  ck_assert_int_eq (nval, 4);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 5);
  value = ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "444");
  nval = ilistGetNum (list, key, 1);
  ck_assert_int_eq (nval, 5);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 6);
  value = ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "555");
  nval = ilistGetNum (list, key, 1);
  ck_assert_int_eq (nval, 6);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, LIST_LOC_INVALID);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "000");
  nval = ilistGetNum (list, key, 1);
  ck_assert_int_eq (nval, 1);

  ilistFree (list);
}
END_TEST

START_TEST(ilist_u_sort)
{
  ilist_t *      list;
  ssize_t          value;
  ilistidx_t          key;
  ilistidx_t      iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- ilist_u_sort");
  mdebugSubTag ("ilist_u_sort");

  list = ilistAlloc ("chk-e", LIST_UNORDERED);
  ck_assert_ptr_nonnull (list);
  ilistSetNum (list, 6, 1, 0);
  ilistSetNum (list, 26, 1, 1);
  ilistSetNum (list, 18, 1, 2);
  ilistSetNum (list, 11, 1, 3);
  ilistSetNum (list, 3, 1, 4);
  ilistSetNum (list, 1, 1, 5);
  ilistSetNum (list, 2, 1, 6);
  ck_assert_int_eq (ilistGetCount (list), 7);
  ck_assert_int_eq (ilistGetAllocCount (list), 10);

  ilistStartIterator (list, &iteridx);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 6);
  value = ilistGetNum (list, key, 1);
  ck_assert_int_eq (value, 0);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 26);
  value = ilistGetNum (list, key, 1);
  ck_assert_int_eq (value, 1);

  ilistSort (list);
  ck_assert_int_eq (ilistGetOrdering (list), LIST_ORDERED);
  ck_assert_int_eq (ilistGetCount (list), 7);
  ilistStartIterator (list, &iteridx);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = ilistGetNum (list, key, 1);
  ck_assert_int_eq (value, 5);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2);
  value = ilistGetNum (list, key, 1);
  ck_assert_int_eq (value, 6);

  ilistFree (list);
}
END_TEST

START_TEST(ilist_replace_str)
{
  ilist_t       *list;
  ilistidx_t    key;
  const char     *value;
  ilistidx_t    iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- ilist_replace_str");
  mdebugSubTag ("ilist_replace_str");

  list = ilistAlloc ("chk-f", LIST_ORDERED);
  ck_assert_ptr_nonnull (list);

  ilistSetStr (list, 1, 0, "000");

  value = ilistGetStr (list, 1, 0);
  ck_assert_str_eq (value, "000");

  ilistSetStr (list, 2, 0, "111");
  ilistSetStr (list, 3, 0, "222");
  ilistSetStr (list, 4, 0, "333");

  value = ilistGetStr (list, 1, 0);
  ck_assert_str_eq (value, "000");

  ilistSetStr (list, 5, 0, "444");
  ilistSetStr (list, 6, 0, "555");

  ck_assert_int_eq (ilistGetCount (list), 6);

  ilistStartIterator (list, &iteridx);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "000");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2);
  value = ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "111");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 3);
  value = ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "222");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 4);
  value = ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "333");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 5);
  value = ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "444");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 6);
  value = ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "555");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, LIST_LOC_INVALID);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "000");

  ilistSetStr (list, 2, 0, "666");
  ilistSetStr (list, 3, 0, "777");
  ilistSetStr (list, 4, 0, "888");

  ilistStartIterator (list, &iteridx);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value =  ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "000");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2);
  value =  ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "666");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 3);
  value =  ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "777");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 4);
  value =  ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "888");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 5);
  value =  ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "444");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 6);
  value =  ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "555");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, LIST_LOC_INVALID);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value =  ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "000");

  ilistFree (list);
}
END_TEST

START_TEST(ilist_free_str)
{
  ilist_t        *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- ilist_free_str");
  mdebugSubTag ("ilist_free_str");

  list = ilistAlloc ("chk-g", LIST_ORDERED);
  ck_assert_ptr_nonnull (list);
  ilistSetStr (list, 6, 2, "0-2L");
  ilistSetStr (list, 6, 3, "0-3L");
  ilistSetStr (list, 26, 2, "1L");
  ilistSetStr (list, 18, 2, "2L");
  ilistSetStr (list, 11, 2, "3L");
  ilistSetStr (list, 3, 2, "4L");
  ilistSetStr (list, 1, 2, "5L");
  ilistSetStr (list, 2, 2, "6L");
  ck_assert_int_eq (ilistGetCount (list), 7);

  ilistFree (list);
}
END_TEST

START_TEST(ilist_exists)
{
  ilist_t   *list;
  bool      val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- ilist_exists");
  mdebugSubTag ("ilist_exists");

  list = ilistAlloc ("chk-g", LIST_ORDERED);
  ck_assert_ptr_nonnull (list);
  ilistSetStr (list, 6, 2, "0-2L");
  ilistSetStr (list, 6, 3, "0-3L");
  ilistSetStr (list, 26, 2, "1L");
  ilistSetStr (list, 18, 2, "2L");
  ilistSetStr (list, 11, 2, "3L");
  ilistSetStr (list, 3, 2, "4L");
  ilistSetStr (list, 1, 2, "5L");
  ilistSetStr (list, 2, 2, "6L");
  ck_assert_int_eq (ilistGetCount (list), 7);

  val = ilistExists (list, 6);
  ck_assert_int_eq (val, 1);
  val = ilistExists (list, 21);
  ck_assert_int_eq (val, 0);

  ilistFree (list);
}
END_TEST

START_TEST(ilist_delete)
{
  ilist_t     *list;
  bool        val;
  ilistidx_t  iteridx;
  ilistidx_t  key;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- ilist_delete");
  mdebugSubTag ("ilist_delete");

  list = ilistAlloc ("chk-g", LIST_ORDERED);
  ck_assert_ptr_nonnull (list);
  ilistSetStr (list, 6, 2, "0-2L");
  ilistSetStr (list, 6, 3, "0-3L");
  ilistSetStr (list, 26, 2, "1L");
  ilistSetStr (list, 18, 2, "2L");
  ilistSetStr (list, 11, 2, "3L");
  ilistSetStr (list, 3, 2, "4L");
  ilistSetStr (list, 1, 2, "5L");
  ilistSetStr (list, 2, 2, "6L");
  ck_assert_int_eq (ilistGetCount (list), 7);

  val = ilistExists (list, 6);
  ck_assert_int_eq (val, 1);
  val = ilistExists (list, 21);
  ck_assert_int_eq (val, 0);

  ilistDelete (list, 6);
  ck_assert_int_eq (ilistGetCount (list), 6);

  val = ilistExists (list, 6);
  ck_assert_int_eq (val, 0);
  val = ilistExists (list, 1);
  ck_assert_int_eq (val, 1);
  val = ilistExists (list, 2);
  ck_assert_int_eq (val, 1);
  val = ilistExists (list, 3);
  ck_assert_int_eq (val, 1);
  val = ilistExists (list, 11);
  ck_assert_int_eq (val, 1);
  val = ilistExists (list, 18);
  ck_assert_int_eq (val, 1);
  val = ilistExists (list, 26);
  ck_assert_int_eq (val, 1);

  ilistStartIterator (list, &iteridx);
  while ((key = ilistIterateKey (list, &iteridx)) >= 0) {
    ck_assert_int_ne (key, 6);
  }

  ilistFree (list);
}
END_TEST

START_TEST(ilist_bug_20231020)
{
  ilist_t     *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- ilist_bug_20231020");
  mdebugSubTag ("ilist_bug_20231020");

  list = ilistAlloc ("chk-h", LIST_ORDERED);
  ck_assert_ptr_nonnull (list);
  ilistSetNum (list, 6, 2, 6);
  ilistSetNum (list, 6, 3, 66);
  ilistSetNum (list, 26, 2, 26);
  ilistSetNum (list, 18, 2, 18);
  ilistSetNum (list, 11, 2, 11);
  ilistSetNum (list, 3, 2, 3);
  ilistSetNum (list, 1, 2, 1);
  ilistSetNum (list, 2, 2, 2);
  ck_assert_int_eq (ilistGetCount (list), 7);

  /* fetch a number that does not exist */
  ilistGetNum (list, 76, 2);
  ck_assert_int_eq (ilistGetCount (list), 7);

  ilistFree (list);
}
END_TEST

Suite *
ilist_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("ilist");
  tc = tcase_create ("ilist");
  tcase_set_tags (tc, "libbasic");
  tcase_add_test (tc, ilist_create_free);
  tcase_add_test (tc, ilist_get_data_str);
  tcase_add_test (tc, ilist_get_data_str_null);
  tcase_add_test (tc, ilist_get_data_str_sub);
  tcase_add_test (tc, ilist_iterate);
  tcase_add_test (tc, ilist_u_sort);
  tcase_add_test (tc, ilist_replace_str);
  tcase_add_test (tc, ilist_free_str);
  tcase_add_test (tc, ilist_exists);
  tcase_add_test (tc, ilist_delete);
  tcase_add_test (tc, ilist_bug_20231020);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
