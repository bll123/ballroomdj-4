#ifndef INC_SEQUENCE_H
#define INC_SEQUENCE_H

#include "datafile.h"

typedef struct {
  datafile_t      *df;
} sequence_t;

sequence_t    *sequenceAlloc (char *);
void          sequenceFree (sequence_t *);

#endif /* INC_SEQUENCE_H */
