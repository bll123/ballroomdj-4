#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "list.h"
#include "log.h"
#include "bdjstring.h"

static void     listSetKey (list_t *list, listkey_t *key, char *keydata);
static listidx_t  listSet (list_t *list, listitem_t *item);
static listidx_t  listGetIdx (list_t *list, listkey_t *key);
static void     listFreeItem (list_t *, ssize_t);
static void     listInsert (list_t *, ssize_t loc, listitem_t *item);
static void     listReplace (list_t *, ssize_t, listitem_t *item);
static int      listBinarySearch (const list_t *, listkey_t *key, ssize_t *);
static int      idxCompare (listidx_t, listidx_t);
static int      listCompare (const list_t *, listkey_t *a, listkey_t *b);
static void     merge (list_t *, ssize_t, ssize_t, ssize_t);
static void     mergeSort (list_t *, ssize_t, ssize_t);

list_t *
listAlloc (char *name, listorder_t ordered, listCompare_t compare,
    listFree_t keyFreeHook, listFree_t valueFreeHook)
{
  list_t    *list;

  logProcBegin (LOG_PROC, "listAlloc");
  list = malloc (sizeof (list_t));
  assert (list != NULL);
    /* always allocate the name so that dynamic names can be created */
  list->name = strdup (name);
  assert (list->name != NULL);
  list->data = NULL;
  list->count = 0;
  list->allocCount = 0;
  list->keytype = KEY_STR;
  list->ordered = ordered;
  list->bumper1 = 0x11223344;
  list->bumper2 = 0x44332211;
  list->compare = compare;
  list->keyCache.strkey = NULL;
  list->locCache = LIST_LOC_INVALID;
  list->readCacheHits = 0;
  list->writeCacheHits = 0;
  list->keyFreeHook = keyFreeHook;
  list->valueFreeHook = valueFreeHook;
  logMsg (LOG_DBG, LOG_LIST, "alloc %s", name);
  logProcEnd (LOG_PROC, "listAlloc", "");
  return list;
}

void
listFree (void *tlist)
{
  list_t *list = (list_t *) tlist;

  if (list != NULL) {
    if (list->readCacheHits > 0 || list->writeCacheHits > 0) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "list %s: cache read:%ld write:%ld",
          list->name, list->readCacheHits, list->writeCacheHits);
    }
    if (list->data != NULL) {
      for (ssize_t i = 0; i < list->count; ++i) {
        listFreeItem (list, i);
      }
      free (list->data);
      list->data = NULL;
    } /* data is not null */
    list->count = 0;
    list->allocCount = 0;
    if (list->keytype == KEY_STR &&
        list->keyCache.strkey != NULL) {
      free (list->keyCache.strkey);
      list->keyCache.strkey = NULL;
      list->locCache = LIST_LOC_INVALID;
    }
    if (list->name != NULL) {
      free (list->name);
      list->name = NULL;
    }
    free (list);
  }
}

inline ssize_t
listGetCount (list_t *list)
{
  if (list == NULL) {
    return 0;
  }
  return list->count;
}

void
listSetSize (list_t *list, ssize_t siz)
{
  if (list == NULL) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "listsetsize: null list");
    return;
  }

  if (siz > list->allocCount) {
    list->allocCount = siz;
    list->data = realloc (list->data, list->allocCount * sizeof (listitem_t));
    assert (list->data != NULL);
    list->allocCount = siz;
  }
}

listidx_t
listSetData (list_t *list, char *keydata, void *data)
{
  listitem_t    item;

  listSetKey (list, &item.key, keydata);
  item.valuetype = VALUE_DATA;
  item.value.data = data;
  return listSet (list, &item);
}

listidx_t
listSetNum (list_t *list, char *keydata, ssize_t lval)
{
  listitem_t    item;

  listSetKey (list, &item.key, keydata);
  item.valuetype = VALUE_NUM;
  item.value.num = lval;
  return listSet (list, &item);
}

listidx_t
listSetDouble (list_t *list, char *keydata, double dval)
{
  listitem_t    item;

  listSetKey (list, &item.key, keydata);
  item.valuetype = VALUE_DOUBLE;
  item.value.dval = dval;
  return listSet (list, &item);
}

