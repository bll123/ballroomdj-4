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

static void ilistSetDataValType (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx,
    void *data, valuetype_t valueType);
static void ilistSetDataItem (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx,
    listitem_t *item);

/* key/value list, keyed by a ilistidx_t */

ilist_t *
ilistAlloc (char *name, ilistorder_t ordered)
{
  ilist_t    *list;

  list = listAlloc (name, ordered, listFree);
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

void
ilistSetStr (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx, char *data)
{
  if (data != NULL) {
    data = strdup (data);
  }
  ilistSetDataValType (list, ikey, lidx, data, VALUE_STR);
}

void
ilistSetList (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx, void *data)
{
  ilistSetDataValType (list, ikey, lidx, data, VALUE_LIST);
}

void
ilistSetData (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx, void *data)
{
  ilistSetDataValType (list, ikey, lidx, data, VALUE_DATA);
}

void
ilistSetNum (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx, ssize_t data)
{
  listitem_t    item;

  if (list == NULL) {
    return;
  }

  item.key.idx = lidx;
  item.valuetype = VALUE_NUM;
  item.value.num = data;
  ilistSetDataItem (list, ikey, lidx, &item);
}

void
ilistSetDouble (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx, double data)
{
  listitem_t    item;

  if (list == NULL) {
    return;
  }

  item.key.idx = lidx;
  item.valuetype = VALUE_DOUBLE;
  item.value.dval = data;

  ilistSetDataItem (list, ikey, lidx, &item);
}

bool
ilistExists (list_t *list, ilistidx_t ikey)
{
  ilist_t   *datalist = NULL;

  if (list == NULL) {
    return false;
  }

  datalist = nlistGetList (list, ikey);
  if (datalist == NULL) {
    return false;
  }

  return true;
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

/* internal routines */

static void
ilistSetDataValType (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx,
    void *data, valuetype_t valueType)
{
  listitem_t    item;

  if (list == NULL) {
    return;
  }

  item.key.idx = lidx;
  item.valuetype = valueType;
  item.value.data = data;
  ilistSetDataItem (list, ikey, lidx, &item);
}

static void
ilistSetDataItem (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx,
    listitem_t *item)
{
  nlist_t       *datalist = NULL;
  char          tbuff [40];

  if (list == NULL) {
    return;
  }

  datalist = nlistGetList (list, ikey);
  if (datalist == NULL) {
    snprintf (tbuff, sizeof (tbuff), "%s-item-%zd", list->name, ikey);
    datalist = nlistAlloc (tbuff, LIST_ORDERED, NULL);
    nlistSetList (list, ikey, datalist);
  }
  if (datalist != NULL) {
    listSet (datalist, item);
  }
  return;
}


