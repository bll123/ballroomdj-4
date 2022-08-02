#ifndef INC_SONGFAV_H
#define INC_SONGFAV_H

#include "datafile.h"
#include "tagdef.h"

typedef enum {
  SONG_FAVORITE_NONE,
  SONG_FAVORITE_RED,
  SONG_FAVORITE_ORANGE,
  SONG_FAVORITE_GREEN,
  SONG_FAVORITE_BLUE,
  SONG_FAVORITE_PURPLE,
  SONG_FAVORITE_PINK_HEART,
  SONG_FAVORITE_BROKEN_HEART,
  SONG_FAVORITE_MAX,
} songfavorite_t;

typedef struct songfavoriteinfo {
  songfavorite_t      idx;
  char                *dispStr;
  char                *color;
  char                *spanStr;
} songfavoriteinfo_t;

void songFavoriteInit (void);
void songFavoriteCleanup (void);
void songConvFavorite (datafileconv_t *conv);
songfavoriteinfo_t  * songFavoriteGet (int value);

#endif /* INC_SONGFAV_H */
