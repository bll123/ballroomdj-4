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

#include "listmodule.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "bdjstring.h"

/* key/value list, keyed by a nlistidx_t */

[[nodiscard]]
nlist_t *
nlistAlloc (const char *name, nlistorder_t ordered, nlistFree_t valueFreeHook)
{
  nlist_t    *list;

  list = listAlloc (name, LIST_KEY_NUM, ordered, valueFreeHook);
  return list;
}

void
nlistFree (void *list)
{
  listFree (LIST_KEY_NUM, list);
}

nlistidx_t
nlistGetCount (nlist_t *list)
{
  return listGetCount (LIST_KEY_NUM, list);
}

void
nlistSetSize (nlist_t *list, nlistidx_t siz)
{
  listSetSize (LIST_KEY_NUM, list, siz);
}

void
nlistSetFreeHook (nlist_t *list, listFree_t valueFreeHook)  /* KEEP */
{
  if (list == NULL) {
    return;
  }

  listSetFreeHook (LIST_KEY_NUM, list, valueFreeHook);
}

void
nlistCalcMaxValueWidth (nlist_t *list)
{
  listCalcMaxValueWidth (LIST_KEY_NUM, list);
}

int
nlistGetMaxValueWidth (nlist_t *list)
{
  int     value;

  if (list == NULL) {
    return 0;
  }

  value = listGetMaxValueWidth (LIST_KEY_NUM, list);
  return value;
}

void
nlistSort (nlist_t *list)
{
  listSort (LIST_KEY_NUM, list);
}

/* version */

void
nlistSetVersion (nlist_t *list, int version)
{
  listSetVersion (LIST_KEY_NUM, list, version);
}

int
nlistGetVersion (nlist_t *list)
{
  return listGetVersion (LIST_KEY_NUM, list);
}

/* set routines */

void
nlistSetData (nlist_t *list, nlistidx_t lkey, void *data)
{
  listSetNumData (LIST_KEY_NUM, list, lkey, data);
}

void
nlistSetStr (nlist_t *list, nlistidx_t lkey, const char *data)
{
  listSetNumStr (LIST_KEY_NUM, list, lkey, data);
}

void
nlistSetNum (nlist_t *list, nlistidx_t lkey, listnum_t data)
{
  listSetNumNum (LIST_KEY_NUM, list, lkey, data);
}

void
nlistSetDouble (nlist_t *list, nlistidx_t lkey, double data)
{
  listSetNumDouble (LIST_KEY_NUM, list, lkey, data);
}

void
nlistSetList (nlist_t *list, nlistidx_t lkey, nlist_t *data)
{
  listSetNumList (LIST_KEY_NUM, list, lkey, data);
}

void
nlistIncrement (nlist_t *list, nlistidx_t lkey)
{
  nlistidx_t      idx;
  listnum_t       value = 0;

  idx = listGetIdxNumKey (LIST_KEY_NUM, list, lkey);
  value = listGetNumByIdx (LIST_KEY_NUM, list, idx);
  if (value == LIST_VALUE_INVALID) {
    value = 0;
  }
  ++value;
  listSetNumNum (LIST_KEY_NUM, list, lkey, value);
}

void
nlistDecrement (nlist_t *list, nlistidx_t lkey)
{
  nlistidx_t      idx;
  listnum_t       value = 0;

  idx = listGetIdxNumKey (LIST_KEY_NUM, list, lkey);
  value = listGetNumByIdx (LIST_KEY_NUM, list, idx);
  if (value == LIST_VALUE_INVALID) {
    value = 0;
  }
  --value;
  listSetNumNum (LIST_KEY_NUM, list, lkey, value);
}

/* get routines */

nlistidx_t
nlistGetIdx (nlist_t *list, nlistidx_t lkey)  /* TESTING */
{
  nlistidx_t      idx;

  if (list == NULL) {
    return LIST_LOC_INVALID;
  }

  idx = listGetIdxNumKey (LIST_KEY_NUM, list, lkey);
  return idx;
}

void *
nlistGetDataByIdx (nlist_t *list, nlistidx_t idx)
{
  return listGetDataByIdx (LIST_KEY_NUM, list, idx);
}

