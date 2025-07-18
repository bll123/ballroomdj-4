/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
#include "listmodule.h"
#include "log.h"
#include "mdebug.h"
#include "tmutil.h"

enum {
  LIST_IDENT = 0xccbbaa007473696c,
};

typedef union {
  const char  *strkey;
  listidx_t   idx;
} listkeylookup_t;

typedef union {
  char        *strkey;
  listidx_t   idx;
} listkey_t;

typedef union {
  void        *data;
  listnum_t   num;
  double      dval;
} listvalue_t;

typedef struct {
  listkey_t     key;
  valuetype_t   valuetype;
  listvalue_t   value;
} listitem_t;

typedef struct list {
  uint64_t        listident;
  char            *name;
  int             version;
  listidx_t       count;
  listidx_t       allocCount;
  int             maxValueWidth;
  keytype_t       keytype;
  listorder_t     ordered;
  listitem_t      *data;        /* array */
  listkey_t       keyCache;
  listidx_t       locCache;
  long            readCacheHits;
  long            writeCacheHits;
  listFree_t      valueFreeHook;
  bool            replace;
  bool            setmaxkey;
} list_t;

static void     listSet (list_t *list, listitem_t *item);
static listidx_t listGetIdx_int (list_t *list, listkeylookup_t *key);
static bool     listCheckIfValid (list_t *list, keytype_t keytype);
static void     listFreeItem (list_t *, listidx_t);
static listidx_t listIterateKeyGetNum (list_t *list, listidx_t *iteridx);
static void     listInsert (list_t *, listidx_t loc, listitem_t *item);
static void     listReplace (list_t *, listidx_t, listitem_t *item);
static int      listBinarySearch (const list_t *, listkeylookup_t *key, listidx_t *);
static int      idxCompare (listidx_t, listidx_t);
static int      listCompare (const list_t *, const listkey_t *a, const listkey_t *b);
static long     merge (list_t *, listidx_t, listidx_t, listidx_t);
static long     mergeSort (list_t *, listidx_t, listidx_t);
static void     listClearCache (list_t *list);
static listidx_t listCheckCache (list_t *list, listkeylookup_t *key);

list_t *
listAlloc (const char *name, keytype_t keytype, listorder_t ordered, listFree_t valueFreeHook)
{
  list_t    *list;

  list = mdmalloc (sizeof (list_t));
  list->listident = LIST_IDENT;
  /* always allocate the name so that dynamic names can be created */
  list->name = mdstrdup (name);
  list->data = NULL;
  list->ordered = ordered;
  list->keytype = keytype;
  list->version = 1;
  list->valueFreeHook = valueFreeHook;
  /* counts */
  list->count = 0;
  list->allocCount = 0;
  list->maxValueWidth = 0;
  /* flags */
  list->replace = false;
  list->setmaxkey = false;
  /* cache */
  list->keyCache.strkey = NULL;
  list->locCache = LIST_LOC_INVALID;
  list->readCacheHits = 0;
  list->writeCacheHits = 0;

  logMsg (LOG_DBG, LOG_LIST, "list alloc %s", name);
  return list;
}

void
listFree (keytype_t keytype, void *tlist)
{
  list_t *list = (list_t *) tlist;

  if (! listCheckIfValid (list, keytype)) {
    return;
  }

  logMsg (LOG_DBG, LOG_LIST, "list free %s", list->name);
  if (list->readCacheHits > 0 || list->writeCacheHits > 0) {
    logMsg (LOG_DBG, LOG_LIST,
        "list %s: cache read:%ld write:%ld",
        list->name, list->readCacheHits, list->writeCacheHits);
  }
  listClearCache (list);
  if (list->data != NULL) {
    for (listidx_t i = 0; i < list->count; ++i) {
      listFreeItem (list, i);
    }
    mdfree (list->data);
    list->data = NULL;
  } /* data is not null */

  list->count = 0;
  list->allocCount = 0;
  dataFree (list->name);
  list->name = NULL;
  mdfree (list);
}

/* list management */

