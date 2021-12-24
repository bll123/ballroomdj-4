#include "config.h"

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
  ck_assert (list->count == 0);
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
  listkey_t     lkey;

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
  lkey.name = "aaaa";
  loc = listFind (list, lkey);
  ck_assert (loc == 0);
  lkey.name = "zzzz";
  loc = listFind (list, lkey);
  ck_assert (loc == 9);
  lkey.name = "ffff";
  loc = listFind (list, lkey);
  ck_assert (loc == 5);
  lkey.name = "bbbb";
  loc = listFind (list, lkey);
  ck_assert (loc == 1);
  lkey.name = "rrrr";
  loc = listFind (list, lkey);
  ck_assert (loc == 8);
  lkey.name = "xyzzy";
  loc = listFind (list, lkey);
  ck_assert (loc == -1);
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

  list = vlistAlloc (KEY_STR, LIST_ORDERED, istringCompare, NULL, NULL);
  ck_assert (list != NULL);
  ck_assert (list->count == 0);
  ck_assert (list->allocCount == 0);
  ck_assert (list->dsiz == sizeof (namevalue_t *));
  ck_assert (list->ordered == LIST_ORDERED);
  ck_assert (list->type == LIST_NAMEVALUE);
  vlistFree (list);
}
END_TEST

START_TEST(vlist_add_str)
{
  list_t        *list;
  namevalue_t   *nv;
  listkey_t         lkey;

  list = vlistAlloc (KEY_STR, LIST_ORDERED, istringCompare, NULL, NULL);
  ck_assert (list != NULL);
  ck_assert (list->dsiz == sizeof (namevalue_t *));
  lkey.name = "ffff";
  vlistSetData (list, lkey, "555");
  lkey.name = "cccc";
  vlistSetData (list, lkey, "222");
  lkey.name = "eeee";
  vlistSetData (list, lkey, "444");
  lkey.name = "dddd";
  vlistSetData (list, lkey, "333");
  lkey.name = "aaaa";
  vlistSetData (list, lkey, "000");
  lkey.name = "bbbb";
  vlistSetData (list, lkey, "111");
  nv = (namevalue_t *) list->data[0];
  ck_assert (strcmp (nv->key.name, "aaaa") == 0);
  ck_assert (strcmp ((char *) nv->u.data, "000") == 0);
  nv = (namevalue_t *) list->data[5];
  ck_assert (strcmp (nv->key.name, "ffff") == 0);
  ck_assert (strcmp ((char *) nv->u.data, "555") == 0);
  vlistFree (list);
}
END_TEST

START_TEST(vlist_add_sort_str)
{
  list_t        *list;
  namevalue_t   *nv;
  listkey_t         lkey;

  list = vlistAlloc (KEY_STR, LIST_UNORDERED, istringCompare, NULL, NULL);
  ck_assert (list != NULL);
  ck_assert (list->dsiz == sizeof (namevalue_t *));
  lkey.name = "ffff";
  vlistSetLong (list, lkey, 0L);
  lkey.name = "zzzz";
  vlistSetLong (list, lkey, 1L);
  lkey.name = "rrrr";
  vlistSetLong (list, lkey, 2L);
  lkey.name = "kkkk";
  vlistSetLong (list, lkey, 3L);
  lkey.name = "cccc";
  vlistSetLong (list, lkey, 4L);
  lkey.name = "aaaa";
  vlistSetLong (list, lkey, 5L);
  lkey.name = "bbbb";
  vlistSetLong (list, lkey, 6L);
  ck_assert (list->count == 7);
  ck_assert (list->allocCount == 7);
  nv = (namevalue_t *) list->data[0];
  ck_assert (strcmp (nv->key.name, "ffff") == 0);
  ck_assert (nv->u.l == 0L);
  vlistSort (list);
  ck_assert (list->ordered == LIST_ORDERED);
  ck_assert (list->count == 7);
  nv = (namevalue_t *) list->data[0];
  ck_assert (strcmp (nv->key.name, "aaaa") == 0);
  ck_assert (nv->u.l == 5L);
  nv = (namevalue_t *) list->data[4];
  ck_assert (strcmp (nv->key.name, "kkkk") == 0);
  ck_assert (nv->u.l == 3L);
  vlistFree (list);
}
END_TEST

