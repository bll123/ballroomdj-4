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
#include "log.h"
#include "slist.h"

START_TEST(simple_list_create_free)
{
  slist_t    *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- simple_list_create_free");

  list = slistAlloc ("chk-a", LIST_UNORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (slistGetCount (list), 0);
  ck_assert_int_eq (slistGetAllocCount (list), 0);
  ck_assert_int_eq (slistGetOrdering (list), LIST_UNORDERED);
  slistFree (list);
}
END_TEST

START_TEST(simple_list_add_unordered)
{
  slist_t    *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- simple_list_add_unordered");


  list = slistAlloc ("chk-b", LIST_UNORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (slistGetOrdering (list), LIST_UNORDERED);
  slistSetStr (list, "ffff", NULL);
  ck_assert_int_eq (slistGetCount (list), 1);
  slistSetStr (list, "zzzz", NULL);
  ck_assert_int_eq (slistGetCount (list), 2);
  slistSetStr (list, "rrrr", NULL);
  ck_assert_int_eq (slistGetCount (list), 3);
  slistSetStr (list, "kkkk", NULL);
  ck_assert_int_eq (slistGetCount (list), 4);
  slistSetStr (list, "cccc", NULL);
  ck_assert_int_eq (slistGetCount (list), 5);
  slistSetStr (list, "aaaa", NULL);
  ck_assert_int_eq (slistGetCount (list), 6);
  slistSetStr (list, "bbbb", NULL);
  ck_assert_int_eq (slistGetCount (list), 7);
  ck_assert_int_eq (slistGetAllocCount (list), 10);
  slistFree (list);
}
END_TEST

START_TEST(simple_list_add_unordered_iterate)
{
  slist_t *  list;
  char *    value;
  slistidx_t  iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- simple_list_add_unordered_iterate");


  list = slistAlloc ("chk-c", LIST_UNORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  slistSetStr (list, "ffff", NULL);
  slistSetStr (list, "zzzz", NULL);
  slistSetStr (list, "rrrr", NULL);
  slistSetStr (list, "kkkk", NULL);
  slistSetStr (list, "cccc", NULL);
  slistSetStr (list, "aaaa", NULL);
  slistSetStr (list, "bbbb", NULL);
  slistStartIterator (list, &iteridx);
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "ffff");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "zzzz");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "rrrr");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "kkkk");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "cccc");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_ptr_null (value);
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "ffff");
  slistFree (list);
}
END_TEST

START_TEST(simple_list_add_ordered_iterate)
{
  slist_t *  list;
  char *    value;
  slistidx_t  iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- simple_list_add_ordered_iterate");


  list = slistAlloc ("chk-d", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (slistGetOrdering (list), LIST_ORDERED);
  slistSetStr (list, "ffff", NULL);
  slistSetStr (list, "zzzz", NULL);
  slistSetStr (list, "rrrr", NULL);
  slistSetStr (list, "kkkk", NULL);
  slistSetStr (list, "cccc", NULL);
  slistSetStr (list, "aaaa", NULL);
  slistSetStr (list, "bbbb", NULL);
  slistStartIterator (list, &iteridx);
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "cccc");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "ffff");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "kkkk");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "rrrr");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "zzzz");
  slistFree (list);
}
END_TEST

START_TEST(simple_list_add_ordered_beg)
{
  slist_t *  list;
  char *    value;
  slistidx_t  iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- simple_list_add_ordered_beg");


  list = slistAlloc ("chk-e", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  slistSetStr (list, "cccc", NULL);
  ck_assert_int_eq (slistGetCount (list), 1);
  slistSetStr (list, "bbbb", NULL);
  ck_assert_int_eq (slistGetCount (list), 2);
  slistSetStr (list, "aaaa", NULL);
  ck_assert_int_eq (slistGetCount (list), 3);
  ck_assert_int_eq (slistGetAllocCount (list), 5);
  slistStartIterator (list, &iteridx);
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "cccc");
  slistFree (list);
}
END_TEST

START_TEST(simple_list_add_ordered_end)
{
  slist_t *  list;
  char *    value;
  slistidx_t  iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- simple_list_add_ordered_end");


  list = slistAlloc ("chk-f", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  slistSetStr (list, "aaaa", NULL);
  ck_assert_int_eq (slistGetCount (list), 1);
  slistSetStr (list, "bbbb", NULL);
  ck_assert_int_eq (slistGetCount (list), 2);
  slistSetStr (list, "cccc", NULL);
  ck_assert_int_eq (slistGetCount (list), 3);
  ck_assert_int_eq (slistGetAllocCount (list), 5);
  slistStartIterator (list, &iteridx);
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "cccc");
  slistFree (list);
}
END_TEST

