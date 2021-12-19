#include <stdio.h>
#include <stdlib.h>
#include <check.h>

#include "list.h"
#include "bdjstring.h"
#include "check_bdj.h"

START_TEST(list_create_free)
{
  list_t    *list;

  list = listAlloc (sizeof (char *), LIST_ORDERED, istringCompare, NULL);
  ck_assert (list != NULL);
  ck_assert (list->count == 0);
  ck_assert (list->allocCount == 0);
  ck_assert (list->dsiz == sizeof (char *));
  ck_assert (list->ordered == LIST_ORDERED);
  ck_assert (list->type == LIST_BASIC);
  listFree (list);
}
END_TEST

START_TEST(list_add_ordered)
{
  list_t    *list;

  list = listAlloc (sizeof (char *), LIST_ORDERED, istringCompare, NULL);
  ck_assert (list != NULL);
  ck_assert (list->dsiz == sizeof (char *));
  listSet (list, "ffff");
  ck_assert (list->count == 1);
  ck_assert (strcmp ((char *) list->data[0], "ffff") == 0);
  listSet (list, "zzzz");
  ck_assert (list->count == 2);
  ck_assert (strcmp ((char *) list->data[0], "ffff") == 0);
  ck_assert (strcmp ((char *) list->data[1], "zzzz") == 0);
  listSet (list, "rrrr");
  ck_assert (list->count == 3);
  ck_assert (strcmp ((char *) list->data[0], "ffff") == 0);
  ck_assert (strcmp ((char *) list->data[1], "rrrr") == 0);
  ck_assert (strcmp ((char *) list->data[2], "zzzz") == 0);
  listSet (list, "kkkk");
  ck_assert (list->count == 4);
  ck_assert (strcmp ((char *) list->data[0], "ffff") == 0);
  ck_assert (strcmp ((char *) list->data[1], "kkkk") == 0);
  ck_assert (strcmp ((char *) list->data[2], "rrrr") == 0);
  ck_assert (strcmp ((char *) list->data[3], "zzzz") == 0);
  listSet (list, "cccc");
  ck_assert (list->count == 5);
  ck_assert (strcmp ((char *) list->data[0], "cccc") == 0);
  ck_assert (strcmp ((char *) list->data[1], "ffff") == 0);
  ck_assert (strcmp ((char *) list->data[2], "kkkk") == 0);
  ck_assert (strcmp ((char *) list->data[3], "rrrr") == 0);
  ck_assert (strcmp ((char *) list->data[4], "zzzz") == 0);
  listSet (list, "aaaa");
  ck_assert (list->count == 6);
  ck_assert (strcmp ((char *) list->data[0], "aaaa") == 0);
  ck_assert (strcmp ((char *) list->data[1], "cccc") == 0);
  ck_assert (strcmp ((char *) list->data[2], "ffff") == 0);
  ck_assert (strcmp ((char *) list->data[3], "kkkk") == 0);
  ck_assert (strcmp ((char *) list->data[4], "rrrr") == 0);
  ck_assert (strcmp ((char *) list->data[5], "zzzz") == 0);
  listSet (list, "bbbb");
  ck_assert (list->count == 7);
  ck_assert (list->allocCount == 7);
  ck_assert (strcmp ((char *) list->data[0], "aaaa") == 0);
  ck_assert (strcmp ((char *) list->data[1], "bbbb") == 0);
  ck_assert (strcmp ((char *) list->data[2], "cccc") == 0);
  ck_assert (strcmp ((char *) list->data[3], "ffff") == 0);
  ck_assert (strcmp ((char *) list->data[4], "kkkk") == 0);
  ck_assert (strcmp ((char *) list->data[5], "rrrr") == 0);
  ck_assert (strcmp ((char *) list->data[6], "zzzz") == 0);
  listFree (list);
}
END_TEST

START_TEST(list_add_ordered_beg)
{
  list_t    *list;

  list = listAlloc (sizeof (char *), LIST_ORDERED, istringCompare, NULL);
  ck_assert (list != NULL);
  listSet (list, "cccc");
  ck_assert (list->count == 1);
  ck_assert (strcmp ((char *) list->data[0], "cccc") == 0);
  listSet (list, "bbbb");
  ck_assert (list->count == 2);
  ck_assert (strcmp ((char *) list->data[0], "bbbb") == 0);
  ck_assert (strcmp ((char *) list->data[1], "cccc") == 0);
  listSet (list, "aaaa");
  ck_assert (list->count == 3);
  ck_assert (list->allocCount == 3);
  ck_assert (strcmp ((char *) list->data[0], "aaaa") == 0);
  ck_assert (strcmp ((char *) list->data[1], "bbbb") == 0);
  ck_assert (strcmp ((char *) list->data[2], "cccc") == 0);
  listFree (list);
}
END_TEST

