/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "musicdb.h"
#include "song.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  PODCAST_KEEP = true,
  PODCAST_DELETE = false,
} pcretain_t;

pcretain_t podcastutilCheckRetain (song_t *song, int retain);
void podcastutilApplyRetain (musicdb_t *musicdb, const char *plname);
void podcastutilDelete (musicdb_t *musicdb, const char *plname);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

