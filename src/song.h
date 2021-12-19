#ifndef _INC_SONG
#define _INC_SONG

#include "list.h"

typedef struct {
  list_t      *songInfo;
} song_t;

song_t *  songAlloc (void);
void      songFree (void *);
void      songSetAll (song_t *, char *[], int);
void      songSetNumeric (song_t *, char *, long);

#endif /* _INC_SONG */
