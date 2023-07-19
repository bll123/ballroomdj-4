/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "bdjstring.h"
#include "ilist.h"
#include "listmodule.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "slist.h"

static nlist_t  *ilistGetDatalist (ilist_t *list, ilistidx_t ikey);

/* key/value list, keyed by a ilistidx_t */

ilist_t *
ilistAlloc (const char *name, ilistorder_t ordered)
{
  ilist_t    *list;

  list = listAlloc (name, LIST_KEY_IND, ordered, ilistFree);
  return list;
}

void
ilistFree (void *list)
{
  listFree (LIST_KEY_IND, list);
}

void
ilistSetVersion (ilist_t *list, int version)
{
  listSetVersion (LIST_KEY_IND, list, version);
}

int
ilistGetVersion (ilist_t *list)
{
  return listGetVersion (LIST_KEY_IND, list);
}

ilistidx_t
ilistGetCount (ilist_t *list)
{
  return listGetCount (LIST_KEY_IND, list);
}

void
ilistSetSize (ilist_t *list, ilistidx_t siz)
{
  listSetSize (LIST_KEY_IND, list, siz);
}

void
ilistSort (ilist_t *list)
{
  listSort (LIST_KEY_IND, list);
}

void
ilistSetDatalist (ilist_t *list, ilistidx_t ikey, nlist_t *datalist)
{
  listSetNumList (LIST_KEY_IND, list, ikey, datalist);
}

void
ilistSetStr (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx, const char *data)
{
  nlist_t *datalist = NULL;

  datalist = ilistGetDatalist (list, ikey);
  nlistSetStr (datalist, lidx, data);
}

void
ilistSetList (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx, void *data)
{
  nlist_t *datalist = NULL;

  datalist = ilistGetDatalist (list, ikey);
  nlistSetList (datalist, lidx, data);
}

void
ilistSetData (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx, void *data)
{
  nlist_t *datalist = NULL;

  datalist = ilistGetDatalist (list, ikey);
  nlistSetData (datalist, lidx, data);
}

void
ilistSetNum (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx, ilistidx_t data)
{
  nlist_t *datalist = NULL;

  if (list == NULL) {
    return;
  }

  datalist = ilistGetDatalist (list, ikey);
  nlistSetNum (datalist, lidx, data);
}

void
ilistSetDouble (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx, double data)
{
  nlist_t     *datalist = NULL;

  if (list == NULL) {
    return;
  }

  datalist = ilistGetDatalist (list, ikey);
  nlistSetDouble (datalist, lidx, data);
}

bool
ilistExists (list_t *list, ilistidx_t ikey)
{
  nlistidx_t      idx;

  if (list == NULL) {
    return false;
  }

  idx = listGetIdxNumKey (LIST_KEY_IND, list, ikey);
  if (idx < 0) {
    return false;
  }

  return true;
}

void *
ilistGetData (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx)
{
  void            *value = NULL;
  ilist_t          *datalist = NULL;

  if (list == NULL) {
    return NULL;
  }

  datalist = ilistGetDatalist (list, ikey);
  if (datalist != NULL) {
    value = nlistGetData (datalist, lidx);
  }
  return value;
}

const char *
ilistGetStr (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx)
{
  const char  *value;

  value = ilistGetData (list, ikey, lidx);
  return value;
}

ilistidx_t
ilistGetNum (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx)
{
  ilistidx_t      value = LIST_VALUE_INVALID;
  nlist_t         *datalist;

  if (list == NULL) {
    return LIST_VALUE_INVALID;
  }

  datalist = ilistGetDatalist (list, ikey);
  if (datalist != NULL) {
    value = nlistGetNum (datalist, lidx);
  }
  return value;
}

double
ilistGetDouble (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx)
{
  double          value = LIST_DOUBLE_INVALID;
  nlist_t         *datalist = NULL;

  if (list == NULL) {
    return LIST_DOUBLE_INVALID;
  }

  datalist = ilistGetDatalist (list, ikey);
  if (datalist != NULL) {
    value = nlistGetDouble (datalist, lidx);
  }
  return value;
}

slist_t *
ilistGetList (ilist_t *list, ilistidx_t ikey, ilistidx_t lidx)
{
  return ilistGetData (list, ikey, lidx);
}

void
ilistDelete (list_t *list, ilistidx_t ikey)
{
  ilistidx_t      idx;

  if (list == NULL) {
    return;
  }

  idx = listGetIdxNumKey (LIST_KEY_IND, list, ikey);
  listDeleteByIdx (LIST_KEY_IND, list, idx);
}

void
ilistStartIterator (ilist_t *list, ilistidx_t *iteridx)
{
  *iteridx = -1;
}

ilistidx_t
ilistIterateKey (ilist_t *list, ilistidx_t *iteridx)
{
  return listIterateKeyNum (LIST_KEY_IND, list, iteridx);
}

/* debug / informational */

void
ilistDumpInfo (ilist_t *list)
{
  listDumpInfo (LIST_KEY_IND, list);
}

/* for testing */
ilistidx_t
ilistGetAllocCount (ilist_t *list)
{
  return listGetAllocCount (LIST_KEY_IND, list);
}

/* for testing */
int
ilistGetOrdering (ilist_t *list)
{
  return listGetOrdering (LIST_KEY_IND, list);
}

/* internal routines */

static nlist_t *
ilistGetDatalist (ilist_t *list, ilistidx_t ikey)
{
  nlist_t         *datalist = NULL;
  char            tbuff [40];
  nlistidx_t      idx;

  if (list == NULL) {
    return NULL;
  }

  idx = listGetIdxNumKey (LIST_KEY_IND, list, ikey);
  datalist = listGetDataByIdx (LIST_KEY_IND, list, idx);

  if (datalist == NULL) {
    snprintf (tbuff, sizeof (tbuff), "%s-item-%d",
        listGetName (LIST_KEY_IND, list), ikey);
    datalist = nlistAlloc (tbuff, LIST_ORDERED, NULL);

    listSetNumList (LIST_KEY_IND, list, ikey, datalist);
  }
  return datalist;
}


