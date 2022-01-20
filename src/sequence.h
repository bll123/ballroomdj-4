#ifndef INC_SEQUENCE_H
#define INC_SEQUENCE_H

#include "datafile.h"
#include "nlist.h"

typedef struct {
  nlist_t          *sequence;
} sequence_t;

sequence_t    *sequenceAlloc (char *);
void          sequenceFree (sequence_t *);
nlist_t       *sequenceGetDanceList (sequence_t *);
void          sequenceStartIterator (sequence_t *sequence);
nlistidx_t    sequenceIterate (sequence_t *sequence);

#endif /* INC_SEQUENCE_H */
