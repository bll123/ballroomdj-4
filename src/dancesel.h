#ifndef INC_DANCESEL_H
#define INC_DANCESEL_H

#include "bdj4.h"
#include "autosel.h"
#include "dance.h"
#include "ilist.h"
#include "nlist.h"
#include "queue.h"

typedef double  danceProb_t;

typedef struct {
  dance_t       *dances;
  autosel_t     *autosel;
  nlist_t       *base;
  double        basetotal;
  nlist_t       *distance;
  ssize_t       maxDistance;
  nlist_t       *selectedCounts;
  double        totalSelCount;
  queue_t       *playedDances;
  nlist_t       *adjustBase;
  ssize_t       selCount;
  nlist_t       *danceProbTable;
  /* autosel variables that will be used */
  ssize_t       histDistance;
  double        tagAdjust;
  double        tagMatch;
  double        logValue;
} dancesel_t;

typedef ilistidx_t (*danceselHistory_t)(void *userdata, ssize_t idx);

dancesel_t      *danceselAlloc (nlist_t *countList);
void            danceselFree (dancesel_t *dancesel);
void            danceselAddCount (dancesel_t *dancesel, ilistidx_t danceIdx);
void            danceselAddPlayed (dancesel_t *dancesel, ilistidx_t danceIdx);
ilistidx_t      danceselSelect (dancesel_t *dancesel, nlist_t *danceCounts,
                    ssize_t priorCount, danceselHistory_t historyProc,
                    void *userdata);

#endif /* INC_DANCESEL_H */