listidx_t
listSetList (list_t *list, char *keydata, list_t *data)
{
  listitem_t    item;

  listSetKey (list, &item.key, keydata);
  item.valuetype = VALUE_LIST;
  item.value.data = data;
  return listSet (list, &item);
}

void *
listGetData (list_t *list, char *keydata)
{
  void            *value = NULL;
  listkey_t       key;
  listidx_t            idx;

  key.strkey = keydata;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.data;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:%s key:%s idx:%ld", list->name, keydata, idx);
  return value;
}

void *
listGetDataByIdx (list_t *list, listidx_t idx)
{
  void    *value;


  if (list == NULL) {
    return NULL;
  }
  if (idx >= list->count) {
    return NULL;
  }
  value = list->data [idx].value.data;
  logMsg (LOG_DBG, LOG_LIST, "list:%s idx:%ld", list->name, idx);
  return value;
}

ssize_t
listGetNumByIdx (list_t *list, listidx_t idx)
{
  ssize_t     value;


  if (list == NULL) {
    return -1;
  }
  if (idx >= list->count) {
    return -1;
  }
  value = list->data [idx].value.num;
  logMsg (LOG_DBG, LOG_LIST, "list:%s idx:%ld", list->name, idx);
  return value;
}

ssize_t
listGetNum (list_t *list, char *keydata)
{
  ssize_t            value = -1L;
  listkey_t       key;
  listidx_t            idx;

  key.strkey = keydata;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.num;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:%s key:%s idx:%ld value:%ld", list->name, keydata, idx, value);
  return value;
}

double
listGetDouble (list_t *list, char *keydata)
{
  double          value = 0.0;
  listkey_t       key;
  listidx_t            idx;

  key.strkey = keydata;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.dval;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:%s key:%s idx:%ld value:%8.2g", list->name, keydata, idx, value);
  return value;
}

list_t *
listGetList (list_t *list, char *keydata)
{
  return listGetData (list, keydata);
}

listidx_t
listGetStrIdx (list_t *list, char *keydata)
{
  listkey_t         key;

  key.strkey = keydata;
  return listGetIdx (list, &key);
}

void
listSort (list_t *list)
{
  list->ordered = LIST_ORDERED;
  mergeSort (list, 0, list->count - 1);
}

inline void
listStartIterator (list_t *list)
{
  list->iteratorIndex = 0;
  list->currentIndex = 0;
}

void
listDumpInfo (list_t *list)
{
  logMsg (LOG_DBG, LOG_BASIC, "list: %s count: %ld key:%d ordered:%d",
      list->name, list->count, list->keytype, list->ordered);
}

void *
listIterateKeyStr (list_t *list)
{
  void      *value = NULL;

  logProcBegin (LOG_PROC, "listIterateKeyStr");
  if (list == NULL) {
    logProcEnd (LOG_PROC, "listIterateKeyStr", "null-list");
    return NULL;
  }

  if (list->iteratorIndex >= list->count) {
    list->iteratorIndex = 0;
    logProcEnd (LOG_PROC, "listIterateKeyStr", "end-list");
    return NULL;  /* indicate the end of the list */
  }

  value = list->data [list->iteratorIndex].key.strkey;
  list->currentIndex = list->iteratorIndex;

  if (list->keytype == KEY_STR &&
      list->keyCache.strkey != NULL) {
    free (list->keyCache.strkey);
    list->keyCache.strkey = NULL;
    list->locCache = LIST_LOC_INVALID;
  }

  list->keyCache.strkey = strdup (value);
  assert (list->keyCache.strkey != NULL);
  list->locCache = list->iteratorIndex;

  ++list->iteratorIndex;
  logProcEnd (LOG_PROC, "listIterateKeyStr", "");
  return value;
}

