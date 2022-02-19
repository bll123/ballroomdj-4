#ifndef INC_SONGFILTER_H
#define INC_SONGFILTER_H

#include <stdbool.h>

#include "musicdb.h"
#include "nlist.h"
#include "slist.h"

enum {
  SONG_FILTER_BPM_LOW,
  SONG_FILTER_BPM_HIGH,
  SONG_FILTER_DANCE,
  SONG_FILTER_KEYWORD,
  SONG_FILTER_LEVEL_HIGH,
  SONG_FILTER_LEVEL_LOW,
  SONG_FILTER_RATING,
  SONG_FILTER_SEARCH,
  SONG_FILTER_STATUS,
  SONG_FILTER_STATUS_PLAYABLE,
  SONG_FILTER_MAX,
};
#define SONG_FILTER_MAX_DATA 2
#define SONG_FILTER_MAX_NUM 4

typedef enum {
  SONG_FILTER_FOR_PLAYBACK,
  SONG_FILTER_FOR_SELECTION,
} songfilterpb_t;

typedef struct {
  char        *sortselection;
  void        *datafilter [SONG_FILTER_MAX];
  nlistidx_t  numfilter [SONG_FILTER_MAX];
  bool        forplayback;
  bool        inuse [SONG_FILTER_MAX];
  slist_t     *indexList;
} songfilter_t;

songfilter_t  * songfilterAlloc (songfilterpb_t pbflag);
void          songfilterFree (songfilter_t *sf);
void          songfilterReset (songfilter_t *sf);
void          songfilterSetSort (songfilter_t *sf, char *sortselection);
void          songfilterSetData (songfilter_t *sf, int filterType, void *value);
void          songfilterSetNum (songfilter_t *sf, int filterType, ssize_t value);
void          songfilterDanceSet (songfilter_t *sf, ilistidx_t danceIdx,
                  int filterType, ssize_t value);
void          songfilterProcess (songfilter_t *sf);
bool          songfilterFilterSong (songfilter_t *sf, song_t *song);
dbidx_t       songfilterGetByIdx (songfilter_t *sf, nlistidx_t idx);

#endif /* INC_SONGFILTER_H */
