#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "list.h"
#include "log.h"
#include "nlist.h"
#include "bdjstring.h"

/* key/value list, keyed by a nlistidx_t */

nlist_t *
nlistAlloc (char *name, nlistorder_t ordered, nlistFree_t valueFreeHook)
{
  nlist_t    *list;

  list = listAlloc (name, ordered, valueFreeHook);
  list->keytype = LIST_KEY_NUM;
  return list;
}

inline void
nlistFree (void *list)
{
  listFree (list);
}

inline ssize_t
nlistGetCount (nlist_t *list)
{
  if (list == NULL) {
    return 0;
  }
  return list->count;
}

inline void
nlistSetSize (nlist_t *list, ssize_t siz)
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
nlistSetNum (nlist_t *list, nlistidx_t lkey, ssize_t data)
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
  listitem_t      item;
  nlistidx_t      idx;
  ssize_t         value = 0;

  item.key.idx = lkey;
  idx = listGetIdx (list, &item.key);
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
  listitem_t      item;
  nlistidx_t      idx;
  ssize_t         value = 0;

  item.key.idx = lkey;
  idx = listGetIdx (list, &item.key);
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
  void        *value = NULL;
  listkey_t   key;
  nlistidx_t  idx;

  if (list == NULL) {
    return NULL;
  }

  key.idx = lkey;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = (char *) list->data [idx].value.data;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:%s key:%ld idx:%ld", list->name, lkey, idx);
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
  listkey_t   key;
  nlistidx_t  idx;

  if (list == NULL) {
    return LIST_LOC_INVALID;
  }
  if (lkey < 0 || lkey >= list->count) {
    return LIST_LOC_INVALID;
  }

  key.idx = lkey;
  idx = listGetIdx (list, &key);
  return idx;
}

inline void *
nlistGetDataByIdx (nlist_t *list, nlistidx_t idx)
{
  return listGetDataByIdx (list, idx);
}

inline ssize_t
nlistGetNumByIdx (nlist_t *list, nlistidx_t idx)
{
  return listGetNumByIdx (list, idx);
}

inline nlistidx_t
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

ssize_t
nlistGetNum (nlist_t *list, nlistidx_t lidx)
{
  ssize_t         value = LIST_VALUE_INVALID;
  listkey_t       key;
  nlistidx_t      idx;

  if (list == NULL) {
    return LIST_VALUE_INVALID;
  }

  key.idx = lidx;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.num;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:%s key:%ld idx:%ld value:%ld", list->name, lidx, idx, value);
  return value;
}

double
nlistGetDouble (nlist_t *list, nlistidx_t lidx)
{
  double         value = LIST_DOUBLE_INVALID;
  listkey_t      key;
  nlistidx_t      idx;

  if (list == NULL) {
    return LIST_DOUBLE_INVALID;
  }

  key.idx = lidx;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.dval;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:%s key:%ld idx:%ld value:%.6f", list->name, lidx, idx, value);
  return value;
}

nlist_t *
nlistGetList (nlist_t *list, nlistidx_t lidx)
{
  void            *value = NULL;
  listkey_t       key;
  nlistidx_t      idx;

  if (list == NULL) {
    return NULL;
  }

  key.idx = lidx;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = (char *) list->data [idx].value.data;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:%s key:%ld idx:%ld", list->name, lidx, idx);
  return value;
}

inline void
nlistSort (nlist_t *list)
{
  listSort (list);
}

inline void
nlistStartIterator (nlist_t *list, nlistidx_t *iteridx)
{
  *iteridx = -1;
}

nlistidx_t
nlistIterateKey (nlist_t *list, nlistidx_t *iteridx)
{
  ssize_t      value = LIST_LOC_INVALID;

  logProcBegin (LOG_PROC, "nlistIterateKey");
  if (list == NULL || list->keytype == LIST_KEY_STR) {
    logProcEnd (LOG_PROC, "nlistIterateKey", "null-list/key-str");
    return LIST_LOC_INVALID;
  }

  ++(*iteridx);
  if (*iteridx >= list->count) {
    *iteridx = -1;
    logProcEnd (LOG_PROC, "nlistIterateKey", "end-list");
    return LIST_LOC_INVALID;      /* indicate the end of the list */
  }

  value = list->data [*iteridx].key.idx;

  list->keyCache.idx = value;
  list->locCache = *iteridx;

  logProcEnd (LOG_PROC, "nlistIterateKey", "");
  return value;
}

inline void *
nlistIterateValueData (nlist_t *list, nlistidx_t *iteridx)
{
  return listIterateValue (list, iteridx);
}

inline ssize_t
nlistIterateValueNum (nlist_t *list, nlistidx_t *iteridx)
{
  return listIterateNum (list, iteridx);
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

