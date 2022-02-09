#ifndef INC_SONGSEL_H
#define INC_SONGSEL_H

#include "song.h"
#include "nlist.h"
#include "musicdb.h"
#include "queue.h"

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
  nlist_t              *danceSelList;
  double              autoselWeight [SONGSEL_ATTR_MAX];
  songselsongdata_t   *lastSelection;
} songsel_t;

typedef bool (*songselFilter_t)(dbidx_t, song_t *, void *);

songsel_t * songselAlloc (nlist_t *dancelist,
              songselFilter_t filterProc, void *userdata);
void      songselFree (songsel_t *songsel);
song_t    * songselSelect (songsel_t *songsel, listidx_t danceIdx);
void      songselSelectFinalize (songsel_t *songsel, listidx_t danceIdx);

#endif /* INC_SONGSEL_H */
