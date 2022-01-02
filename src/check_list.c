#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <check.h>

#include "list.h"
#include "bdjstring.h"
#include "check_bdj.h"

START_TEST(simple_list_create_free)
{
  list_t    *list;

  list = listAlloc ("chk", sizeof (char *), istringCompare, NULL);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 0);
  ck_assert_int_eq (list->dsiz, sizeof (char *));
  ck_assert_int_eq (list->ordered, LIST_UNORDERED);
  ck_assert_int_eq (list->type, LIST_BASIC);
  listFree (list);
}
END_TEST

START_TEST(simple_list_add_unordered)
{
  list_t    *list;

  list = listAlloc ("chk", sizeof (char *), istringCompare, NULL);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (list->ordered, LIST_UNORDERED);
  /* change to ordered list */
  listSort (list);
  ck_assert_int_eq (list->dsiz, sizeof (char *));
  ck_assert_int_eq (list->ordered, LIST_ORDERED);
  listSet (list, "ffff");
  ck_assert_int_eq (list->count, 1);
  listSet (list, "zzzz");
  ck_assert_int_eq (list->count, 2);
  listSet (list, "rrrr");
  ck_assert_int_eq (list->count, 3);
  listSet (list, "kkkk");
  ck_assert_int_eq (list->count, 4);
  listSet (list, "cccc");
  ck_assert_int_eq (list->count, 5);
  listSet (list, "aaaa");
  ck_assert_int_eq (list->count, 6);
  listSet (list, "bbbb");
  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (list->allocCount, 7);
  listFree (list);
}
END_TEST

START_TEST(simple_list_add_unordered_iterate)
{
  list_t *  list;
  char *    value;

  list = listAlloc ("chk", sizeof (char *), istringCompare, NULL);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (list->dsiz, sizeof (char *));
  listSet (list, "ffff");
  listSet (list, "zzzz");
  listSet (list, "rrrr");
  listSet (list, "kkkk");
  listSet (list, "cccc");
  listSet (list, "aaaa");
  listSet (list, "bbbb");
  listStartIterator (list);
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "ffff");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "zzzz");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "rrrr");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "kkkk");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "cccc");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) listIterateData (list);
  ck_assert_ptr_null (value);
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "ffff");
  listFree (list);
}
END_TEST

START_TEST(simple_list_add_ordered_iterate)
{
  list_t *  list;
  char *    value;

  list = listAlloc ("chk", sizeof (char *), istringCompare, NULL);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (list->dsiz, sizeof (char *));
  ck_assert_int_eq (list->ordered, LIST_UNORDERED);
  /* change to ordered list */
  listSort (list);
  ck_assert_int_eq (list->ordered, LIST_ORDERED);
  listSet (list, "ffff");
  listSet (list, "zzzz");
  listSet (list, "rrrr");
  listSet (list, "kkkk");
  listSet (list, "cccc");
  listSet (list, "aaaa");
  listSet (list, "bbbb");
  listStartIterator (list);
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "cccc");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "ffff");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "kkkk");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "rrrr");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "zzzz");
  listFree (list);
}
END_TEST

START_TEST(simple_list_add_ordered_beg)
{
  list_t *  list;
  char *    value;

  list = listAlloc ("chk", sizeof (char *), istringCompare, NULL);
  /* change to ordered list */
  listSort (list);
  ck_assert_ptr_nonnull (list);
  listSet (list, "cccc");
  ck_assert_int_eq (list->count, 1);
  listSet (list, "bbbb");
  ck_assert_int_eq (list->count, 2);
  listSet (list, "aaaa");
  ck_assert_int_eq (list->count, 3);
  ck_assert_int_eq (list->allocCount, 3);
  listStartIterator (list);
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "cccc");
  listFree (list);
}
END_TEST

START_TEST(simple_list_add_ordered_end)
{
  list_t *  list;
  char *    value;

  list = listAlloc ("chk", sizeof (char *), istringCompare, NULL);
  /* change to ordered list */
  listSort (list);
  ck_assert_ptr_nonnull (list);
  listSet (list, "aaaa");
  ck_assert_int_eq (list->count, 1);
  listSet (list, "bbbb");
  ck_assert_int_eq (list->count, 2);
  listSet (list, "cccc");
  ck_assert_int_eq (list->count, 3);
  ck_assert_int_eq (list->allocCount, 3);
  listStartIterator (list);
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "cccc");
  listFree (list);
}
END_TEST

