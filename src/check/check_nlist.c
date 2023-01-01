/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#pragma clang diagnostic push
#pragma gcc diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdjstring.h"
#include "check_bdj.h"
#include "log.h"
#include "nlist.h"

typedef struct {
  int     alloc;
  char    *value;
} chk_item_t;

static chk_item_t *allocItem (const char *str);
static void freeItem (void *item);

/*
  nlistAlloc          //
  nlistFree           //
  nlistSetVersion     //
  nlistGetVersion     //
  nlistGetCount       //
  nlistSetSize        //
  nlistSetFreeHook    /
  nlistSetData        //
  nlistSetStr         //
  nlistSetNum         //
  nlistSetDouble      //
  nlistSetList        //
  nlistIncrement
  nlistDecrement
  nlistGetData
  nlistGetStr         /s
  nlistGetIdx
  nlistGetDataByIdx   /u
  nlistGetNumByIdx    /u
  nlistGetKeyByIdx    /u
  nlistGetNum         /
  nlistGetDouble      /
  nlistGetList
  nlistStartIterator        /us
  nlistIterateKey           /us
  nlistIterateKeyPrevious   /
  nlistIterateValueData     /us
  nlistIterateValueNum      /
  nlistSort                 /
  nlistDumpInfo
  nlistSearchProbTable      /
*/

START_TEST(nlist_create_free)
{
  nlist_t    *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_create_free");

  list = nlistAlloc ("chk-a", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 0);
  ck_assert_int_eq (list->ordered, LIST_ORDERED);
  ck_assert_int_eq (list->keytype, LIST_KEY_NUM);
  nlistFree (list);
}
END_TEST

START_TEST(nlist_version)
{
  nlist_t *list;
  int     vers;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_version");

  list = nlistAlloc ("chk-b", LIST_ORDERED, NULL);
  vers = nlistGetVersion (list);
  ck_assert_int_eq (vers, 1);

  nlistSetVersion (list, 20);
  ck_assert_int_eq (list->version, 20);

  vers = nlistGetVersion (list);
  ck_assert_int_eq (vers, 20);

  nlistFree (list);
}
END_TEST

START_TEST(nlist_set_size)
{
  nlist_t        *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_set_size");

  list = nlistAlloc ("chk-c", LIST_UNORDERED, NULL);
  nlistSetSize (list, 7);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 7);
  nlistFree (list);
}
END_TEST

START_TEST(nlist_get_count)
{
  nlist_t        *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_get_count");

  list = nlistAlloc ("chk-d", LIST_UNORDERED, NULL);
  nlistSetSize (list, 7);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (nlistGetCount (list), 0);
  ck_assert_int_eq (list->allocCount, 7);
  nlistFree (list);
}
END_TEST

/* unordered set */
START_TEST(nlist_u_set_num)
{
  nlist_t        *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_u_set_num");

  list = nlistAlloc ("chk-e", LIST_UNORDERED, NULL);
  nlistSetSize (list, 7);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 7);
  nlistSetNum (list, 6, 0);
  nlistSetNum (list, 26, 1);
  nlistSetNum (list, 18, 2);
  ck_assert_int_eq (nlistGetCount (list), 3);
  nlistSetNum (list, 11, 3);
  nlistSetNum (list, 3, 4);
  nlistSetNum (list, 1, 5);
  nlistSetNum (list, 2, 6);
  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (nlistGetCount (list), 7);
  ck_assert_int_eq (list->allocCount, 7);
  nlistFree (list);
}
END_TEST

/* unordered set */
START_TEST(nlist_u_set_double)
{
  nlist_t        *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_u_set_double");

  list = nlistAlloc ("chk-f", LIST_UNORDERED, NULL);
  nlistSetSize (list, 7);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 7);
  nlistSetDouble (list, 6, 0.0);
  nlistSetDouble (list, 26, 1.0);
  nlistSetDouble (list, 18, 2.0);
  ck_assert_int_eq (nlistGetCount (list), 3);
  nlistSetDouble (list, 11, 3.0);
  nlistSetDouble (list, 3, 4.0);
  nlistSetDouble (list, 1, 5.0);
  nlistSetDouble (list, 2, 6.0);
  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (nlistGetCount (list), 7);
  ck_assert_int_eq (list->allocCount, 7);
  nlistFree (list);
}
END_TEST

