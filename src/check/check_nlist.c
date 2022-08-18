#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#pragma clang diagnostic push
#pragma gcc diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "nlist.h"
#include "bdjstring.h"
#include "check_bdj.h"
#include "log.h"

typedef struct {
  int     alloc;
  char    *value;
} chk_item_t;

static chk_item_t *allocItem (const char *str);
static void freeItem (void *item);

START_TEST(nlist_create_free)
{
  nlist_t    *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_create_free");


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

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_version");

  list = nlistAlloc ("chk-b", LIST_ORDERED, NULL);
  nlistSetVersion (list, 20);
  ck_assert_int_eq (list->version, 20);
  vers = nlistGetVersion (list);
  ck_assert_int_eq (vers, 20);
  nlistFree (list);
}
END_TEST

START_TEST(nlist_u_set)
{
  nlist_t        *list;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_u_set");

  list = nlistAlloc ("chk-c", LIST_UNORDERED, NULL);
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
  nlistFree (list);
}
END_TEST

START_TEST(nlist_u_getbyidx)
{
  nlist_t       *list;
  char          *value;
  ssize_t       key;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_u_getbyidx");

  list = nlistAlloc ("chk-d", LIST_UNORDERED, NULL);
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

  /* unordered */
  key = nlistGetKeyByIdx (list, 0);
  ck_assert_int_eq (key, 6);
  key = nlistGetKeyByIdx (list, 4);
  ck_assert_int_eq (key, 3);

  value = (char *) nlistGetDataByIdx (list, 1);
  ck_assert_str_eq (value, "1L");
  value = (char *) nlistGetDataByIdx (list, 5);
  ck_assert_str_eq (value, "5L");

  nlistFree (list);
}
END_TEST

START_TEST(nlist_u_iterate)
{
  nlist_t       *list;
  nlistidx_t    iteridx;
  char          *value;
  ssize_t       key;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_u_iterate");

  list = nlistAlloc ("chk-e", LIST_UNORDERED, NULL);
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

START_TEST(nlist_s_get_str)
{
  nlist_t        *list;
  char          *value;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_s_get_str");


  list = nlistAlloc ("chk-f", LIST_UNORDERED, NULL);
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

START_TEST(nlist_s_iterate_str)
{
  nlist_t *      list;
  char *        value;
  nlistidx_t          key;
  nlistidx_t    iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_s_iterate_str");


  list = nlistAlloc ("chk-g", LIST_ORDERED, NULL);
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

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_set_get_num");

  list = nlistAlloc ("chk-h", LIST_UNORDERED, NULL);
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

START_TEST(nlist_s_replace_str)
{
  nlist_t       *list;
  nlistidx_t    key;
  char          *value;
  nlistidx_t    iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_s_replace_str");

  list = nlistAlloc ("chk-i", LIST_ORDERED, NULL);
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
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "111");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 3);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "222");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 4);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "333");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 5);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "444");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 6);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "555");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, -1);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");

  nlistSetStr (list, 2, "666");
  nlistSetStr (list, 3, "777");
  nlistSetStr (list, 4, "888");

  nlistStartIterator (list, &iteridx);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "000");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "666");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 3);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "777");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 4);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "888");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 5);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "444");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 6);
  value = (char *) nlistGetStr (list, key);
  ck_assert_str_eq (value, "555");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, -1);
  key = nlistIterateKey (list, &iteridx);
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


  list = nlistAlloc ("chk-j", LIST_UNORDERED, free);
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

START_TEST(nlist_free_item)
{
  nlist_t       *list;
  chk_item_t    *item [20];
  int           itemc = 0;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_free_item");


  list = nlistAlloc ("chk-k", LIST_UNORDERED, freeItem);
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

START_TEST(nlist_set_get_mixed)
{
  nlist_t *       list;
  ssize_t         nval;
  char            *sval;
  double          dval;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_set_get_mixed");

  list = nlistAlloc ("chk-l", LIST_ORDERED, free);
  ck_assert_ptr_nonnull (list);
  nlistSetNum (list, 6, 0);
  nlistSetDouble (list, 26, 1.0);
  nlistSetStr (list, 18, "2");
  ck_assert_int_eq (list->count, 3);
  ck_assert_int_eq (list->allocCount, 3);

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

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_inc_dec");

  list = nlistAlloc ("chk-m", LIST_ORDERED, free);
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

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_byidx");

  list = nlistAlloc ("chk-n", LIST_ORDERED, NULL);
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

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_byidx_bug_20220815");

  list = nlistAlloc ("chk-n", LIST_ORDERED, NULL);
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

  logMsg (LOG_DBG, LOG_IMPORTANT, "==== nlist_prob_search");

  list = nlistAlloc ("chk-o", LIST_ORDERED, NULL);
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

Suite *
nlist_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("nlist");
  tc = tcase_create ("nlist");
  tcase_add_test (tc, nlist_create_free);
  tcase_add_test (tc, nlist_version);
  tcase_add_test (tc, nlist_u_set);
  tcase_add_test (tc, nlist_u_getbyidx);
  tcase_add_test (tc, nlist_u_iterate);
  tcase_add_test (tc, nlist_s_get_str);
  tcase_add_test (tc, nlist_s_iterate_str);
  tcase_add_test (tc, nlist_set_get_num);
  tcase_add_test (tc, nlist_s_replace_str);
  tcase_add_test (tc, nlist_free_str);
  tcase_add_test (tc, nlist_free_item);
  tcase_add_test (tc, nlist_set_get_mixed);
  tcase_add_test (tc, nlist_inc_dec);
  tcase_add_test (tc, nlist_byidx);
  tcase_add_test (tc, nlist_byidx_bug_20220815);
  tcase_add_test (tc, nlist_prob_search);
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

