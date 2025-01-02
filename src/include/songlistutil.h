/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_SONGLISTUTIL_H
#define INC_SONGLISTUTIL_H

#include "musicdb.h"
#include "nlist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

void songlistutilCreateFromList (musicdb_t *musicdb, const char *fname, nlist_t *dbidxlist);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_SONGLISTUTIL_H */
