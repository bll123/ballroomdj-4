/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "list.h"
#include "log.h"
#include "nlist.h"
#include "bdjstring.h"

/* key/value list, keyed by a nlistidx_t */

nlist_t *
nlistAlloc (const char *name, nlistorder_t ordered, nlistFree_t valueFreeHook)
{
  nlist_t    *list;

  list = listAlloc (name, ordered, valueFreeHook);
  list->keytype = LIST_KEY_NUM;
  return list;
}

void
nlistFree (void *list)
{
  listFree (list);
}

void
nlistSetVersion (nlist_t *list, int version)
{
  listSetVersion (list, version);
}

int
nlistGetVersion (nlist_t *list)
{
  return listGetVersion (list);
}

nlistidx_t
nlistGetCount (nlist_t *list)
{
  if (list == NULL) {
    return 0;
  }
  return list->count;
}

void
nlistSetSize (nlist_t *list, nlistidx_t siz)
{
  listSetSize (list, siz);
}

void
nlistSetFreeHook (nlist_t *list, listFree_t valueFreeHook)
{
  if (list == NULL) {
    return;
  }

  list->valueFreeHook = valueFreeHook;
}

void
nlistSetData (nlist_t *list, nlistidx_t lkey, void *data)
{
  listitem_t    item;

  item.key.idx = lkey;
  item.valuetype = VALUE_DATA;
  item.value.data = data;
  listSet (list, &item);
}

void
nlistSetStr (nlist_t *list, nlistidx_t lkey, const char *data)
{
  listitem_t    item;

  item.key.idx = lkey;
  item.valuetype = VALUE_STR;
  item.value.data = NULL;
  if (data != NULL) {
    item.value.data = strdup (data);
  }
  listSet (list, &item);
}

void
nlistSetNum (nlist_t *list, nlistidx_t lkey, listnum_t data)
{
  listitem_t    item;

  item.key.idx = lkey;
  item.valuetype = VALUE_NUM;
  item.value.num = data;
  listSet (list, &item);
}

void
nlistSetDouble (nlist_t *list, nlistidx_t lkey, double data)
{
  listitem_t    item;

  item.key.idx = lkey;
  item.valuetype = VALUE_DOUBLE;
  item.value.dval = data;
  listSet (list, &item);
}

void
nlistSetList (nlist_t *list, nlistidx_t lkey, nlist_t *data)
{
  listitem_t    item;

  item.key.idx = lkey;
  item.valuetype = VALUE_LIST;
  item.value.data = data;
  listSet (list, &item);
}

void
nlistIncrement (nlist_t *list, nlistidx_t lkey)
{
  listkeylookup_t key;
  listitem_t      item;
  nlistidx_t      idx;
  listnum_t       value = 0;

  key.idx = lkey;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.num;
  }
  ++value;
  item.key.idx = lkey;
  item.valuetype = VALUE_NUM;
  item.value.num = value;
  listSet (list, &item);
}

void
nlistDecrement (nlist_t *list, nlistidx_t lkey)
{
  listkeylookup_t key;
  listitem_t      item;
  nlistidx_t      idx;
  listnum_t       value = 0;

  key.idx = lkey;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.num;
  }
  --value;
  item.key.idx = lkey;
  item.valuetype = VALUE_NUM;
  item.value.num = value;
  listSet (list, &item);
}

void *
nlistGetData (nlist_t *list, nlistidx_t lkey)
{
  void            *value = NULL;
  listkeylookup_t key;
  nlistidx_t      idx;

  if (list == NULL) {
    return NULL;
  }

  key.idx = lkey;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.data;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:%s key:%d idx:%d", list->name, lkey, idx);
  return value;
}

char *
nlistGetStr (nlist_t *list, nlistidx_t lkey)
{
  return nlistGetData (list, lkey);
}

nlistidx_t
nlistGetIdx (nlist_t *list, nlistidx_t lkey)
{
  listkeylookup_t key;
  nlistidx_t      idx;

  if (list == NULL) {
    return LIST_LOC_INVALID;
  }

  key.idx = lkey;
  idx = listGetIdx (list, &key);
  return idx;
}

void *
nlistGetDataByIdx (nlist_t *list, nlistidx_t idx)
{
  return listGetDataByIdx (list, idx);
}

listnum_t
nlistGetNumByIdx (nlist_t *list, nlistidx_t idx)
{
  return listGetNumByIdx (list, idx);
}

nlistidx_t
nlistGetKeyByIdx (nlist_t *list, nlistidx_t idx)
{
  if (list == NULL) {
    return LIST_LOC_INVALID;
  }
  if (idx < 0 || idx >= list->count) {
    return LIST_LOC_INVALID;
  }

  return list->data [idx].key.idx;
}

listnum_t
nlistGetNum (nlist_t *list, nlistidx_t lidx)
{
  listnum_t       value = LIST_VALUE_INVALID;
  listkeylookup_t key;
  nlistidx_t      idx;

  if (list == NULL) {
    return LIST_VALUE_INVALID;
  }

  key.idx = lidx;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.num;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:%s key:%d idx:%d value:%"PRId64, list->name, lidx, idx, value);
  return value;
}

double
nlistGetDouble (nlist_t *list, nlistidx_t lidx)
{
  double          value = LIST_DOUBLE_INVALID;
  listkeylookup_t key;
  nlistidx_t      idx;

  if (list == NULL) {
    return LIST_DOUBLE_INVALID;
  }

  key.idx = lidx;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.dval;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:%s key:%d idx:%d value:%.6f", list->name, lidx, idx, value);
  return value;
}

nlist_t *
nlistGetList (nlist_t *list, nlistidx_t lidx)
{
  void            *value = NULL;
  listkeylookup_t key;
  nlistidx_t      idx;

  if (list == NULL) {
    return NULL;
  }

  key.idx = lidx;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.data;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:%s key:%d idx:%d", list->name, lidx, idx);
  return value;
}

void
nlistSort (nlist_t *list)
{
  listSort (list);
}

void
nlistStartIterator (nlist_t *list, nlistidx_t *iteridx)
{
  *iteridx = LIST_END_LIST;
}

nlistidx_t
nlistIterateKey (nlist_t *list, nlistidx_t *iteridx)
{
  return listIterateKeyNum (list, iteridx);
}

nlistidx_t
nlistIterateKeyPrevious (nlist_t *list, nlistidx_t *iteridx)
{
  return listIterateKeyPreviousNum (list, iteridx);
}

void *
nlistIterateValueData (nlist_t *list, nlistidx_t *iteridx)
{
  return listIterateValue (list, iteridx);
}

listnum_t
nlistIterateValueNum (nlist_t *list, nlistidx_t *iteridx)
{
  return listIterateValueNum (list, iteridx);
}

void
nlistDumpInfo (nlist_t *list)
{
  listDumpInfo (list);
}

/* returns the key value, not the table index */
nlistidx_t
nlistSearchProbTable (nlist_t *probTable, double dval)
{
  nlistidx_t        l = 0;
  nlistidx_t        r = 0;
  nlistidx_t        m = 0;
  int               rca = 0;
  int               rcb = 0;
  double            d;


  r = probTable->count - 1;

  while (l <= r) {
    m = l + (r - l) / 2;

    if (m != 0) {
      d = probTable->data [m-1].value.dval;
      rca = dval > d;
    }
    d = probTable->data [m].value.dval;
    rcb = dval <= d;
    if ((m == 0 || rca) && rcb) {
      return probTable->data [m].key.idx;
    }

    if (! rcb) {
      l = m + 1;
    } else {
      r = m - 1;
    }
  }

  return -1;
}