/* unordered set */
/* data is static, not freed */
START_TEST(nlist_u_set_data_static)
{
  nlist_t        *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_u_set_data_static");

  list = nlistAlloc ("chk-g", LIST_UNORDERED, NULL);
  nlistSetSize (list, 7);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 7);
  nlistSetData (list, 6, "0L");
  nlistSetData (list, 26, "1L");
  nlistSetData (list, 18, "2L");
  ck_assert_int_eq (nlistGetCount (list), 3);
  nlistSetData (list, 11, "3L");
  nlistSetData (list, 3, "4L");
  nlistSetData (list, 1, "5L");
  nlistSetData (list, 2, "6L");
  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (nlistGetCount (list), 7);
  ck_assert_int_eq (list->allocCount, 7);
  nlistFree (list);
}
END_TEST

/* unordered set */
/* note that the strings are duplicated and freed */
START_TEST(nlist_u_set_str)
{
  nlist_t        *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_u_set_str");

  list = nlistAlloc ("chk-h", LIST_UNORDERED, NULL);
  nlistSetSize (list, 7);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 7);
  nlistSetStr (list, 6, "0L");
  nlistSetStr (list, 26, "1L");
  nlistSetStr (list, 18, "2L");
  ck_assert_int_eq (nlistGetCount (list), 3);
  nlistSetStr (list, 11, "3L");
  nlistSetStr (list, 3, "4L");
  nlistSetStr (list, 1, "5L");
  nlistSetStr (list, 2, "6L");
  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (nlistGetCount (list), 7);
  ck_assert_int_eq (list->allocCount, 7);
  nlistFree (list);
}
END_TEST

/* unordered set */
START_TEST(nlist_u_set_list)
{
  nlist_t        *list;
  nlist_t        *tlist;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_u_set_list");

  list = nlistAlloc ("chk-i", LIST_UNORDERED, NULL);
  nlistSetSize (list, 7);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 7);
  tlist = nlistAlloc ("chk-i1", LIST_UNORDERED, NULL);
  nlistSetNum (tlist, 1, 7);
  nlistSetList (list, 6, tlist);
  tlist = nlistAlloc ("chk-i2", LIST_UNORDERED, NULL);
  nlistSetNum (tlist, 1, 7);
  nlistSetData (list, 26, tlist);
  tlist = nlistAlloc ("chk-i3", LIST_UNORDERED, NULL);
  nlistSetNum (tlist, 1, 7);
  nlistSetData (list, 18, tlist);
  ck_assert_int_eq (nlistGetCount (list), 3);
  tlist = nlistAlloc ("chk-i4", LIST_UNORDERED, NULL);
  nlistSetNum (tlist, 1, 7);
  nlistSetData (list, 11, tlist);
  tlist = nlistAlloc ("chk-i5", LIST_UNORDERED, NULL);
  nlistSetNum (tlist, 1, 7);
  nlistSetData (list, 3, tlist);
  nlistSetData (list, 1, tlist);
  tlist = nlistAlloc ("chk-i6", LIST_UNORDERED, NULL);
  nlistSetNum (tlist, 1, 7);
  nlistSetData (list, 2, tlist);
  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (nlistGetCount (list), 7);
  ck_assert_int_eq (list->allocCount, 7);
  nlistFree (list);
}
END_TEST

START_TEST(nlist_u_set_str_no_size)
{
  nlist_t        *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_u_set_str_no_size");

  list = nlistAlloc ("chk-aa", LIST_UNORDERED, NULL);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 0);
  nlistSetStr (list, 6, "0L");
  ck_assert_int_eq (list->allocCount, 5);
  nlistSetStr (list, 26, "1L");
  ck_assert_int_eq (list->allocCount, 5);
  nlistSetStr (list, 18, "2L");
  ck_assert_int_eq (list->allocCount, 5);
  nlistSetStr (list, 11, "3L");
  ck_assert_int_eq (list->allocCount, 5);
  nlistSetStr (list, 3, "4L");
  ck_assert_int_eq (list->allocCount, 5);
  nlistSetStr (list, 1, "5L");
  ck_assert_int_eq (list->allocCount, 10);
  nlistSetStr (list, 2, "6L");
  ck_assert_int_eq (list->allocCount, 10);
  ck_assert_int_eq (list->count, 7);
  nlistFree (list);
}
END_TEST

