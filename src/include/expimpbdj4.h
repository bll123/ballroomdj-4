/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "nodiscard.h"
#include "musicdb.h"
#include "nlist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  EIBDJ4_EXPORT,
  EIBDJ4_IMPORT,
};

typedef struct eibdj4 eibdj4_t;

NODISCARD eibdj4_t *eibdj4Init (musicdb_t *musicdb, const char *dirname, int eiflag);
void eibdj4Free (eibdj4_t *eibdj4);
void eibdj4SetDBIdxList (eibdj4_t *eibdj4, nlist_t *dbidxlist);
void eibdj4SetPlaylist (eibdj4_t *eibdj4, const char *name);
void eibdj4SetNewName (eibdj4_t *eibdj4, const char *name);
void eibdj4GetCount (eibdj4_t *eibdj4, int *count, int *tot);
bool eibdj4Process (eibdj4_t *eibdj4);
bool eibdj4DatabaseChanged (eibdj4_t *eibdj4);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

