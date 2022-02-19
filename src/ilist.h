#ifndef INC_ILIST_H
#define INC_ILIST_H

#include "list.h"
#include "slist.h"

typedef list_t      ilist_t;
typedef listidx_t   ilistidx_t;
typedef listorder_t ilistorder_t;
typedef listFree_t  ilistFree_t;

  /* keyed by a ilistidx_t */
ilist_t   *ilistAlloc (char *name, ilistorder_t, ilistFree_t valueFreeHook);
void      ilistFree (void * list);
ssize_t   ilistGetVersion (ilist_t *list);
ssize_t   ilistGetCount (ilist_t *list);
void      ilistSetSize (ilist_t *, ssize_t);
  /* set routines */
void      ilistSetData (list_t *, ilistidx_t ikey, ilistidx_t lidx, void *value);
void      ilistSetStr (list_t *, ilistidx_t ikey, ilistidx_t lidx, char *value);
void      ilistSetNum (list_t *, ilistidx_t ikey, ilistidx_t lidx, ssize_t value);
void      ilistSetDouble (list_t *, ilistidx_t ikey, ilistidx_t lidx, double value);
void      ilistSetList (list_t *, ilistidx_t ikey, ilistidx_t lidx, slist_t *slist);
  /* get routines */
bool      ilistExists (list_t *, ilistidx_t ikey);
void      *ilistGetData (list_t *, ilistidx_t ikey, ilistidx_t lidx);
char      *ilistGetStr (list_t *, ilistidx_t ikey, ilistidx_t lidx);
ssize_t   ilistGetNum (list_t *, ilistidx_t ikey, ilistidx_t lidx);
double    ilistGetDouble (list_t *, ilistidx_t ikey, ilistidx_t lidx);
slist_t   *ilistGetList (list_t *, ilistidx_t ikey, ilistidx_t lidx);
  /* iterators */
void      ilistStartIterator (ilist_t *list, ilistidx_t *idx);
ilistidx_t ilistIterateKey (ilist_t *list, ilistidx_t *idx);
  /* aux routines */
void      ilistSort (ilist_t *);
void      ilistDumpInfo (ilist_t *list);

#endif /* INC_ILIST_H */


