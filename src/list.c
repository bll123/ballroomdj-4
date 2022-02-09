#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "list.h"
#include "log.h"
#include "bdjstring.h"
#include "tmutil.h"


static void     listFreeItem (list_t *, ssize_t);
static void     listInsert (list_t *, ssize_t loc, listitem_t *item);
static void     listReplace (list_t *, ssize_t, listitem_t *item);
static int      listBinarySearch (const list_t *, listkey_t *key, ssize_t *);
static int      idxCompare (listidx_t, listidx_t);
static int      listCompare (const list_t *, listkey_t *a, listkey_t *b);
static void     merge (list_t *, ssize_t, ssize_t, ssize_t);
static void     mergeSort (list_t *, ssize_t, ssize_t);

list_t *
listAlloc (char *name, listorder_t ordered,
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
  list->version = 1;
  list->count = 0;
  list->allocCount = 0;
  list->replace = false;
  list->keytype = LIST_KEY_STR;
  list->ordered = ordered;
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
    if (list->keytype == LIST_KEY_STR &&
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

inline void
listSetVersion (list_t *list, ssize_t version)
{
  if (list == NULL) {
    return;
  }
  list->version = version;
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
    list->data = realloc (list->data,
        (size_t) list->allocCount * sizeof (listitem_t));
    assert (list->data != NULL);
    list->allocCount = siz;
  }
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
    return LIST_VALUE_INVALID;
  }
  if (idx >= list->count) {
    return LIST_VALUE_INVALID;
  }
  value = list->data [idx].value.num;
  logMsg (LOG_DBG, LOG_LIST, "list:%s idx:%ld", list->name, idx);
  return value;
}

ssize_t
listGetNum (list_t *list, char *keydata)
{
  ssize_t     value = LIST_VALUE_INVALID;
  listkey_t   key;
  listidx_t   idx;

  key.strkey = keydata;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.num;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:%s key:%s idx:%ld value:%ld", list->name, keydata, idx, value);
  return value;
}

void
listSort (list_t *list)
{
  mstime_t      tm;
  time_t        elapsed;

  mstimestart (&tm);
  list->ordered = LIST_ORDERED;
  mergeSort (list, 0, list->count - 1);
  elapsed = mstimeend (&tm);
  if (elapsed > 0) {
    logMsg (LOG_DBG, LOG_BASIC, "sort of %s took %ld ms", list->name, elapsed);
  }
}

inline void
listStartIterator (list_t *list, listidx_t *iteridx)
{
  *iteridx = -1;
}

void
listDumpInfo (list_t *list)
{
  logMsg (LOG_DBG, LOG_BASIC, "list: %s count: %ld key:%d ordered:%d",
      list->name, list->count, list->keytype, list->ordered);
}

void *
listIterateValue (list_t *list, listidx_t *iteridx)
{
  void      *value = NULL;

  logProcBegin (LOG_PROC, "listIterateValue");
  if (list == NULL) {
    logProcEnd (LOG_PROC, "listIterateValue", "null-list");
    return NULL;
  }

  ++(*iteridx);
  if (*iteridx >= list->count) {
    *iteridx = -1;
    logProcEnd (LOG_PROC, "listIterateValue", "end-list");
    return NULL;  /* indicate the end of the list */
  }

  value = list->data [*iteridx].value.data;
  logProcEnd (LOG_PROC, "listIterateValue", "");
  return value;
}

ssize_t
listIterateNum (list_t *list, listidx_t *iteridx)
{
  ssize_t     value = LIST_VALUE_INVALID;

  logProcBegin (LOG_PROC, "listIterateNum");
  if (list == NULL) {
    logProcEnd (LOG_PROC, "listIterateNum", "null-list");
    return LIST_VALUE_INVALID;
  }

  ++(*iteridx);
  if (*iteridx >= list->count) {
    *iteridx = -1;
    logProcEnd (LOG_PROC, "listIterateNum", "end-list");
    return LIST_VALUE_INVALID;  /* indicate the end of the list */
  }

  value = list->data [*iteridx].value.num;
  logProcEnd (LOG_PROC, "listIterateNum", "");
  return value;
}

