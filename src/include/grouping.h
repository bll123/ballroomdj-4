/*
 * Copyright 2024-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_GROUPING_H
#define INC_GROUPING_H

#include "musicdb.h"
#include "nlist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct grouping grouping_t;

grouping_t *groupingAlloc (musicdb_t *musicdb);
void groupingFree (grouping_t *grouping);
void groupingStartIterator (grouping_t *grouping, nlistidx_t *iteridx);
dbidx_t groupingIterate (grouping_t *grouping, dbidx_t seldbidx, nlistidx_t *iteridx);
int groupingCheck (grouping_t *grouping, dbidx_t seldbidx, dbidx_t chkdbidx);
void groupingRebuild (grouping_t *grouping, musicdb_t *musicdb);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_GROUPING_H */
