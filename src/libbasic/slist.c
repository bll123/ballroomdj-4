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

#include "bdjstring.h"
#include "istring.h"
#define BDJ4_LIST_MODULE 1
#include "list.h"
#undef BDJ4_LIST_MODULE
#include "log.h"
#include "mdebug.h"
#include "slist.h"

static void   slistSetKey (list_t *list, listkey_t *key, const char *keydata);
static void   slistUpdateMaxKeyWidth (list_t *list, const char *keydata);

/* key/value list, keyed by a listidx_t */

slist_t *
slistAlloc (const char *name, listorder_t ordered, slistFree_t valueFreeHook)
{
  slist_t    *list;

  list = listAlloc (name, LIST_KEY_STR, ordered, valueFreeHook);
  return list;
}

void
slistFree (void *list)
{
  listFree (LIST_KEY_STR, list);
}

void
slistSetVersion (slist_t *list, int version)
{
  listSetVersion (LIST_KEY_STR, list, version);
}

int
slistGetVersion (slist_t *list)
{
  return listGetVersion (LIST_KEY_STR, list);
}

slistidx_t
slistGetCount (slist_t *list)
{
  return listGetCount (LIST_KEY_STR, list);
}

/* for testing */
slistidx_t
slistGetAllocCount (slist_t *list)
{
  return listGetAllocCount (LIST_KEY_STR, list);
}

void
slistSetSize (slist_t *list, listidx_t siz)
{
  listSetSize (LIST_KEY_STR, list, siz);
}

void
slistSetData (slist_t *list, const char *sidx, void *data)
{
  listitem_t    item;

  slistSetKey (list, &item.key, sidx);
  item.valuetype = VALUE_DATA;
  item.value.data = data;

  slistUpdateMaxKeyWidth (list, sidx);

  listSet (LIST_KEY_STR, list, &item);
}

void
slistSetStr (slist_t *list, const char *sidx, const char *data)
{
  listitem_t    item;

  slistSetKey (list, &item.key, sidx);
  item.valuetype = VALUE_STR;
  item.value.data = NULL;
  if (data != NULL) {
    item.value.data = mdstrdup (data);
  }

  slistUpdateMaxKeyWidth (list, sidx);

  listSet (LIST_KEY_STR, list, &item);
}

void
slistSetNum (slist_t *list, const char *sidx, listnum_t data)
{
  listitem_t    item;

  slistSetKey (list, &item.key, sidx);
  item.valuetype = VALUE_NUM;
  item.value.num = data;

  slistUpdateMaxKeyWidth (list, sidx);

  listSet (LIST_KEY_STR, list, &item);
}

void
slistSetDouble (slist_t *list, const char *sidx, double data)
{
  listitem_t    item;

  slistSetKey (list, &item.key, sidx);
  item.valuetype = VALUE_DOUBLE;
  item.value.dval = data;

  slistUpdateMaxKeyWidth (list, sidx);

  listSet (LIST_KEY_STR, list, &item);
}

void
slistSetList (slist_t *list, const char *sidx, slist_t *data)
{
  listitem_t    item;

  slistSetKey (list, &item.key, sidx);
  item.valuetype = VALUE_LIST;
  item.value.data = data;

  slistUpdateMaxKeyWidth (list, sidx);

  listSet (LIST_KEY_STR, list, &item);
}

slistidx_t
slistGetIdx (slist_t *list, const char *sidx)
{
  listkeylookup_t   key;

  if (sidx == NULL) {
    return LIST_LOC_INVALID;
  }

  key.strkey = sidx;
  return listGetIdx (LIST_KEY_STR, list, &key);
}

void *
slistGetData (slist_t *list, const char *sidx)
{
  return listGetData (LIST_KEY_STR, list, sidx);
}

char *
slistGetStr (slist_t *list, const char *sidx)
{
  return listGetData (LIST_KEY_STR, list, sidx);
}

void *
slistGetDataByIdx (slist_t *list, slistidx_t idx)
{
  return listGetDataByIdx (LIST_KEY_STR, list, idx);
}

listnum_t
slistGetNumByIdx (slist_t *list, slistidx_t idx)
{
  return listGetNumByIdx (LIST_KEY_STR, list, idx);
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

listnum_t
slistGetNum (slist_t *list, const char *sidx)
{
  listnum_t       value = LIST_VALUE_INVALID;
  listkeylookup_t key;
  slistidx_t      idx;

  key.strkey = sidx;
  idx = listGetIdx (LIST_KEY_STR, list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.num;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:%s key:%s idx:%d value:%" PRId64, list->name, sidx, idx, value);
  return value;
}

double
slistGetDouble (slist_t *list, const char *sidx)
{
  double          value = LIST_DOUBLE_INVALID;
  listkeylookup_t key;
  slistidx_t      idx;

  key.strkey = sidx;
  idx = listGetIdx (LIST_KEY_STR, list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.dval;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:%s key:%s idx:%d value:%8.2g", list->name, sidx, idx, value);
  return value;
}

slist_t *
slistGetList (slist_t *list, const char *sidx)
{
  void            *value = NULL;
  listkeylookup_t key;
  slistidx_t      idx;

  if (list == NULL) {
    return NULL;
  }

  key.strkey = sidx;
  idx = listGetIdx (LIST_KEY_STR, list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.data;
  }
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
  return listIterateGetIdx (LIST_KEY_STR, list, iteridx);
}

void
slistSort (slist_t *list)
{
  listSort (LIST_KEY_STR, list);
}

void
slistStartIterator (slist_t *list, slistidx_t *iteridx)
{
  *iteridx = -1;
}

char *
slistIterateKey (slist_t *list, slistidx_t *iteridx)
{

  return listIterateKeyStr (LIST_KEY_STR, list, iteridx);
}

void *
slistIterateValueData (slist_t *list, slistidx_t *iteridx)
{
  return listIterateValue (LIST_KEY_STR, list, iteridx);
}

listnum_t
slistIterateValueNum (slist_t *list, slistidx_t *iteridx)
{
  return listIterateValueNum (LIST_KEY_STR, list, iteridx);
}

void
slistDumpInfo (slist_t *list)
{
  listDumpInfo (LIST_KEY_STR, list);
}

/* internal routines */

static void
slistSetKey (list_t *list, listkey_t *key, const char *keydata)
{
  key->strkey = mdstrdup (keydata);
}

static void
slistUpdateMaxKeyWidth (list_t *list, const char *keydata)
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
