#ifndef _INC_SONG
#define _INC_SONG

#include "list.h"

typedef struct {
  list_t      *songInfo;
} song_t;

song_t *  songAlloc (void);
void      songFree (song_t *);
void      songSet (song_t *, char *[], int);

#endif /* _INC_SONG */
