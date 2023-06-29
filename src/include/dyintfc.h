#ifndef INC_DYINTFC_H
#define INC_DYINTFC_H

#include "slist.h"

typedef struct dyiter dyiter_t;

slist_t * dyInterfaceList (const char *pfx, const char *funcnm);

#if 0
dyiter_t  *dyInterfaceStartIterator (const char *pfx, const char *funcnm);
void      *dyInterfaceIterate (dyiter_t *dyiter);
void      dyInterfaceCleanIterator (dyiter_t *dyiter);
#endif

#endif /* INC_DYINTFC_H */
