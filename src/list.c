#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "list.h"

static long listBinarySearch (const list_t *, void *, listCompare_t);
static int  nameValueCompare (void *, void *, listCompare_t);
static void listSwap (list_t *, long, long);
static void heapify (list_t *, long, long, listCompare_t);
static void heapSort (list_t *, listCompare_t);

/* best for short lists of data   */
/* data size must be consistent   */

list_t *
listAlloc (size_t dsiz, listorder_t ordered)
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
listAdd (list_t *list, void *data, listCompare_t compare)
{
  long        loc;

  loc = 0;
  if (list->count > 0) {
    if (list->ordered == LIST_ORDERED) {
      loc = listBinarySearch (list, data, compare);
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
listFind (list_t *list, void *data, listCompare_t compare)
{
  long        loc;

  loc = -1;
  if (list->count > 0) {
    if (list->ordered == LIST_ORDERED) {
      loc = listBinarySearch (list, data, compare);
    } else {
      loc = 0; /* ### FIX */
    }
  }
  return loc;
}

void
listSort (list_t *list, listCompare_t compare)
{
  heapSort (list, compare);
  list->ordered = LIST_ORDERED;
}

/* value list */

list_t *
vlistAlloc (listorder_t ordered, valuetype_t valuetype)
{
  list_t    *list;

  list = listAlloc (sizeof (namevalue_t *), ordered);
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
vlistAddStr (list_t *list, char *name, char *value, listCompare_t compare)
{
  namevalue_t *nv;

  nv = malloc (sizeof (namevalue_t));
  assert (nv != NULL);
  nv->name = name;
  nv->u.str = value;
  listAdd (list, nv, compare);
  return list;
}

list_t *
vlistAddLong (list_t *list, char *name, long value, listCompare_t compare)
{
  namevalue_t *nv;

  nv = malloc (sizeof (namevalue_t));
  assert (nv != NULL);
  nv->name = name;
  nv->u.l = value;
  listAdd (list, nv, compare);
  return list;
}

long
vlistFind (list_t *list, char *name, listCompare_t compare)
{
  long      rc;
  rc = listFind (list, name, compare);
  return rc;
}

void
vlistSort (list_t *list, listCompare_t compare)
{
  listSort (list, compare);
}

/* internal routines */

static int
nameValueCompare (void *d1, void *d2, listCompare_t compare)
{
  int           rc;
  namevalue_t     *nv1, *nv2;

  nv1 = (namevalue_t *) d1;
  nv2 = (namevalue_t *) d2;
  rc = compare (nv1->name, nv2->name);
  return rc;
}

/* returns the location after as a negative number if not found */
static long
listBinarySearch (const list_t *list, void *data, listCompare_t compare)
{
  long      l = 0;
  long      r = (long) list->count - 1;
  long      m = 0;
  long      rm;
  int       rc;

  rm = 0;
  while (l <= r) {
    m = l + (r - l) / 2;

    if (list->type == LIST_NAMEVALUE) {
      rc = nameValueCompare (list->data[m], data, compare);
    } else {
      rc = compare (list->data[m], data);
    }
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

static void
listSwap (list_t *list, long a, long b) {
  void *temp = list->data[a];
  list->data[a] = list->data[b];
  list->data[b] = temp;
}

static void
heapify (list_t *list, long n, long i, listCompare_t compare)
{
  long  largest = i;
  long  left = 2 * i + 1;
  long  right = 2 * i + 2;
  int   rc;

  if (left < n) {
    if (list->type == LIST_NAMEVALUE) {
      rc = nameValueCompare (list->data [left], list->data [largest], compare);
    } else {
      rc = compare (list->data [left], list->data [largest]);
    }
    if (rc > 0) {
      largest = left;
    }
  }

  if (right < n) {
    if (list->type == LIST_NAMEVALUE) {
      rc = nameValueCompare (list->data [right], list->data [largest], compare);
    } else {
      rc = compare (list->data [right], list->data [largest]);
    }
    if (rc > 0) {
      largest = right;
    }
  }

  if (largest != i) {
    listSwap (list, i, largest);
    heapify (list, n, largest, compare);
  }
}

// Main function to do heap sort
static void
heapSort (list_t *list, listCompare_t compare)
{
  // Build max heap
  for (long i = (long) list->count / 2 - 1; i >= 0; i--) {
    heapify (list, (long) list->count, i, compare);
  }

  // Heap sort
  for (long i = (long) list->count - 1; i >= 0; i--) {
    listSwap (list, 0, i);

    // Heapify root element to get highest element at root again
    heapify (list, i, 0, compare);
  }
}
