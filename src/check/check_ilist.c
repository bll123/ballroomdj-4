#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <check.h>

#include "ilist.h"
#include "bdjstring.h"
#include "check_bdj.h"
#include "log.h"

START_TEST(ilist_create_free)
{
  ilist_t    *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== ilist_create_free");

  list = ilistAlloc ("chk-a", LIST_ORDERED);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 0);
  ck_assert_int_eq (list->ordered, LIST_ORDERED);
  ck_assert_int_eq (list->keytype, LIST_KEY_NUM);
  ilistFree (list);
}
END_TEST

START_TEST(ilist_get_data_str)
{
  ilist_t        *list;
  char          *value;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== ilist_get_data_str");

  list = ilistAlloc ("chk-b", LIST_ORDERED);
  ilistSetSize (list, 7);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 7);
  ilistSetStr (list, 6, 0, "0L");
  ilistSetStr (list, 26, 0, "1L");
  ilistSetStr (list, 18, 0, "2L");
  ilistSetStr (list, 11, 0, "3L");
  ilistSetStr (list, 3, 0, "4L");
  ilistSetStr (list, 1, 0, "5L");
  ilistSetStr (list, 2, 0, "6L");
  ck_assert_int_eq (list->count, 7);
  value = ilistGetStr (list, 3, 0);
  ck_assert_ptr_nonnull (value);
  ck_assert_str_eq (value, "4L");
  ilistFree (list);
}
END_TEST

START_TEST(ilist_get_data_str_sub)
{
  ilist_t        *list;
  char          *value;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== ilist_get_data_str_sub");

  list = ilistAlloc ("chk-c", LIST_ORDERED);
  ilistSetSize (list, 7);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 7);
  ilistSetStr (list, 6, 0, "0L");
  ilistSetStr (list, 6, 1, "8L");
  ilistSetStr (list, 26, 0, "1L");
  ilistSetStr (list, 26, 1, "9L");
  ilistSetStr (list, 18, 0, "2L");
  ilistSetStr (list, 18, 1, "10L");
  ilistSetStr (list, 11, 0, "3L");
  ilistSetStr (list, 11, 1, "11L");
  ilistSetStr (list, 3, 0, "4L");
  ilistSetStr (list, 3, 1, "12L");
  ilistSetStr (list, 1, 0, "5L");
  ilistSetStr (list, 1, 1, "13L");
  ilistSetStr (list, 2, 0, "6L");
  ilistSetStr (list, 2, 1, "14L");
  ck_assert_int_eq (list->count, 7);
  value = ilistGetStr (list, 3, 0);
  ck_assert_ptr_nonnull (value);
  ck_assert_str_eq (value, "4L");
  value = ilistGetStr (list, 26, 1);
  ck_assert_ptr_nonnull (value);
  ck_assert_str_eq (value, "9L");
  ilistFree (list);
}
END_TEST

START_TEST(ilist_add_str_iterate)
{
  ilist_t *      list;
  char *        value;
  ilistidx_t          key;
  ilistidx_t    iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== ilist_add_str_iterate");

  list = ilistAlloc ("chk-d", LIST_ORDERED);
  ck_assert_ptr_nonnull (list);
  ilistSetStr (list, 6, 0, "555");
  ilistSetStr (list, 3, 0, "222");
  ilistSetStr (list, 5, 0, "444");
  ilistSetStr (list, 4, 0, "333");
  ilistSetStr (list, 1, 0, "000");
  ilistSetStr (list, 2, 0, "111");
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
  ck_assert_int_eq (key, -1);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = ilistGetStr (list, key, 0);
  ck_assert_str_eq (value, "000");
  ilistFree (list);
}
END_TEST

START_TEST(ilist_add_sort_str)
{
  ilist_t *      list;
  ssize_t          value;
  ilistidx_t          key;
  ilistidx_t      iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== ilist_add_sort_str");

  list = ilistAlloc ("chk-e", LIST_UNORDERED);
  ck_assert_ptr_nonnull (list);
  ilistSetNum (list, 6, 1, 0);
  ilistSetNum (list, 26, 1, 1);
  ilistSetNum (list, 18, 1, 2);
  ilistSetNum (list, 11, 1, 3);
  ilistSetNum (list, 3, 1, 4);
  ilistSetNum (list, 1, 1, 5);
  ilistSetNum (list, 2, 1, 6);
  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (list->allocCount, 7);

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
  ck_assert_int_eq (list->ordered, LIST_ORDERED);
  ck_assert_int_eq (list->count, 7);
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
  char          *value;
  ilistidx_t    iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== ilist_replace_str");

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

  ck_assert_int_eq (list->count, 6);

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
  ck_assert_int_eq (key, -1);
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
  ck_assert_int_eq (key, -1);
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

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== ilist_free_str");

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
  ck_assert_int_eq (list->count, 7);
  ilistFree (list);
}
END_TEST

Suite *
ilist_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("ilist Suite");
  tc = tcase_create ("ilist");
  tcase_add_test (tc, ilist_create_free);
  tcase_add_test (tc, ilist_get_data_str);
  tcase_add_test (tc, ilist_get_data_str_sub);
  tcase_add_test (tc, ilist_add_str_iterate);
  tcase_add_test (tc, ilist_add_sort_str);
  tcase_add_test (tc, ilist_replace_str);
  tcase_add_test (tc, ilist_free_str);
  suite_add_tcase (s, tc);
  return s;
}
