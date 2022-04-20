#ifndef INC_DNCTYPES_H
#define INC_DNCTYPES_H

#include "datafile.h"
#include "slist.h"

typedef struct {
  datafile_t  *df;
  slist_t     *dnctypes;
} dnctype_t;

dnctype_t     *dnctypesAlloc (void);
void          dnctypesFree (dnctype_t *);
void          dnctypesStartIterator (dnctype_t *dnctypes, slistidx_t *iteridx);
char          *dnctypesIterate (dnctype_t *dnctypes, slistidx_t *iteridx);
void          dnctypesConv (datafileconv_t *conv);

#endif /* INC_DNCTYPES_H */
