#ifndef INC_SONG
#define INC_SONG

#include "list.h"

typedef struct {
  list_t      *songInfo;
} song_t;

song_t *  songAlloc (void);
void      songFree (void *);
char *    songGet (song_t *, char *);
void      songSetAll (song_t *, char *[], size_t);
void      songSetLong (song_t *, char *, long);

#endif /* INC_SONG */
