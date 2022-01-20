#include "config.h"
#include "configt.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <check.h>

#include "nlist.h"
#include "bdjstring.h"
#include "check_bdj.h"
#include "log.h"

START_TEST(nlist_create_free)
{
  nlist_t    *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_create_free");


  list = nlistAlloc ("chk", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 0);
  ck_assert_int_eq (list->ordered, LIST_ORDERED);
  ck_assert_int_eq (list->keytype, LIST_KEY_NUM);
  nlistFree (list);
}
END_TEST

START_TEST(nlist_get_data_str)
{
  nlist_t        *list;
  char          *value;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_get_data_str");


  list = nlistAlloc ("chk", LIST_UNORDERED, NULL);
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
  value = (char *) nlistGetStr (list, 3);
  ck_assert_ptr_nonnull (value);
  ck_assert_str_eq (value, "4L");
  nlistFree (list);
}
END_TEST

START_TEST(nlist_add_str_iterate)
{
  nlist_t *      list;
  char *        value;
  nlistidx_t          key;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_add_str_iterate");


  list = nlistAlloc ("chk", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  nlistSetStr (list, 6, "555");
  nlistSetStr (list, 3, "222");
  nlistSetStr (list, 5, "444");
  nlistSetStr (list, 4, "333");
  nlistSetStr (list, 1, "000");
  nlistSetStr (list, 2, "111");
  nlistStartIterator (list);
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 1);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 2);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "111");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 3);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "222");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 4);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "333");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 5);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "444");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 6);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "555");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, -1);
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 1);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");
  nlistFree (list);
}
END_TEST

START_TEST(nlist_add_sort_str)
{
  nlist_t *      list;
  ssize_t          value;
  nlistidx_t          key;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_add_sort_str");


  list = nlistAlloc ("chk", LIST_UNORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  nlistSetNum (list, 6, 0);
  nlistSetNum (list, 26, 1);
  nlistSetNum (list, 18, 2);
  nlistSetNum (list, 11, 3);
  nlistSetNum (list, 3, 4);
  nlistSetNum (list, 1, 5);
  nlistSetNum (list, 2, 6);
  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (list->allocCount, 7);

  nlistStartIterator (list);
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 6);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 0);
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 26);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 1);

  nlistSort (list);
  ck_assert_int_eq (list->ordered, LIST_ORDERED);
  ck_assert_int_eq (list->count, 7);
  nlistStartIterator (list);
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 1);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 5);
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 2);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 6);

  nlistFree (list);
}
END_TEST

START_TEST(nlist_replace_str)
{
  nlist_t       *list;
  nlistidx_t    key;
  char          *value;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_replace_str");


  list = nlistAlloc ("chk", LIST_ORDERED, NULL);
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

  nlistStartIterator (list);
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 1);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 2);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "111");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 3);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "222");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 4);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "333");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 5);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "444");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 6);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "555");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, -1);
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 1);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");

  nlistSetStr (list, 2, "666");
  nlistSetStr (list, 3, "777");
  nlistSetStr (list, 4, "888");

  nlistStartIterator (list);
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 1);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 2);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "666");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 3);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "777");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 4);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "888");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 5);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "444");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 6);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "555");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, -1);
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 1);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");

  nlistFree (list);
}
END_TEST

START_TEST(nlist_free_str)
{
  nlist_t        *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_free_str");


  list = nlistAlloc ("chk", LIST_UNORDERED, free);
  ck_assert_ptr_nonnull (list);
  nlistSetStr (list, 6, strdup("0L"));
  nlistSetStr (list, 26, strdup("1L"));
  nlistSetStr (list, 18, strdup("2L"));
  nlistSetStr (list, 11, strdup("3L"));
  nlistSetStr (list, 3, strdup("4L"));
  nlistSetStr (list, 1, strdup("5L"));
  nlistSetStr (list, 2, strdup("6L"));
  ck_assert_int_eq (list->count, 7);
  nlistFree (list);
}
END_TEST

Suite *
nlist_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("nList Suite");
  tc = tcase_create ("nList");
  tcase_add_test (tc, nlist_create_free);
  tcase_add_test (tc, nlist_get_data_str);
  tcase_add_test (tc, nlist_add_str_iterate);
  tcase_add_test (tc, nlist_add_sort_str);
  tcase_add_test (tc, nlist_replace_str);
  tcase_add_test (tc, nlist_free_str);
  suite_add_tcase (s, tc);
  return s;
}
