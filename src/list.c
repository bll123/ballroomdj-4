#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "list.h"
#include "log.h"

typedef struct {
  listkey_t     lkey;
  valuetype_t   valuetype;
  union {
    void        *data;
    long        l;
    double      d;
  } u;
} namevalue_t;

static long           listLocate (list_t *list, listkey_t key);
static namevalue_t *  slistSetKey (list_t *list, char *key);
static namevalue_t *  slistGetNV (list_t *list, char *key);
static namevalue_t *  llistSetKey (long key);
static namevalue_t *  llistGetNV (list_t *list, long key);
static void           listFreeItem (list_t *, size_t);
static void           listInsert (list_t *, size_t, void *);
static void           listReplace (list_t *, size_t, void *);
static int            listBinarySearch (const list_t *, void *, size_t *);
static int            nameValueCompare (const list_t *, void *, void *);
static int            longCompare (long, long);
static int            listCompare (const list_t *, void *, void *);
static void           merge (list_t *, size_t, size_t, size_t);
static void           mergeSort (list_t *, size_t, size_t);

/* data size must be consistent   */

list_t *
listAlloc (char *name, size_t dsiz, listCompare_t compare, listFree_t freeHook)
{
  list_t    *list;

  list = malloc (sizeof (list_t));
  assert (list != NULL);
  list->name = name;
  list->data = NULL;
  list->count = 0;
  list->allocCount = 0;
  list->dsiz = dsiz;
  list->keytype = KEY_STR;
  list->ordered = LIST_UNORDERED;
  list->type = LIST_BASIC;
  list->bumper1 = 0x11223344;
  list->bumper2 = 0x44332211;
  list->compare = compare;
  list->freeHook = freeHook;
  list->freeHookB = NULL;
  list->cacheKeyType = KEY_STR;
  list->keyCache.strkey = NULL;
  list->locCache = LIST_LOC_INVALID;
  list->cacheHits = 0;
  return list;
}

void
listFree (void *tlist)
{
  list_t *list = (list_t *) tlist;

  if (list != NULL) {
    if (list->data != NULL) {
      for (size_t i = 0; i < list->count; ++i) {
        listFreeItem (list, i);
      } /* for each list item */
      free (list->data);
    } /* data is not null */
    list->count = 0;
    list->allocCount = 0;
    list->data = NULL;
    if (list->cacheKeyType == KEY_STR) {
      if (list->keyCache.strkey != NULL) {
        free (list->keyCache.strkey);
        list->keyCache.strkey = NULL;
      }
    }
    if (list->cacheHits > 0) {
      logMsg (LOG_DBG, LOG_LVL_1, "list %s: %ld cache hits", list->name, list->cacheHits);
    }
    free (list);
  }
}

void
listSetSize (list_t *list, size_t siz)
{
  list->allocCount = siz;
  list->data = realloc (list->data, list->allocCount * list->dsiz);
}

list_t *
listSet (list_t *list, void *data)
{
  size_t        loc;
  int           rc;

  loc = 0L;
  rc = -1;
  if (list->count > 0) {
    if (list->ordered == LIST_ORDERED) {
      rc = listBinarySearch (list, data, &loc);
    } else {
      loc = list->count;
    }
  }
  if (list->ordered == LIST_ORDERED) {
    if (rc < 0) {
      listInsert (list, loc, data);
    } else {
      listReplace (list, loc, data);
    }
  } else {
    listInsert (list, loc, data);
  }
  return list;
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
}

void *
listIterateData (list_t *list)
{
  void      *value = NULL;

  logProcBegin (LOG_LVL_6, "listIterateData");
  if (list == NULL) {
    logProcEnd (LOG_LVL_6, "listIterateData", "null-list");
    return NULL;
  }

  if (list->iteratorIndex >= list->count) {
    list->iteratorIndex = 0;
    logProcEnd (LOG_LVL_6, "listIterateData", "end-list");
    return NULL;  /* indicate the end of the list */
  }

  if (list->type == LIST_BASIC) {
    value = list->data [list->iteratorIndex];
  }
  ++list->iteratorIndex;

  logProcEnd (LOG_LVL_6, "listIterateData", "");
  return value;
}

/* key/value list, keyed by a string*/

list_t *
slistAlloc (char *name, listorder_t ordered, listCompare_t compare,
    listFree_t freeHook, listFree_t freeHookB)
{
  list_t    *list;

  list = listAlloc (name, sizeof (namevalue_t *), compare, NULL);
  list->ordered = ordered;
  list->type = LIST_NAMEVALUE;
  list->keytype = KEY_STR;
  list->freeHook = freeHook;
  list->freeHookB = freeHookB;
  return list;
}

inline void
slistFree (void *list)
{
  listFree (list);
}

