#include "config.h"
#include "configt.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <check.h>

#include "list.h"
#include "bdjstring.h"
#include "check_bdj.h"

START_TEST(simple_list_create_free)
{
  list_t    *list;

  list = listAlloc ("chk", LIST_UNORDERED, istringCompare, NULL, NULL);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 0);
  ck_assert_int_eq (list->ordered, LIST_UNORDERED);
  listFree (list);
}
END_TEST

START_TEST(simple_list_add_unordered)
{
  list_t    *list;


  list = listAlloc ("chk", LIST_UNORDERED, istringCompare, NULL, NULL);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (list->ordered, LIST_UNORDERED);
  listSetData (list, "ffff", NULL);
  ck_assert_int_eq (list->count, 1);
  listSetData (list, "zzzz", NULL);
  ck_assert_int_eq (list->count, 2);
  listSetData (list, "rrrr", NULL);
  ck_assert_int_eq (list->count, 3);
  listSetData (list, "kkkk", NULL);
  ck_assert_int_eq (list->count, 4);
  listSetData (list, "cccc", NULL);
  ck_assert_int_eq (list->count, 5);
  listSetData (list, "aaaa", NULL);
  ck_assert_int_eq (list->count, 6);
  listSetData (list, "bbbb", NULL);
  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (list->allocCount, 7);
  listFree (list);
}
END_TEST

START_TEST(simple_list_add_unordered_iterate)
{
  list_t *  list;
  char *    value;


  list = listAlloc ("chk", LIST_UNORDERED, istringCompare, NULL, NULL);
  ck_assert_ptr_nonnull (list);
  listSetData (list, "ffff", NULL);
  listSetData (list, "zzzz", NULL);
  listSetData (list, "rrrr", NULL);
  listSetData (list, "kkkk", NULL);
  listSetData (list, "cccc", NULL);
  listSetData (list, "aaaa", NULL);
  listSetData (list, "bbbb", NULL);
  listStartIterator (list);
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "ffff");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "zzzz");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "rrrr");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "kkkk");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "cccc");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) listIterateKeyStr (list);
  ck_assert_ptr_null (value);
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "ffff");
  listFree (list);
}
END_TEST

START_TEST(simple_list_add_ordered_iterate)
{
  list_t *  list;
  char *    value;


  list = listAlloc ("chk", LIST_ORDERED, istringCompare, NULL, NULL);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (list->ordered, LIST_ORDERED);
  listSetData (list, "ffff", NULL);
  listSetData (list, "zzzz", NULL);
  listSetData (list, "rrrr", NULL);
  listSetData (list, "kkkk", NULL);
  listSetData (list, "cccc", NULL);
  listSetData (list, "aaaa", NULL);
  listSetData (list, "bbbb", NULL);
  listStartIterator (list);
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "cccc");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "ffff");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "kkkk");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "rrrr");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "zzzz");
  listFree (list);
}
END_TEST

START_TEST(simple_list_add_ordered_beg)
{
  list_t *  list;
  char *    value;


  list = listAlloc ("chk", LIST_ORDERED, istringCompare, NULL, NULL);
  ck_assert_ptr_nonnull (list);
  listSetData (list, "cccc", NULL);
  ck_assert_int_eq (list->count, 1);
  listSetData (list, "bbbb", NULL);
  ck_assert_int_eq (list->count, 2);
  listSetData (list, "aaaa", NULL);
  ck_assert_int_eq (list->count, 3);
  ck_assert_int_eq (list->allocCount, 3);
  listStartIterator (list);
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "cccc");
  listFree (list);
}
END_TEST

START_TEST(simple_list_add_ordered_end)
{
  list_t *  list;
  char *    value;


  list = listAlloc ("chk", LIST_ORDERED, istringCompare, NULL, NULL);
  ck_assert_ptr_nonnull (list);
  listSetData (list, "aaaa", NULL);
  ck_assert_int_eq (list->count, 1);
  listSetData (list, "bbbb", NULL);
  ck_assert_int_eq (list->count, 2);
  listSetData (list, "cccc", NULL);
  ck_assert_int_eq (list->count, 3);
  ck_assert_int_eq (list->allocCount, 3);
  listStartIterator (list);
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "cccc");
  listFree (list);
}
END_TEST

