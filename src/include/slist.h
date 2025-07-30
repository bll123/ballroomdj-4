/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_SLIST_H
#define INC_SLIST_H

#include "list.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef list_t      slist_t;
typedef listidx_t   slistidx_t;
typedef listorder_t slistorder_t;
typedef listFree_t  slistFree_t;

/* keyed by a string */

[[nodiscard]] slist_t   *slistAlloc (const char *name, slistorder_t ordered, slistFree_t valueFreeHook);
void      slistFree (void * list);
void      slistSetVersion (slist_t *list, int version);
int       slistGetVersion (slist_t *list);
slistidx_t slistGetCount (slist_t *list);
void      slistSetSize (slist_t *, slistidx_t);
void      slistSort (slist_t *);
/* set routines */
void      slistSetData (slist_t *, const char *sidx, void *data);
void      slistSetStr (slist_t *, const char *sidx, const char *data);
void      slistSetNum (slist_t *, const char *sidx, listnum_t lval);
void      slistSetList (slist_t *, const char *sidx, slist_t *listval);
void      slistDelete (list_t *list, const char *sidx);
/* get routines */
slistidx_t  slistGetIdx (slist_t *, const char *sidx);
void        *slistGetDataByIdx (slist_t *, slistidx_t idx);
listnum_t   slistGetNumByIdx (slist_t *list, slistidx_t idx);
const char  *slistGetKeyByIdx (slist_t *list, slistidx_t lidx);
void        *slistGetData (slist_t *, const char *sidx);
const char  *slistGetStr (slist_t *, const char *sidx);
listnum_t   slistGetNum (slist_t *, const char *sidx);
slist_t     *slistGetList (slist_t *, const char *sidx);
/* iterators */
void      slistStartIterator (slist_t *list, slistidx_t *idx);
const char *slistIterateKey (slist_t *list, slistidx_t *idx);
void      *slistIterateValueData (slist_t *list, slistidx_t *idx);
listnum_t slistIterateValueNum (slist_t *list, slistidx_t *idx);
/* debug / information routines */
slistidx_t slistGetAllocCount (slist_t *list);
void      slistDumpInfo (slist_t *list);
int       slistGetOrdering (slist_t *list);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_SLIST_H */