inline void
slistSetSize (list_t *list, size_t siz)
{
  listSetSize (list, siz);
}

list_t *
slistSetData (list_t *list, char *key, void *value)
{
  namevalue_t *nv;

  nv = slistSetKey (list, key);
  nv->valuetype = VALUE_DATA;
  nv->u.data = value;
  listSet (list, nv);
  return list;
}

list_t *
slistSetLong (list_t *list, char *key, long value)
{
  namevalue_t *nv;

  nv = slistSetKey (list, key);
  nv->valuetype = VALUE_LONG;
  nv->u.l = value;
  listSet (list, nv);
  return list;
}

list_t *
slistSetDouble (list_t *list, char *key, double value)
{
  namevalue_t *nv;

  nv = slistSetKey (list, key);
  nv->valuetype = VALUE_DOUBLE;
  nv->u.d = value;
  listSet (list, nv);
  return list;
}

list_t *
slistSetList (list_t *list, char *key, list_t *value)
{
  namevalue_t *nv;

  nv = slistSetKey (list, key);
  nv->valuetype = VALUE_LIST;
  nv->u.data = value;
  listSet (list, nv);
  return list;
}

void *
slistGetData (list_t *list, char *key)
{
  void            *value = NULL;
  namevalue_t     *nv;

  nv = slistGetNV (list, key);
  if (nv != NULL &&
      (nv->valuetype == VALUE_DATA || nv->valuetype == VALUE_LIST)) {
    value = (char *) nv->u.data;
  }
  return value;
}

long
slistGetLong (list_t *list, char *key)
{
  long            value = -1L;
  namevalue_t     *nv;

  nv = slistGetNV (list, key);
  if (nv != NULL && nv->valuetype == VALUE_LONG) {
    value = nv->u.l;
  }
  return value;
}

double
slistGetDouble (list_t *list, char *key)
{
  double          value = 0.0;
  namevalue_t     *nv;

  nv = slistGetNV (list, key);
  if (nv != NULL && nv->valuetype == VALUE_LONG) {
    value = nv->u.d;
  }
  return value;
}

inline void
slistSort (list_t *list)
{
  listSort (list);
}

inline void
slistStartIterator (list_t *list)
{
  list->iteratorIndex = 0;
}

char *
slistIterateKeyStr (list_t *list)
{
  char      *value = NULL;

  logProcBegin (LOG_LVL_6, "slistIterateKeyStr");
  if (list == NULL || list->keytype == KEY_LONG) {
    logProcEnd (LOG_LVL_6, "slistIterateKeyStr", "null-list/key-long");
    return NULL;
  }

  if (list->iteratorIndex >= list->count) {
    list->iteratorIndex = 0;
    logProcEnd (LOG_LVL_6, "slistIterateKeyStr", "end-list");
    return NULL;      /* indicate the end of the list */
  }

  if (list->type == LIST_NAMEVALUE) {
    namevalue_t *nv = (namevalue_t *) list->data[list->iteratorIndex];
    value = nv->lkey.strkey;

    if (list->cacheKeyType == KEY_STR &&
        list->keyCache.strkey != NULL) {
      free (list->keyCache.strkey);
      list->keyCache.strkey = NULL;
    }

    list->cacheKeyType = KEY_STR;
    list->keyCache.strkey = strdup (value);
    list->locCache = list->iteratorIndex;
  }

  ++list->iteratorIndex;
  logProcEnd (LOG_LVL_6, "slistIterateKeyStr", "");
  return value;
}

/* key/value list, keyed by a long */

list_t *
llistAlloc (char *name, listorder_t ordered, listFree_t freeHookB)
{
  list_t    *list;

  list = listAlloc (name, sizeof (namevalue_t *), NULL, NULL);
  list->ordered = ordered;
  list->type = LIST_NAMEVALUE;
  list->keytype = KEY_LONG;
  list->freeHook = NULL;
  list->freeHookB = freeHookB;
  return list;
}

inline void
llistFree (void *list)
{
  listFree (list);
}

void
llistSetFreeHook (list_t *list, listFree_t freeHookB)
{
  list->freeHookB = freeHookB;
}

inline void
llistSetSize (list_t *list, size_t siz)
{
  listSetSize (list, siz);
}

list_t *
llistSetData (list_t *list, long key, void *value)
{
  namevalue_t *nv;

  nv = llistSetKey (key);
  nv->valuetype = VALUE_DATA;
  nv->u.data = value;
  listSet (list, nv);
  return list;
}

list_t *
llistSetLong (list_t *list, long key, long value)
{
  namevalue_t *nv;

  nv = llistSetKey (key);
  nv->valuetype = VALUE_LONG;
  nv->u.l = value;
  listSet (list, nv);
  return list;
}

