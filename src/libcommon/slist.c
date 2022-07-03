#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdjstring.h"
#include "list.h"
#include "log.h"
#include "slist.h"
#include "bdjstring.h"

static void   slistSetKey (list_t *list, listkey_t *key, char *keydata);
static void   slistUpdateMaxKeyWidth (list_t *list, char *keydata);

/* key/value list, keyed by a listidx_t */

slist_t *
slistAlloc (char *name, listorder_t ordered, slistFree_t valueFreeHook)
{
  slist_t    *list;

  list = listAlloc (name, ordered, valueFreeHook);
  list->keytype = LIST_KEY_STR;
  return list;
}

inline void
slistFree (void *list)
{
  listFree (list);
}

inline ssize_t
slistGetCount (slist_t *list)
{
  if (list == NULL) {
    return 0;
  }
  return list->count;
}

inline void
slistSetSize (slist_t *list, ssize_t siz)
{
  listSetSize (list, siz);
}

void
slistSetData (slist_t *list, char *sidx, void *data)
{
  listitem_t    item;

  slistSetKey (list, &item.key, sidx);
  item.valuetype = VALUE_DATA;
  item.value.data = data;

  slistUpdateMaxKeyWidth (list, sidx);

  listSet (list, &item);
}

void
slistSetStr (slist_t *list, char *sidx, const char *data)
{
  listitem_t    item;

  slistSetKey (list, &item.key, sidx);
  item.valuetype = VALUE_STR;
  item.value.data = NULL;
  if (data != NULL) {
    item.value.data = strdup (data);
  }

  slistUpdateMaxKeyWidth (list, sidx);

  listSet (list, &item);
}

void
slistSetNum (slist_t *list, char *sidx, ssize_t data)
{
  listitem_t    item;

  slistSetKey (list, &item.key, sidx);
  item.valuetype = VALUE_NUM;
  item.value.num = data;

  slistUpdateMaxKeyWidth (list, sidx);

  listSet (list, &item);
}

void
slistSetDouble (slist_t *list, char *sidx, double data)
{
  listitem_t    item;

  slistSetKey (list, &item.key, sidx);
  item.valuetype = VALUE_DOUBLE;
  item.value.dval = data;

  slistUpdateMaxKeyWidth (list, sidx);

  listSet (list, &item);
}

void
slistSetList (slist_t *list, char *sidx, slist_t *data)
{
  listitem_t    item;

  slistSetKey (list, &item.key, sidx);
  item.valuetype = VALUE_LIST;
  item.value.data = data;

  slistUpdateMaxKeyWidth (list, sidx);

  listSet (list, &item);
}

slistidx_t
slistGetIdx (slist_t *list, char *sidx)
{
  listkey_t   key;

  key.strkey = (char *) sidx;
  return listGetIdx (list, &key);
}

void *
slistGetData (slist_t *list, const char *sidx)
{
  void            *value = NULL;
  listkey_t       key;
  slistidx_t      idx;

  key.strkey = (char *) sidx;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = (char *) list->data [idx].value.data;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:%s key:%s idx:%ld", list->name, sidx, idx);
  return value;
}

char *
slistGetStr (slist_t *list, const char *sidx)
{
  return slistGetData (list, sidx);
}

inline void *
slistGetDataByIdx (slist_t *list, slistidx_t idx)
{
  return listGetDataByIdx (list, idx);
}

inline ssize_t
slistGetNumByIdx (slist_t *list, slistidx_t idx)
{
  return listGetNumByIdx (list, idx);
}

char *
slistGetKeyByIdx (slist_t *list, slistidx_t idx)
{
  if (list == NULL) {
    return NULL;
  }
  if (idx < 0 || idx >= list->count) {
    return NULL;
  }

  return list->data [idx].key.strkey;
}

ssize_t
slistGetNum (slist_t *list, const char *sidx)
{
  ssize_t         value = LIST_VALUE_INVALID;
  listkey_t       key;
  slistidx_t      idx;

  key.strkey = (char *) sidx;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.num;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:%s key:%s idx:%ld value:%ld", list->name, sidx, idx, value);
  return value;
}

double
slistGetDouble (slist_t *list, char *sidx)
{
  double         value = LIST_DOUBLE_INVALID;
  listkey_t      key;
  slistidx_t      idx;

  key.strkey = sidx;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.dval;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:%s key:%s idx:%ld value:%8.2g", list->name, sidx, idx, value);
  return value;
}

int
slistGetMaxKeyWidth (slist_t *list)
{
  if (list == NULL) {
    return 0;
  }

  return list->maxKeyWidth;
}

int
slistGetMaxDataWidth (slist_t *list)
{
  if (list == NULL) {
    return 0;
  }

  return list->maxDataWidth;
}

slistidx_t
slistIterateGetIdx (slist_t *list, slistidx_t *iteridx)
{
  return listIterateGetIdx (list, iteridx);
}

inline void
slistSort (slist_t *list)
{
  listSort (list);
}

inline void
slistStartIterator (slist_t *list, slistidx_t *iteridx)
{
  *iteridx = -1;
}

char *
slistIterateKey (slist_t *list, slistidx_t *iteridx)
{
  char    *value = NULL;

  logProcBegin (LOG_PROC, "slistIterateKey");
  if (list == NULL || list->keytype == LIST_KEY_NUM) {
    logProcEnd (LOG_PROC, "slistIterateKey", "null-list/key-num");
    return NULL;
  }

  ++(*iteridx);
  if (*iteridx >= list->count) {
    *iteridx = -1;
    logProcEnd (LOG_PROC, "slistIterateKey", "end-list");
    return NULL;
  }

  value = list->data [*iteridx].key.strkey;

  if (list->keyCache.strkey != NULL) {
    free (list->keyCache.strkey);
    list->keyCache.strkey = NULL;
    list->locCache = LIST_LOC_INVALID;
  }

  list->keyCache.strkey = strdup (value);
  list->locCache = *iteridx;

  logProcEnd (LOG_PROC, "slistIterateKey", "");
  return value;
}

inline void *
slistIterateValueData (slist_t *list, slistidx_t *iteridx)
{
  return listIterateValue (list, iteridx);
}

inline ssize_t
slistIterateValueNum (slist_t *list, slistidx_t *iteridx)
{
  return listIterateNum (list, iteridx);
}

void
slistDumpInfo (slist_t *list)
{
  listDumpInfo (list);
}

/* internal routines */

static void
slistSetKey (list_t *list, listkey_t *key, char *keydata)
{
  key->strkey = strdup (keydata);
}

static void
slistUpdateMaxKeyWidth (list_t *list, char *keydata)
{
  if (list == NULL) {
    return;
  }

  if (keydata != NULL) {
    ssize_t       len;

    len = istrlen (keydata);
    if (len > list->maxKeyWidth) {
      list->maxKeyWidth = len;
    }
  }
}
