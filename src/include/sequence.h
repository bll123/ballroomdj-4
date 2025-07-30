/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_SEQUENCE_H
#define INC_SEQUENCE_H

// #include "datafile.h"
#include "nlist.h"
#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct sequence sequence_t;

[[nodiscard]] sequence_t    *sequenceLoad (const char *fname);
[[nodiscard]] sequence_t    *sequenceCreate (const char *fname);
void          sequenceFree (sequence_t *sequence);
bool          sequenceExists (const char *name);
int32_t       sequenceGetCount (sequence_t *sequence);
nlist_t       *sequenceGetDanceList (sequence_t *sequence);
void          sequenceStartIterator (sequence_t *sequence, nlistidx_t *idx);
nlistidx_t    sequenceIterate (sequence_t *sequence, nlistidx_t *idx);
void          sequenceSave (sequence_t *sequence, slist_t *slist);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_SEQUENCE_H */