void
listSetSize (keytype_t keytype, list_t *list, listidx_t siz)
{
  if (! listCheckIfValid (list, keytype)) {
    return;
  }

  if (siz > list->allocCount) {
    listidx_t   tsiz;

    tsiz = list->allocCount;
    list->allocCount = siz;
    list->data = mdrealloc (list->data,
        (size_t) list->allocCount * sizeof (listitem_t));
    memset (list->data + tsiz, '\0', sizeof (listitem_t) * (siz - tsiz));
  }
}

void
listSort (keytype_t keytype, list_t *list)
{
  mstime_t      tm;
  time_t        elapsed;
  long          swaps;

  if (! listCheckIfValid (list, keytype)) {
    return;
  }

  mstimestart (&tm);
  list->ordered = LIST_ORDERED;
  swaps = mergeSort (list, 0, list->count - 1);
  elapsed = mstimeend (&tm);
  if (elapsed > 0) {
    logMsg (LOG_DBG, LOG_LIST, "sort of %s took %" PRId64 " ms with %ld swaps", list->name, (int64_t) elapsed, swaps);
  }
}

void
listCalcMaxValueWidth (keytype_t keytype, list_t *list)
{
  int     maxlen = 10;

  if (! listCheckIfValid (list, keytype)) {
    return;
  }

  for (listidx_t i = 0; i < list->count; ++i) {
    listitem_t    *item = &list->data [i];

    if (item->valuetype == VALUE_STR) {
      size_t    len;

      len = istrlen ((const char *) list->data [i].value.data);
      if ((int) len > maxlen) {
        maxlen = len;
      }
    }
  }
  list->maxValueWidth = maxlen;
}

const char *
listGetName (keytype_t keytype, list_t *list)
{
  if (! listCheckIfValid (list, keytype)) {
    return 0;
  }
  return list->name;
}

void
listSetFreeHook (keytype_t keytype, list_t *list, listFree_t valueFreeHook)
{
  if (! listCheckIfValid (list, keytype)) {
    return;
  }
  list->valueFreeHook = valueFreeHook;
}

/* counts */

listidx_t
listGetCount (keytype_t keytype, list_t *list)
{
  if (! listCheckIfValid (list, keytype)) {
    return 0;
  }
  return list->count;
}

int
listGetMaxValueWidth (keytype_t keytype, list_t *list)
{
  if (! listCheckIfValid (list, keytype)) {
    return 0;
  }
  return list->maxValueWidth;
}

/* version */

void
listSetVersion (keytype_t keytype, list_t *list, int version)
{
  if (! listCheckIfValid (list, keytype)) {
    return;
  }
  list->version = version;
}

int
listGetVersion (keytype_t keytype, list_t *list)
{
  if (! listCheckIfValid (list, keytype)) {
    return LIST_NO_VERSION;
  }
  return list->version;
}

/* iterators */

listidx_t
listIterateKeyNum (keytype_t keytype, list_t *list, listidx_t *iteridx)
{
  if (! listCheckIfValid (list, keytype)) {
    return LIST_LOC_INVALID;
  }

  ++(*iteridx);

  return listIterateKeyGetNum (list, iteridx);
}

listidx_t
listIterateKeyPreviousNum (keytype_t keytype, list_t *list, listidx_t *iteridx)
{
  if (! listCheckIfValid (list, keytype)) {
    return LIST_LOC_INVALID;
  }

  --(*iteridx);
  if (*iteridx < 0) {
    /* do not decrement further! */
    *iteridx = -1;
  }

  return listIterateKeyGetNum (list, iteridx);
}

const char *
listIterateKeyStr (keytype_t keytype, list_t *list, listidx_t *iteridx)
{
  char    *value = NULL;

  if (! listCheckIfValid (list, keytype)) {
    return NULL;
  }

  ++(*iteridx);
  if (*iteridx >= list->count) {
    *iteridx = LIST_END_LIST;
    return NULL;
  }

  value = list->data [*iteridx].key.strkey;

  listClearCache (list);

  list->keyCache.strkey = mdstrdup (value);
  list->locCache = *iteridx;

  return value;
}

void *
listIterateValue (keytype_t keytype, list_t *list, listidx_t *iteridx)
{
  void  *value = NULL;

  if (! listCheckIfValid (list, keytype)) {
    return NULL;
  }

  ++(*iteridx);
  if (*iteridx >= list->count) {
    *iteridx = LIST_END_LIST;
    return NULL;  /* indicate the end of the list */
  }

  value = list->data [*iteridx].value.data;
  return value;
}