nlistnum_t
nlistGetNumByIdx (nlist_t *list, nlistidx_t idx)
{
  return listGetNumByIdx (LIST_KEY_NUM, list, idx);
}

nlistidx_t
nlistGetKeyByIdx (nlist_t *list, nlistidx_t idx) /* TESTING */
{
  return listGetKeyNumByIdx (LIST_KEY_NUM, list, idx);
}

void *
nlistGetData (nlist_t *list, nlistidx_t lkey)
{
  nlistidx_t      idx;

  idx = listGetIdxNumKey (LIST_KEY_NUM, list, lkey);
  return listGetDataByIdx (LIST_KEY_NUM, list, idx);
}

const char *
nlistGetStr (nlist_t *list, nlistidx_t lkey)
{
  nlistidx_t      idx;

  idx = listGetIdxNumKey (LIST_KEY_NUM, list, lkey);
  return listGetStrByIdx (LIST_KEY_NUM, list, idx);
}

nlistnum_t
nlistGetNum (nlist_t *list, nlistidx_t lkey)
{
  nlistidx_t      idx;

  idx = listGetIdxNumKey (LIST_KEY_NUM, list, lkey);
  return listGetNumByIdx (LIST_KEY_NUM, list, idx);
}

double
nlistGetDouble (nlist_t *list, nlistidx_t lkey)
{
  nlistidx_t      idx;

  idx = listGetIdxNumKey (LIST_KEY_NUM, list, lkey);
  return listGetDoubleByIdx (LIST_KEY_NUM, list, idx);
}

nlist_t *
nlistGetList (nlist_t *list, nlistidx_t lkey)
{
  return nlistGetData (list, lkey);
}

/* iterators */

void
nlistStartIterator (nlist_t *list, nlistidx_t *iteridx)
{
  *iteridx = LIST_END_LIST;
}

nlistidx_t
nlistIterateKey (nlist_t *list, nlistidx_t *iteridx)
{
  return listIterateKeyNum (LIST_KEY_NUM, list, iteridx);
}

nlistidx_t
nlistIterateKeyPrevious (nlist_t *list, nlistidx_t *iteridx)
{
  return listIterateKeyPreviousNum (LIST_KEY_NUM, list, iteridx);
}

void *
nlistIterateValueData (nlist_t *list, nlistidx_t *iteridx)
{
  return listIterateValue (LIST_KEY_NUM, list, iteridx);
}

listnum_t
nlistIterateValueNum (nlist_t *list, nlistidx_t *iteridx)
{
  return listIterateValueNum (LIST_KEY_NUM, list, iteridx);
}

/* search */

/* returns the key value, not the table index */
nlistidx_t
nlistSearchProbTable (nlist_t *probTable, double dval)
{
  nlistidx_t        l = 0;
  nlistidx_t        r = 0;
  nlistidx_t        m = 0;
  int               rca = 0;
  int               rcb = 0;
  double            d;


  r = listGetCount (LIST_KEY_NUM, probTable) - 1;

  while (l <= r) {
    m = l + (r - l) / 2;

    if (m != 0) {
      d = listGetDoubleByIdx (LIST_KEY_NUM, probTable, m - 1);
      rca = dval > d;
    }
    d = listGetDoubleByIdx (LIST_KEY_NUM, probTable, m);
    rcb = dval <= d;
    if ((m == 0 || rca) && rcb) {
      return listGetKeyNumByIdx (LIST_KEY_NUM, probTable, m);
    }

    if (! rcb) {
      l = m + 1;
    } else {
      r = m - 1;
    }
  }

  return -1;
}

/* debug / informational */

bool
nlistDebugIsCached (list_t *list, listidx_t key) /* TESTING */
{
  return listDebugIsCached (LIST_KEY_NUM, list, key);
}

/* for testing */
nlistidx_t
nlistGetAllocCount (nlist_t *list) /* TESTING */
{
  return listGetAllocCount (LIST_KEY_NUM, list);
}

void
nlistDumpInfo (nlist_t *list)
{
  listDumpInfo (LIST_KEY_NUM, list);
}

/* for testing */
int
nlistGetOrdering (nlist_t *list) /* TESTING */
{
  return listGetOrdering (LIST_KEY_NUM, list);
}