START_TEST(nlist_u_getbyidx)
{
  nlist_t       *list;
  const char    *value;
  int           tval;
  ssize_t       key;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_u_getbyidx");

  list = nlistAlloc ("chk-bb", LIST_UNORDERED, NULL);
  nlistSetSize (list, 7);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 7);
  nlistSetStr (list, 6, "0L");
  nlistSetStr (list, 26, "1L");
  nlistSetStr (list, 18, "2L");
  nlistSetStr (list, 11, "3L");
  nlistSetStr (list, 3, "4L");
  nlistSetStr (list, 1, "5L");
  nlistSetStr (list, 2, "6L");
  nlistSetNum (list, 14, 7);
  nlistSetNum (list, 8, 8);
  nlistSetNum (list, 9, 9);
  ck_assert_int_eq (list->count, 10);

  /* unordered */
  key = nlistGetKeyByIdx (list, 0);
  ck_assert_int_eq (key, 6);
  key = nlistGetKeyByIdx (list, 4);
  ck_assert_int_eq (key, 3);

  value = nlistGetDataByIdx (list, 1);
  ck_assert_str_eq (value, "1L");
  value = nlistGetDataByIdx (list, 5);
  ck_assert_str_eq (value, "5L");

  tval = nlistGetNumByIdx (list, 9);
  ck_assert_int_eq (tval, 9);
  tval = nlistGetNumByIdx (list, 7);
  ck_assert_int_eq (tval, 7);

  nlistFree (list);
}
END_TEST

START_TEST(nlist_u_iterate)
{
  nlist_t       *list;
  nlistidx_t    iteridx;
  char          *value;
  ssize_t       key;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_u_iterate");

  list = nlistAlloc ("chk-cc", LIST_UNORDERED, NULL);
  nlistSetSize (list, 7);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 7);
  nlistSetStr (list, 6, "0L");
  nlistSetStr (list, 26, "1L");
  nlistSetStr (list, 18, "2L");
  nlistSetStr (list, 11, "3L");
  nlistSetStr (list, 3, "4L");
  nlistSetStr (list, 1, "5L");
  nlistSetStr (list, 2, "6L");
  ck_assert_int_eq (list->count, 7);

  nlistStartIterator (list, &iteridx);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 6);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 26);

  nlistStartIterator (list, &iteridx);
  value = nlistIterateValueData (list, &iteridx);
  ck_assert_str_eq (value, "0L");
  value = nlistIterateValueData (list, &iteridx);
  ck_assert_str_eq (value, "1L");
  value = nlistIterateValueData (list, &iteridx);
  ck_assert_str_eq (value, "2L");
  value = nlistIterateValueData (list, &iteridx);
  ck_assert_str_eq (value, "3L");
  value = nlistIterateValueData (list, &iteridx);
  ck_assert_str_eq (value, "4L");
  value = nlistIterateValueData (list, &iteridx);
  ck_assert_str_eq (value, "5L");
  value = nlistIterateValueData (list, &iteridx);
  ck_assert_str_eq (value, "6L");
  value = nlistIterateValueData (list, &iteridx);
  ck_assert_ptr_null (value);
  value = nlistIterateValueData (list, &iteridx);
  ck_assert_str_eq (value, "0L");

  nlistFree (list);
}
END_TEST

START_TEST(nlist_s_set_size_sort)
{
  nlist_t        *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_s_set_size_sort");

  list = nlistAlloc ("chk-dd", LIST_UNORDERED, NULL);
  nlistSetSize (list, 7);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 7);
  nlistSetStr (list, 6, "0L");
  nlistSetStr (list, 26, "1L");
  nlistSetStr (list, 18, "2L");
  nlistSetStr (list, 11, "3L");
  nlistSetStr (list, 3, "4L");
  nlistSetStr (list, 1, "5L");
  nlistSetStr (list, 2, "6L");
  ck_assert_int_eq (list->count, 7);
  nlistSort (list);

  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (list->allocCount, 7);
  nlistFree (list);
}
END_TEST

START_TEST(nlist_s_no_size_sort)
{
  nlist_t        *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_s_no_size_sort");

  list = nlistAlloc ("chk-dd-bb", LIST_UNORDERED, NULL);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 0);
  nlistSetStr (list, 6, "0L");
  ck_assert_int_eq (list->allocCount, 5);
  nlistSetStr (list, 26, "1L");
  nlistSetStr (list, 18, "2L");
  nlistSetStr (list, 11, "3L");
  nlistSetStr (list, 3, "4L");
  nlistSetStr (list, 1, "5L");
  nlistSetStr (list, 2, "6L");
  ck_assert_int_eq (list->allocCount, 10);
  ck_assert_int_eq (list->count, 7);
  nlistSort (list);

  ck_assert_int_eq (list->count, 7);
  nlistFree (list);
}
END_TEST

START_TEST(nlist_s_ordered)
{
  nlist_t        *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_s_ordered");

  list = nlistAlloc ("chk-dd-cc", LIST_ORDERED, NULL);
  ck_assert_int_eq (list->count, 0);
  nlistSetStr (list, 6, "0L");
  nlistSetStr (list, 26, "1L");
  nlistSetStr (list, 18, "2L");
  nlistSetStr (list, 11, "3L");
  nlistSetStr (list, 3, "4L");
  nlistSetStr (list, 1, "5L");
  nlistSetStr (list, 2, "6L");

  ck_assert_int_eq (list->count, 7);
  nlistFree (list);
}
END_TEST

