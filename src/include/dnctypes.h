/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "nodiscard.h"
#include "datafile.h"
#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct dnctype dnctype_t;

NODISCARD dnctype_t   *dnctypesAlloc (void);
void        dnctypesFree (dnctype_t *);
void        dnctypesStartIterator (dnctype_t *dnctypes, slistidx_t *iteridx);
const char  *dnctypesIterate (dnctype_t *dnctypes, slistidx_t *iteridx);
void        dnctypesConv (datafileconv_t *conv);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

