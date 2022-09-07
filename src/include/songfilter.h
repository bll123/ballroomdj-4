#ifndef INC_SONGFILTER_H
#define INC_SONGFILTER_H

#include <stdbool.h>
#include <sys/types.h>

#include "datafile.h"
#include "musicdb.h"
#include "nlist.h"

enum {
  SONG_FILTER_BPM_HIGH,
  SONG_FILTER_BPM_LOW,
  SONG_FILTER_DANCE_LIST,
  SONG_FILTER_DANCE_IDX,
  SONG_FILTER_FAVORITE,
  SONG_FILTER_GENRE,
  SONG_FILTER_KEYWORD,
  SONG_FILTER_LEVEL_HIGH,
  SONG_FILTER_LEVEL_LOW,
  SONG_FILTER_PLAYLIST,
  SONG_FILTER_RATING,
  SONG_FILTER_SEARCH,
  SONG_FILTER_STATUS,
  SONG_FILTER_STATUS_PLAYABLE,
  SONG_FILTER_MAX,
};

enum {
  FILTER_DISP_GENRE,
  FILTER_DISP_DANCELEVEL,
  FILTER_DISP_STATUS,
  FILTER_DISP_FAVORITE,
  FILTER_DISP_STATUSPLAYABLE,
  FILTER_DISP_MAX,
};

typedef enum {
  SONG_FILTER_FOR_SELECTION = 0,
  SONG_FILTER_FOR_PLAYBACK = 1,
} songfilterpb_t;

typedef struct songfilter songfilter_t;
extern datafilekey_t filterdisplaydfkeys [FILTER_DISP_MAX];

songfilter_t  * songfilterAlloc (void);
void          songfilterFree (songfilter_t *sf);
void          songfilterReset (songfilter_t *sf);
bool          songfilterCheckSelection (songfilter_t *sf, int type);
bool          songfilterIsChanged (songfilter_t *sf, time_t tm);
void          songfilterSetSort (songfilter_t *sf, char *sortselection);
void          songfilterClear (songfilter_t *sf, int filterType);
bool          songfilterInUse (songfilter_t *sf, int filterType);
void          songfilterOff (songfilter_t *sf, int filterType);
void          songfilterOn (songfilter_t *sf, int filterType);
void          songfilterSetData (songfilter_t *sf, int filterType, void *value);
void          songfilterSetNum (songfilter_t *sf, int filterType, ssize_t value);
void          songfilterDanceSet (songfilter_t *sf, ilistidx_t danceIdx,
                  int filterType, ssize_t value);
dbidx_t       songfilterProcess (songfilter_t *sf, musicdb_t *musicdb);
bool          songfilterFilterSong (songfilter_t *sf, song_t *song);
dbidx_t       songfilterGetByIdx (songfilter_t *sf, nlistidx_t lookupIdx);
char *        songfilterGetSort (songfilter_t *sf);
dbidx_t       songfilterGetCount (songfilter_t *sf);

#endif /* INC_SONGFILTER_H */