void *
listIterateValue (list_t *list)
{
  void      *value = NULL;

  logProcBegin (LOG_PROC, "listIterateValue");
  if (list == NULL) {
    logProcEnd (LOG_PROC, "listIterateValue", "null-list");
    return NULL;
  }

  if (list->iteratorIndex >= list->count) {
    list->iteratorIndex = 0;
    logProcEnd (LOG_PROC, "listIterateValue", "end-list");
    return NULL;  /* indicate the end of the list */
  }

  value = list->data [list->iteratorIndex].value.data;
  list->currentIndex = list->iteratorIndex;
  ++list->iteratorIndex;
  logProcEnd (LOG_PROC, "listIterateValue", "");
  return value;
}

ssize_t
listIterateNum (list_t *list)
{
  ssize_t     value = -1;

  logProcBegin (LOG_PROC, "listIterateNum");
  if (list == NULL) {
    logProcEnd (LOG_PROC, "listIterateNum", "null-list");
    return -1;
  }

  if (list->iteratorIndex >= list->count) {
    list->iteratorIndex = 0;
    logProcEnd (LOG_PROC, "listIterateNum", "end-list");
    return -1;  /* indicate the end of the list */
  }

  value = list->data [list->iteratorIndex].value.num;
  list->currentIndex = list->iteratorIndex;
  ++list->iteratorIndex;
  logProcEnd (LOG_PROC, "listIterateNum", "");
  return value;
}

listidx_t
listIterateGetIdx (list_t *list)
{
  if (list == NULL) {
    return -1;
  }

  return list->currentIndex;
}

/* key/value list, keyed by a listidx_t */

list_t *
llistAlloc (char *name, listorder_t ordered, listFree_t valueFreeHook)
{
  list_t    *list;

  list = listAlloc (name, ordered, NULL, NULL, valueFreeHook);
  list->keytype = KEY_NUM;
  return list;
}

inline void
llistFree (void *list)
{
  listFree (list);
}

inline ssize_t
llistGetCount (list_t *list)
{
  if (list == NULL) {
    return 0;
  }
  return list->count;
}

inline void
llistSetSize (list_t *list, ssize_t siz)
{
  listSetSize (list, siz);
}

void
llistSetFreeHook (list_t *list, listFree_t valueFreeHook)
{
  if (list == NULL) {
    return;
  }

  list->valueFreeHook = valueFreeHook;
}

listidx_t
llistSetData (list_t *list, listidx_t lidx, void *data)
{
  listitem_t    item;

  item.key.idx = lidx;
  item.valuetype = VALUE_DATA;
  item.value.data = data;
  return listSet (list, &item);
}

listidx_t
llistSetNum (list_t *list, listidx_t lidx, ssize_t data)
{
  listitem_t    item;

  item.key.idx = lidx;
  item.valuetype = VALUE_NUM;
  item.value.num = data;
  return listSet (list, &item);
}

listidx_t
llistSetDouble (list_t *list, listidx_t lidx, double data)
{
  listitem_t    item;

  item.key.idx = lidx;
  item.valuetype = VALUE_DOUBLE;
  item.value.dval = data;
  return listSet (list, &item);
}

listidx_t
llistSetList (list_t *list, listidx_t lidx, list_t *data)
{
  listitem_t    item;

  item.key.idx = lidx;
  item.valuetype = VALUE_LIST;
  item.value.data = data;
  return listSet (list, &item);
}

void *
llistGetData (list_t *list, listidx_t lidx)
{
  void            *value = NULL;
  listkey_t       key;
  listidx_t       idx;

  key.idx = lidx;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = (char *) list->data [idx].value.data;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:%s key:%ld idx:%ld", list->name, lidx, idx);
  return value;
}

inline void *
llistGetDataByIdx (list_t *list, listidx_t lidx)
{
  return listGetDataByIdx (list, lidx);
}

inline ssize_t
llistGetNumByIdx (list_t *list, listidx_t lidx)
{
  return listGetNumByIdx (list, lidx);
}

ssize_t
llistGetNum (list_t *list, listidx_t lidx)
{
  ssize_t            value = -1;
  listkey_t       key;
  listidx_t       idx;

  key.idx = lidx;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.num;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:%s key:%ld idx:%ld value:%ld", list->name, lidx, idx, value);
  return value;
}

double
llistGetDouble (list_t *list, listidx_t lidx)
{
  double          value = -1.0;
  listkey_t       key;
  listidx_t            idx;

  key.idx = lidx;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.dval;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:%s key:%ld idx:%ld value:%8.2g", list->name, lidx, idx, value);
  return value;
}

