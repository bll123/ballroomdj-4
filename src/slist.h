#ifndef INC_SLIST_H
#define INC_SLIST_H

#include "list.h"

typedef list_t      slist_t;
typedef listidx_t   slistidx_t;
typedef listorder_t slistorder_t;
typedef listFree_t  slistFree_t;

  /* keyed by a string */
slist_t   *slistAlloc (char *name, slistorder_t ordered,
              slistFree_t keyFreeHook, slistFree_t valueFreeHook);
void      slistFree (void * list);
ssize_t   slistGetCount (slist_t *list);
void      slistSetSize (slist_t *, ssize_t);
  /* set routines */
slistidx_t slistSetData (slist_t *, char *sidx, void *data);
slistidx_t slistSetStr (slist_t *, char *sidx, char *data);
slistidx_t slistSetNum (slist_t *, char *sidx, ssize_t lval);
slistidx_t slistSetDouble (slist_t *, char *sidx, double dval);
slistidx_t slistSetList (slist_t *, char *sidx, slist_t *listval);
  /* get routines */
slistidx_t  slistGetIdx (slist_t *, char *sidx);
void      *slistGetData (slist_t *, char *sidx);
char      *slistGetStr (slist_t *, char *sidx);
void      *slistGetDataByIdx (slist_t *, slistidx_t idx);
ssize_t   slistGetNumByIdx (slist_t *list, slistidx_t idx);
ssize_t   slistGetNum (slist_t *, char *sidx);
double    slistGetDouble (slist_t *, char *sidx);
slist_t   *slistGetList (slist_t *, char *sidx);
  /* iterators */
void      slistStartIterator (slist_t *list);
char      *slistIterateKey (slist_t *list);
void      *slistIterateValueData (slist_t *list);
ssize_t   slistIterateValueNum (slist_t *list);
  /* aux routines */
listidx_t slistIterateGetIdx (list_t *list);
void      slistSort (slist_t *);
void      slistDumpInfo (slist_t *list);

#endif /* INC_SLIST_H */
