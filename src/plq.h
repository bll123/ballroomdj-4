#ifndef INC_PLQ_H
#define INC_PLQ_H

#include "queue.h"
#include "datafile.h"

typedef struct {
  datafile_t      *playlist;
} plqitem_t;

typedef struct {
  queue_t         *q;
} plq_t;

plq_t *   plqAlloc (void);
void      plqFree (plq_t *plq);
void      plqPush (plq_t *plq, char *plname);

#endif /* INC_PLQ_H */