START_TEST(vlist_replace_str)
{
  list_t        *list;
  namevalue_t   *nv;
  listkey_t         lkey;

  list = vlistAlloc (KEY_STR, LIST_ORDERED, istringCompare, NULL, NULL);
  ck_assert (list != NULL);
  ck_assert (list->type == LIST_NAMEVALUE);
  ck_assert (list->dsiz == sizeof (namevalue_t *));
  lkey.name = "aaaa";
  vlistSetData (list, lkey, "000");
  nv = (namevalue_t *) list->data[0];
  ck_assert (strcmp (nv->key.name, "aaaa") == 0);
  ck_assert (strcmp ((char *) nv->u.data, "000") == 0);
  lkey.name = "bbbb";
  vlistSetData (list, lkey, "111");
  lkey.name = "cccc";
  vlistSetData (list, lkey, "222");
  lkey.name = "dddd";
  vlistSetData (list, lkey, "333");
  nv = (namevalue_t *) list->data[0];
  ck_assert (strcmp (nv->key.name, "aaaa") == 0);
  ck_assert (strcmp ((char *) nv->u.data, "000") == 0);
  lkey.name = "eeee";
  vlistSetData (list, lkey, "444");
  lkey.name = "ffff";
  vlistSetData (list, lkey, "555");
  ck_assert (list->count == 6);
  nv = (namevalue_t *) list->data[0];
  ck_assert (strcmp (nv->key.name, "aaaa") == 0);
  ck_assert (strcmp ((char *) nv->u.data, "000") == 0);
  nv = (namevalue_t *) list->data[2];
  ck_assert (strcmp (nv->key.name, "cccc") == 0);
  ck_assert (strcmp ((char *) nv->u.data, "222") == 0);
  lkey.name = "cccc";
  vlistSetData (list, lkey, "666");
  nv = (namevalue_t *) list->data[2];
  ck_assert (strcmp (nv->key.name, "cccc") == 0);
  ck_assert (strcmp ((char *) nv->u.data, "666") == 0);
  vlistFree (list);
}
END_TEST

START_TEST(vlist_get_data_str)
{
  list_t        *list;
  char          *value;
  listkey_t         lkey;

  list = vlistAlloc (KEY_STR, LIST_UNORDERED, istringCompare, NULL, NULL);
  vlistSetSize (list, 7);
  ck_assert (list->count == 0);
  ck_assert (list->allocCount == 7);
  lkey.name = "ffff";
  vlistSetData (list, lkey, "0L");
  lkey.name = "zzzz";
  vlistSetData (list, lkey, "1L");
  lkey.name = "rrrr";
  vlistSetData (list, lkey, "2L");
  lkey.name = "kkkk";
  vlistSetData (list, lkey, "3L");
  lkey.name = "cccc";
  vlistSetData (list, lkey, "4L");
  lkey.name = "aaaa";
  vlistSetData (list, lkey, "5L");
  lkey.name = "bbbb";
  vlistSetData (list, lkey, "6L");
  ck_assert (list->count == 7);
  vlistSort (list);
  ck_assert (list->count == 7);
  lkey.name = "cccc";
  value = (char *) vlistGetData (list, lkey);
  ck_assert (value != NULL);
  ck_assert (strcmp (value, "4L") == 0);
  vlistFree (list);
}
END_TEST

START_TEST(vlist_free_str)
{
  list_t        *list;
  listkey_t         lkey;

  list = vlistAlloc (KEY_STR, LIST_UNORDERED, istringCompare, free, free);
  ck_assert (list != NULL);
  ck_assert (list->dsiz == sizeof (namevalue_t *));
  lkey.name = strdup ("ffff");
  vlistSetData (list, lkey, strdup("0L"));
  lkey.name = strdup ("zzzz");
  vlistSetData (list, lkey, strdup("1L"));
  lkey.name = strdup ("rrrr");
  vlistSetData (list, lkey, strdup("2L"));
  lkey.name = strdup ("kkkk");
  vlistSetData (list, lkey, strdup("3L"));
  lkey.name = strdup ("cccc");
  vlistSetData (list, lkey, strdup("4L"));
  lkey.name = strdup ("aaaa");
  vlistSetData (list, lkey, strdup("5L"));
  lkey.name = strdup ("bbbb");
  vlistSetData (list, lkey, strdup("6L"));
  ck_assert (list->count == 7);
  vlistFree (list);
}
END_TEST

