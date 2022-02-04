#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <check.h>

#include "nlist.h"
#include "slist.h"
#include "ilist.h"
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
  listidx_t iteridx;


  list = listAlloc ("chk", LIST_UNORDERED, istringCompare, NULL, NULL);
  ck_assert_ptr_nonnull (list);
  listSetData (list, "ffff", NULL);
  listSetData (list, "zzzz", NULL);
  listSetData (list, "rrrr", NULL);
  listSetData (list, "kkkk", NULL);
  listSetData (list, "cccc", NULL);
  listSetData (list, "aaaa", NULL);
  listSetData (list, "bbbb", NULL);
  listStartIterator (list, &iteridx);
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "ffff");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "zzzz");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "rrrr");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "kkkk");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "cccc");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_ptr_null (value);
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "ffff");
  listFree (list);
}
END_TEST

START_TEST(simple_list_add_ordered_iterate)
{
  list_t *  list;
  char *    value;
  listidx_t iteridx;


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
  listStartIterator (list, &iteridx);
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "cccc");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "ffff");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "kkkk");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "rrrr");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "zzzz");
  listFree (list);
}
END_TEST

START_TEST(simple_list_add_ordered_beg)
{
  list_t *  list;
  char *    value;
  listidx_t iteridx;


  list = listAlloc ("chk", LIST_ORDERED, istringCompare, NULL, NULL);
  ck_assert_ptr_nonnull (list);
  listSetData (list, "cccc", NULL);
  ck_assert_int_eq (list->count, 1);
  listSetData (list, "bbbb", NULL);
  ck_assert_int_eq (list->count, 2);
  listSetData (list, "aaaa", NULL);
  ck_assert_int_eq (list->count, 3);
  ck_assert_int_eq (list->allocCount, 3);
  listStartIterator (list, &iteridx);
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "cccc");
  listFree (list);
}
END_TEST

START_TEST(simple_list_add_ordered_end)
{
  list_t *  list;
  char *    value;
  listidx_t iteridx;


  list = listAlloc ("chk", LIST_ORDERED, istringCompare, NULL, NULL);
  ck_assert_ptr_nonnull (list);
  listSetData (list, "aaaa", NULL);
  ck_assert_int_eq (list->count, 1);
  listSetData (list, "bbbb", NULL);
  ck_assert_int_eq (list->count, 2);
  listSetData (list, "cccc", NULL);
  ck_assert_int_eq (list->count, 3);
  ck_assert_int_eq (list->allocCount, 3);
  listStartIterator (list, &iteridx);
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "cccc");
  listFree (list);
}
END_TEST

START_TEST(simple_list_add_ordered_prealloc)
{
  list_t *  list;
  char *    value;
  listidx_t iteridx;


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
  listStartIterator (list, &iteridx);
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "cccc");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "ffff");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "kkkk");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "rrrr");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "zzzz");
  listFree (list);
}
END_TEST

START_TEST(simple_list_add_sort)
{
  list_t *  list;
  char *    value;
  listidx_t iteridx;


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
  listStartIterator (list, &iteridx);
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "ffff");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "zzzz");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "rrrr");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "kkkk");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "cccc");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "bbbb");
  listSort (list);
  ck_assert_int_eq (list->ordered, LIST_ORDERED);
  ck_assert_int_eq (list->count, 7);
  listStartIterator (list, &iteridx);
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "cccc");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "ffff");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "kkkk");
  value = (char *) ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "rrrr");
  value = (char *) ilistIterateKey (list, &iteridx);
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
  listidx_t     iteridx;


  list = listAlloc ("chk", LIST_ORDERED, istringCompare, NULL, NULL);
  ck_assert_ptr_nonnull (list);
  listSetData (list, "ffff", "555");
  listSetData (list, "cccc", "222");
  listSetData (list, "eeee", "444");
  listSetData (list, "dddd", "333");
  listSetData (list, "aaaa", "000");
  listSetData (list, "bbbb", "111");
  listStartIterator (list, &iteridx);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "000");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "bbbb");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "111");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "cccc");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "222");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "dddd");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "333");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "eeee");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "444");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "ffff");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "555");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_ptr_null (key);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "000");
  listFree (list);
}
END_TEST

