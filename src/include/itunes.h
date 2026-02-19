/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "nodiscard.h"
#include "nlist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  ITUNES_STARS_10,
  ITUNES_STARS_20,
  ITUNES_STARS_30,
  ITUNES_STARS_40,
  ITUNES_STARS_50,
  ITUNES_STARS_60,
  ITUNES_STARS_70,
  ITUNES_STARS_80,
  ITUNES_STARS_90,
  ITUNES_STARS_100,
  ITUNES_STARS_MAX,
};

typedef struct itunes itunes_t;

bool  itunesConfigured (void);
BDJ_NODISCARD itunes_t  *itunesAlloc (void);
void  itunesFree (itunes_t *itunes);
int   itunesGetStars (itunes_t *itunes, int idx);
void  itunesSetStars (itunes_t *itunes, int idx, int value);
void  itunesSaveStars (itunes_t *itunes);
int   itunesGetField (itunes_t *itunes, int idx);
void  itunesSetField (itunes_t *itunes, int idx, int value);
void  itunesSaveFields (itunes_t *itunes);
void  itunesStartIterateAvailFields (itunes_t *itunes);
const char * itunesIterateAvailFields (itunes_t *itunes, int *val);
bool  itunesParse (itunes_t *itunes);
nlist_t * itunesGetSongData (itunes_t *itunes, nlistidx_t idx);
nlist_t * itunesGetSongDataByName (itunes_t *itunes, const char *skey);
nlist_t * itunesGetPlaylistData (itunes_t *itunes, const char *skey);
void  itunesStartIteratePlaylists (itunes_t *itunes);
const char *itunesIteratePlaylists (itunes_t *itunes);


#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