START_TEST(vlist_add_long)
{
  list_t        *list;
  namevalue_t   *nv;
  listkey_t         lkey;

  list = vlistAlloc (KEY_LONG, LIST_ORDERED, istringCompare, NULL, NULL);
  ck_assert (list != NULL);
  ck_assert (list->dsiz == sizeof (namevalue_t *));
  lkey.key = 6L;
  vlistSetData (list, lkey, "555");
  lkey.key = 3L;
  vlistSetData (list, lkey, "222");
  lkey.key = 5L;
  vlistSetData (list, lkey, "444");
  lkey.key = 4L;
  vlistSetData (list, lkey, "333");
  lkey.key = 1L;
  vlistSetData (list, lkey, "000");
  lkey.key = 2L;
  vlistSetData (list, lkey, "111");
  nv = (namevalue_t *) list->data[0];
  ck_assert (nv->key.key == 1L);
  ck_assert (strcmp ((char *) nv->u.data, "000") == 0);
  nv = (namevalue_t *) list->data[5];
  ck_assert (nv->key.key == 6L);
  ck_assert (strcmp ((char *) nv->u.data, "555") == 0);
  vlistFree (list);
}
END_TEST

START_TEST(vlist_add_sort_long)
{
  list_t        *list;
  namevalue_t   *nv;
  listkey_t         lkey;

  list = vlistAlloc (KEY_LONG, LIST_UNORDERED, istringCompare, NULL, NULL);
  ck_assert (list != NULL);
  ck_assert (list->dsiz == sizeof (namevalue_t *));
  lkey.key = 6L;
  vlistSetLong (list, lkey, 0L);
  lkey.key = 26L;
  vlistSetLong (list, lkey, 1L);
  lkey.key = 18L;
  vlistSetLong (list, lkey, 2L);
  lkey.key = 11L;
  vlistSetLong (list, lkey, 3L);
  lkey.key = 3L;
  vlistSetLong (list, lkey, 4L);
  lkey.key = 1L;
  vlistSetLong (list, lkey, 5L);
  lkey.key = 2L;
  vlistSetLong (list, lkey, 6L);
  ck_assert (list->count == 7);
  ck_assert (list->allocCount == 7);
  nv = (namevalue_t *) list->data[0];
  ck_assert (nv->key.key == 6L);
  ck_assert (nv->u.l == 0L);
  vlistSort (list);
  ck_assert (list->ordered == LIST_ORDERED);
  ck_assert (list->count == 7);
  nv = (namevalue_t *) list->data[0];
  ck_assert (nv->key.key == 1L);
  ck_assert (nv->u.l == 5L);
  nv = (namevalue_t *) list->data[4];
  ck_assert (nv->key.key == 11L);
  ck_assert (nv->u.l == 3L);
  vlistFree (list);
}
END_TEST

START_TEST(vlist_replace_long)
{
  list_t        *list;
  namevalue_t   *nv;
  listkey_t         lkey;

  list = vlistAlloc (KEY_LONG, LIST_ORDERED, istringCompare, NULL, NULL);
  ck_assert (list != NULL);
  ck_assert (list->type == LIST_NAMEVALUE);
  ck_assert (list->dsiz == sizeof (namevalue_t *));
  lkey.key = 1L;
  vlistSetData (list, lkey, "000");
  nv = (namevalue_t *) list->data[0];
  ck_assert (nv->key.key == 1L);
  ck_assert (strcmp ((char *) nv->u.data, "000") == 0);
  lkey.key = 2L;
  vlistSetData (list, lkey, "111");
  lkey.key = 3L;
  vlistSetData (list, lkey, "222");
  lkey.key = 4L;
  vlistSetData (list, lkey, "333");
  nv = (namevalue_t *) list->data[0];
  ck_assert (nv->key.key == 1L);
  ck_assert (strcmp ((char *) nv->u.data, "000") == 0);
  lkey.key = 5L;
  vlistSetData (list, lkey, "444");
  lkey.key = 6L;
  vlistSetData (list, lkey, "555");
  ck_assert (list->count == 6);
  nv = (namevalue_t *) list->data[0];
  ck_assert (nv->key.key == 1L);
  ck_assert (strcmp ((char *) nv->u.data, "000") == 0);
  nv = (namevalue_t *) list->data[2];
  ck_assert (nv->key.key == 3L);
  ck_assert (strcmp ((char *) nv->u.data, "222") == 0);
  lkey.key = 3L;
  vlistSetData (list, lkey, "666");
  nv = (namevalue_t *) list->data[2];
  ck_assert (nv->key.key == 3L);
  ck_assert (strcmp ((char *) nv->u.data, "666") == 0);
  vlistFree (list);
}
END_TEST

