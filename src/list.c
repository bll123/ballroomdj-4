#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "list.h"
#include "log.h"
#include "bdjstring.h"

static void     listSetKey (list_t *list, listkey_t *key, char *keydata);
static void     listSet (list_t *list, listitem_t *item);
static long     listGetIdx (list_t *list, listkey_t *key);
static void     listFreeItem (list_t *, size_t);
static void     listInsert (list_t *, size_t loc, listitem_t *item);
static void     listReplace (list_t *, size_t, listitem_t *item);
static int      listBinarySearch (const list_t *, listkey_t *key, size_t *);
static int      longCompare (long, long);
static int      listCompare (const list_t *, listkey_t *a, listkey_t *b);
static void     merge (list_t *, size_t, size_t, size_t);
static void     mergeSort (list_t *, size_t, size_t);

list_t *
listAlloc (char *name, listorder_t ordered, listCompare_t compare,
    listFree_t keyFreeHook, listFree_t valueFreeHook)
{
  list_t    *list;

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
  list->cacheHits = 0;
  list->keyFreeHook = keyFreeHook;
  list->valueFreeHook = valueFreeHook;
  return list;
}

void
listFree (void *tlist)
{
  list_t *list = (list_t *) tlist;

  if (list != NULL) {
    if (list->name != NULL) {
      free (list->name);
      list->name = NULL;
    }
    if (list->data != NULL) {
      for (size_t i = 0; i < list->count; ++i) {
        listFreeItem (list, i);
      } /* for each list item */
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
    if (list->cacheHits > 0) {
      logMsg (LOG_DBG, LOG_LVL_1, "list %s: %ld cache hits", list->name, list->cacheHits);
    }
    free (list);
  }
}

inline size_t
listGetSize (list_t *list)
{
  if (list == NULL) {
    return 0;
  }
  return list->count;
}

void
listSetSize (list_t *list, size_t siz)
{
  if (list == NULL) {
    logMsg (LOG_DBG, LOG_LVL_1, "listsetsize: null list");
    return;
  }

  if (siz > list->allocCount) {
    list->allocCount = siz;
    list->data = realloc (list->data, list->allocCount * sizeof (listitem_t));
    assert (list->data != NULL);
    list->allocCount = siz;
  }
}

void
listSetData (list_t *list, char *keydata, void *data)
{
  listitem_t    item;

  listSetKey (list, &item.key, keydata);
  item.valuetype = VALUE_DATA;
  item.value.data = data;
  listSet (list, &item);
}

void
listSetLong (list_t *list, char *keydata, long lval)
{
  listitem_t    item;

  listSetKey (list, &item.key, keydata);
  item.valuetype = VALUE_LONG;
  item.value.l = lval;
  listSet (list, &item);
}

void
listSetDouble (list_t *list, char *keydata, double dval)
{
  listitem_t    item;

  listSetKey (list, &item.key, keydata);
  item.valuetype = VALUE_DOUBLE;
  item.value.d = dval;
  listSet (list, &item);
}

void
listSetList (list_t *list, char *keydata, list_t *data)
{
  listitem_t    item;

  listSetKey (list, &item.key, keydata);
  item.valuetype = VALUE_LIST;
  item.value.data = data;
  listSet (list, &item);
}

void *
listGetData (list_t *list, char *keydata)
{
  void            *value = NULL;
  listkey_t       key;
  long            idx;

  key.strkey = keydata;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.data;
  }
  return value;
}

long
listGetLong (list_t *list, char *keydata)
{
  long            value = -1L;
  listkey_t       key;
  long            idx;

  key.strkey = keydata;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.l;
  }
  return value;
}

double
listGetDouble (list_t *list, char *keydata)
{
  double          value = 0.0;
  listkey_t       key;
  long            idx;

  key.strkey = keydata;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.d;
  }
  return value;
}

list_t *
listGetList (list_t *list, char *keydata)
{
  return listGetData (list, keydata);
}

long
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

void *
listIterateKeyStr (list_t *list)
{
  void      *value = NULL;

  logProcBegin (LOG_LVL_8, "listIterateKeyStr");
  if (list == NULL) {
    logProcEnd (LOG_LVL_8, "listIterateKeyStr", "null-list");
    return NULL;
  }

  if (list->iteratorIndex >= list->count) {
    list->iteratorIndex = 0;
    logProcEnd (LOG_LVL_8, "listIterateKeyStr", "end-list");
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
  list->locCache = (long) list->iteratorIndex;

  ++list->iteratorIndex;
  logProcEnd (LOG_LVL_8, "listIterateKeyStr", "");
  return value;
}

/* key/value list, keyed by a long */

list_t *
llistAlloc (char *name, listorder_t ordered, listFree_t valueFreeHook)
{
  list_t    *list;

  list = listAlloc (name, ordered, NULL, NULL, valueFreeHook);
  list->keytype = KEY_LONG;
  return list;
}

inline void
llistFree (void *list)
{
  listFree (list);
}

inline void
llistSetSize (list_t *list, size_t siz)
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

void
llistSetData (list_t *list, long lkey, void *data)
{
  listitem_t    item;

  item.key.lkey = lkey;
  item.valuetype = VALUE_DATA;
  item.value.data = data;
  listSet (list, &item);
}

void
llistSetLong (list_t *list, long lkey, long data)
{
  listitem_t    item;

  item.key.lkey = lkey;
  item.valuetype = VALUE_LONG;
  item.value.l = data;
  listSet (list, &item);
}

void
llistSetDouble (list_t *list, long lkey, double data)
{
  listitem_t    item;

  item.key.lkey = lkey;
  item.valuetype = VALUE_DOUBLE;
  item.value.d = data;
  listSet (list, &item);
}

void
llistSetList (list_t *list, long lkey, list_t *data)
{
  listitem_t    item;

  item.key.lkey = lkey;
  item.valuetype = VALUE_LIST;
  item.value.data = data;
  listSet (list, &item);
}

void *
llistGetData (list_t *list, long lkey)
{
  void            *value = NULL;
  listkey_t       key;
  long            idx;

  key.lkey = lkey;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = (char *) list->data [idx].value.data;
  }
  return value;
}

