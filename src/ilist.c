#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdjstring.h"
#include "ilist.h"
#include "list.h"
#include "log.h"
#include "nlist.h"
#include "slist.h"

/* key/value list, keyed by a ilistidx_t */

ilist_t *
ilistAlloc (char *name, ilistorder_t ordered, ilistFree_t valueFreeHook)
{
  ilist_t    *list;

  ordered = LIST_ORDERED;
  list = listAlloc (name, ordered, listFree, NULL);
  list->keytype = LIST_KEY_NUM;
  return list;
}

inline void
ilistFree (void *list)
{
  listFree (list);
}

inline ssize_t
ilistGetVersion (ilist_t *list)
{
  if (list == NULL) {
    return 0;
  }
  return list->version;
}

inline ssize_t
ilistGetCount (ilist_t *list)
{
  if (list == NULL) {
    return 0;
  }
  return list->count;
}

inline void
ilistSetSize (ilist_t *list, ssize_t siz)
{
  listSetSize (list, siz);
}

ilistidx_t
ilistSetData (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx, void *data)
{
  listitem_t    item;
  nlist_t       *datalist = NULL;
  char          tbuff [40];

  if (list == NULL) {
    return LIST_LOC_INVALID;
  }

  datalist = nlistGetList (list, ikey);
  if (datalist == NULL) {
    snprintf (tbuff, sizeof (tbuff), "%s-item-%zd", list->name, ikey);
    datalist = nlistAlloc (tbuff, LIST_ORDERED, NULL);
    nlistSetList (list, ikey, datalist);
  }
  if (datalist != NULL) {
    item.key.idx = lidx;
    item.valuetype = VALUE_DATA;
    item.value.data = data;
    return listSet (datalist, &item);
  }
  return LIST_LOC_INVALID;
}

ilistidx_t
ilistSetStr (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx, char *data)
{
  return ilistSetData (list, ikey, lidx, data);
}

ilistidx_t
ilistSetNum (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx, ssize_t data)
{
  listitem_t    item;
  nlist_t       *datalist = NULL;
  char          tbuff [40];

  if (list == NULL) {
    return LIST_LOC_INVALID;
  }

  datalist = nlistGetList (list, ikey);
  if (datalist == NULL) {
    snprintf (tbuff, sizeof (tbuff), "%s-item-%zd", list->name, ikey);
    datalist = nlistAlloc (tbuff, LIST_ORDERED, NULL);
    nlistSetList (list, ikey, datalist);
  }
  if (datalist != NULL) {
    item.key.idx = lidx;
    item.valuetype = VALUE_NUM;
    item.value.num = data;
    return listSet (datalist, &item);
  }
  return LIST_LOC_INVALID;
}

ilistidx_t
ilistSetDouble (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx, double data)
{
  listitem_t    item;
  nlist_t       *datalist = NULL;
  char          tbuff [40];

  if (list == NULL) {
    return LIST_LOC_INVALID;
  }

  datalist = nlistGetList (list, ikey);
  if (datalist == NULL) {
    snprintf (tbuff, sizeof (tbuff), "%s-item-%zd", list->name, ikey);
    datalist = nlistAlloc (tbuff, LIST_ORDERED, NULL);
    nlistSetList (list, ikey, datalist);
  }
  if (datalist != NULL) {
    item.key.idx = lidx;
    item.valuetype = VALUE_DOUBLE;
    item.value.dval = data;
    return listSet (datalist, &item);
  }
  return LIST_LOC_INVALID;
}

void *
ilistGetData (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx)
{
  void            *value = NULL;
  listkey_t       key;
  ilistidx_t       idx = 0;
  ilist_t          *datalist = NULL;

  if (list == NULL) {
    return NULL;
  }

  datalist = nlistGetList (list, ikey);
  if (datalist != NULL) {
    key.idx = lidx;
    idx = listGetIdx (datalist, &key);
    if (idx >= 0) {
      value = (char *) datalist->data [idx].value.data;
    }
    logMsg (LOG_DBG, LOG_LIST, "ilist:%s key:%ld idx:%ld", datalist->name, lidx, idx);
  }
  return value;
}

char *
ilistGetStr (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx)
{
  return ilistGetData (list, ikey, lidx);
}

ssize_t
ilistGetNum (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx)
{
  ssize_t     value = LIST_VALUE_INVALID;
  listkey_t   key;
  ilistidx_t   idx;
  nlist_t     *datalist;

  if (list == NULL) {
    return LIST_VALUE_INVALID;
  }

  datalist = nlistGetList (list, ikey);
  if (datalist != NULL) {
    key.idx = lidx;
    idx = listGetIdx (datalist, &key);
    if (idx >= 0) {
      value = datalist->data [idx].value.num;
    }
    logMsg (LOG_DBG, LOG_LIST, "list:%s key:%ld idx:%ld value:%ld", list->name, lidx, idx, value);
  }
  return value;
}

double
ilistGetDouble (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx)
{
  double          value = LIST_DOUBLE_INVALID;
  listkey_t       key;
  ilistidx_t      idx;
  nlist_t         *datalist;

  if (list == NULL) {
    return LIST_DOUBLE_INVALID;
  }

  datalist = nlistGetList (list, ikey);
  if (datalist != NULL) {
    key.idx = lidx;
    idx = listGetIdx (datalist, &key);
    if (idx >= 0) {
      value = datalist->data [idx].value.dval;
    }
    logMsg (LOG_DBG, LOG_LIST, "list:%s key:%ld idx:%ld value:%8.2g", list->name, lidx, idx, value);
  }
  return value;
}

slist_t *
ilistGetList (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx)
{
  return ilistGetData (list, ikey, lidx);
}


inline void
ilistSort (ilist_t *list)
{
  listSort (list);
}

inline void
ilistStartIterator (ilist_t *list, ilistidx_t *iteridx)
{
  *iteridx = -1;
}

ilistidx_t
ilistIterateKey (ilist_t *list, ilistidx_t *iteridx)
{
  ssize_t      value = LIST_LOC_INVALID;

  logProcBegin (LOG_PROC, "ilistIterateKey");
  if (list == NULL || list->keytype == LIST_KEY_STR) {
    logProcEnd (LOG_PROC, "ilistIterateKey", "null-list/key-str");
    return LIST_LOC_INVALID;
  }

  ++(*iteridx);
  if (*iteridx >= list->count) {
    *iteridx = -1;
    logProcEnd (LOG_PROC, "ilistIterateKey", "end-list");
    return LIST_LOC_INVALID;      /* indicate the end of the list */
  }

  value = list->data [*iteridx].key.idx;

  list->keyCache.idx = value;
  list->locCache = *iteridx;

  logProcEnd (LOG_PROC, "ilistIterateKey", "");
  return value;
}

void
ilistDumpInfo (ilist_t *list)
{
  listDumpInfo (list);
}

/* indirect routines */