START_TEST(simple_list_add_ordered_prealloc)
{
  list_t *  list;
  char *    value;

  list = listAlloc ("chk", sizeof (char *), istringCompare, NULL);
  listSetSize (list, 7);
  /* change to ordered list */
  listSort (list);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (list->dsiz, sizeof (char *));
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 7);
  listSet (list, "ffff");
  listSet (list, "zzzz");
  listSet (list, "rrrr");
  listSet (list, "kkkk");
  listSet (list, "cccc");
  listSet (list, "aaaa");
  listSet (list, "bbbb");
  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (list->allocCount, 7);
  listStartIterator (list);
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "cccc");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "ffff");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "kkkk");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "rrrr");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "zzzz");
  listFree (list);
}
END_TEST

START_TEST(simple_list_add_sort)
{
  list_t *  list;
  char *    value;

  list = listAlloc ("chk", sizeof (char *), istringCompare, NULL);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (list->dsiz, sizeof (char *));
  listSet (list, "ffff");
  listSet (list, "zzzz");
  listSet (list, "rrrr");
  listSet (list, "kkkk");
  listSet (list, "cccc");
  listSet (list, "aaaa");
  listSet (list, "bbbb");
  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (list->allocCount, 7);
  listStartIterator (list);
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "ffff");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "zzzz");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "rrrr");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "kkkk");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "cccc");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "bbbb");
  listSort (list);
  ck_assert_int_eq (list->ordered, LIST_ORDERED);
  ck_assert_int_eq (list->count, 7);
  listStartIterator (list);
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "aaaa");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "bbbb");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "cccc");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "ffff");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "kkkk");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "rrrr");
  value = (char *) listIterateData (list);
  ck_assert_str_eq (value, "zzzz");
  listFree (list);
}
END_TEST

START_TEST(slist_create_free)
{
  list_t    *list;

  list = slistAlloc ("chk", LIST_ORDERED, istringCompare, NULL, NULL);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 0);
  ck_assert_int_eq (list->ordered, LIST_ORDERED);
  ck_assert_int_eq (list->type, LIST_NAMEVALUE);
  ck_assert_int_eq (list->keytype, KEY_STR);
  slistFree (list);
}
END_TEST

START_TEST(slist_get_data_str)
{
  list_t        *list;
  char          *value;

  list = slistAlloc ("chk", LIST_UNORDERED, istringCompare, NULL, NULL);
  slistSetSize (list, 7);
  ck_assert_int_eq (list->count, 0);
  ck_assert_int_eq (list->allocCount, 7);
  slistSetData (list, "ffff", "0L");
  slistSetData (list, "zzzz", "1L");
  slistSetData (list, "rrrr", "2L");
  slistSetData (list, "kkkk", "3L");
  slistSetData (list, "cccc", "4L");
  slistSetData (list, "aaaa", "5L");
  slistSetData (list, "bbbb", "6L");
  ck_assert_int_eq (list->count, 7);
  slistSort (list);
  ck_assert_int_eq (list->count, 7);
  value = (char *) slistGetData (list, "cccc");
  ck_assert_ptr_nonnull (value);
  ck_assert_str_eq (value, "4L");
  slistFree (list);
}
END_TEST

START_TEST(slist_add_str_iterate)
{
  list_t *      list;
  char *        value;
  char *        key;

  list = slistAlloc ("chk", LIST_ORDERED, istringCompare, NULL, NULL);
  ck_assert_ptr_nonnull (list);
  slistSetData (list, "ffff", "555");
  slistSetData (list, "cccc", "222");
  slistSetData (list, "eeee", "444");
  slistSetData (list, "dddd", "333");
  slistSetData (list, "aaaa", "000");
  slistSetData (list, "bbbb", "111");
  slistStartIterator (list);
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "000");
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "bbbb");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "111");
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "cccc");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "222");
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "dddd");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "333");
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "eeee");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "444");
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "ffff");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "555");
  key = slistIterateKeyStr (list);
  ck_assert_ptr_null (key);
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "000");
  slistFree (list);
}
END_TEST

