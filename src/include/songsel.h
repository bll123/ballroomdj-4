#ifndef INC_SONGSEL_H
#define INC_SONGSEL_H

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

typedef struct {
  nlistidx_t     idx;
} songselidx_t;

typedef struct {
  nlistidx_t    idx;
  dbidx_t       dbidx;
  nlistidx_t    rating;
  nlistidx_t    level;
  nlistidx_t    attrIdx [SONGSEL_ATTR_MAX];
  double        percentage;
} songselsongdata_t;

  /* used for both ratings and levels */
typedef struct {
  ssize_t       origCount;      // count of number of songs for this idx
  ssize_t       weight;         // weight for this idx
  ssize_t       count;          // current count of number of songs
  double        calcperc;       // current percentage adjusted by weight
} songselperc_t;

typedef struct {
  nlistidx_t  danceIdx;
  nlist_t     *songIdxList;
  queue_t     *currentIndexes;
  nlist_t     *currentIdxList;
  nlist_t     *attrList [SONGSEL_ATTR_MAX];
} songseldance_t;

typedef struct {
  nlist_t             *danceSelList;
  double              autoselWeight [SONGSEL_ATTR_MAX];
  songselsongdata_t   *lastSelection;
  musicdb_t           *musicdb;
} songsel_t;

songsel_t * songselAlloc (musicdb_t *musicdb, nlist_t *dancelist, songfilter_t *songfilter);
void      songselFree (songsel_t *songsel);
song_t    * songselSelect (songsel_t *songsel, listidx_t danceIdx);
void      songselSelectFinalize (songsel_t *songsel, listidx_t danceIdx);

#endif /* INC_SONGSEL_H */