START_TEST(nlist_s_get_str)
{
  nlist_t        *list;
  const char     *value;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_s_get_str");

  list = nlistAlloc ("chk-dd-dd", LIST_UNORDERED, NULL);
  nlistSetSize (list, 7);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 7);
  nlistSetStr (list, 6, "0L");
  nlistSetStr (list, 26, "1L");
  nlistSetStr (list, 18, "2L");
  nlistSetStr (list, 11, "3L");
  nlistSetStr (list, 3, "4L");
  nlistSetStr (list, 1, "5L");
  nlistSetStr (list, 2, "6L");
  ck_assert_int_eq (list->count, 7);
  nlistSort (list);

  ck_assert_int_eq (list->count, 7);
  value = nlistGetStr (list, 3);
  ck_assert_ptr_nonnull (value);
  ck_assert_str_eq (value, "4L");
  value = nlistGetStr (list, 1);
  ck_assert_str_eq (value, "5L");
  value = nlistGetStr (list, 26);
  ck_assert_str_eq (value, "1L");
  nlistFree (list);
}
END_TEST

START_TEST(nlist_s_cache_bug_20221013)
{
  nlist_t        *list;
  const char     *value;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_s_cache_bug_20221013");

  list = nlistAlloc ("chk-ee", LIST_ORDERED, NULL);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 0);
  nlistSetStr (list, 6, "0L");
  nlistSetStr (list, 26, "1L");
  nlistSetStr (list, 18, "2L");
  nlistSetStr (list, 11, "3L");
  nlistSetStr (list, 3, "4L");
  nlistSetStr (list, 1, "5L");
  nlistSetStr (list, 2, "6L");
  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (list->allocCount, 10);

  value = nlistGetStr (list, 3);
  ck_assert_ptr_nonnull (value);
  ck_assert_str_eq (value, "4L");
  /* should be in cache */
  ck_assert_int_eq (listDebugIsCached (list, 3), 1);

  nlistSetStr (list, 14, "7L");
  /* should not be in cache after an insert */
  ck_assert_int_eq (listDebugIsCached (list, 3), 0);

  nlistFree (list);
}
END_TEST

START_TEST(nlist_s_iterate_str)
{
  nlist_t *      list;
  char *        value;
  nlistidx_t          key;
  nlistidx_t    iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_s_iterate_str");

  list = nlistAlloc ("chk-ff", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  nlistSetStr (list, 6, "555");
  nlistSetStr (list, 3, "222");
  nlistSetStr (list, 5, "444");
  nlistSetStr (list, 4, "333");
  nlistSetStr (list, 1, "000");
  nlistSetStr (list, 2, "111");

  nlistStartIterator (list, &iteridx);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "111");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 3);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "222");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 4);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "333");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 5);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "444");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 6);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "555");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, -1);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");

  nlistStartIterator (list, &iteridx);
  value = nlistIterateValueData (list, &iteridx);
  ck_assert_str_eq (value, "000");
  value = nlistIterateValueData (list, &iteridx);
  ck_assert_str_eq (value, "111");
  value = nlistIterateValueData (list, &iteridx);
  ck_assert_str_eq (value, "222");
  value = nlistIterateValueData (list, &iteridx);
  ck_assert_str_eq (value, "333");
  value = nlistIterateValueData (list, &iteridx);
  ck_assert_str_eq (value, "444");
  value = nlistIterateValueData (list, &iteridx);
  ck_assert_str_eq (value, "555");
  value = nlistIterateValueData (list, &iteridx);
  ck_assert_ptr_null (value);
  value = nlistIterateValueData (list, &iteridx);
  ck_assert_str_eq (value, "000");

  nlistFree (list);
}
END_TEST

