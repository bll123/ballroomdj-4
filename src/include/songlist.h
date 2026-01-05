/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "nodiscard.h"
#include "datafile.h"
#include "ilist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  SONGLIST_URI,
  SONGLIST_TITLE,
  SONGLIST_DANCE,
} songlistkey_t;

enum {
  SONGLIST_UPDATE_TIMESTAMP,
  SONGLIST_PRESERVE_TIMESTAMP,
};

enum {
  SONGLIST_USE_DIST_VERSION = -3,
};

typedef struct songlist songlist_t;

NODISCARD songlist_t * songlistCreate (const char *fname);
NODISCARD songlist_t * songlistLoad (const char *fname);
void songlistFree (songlist_t *);
bool songlistExists (const char *name);
int  songlistGetCount (songlist_t *);
void songlistStartIterator (songlist_t *sl, ilistidx_t *iteridx);
ilistidx_t songlistIterate (songlist_t *sl, ilistidx_t *iteridx);
ilistidx_t songlistGetNum (songlist_t *sl, ilistidx_t ikey, ilistidx_t lidx);
const char * songlistGetStr (songlist_t *sl, ilistidx_t ikey, ilistidx_t lidx);
void songlistSetNum (songlist_t *sl, ilistidx_t ikey, ilistidx_t lidx, ilistidx_t val);
void songlistSetStr (songlist_t *sl, ilistidx_t ikey, ilistidx_t lidx, const char *sval);
void songlistClear (songlist_t *sl);
void songlistSave (songlist_t *sl, int tmflag, int distvers);
int songlistDistVersion (songlist_t *sl);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

