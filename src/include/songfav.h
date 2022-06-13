#ifndef INC_SONGFAV_H
#define INC_SONGFAV_H

typedef enum {
  SONG_FAVORITE_NONE,
  SONG_FAVORITE_RED,
  SONG_FAVORITE_ORANGE,
  SONG_FAVORITE_GREEN,
  SONG_FAVORITE_BLUE,
  SONG_FAVORITE_PURPLE,
  SONG_FAVORITE_MAX,
} songfavorite_t;

void songConvFavorite (datafileconv_t *conv);

#endif /* INC_SONGFAV_H */
