#ifndef INC_MUSICQ_H
#define INC_MUSICQ_H

#include "queue.h"

typedef struct {
  queue_t         *q;
} musicq_t;

musicq_t *   musicqAlloc (void);
void         musicqFree (musicq_t *q);

#endif /* INC_MUSICQ_H */