long
llistGetLong (list_t *list, long lkey)
{
  long            value = -1;
  listkey_t       key;
  long            idx;

  key.lkey = lkey;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.l;
  }
  return value;
}

double
llistGetDouble (list_t *list, long lkey)
{
  double          value = -1.0;
  listkey_t       key;
  long            idx;

  key.lkey = lkey;
  idx = listGetIdx (list, &key);
  if (idx >= 0) {
    value = list->data [idx].value.d;
  }
  return value;
}

list_t *
llistGetList (list_t *list, long lkey)
{
  return llistGetData (list, lkey);
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

long
llistIterateKeyLong (list_t *list)
{
  long      value = -1L;

  logProcBegin (LOG_LVL_8, "llistIterateKeyLong");
  if (list == NULL || list->keytype == KEY_STR) {
    logProcEnd (LOG_LVL_8, "llistIterateKeyLong", "null-list/key-str");
    return -1L;
  }

  if (list->iteratorIndex >= list->count) {
    list->iteratorIndex = 0;
    logProcEnd (LOG_LVL_8, "llistIterateKeyLong", "end-list");
    return -1L;      /* indicate the end of the list */
  }

  value = list->data [list->iteratorIndex].key.lkey;

  list->keyCache.lkey = value;
  list->locCache = (long) list->iteratorIndex;

  ++list->iteratorIndex;
  logProcEnd (LOG_LVL_8, "llistIterateKeyLong", "");
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

static void
listSet (list_t *list, listitem_t *item)
{
  size_t          loc;
  int             rc;

  loc = 0L;
  rc = -1;

  if (list->count > 0) {
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

static long
listGetIdx (list_t *list, listkey_t *key)
{
  size_t      idx;
  long        ridx;
  int         rc;
  char        *str;

    /* check the cache */
  if (list->locCache >= 0L) {
    if ((list->keytype == KEY_STR &&
         strcmp (key->strkey, list->keyCache.strkey) == 0) ||
        (list->keytype == KEY_LONG &&
         key->lkey == list->keyCache.lkey)) {
      ridx = list->locCache;
      ++list->cacheHits;
      return ridx;
    }
  }

  ridx = -1;
  if (list->ordered == LIST_ORDERED) {
    rc = listBinarySearch (list, key, &idx);
    if (rc == 0) {
      ridx = (long) idx;
    }
  } else {
    listStartIterator (list);
    while ((str = listIterateKeyStr (list)) != NULL) {
      if (list->compare (str, key->strkey) == 0) {
        ridx = (long) list->currentIndex;
        break;
      }
    }
  }

  if (ridx >= 0) {
    if (list->keytype == KEY_STR) {
      list->keyCache.strkey = strdup (key->strkey);
    }
    if (list->keytype == KEY_LONG) {
      list->keyCache.lkey = key->lkey;
    }
    list->locCache = ridx;
  }
  return ridx;
}

static void
listFreeItem (list_t *list, size_t idx)
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
listInsert (list_t *list, size_t loc, listitem_t *item)
{
  size_t      copycount;

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
listReplace (list_t *list, size_t loc, listitem_t *item)
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
longCompare (long la, long lb)
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
      if (list->keytype == KEY_LONG) {
        rc = longCompare (a->lkey, b->lkey);
      }
    }
  }
  return rc;
}

/* returns the location after as a negative number if not found */
static int
listBinarySearch (const list_t *list, listkey_t *key, size_t *loc)
{
  long      l = 0;
  long      r = (long) list->count - 1;
  long      m = 0;
  long      rm;
  int       rc;

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
merge (list_t *list, size_t start, size_t mid, size_t end)
{
  size_t      start2 = mid + 1;
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
      size_t index = start2;

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
mergeSort (list_t *list, size_t l, size_t r)
{
  if (list->count > 0 && l < r) {
    size_t m = l + (r - l) / 2;
    mergeSort (list, l, m);
    mergeSort (list, m + 1, r);
    merge (list, l, m, r);
  }
}