START_TEST(simple_list_add_ordered_prealloc)
{
  list_t *  list;
  char *    value;


  list = listAlloc ("chk", LIST_ORDERED, istringCompare, NULL, NULL);
  listSetSize (list, 7);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 7);
  listSetData (list, "ffff", NULL);
  listSetData (list, "zzzz", NULL);
  listSetData (list, "rrrr", NULL);
  listSetData (list, "kkkk", NULL);
  listSetData (list, "cccc", NULL);
  listSetData (list, "aaaa", NULL);
  listSetData (list, "bbbb", NULL);
  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (list->allocCount, 7);
  listStartIterator (list);
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "cccc");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "ffff");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "kkkk");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "rrrr");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "zzzz");
  listFree (list);
}
END_TEST

START_TEST(simple_list_add_sort)
{
  list_t *  list;
  char *    value;


  list = listAlloc ("chk", LIST_UNORDERED, istringCompare, NULL, NULL);
  ck_assert_ptr_nonnull (list);
  listSetData (list, "ffff", NULL);
  listSetData (list, "zzzz", NULL);
  listSetData (list, "rrrr", NULL);
  listSetData (list, "kkkk", NULL);
  listSetData (list, "cccc", NULL);
  listSetData (list, "aaaa", NULL);
  listSetData (list, "bbbb", NULL);
  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (list->allocCount, 7);
  listStartIterator (list);
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "ffff");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "zzzz");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "rrrr");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "kkkk");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "cccc");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "bbbb");
  listSort (list);
  ck_assert_int_eq (list->ordered, LIST_ORDERED);
  ck_assert_int_eq (list->count, 7);
  listStartIterator (list);
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "cccc");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "ffff");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "kkkk");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "rrrr");
  value = (char *) listIterateKeyStr (list);
  ck_assert_str_eq (value, "zzzz");
  listFree (list);
}
END_TEST

START_TEST(list_get_data_str)
{
  list_t        *list;
  char          *value;


  list = listAlloc ("chk", LIST_UNORDERED, istringCompare, NULL, NULL);
  listSetSize (list, 7);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 7);
  listSetData (list, "ffff", "0L");
  listSetData (list, "zzzz", "1L");
  listSetData (list, "rrrr", "2L");
  listSetData (list, "kkkk", "3L");
  listSetData (list, "cccc", "4L");
  listSetData (list, "aaaa", "5L");
  listSetData (list, "bbbb", "6L");
  ck_assert_int_eq (list->count, 7);
  listSort (list);
  ck_assert_int_eq (list->count, 7);
  value = (char *) listGetData (list, "cccc");
  ck_assert_ptr_nonnull (value);
  ck_assert_str_eq (value, "4L");
  listFree (list);
}
END_TEST

START_TEST(list_add_str_iterate)
{
  list_t *      list;
  char *        value;
  char *        key;


  list = listAlloc ("chk", LIST_ORDERED, istringCompare, NULL, NULL);
  ck_assert_ptr_nonnull (list);
  listSetData (list, "ffff", "555");
  listSetData (list, "cccc", "222");
  listSetData (list, "eeee", "444");
  listSetData (list, "dddd", "333");
  listSetData (list, "aaaa", "000");
  listSetData (list, "bbbb", "111");
  listStartIterator (list);
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "000");
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "bbbb");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "111");
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "cccc");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "222");
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "dddd");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "333");
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "eeee");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "444");
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "ffff");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "555");
  key = listIterateKeyStr (list);
  ck_assert_ptr_null (key);
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "000");
  listFree (list);
}
END_TEST

START_TEST(list_add_sort_str)
{
  list_t *      list;
  long          value;
  char          *key;


  list = listAlloc ("chk", LIST_UNORDERED, istringCompare, NULL, NULL);
  ck_assert_ptr_nonnull (list);
  listSetLong (list, "ffff", 0L);
  listSetLong (list, "zzzz", 1L);
  listSetLong (list, "rrrr", 2L);
  listSetLong (list, "kkkk", 3L);
  listSetLong (list, "cccc", 4L);
  listSetLong (list, "aaaa", 5L);
  listSetLong (list, "bbbb", 6L);
  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (list->allocCount, 7);

  listStartIterator (list);
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "ffff");
  value = listGetLong (list, key);
  ck_assert_int_eq (value, 0L);
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "zzzz");
  value = listGetLong (list, key);
  ck_assert_int_eq (value, 1L);

  listSort (list);
  ck_assert_int_eq (list->ordered, LIST_ORDERED);
  ck_assert_int_eq (list->count, 7);
  listStartIterator (list);
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "aaaa");
  value = listGetLong (list, key);
  ck_assert_int_eq (value, 5L);
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "bbbb");
  value = listGetLong (list, key);
  ck_assert_int_eq (value, 6L);

  listFree (list);
}
END_TEST

