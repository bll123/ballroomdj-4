#ifndef INC_SEQUENCE_H
#define INC_SEQUENCE_H

#include "datafile.h"
#include "list.h"

typedef struct {
  list_t          *sequence;
} sequence_t;

sequence_t    *sequenceAlloc (char *);
void          sequenceFree (sequence_t *);
list_t        *sequenceGetDanceList (sequence_t *);
void          sequenceStartIterator (sequence_t *sequence);
long          sequenceIterate (sequence_t *sequence);

#endif /* INC_SEQUENCE_H */