listnum_t
listIterateValueNum (keytype_t keytype, list_t *list, listidx_t *iteridx)
{
  listnum_t   value = LIST_VALUE_INVALID;

  if (! listCheckIfValid (list, keytype)) {
    return LIST_VALUE_INVALID;
  }

  ++(*iteridx);
  if (*iteridx >= list->count) {
    *iteridx = LIST_END_LIST;
    return LIST_VALUE_INVALID;  /* indicate the end of the list */
  }

  value = list->data [*iteridx].value.num;
  return value;
}

listidx_t
listGetIdxNumKey (keytype_t keytype, list_t *list, listidx_t idx)
{
  listkeylookup_t key;

  if (! listCheckIfValid (list, keytype)) {
    return LIST_LOC_INVALID;
  }

  key.idx = idx;
  return listGetIdx_int (list, &key);
}

listidx_t
listGetIdxStrKey (keytype_t keytype, list_t *list, const char *str)
{
  listkeylookup_t key;

  if (! listCheckIfValid (list, keytype)) {
    return LIST_LOC_INVALID;
  }

  key.strkey = str;
  return listGetIdx_int (list, &key);
}

listidx_t
listGetKeyNumByIdx (keytype_t keytype, list_t *list, listidx_t idx)
{
  if (! listCheckIfValid (list, keytype)) {
    return LIST_VALUE_INVALID;
  }

  if (idx >= 0 && idx < list->count) {
    return list->data [idx].key.idx;
  }
  return LIST_VALUE_INVALID;
}

const char *
listGetKeyStrByIdx (keytype_t keytype, list_t *list, listidx_t idx)
{
  if (! listCheckIfValid (list, keytype)) {
    return NULL;
  }

  if (idx >= 0 && idx < list->count) {
    return list->data [idx].key.strkey;
  }
  return NULL;
}

void *
listGetDataByIdx (keytype_t keytype, list_t *list, listidx_t idx)
{
  void    *value = NULL;

  if (! listCheckIfValid (list, keytype)) {
    return value;
  }
  if (idx >= 0 && idx < list->count) {
    value = list->data [idx].value.data;
  }

  logMsg (LOG_DBG, LOG_LIST, "list:gdatabi:%s idx:%" PRId32 " ?/%d", list->name, idx, value == NULL);
  return value;
}

const char *
listGetStrByIdx (keytype_t keytype, list_t *list, listidx_t idx)
{
  const char  *value = NULL;

  if (! listCheckIfValid (list, keytype)) {
    return value;
  }
  if (idx >= 0 && idx < list->count) {
    value = list->data [idx].value.data;
  }

  value = listGetDataByIdx (keytype, list, idx);
  logMsg (LOG_DBG, LOG_LIST, "list:gsbi:%s idx:%" PRId32 " %s/%d", list->name, idx, value, value == NULL);
  return value;
}

listnum_t
listGetNumByIdx (keytype_t keytype, list_t *list, listidx_t idx)
{
  listnum_t   value = LIST_VALUE_INVALID;

  if (! listCheckIfValid (list, keytype)) {
    return value;
  }
  if (idx >= 0 && idx < list->count) {
    value = list->data [idx].value.num;
  }
  logMsg (LOG_DBG, LOG_LIST, "list:gnbi:%s idx:%" PRId32 " %" PRId64, list->name, idx, value);
  return value;
}

double
listGetDoubleByIdx (keytype_t keytype, list_t *list, listidx_t idx)
{
  double    value = LIST_DOUBLE_INVALID;

  if (! listCheckIfValid (list, keytype)) {
    return value;
  }
  if (idx >= 0 && idx < list->count) {
    value = list->data [idx].value.dval;
  }

  logMsg (LOG_DBG, LOG_LIST, "list:gdbi:%s idx:%" PRId32 " %.2f", list->name, idx, value);
  return value;
}

