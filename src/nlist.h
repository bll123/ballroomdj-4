#ifndef INC_NLIST_H
#define INC_NLIST_H

#include "list.h"

typedef list_t      nlist_t;
typedef listidx_t   nlistidx_t;
typedef listorder_t nlistorder_t;
typedef listFree_t  nlistFree_t;

  /* keyed by a nlistidx_t */
nlist_t   *nlistAlloc (char *name, nlistorder_t, nlistFree_t valueFreeHook);
void      nlistFree (void * list);
ssize_t   nlistGetCount (nlist_t *list);
void      nlistSetSize (nlist_t *, ssize_t);
void      nlistSetFreeHook (nlist_t *, nlistFree_t valueFreeHook);
  /* set routines */
nlistidx_t nlistSetData (nlist_t *, nlistidx_t idx, void *data);
nlistidx_t nlistSetStr (nlist_t *, nlistidx_t idx, char *data);
nlistidx_t nlistSetNum (nlist_t *, nlistidx_t idx, ssize_t lval);
nlistidx_t nlistSetDouble (nlist_t *, nlistidx_t idx, double dval);
nlistidx_t nlistSetList (nlist_t *list, nlistidx_t lidx, nlist_t *data);
nlistidx_t nlistIncrement (nlist_t *, nlistidx_t idx);
nlistidx_t nlistDecrement (nlist_t *, nlistidx_t idx);
  /* get routines */
void      *nlistGetData (nlist_t *, nlistidx_t idx);
char      *nlistGetStr (nlist_t *, nlistidx_t idx);
void      *nlistGetDataByIdx (nlist_t *, nlistidx_t idx);
ssize_t   nlistGetNumByIdx (nlist_t *list, nlistidx_t idx);
ssize_t   nlistGetNum (nlist_t *, nlistidx_t idx);
double    nlistGetDouble (nlist_t *, nlistidx_t idx);
nlist_t   *nlistGetList (nlist_t *, nlistidx_t idx);
  /* iterators */
void      nlistStartIterator (nlist_t *list);
nlistidx_t nlistIterateKey (nlist_t *list);
void      *nlistIterateValueData (nlist_t *list);
ssize_t   nlistIterateValueNum (nlist_t *list);
  /* aux routines */
void      nlistSort (nlist_t *);
void      nlistDumpInfo (nlist_t *list);
nlistidx_t nlistSearchProbTable (nlist_t *probTable, double dval);

#endif /* INC_NLIST_H */
