#ifndef INC_SEQUENCE_H
#define INC_SEQUENCE_H

#include "datafile.h"
#include "nlist.h"

typedef struct {
  nlist_t          *sequence;
} sequence_t;

sequence_t    *sequenceAlloc (const char *fname);
void          sequenceFree (sequence_t *sequence);
nlist_t       *sequenceGetDanceList (sequence_t *sequence);
void          sequenceStartIterator (sequence_t *sequence, nlistidx_t *idx);
nlistidx_t    sequenceIterate (sequence_t *sequence, nlistidx_t *idx);

#endif /* INC_SEQUENCE_H */
