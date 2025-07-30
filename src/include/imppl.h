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

typedef struct imppl imppl_t;

[[nodiscard]] imppl_t * impplInit (slist_t *songidxlist, int imptype, const char *uri, const char *oplname, const char *plname, int askey);
void impplFree (imppl_t *imppl);
bool impplProcess (imppl_t *imppl);
void impplGetCount (imppl_t *imppl, int *count, int *tot);
int  impplGetType (imppl_t *imppl);
void impplSetDB (imppl_t *imppl, musicdb_t *musicdb);
bool impplIsDBChanged (imppl_t *imppl);
void impplFinalize (imppl_t *imppl);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_IMPPL_H */