list_t *
llistSetDouble (list_t *list, long key, double value)
{
  namevalue_t *nv;

  nv = llistSetKey (key);
  nv->valuetype = VALUE_DOUBLE;
  nv->u.d = value;
  listSet (list, nv);
  return list;
}

list_t *
llistSetList (list_t *list, long key, list_t *value)
{
  namevalue_t *nv;

  nv = llistSetKey (key);
  nv->valuetype = VALUE_LIST;
  nv->u.data = value;
  listSet (list, nv);
  return list;
}

void *
llistGetData (list_t *list, long key)
{
  void            *value = NULL;
  namevalue_t     *nv;

  nv = llistGetNV (list, key);
  if (nv != NULL &&
      (nv->valuetype == VALUE_DATA || nv->valuetype == VALUE_LIST)) {
    value = (char *) nv->u.data;
  }
  return value;
}

long
llistGetLong (list_t *list, long key)
{
  long            value = -1L;
  namevalue_t     *nv;

  nv = llistGetNV (list, key);
  if (nv != NULL && nv->valuetype == VALUE_LONG) {
    value = nv->u.l;
  }
  return value;
}

double
llistGetDouble (list_t *list, long key)
{
  double          value = 0.0;
  namevalue_t     *nv;

  nv = llistGetNV (list, key);
  if (nv != NULL && nv->valuetype == VALUE_LONG) {
    value = nv->u.d;
  }
  return value;
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

  logProcBegin (LOG_LVL_6, "llistIterateKeyLong");
  if (list == NULL || list->keytype == KEY_STR) {
    logProcEnd (LOG_LVL_6, "llistIterateKeyLong", "null-list/key-str");
    return -1L;
  }

  if (list->iteratorIndex >= list->count) {
    list->iteratorIndex = 0;
    logProcEnd (LOG_LVL_6, "llistIterateKeyLong", "end-list");
    return -1L;      /* indicate the end of the list */
  }

  if (list->type == LIST_NAMEVALUE) {
    namevalue_t *nv = (namevalue_t *) list->data[list->iteratorIndex];
    value = nv->lkey.lkey;

    list->cacheKeyType = KEY_LONG;
    list->keyCache.lkey = value;
    list->locCache = list->iteratorIndex;
  }

  ++list->iteratorIndex;
  logProcEnd (LOG_LVL_6, "llistIterateKeyLong", "");
  return value;
}

/* internal routines */

static long
listLocate (list_t *list, listkey_t key)
{
  long        loc;
  namevalue_t nv;
  int         rc;

  assert (list->ordered == LIST_ORDERED);
  loc = LIST_LOC_INVALID;
  rc = -1;
  if (list->count > 0) {
    if (list->ordered == LIST_ORDERED) {
      if (list->type == LIST_NAMEVALUE) {
        if (list->keytype == KEY_STR) {
          nv.lkey.strkey = key.strkey;
        }
        if (list->keytype == KEY_LONG) {
          nv.lkey.lkey = key.lkey;
        }
        rc = listBinarySearch (list, &nv, (size_t *) &loc);
      } else {
        rc = listBinarySearch (list, key.strkey, (size_t *) &loc);
      }
      if (rc < 0) {
        loc = LIST_LOC_INVALID;
      }
    }
  }

  return loc;
}

static namevalue_t *
slistSetKey (list_t *list, char *key)
{
  namevalue_t     *nv;

  nv = malloc (sizeof (namevalue_t));
  assert (nv != NULL);
  if (list->freeHook != NULL) {
    key = strdup (key);
    assert (key != NULL);
  }
  nv->lkey.strkey = key;
  return nv;
}

static namevalue_t *
slistGetNV (list_t *list, char *key)
{
  namevalue_t     *nv = NULL;
  listkey_t       lkey;
  long            loc;

  if (list != NULL &&
      list->type == LIST_NAMEVALUE) {

      /* check the cache */
    if (list->locCache >= 0L) {
      if (list->cacheKeyType == KEY_STR &&
          strcmp (key, list->keyCache.strkey) == 0) {
        loc = list->locCache;
        nv = list->data [loc];
        ++list->cacheHits;
        return nv;
      }

      if (list->keyCache.strkey != NULL) {
        free (list->keyCache.strkey);
        list->keyCache.strkey = NULL;
        list->locCache = LIST_LOC_INVALID;
      }
    }

    lkey.strkey = key;
    loc = listLocate (list, lkey);

    if (loc >= 0) {
      list->cacheKeyType = KEY_STR;
      list->keyCache.strkey = strdup (key);
      list->locCache = loc;

      nv = list->data [loc];
    }
  }

  return nv;
}