START_TEST(simple_list_add_ordered_prealloc)
{
  slist_t *  list;
  char *    value;
  slistidx_t  iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- simple_list_add_ordered_prealloc");


  list = slistAlloc ("chk-g", LIST_ORDERED, NULL);
  slistSetSize (list, 7);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (slistGetCount (list), 0);
  ck_assert_int_eq (slistGetAllocCount (list), 7);
  slistSetStr (list, "ffff", NULL);
  slistSetStr (list, "zzzz", NULL);
  slistSetStr (list, "rrrr", NULL);
  slistSetStr (list, "kkkk", NULL);
  slistSetStr (list, "cccc", NULL);
  slistSetStr (list, "aaaa", NULL);
  slistSetStr (list, "bbbb", NULL);
  ck_assert_int_eq (slistGetCount (list), 7);
  ck_assert_int_eq (slistGetAllocCount (list), 7);
  slistStartIterator (list, &iteridx);
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "cccc");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "ffff");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "kkkk");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "rrrr");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "zzzz");
  slistFree (list);
}
END_TEST

START_TEST(simple_list_add_sort)
{
  slist_t *  list;
  char *    value;
  slistidx_t  iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- simple_list_add_sort");


  list = slistAlloc ("chk-h", LIST_UNORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  slistSetStr (list, "ffff", NULL);
  slistSetStr (list, "zzzz", NULL);
  slistSetStr (list, "rrrr", NULL);
  slistSetStr (list, "kkkk", NULL);
  slistSetStr (list, "cccc", NULL);
  slistSetStr (list, "aaaa", NULL);
  slistSetStr (list, "bbbb", NULL);
  ck_assert_int_eq (slistGetCount (list), 7);
  ck_assert_int_eq (slistGetAllocCount (list), 10);
  slistStartIterator (list, &iteridx);
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "ffff");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "zzzz");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "rrrr");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "kkkk");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "cccc");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "bbbb");
  slistSort (list);
  ck_assert_int_eq (slistGetOrdering (list), LIST_ORDERED);
  ck_assert_int_eq (slistGetCount (list), 7);
  slistStartIterator (list, &iteridx);
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "cccc");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "ffff");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "kkkk");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "rrrr");
  value = (char *) slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "zzzz");
  slistFree (list);
}
END_TEST

START_TEST(slist_get_data_str)
{
  slist_t        *list;
  char          *value;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- slist_get_data_str");


  list = slistAlloc ("chk-i", LIST_UNORDERED, NULL);
  slistSetSize (list, 7);
  ck_assert_int_eq (slistGetCount (list), 0);
  ck_assert_int_eq (slistGetAllocCount (list), 7);
  slistSetStr (list, "ffff", "0L");
  slistSetStr (list, "zzzz", "1L");
  slistSetStr (list, "rrrr", "2L");
  slistSetStr (list, "kkkk", "3L");
  slistSetStr (list, "cccc", "4L");
  slistSetStr (list, "aaaa", "5L");
  slistSetStr (list, "bbbb", "6L");
  ck_assert_int_eq (slistGetCount (list), 7);
  slistSort (list);
  ck_assert_int_eq (slistGetCount (list), 7);
  value = (char *) slistGetStr (list, "cccc");
  ck_assert_ptr_nonnull (value);
  ck_assert_str_eq (value, "4L");
  slistFree (list);
}
END_TEST

START_TEST(slist_get_data_num)
{
  slist_t *list;
  int     val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- slist_get_data_num");


  list = slistAlloc ("chk-i", LIST_UNORDERED, NULL);
  slistSetSize (list, 7);
  ck_assert_int_eq (slistGetCount (list), 0);
  ck_assert_int_eq (slistGetAllocCount (list), 7);
  slistSetNum (list, "ffff", 10);
  slistSetNum (list, "zzzz", 11);
  slistSetNum (list, "rrrr", 12);
  slistSetNum (list, "kkkk", 13);
  slistSetNum (list, "cccc", 14);
  slistSetNum (list, "aaaa", 15);
  slistSetNum (list, "bbbb", 16);
  ck_assert_int_eq (slistGetCount (list), 7);
  slistSort (list);
  ck_assert_int_eq (slistGetCount (list), 7);
  val = slistGetNum (list, "cccc");
  ck_assert_int_eq (val, 14);
  val = slistGetNum (list, "aaaa");
  ck_assert_int_eq (val, 15);
  val = slistGetNum (list, "zzzz");
  ck_assert_int_eq (val, 11);
  val = slistGetNum (list, "bbbb");
  ck_assert_int_eq (val, 16);
  val = slistGetNum (list, "ffff");
  ck_assert_int_eq (val, 10);
  val = slistGetNum (list, "rrrr");
  ck_assert_int_eq (val, 12);
  val = slistGetNum (list, "kkkk");
  ck_assert_int_eq (val, 13);
  val = slistGetNum (list, "abcd");
  ck_assert_int_eq (val, LIST_VALUE_INVALID);
  val = slistGetNum (list, NULL);
  ck_assert_int_eq (val, LIST_VALUE_INVALID);
  slistFree (list);
}
END_TEST

