/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "musicdb.h"
#include "nlist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

void songlistutilCreateFromList (musicdb_t *musicdb, const char *fname, nlist_t *dbidxlist);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