listidx_t
listIterateGetIdx (list_t *list, listidx_t *iteridx)
{
  if (list == NULL) {
    return LIST_LOC_INVALID;
  }

  return *iteridx;
}

listidx_t
listGetIdx (list_t *list, listkey_t *key)
{
  listidx_t   idx;
  listidx_t   ridx;
  int         rc;

    /* check the cache */
  if (list->locCache >= 0L) {
    if ((list->keytype == LIST_KEY_STR &&
         strcmp (key->strkey, list->keyCache.strkey) == 0) ||
        (list->keytype == LIST_KEY_NUM &&
         key->idx == list->keyCache.idx)) {
      ridx = list->locCache;
      ++list->readCacheHits;
      return ridx;
    }
  }

  ridx = LIST_LOC_INVALID;
  if (list->ordered == LIST_ORDERED) {
    rc = listBinarySearch (list, key, &idx);
    if (rc == 0) {
      ridx = idx;
    }
  } else if (list->replace) {
    for (ssize_t i = 0; i < list->count; ++i) {
      if (list->keytype == LIST_KEY_STR) {
        if (strcmp (list->data [i].key.strkey, key->strkey) == 0) {
          ridx = i;
          break;
        }
      } else {
        if (list->data [i].key.idx == key->idx) {
          ridx = i;
          break;
        }
      }
    }
  }

  if (list->keytype == LIST_KEY_STR &&
      list->keyCache.strkey != NULL) {
    free (list->keyCache.strkey);
    list->keyCache.strkey = NULL;
    list->locCache = LIST_LOC_INVALID;
  }

  if (ridx >= 0) {
    if (list->keytype == LIST_KEY_STR) {
      list->keyCache.strkey = strdup (key->strkey);
    }
    if (list->keytype == LIST_KEY_NUM) {
      list->keyCache.idx = key->idx;
    }
    list->locCache = ridx;
  }
  return ridx;
}

void
listSet (list_t *list, listitem_t *item)
{
  listidx_t       loc = 0L;
  int             rc = -1;
  int             found = 0;

  if (list->locCache >= 0L) {
    if ((list->keytype == LIST_KEY_STR &&
         strcmp (item->key.strkey, list->keyCache.strkey) == 0) ||
        (list->keytype == LIST_KEY_NUM &&
         item->key.idx == list->keyCache.idx)) {
      loc = list->locCache;
      ++list->writeCacheHits;
      found = 1;
      rc = 0;
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
  return;
}

/* internal routines */

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
    if (list->keytype == LIST_KEY_STR &&
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
    list->data = realloc (list->data,
        (size_t) list->allocCount * sizeof (listitem_t));
    assert (list->data != NULL);
  }

  assert (list->data != NULL);
  assert ((list->count > 0 && loc < list->count) ||
          (list->count == 0 && loc == 0));

  copycount = list->count - (loc + 1);
  if (copycount > 0) {
    memcpy (list->data + loc + 1, list->data + loc,
        (size_t) copycount * sizeof (listitem_t));
  }
  memcpy (&list->data [loc], item, sizeof (listitem_t));
}

static void
listReplace (list_t *list, ssize_t loc, listitem_t *item)
{
  assert (list->data != NULL);
  assert ((list->count > 0 && loc < list->count) ||
          (list->count == 0 && loc == 0));

  listFreeItem (list, loc);
  memcpy (&list->data [loc], item, sizeof (listitem_t));
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
    if (list->keytype == LIST_KEY_STR) {
      rc = istringCompare (a->strkey, b->strkey);
    }
    if (list->keytype == LIST_KEY_NUM) {
      rc = idxCompare (a->idx, b->idx);
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
      *loc = (ssize_t) m;
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

  *loc = (ssize_t) rm;
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
      ssize_t       index;


      value = list->data [start2];
      index = start2;

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

