#ifndef INC_PLQ_H
#define INC_PLQ_H

#include "queue.h"
#include "playlist.h"

typedef struct {
  playlist_t      *playlist;
} plqitem_t;

typedef struct {
  queue_t         *q;
} plq_t;

plq_t       *plqAlloc (void);
void        plqFree (plq_t *plq);
void        plqPush (plq_t *plq, char *plname);
void        plqPop (plq_t *plq);
playlist_t  *plqGetCurrent (plq_t *plq);

#endif /* INC_PLQ_H */