START_TEST(slist_add_str_iterate)
{
  slist_t *     list;
  const char *  value;
  const char *  key;
  slistidx_t    iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- slist_add_str_iterate");


  list = slistAlloc ("chk-j", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  slistSetStr (list, "ffff", "555");
  slistSetStr (list, "cccc", "222");
  slistSetStr (list, "eeee", "444");
  slistSetStr (list, "dddd", "333");
  slistSetStr (list, "aaaa", "000");
  slistSetStr (list, "bbbb", "111");
  slistStartIterator (list, &iteridx);
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "000");
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "bbbb");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "111");
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "cccc");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "222");
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "dddd");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "333");
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "eeee");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "444");
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "ffff");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "555");
  key = slistIterateKey (list, &iteridx);
  ck_assert_ptr_null (key);
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "000");
  slistFree (list);
}
END_TEST

START_TEST(slist_add_sort_str)
{
  slist_t *      list;
  const char *  key;
  ssize_t          value;
  slistidx_t    iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- slist_add_sort_str");


  list = slistAlloc ("chk-k", LIST_UNORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  slistSetNum (list, "ffff", 0L);
  slistSetNum (list, "zzzz", 1L);
  slistSetNum (list, "rrrr", 2L);
  slistSetNum (list, "kkkk", 3L);
  slistSetNum (list, "cccc", 4L);
  slistSetNum (list, "aaaa", 5L);
  slistSetNum (list, "bbbb", 6L);
  ck_assert_int_eq (slistGetCount (list), 7);
  ck_assert_int_eq (slistGetAllocCount (list), 10);

  slistStartIterator (list, &iteridx);
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "ffff");
  value = slistGetNum (list, key);
  ck_assert_int_eq (value, 0L);
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "zzzz");
  value = slistGetNum (list, key);
  ck_assert_int_eq (value, 1L);

  slistSort (list);
  ck_assert_int_eq (slistGetOrdering (list), LIST_ORDERED);
  ck_assert_int_eq (slistGetCount (list), 7);
  slistStartIterator (list, &iteridx);
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "aaaa");
  value = slistGetNum (list, key);
  ck_assert_int_eq (value, 5L);
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "bbbb");
  value = slistGetNum (list, key);
  ck_assert_int_eq (value, 6L);

  slistFree (list);
}
END_TEST

START_TEST(slist_replace_str)
{
  slist_t        *list;
  const char *  key;
  const char *  value;
  slistidx_t    iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- slist_replace_str");


  list = slistAlloc ("chk-l", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);

  slistSetStr (list, "aaaa", "000");

  value = slistGetStr (list, "aaaa");
  ck_assert_str_eq (value, "000");

  slistSetStr (list, "bbbb", "111");
  slistSetStr (list, "cccc", "222");
  slistSetStr (list, "dddd", "333");

  value = slistGetStr (list, "aaaa");
  ck_assert_str_eq (value, "000");

  slistSetStr (list, "eeee", "444");
  slistSetStr (list, "ffff", "555");

  ck_assert_int_eq (slistGetCount (list), 6);

  slistStartIterator (list, &iteridx);
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "000");
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "bbbb");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "111");
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "cccc");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "222");
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "dddd");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "333");
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "eeee");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "444");
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "ffff");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "555");
  key = slistIterateKey (list, &iteridx);
  ck_assert_ptr_null (key);
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "000");

  slistSetStr (list, "bbbb", "666");
  slistSetStr (list, "cccc", "777");
  slistSetStr (list, "dddd", "888");

  slistStartIterator (list, &iteridx);
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "000");
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "bbbb");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "666");
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "cccc");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "777");
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "dddd");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "888");
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "eeee");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "444");
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "ffff");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "555");
  key = slistIterateKey (list, &iteridx);
  ck_assert_ptr_null (key);
  key = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) slistGetStr (list, key);
  ck_assert_str_eq (value, "000");

  slistFree (list);
}
END_TEST

START_TEST(slist_free_str)
{
  slist_t        *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- slist_free_str");

  list = slistAlloc ("chk-m", LIST_UNORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  slistSetStr (list, "ffff", "0L");
  slistSetStr (list, "zzzz", "1L");
  slistSetStr (list, "rrrr", "2L");
  slistSetStr (list, "kkkk", "3L");
  slistSetStr (list, "cccc", "4L");
  slistSetStr (list, "aaaa", "5L");
  slistSetStr (list, "bbbb", "6L");
  ck_assert_int_eq (slistGetCount (list), 7);
  slistFree (list);
}
END_TEST

Suite *
slist_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("slist");
  tc = tcase_create ("slist");
  tcase_set_tags (tc, "libbasic");
  tcase_add_test (tc, simple_list_create_free);
  tcase_add_test (tc, simple_list_add_unordered);
  tcase_add_test (tc, simple_list_add_unordered_iterate);
  tcase_add_test (tc, simple_list_add_ordered_iterate);
  tcase_add_test (tc, simple_list_add_ordered_beg);
  tcase_add_test (tc, simple_list_add_ordered_end);
  tcase_add_test (tc, simple_list_add_ordered_prealloc);
  tcase_add_test (tc, simple_list_add_sort);
  tcase_add_test (tc, slist_get_data_str);
  tcase_add_test (tc, slist_get_data_num);
  tcase_add_test (tc, slist_add_str_iterate);
  tcase_add_test (tc, slist_add_sort_str);
  tcase_add_test (tc, slist_replace_str);
  tcase_add_test (tc, slist_free_str);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
