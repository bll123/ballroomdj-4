#ifndef INC_SONG
#define INC_SONG

#include "list.h"

#define SONG_ADJUST_NONE    0b00000000
#define SONG_ADJUST_NORM    0b00000001
#define SONG_ADJUST_TRIM    0b00000010
#define SONG_ADJUST_SPEED   0b00000100

typedef struct {
  list_t      *songInfo;
} song_t;

song_t *  songAlloc (void);
void      songFree (void *);
void      songParse (song_t *song, char *data);
char *    songGetData (song_t *, long);
long      songGetLong (song_t *, long);
void      songSetLong (song_t *, long, long);

#endif /* INC_SONG */
