#ifndef INC_SONG
#define INC_SONG

#include "list.h"

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