START_TEST(nlist_set_get_num)
{
  nlist_t *      list;
  ssize_t          value;
  nlistidx_t          key;
  nlistidx_t      iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_set_get_num");

  list = nlistAlloc ("chk-gg", LIST_UNORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  nlistSetNum (list, 6, 0);
  nlistSetNum (list, 26, 1);
  nlistSetNum (list, 18, 2);
  nlistSetNum (list, 11, 3);
  nlistSetNum (list, 3, 4);
  nlistSetNum (list, 1, 5);
  nlistSetNum (list, 2, 6);
  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (list->allocCount, 10);

  /* unordered */
  nlistStartIterator (list, &iteridx);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 6);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 0);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 26);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 1);

  nlistSort (list);
  ck_assert_int_eq (list->ordered, LIST_ORDERED);
  ck_assert_int_eq (list->count, 7);

  /* ordered */
  nlistStartIterator (list, &iteridx);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 5);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 6);

  nlistStartIterator (list, &iteridx);
  value = nlistIterateValueNum (list, &iteridx);
  ck_assert_int_eq (value, 5);
  value = nlistIterateValueNum (list, &iteridx);
  ck_assert_int_eq (value, 6);
  value = nlistIterateValueNum (list, &iteridx);
  ck_assert_int_eq (value, 4);
  value = nlistIterateValueNum (list, &iteridx);
  ck_assert_int_eq (value, 0);
  value = nlistIterateValueNum (list, &iteridx);
  ck_assert_int_eq (value, 3);
  value = nlistIterateValueNum (list, &iteridx);
  ck_assert_int_eq (value, 2);
  value = nlistIterateValueNum (list, &iteridx);
  ck_assert_int_eq (value, 1);
  value = nlistIterateValueNum (list, &iteridx);
  ck_assert_int_eq (value, LIST_VALUE_INVALID);
  value = nlistIterateValueNum (list, &iteridx);
  ck_assert_int_eq (value, 5);

  nlistFree (list);
}
END_TEST

START_TEST(nlist_s_iterate_num)
{
  nlist_t *     list;
  nlistidx_t    value;
  nlistidx_t    key;
  nlistidx_t    iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_s_iterate_num");

  list = nlistAlloc ("chk-hh", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  nlistSetNum (list, 6, 555);
  nlistSetNum (list, 3, 222);
  nlistSetNum (list, 5, 444);
  nlistSetNum (list, 4, 333);
  nlistSetNum (list, 1, 0);
  nlistSetNum (list, 2, 111);

  nlistStartIterator (list, &iteridx);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 0);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 111);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 3);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 222);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 4);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 333);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 5);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 444);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 6);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 555);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, -1);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 0);

  nlistStartIterator (list, &iteridx);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 0);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 111);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 3);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 222);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 4);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 333);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 5);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 444);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 6);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 555);

  key = nlistIterateKeyPrevious (list, &iteridx);
  ck_assert_int_eq (key, 5);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 444);
  key = nlistIterateKeyPrevious (list, &iteridx);
  ck_assert_int_eq (key, 4);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 333);
  key = nlistIterateKeyPrevious (list, &iteridx);
  ck_assert_int_eq (key, 3);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 222);
  key = nlistIterateKeyPrevious (list, &iteridx);
  ck_assert_int_eq (key, 2);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 111);
  key = nlistIterateKeyPrevious (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 0);
  /* previous does not wrap at this time */
  key = nlistIterateKeyPrevious (list, &iteridx);
  ck_assert_int_eq (key, -1);
  key = nlistIterateKeyPrevious (list, &iteridx);
  ck_assert_int_eq (key, -1);

  nlistStartIterator (list, &iteridx);
  value = nlistIterateValueNum (list, &iteridx);
  ck_assert_int_eq (value, 0);
  value = nlistIterateValueNum (list, &iteridx);
  ck_assert_int_eq (value, 111);
  value = nlistIterateValueNum (list, &iteridx);
  ck_assert_int_eq (value, 222);
  value = nlistIterateValueNum (list, &iteridx);
  ck_assert_int_eq (value, 333);
  value = nlistIterateValueNum (list, &iteridx);
  ck_assert_int_eq (value, 444);
  value = nlistIterateValueNum (list, &iteridx);
  ck_assert_int_eq (value, 555);
  value = nlistIterateValueNum (list, &iteridx);
  ck_assert_int_eq (value, LIST_VALUE_INVALID);
  value = nlistIterateValueNum (list, &iteridx);
  ck_assert_int_eq (value, 0);

  nlistFree (list);
}
END_TEST

START_TEST(nlist_s_replace_str)
{
  nlist_t       *list;
  nlistidx_t    key;
  const char    *value;
  nlistidx_t    iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_s_replace_str");

  list = nlistAlloc ("chk-ii", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);

  nlistSetStr (list, 1, "000");

  value = nlistGetStr (list, 1);
  ck_assert_str_eq (value, "000");

  nlistSetStr (list, 2, "111");
  nlistSetStr (list, 3, "222");
  nlistSetStr (list, 4, "333");

  value = nlistGetStr (list, 1);
  ck_assert_str_eq (value, "000");

  nlistSetStr (list, 5, "444");
  nlistSetStr (list, 6, "555");

  ck_assert_int_eq (list->count, 6);

  nlistStartIterator (list, &iteridx);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "111");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 3);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "222");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 4);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "333");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 5);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "444");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 6);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "555");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, -1);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");

  nlistSetStr (list, 2, "666");
  nlistSetStr (list, 3, "777");
  nlistSetStr (list, 4, "888");

  nlistStartIterator (list, &iteridx);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "666");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 3);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "777");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 4);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "888");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 5);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "444");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 6);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "555");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, -1);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");

  nlistFree (list);
}
END_TEST