list_t *
llistGetList (list_t *list, listidx_t lidx)
{
  return llistGetData (list, lidx);
}

inline void
llistSort (list_t *list)
{
  listSort (list);
}

inline void
llistStartIterator (list_t *list)
{
  list->iteratorIndex = 0;
}

listidx_t
llistIterateKeyNum (list_t *list)
{
  ssize_t      value = -1L;

  logProcBegin (LOG_PROC, "llistIterateKeyNum");
  if (list == NULL || list->keytype == KEY_STR) {
    logProcEnd (LOG_PROC, "llistIterateKeyNum", "null-list/key-str");
    return -1L;
  }

  if (list->iteratorIndex >= list->count) {
    list->iteratorIndex = 0;
    logProcEnd (LOG_PROC, "llistIterateKeyNum", "end-list");
    return -1L;      /* indicate the end of the list */
  }

  value = list->data [list->iteratorIndex].key.idx;

  list->keyCache.idx = value;
  list->locCache = list->iteratorIndex;

  ++list->iteratorIndex;
  logProcEnd (LOG_PROC, "llistIterateKeyNum", "");
  return value;
}

inline void *
llistIterateValue (list_t *list)
{
  return listIterateValue (list);
}

inline ssize_t
llistIterateNum (list_t *list)
{
  return listIterateNum (list);
}

void
llistDumpInfo (list_t *list)
{
  listDumpInfo (list);
}

/* indirect routines */

listidx_t
ilistSetData (list_t *list, listidx_t ikey, listidx_t lidx, void *data)
{
  listitem_t    item;
  list_t        *ilist;

  if (list == NULL) {
    return -1;
  }

  ilist = llistGetList (list, ikey);
  if (ilist != NULL) {
    item.key.idx = lidx;
    item.valuetype = VALUE_DATA;
    item.value.data = data;
    return listSet (ilist, &item);
  }
  return -1;
}

listidx_t
ilistSetNum (list_t *list, listidx_t ikey, listidx_t lidx, ssize_t data)
{
  listitem_t    item;
  list_t        *ilist;

  if (list == NULL) {
    return -1;
  }

  ilist = llistGetList (list, ikey);
  if (ilist != NULL) {
    item.key.idx = lidx;
    item.valuetype = VALUE_NUM;
    item.value.num = data;
    return listSet (list, &item);
  }
  return -1;
}

listidx_t
ilistSetDouble (list_t *list, listidx_t ikey, listidx_t lidx, double data)
{
  listitem_t    item;
  list_t        *ilist;

  if (list == NULL) {
    return -1;
  }

  ilist = llistGetList (list, ikey);
  if (ilist != NULL) {
    item.key.idx = lidx;
    item.valuetype = VALUE_DOUBLE;
    item.value.dval = data;
    return listSet (list, &item);
  }
  return -1;
}

void *
ilistGetData (list_t *list, listidx_t ikey, listidx_t lidx)
{
  void            *value = NULL;
  listkey_t       key;
  listidx_t       idx = 0;
  list_t          *ilist = NULL;

  if (list == NULL) {
    return NULL;
  }

  ilist = llistGetList (list, ikey);
  if (ilist != NULL) {
    key.idx = lidx;
    idx = listGetIdx (ilist, &key);
    if (idx >= 0) {
      value = (char *) ilist->data [idx].value.data;
    }
    logMsg (LOG_DBG, LOG_LIST, "list:%s key:%ld idx:%ld", ilist->name, lidx, idx);
  }
  return value;
}

ssize_t
ilistGetNum (list_t *list, listidx_t ikey, listidx_t lidx)
{
  ssize_t            value = -1L;
  listkey_t       key;
  listidx_t            idx;
  list_t          *ilist;

  if (list == NULL) {
    return -1L;
  }

  ilist = llistGetList (list, ikey);
  if (ilist != NULL) {
    key.idx = lidx;
    idx = listGetIdx (ilist, &key);
    if (idx >= 0) {
      value = ilist->data [idx].value.num;
    }
    logMsg (LOG_DBG, LOG_LIST, "list:%s key:%ld idx:%ld value:%ld", list->name, lidx, idx, value);
  }
  return value;
}

