#ifndef INC_DANCESEL_H
#define INC_DANCESEL_H

#include "bdj4.h"
#include "autosel.h"
#include "dance.h"
#include "nlist.h"
#include "playlist.h"
#include "queue.h"

typedef double  danceProb_t;

typedef struct {
  dance_t       *dances;
  autosel_t     *autosel;
  nlist_t        *base;
  double        basetotal;
  nlist_t        *distance;
  ssize_t       maxDistance;
  nlist_t        *playedCounts;
  queue_t       *playedDances;
  double        totalPlayed;
  nlist_t        *adjustBase;
  ssize_t       selCount;
  nlist_t        *danceProbTable;
} dancesel_t;

typedef ssize_t (*danceselCallback_t)(void *userdata, listidx_t danceKey);

dancesel_t      *danceselAlloc (playlist_t *pl);
void            danceselFree (dancesel_t *dancesel);
void            danceselAddPlayed (dancesel_t *dancesel, listidx_t danceKey);
listidx_t       danceselSelect (dancesel_t *dancesel,
                    danceselCallback_t callback, void *userdata);

#endif /* INC_DANCESEL_H */