START_TEST(list_add_ordered_end)
{
  list_t    *list;

  list = listAlloc (sizeof (char *), LIST_ORDERED, istringCompare, NULL);
  ck_assert (list != NULL);
  listSet (list, "aaaa");
  ck_assert (list->count == 1);
  ck_assert (strcmp ((char *) list->data[0], "aaaa") == 0);
  listSet (list, "bbbb");
  ck_assert (list->count == 2);
  ck_assert (strcmp ((char *) list->data[0], "aaaa") == 0);
  ck_assert (strcmp ((char *) list->data[1], "bbbb") == 0);
  listSet (list, "cccc");
  ck_assert (list->count == 3);
  ck_assert (list->allocCount == 3);
  ck_assert (strcmp ((char *) list->data[0], "aaaa") == 0);
  ck_assert (strcmp ((char *) list->data[1], "bbbb") == 0);
  ck_assert (strcmp ((char *) list->data[2], "cccc") == 0);
  listFree (list);
}
END_TEST

START_TEST(list_add_ordered_prealloc)
{
  list_t    *list;

  list = listAlloc (sizeof (char *), LIST_ORDERED, istringCompare, NULL);
  listSetSize (list, 7);
  ck_assert (list != NULL);
  ck_assert (list->dsiz == sizeof (char *));
  ck_assert (list->allocCount == 7);
  listSet (list, "ffff");
  listSet (list, "zzzz");
  listSet (list, "rrrr");
  listSet (list, "kkkk");
  listSet (list, "cccc");
  listSet (list, "aaaa");
  listSet (list, "bbbb");
  ck_assert (list->count == 7);
  ck_assert (list->allocCount == 7);
  ck_assert (strcmp ((char *) list->data[0], "aaaa") == 0);
  ck_assert (strcmp ((char *) list->data[1], "bbbb") == 0);
  ck_assert (strcmp ((char *) list->data[2], "cccc") == 0);
  ck_assert (strcmp ((char *) list->data[3], "ffff") == 0);
  ck_assert (strcmp ((char *) list->data[4], "kkkk") == 0);
  ck_assert (strcmp ((char *) list->data[5], "rrrr") == 0);
  ck_assert (strcmp ((char *) list->data[6], "zzzz") == 0);
  listFree (list);
}
END_TEST

START_TEST(list_add_unordered)
{
  list_t    *list;

  list = listAlloc (sizeof (char *), LIST_UNORDERED, istringCompare, NULL);
  ck_assert (list != NULL);
  ck_assert (list->ordered == LIST_UNORDERED);
  listSet (list, "cccc");
  ck_assert (list->count == 1);
  ck_assert (strcmp ((char *) list->data[0], "cccc") == 0);
  listSet (list, "aaaa");
  ck_assert (list->count == 2);
  ck_assert (strcmp ((char *) list->data[0], "cccc") == 0);
  ck_assert (strcmp ((char *) list->data[1], "aaaa") == 0);
  listSet (list, "bbbb");
  ck_assert (list->count == 3);
  ck_assert (list->allocCount == 3);
  ck_assert (strcmp ((char *) list->data[0], "cccc") == 0);
  ck_assert (strcmp ((char *) list->data[1], "aaaa") == 0);
  ck_assert (strcmp ((char *) list->data[2], "bbbb") == 0);
  listFree (list);
}
END_TEST

