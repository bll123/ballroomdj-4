#ifndef INC_DNCTYPES_H
#define INC_DNCTYPES_H

#include "datafile.h"

typedef struct {
  datafile_t        *df;
} dnctype_t;

dnctype_t     *dnctypesAlloc (char *);
void          dnctypesFree (dnctype_t *);
void          dnctypesConv (char *keydata, datafileret_t *ret);

#endif /* INC_DNCTYPES_H */
