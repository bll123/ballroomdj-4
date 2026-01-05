/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "nodiscard.h"
#include "datafile.h"
#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct sortopt sortopt_t;

NODISCARD sortopt_t     *sortoptAlloc (void);
void          sortoptFree (sortopt_t *);
slist_t       *sortoptGetList (sortopt_t *);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

