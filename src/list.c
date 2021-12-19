#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "list.h"

static void listFreeItem (list_t *, size_t);
static void listInsert (list_t *, size_t, void *);
static void listReplace (list_t *, size_t, void *);
static int  listBinarySearch (const list_t *, void *, size_t *);
static int  nameValueCompare (const list_t *, void *, void *);
static int  listCompare (const list_t *, void *, void *);
static void merge (list_t *, size_t, size_t, size_t);
static void mergeSort (list_t *, size_t, size_t);

/* best for short lists of data   */
/* data size must be consistent   */

list_t *
listAlloc (size_t dsiz, listorder_t ordered, listCompare_t compare,
    listFree_t freeHook)
{
  list_t    *list;

  list = malloc (sizeof (list_t));
  assert (list != NULL);
  list->data = NULL;
  list->count = 0;
  list->allocCount = 0;
  list->dsiz = dsiz;
  list->ordered = ordered;
  list->type = LIST_BASIC;
  list->valuetype = VALUE_NONE;
  list->bumper1 = 0x11223344;
  list->bumper2 = 0x44332211;
  list->compare = compare;
  list->freeHook = freeHook;
  list->freeHookB = NULL;
  return list;
}

void
listFree (list_t *list)
{
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

long
listFind (list_t *list, void *data)
{
  long        loc;

  assert (list->ordered == LIST_ORDERED);
  loc = -1;
  if (list->count > 0) {
    if (list->ordered == LIST_ORDERED) {
      int rc = listBinarySearch (list, data, (size_t *) &loc);
      if (rc < 0) {
        loc = -1;
      }
    }
  }
  return loc;
}

void
listSort (list_t *list)
{
  list->ordered = LIST_ORDERED;
  mergeSort (list, 0, list->count - 1);
}

/* value list */

list_t *
vlistAlloc (listorder_t ordered, valuetype_t valuetype, listCompare_t compare,
    listFree_t freeHook, listFree_t freeHookB)
{
  list_t    *list;

  list = listAlloc (sizeof (namevalue_t *), ordered, compare, NULL);
  list->type = LIST_NAMEVALUE;
  list->valuetype = valuetype;
  list->freeHook = freeHook;
  list->freeHookB = freeHookB;
  return list;
}

void
vlistFree (list_t *list)
{
  listFree (list);
}

void
vlistSetSize (list_t *list, size_t siz)
{
  listSetSize (list, siz);
}

list_t *
vlistSetData (list_t *list, char *name, void *value)
{
  namevalue_t *nv;

  nv = malloc (sizeof (namevalue_t));
  assert (nv != NULL);
  nv->name = name;
  nv->u.data = value;
  listSet (list, nv);
  return list;
}

list_t *
vlistSetLong (list_t *list, char *name, long value)
{
  namevalue_t *nv;

  nv = malloc (sizeof (namevalue_t));
  assert (nv != NULL);
  nv->name = name;
  nv->u.l = value;
  listSet (list, nv);
  return list;
}

long
vlistFind (list_t *list, char *name)
{
  long      rc;
  rc = listFind (list, name);
  return rc;
}

void
vlistSort (list_t *list)
{
  listSort (list);
}

/* internal routines */

static void
listFreeItem (list_t *list, size_t idx)
{
  void *dp = list->data [idx];
  if (dp != NULL) {
    if (list->type == LIST_NAMEVALUE) {
      namevalue_t *nv = (namevalue_t *) dp;
      if (nv->name != NULL && list->freeHook != NULL) {
        list->freeHook (nv->name);
      }
      if (list->valuetype == VALUE_DATA && nv->u.data != NULL &&
          list->freeHookB != NULL) {
        list->freeHookB (nv->u.data);
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
  rc = list->compare (nv1->name, nv2->name);
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
    } else {
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
  if (l < r) {
    size_t m = l + (r - l) / 2;
    mergeSort (list, l, m);
    mergeSort (list, m + 1, r);
    merge (list, l, m, r);
  }
}
