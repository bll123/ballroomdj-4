#ifndef INC_DNCTYPES_H
#define INC_DNCTYPES_H

#include "datafile.h"

datafile_t *  dnctypesAlloc (char *);
void          dnctypesFree (datafile_t *);
list_t        * dnctypesGetList (datafile_t *df);
void          dnctypesConv (char *keydata, datafileret_t *ret);

#endif /* INC_DNCTYPES_H */