void
listDeleteByIdx (keytype_t keytype, list_t *list, listidx_t idx)
{
  listidx_t   copycount;

  if (! listCheckIfValid (list, keytype)) {
    return;
  }
  if (idx < 0 || idx >= list->count) {
    return;
  }

  /* the cache must be invalidated */
  listClearCache (list);

  copycount = list->count - idx - 1;
  listFreeItem (list, idx);
  list->count -= 1;

  if (copycount > 0) {
    for (listidx_t i = idx; i < list->count; ++i) {
       memcpy (list->data + i, list->data + i + 1, sizeof (listitem_t));
    }
  }
  logMsg (LOG_DBG, LOG_LIST, "list-del:%s idx:%" PRId32, list->name, idx);
}

void
listSetStrData (keytype_t keytype, list_t *list, const char *key, void *data)
{
  listitem_t    item;

  if (! listCheckIfValid (list, keytype)) {
    return;
  }

  item.key.strkey = mdstrdup (key);
  item.valuetype = VALUE_DATA;
  item.value.data = data;
  listSet (list, &item);
}

void
listSetStrList (keytype_t keytype, list_t *list, const char *key, void *data)
{
  listitem_t    item;

  if (! listCheckIfValid (list, keytype)) {
    return;
  }

  item.key.strkey = mdstrdup (key);
  item.valuetype = VALUE_LIST;
  item.value.data = data;
  listSet (list, &item);
}

void
listSetStrStr (keytype_t keytype, list_t *list, const char *key, const char *str)
{
  listitem_t    item;

  if (! listCheckIfValid (list, keytype)) {
    return;
  }

  item.key.strkey = mdstrdup (key);
  item.valuetype = VALUE_STR;
  item.value.data = NULL;
  if (str != NULL) {
    item.value.data = mdstrdup (str);
  }
  listSet (list, &item);
}

void
listSetStrNum (keytype_t keytype, list_t *list, const char *key, listnum_t val)
{
  listitem_t    item;

  if (! listCheckIfValid (list, keytype)) {
    return;
  }

  item.key.strkey = mdstrdup (key);
  item.valuetype = VALUE_NUM;
  item.value.num = val;
  listSet (list, &item);
}

void
listSetNumData (keytype_t keytype, list_t *list, listidx_t key, void *data)
{
  listitem_t    item;

  if (! listCheckIfValid (list, keytype)) {
    return;
  }

  item.key.idx = key;
  item.valuetype = VALUE_DATA;
  item.value.data = data;
  listSet (list, &item);
}

void
listSetNumList (keytype_t keytype, list_t *list, listidx_t key, void *data)
{
  listitem_t    item;

  if (! listCheckIfValid (list, keytype)) {
    return;
  }

  item.key.idx = key;
  item.valuetype = VALUE_LIST;
  item.value.data = data;
  listSet (list, &item);
}

void
listSetNumStr (keytype_t keytype, list_t *list, listidx_t key, const char *str)
{
  listitem_t    item;

  if (! listCheckIfValid (list, keytype)) {
    return;
  }

  item.key.idx = key;
  item.valuetype = VALUE_STR;
  item.value.data = NULL;
  if (str != NULL) {
    item.value.data = mdstrdup (str);
  }
  listSet (list, &item);
}

void
listSetNumNum (keytype_t keytype, list_t *list, listidx_t key, listnum_t val)
{
  listitem_t    item;

  if (! listCheckIfValid (list, keytype)) {
    return;
  }

  item.key.idx = key;
  item.valuetype = VALUE_NUM;
  item.value.num = val;
  listSet (list, &item);
}

void
listSetNumDouble (keytype_t keytype, list_t *list, listidx_t key, double dval)
{
  listitem_t    item;

  if (! listCheckIfValid (list, keytype)) {
    return;
  }

  item.key.idx = key;
  item.valuetype = VALUE_DOUBLE;
  item.value.dval = dval;
  listSet (list, &item);
}

/* debug / informational */

void
listDumpInfo (keytype_t keytype, list_t *list)
{
  if (! listCheckIfValid (list, keytype)) {
    if (list == NULL) {
      logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: list: null");
    }
    return;
  }
  logMsg (LOG_DBG, LOG_DATAFILE, "list: %s count: %" PRId32 " key:%d ordered:%d",
      list->name, list->count, list->keytype, list->ordered);
}