START_TEST(list_add_sort_str)
{
  list_t *      list;
  ssize_t          value;
  char          *key;
  listidx_t     iteridx;


  list = listAlloc ("chk", LIST_UNORDERED, istringCompare, NULL, NULL);
  ck_assert_ptr_nonnull (list);
  listSetNum (list, "ffff", 0L);
  listSetNum (list, "zzzz", 1L);
  listSetNum (list, "rrrr", 2L);
  listSetNum (list, "kkkk", 3L);
  listSetNum (list, "cccc", 4L);
  listSetNum (list, "aaaa", 5L);
  listSetNum (list, "bbbb", 6L);
  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (list->allocCount, 7);

  listStartIterator (list, &iteridx);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "ffff");
  value = listGetNum (list, key);
  ck_assert_int_eq (value, 0L);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "zzzz");
  value = listGetNum (list, key);
  ck_assert_int_eq (value, 1L);

  listSort (list);
  ck_assert_int_eq (list->ordered, LIST_ORDERED);
  ck_assert_int_eq (list->count, 7);
  listStartIterator (list, &iteridx);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "aaaa");
  value = listGetNum (list, key);
  ck_assert_int_eq (value, 5L);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "bbbb");
  value = listGetNum (list, key);
  ck_assert_int_eq (value, 6L);

  listFree (list);
}
END_TEST

START_TEST(list_replace_str)
{
  list_t        *list;
  char          *key;
  char          *value;
  listidx_t     iteridx;


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

  listStartIterator (list, &iteridx);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "000");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "bbbb");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "111");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "cccc");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "222");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "dddd");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "333");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "eeee");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "444");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "ffff");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "555");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_ptr_null (key);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "000");

  listSetData (list, "bbbb", "666");
  listSetData (list, "cccc", "777");
  listSetData (list, "dddd", "888");

  listStartIterator (list, &iteridx);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "000");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "bbbb");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "666");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "cccc");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "777");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "dddd");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "888");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "eeee");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "444");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_str_eq (key, "ffff");
  value = (char *) listGetData (list, key);
  ck_assert_str_eq (value, "555");
  key = ilistIterateKey (list, &iteridx);
  ck_assert_ptr_null (key);
  key = ilistIterateKey (list, &iteridx);
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

START_TEST(nlist_create_free)
{
  list_t    *list;


  list = nlistAlloc ("chk", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 0);
  ck_assert_int_eq (list->ordered, LIST_ORDERED);
  ck_assert_int_eq (list->keytype, KEY_NUM);
  nlistFree (list);
}
END_TEST

START_TEST(nlist_get_data_str)
{
  list_t        *list;
  char          *value;


  list = nlistAlloc ("chk", LIST_UNORDERED, NULL);
  nlistSetSize (list, 7);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 7);
  nlistSetStr (list, 6L, "0L");
  nlistSetStr (list, 26L, "1L");
  nlistSetStr (list, 18L, "2L");
  nlistSetStr (list, 11L, "3L");
  nlistSetStr (list, 3L, "4L");
  nlistSetStr (list, 1L, "5L");
  nlistSetStr (list, 2L, "6L");
  ck_assert_int_eq (list->count, 7);
  nlistSort (list);
  ck_assert_int_eq (list->count, 7);
  value = (char *) nlistGetStr (list, 3L);
  ck_assert_ptr_nonnull (value);
  ck_assert_str_eq (value, "4L");
  nlistFree (list);
}
END_TEST

