#include <stdio.h>
#include <check.h>

#include "list.h"
#include "bdjstring.h"
#include "check_bdj.h"

START_TEST(list_create_free)
{
  list_t    *list;

  list = listAlloc (sizeof (char *), LIST_ORDERED);
  ck_assert (list != NULL);
  ck_assert (list->count == 0);
  ck_assert (list->dsiz == sizeof (char *));
  ck_assert (list->ordered == LIST_ORDERED);
  ck_assert (list->type == LIST_BASIC);
  listFree (list);
}
END_TEST

START_TEST(list_add_ordered)
{
  list_t    *list;

  list = listAlloc (sizeof (char *), LIST_ORDERED);
  ck_assert (list != NULL);
  ck_assert (list->dsiz == sizeof (char *));
  listAdd (list, "ffff", istringCompare);
  ck_assert (list->count == 1);
  ck_assert (strcmp ((char *) list->data[0], "ffff") == 0);
  listAdd (list, "zzzz", istringCompare);
  ck_assert (list->count == 2);
  ck_assert (strcmp ((char *) list->data[0], "ffff") == 0);
  ck_assert (strcmp ((char *) list->data[1], "zzzz") == 0);
  listAdd (list, "rrrr", istringCompare);
  ck_assert (list->count == 3);
  ck_assert (strcmp ((char *) list->data[0], "ffff") == 0);
  ck_assert (strcmp ((char *) list->data[1], "rrrr") == 0);
  ck_assert (strcmp ((char *) list->data[2], "zzzz") == 0);
  listAdd (list, "kkkk", istringCompare);
  ck_assert (list->count == 4);
  ck_assert (strcmp ((char *) list->data[0], "ffff") == 0);
  ck_assert (strcmp ((char *) list->data[1], "kkkk") == 0);
  ck_assert (strcmp ((char *) list->data[2], "rrrr") == 0);
  ck_assert (strcmp ((char *) list->data[3], "zzzz") == 0);
  listAdd (list, "cccc", istringCompare);
  ck_assert (list->count == 5);
  ck_assert (strcmp ((char *) list->data[0], "cccc") == 0);
  ck_assert (strcmp ((char *) list->data[1], "ffff") == 0);
  ck_assert (strcmp ((char *) list->data[2], "kkkk") == 0);
  ck_assert (strcmp ((char *) list->data[3], "rrrr") == 0);
  ck_assert (strcmp ((char *) list->data[4], "zzzz") == 0);
  listAdd (list, "aaaa", istringCompare);
  ck_assert (list->count == 6);
  ck_assert (strcmp ((char *) list->data[0], "aaaa") == 0);
  ck_assert (strcmp ((char *) list->data[1], "cccc") == 0);
  ck_assert (strcmp ((char *) list->data[2], "ffff") == 0);
  ck_assert (strcmp ((char *) list->data[3], "kkkk") == 0);
  ck_assert (strcmp ((char *) list->data[4], "rrrr") == 0);
  ck_assert (strcmp ((char *) list->data[5], "zzzz") == 0);
  listAdd (list, "bbbb", istringCompare);
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

START_TEST(list_add_ordered_beg)
{
  list_t    *list;

  list = listAlloc (sizeof (char *), LIST_ORDERED);
  ck_assert (list != NULL);
  listAdd (list, "cccc", istringCompare);
  ck_assert (list->count == 1);
  ck_assert (strcmp ((char *) list->data[0], "cccc") == 0);
  listAdd (list, "bbbb", istringCompare);
  ck_assert (list->count == 2);
  ck_assert (strcmp ((char *) list->data[0], "bbbb") == 0);
  ck_assert (strcmp ((char *) list->data[1], "cccc") == 0);
  listAdd (list, "aaaa", istringCompare);
  ck_assert (list->count == 3);
  ck_assert (strcmp ((char *) list->data[0], "aaaa") == 0);
  ck_assert (strcmp ((char *) list->data[1], "bbbb") == 0);
  ck_assert (strcmp ((char *) list->data[2], "cccc") == 0);
  listFree (list);
}
END_TEST

START_TEST(list_add_ordered_end)
{
  list_t    *list;

  list = listAlloc (sizeof (char *), LIST_ORDERED);
  ck_assert (list != NULL);
  listAdd (list, "aaaa", istringCompare);
  ck_assert (list->count == 1);
  ck_assert (strcmp ((char *) list->data[0], "aaaa") == 0);
  listAdd (list, "bbbb", istringCompare);
  ck_assert (list->count == 2);
  ck_assert (strcmp ((char *) list->data[0], "aaaa") == 0);
  ck_assert (strcmp ((char *) list->data[1], "bbbb") == 0);
  listAdd (list, "cccc", istringCompare);
  ck_assert (list->count == 3);
  ck_assert (strcmp ((char *) list->data[0], "aaaa") == 0);
  ck_assert (strcmp ((char *) list->data[1], "bbbb") == 0);
  ck_assert (strcmp ((char *) list->data[2], "cccc") == 0);
  listFree (list);
}
END_TEST

START_TEST(list_add_unordered)
{
  list_t    *list;

  list = listAlloc (sizeof (char *), LIST_UNORDERED);
  ck_assert (list != NULL);
  ck_assert (list->ordered == LIST_UNORDERED);
  listAdd (list, "cccc", istringCompare);
  ck_assert (list->count == 1);
  ck_assert (strcmp ((char *) list->data[0], "cccc") == 0);
  listAdd (list, "aaaa", istringCompare);
  ck_assert (list->count == 2);
  ck_assert (strcmp ((char *) list->data[0], "cccc") == 0);
  ck_assert (strcmp ((char *) list->data[1], "aaaa") == 0);
  listAdd (list, "bbbb", istringCompare);
  ck_assert (list->count == 3);
  ck_assert (strcmp ((char *) list->data[0], "cccc") == 0);
  ck_assert (strcmp ((char *) list->data[1], "aaaa") == 0);
  ck_assert (strcmp ((char *) list->data[2], "bbbb") == 0);
  listFree (list);
}
END_TEST

START_TEST(list_add_sort)
{
  list_t    *list;

  list = listAlloc (sizeof (char *), LIST_UNORDERED);
  ck_assert (list != NULL);
  ck_assert (list->dsiz == sizeof (char *));
  listAdd (list, "ffff", istringCompare);
  listAdd (list, "zzzz", istringCompare);
  listAdd (list, "rrrr", istringCompare);
  listAdd (list, "kkkk", istringCompare);
  listAdd (list, "cccc", istringCompare);
  listAdd (list, "aaaa", istringCompare);
  listAdd (list, "bbbb", istringCompare);
  ck_assert (list->count == 7);
  ck_assert (strcmp ((char *) list->data[0], "ffff") == 0);
  ck_assert (strcmp ((char *) list->data[1], "zzzz") == 0);
  ck_assert (strcmp ((char *) list->data[2], "rrrr") == 0);
  ck_assert (strcmp ((char *) list->data[3], "kkkk") == 0);
  ck_assert (strcmp ((char *) list->data[4], "cccc") == 0);
  ck_assert (strcmp ((char *) list->data[5], "aaaa") == 0);
  ck_assert (strcmp ((char *) list->data[6], "bbbb") == 0);
  listSort (list, istringCompare);
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

  list = listAlloc (sizeof (char *), LIST_ORDERED);
  ck_assert (list != NULL);
  listAdd (list, "ffff", istringCompare);
  listAdd (list, "zzzz", istringCompare);
  listAdd (list, "rrrr", istringCompare);
  listAdd (list, "kkkk", istringCompare);
  listAdd (list, "cccc", istringCompare);
  listAdd (list, "aaaa", istringCompare);
  listAdd (list, "bbbb", istringCompare);
  listAdd (list, "dddd", istringCompare);
  listAdd (list, "eeee", istringCompare);
  listAdd (list, "gggg", istringCompare);
  listAdd (list, "ffff", istringCompare);
  ck_assert (list->count == 11);
  loc = listFind (list, "aaaa", istringCompare);
  ck_assert (loc == 0);
  loc = listFind (list, "zzzz", istringCompare);
  ck_assert (loc == 10);
  loc = listFind (list, "ffff", istringCompare);
  ck_assert (loc == 5);
  loc = listFind (list, "bbbb", istringCompare);
  ck_assert (loc == 1);
  loc = listFind (list, "rrrr", istringCompare);
  ck_assert (loc == 9);
  listFree (list);
}
END_TEST

START_TEST(vlist_create_free)
{
  list_t    *list;

  list = vlistAlloc (LIST_ORDERED, VALUE_STR);
  ck_assert (list != NULL);
  ck_assert (list->count == 0);
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

  list = vlistAlloc (LIST_ORDERED, VALUE_STR);
  ck_assert (list != NULL);
  ck_assert (list->dsiz == sizeof (namevalue_t *));
  vlistAddStr (list, "aaaa", "000", istringCompare);
  nv = (namevalue_t *) list->data[0];
  ck_assert (strcmp (nv->name, "aaaa") == 0);
  ck_assert (strcmp (nv->u.str, "000") == 0);
  vlistFree (list);
}
END_TEST

START_TEST(vlist_add_sort)
{
  list_t        *list;
  namevalue_t   *nv;

  list = vlistAlloc (LIST_UNORDERED, VALUE_LONG);
  ck_assert (list != NULL);
  ck_assert (list->dsiz == sizeof (namevalue_t *));
  ck_assert (list->valuetype == VALUE_LONG);
  vlistAddLong (list, "ffff", 0L, istringCompare);
  vlistAddLong (list, "zzzz", 1L, istringCompare);
  vlistAddLong (list, "rrrr", 2L, istringCompare);
  vlistAddLong (list, "kkkk", 3L, istringCompare);
  vlistAddLong (list, "cccc", 4L, istringCompare);
  vlistAddLong (list, "aaaa", 5L, istringCompare);
  vlistAddLong (list, "bbbb", 6L, istringCompare);
  ck_assert (list->count == 7);
  nv = (namevalue_t *) list->data[0];
  ck_assert (strcmp (nv->name, "ffff") == 0);
  ck_assert (nv->u.l == 0L);
  vlistSort (list, istringCompare);
  ck_assert (list->ordered == LIST_ORDERED);
  ck_assert (list->count == 7);
  nv = (namevalue_t *) list->data[0];
  ck_assert (strcmp (nv->name, "aaaa") == 0);
  ck_assert (nv->u.l == 5L);
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
  tcase_add_test (tc_list, list_add_unordered);
  tcase_add_test (tc_list, list_add_sort);
  tcase_add_test (tc_list, list_find);
  tcase_add_test (tc_list, vlist_create_free);
  tcase_add_test (tc_list, vlist_add);
  tcase_add_test (tc_list, vlist_add_sort);
  suite_add_tcase (s, tc_list);
  return s;
}

