#ifndef INC_SONGSEL_H
#define INC_SONGSEL_H

#include "song.h"
#include "list.h"

typedef struct {
  size_t        dbidx;
  long          ratingIdx;
  long          levelIdx;
  double        percentage;
} songselsong_t;

  /* used for both ratings and levels */
typedef struct {
  size_t        origCount;      // count of number of songs for this idx
  long          weight;         // weight for this idx
  size_t        count;          // current count of number of songs
  double        calcperc;       // current percentage adjusted by weight
} songselperc_t;

typedef struct {
  list_t      *songIdxList;
  list_t      *currIdxList;
  list_t      *ratingList;
  list_t      *levelList;
} songseldance_t;

typedef struct {
  list_t    *danceSelList;
} songsel_t;

songsel_t     * songselAlloc (list_t *dancelist);
void          songselFree (songsel_t *songsel);
song_t        * songselSelect (songsel_t *songsel, long danceIdx);

#endif /* INC_SONGSEL_H */