START_TEST(vlist_get_data_long)
{
  list_t        *list;
  char          *value;
  listkey_t         lkey;

  list = vlistAlloc (KEY_LONG, LIST_UNORDERED, istringCompare, NULL, NULL);
  vlistSetSize (list, 7);
  ck_assert (list->count == 0);
  ck_assert (list->allocCount == 7);
  lkey.key = 6L;
  vlistSetData (list, lkey, "0L");
  lkey.key = 26L;
  vlistSetData (list, lkey, "1L");
  lkey.key = 18L;
  vlistSetData (list, lkey, "2L");
  lkey.key = 11L;
  vlistSetData (list, lkey, "3L");
  lkey.key = 3L;
  vlistSetData (list, lkey, "4L");
  lkey.key = 1L;
  vlistSetData (list, lkey, "5L");
  lkey.key = 2L;
  vlistSetData (list, lkey, "6L");
  ck_assert (list->count == 7);
  vlistSort (list);
  ck_assert (list->count == 7);
  lkey.key = 3L;
  value = (char *) vlistGetData (list, lkey);
  ck_assert (value != NULL);
  ck_assert (strcmp (value, "4L") == 0);
  vlistFree (list);
}
END_TEST

START_TEST(vlist_free_long)
{
  list_t        *list;
  listkey_t         lkey;

  /* the first free() is incorrect, should be ignored */
  list = vlistAlloc (KEY_LONG, LIST_UNORDERED, istringCompare, free, free);
  ck_assert (list != NULL);
  ck_assert (list->dsiz == sizeof (namevalue_t *));
  lkey.key = 6L;
  vlistSetData (list, lkey, strdup("0L"));
  lkey.key = 26L;
  vlistSetData (list, lkey, strdup("1L"));
  lkey.key = 18L;
  vlistSetData (list, lkey, strdup("2L"));
  lkey.key = 11L;
  vlistSetData (list, lkey, strdup("3L"));
  lkey.key = 3L;
  vlistSetData (list, lkey, strdup("4L"));
  lkey.key = 1L;
  vlistSetData (list, lkey, strdup("5L"));
  lkey.key = 2L;
  vlistSetData (list, lkey, strdup("6L"));
  ck_assert (list->count == 7);
  vlistFree (list);
}
END_TEST

Suite *
list_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("List Suite");
  tc = tcase_create ("List");
  tcase_add_test (tc, list_create_free);
  tcase_add_test (tc, list_add_ordered);
  tcase_add_test (tc, list_add_ordered_beg);
  tcase_add_test (tc, list_add_ordered_end);
  tcase_add_test (tc, list_add_ordered_prealloc);
  tcase_add_test (tc, list_add_unordered);
  tcase_add_test (tc, list_add_sort);
  tcase_add_test (tc, list_find);
  tcase_add_test (tc, list_free);
  tcase_add_test (tc, vlist_create_free);
  tcase_add_test (tc, vlist_add_str);
  tcase_add_test (tc, vlist_add_sort_str);
  tcase_add_test (tc, vlist_replace_str);
  tcase_add_test (tc, vlist_get_data_str);
  tcase_add_test (tc, vlist_free_str);
  tcase_add_test (tc, vlist_add_long);
  tcase_add_test (tc, vlist_add_sort_long);
  tcase_add_test (tc, vlist_replace_long);
  tcase_add_test (tc, vlist_get_data_long);
  tcase_add_test (tc, vlist_free_long);
  suite_add_tcase (s, tc);
  return s;
}