/* used by the test suite */
bool
listDebugIsCached (keytype_t keytype, list_t *list, listidx_t key)
{
  bool  rc;

  if (! listCheckIfValid (list, keytype)) {
    return false;
  }

  rc = list->locCache != LIST_LOC_INVALID &&
      list->keyCache.idx == key;
  return rc;
}

/* for testing */
listidx_t
listGetAllocCount (keytype_t keytype, list_t *list)
{
  if (! listCheckIfValid (list, keytype)) {
    return 0;
  }
  return list->allocCount;
}

/* for testing */
int
listGetOrdering (keytype_t keytype, list_t *list)
{
  if (! listCheckIfValid (list, keytype)) {
    return LIST_UNORDERED;
  }

  return list->ordered;
}

/* internal routines */

static void
listSet (list_t *list, listitem_t *item)
{
  listidx_t       loc = 0;
  int             rc = -1;
  int             found = 0;

  if (list == NULL) {
    return;
  }

  loc = listCheckCache (list, (listkeylookup_t *) &item->key);
  if (loc != LIST_LOC_INVALID) {
    ++list->writeCacheHits;
    found = 1;
    rc = 0;
  }

  if (! found) {
    loc = 0;
  }

  if (! found && list->count > 0) {
    if (list->ordered == LIST_ORDERED) {
      rc = listBinarySearch (list, (listkeylookup_t *) &item->key, &loc);
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

listidx_t
listGetIdx_int (list_t *list, listkeylookup_t *key)
{
  listidx_t   idx;
  listidx_t   ridx = LIST_LOC_INVALID;
  int         rc;

  if (list == NULL) {
    return LIST_LOC_INVALID;
  }

  /* check the cache */
  ridx = listCheckCache (list, key);
  if (ridx != LIST_LOC_INVALID) {
    ++list->readCacheHits;
    return ridx;
  }

  if (list->ordered == LIST_ORDERED) {
    rc = listBinarySearch (list, key, &idx);
    if (rc == 0) {
      ridx = idx;
    }
  } else if (list->replace) {
    for (listidx_t i = 0; i < list->count; ++i) {
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

  listClearCache (list);

  if (ridx >= 0) {
    list->keyCache.strkey = NULL;
    if (list->keytype == LIST_KEY_STR && key->strkey != NULL) {
      list->keyCache.strkey = mdstrdup (key->strkey);
    }
    if (list->keytype == LIST_KEY_NUM ||
        list->keytype == LIST_KEY_IND) {
      list->keyCache.idx = key->idx;
    }
    list->locCache = ridx;
  }
  return ridx;
}

static bool
listCheckIfValid (list_t *list, keytype_t keytype)
{
  if (list == NULL) {
    return false;
  }

  if (list->listident != LIST_IDENT) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: list: bad ident");
    return false;
  }

  if (list->keytype != keytype) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: list: mismatched key %s have:%d got:%d", list->name, list->keytype, keytype);
    return false;
  }

  return true;
}

static void
listFreeItem (list_t *list, listidx_t idx)
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
        dp->key.strkey != NULL) {
      mdfree (dp->key.strkey);
      dp->key.strkey = NULL;
    }
    if (dp->valuetype == VALUE_STR &&
        dp->value.data != NULL) {
      mdfree (dp->value.data);
      dp->value.data = NULL;
    }
    if (dp->valuetype == VALUE_DATA &&
        dp->value.data != NULL &&
        list->valueFreeHook != NULL) {
      list->valueFreeHook (dp->value.data);
    }
    if (dp->valuetype == VALUE_LIST &&
        dp->value.data != NULL) {
      listFree (((list_t *) dp->value.data)->keytype, dp->value.data);
    }
  } /* if the data pointer is not null */
}

static listidx_t
listIterateKeyGetNum (list_t *list, listidx_t *iteridx)
{
  listidx_t   value = LIST_LOC_INVALID;

  if (*iteridx < 0 || *iteridx >= list->count) {
    *iteridx = LIST_END_LIST;
    return LIST_LOC_INVALID;      /* indicate the beg/end of the list */
  }

  value = list->data [*iteridx].key.idx;

  list->keyCache.idx = value;
  list->locCache = *iteridx;
  return value;
}

