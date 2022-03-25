#ifndef INC_DNCTYPES_H
#define INC_DNCTYPES_H

#include "datafile.h"
#include "ilist.h"

typedef struct {
  datafile_t  *df;
  ilist_t     *dnctypes;
} dnctype_t;

dnctype_t     *dnctypesAlloc (char *);
void          dnctypesFree (dnctype_t *);
void          dnctypesConv (datafileconv_t *conv);

#endif /* INC_DNCTYPES_H */