START_TEST(list_add_sort)
{
  list_t    *list;

  list = listAlloc (sizeof (char *), LIST_UNORDERED, istringCompare, NULL);
  ck_assert (list != NULL);
  ck_assert (list->dsiz == sizeof (char *));
  listSet (list, "ffff");
  listSet (list, "zzzz");
  listSet (list, "rrrr");
  listSet (list, "kkkk");
  listSet (list, "cccc");
  listSet (list, "aaaa");
  listSet (list, "bbbb");
  ck_assert (list->count == 7);
  ck_assert (list->allocCount == 7);
  ck_assert (strcmp ((char *) list->data[0], "ffff") == 0);
  ck_assert (strcmp ((char *) list->data[1], "zzzz") == 0);
  ck_assert (strcmp ((char *) list->data[2], "rrrr") == 0);
  ck_assert (strcmp ((char *) list->data[3], "kkkk") == 0);
  ck_assert (strcmp ((char *) list->data[4], "cccc") == 0);
  ck_assert (strcmp ((char *) list->data[5], "aaaa") == 0);
  ck_assert (strcmp ((char *) list->data[6], "bbbb") == 0);
  listSort (list);
  ck_assert (list->ordered == LIST_ORDERED);
  ck_assert (list->count == 7);
  ck_assert (strcmp ((char *) list->data[0], "aaaa") == 0);
  ck_assert (strcmp ((char *) list->data[1], "bbbb") == 0);
  ck_assert (strcmp ((char *) list->data[2], "cccc") == 0);
  ck_assert (strcmp ((char *) list->data[3], "ffff") == 0);
  ck_assert (strcmp ((char *) list->data[4], "kkkk") == 0);
  ck_assert (strcmp ((char *) list->data[5], "rrrr") == 0);
  ck_assert (strcmp ((char *) list->data[6], "zzzz") == 0);
  listFree (list);
}
END_TEST

START_TEST(list_find)
{
  list_t    *list;
  long      loc;

  list = listAlloc (sizeof (char *), LIST_ORDERED, istringCompare, NULL);
  ck_assert (list != NULL);
  listSet (list, "ffff");
  listSet (list, "zzzz");
  listSet (list, "rrrr");
  listSet (list, "kkkk");
  listSet (list, "cccc");
  listSet (list, "aaaa");
  listSet (list, "bbbb");
  listSet (list, "dddd");
  listSet (list, "eeee");
  listSet (list, "gggg");
  ck_assert (list->count == 10);
  ck_assert (list->allocCount == 10);
  loc = listFind (list, "aaaa");
  ck_assert (loc == 0);
  loc = listFind (list, "zzzz");
  ck_assert (loc == 9);
  loc = listFind (list, "ffff");
  ck_assert (loc == 5);
  loc = listFind (list, "bbbb");
  ck_assert (loc == 1);
  loc = listFind (list, "rrrr");
  ck_assert (loc == 8);
  listFree (list);
}
END_TEST

START_TEST(list_free)
{
  list_t    *list;

  list = listAlloc (sizeof (char *), LIST_ORDERED, istringCompare, free);
  ck_assert (list != NULL);
  listSet (list, strdup ("ffff"));
  listSet (list, strdup ("zzzz"));
  listSet (list, strdup ("rrrr"));
  listSet (list, strdup ("kkkk"));
  listSet (list, strdup ("cccc"));
  listSet (list, strdup ("aaaa"));
  listSet (list, strdup ("bbbb"));
  listSet (list, strdup ("dddd"));
  listSet (list, strdup ("eeee"));
  listSet (list, strdup ("gggg"));
  ck_assert (list->count == 10);
  ck_assert (list->allocCount == 10);
  listFree (list);
}
END_TEST

START_TEST(vlist_create_free)
{
  list_t    *list;

  list = vlistAlloc (LIST_ORDERED, VALUE_DATA, istringCompare, NULL, NULL);
  ck_assert (list != NULL);
  ck_assert (list->count == 0);
  ck_assert (list->allocCount == 0);
  ck_assert (list->dsiz == sizeof (namevalue_t *));
  ck_assert (list->ordered == LIST_ORDERED);
  ck_assert (list->type == LIST_NAMEVALUE);
  vlistFree (list);
}
END_TEST

START_TEST(vlist_add)
{
  list_t        *list;
  namevalue_t   *nv;

  list = vlistAlloc (LIST_ORDERED, VALUE_DATA, istringCompare, NULL, NULL);
  ck_assert (list != NULL);
  ck_assert (list->dsiz == sizeof (namevalue_t *));
  vlistSetData (list, "aaaa", "000");
  nv = (namevalue_t *) list->data[0];
  ck_assert (strcmp (nv->name, "aaaa") == 0);
  ck_assert (strcmp ((char *) nv->u.data, "000") == 0);
  vlistFree (list);
}
END_TEST