static void
listInsert (list_t *list, listidx_t loc, listitem_t *item)
{
  listidx_t      copycount;

  /* the cache must be invalidated */
  listClearCache (list);

  ++list->count;
  if (list->count > list->allocCount) {
    list->allocCount += 5;
    list->data = mdrealloc (list->data,
        (size_t) list->allocCount * sizeof (listitem_t));
  }

  //assert ((list->count > 0 && loc < list->count) ||
  //        (list->count == 0 && loc == 0));

  copycount = list->count - (loc + 1);
  if (loc != -1 && copycount > 0) {
    for (listidx_t i = copycount - 1; i >= 0; --i) {
       memcpy (list->data + loc + i + 1, list->data + loc + i, sizeof (listitem_t));
    }
  }
  memcpy (&list->data [loc], item, sizeof (listitem_t));
}

static void
listReplace (list_t *list, listidx_t loc, listitem_t *item)
{
  //assert ((list->count > 0 && loc < list->count) ||
  //        (list->count == 0 && loc == 0));

  listFreeItem (list, loc);
  memcpy (&list->data [loc], item, sizeof (listitem_t));
}

static inline int
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
listCompare (const list_t *list, const listkey_t *a, const listkey_t *b)
{
  int         rc;

  rc = 0;
  if (list->ordered == LIST_UNORDERED) {
    rc = 0;
  } else {
    if (list->keytype == LIST_KEY_STR) {
      rc = istringCompare (a->strkey, b->strkey);
    }
    if (list->keytype == LIST_KEY_NUM ||
        list->keytype == LIST_KEY_IND) {
      rc = idxCompare (a->idx, b->idx);
    }
  }
  return rc;
}

/* returns the location after as a negative number if not found */
static int
listBinarySearch (const list_t *list, listkeylookup_t *key, listidx_t *loc)
{
  listidx_t     l = 0;
  listidx_t     r = list->count - 1;
  listidx_t     m = 0;
  listidx_t     rm;
  int           rc;

  rm = 0;
  while (l <= r) {
    m = l + (r - l) / 2;

    rc = listCompare (list, &list->data [m].key, (listkey_t *) key);
    if (rc == 0) {
      *loc = (listidx_t) m;
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

  *loc = (listidx_t) rm;
  return -1;
}

/*
 * in-place merge sort from:
 * https://www.geeksforgeeks.org/in-place-merge-sort/
 */

static long
merge (list_t *list, listidx_t start, listidx_t mid, listidx_t end)
{
  listidx_t   start2 = mid + 1;
  listitem_t  value;
  long        swaps = 0;

  int rc = listCompare (list, &list->data [mid].key, &list->data [start2].key);
  if (rc <= 0) {
    return swaps;
  }

  while (start <= mid && start2 <= end) {
    rc = listCompare (list, &list->data [start].key, &list->data [start2].key);
    if (rc <= 0) {
      start++;
    } else {
      listidx_t       index;

      ++swaps;
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

  return swaps;
}

static long
mergeSort (list_t *list, listidx_t l, listidx_t r)
{
  long swaps = 0;

  if (list->count > 0 && l < r) {
    listidx_t m = l + (r - l) / 2;
    swaps += mergeSort (list, l, m);
    swaps += mergeSort (list, m + 1, r);
    swaps += merge (list, l, m, r);
  }

  return swaps;
}

static inline void
listClearCache (list_t *list)
{
  if (list->keytype == LIST_KEY_STR &&
      list->keyCache.strkey != NULL) {
    mdfree (list->keyCache.strkey);
    list->keyCache.strkey = NULL;
  }
  list->locCache = LIST_LOC_INVALID;
}

static inline listidx_t
listCheckCache (list_t *list, listkeylookup_t *key)
{
  listidx_t   ridx = LIST_LOC_INVALID;

  if (list->locCache >= 0L) {
    if ((list->keytype == LIST_KEY_STR &&
         key->strkey != NULL &&
         list->keyCache.strkey != NULL &&
         strcmp (key->strkey, list->keyCache.strkey) == 0) ||
        ((list->keytype == LIST_KEY_NUM ||
          list->keytype == LIST_KEY_IND) &&
         key->idx == list->keyCache.idx)) {
      ridx = list->locCache;
    }
  }

  return ridx;
}
