/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "nodiscard.h"
#include "autosel.h"
#include "dance.h"
#include "ilist.h"
#include "nlist.h"
#include "queue.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef double  danceProb_t;

typedef ilistidx_t (*danceselQueueLookup_t)(void *userdata, ilistidx_t idx);

typedef struct dancesel dancesel_t;

NODISCARD dancesel_t *danceselAlloc (nlist_t *countList, danceselQueueLookup_t queueLookupProc, void *userdata);
void            danceselFree (dancesel_t *dancesel);
void            danceselDecrementBase (dancesel_t *dancesel, ilistidx_t danceIdx);
void            danceselAddCount (dancesel_t *dancesel, ilistidx_t danceIdx);
void            danceselAddPlayed (dancesel_t *dancesel, ilistidx_t danceIdx);
ilistidx_t      danceselSelect (dancesel_t *dancesel, ilistidx_t priorHistCount);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

