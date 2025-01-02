/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_NLIST_H
#define INC_NLIST_H

#include "list.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef list_t      nlist_t;
typedef listidx_t   nlistidx_t;
typedef listnum_t   nlistnum_t;
typedef listorder_t nlistorder_t;
typedef listFree_t  nlistFree_t;

/* keyed by a nlistidx_t */
nlist_t     *nlistAlloc (const char *name, nlistorder_t, nlistFree_t valueFreeHook);
void        nlistFree (void * list);
nlistidx_t  nlistGetCount (nlist_t *list);
void        nlistSetSize (nlist_t *, nlistidx_t);
void        nlistSetFreeHook (nlist_t *, nlistFree_t valueFreeHook);
void        nlistCalcMaxValueWidth (nlist_t *list);
int         nlistGetMaxValueWidth (nlist_t *);
void        nlistSort (nlist_t *);
/* version */
void        nlistSetVersion (nlist_t *list, int version);
int         nlistGetVersion (nlist_t *list);
/* set routines */
void        nlistSetData (nlist_t *, nlistidx_t lkey, void *data);
void        nlistSetStr (nlist_t *, nlistidx_t lkey, const char *data);
void        nlistSetNum (nlist_t *, nlistidx_t lkey, nlistnum_t lval);
void        nlistSetDouble (nlist_t *, nlistidx_t lkey, double dval);
void        nlistSetList (nlist_t *list, nlistidx_t lkey, nlist_t *data);
void        nlistIncrement (nlist_t *, nlistidx_t lkey);
void        nlistDecrement (nlist_t *, nlistidx_t lkey);
/* get routines */
void        *nlistGetData (nlist_t *, nlistidx_t lkey);
const char  *nlistGetStr (nlist_t *, nlistidx_t lkey);
nlistidx_t  nlistGetIdx (nlist_t *list, nlistidx_t idx);
void        *nlistGetDataByIdx (nlist_t *, nlistidx_t idx);
nlistnum_t  nlistGetNumByIdx (nlist_t *list, nlistidx_t idx);
nlistidx_t  nlistGetKeyByIdx (nlist_t *list, nlistidx_t idx);
nlistnum_t  nlistGetNum (nlist_t *, nlistidx_t idx);
double      nlistGetDouble (nlist_t *, nlistidx_t idx);
nlist_t     *nlistGetList (nlist_t *, nlistidx_t idx);
/* iterators */
void        nlistStartIterator (nlist_t *list, nlistidx_t *idx);
nlistidx_t  nlistIterateKey (nlist_t *list, nlistidx_t *idx);
nlistidx_t  nlistIterateKeyPrevious (nlist_t *list, nlistidx_t *idx);
void        *nlistIterateValueData (nlist_t *list, nlistidx_t *idx);
nlistnum_t  nlistIterateValueNum (nlist_t *list, nlistidx_t *idx);
/* search */
nlistidx_t  nlistSearchProbTable (nlist_t *probTable, double dval);
/* debug / information routines */
void        nlistDumpInfo (nlist_t *list);
bool        nlistDebugIsCached (list_t *list, listidx_t key);
nlistidx_t  nlistGetAllocCount (nlist_t *list);
int         nlistGetOrdering (nlist_t *list);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_NLIST_H */
