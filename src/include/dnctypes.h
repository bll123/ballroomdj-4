/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_DNCTYPES_H
#define INC_DNCTYPES_H

#include "datafile.h"
#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct dnctype dnctype_t;

dnctype_t   *dnctypesAlloc (void);
void        dnctypesFree (dnctype_t *);
void        dnctypesStartIterator (dnctype_t *dnctypes, slistidx_t *iteridx);
const char  *dnctypesIterate (dnctype_t *dnctypes, slistidx_t *iteridx);
void        dnctypesConv (datafileconv_t *conv);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_DNCTYPES_H */
