#ifndef INC_SONG
#define INC_SONG

#include "nlist.h"

typedef enum {
  SONG_FAVORITE_NONE,
  SONG_FAVORITE_RED,
  SONG_FAVORITE_ORANGE,
  SONG_FAVORITE_GREEN,
  SONG_FAVORITE_BLUE,
  SONG_FAVORITE_PURPLE,
  SONG_FAVORITE_MAX,
} songfavorite_t;

typedef struct {
  songfavorite_t      idx;
  char                *dispStr;
  char                *color;
} songfavoriteinfo_t;

typedef enum {
  SONG_ADJUST_NONE    = 0x0000,
  SONG_ADJUST_NORM    = 0x0001,
  SONG_ADJUST_TRIM    = 0x0002,
  SONG_ADJUST_SPEED   = 0x0004,
} songadjust_t;

typedef struct {
  nlist_t      *songInfo;
} song_t;

song_t *  songAlloc (void);
void      songFree (void *);
void      songParse (song_t *song, char *data);
char *    songGetData (song_t *, nlistidx_t);
ssize_t   songGetNum (song_t *, nlistidx_t);
songfavoriteinfo_t  * songGetFavoriteData (song_t *);
songfavoriteinfo_t  * songGetFavorite (int value);
double    songGetDouble (song_t *, nlistidx_t);
void      songSetNum (song_t *, nlistidx_t, ssize_t);
void      songSetData (song_t *song, nlistidx_t tagidx, char *str);
void      songChangeFavorite (song_t *song);
bool      songAudioFileExists (song_t *song);

#endif /* INC_SONG */
