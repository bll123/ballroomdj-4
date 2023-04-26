/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_SONGLISTUTIL_H
#define INC_SONGLISTUTIL_H

#include "musicdb.h"
#include "nlist.h"

void songlistutilCreateFromList (musicdb_t *musicdb, const char *fname, nlist_t *dbidxlist);

#endif /* INC_SONGLISTUTIL_H */
