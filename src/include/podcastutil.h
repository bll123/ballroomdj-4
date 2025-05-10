/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_PODCASTUTIL_H
#define INC_PODCASTUTIL_H

#include "musicdb.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

void podcastutilApplyRetain (musicdb_t *musicdb, const char *plname);
void podcastutilDelete (musicdb_t *musicdb, const char *plname);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_PODCASTUTIL_H */
