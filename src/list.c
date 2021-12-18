#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "list.h"

static long listBinarySearch (const list_t *, void *);
static int  nameValueCompare (const list_t *, void *, void *);
static int  listCompare (const list_t *, void *, void *);
static void merge (list_t *, size_t, size_t, size_t);
static void mergeSort (list_t *, size_t, size_t);

/* best for short lists of data   */
/* data size must be consistent   */

list_t *
listAlloc (size_t dsiz, listorder_t ordered, listCompare_t compare)
{
  list_t    *list;

  list = malloc (sizeof (list_t));
  assert (list != NULL);
  list->data = NULL;
  list->count = 0;
  list->dsiz = dsiz;
  list->ordered = ordered;
  list->type = LIST_BASIC;
  list->valuetype = VALUE_NONE;
  list->bumper1 = 0x11223344;
  list->bumper2 = 0x44332211;
  list->compare = compare;
  return list;
}

void
listFree (list_t *list)
{
  namevalue_t     *nv;

  if (list != NULL) {
    if (list->data != NULL) {
      if (list->type == LIST_NAMEVALUE) {
        for (size_t i = 0; i < list->count; ++i) {
          nv = (namevalue_t *) list->data[i];
          free (nv);
        }
      }
      free (list->data);
    }
    free (list);
  }
}

/* all items added to the list must have been allocated */
void
listFreeAll (list_t *list)
{
  void          *dp;
  namevalue_t   *nv;

  if (list != NULL) {
    if (list->data != NULL) {
      for (size_t i = 0; i < list->count; ++i) {
        dp = list->data + i;
        if (dp != NULL) {
          if (list->type == LIST_NAMEVALUE) {
            nv = (namevalue_t *) dp;
            if (nv->name != NULL) {
              free (nv->name);
            }
            if (list->valuetype == VALUE_STR && nv->u.str != NULL) {
              free (nv->u.str);
            }
            free (nv);
          }
          free (list->data + i);
        }
      }
      free (list->data);
    }
    free (list);
  }
}

list_t *
listAdd (list_t *list, void *data)
{
  long        loc;

  loc = 0;
  if (list->count > 0) {
    if (list->ordered == LIST_ORDERED) {
      loc = listBinarySearch (list, data);
    } else {
      loc = (long) list->count;
    }
  }
  if (loc < 0) {
    loc = - loc;
  }
  listInsert (list, (size_t) loc, data);
  return list;
}

void
listInsert (list_t *list, size_t loc, void *data)
{
  size_t      copycount;

  ++list->count;
  list->data = realloc (list->data, list->count * list->dsiz);

  assert (list->data != NULL);
  assert ((list->count > 0 && loc < list->count) ||
          (list->count == 0 && loc == 0));

  copycount = list->count - (loc + 1);
  if (copycount > 0) {
    memcpy (list->data + loc + 1, list->data + loc, copycount * list->dsiz);
  }
  list->data[loc] = data;
  assert (list->bumper1 == 0x11223344);
  assert (list->bumper2 == 0x44332211);
}

long
listFind (list_t *list, void *data)
{
  long        loc;

  loc = -1;
  if (list->count > 0) {
    if (list->ordered == LIST_ORDERED) {
      loc = listBinarySearch (list, data);
    } else {
      loc = 0; /* ### FIX */
    }
  }
  return loc;
}

void
listSort (list_t *list)
{
  mergeSort (list, 0, list->count - 1);
  list->ordered = LIST_ORDERED;
}

/* value list */

list_t *
vlistAlloc (listorder_t ordered, valuetype_t valuetype, listCompare_t compare)
{
  list_t    *list;

  list = listAlloc (sizeof (namevalue_t *), ordered, compare);
  list->type = LIST_NAMEVALUE;
  list->valuetype = valuetype;
  return list;
}

void
vlistFree (list_t *list)
{
  listFree (list);
}

/* all items added to the list must have been allocated */
void
vlistFreeAll (list_t *list)
{
  listFreeAll (list);
}

list_t *
vlistAddStr (list_t *list, char *name, char *value)
{
  namevalue_t *nv;

  nv = malloc (sizeof (namevalue_t));
  assert (nv != NULL);
  nv->name = name;
  nv->u.str = value;
  listAdd (list, nv);
  return list;
}

list_t *
vlistAddLong (list_t *list, char *name, long value)
{
  namevalue_t *nv;

  nv = malloc (sizeof (namevalue_t));
  assert (nv != NULL);
  nv->name = name;
  nv->u.l = value;
  listAdd (list, nv);
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

  if (list->type == LIST_NAMEVALUE) {
    rc = nameValueCompare (list, a, b);
  } else {
    rc = list->compare (a, b);
  }
  return rc;
}

/* returns the location after as a negative number if not found */
static long
listBinarySearch (const list_t *list, void *data)
{
  long      l = 0;
  long      r = (long) list->count - 1;
  long      m = 0;
  long      rm;
  int       rc;

  rm = 0;
  while (l <= r) {
    m = l + (r - l) / 2;

    rc = listCompare (list, list->data[m], data);
    if (rc == 0) {
      return m;
    }

    if (rc < 0) {
      l = m + 1;
      rm = l;
    } else {
      r = m - 1;
      rm = m;
    }
  }

  return rm;
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
