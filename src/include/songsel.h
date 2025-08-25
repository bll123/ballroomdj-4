/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "nodiscard.h"
#include "ilist.h"
#include "musicdb.h"
#include "nlist.h"
#include "slist.h"
#include "song.h"
#include "songfilter.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct songsel songsel_t;

NODISCARD songsel_t * songselAlloc (musicdb_t *musicdb, nlist_t *dancelist);
void      songselFree (songsel_t *songsel);
void      songselSetTags (songsel_t *songsel, slist_t *taglist, int tagweight);
void      songselInitialize (songsel_t *songsel, nlist_t *songlist, songfilter_t *songfilter);
song_t    * songselSelect (songsel_t *songsel, ilistidx_t danceIdx);
void      songselSelectFinalize (songsel_t *songsel, ilistidx_t danceIdx);
void      songselFinalizeByIndex (songsel_t *songsel, ilistidx_t danceIdx, dbidx_t dbidx);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

