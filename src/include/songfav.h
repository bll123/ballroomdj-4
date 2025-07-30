/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_SONGFAV_H
#define INC_SONGFAV_H

#include "datafile.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  SONGFAV_NAME,
  SONGFAV_DISPLAY,
  SONGFAV_COLOR,
  SONGFAV_USERSEL,
  SONGFAV_KEY_MAX,
};

enum {
  /* key 0 must be defined as the no-favorite selection in favorites.txt */
  SONG_FAVORITE_NONE = 0,
};

typedef struct songfav songfav_t;

[[nodiscard]] songfav_t *songFavoriteAlloc (void);
void  songFavoriteFree (songfav_t *songfav);
int   songFavoriteGetCount (songfav_t *songfav);
int   songFavoriteGetNextValue (songfav_t *songfav, int value);
const char * songFavoriteGetStr (songfav_t *songfav, ilistidx_t key, int idx);
void  songFavoriteConv (datafileconv_t *conv);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_SONGFAV_H */
