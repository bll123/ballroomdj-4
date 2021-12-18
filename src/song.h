#ifndef _INC_SONG
#define _INC_SONG

#include "list.h"

enum internalsongtag {
  SONG_INTERNAL_TAG,
  SONG_TAG
};

typedef struct {
  char                  *name;
  char                  *value;
  enum internalsongtag  internal;
} songtag_t;

typedef struct {
  list_t      *songInfo;
} song_t;

song_t *songAlloc (void);
void songFree (song_t *song);

#endif /* _INC_SONG */