double
ilistGetDouble (list_t *list, listidx_t ikey, listidx_t lidx)
{
  double          value = -1.0;
  listkey_t       key;
  listidx_t            idx;
  list_t          *ilist;

  if (list == NULL) {
    return -1.0;
  }

  ilist = llistGetList (list, ikey);
  if (ilist != NULL) {
    key.idx = lidx;
    idx = listGetIdx (ilist, &key);
    if (idx >= 0) {
      value = ilist->data [idx].value.dval;
    }
    logMsg (LOG_DBG, LOG_LIST, "list:%s key:%ld idx:%ld value:%8.2g", list->name, lidx, idx, value);
  }
  return value;
}

/* internal routines */

static void
listSetKey (list_t *list, listkey_t *key, char *keydata)
{
  if (list->keyFreeHook != NULL) {
    key->strkey = strdup (keydata);
  } else {
    key->strkey = keydata;
  }
}

static listidx_t
listSet (list_t *list, listitem_t *item)
{
  listidx_t       loc = 0L;
  int             rc = -1;
  int             found = 0;

  if (list->locCache >= 0L) {
    if ((list->keytype == KEY_STR &&
         strcmp (item->key.strkey, list->keyCache.strkey) == 0) ||
        (list->keytype == KEY_NUM &&
         item->key.idx == list->keyCache.idx)) {
      loc = list->locCache;
      ++list->writeCacheHits;
      found = 1;
    }
  }

  if (! found && list->count > 0) {
    if (list->ordered == LIST_ORDERED) {
      rc = listBinarySearch (list, &item->key, &loc);
    } else {
      loc = list->count;
    }
  }

  if (list->ordered == LIST_ORDERED) {
    if (rc < 0) {
      listInsert (list, loc, item);
    } else {
      listReplace (list, loc, item);
    }
  } else {
    listInsert (list, loc, item);
  }
  return loc;
}

static listidx_t
listGetIdx (list_t *list, listkey_t *key)
{
  listidx_t   idx;
  listidx_t   ridx;
  int         rc;
  char        *str;

    /* check the cache */
  if (list->locCache >= 0L) {
    if ((list->keytype == KEY_STR &&
         strcmp (key->strkey, list->keyCache.strkey) == 0) ||
        (list->keytype == KEY_NUM &&
         key->idx == list->keyCache.idx)) {
      ridx = list->locCache;
      ++list->readCacheHits;
      return ridx;
    }
  }

  ridx = -1;
  if (list->ordered == LIST_ORDERED) {
    rc = listBinarySearch (list, key, &idx);
    if (rc == 0) {
      ridx = idx;
    }
  } else {
    listStartIterator (list);
    while ((str = listIterateKeyStr (list)) != NULL) {
      if (list->compare (str, key->strkey) == 0) {
        ridx = list->currentIndex;
        break;
      }
    }
  }

  if (list->keytype == KEY_STR &&
      list->keyCache.strkey != NULL) {
    free (list->keyCache.strkey);
    list->keyCache.strkey = NULL;
    list->locCache = LIST_LOC_INVALID;
  }

  if (ridx >= 0) {
    if (list->keytype == KEY_STR) {
      list->keyCache.strkey = strdup (key->strkey);
    }
    if (list->keytype == KEY_NUM) {
      list->keyCache.idx = key->idx;
    }
    list->locCache = ridx;
  }
  return ridx;
}

static void
listFreeItem (list_t *list, ssize_t idx)
{
  listitem_t    *dp;

  if (list == NULL) {
    return;
  }
  if (list->data == NULL) {
    return;
  }
  if (idx >= list->count) {
    return;
  }

  dp = &list->data [idx];

  if (dp != NULL) {
    if (list->keytype == KEY_STR &&
        dp->key.strkey != NULL &&
        list->keyFreeHook != NULL) {
      list->keyFreeHook (dp->key.strkey);
    }
    if (dp->valuetype == VALUE_DATA &&
        dp->value.data != NULL &&
        list->valueFreeHook != NULL) {
      list->valueFreeHook (dp->value.data);
    }
    if (dp->valuetype == VALUE_LIST &&
        dp->value.data != NULL) {
      listFree (dp->value.data);
    }
  } /* if the data pointer is not null */
}

