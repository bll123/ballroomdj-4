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
#include "slist.h"

/* keyed by a string */

slist_t *
slistAlloc (const char *name, listorder_t ordered, slistFree_t valueFreeHook)
{
  slist_t    *list;

  list = listAlloc (name, LIST_KEY_STR, ordered, valueFreeHook);
  return list;
}

void
slistFree (void *list)
{
  listFree (LIST_KEY_STR, list);
}

void
slistSetVersion (slist_t *list, int version)
{
  listSetVersion (LIST_KEY_STR, list, version);
}

int
slistGetVersion (slist_t *list)
{
  return listGetVersion (LIST_KEY_STR, list);
}

slistidx_t
slistGetCount (slist_t *list)
{
  return listGetCount (LIST_KEY_STR, list);
}

void
slistSetSize (slist_t *list, listidx_t siz)
{
  listSetSize (LIST_KEY_STR, list, siz);
}

void
slistSetData (slist_t *list, const char *sidx, void *data)
{
  listSetStrData (LIST_KEY_STR, list, sidx, data);
}

void
slistSetStr (slist_t *list, const char *sidx, const char *data)
{
  listSetStrStr (LIST_KEY_STR, list, sidx, data);
}

void
slistSetNum (slist_t *list, const char *sidx, listnum_t data)
{
  listSetStrNum (LIST_KEY_STR, list, sidx, data);
}


void
slistSetList (slist_t *list, const char *sidx, slist_t *data)
{
  listSetStrList (LIST_KEY_STR, list, sidx, data);
}

void
slistDelete (list_t *list, const char *sidx)
{
  slistidx_t      idx;

  if (list == NULL) {
    return;
  }
  if (listGetOrdering (LIST_KEY_STR, list) == LIST_UNORDERED) {
    return;
  }

  idx = listGetIdxStrKey (LIST_KEY_STR, list, sidx);
  listDeleteByIdx (LIST_KEY_STR, list, idx);
}

slistidx_t
slistGetIdx (slist_t *list, const char *sidx)
{
  if (list == NULL) {
    return LIST_LOC_INVALID;
  }
  if (sidx == NULL) {
    return LIST_LOC_INVALID;
  }

  return listGetIdxStrKey (LIST_KEY_STR, list, sidx);
}

void *
slistGetDataByIdx (slist_t *list, slistidx_t idx)
{
  return listGetDataByIdx (LIST_KEY_STR, list, idx);
}

listnum_t
slistGetNumByIdx (slist_t *list, slistidx_t idx)
{
  return listGetNumByIdx (LIST_KEY_STR, list, idx);
}

const char *
slistGetKeyByIdx (slist_t *list, slistidx_t idx)
{
  return listGetKeyStrByIdx (LIST_KEY_STR, list, idx);
}

void *
slistGetData (slist_t *list, const char *sidx)
{
  slistidx_t      idx;

  idx = listGetIdxStrKey (LIST_KEY_STR, list, sidx);
  return listGetDataByIdx (LIST_KEY_STR, list, idx);
}

const char *
slistGetStr (slist_t *list, const char *sidx)
{
  slistidx_t  idx;

  idx = listGetIdxStrKey (LIST_KEY_STR, list, sidx);
  return listGetStrByIdx (LIST_KEY_STR, list, idx);
}

listnum_t
slistGetNum (slist_t *list, const char *sidx)
{
  listnum_t       value = LIST_VALUE_INVALID;
  slistidx_t      idx;

  idx = listGetIdxStrKey (LIST_KEY_STR, list, sidx);
  value = listGetNumByIdx (LIST_KEY_STR, list, idx);
  return value;
}

slist_t *
slistGetList (slist_t *list, const char *sidx)
{
  return slistGetData (list, sidx);
}

void
slistSort (slist_t *list)
{
  listSort (LIST_KEY_STR, list);
}

void
slistStartIterator (slist_t *list, slistidx_t *iteridx)
{
  *iteridx = -1;
}

const char *
slistIterateKey (slist_t *list, slistidx_t *iteridx)
{
  return listIterateKeyStr (LIST_KEY_STR, list, iteridx);
}

void *
slistIterateValueData (slist_t *list, slistidx_t *iteridx)
{
  return listIterateValue (LIST_KEY_STR, list, iteridx);
}

listnum_t
slistIterateValueNum (slist_t *list, slistidx_t *iteridx)
{
  return listIterateValueNum (LIST_KEY_STR, list, iteridx);
}

/* debug / informational */

void
slistDumpInfo (slist_t *list)
{
  listDumpInfo (LIST_KEY_STR, list);
}

/* for testing */
slistidx_t
slistGetAllocCount (slist_t *list)  /* TESTING */
{
  return listGetAllocCount (LIST_KEY_STR, list);
}

/* for testing */
int
slistGetOrdering (slist_t *list) /* TESTING */
{
  return listGetOrdering (LIST_KEY_STR, list);
}