START_TEST(nlist_free_str)
{
  nlist_t        *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_free_str");


  list = nlistAlloc ("chk-jj", LIST_UNORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  nlistSetStr (list, 6, "0L");
  nlistSetStr (list, 26, "1L");
  nlistSetStr (list, 18, "2L");
  nlistSetStr (list, 11, "3L");
  nlistSetStr (list, 3, "4L");
  nlistSetStr (list, 1, "5L");
  nlistSetStr (list, 2, "6L");
  ck_assert_int_eq (list->count, 7);
  nlistFree (list);
}
END_TEST

/* exercises set-data */
START_TEST(nlist_free_item)
{
  nlist_t       *list;
  chk_item_t    *item [20];
  int           itemc = 0;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_free_item");


  list = nlistAlloc ("chk-kk", LIST_UNORDERED, freeItem);
  nlistSetFreeHook (list, freeItem);
  ck_assert_ptr_nonnull (list);

  item [itemc] = allocItem ("0L");
  nlistSetData (list, 6, item [itemc]);
  ++itemc;

  item [itemc] = allocItem ("1L");
  nlistSetData (list, 26, item [itemc]);
  ++itemc;

  item [itemc] = allocItem ("2L");
  nlistSetData (list, 18, item [itemc]);
  ++itemc;

  item [itemc] = allocItem ("3L");
  nlistSetData (list, 11, item [itemc]);
  ++itemc;

  item [itemc] = allocItem ("4L");
  nlistSetData (list, 3, item [itemc]);
  ++itemc;

  item [itemc] = allocItem ("5L");
  nlistSetData (list, 1, item [itemc]);
  ++itemc;

  item [itemc] = allocItem ("6L");
  nlistSetData (list, 2, item [itemc]);
  ++itemc;

  ck_assert_int_eq (list->count, 7);

  for (int i = 0; i < itemc; ++i) {
    ck_assert_int_eq (item [i]->alloc, 1);
  }

  nlistFree (list);

  for (int i = 0; i < itemc; ++i) {
    ck_assert_int_eq (item [i]->alloc, 0);
    free (item [i]);
  }
}
END_TEST

/* exercises set-list */
START_TEST(nlist_free_list)
{
  nlist_t       *list;
  nlist_t       *item [20];
  int           itemc = 0;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_free_list");

  list = nlistAlloc ("chk-ll", LIST_UNORDERED, NULL);
  ck_assert_ptr_nonnull (list);

  item [itemc] = nlistAlloc ("chk-ll-0", LIST_ORDERED, NULL);
  nlistSetStr (item [itemc], 0, "000");
  nlistSetNum (item [itemc], 1, 0);
  nlistSetList (list, 6, item [itemc]);
  ++itemc;

  item [itemc] = nlistAlloc ("chk-ll-1", LIST_ORDERED, NULL);
  nlistSetStr (item [itemc], 1, "111");
  nlistSetNum (item [itemc], 2, 1);
  nlistSetList (list, 26, item [itemc]);
  ++itemc;

  item [itemc] = nlistAlloc ("chk-ll-2", LIST_ORDERED, NULL);
  nlistSetStr (item [itemc], 2, "222");
  nlistSetNum (item [itemc], 3, 2);
  nlistSetList (list, 18, item [itemc]);
  ++itemc;

  item [itemc] = nlistAlloc ("chk-ll-3", LIST_ORDERED, NULL);
  nlistSetStr (item [itemc], 3, "333");
  nlistSetNum (item [itemc], 4, 3);
  nlistSetList (list, 11, item [itemc]);
  ++itemc;

  item [itemc] = nlistAlloc ("chk-ll-4", LIST_ORDERED, NULL);
  nlistSetStr (item [itemc], 4, "444");
  nlistSetNum (item [itemc], 5, 4);
  nlistSetList (list, 3, item [itemc]);
  ++itemc;

  item [itemc] = nlistAlloc ("chk-ll-5", LIST_ORDERED, NULL);
  nlistSetStr (item [itemc], 5, "555");
  nlistSetNum (item [itemc], 6, 5);
  nlistSetList (list, 1, item [itemc]);
  ++itemc;

  item [itemc] = nlistAlloc ("chk-ll-6", LIST_ORDERED, NULL);
  nlistSetStr (item [itemc], 6, "666");
  nlistSetNum (item [itemc], 7, 6);
  nlistSetList (list, 2, item [itemc]);
  ++itemc;

  ck_assert_int_eq (list->count, 7);

  for (int i = 0; i < itemc; ++i) {
    ck_assert_int_eq (nlistGetNum (item [i], i + 1), i);
  }

  nlistFree (list);
}
END_TEST

START_TEST(nlist_set_get_mixed)
{
  nlist_t *       list;
  ssize_t         nval;
  char            *sval;
  double          dval;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_set_get_mixed");

  list = nlistAlloc ("chk-mm", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  nlistSetNum (list, 6, 0);
  nlistSetDouble (list, 26, 1.0);
  nlistSetStr (list, 18, "2");
  ck_assert_int_eq (list->count, 3);
  ck_assert_int_eq (list->allocCount, 5);

  nval = nlistGetNum (list, 6);
  ck_assert_int_eq (nval, 0);
  dval = nlistGetDouble (list, 26);
  ck_assert_float_eq (dval, 1.0);
  sval = nlistGetStr (list, 18);
  ck_assert_str_eq (sval, "2");

  nlistFree (list);
}
END_TEST

START_TEST(nlist_inc_dec)
{
  nlist_t *       list;
  ssize_t         nval;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_inc_dec");

  list = nlistAlloc ("chk-nn", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  nlistSetNum (list, 6, 0);

  nval = nlistGetNum (list, 6);
  ck_assert_int_eq (nval, 0);
  nlistIncrement (list, 6);
  nval = nlistGetNum (list, 6);
  ck_assert_int_eq (nval, 1);
  nlistIncrement (list, 6);
  nval = nlistGetNum (list, 6);
  ck_assert_int_eq (nval, 2);
  nlistDecrement (list, 6);
  nval = nlistGetNum (list, 6);
  ck_assert_int_eq (nval, 1);

  nval = nlistGetNum (list, 7);
  ck_assert_int_eq (nval, LIST_VALUE_INVALID);
  nlistIncrement (list, 7);
  nval = nlistGetNum (list, 7);
  ck_assert_int_eq (nval, 1);
  nlistIncrement (list, 7);
  nval = nlistGetNum (list, 7);
  ck_assert_int_eq (nval, 2);
  nlistDecrement (list, 7);
  nval = nlistGetNum (list, 7);
  ck_assert_int_eq (nval, 1);
  ck_assert_int_eq (list->count, 2);

  nlistFree (list);
}
END_TEST

START_TEST(nlist_byidx)
{
  nlist_t *     list;
  ssize_t       value;
  nlistidx_t    key;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_byidx");

  list = nlistAlloc ("chk-oo", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  nlistSetNum (list, 6, 0);
  nlistSetNum (list, 26, 1);
  nlistSetNum (list, 18, 2);
  nlistSetNum (list, 11, 3);
  nlistSetNum (list, 3, 4);
  nlistSetNum (list, 1, 5);
  nlistSetNum (list, 2, 6);

  key = nlistGetIdx (list, 3);
  ck_assert_int_eq (key, 2);

  key = nlistGetIdx (list, 1);
  ck_assert_int_eq (key, 0);

  /* should be cached */
  key = nlistGetIdx (list, 1);
  ck_assert_int_eq (key, 0);

  key = nlistGetIdx (list, 2);
  ck_assert_int_eq (key, 1);

  key = nlistGetIdx (list, 18);
  ck_assert_int_eq (key, 5);

  key = nlistGetIdx (list, 6);
  ck_assert_int_eq (key, 3);

  key = nlistGetIdx (list, 11);
  ck_assert_int_eq (key, 4);

  key = nlistGetIdx (list, 26);
  value = nlistGetNumByIdx (list, key);
  ck_assert_int_eq (key, 6);
  ck_assert_int_eq (value, 1);

  key = nlistGetIdx (list, 18);
  value = nlistGetKeyByIdx (list, key);
  ck_assert_int_eq (key, 5);
  ck_assert_int_eq (value, 18);

  nlistFree (list);
}
END_TEST

START_TEST(nlist_byidx_bug_20220815)
{
  nlist_t *     list;
  nlistidx_t    key;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_byidx_bug_20220815");

  list = nlistAlloc ("chk-pp", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  nlistSetNum (list, 6, 0);
  nlistSetNum (list, 26, 1);
  nlistSetNum (list, 18, 2);
  nlistSetNum (list, 11, 3);
  nlistSetNum (list, 3, 4);
  nlistSetNum (list, 1, 5);
  nlistSetNum (list, 2, 6);

  key = nlistGetIdx (list, 3);
  ck_assert_int_eq (key, 2);
  /* should be in cache */
  ck_assert_int_eq (listDebugIsCached (list, 3), 1);
  /* not found, returns -1 */
  key = nlistGetIdx (list, 99);
  ck_assert_int_lt (key, 0);
  ck_assert_int_eq (listDebugIsCached (list, 3), 0);

  nlistFree (list);
}
END_TEST

START_TEST(nlist_prob_search)
{
  nlist_t *     list;
  nlistidx_t    key;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_prob_search");

  list = nlistAlloc ("chk-qq", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  nlistSetDouble (list, 5, 0.1);
  nlistSetDouble (list, 7, 0.2);
  nlistSetDouble (list, 8, 0.3);
  nlistSetDouble (list, 9, 0.5);
  nlistSetDouble (list, 10, 0.6);
  nlistSetDouble (list, 11, 0.9);
  nlistSetDouble (list, 13, 1.0);

  key = nlistSearchProbTable (list, 0.001);
  ck_assert_int_eq (key, 5);
  key = nlistSearchProbTable (list, 0.82);
  ck_assert_int_eq (key, 11);
  key = nlistSearchProbTable (list, 0.15);
  ck_assert_int_eq (key, 7);
  key = nlistSearchProbTable (list, 0.09);
  ck_assert_int_eq (key, 5);
  key = nlistSearchProbTable (list, 0.98);
  ck_assert_int_eq (key, 13);

  nlistFree (list);
}
END_TEST

START_TEST(nlist_set_null)
{
  nlist_t        *list;
  const char     *value;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- nlist_set_null");

  list = nlistAlloc ("chk-rr", LIST_UNORDERED, NULL);
  nlistSetSize (list, 7);
  nlistSetStr (list, 6, "0L");
  nlistSetStr (list, 26, "1L");
  nlistSetStr (list, 18, "2L");
  nlistSetStr (list, 11, "3L");
  nlistSetStr (list, 3, "4L");
  nlistSetStr (list, 1, "5L");
  nlistSetStr (list, 2, "6L");
  nlistSort (list);

  value = nlistGetStr (list, 3);
  ck_assert_ptr_nonnull (value);
  nlistSetStr (list, 3, NULL);
  value = nlistGetStr (list, 3);
  ck_assert_ptr_null (value);
  nlistFree (list);
}
END_TEST

Suite *
nlist_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("nlist");
  tc = tcase_create ("nlist");
  tcase_set_tags (tc, "libbasic");
  tcase_add_test (tc, nlist_create_free);
  tcase_add_test (tc, nlist_version);
  tcase_add_test (tc, nlist_set_size);
  tcase_add_test (tc, nlist_get_count);
  tcase_add_test (tc, nlist_u_set_num);
  tcase_add_test (tc, nlist_u_set_double);
  tcase_add_test (tc, nlist_u_set_data_static);
  tcase_add_test (tc, nlist_u_set_str);
  tcase_add_test (tc, nlist_u_set_list);
  tcase_add_test (tc, nlist_u_set_str_no_size);
  tcase_add_test (tc, nlist_u_getbyidx);
  tcase_add_test (tc, nlist_u_iterate);
  tcase_add_test (tc, nlist_s_set_size_sort);
  tcase_add_test (tc, nlist_s_no_size_sort);
  tcase_add_test (tc, nlist_s_ordered);
  tcase_add_test (tc, nlist_s_get_str);
  tcase_add_test (tc, nlist_s_cache_bug_20221013);
  tcase_add_test (tc, nlist_s_iterate_str);
  tcase_add_test (tc, nlist_set_get_num);
  tcase_add_test (tc, nlist_s_iterate_num);
  tcase_add_test (tc, nlist_s_replace_str);
  tcase_add_test (tc, nlist_free_str);
  tcase_add_test (tc, nlist_free_item);
  tcase_add_test (tc, nlist_free_list);
  tcase_add_test (tc, nlist_set_get_mixed);
  tcase_add_test (tc, nlist_inc_dec);
  tcase_add_test (tc, nlist_byidx);
  tcase_add_test (tc, nlist_byidx_bug_20220815);
  tcase_add_test (tc, nlist_prob_search);
  tcase_add_test (tc, nlist_set_null);
  suite_add_tcase (s, tc);
  return s;
}

static void
freeItem (void *titem)
{
  chk_item_t *item = titem;

  if (item->value != NULL) {
    item->alloc = false;
    free (item->value);
  }
}

static chk_item_t *
allocItem (const char *str)
{
  chk_item_t  *item;

  item = malloc (sizeof (chk_item_t));
  item->alloc = true;
  item->value = strdup (str);
  return item;
}

