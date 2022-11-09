#ifndef INC_DANCESEL_H
#define INC_DANCESEL_H

#include "bdj4.h"
#include "autosel.h"
#include "dance.h"
#include "ilist.h"
#include "nlist.h"
#include "queue.h"

typedef double  danceProb_t;

typedef struct dancesel dancesel_t;

typedef ilistidx_t (*danceselHistory_t)(void *userdata, ilistidx_t idx);

dancesel_t      *danceselAlloc (nlist_t *countList);
void            danceselFree (dancesel_t *dancesel);
void            danceselDecrementBase (dancesel_t *dancesel, ilistidx_t danceIdx);
void            danceselAddCount (dancesel_t *dancesel, ilistidx_t danceIdx);
void            danceselAddPlayed (dancesel_t *dancesel, ilistidx_t danceIdx);
ilistidx_t      danceselSelect (dancesel_t *dancesel, ssize_t priorHistCount,
                    danceselHistory_t historyProc, void *userdata);

#endif /* INC_DANCESEL_H */