static void
listInsert (list_t *list, ssize_t loc, listitem_t *item)
{
  ssize_t      copycount;

  ++list->count;
  if (list->count > list->allocCount) {
    ++list->allocCount;
    list->data = realloc (list->data, list->allocCount * sizeof (listitem_t));
    assert (list->data != NULL);
  }

  assert (list->data != NULL);
  assert ((list->count > 0 && loc < list->count) ||
          (list->count == 0 && loc == 0));

  copycount = list->count - (loc + 1);
  if (copycount > 0) {
    memcpy (list->data + loc + 1, list->data + loc, copycount * sizeof (listitem_t));
  }
  memcpy (&list->data [loc], item, sizeof (listitem_t));
  assert (list->bumper1 == 0x11223344);
  assert (list->bumper2 == 0x44332211);
}

static void
listReplace (list_t *list, ssize_t loc, listitem_t *item)
{
  assert (list->data != NULL);
  assert ((list->count > 0 && loc < list->count) ||
          (list->count == 0 && loc == 0));

  listFreeItem (list, loc);
  memcpy (&list->data [loc], item, sizeof (listitem_t));
  assert (list->bumper1 == 0x11223344);
  assert (list->bumper2 == 0x44332211);
}

static int
idxCompare (listidx_t la, listidx_t lb)
{
  int rc = 0;

  if (la < lb) {
    rc = -1;
  } else if (la > lb) {
    rc = 1;
  }
  return rc;
}

static int
listCompare (const list_t *list, listkey_t *a, listkey_t *b)
{
  int         rc;

  rc = 0;
  if (list->ordered == LIST_UNORDERED) {
    rc = 0;
  } else {
    if (list->compare != NULL) {
      rc = list->compare (a->strkey, b->strkey);
    } else {
      if (list->keytype == KEY_STR) {
        rc = stringCompare (a->strkey, b->strkey);
      }
      if (list->keytype == KEY_NUM) {
        rc = idxCompare (a->idx, b->idx);
      }
    }
  }
  return rc;
}

/* returns the location after as a negative number if not found */
static int
listBinarySearch (const list_t *list, listkey_t *key, ssize_t *loc)
{
  listidx_t     l = 0;
  listidx_t     r = list->count - 1;
  listidx_t     m = 0;
  listidx_t     rm;
  int           rc;

  rm = 0;
  while (l <= r) {
    m = l + (r - l) / 2;

    rc = listCompare (list, &list->data [m].key, key);
    if (rc == 0) {
      *loc = (size_t) m;
      return 0;
    }

    if (rc < 0) {
      l = m + 1;
      rm = l;
    } else {
      r = m - 1;
      rm = m;
    }
  }

  *loc = (size_t) rm;
  return -1;
}

/*
 * in-place merge sort from:
 * https://www.geeksforgeeks.org/in-place-merge-sort/
 */

static void
merge (list_t *list, ssize_t start, ssize_t mid, ssize_t end)
{
  ssize_t      start2 = mid + 1;
  listitem_t  value;

  int rc = listCompare (list, &list->data [mid].key, &list->data [start2].key);
  if (rc <= 0) {
    return;
  }

  while (start <= mid && start2 <= end) {
    rc = listCompare (list, &list->data [start].key, &list->data [start2].key);
    if (rc <= 0) {
      start++;
    } else {
      value = list->data [start2];
      ssize_t index = start2;

      while (index != start) {
        list->data [index] = list->data [index - 1];
        index--;
      }
      list->data [start] = value;

      start++;
      mid++;
      start2++;
    }
  }
}

static void
mergeSort (list_t *list, ssize_t l, ssize_t r)
{
  if (list->count > 0 && l < r) {
    ssize_t m = l + (r - l) / 2;
    mergeSort (list, l, m);
    mergeSort (list, m + 1, r);
    merge (list, l, m, r);
  }
}

