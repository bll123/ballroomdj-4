/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_SEQUENCE_H
#define INC_SEQUENCE_H

#include "datafile.h"
#include "nlist.h"
#include "slist.h"

typedef struct {
  nlist_t  *sequence;
  char     *name;
  char     *path;
} sequence_t;

sequence_t    *sequenceAlloc (const char *fname);
sequence_t    *sequenceCreate (const char *fname);
void          sequenceFree (sequence_t *sequence);
nlist_t       *sequenceGetDanceList (sequence_t *sequence);
void          sequenceStartIterator (sequence_t *sequence, nlistidx_t *idx);
nlistidx_t    sequenceIterate (sequence_t *sequence, nlistidx_t *idx);
void          sequenceSave (sequence_t *sequence, slist_t *slist);

#endif /* INC_SEQUENCE_H */