static namevalue_t *
llistSetKey (long key)
{
  namevalue_t     *nv;

  nv = malloc (sizeof (namevalue_t));
  assert (nv != NULL);
  nv->lkey.lkey = key;
  return nv;
}

static namevalue_t *
llistGetNV (list_t *list, long key)
{
  namevalue_t     *nv = NULL;
  listkey_t       lkey;
  long            loc;

  if (list != NULL &&
      list->type == LIST_NAMEVALUE) {

      /* check the cache */
    if (list->locCache >= 0L) {
      if (list->cacheKeyType == KEY_LONG &&
          key == list->keyCache.lkey) {
        loc = list->locCache;
        nv = list->data [loc];
        ++list->cacheHits;
        return nv;
      }

      list->keyCache.lkey = -1L;
      list->locCache = LIST_LOC_INVALID;
    }

    lkey.lkey = key;
    loc = listLocate (list, lkey);

    if (loc >= 0) {
      list->cacheKeyType = KEY_LONG;
      list->keyCache.lkey = key;
      list->locCache = loc;

      nv = list->data [loc];
    }
  }

  return nv;
}


static void
listFreeItem (list_t *list, size_t idx)
{
  void *dp = list->data [idx];
  if (dp != NULL) {
    if (list->type == LIST_NAMEVALUE) {
      namevalue_t *nv = (namevalue_t *) dp;
      if (list->keytype == KEY_STR &&
          nv->lkey.strkey != NULL &&
          list->freeHook != NULL) {
        list->freeHook (nv->lkey.strkey);
      }
      if (nv->valuetype == VALUE_DATA && nv->u.data != NULL &&
          list->freeHookB != NULL) {
        list->freeHookB (nv->u.data);
      }
      if (nv->valuetype == VALUE_LIST && nv->u.data != NULL) {
        listFree (nv->u.data);
      }
      free (nv);
    } else {
      if (list->freeHook != NULL) {
        list->freeHook (dp);
      }
    } /* else not a name/value pair */
  } /* if the data pointer is not null */
}

static void
listInsert (list_t *list, size_t loc, void *data)
{
  size_t      copycount;

  ++list->count;
  if (list->count > list->allocCount) {
    ++list->allocCount;
    list->data = realloc (list->data, list->allocCount * list->dsiz);
  }

  assert (list->data != NULL);
  assert ((list->count > 0 && loc < list->count) ||
          (list->count == 0 && loc == 0));

  copycount = list->count - (loc + 1);
  if (copycount > 0) {
    memcpy (list->data + loc + 1, list->data + loc, copycount * list->dsiz);
  }
  list->data [loc] = data;
  assert (list->bumper1 == 0x11223344);
  assert (list->bumper2 == 0x44332211);
}

static void
listReplace (list_t *list, size_t loc, void *data)
{
  assert (list->data != NULL);
  assert ((list->count > 0 && loc < list->count) ||
          (list->count == 0 && loc == 0));

  listFreeItem (list, loc);
  list->data [loc] = data;
  assert (list->bumper1 == 0x11223344);
  assert (list->bumper2 == 0x44332211);
}

static int
nameValueCompare (const list_t *list, void *d1, void *d2)
{
  int           rc;
  namevalue_t     *nv1, *nv2;

  nv1 = (namevalue_t *) d1;
  nv2 = (namevalue_t *) d2;
  rc = 0;
  if (list->compare != NULL && list->keytype == KEY_STR) {
    rc = list->compare (nv1->lkey.strkey, nv2->lkey.strkey);
  }
  if (list->keytype == KEY_LONG) {
    rc = longCompare (nv1->lkey.lkey, nv2->lkey.lkey);
  }
  return rc;
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
listCompare (const list_t *list, void *a, void *b)
{
  int     rc;

  rc = 0;
  if (list->ordered == LIST_UNORDERED) {
    rc = 0;
  } else {
    if (list->type == LIST_NAMEVALUE) {
      rc = nameValueCompare (list, a, b);
    } else if (list->compare != NULL || list->keytype == KEY_LONG) {
      rc = list->compare (a, b);
    }
  }
  return rc;
}

/* returns the location after as a negative number if not found */
static int
listBinarySearch (const list_t *list, void *data, size_t *loc)
{
  long      l = 0;
  long      r = (long) list->count - 1;
  long      m = 0;
  long      rm;
  int       rc;

  rm = 0;
  while (l <= r) {
    m = l + (r - l) / 2;

    rc = listCompare (list, list->data [m], data);
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
  size_t start2 = mid + 1;

  int rc = listCompare (list, list->data [mid], list->data [start2]);
  if (rc <= 0) {
    return;
  }

  while (start <= mid && start2 <= end) {
    rc = listCompare (list, list->data [start], list->data [start2]);
    if (rc <= 0) {
      start++;
    } else {
      void * value = list->data [start2];
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
