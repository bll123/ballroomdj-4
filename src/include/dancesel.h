/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_DANCESEL_H
#define INC_DANCESEL_H

#include "bdj4.h"
#include "autosel.h"
#include "dance.h"
#include "ilist.h"
#include "nlist.h"
#include "queue.h"

typedef double  danceProb_t;

typedef ilistidx_t (*danceselQueueLookup_t)(void *userdata, ilistidx_t idx);

typedef struct dancesel dancesel_t;

dancesel_t      *danceselAlloc (nlist_t *countList,
                    danceselQueueLookup_t queueLookupProc, void *userdata);
void            danceselFree (dancesel_t *dancesel);
void            danceselDecrementBase (dancesel_t *dancesel, ilistidx_t danceIdx);
void            danceselAddCount (dancesel_t *dancesel, ilistidx_t danceIdx);
void            danceselAddPlayed (dancesel_t *dancesel, ilistidx_t danceIdx);
ilistidx_t      danceselSelect (dancesel_t *dancesel, ilistidx_t priorHistCount);

#endif /* INC_DANCESEL_H */