START_TEST(slist_add_sort_str)
{
  list_t *      list;
  long          value;
  char          *key;

  list = slistAlloc ("chk", LIST_UNORDERED, istringCompare, NULL, NULL);
  ck_assert_ptr_nonnull (list);
  slistSetLong (list, "ffff", 0L);
  slistSetLong (list, "zzzz", 1L);
  slistSetLong (list, "rrrr", 2L);
  slistSetLong (list, "kkkk", 3L);
  slistSetLong (list, "cccc", 4L);
  slistSetLong (list, "aaaa", 5L);
  slistSetLong (list, "bbbb", 6L);
  ck_assert_int_eq (list->count, 7);
  ck_assert_int_eq (list->allocCount, 7);

  slistStartIterator (list);
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "ffff");
  value = slistGetLong (list, key);
  ck_assert_int_eq (value, 0L);
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "zzzz");
  value = slistGetLong (list, key);
  ck_assert_int_eq (value, 1L);

  slistSort (list);
  ck_assert_int_eq (list->ordered, LIST_ORDERED);
  ck_assert_int_eq (list->count, 7);
  slistStartIterator (list);
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "aaaa");
  value = slistGetLong (list, key);
  ck_assert_int_eq (value, 5L);
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "bbbb");
  value = slistGetLong (list, key);
  ck_assert_int_eq (value, 6L);

  slistFree (list);
}
END_TEST

START_TEST(slist_replace_str)
{
  list_t        *list;
  char          *key;
  char          *value;

  list = slistAlloc ("chk", LIST_ORDERED, istringCompare, NULL, NULL);
  ck_assert_ptr_nonnull (list);
  ck_assert_int_eq (list->type, LIST_NAMEVALUE);

  slistSetData (list, "aaaa", "000");

  value = slistGetData (list, "aaaa");
  ck_assert_str_eq (value, "000");

  slistSetData (list, "bbbb", "111");
  slistSetData (list, "cccc", "222");
  slistSetData (list, "dddd", "333");

  value = slistGetData (list, "aaaa");
  ck_assert_str_eq (value, "000");

  slistSetData (list, "eeee", "444");
  slistSetData (list, "ffff", "555");

  ck_assert_int_eq (list->count, 6);

  slistStartIterator (list);
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "000");
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "bbbb");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "111");
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "cccc");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "222");
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "dddd");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "333");
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "eeee");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "444");
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "ffff");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "555");
  key = slistIterateKeyStr (list);
  ck_assert_ptr_null (key);
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "000");

  slistSetData (list, "bbbb", "666");
  slistSetData (list, "cccc", "777");
  slistSetData (list, "dddd", "888");

  slistStartIterator (list);
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "000");
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "bbbb");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "666");
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "cccc");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "777");
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "dddd");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "888");
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "eeee");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "444");
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "ffff");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "555");
  key = slistIterateKeyStr (list);
  ck_assert_ptr_null (key);
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "aaaa");
  value = (char *) slistGetData (list, key);
  ck_assert_str_eq (value, "000");

  slistFree (list);
}
END_TEST

START_TEST(slist_free_str)
{
  list_t        *list;

  list = slistAlloc ("chk", LIST_UNORDERED, istringCompare, free, free);
  ck_assert_ptr_nonnull (list);
  slistSetData (list, "ffff", strdup("0L"));
  slistSetData (list, "zzzz", strdup("1L"));
  slistSetData (list, "rrrr", strdup("2L"));
  slistSetData (list, "kkkk", strdup("3L"));
  slistSetData (list, "cccc", strdup("4L"));
  slistSetData (list, "aaaa", strdup("5L"));
  slistSetData (list, "bbbb", strdup("6L"));
  ck_assert_int_eq (list->count, 7);
  slistFree (list);
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
  ck_assert_int_eq (list->type, LIST_NAMEVALUE);
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
  ck_assert_int_eq (list->type, LIST_NAMEVALUE);

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
  tcase_add_test (tc, slist_create_free);
  tcase_add_test (tc, slist_get_data_str);
  tcase_add_test (tc, slist_add_str_iterate);
  tcase_add_test (tc, slist_add_sort_str);
  tcase_add_test (tc, slist_replace_str);
  tcase_add_test (tc, slist_free_str);
  tcase_add_test (tc, llist_create_free);
  tcase_add_test (tc, llist_get_data_str);
  tcase_add_test (tc, llist_add_str_iterate);
  tcase_add_test (tc, llist_add_sort_str);
  tcase_add_test (tc, llist_replace_str);
  tcase_add_test (tc, llist_free_str);
  suite_add_tcase (s, tc);
  return s;
}
