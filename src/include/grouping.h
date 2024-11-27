/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
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
int groupingCheck (grouping_t *grouping, dbidx_t seldbidx, dbidx_t chkdbidx);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_GROUPING_H */