START_TEST(list_replace_str)
{
  list_t        *list;
  char          *key;
  char          *value;


  list = listAlloc ("chk", LIST_ORDERED, istringCompare, NULL, NULL);
  ck_assert_ptr_nonnull (list);

  listSetData (list, "aaaa", "000");

  value = listGetData (list, "aaaa");
  ck_assert_str_eq (value, "000");

  listSetData (list, "bbbb", "111");
  listSetData (list, "cccc", "222");
  listSetData (list, "dddd", "333");

  value = listGetData (list, "aaaa");
  ck_assert_str_eq (value, "000");

  listSetData (list, "eeee", "444");
  listSetData (list, "ffff", "555");

  ck_assert_int_eq (list->count, 6);

  listStartIterator (list);
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "000");
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "bbbb");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "111");
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "cccc");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "222");
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "dddd");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "333");
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "eeee");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "444");
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "ffff");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "555");
  key = listIterateKeyStr (list);
  ck_assert_ptr_null (key);
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "000");

  listSetData (list, "bbbb", "666");
  listSetData (list, "cccc", "777");
  listSetData (list, "dddd", "888");

  listStartIterator (list);
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "000");
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "bbbb");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "666");
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "cccc");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "777");
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "dddd");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "888");
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "eeee");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "444");
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "ffff");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "555");
  key = listIterateKeyStr (list);
  ck_assert_ptr_null (key);
  key = listIterateKeyStr (list);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "000");

  listFree (list);
}
END_TEST

START_TEST(list_free_str)
{
  list_t        *list;


  list = listAlloc ("chk", LIST_UNORDERED, istringCompare, free, free);
  ck_assert_ptr_nonnull (list);
  listSetData (list, "ffff", strdup("0L"));
  listSetData (list, "zzzz", strdup("1L"));
  listSetData (list, "rrrr", strdup("2L"));
  listSetData (list, "kkkk", strdup("3L"));
  listSetData (list, "cccc", strdup("4L"));
  listSetData (list, "aaaa", strdup("5L"));
  listSetData (list, "bbbb", strdup("6L"));
  ck_assert_int_eq (list->count, 7);
  listFree (list);
}
END_TEST

START_TEST(llist_create_free)
{
  list_t    *list;


  list = llistAlloc ("chk", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 0);
  ck_assert_int_eq (list->ordered, LIST_ORDERED);
  ck_assert_int_eq (list->keytype, KEY_LONG);
  llistFree (list);
}
END_TEST

START_TEST(llist_get_data_str)
{
  list_t        *list;
  char          *value;


  list = llistAlloc ("chk", LIST_UNORDERED, NULL);
  llistSetSize (list, 7);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 7);
  llistSetData (list, 6L, "0L");
  llistSetData (list, 26L, "1L");
  llistSetData (list, 18L, "2L");
  llistSetData (list, 11L, "3L");
  llistSetData (list, 3L, "4L");
  llistSetData (list, 1L, "5L");
  llistSetData (list, 2L, "6L");
  ck_assert_int_eq (list->count, 7);
  llistSort (list);
  ck_assert_int_eq (list->count, 7);
  value = (char *) llistGetData (list, 3L);
  ck_assert_ptr_nonnull (value);
  ck_assert_str_eq (value, "4L");
  llistFree (list);
}
END_TEST

START_TEST(llist_add_str_iterate)
{
  list_t *      list;
  char *        value;
  long          key;


  list = llistAlloc ("chk", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  llistSetData (list, 6L, "555");
  llistSetData (list, 3L, "222");
  llistSetData (list, 5L, "444");
  llistSetData (list, 4L, "333");
  llistSetData (list, 1L, "000");
  llistSetData (list, 2L, "111");
  llistStartIterator (list);
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 1L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "000");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 2L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "111");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 3L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "222");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 4L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "333");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 5L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "444");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 6L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "555");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, -1L);
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 1L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "000");
  llistFree (list);
}
END_TEST

