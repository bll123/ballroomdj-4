#ifndef INC_SONG
#define INC_SONG

#include "datafile.h"
#include "nlist.h"
#include "slist.h"
#include "songfav.h"

typedef struct songfavoriteinfo {
  songfavorite_t      idx;
  char                *dispStr;
  char                *color;
  char                *spanStr;
} songfavoriteinfo_t;

typedef enum {
  SONG_ADJUST_NONE    = 0x0000,
  SONG_ADJUST_NORM    = 0x0001,
  SONG_ADJUST_TRIM    = 0x0002,
  SONG_ADJUST_SPEED   = 0x0004,
} songadjust_t;

typedef struct song song_t;

song_t *  songAlloc (void);
void      songFree (void *);
void      songParse (song_t *song, char *data, ssize_t didx);
char *    songGetStr (song_t *, nlistidx_t);
ssize_t   songGetNum (song_t *, nlistidx_t);
songfavoriteinfo_t  * songGetFavoriteData (song_t *);
songfavoriteinfo_t  * songGetFavorite (int value);
double    songGetDouble (song_t *, nlistidx_t);
slist_t * songGetList (song_t *song, nlistidx_t idx);
void      songSetNum (song_t *, nlistidx_t, ssize_t val);
void      songSetDouble (song_t *, nlistidx_t, double val);
void      songSetStr (song_t *song, nlistidx_t tagidx, const char *str);
void      songChangeFavorite (song_t *song);
bool      songAudioFileExists (song_t *song);
void      songConvFavorite (datafileconv_t *conv);
char *    songDisplayString (song_t *song, int tagidx);
slist_t * songTagList (song_t *song);
void      songSetDurCache (song_t *song, long dur);
long      songGetDurCache (song_t *song);

#endif /* INC_SONG */
