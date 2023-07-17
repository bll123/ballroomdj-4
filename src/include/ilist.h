/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_ILIST_H
#define INC_ILIST_H

#include "list.h"
#include "slist.h"

typedef list_t      ilist_t;
typedef listidx_t   ilistidx_t;
typedef listorder_t ilistorder_t;

  /* keyed by a ilistidx_t */
ilist_t   *ilistAlloc (const char *name, ilistorder_t);
void      ilistFree (void * list);
void      ilistSetVersion (ilist_t *list, int version);
int       ilistGetVersion (ilist_t *list);
ilistidx_t   ilistGetCount (ilist_t *list);
ilistidx_t   ilistGetAllocCount (ilist_t *list);
void      ilistSetSize (ilist_t *, ilistidx_t);
  /* set routines */
void      ilistSetStr (list_t *, ilistidx_t ikey, ilistidx_t lidx, const char *value);
void      ilistSetList (list_t *, ilistidx_t ikey, ilistidx_t lidx, void *value);
void      ilistSetData (list_t *, ilistidx_t ikey, ilistidx_t lidx, void *value);
void      ilistSetNum (list_t *, ilistidx_t ikey, ilistidx_t lidx, ilistidx_t value);
void      ilistSetDouble (list_t *, ilistidx_t ikey, ilistidx_t lidx, double value);
  /* get routines */
bool      ilistExists (list_t *, ilistidx_t ikey);
void      *ilistGetData (list_t *, ilistidx_t ikey, ilistidx_t lidx);
char      *ilistGetStr (list_t *, ilistidx_t ikey, ilistidx_t lidx);
ilistidx_t   ilistGetNum (list_t *, ilistidx_t ikey, ilistidx_t lidx);
double    ilistGetDouble (list_t *, ilistidx_t ikey, ilistidx_t lidx);
slist_t   *ilistGetList (list_t *, ilistidx_t ikey, ilistidx_t lidx);
void      ilistDelete (list_t *, ilistidx_t ikey);
  /* iterators */
void      ilistStartIterator (ilist_t *list, ilistidx_t *idx);
ilistidx_t ilistIterateKey (ilist_t *list, ilistidx_t *idx);
  /* aux routines */
void      ilistSort (ilist_t *);
void      ilistDumpInfo (ilist_t *list);

#endif /* INC_ILIST_H */