START_TEST(llist_add_sort_str)
{
  list_t *      list;
  long          value;
  long          key;


  list = llistAlloc ("chk", LIST_UNORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  llistSetLong (list, 6L, 0L);
  llistSetLong (list, 26L, 1L);
  llistSetLong (list, 18L, 2L);
  llistSetLong (list, 11L, 3L);
  llistSetLong (list, 3L, 4L);
  llistSetLong (list, 1L, 5L);
  llistSetLong (list, 2L, 6L);
  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (list->allocCount, 7);

  llistStartIterator (list);
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 6L);
  value = llistGetLong (list, key);
  ck_assert_int_eq (value, 0L);
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 26L);
  value = llistGetLong (list, key);
  ck_assert_int_eq (value, 1L);

  llistSort (list);
  ck_assert_int_eq (list->ordered, LIST_ORDERED);
  ck_assert_int_eq (list->count, 7);
  llistStartIterator (list);
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 1L);
  value = llistGetLong (list, key);
  ck_assert_int_eq (value, 5L);
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 2L);
  value = llistGetLong (list, key);
  ck_assert_int_eq (value, 6L);

  llistFree (list);
}
END_TEST

START_TEST(llist_replace_str)
{
  list_t        *list;
  long          key;
  char          *value;


  list = llistAlloc ("chk", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);

  llistSetData (list, 1L, "000");

  value = llistGetData (list, 1L);
  ck_assert_str_eq (value, "000");

  llistSetData (list, 2L, "111");
  llistSetData (list, 3L, "222");
  llistSetData (list, 4L, "333");

  value = llistGetData (list, 1L);
  ck_assert_str_eq (value, "000");

  llistSetData (list, 5L, "444");
  llistSetData (list, 6L, "555");

  ck_assert_int_eq (list->count, 6);

  llistStartIterator (list);
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 1L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "000");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 2L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "111");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 3L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "222");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 4L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "333");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 5L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "444");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 6L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "555");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, -1L);
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 1L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "000");

  llistSetData (list, 2L, "666");
  llistSetData (list, 3L, "777");
  llistSetData (list, 4L, "888");

  llistStartIterator (list);
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 1L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "000");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 2L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "666");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 3L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "777");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 4L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "888");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 5L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "444");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 6L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "555");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, -1L);
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 1L);
  value = (char *) llistGetData (list, key);
  ck_assert_str_eq (value, "000");

  llistFree (list);
}
END_TEST

START_TEST(llist_free_str)
{
  list_t        *list;


  list = llistAlloc ("chk", LIST_UNORDERED, free);
  ck_assert_ptr_nonnull (list);
  llistSetData (list, 6L, strdup("0L"));
  llistSetData (list, 26L, strdup("1L"));
  llistSetData (list, 18L, strdup("2L"));
  llistSetData (list, 11L, strdup("3L"));
  llistSetData (list, 3L, strdup("4L"));
  llistSetData (list, 1L, strdup("5L"));
  llistSetData (list, 2L, strdup("6L"));
  ck_assert_int_eq (list->count, 7);
  llistFree (list);
}
END_TEST

Suite *
list_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("List Suite");
  tc = tcase_create ("List");
  tcase_add_test (tc, simple_list_create_free);
  tcase_add_test (tc, simple_list_add_unordered);
  tcase_add_test (tc, simple_list_add_unordered_iterate);
  tcase_add_test (tc, simple_list_add_ordered_iterate);
  tcase_add_test (tc, simple_list_add_ordered_beg);
  tcase_add_test (tc, simple_list_add_ordered_end);
  tcase_add_test (tc, simple_list_add_ordered_prealloc);
  tcase_add_test (tc, simple_list_add_sort);
  tcase_add_test (tc, list_get_data_str);
  tcase_add_test (tc, list_add_str_iterate);
  tcase_add_test (tc, list_add_sort_str);
  tcase_add_test (tc, list_replace_str);
  tcase_add_test (tc, list_free_str);
  tcase_add_test (tc, llist_create_free);
  tcase_add_test (tc, llist_get_data_str);
  tcase_add_test (tc, llist_add_str_iterate);
  tcase_add_test (tc, llist_add_sort_str);
  tcase_add_test (tc, llist_replace_str);
  tcase_add_test (tc, llist_free_str);
  suite_add_tcase (s, tc);
  return s;
}