START_TEST(vlist_add_sort)
{
  list_t        *list;
  namevalue_t   *nv;

  list = vlistAlloc (LIST_UNORDERED, VALUE_LONG, istringCompare, NULL, NULL);
  ck_assert (list != NULL);
  ck_assert (list->dsiz == sizeof (namevalue_t *));
  ck_assert (list->valuetype == VALUE_LONG);
  vlistSetLong (list, "ffff", 0L);
  vlistSetLong (list, "zzzz", 1L);
  vlistSetLong (list, "rrrr", 2L);
  vlistSetLong (list, "kkkk", 3L);
  vlistSetLong (list, "cccc", 4L);
  vlistSetLong (list, "aaaa", 5L);
  vlistSetLong (list, "bbbb", 6L);
  ck_assert (list->count == 7);
  ck_assert (list->allocCount == 7);
  nv = (namevalue_t *) list->data[0];
  ck_assert (strcmp (nv->name, "ffff") == 0);
  ck_assert (nv->u.l == 0L);
  vlistSort (list);
  ck_assert (list->ordered == LIST_ORDERED);
  ck_assert (list->count == 7);
  nv = (namevalue_t *) list->data[0];
  ck_assert (strcmp (nv->name, "aaaa") == 0);
  ck_assert (nv->u.l == 5L);
  vlistFree (list);
}
END_TEST

START_TEST(vlist_replace)
{
  list_t        *list;
  namevalue_t   *nv;

  list = vlistAlloc (LIST_ORDERED, VALUE_DATA, istringCompare, NULL, NULL);
  ck_assert (list != NULL);
  ck_assert (list->dsiz == sizeof (namevalue_t *));
  vlistSetData (list, "aaaa", "000");
  vlistSetData (list, "bbbb", "111");
  vlistSetData (list, "cccc", "222");
  vlistSetData (list, "dddd", "333");
  vlistSetData (list, "eeee", "444");
  vlistSetData (list, "ffff", "555");
  ck_assert (list->count == 6);
  nv = (namevalue_t *) list->data[2];
  ck_assert (strcmp (nv->name, "cccc") == 0);
  ck_assert (strcmp ((char *) nv->u.data, "222") == 0);
  vlistSetData (list, "cccc", "666");
  nv = (namevalue_t *) list->data[2];
  ck_assert (strcmp (nv->name, "cccc") == 0);
  ck_assert (strcmp ((char *) nv->u.data, "666") == 0);
  vlistFree (list);
}
END_TEST

START_TEST(vlist_free)
{
  list_t        *list;

  list = vlistAlloc (LIST_UNORDERED, VALUE_DATA, istringCompare, free, free);
  ck_assert (list != NULL);
  ck_assert (list->dsiz == sizeof (namevalue_t *));
  ck_assert (list->valuetype == VALUE_DATA);
  vlistSetData (list, strdup ("ffff"), strdup("0L"));
  vlistSetData (list, strdup ("zzzz"), strdup("1L"));
  vlistSetData (list, strdup ("rrrr"), strdup("2L"));
  vlistSetData (list, strdup ("kkkk"), strdup("3L"));
  vlistSetData (list, strdup ("cccc"), strdup("4L"));
  vlistSetData (list, strdup ("aaaa"), strdup("5L"));
  vlistSetData (list, strdup ("bbbb"), strdup("6L"));
  ck_assert (list->count == 7);
  vlistFree (list);
}
END_TEST

Suite *
list_suite (void)
{
  Suite     *s;
  TCase     *tc_list;

  s = suite_create ("List Suite");
  tc_list = tcase_create ("List");
  tcase_add_test (tc_list, list_create_free);
  tcase_add_test (tc_list, list_add_ordered);
  tcase_add_test (tc_list, list_add_ordered_beg);
  tcase_add_test (tc_list, list_add_ordered_end);
  tcase_add_test (tc_list, list_add_ordered_prealloc);
  tcase_add_test (tc_list, list_add_unordered);
  tcase_add_test (tc_list, list_add_sort);
  tcase_add_test (tc_list, list_find);
  tcase_add_test (tc_list, list_free);
  tcase_add_test (tc_list, vlist_create_free);
  tcase_add_test (tc_list, vlist_add);
  tcase_add_test (tc_list, vlist_add_sort);
  tcase_add_test (tc_list, vlist_replace);
  tcase_add_test (tc_list, vlist_free);
  suite_add_tcase (s, tc_list);
  return s;
}

