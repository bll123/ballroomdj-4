/*
 * Copyright 2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_IMPPL_H
#define INC_IMPPL_H

#include "musicdb.h"
#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

bool impplPlaylistImport (slist_t *songidxlist, musicdb_t *musicdb, int imptype, const char *uri, const char *oplname, const char *plname, int askey);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_IMPPL_H */
