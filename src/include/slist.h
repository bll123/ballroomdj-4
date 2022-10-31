#ifndef INC_SLIST_H
#define INC_SLIST_H

#include "list.h"

typedef list_t      slist_t;
typedef listidx_t   slistidx_t;
typedef listorder_t slistorder_t;
typedef listFree_t  slistFree_t;

  /* keyed by a string */
slist_t   *slistAlloc (const char *name, slistorder_t ordered,
              slistFree_t valueFreeHook);
void      slistFree (void * list);
void      slistSetVersion (slist_t *list, int version);
int       slistGetVersion (slist_t *list);
slistidx_t slistGetCount (slist_t *list);
void      slistSetSize (slist_t *, ssize_t);
  /* set routines */
void      slistSetData (slist_t *, const char *sidx, void *data);
void      slistSetStr (slist_t *, const char *sidx, const char *data);
void      slistSetNum (slist_t *, const char *sidx, ssize_t lval);
void      slistSetDouble (slist_t *, const char *sidx, double dval);
void      slistSetList (slist_t *, const char *sidx, slist_t *listval);
  /* get routines */
slistidx_t  slistGetIdx (slist_t *, const char *sidx);
void      *slistGetData (slist_t *, const char *sidx);
char      *slistGetStr (slist_t *, const char *sidx);
void      *slistGetDataByIdx (slist_t *, slistidx_t idx);
ssize_t   slistGetNumByIdx (slist_t *list, slistidx_t idx);
char *    slistGetKeyByIdx (slist_t *list, slistidx_t lidx);
ssize_t   slistGetNum (slist_t *, const char *sidx);
double    slistGetDouble (slist_t *, const char *sidx);
int       slistGetMaxKeyWidth (slist_t *);
int       slistGetMaxDataWidth (slist_t *);
slist_t   *slistGetList (slist_t *, const char *sidx);
  /* iterators */
void      slistStartIterator (slist_t *list, slistidx_t *idx);
char      *slistIterateKey (slist_t *list, slistidx_t *idx);
void      *slistIterateValueData (slist_t *list, slistidx_t *idx);
ssize_t   slistIterateValueNum (slist_t *list, slistidx_t *idx);
  /* aux routines */
listidx_t slistIterateGetIdx (list_t *list, slistidx_t *idx);
void      slistSort (slist_t *);
void      slistDumpInfo (slist_t *list);

#endif /* INC_SLIST_H */
