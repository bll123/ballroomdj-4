#ifndef INC_SONGSEL_H
#define INC_SONGSEL_H

#include "ilist.h"
#include "musicdb.h"
#include "nlist.h"
#include "queue.h"
#include "song.h"
#include "songfilter.h"

typedef enum {
  SONGSEL_ATTR_RATING,
  SONGSEL_ATTR_LEVEL,
  SONGSEL_ATTR_MAX,
} songselattr_t;

typedef struct songselidx songselidx_t;

typedef struct songselsongdata songselsongdata_t;

typedef struct songselperc songselperc_t;

typedef struct songseldance songseldance_t;

typedef struct songsel songsel_t;

songsel_t * songselAlloc (musicdb_t *musicdb, nlist_t *dancelist, nlist_t *songlist, songfilter_t *songfilter);
void      songselFree (songsel_t *songsel);
song_t    * songselSelect (songsel_t *songsel, ilistidx_t danceIdx);
void      songselSelectFinalize (songsel_t *songsel, ilistidx_t danceIdx);

#endif /* INC_SONGSEL_H */