START_TEST(nlist_add_str_iterate)
{
  list_t *      list;
  char *        value;
  listidx_t          key;
  listidx_t     iteridx;


  list = nlistAlloc ("chk", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  nlistSetStr (list, 6L, "555");
  nlistSetStr (list, 3L, "222");
  nlistSetStr (list, 5L, "444");
  nlistSetStr (list, 4L, "333");
  nlistSetStr (list, 1L, "000");
  nlistSetStr (list, 2L, "111");
  nlistStartIterator (list, &iteridx);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "111");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 3L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "222");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 4L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "333");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 5L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "444");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 6L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "555");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, -1L);
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 1L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");
  nlistFree (list);
}
END_TEST

START_TEST(nlist_add_sort_str)
{
  list_t *      list;
  ssize_t          value;
  listidx_t          key;


  list = nlistAlloc ("chk", LIST_UNORDERED, NULL);
  ck_assert_ptr_nonnull (list);
  nlistSetNum (list, 6L, 0L);
  nlistSetNum (list, 26L, 1L);
  nlistSetNum (list, 18L, 2L);
  nlistSetNum (list, 11L, 3L);
  nlistSetNum (list, 3L, 4L);
  nlistSetNum (list, 1L, 5L);
  nlistSetNum (list, 2L, 6L);
  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (list->allocCount, 7);

  nlistStartIterator (list);
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 6L);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 0L);
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 26L);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 1L);

  nlistSort (list);
  ck_assert_int_eq (list->ordered, LIST_ORDERED);
  ck_assert_int_eq (list->count, 7);
  nlistStartIterator (list);
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 1L);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 5L);
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 2L);
  value = nlistGetNum (list, key);
  ck_assert_int_eq (value, 6L);

  nlistFree (list);
}
END_TEST

START_TEST(nlist_replace_str)
{
  list_t        *list;
  listidx_t          key;
  char          *value;


  list = nlistAlloc ("chk", LIST_ORDERED, NULL);
  ck_assert_ptr_nonnull (list);

  nlistSetStr (list, 1L, "000");

  value = nlistGetStr (list, 1L);
  ck_assert_str_eq (value, "000");

  nlistSetStr (list, 2L, "111");
  nlistSetStr (list, 3L, "222");
  nlistSetStr (list, 4L, "333");

  value = nlistGetStr (list, 1L);
  ck_assert_str_eq (value, "000");

  nlistSetStr (list, 5L, "444");
  nlistSetStr (list, 6L, "555");

  ck_assert_int_eq (list->count, 6);

  nlistStartIterator (list);
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 1L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 2L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "111");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 3L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "222");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 4L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "333");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 5L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "444");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 6L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "555");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, -1L);
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 1L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");

  nlistSetStr (list, 2L, "666");
  nlistSetStr (list, 3L, "777");
  nlistSetStr (list, 4L, "888");

  nlistStartIterator (list);
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 1L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 2L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "666");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 3L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "777");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 4L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "888");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 5L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "444");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 6L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "555");
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, -1L);
  key = nlistIterateKey (list);
  ck_assert_int_eq (key, 1L);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");

  nlistFree (list);
}
END_TEST

START_TEST(nlist_free_str)
{
  list_t        *list;


  list = nlistAlloc ("chk", LIST_UNORDERED, free);
  ck_assert_ptr_nonnull (list);
  nlistSetStr (list, 6L, strdup("0L"));
  nlistSetStr (list, 26L, strdup("1L"));
  nlistSetStr (list, 18L, strdup("2L"));
  nlistSetStr (list, 11L, strdup("3L"));
  nlistSetStr (list, 3L, strdup("4L"));
  nlistSetStr (list, 1L, strdup("5L"));
  nlistSetStr (list, 2L, strdup("6L"));
  ck_assert_int_eq (list->count, 7);
  nlistFree (list);
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
  tcase_add_test (tc, nlist_create_free);
  tcase_add_test (tc, nlist_get_data_str);
  tcase_add_test (tc, nlist_add_str_iterate);
  tcase_add_test (tc, nlist_add_sort_str);
  tcase_add_test (tc, nlist_replace_str);
  tcase_add_test (tc, nlist_free_str);
  suite_add_tcase (s, tc);
  return s;
}
