#ifndef INC_PLAYLIST_H
#define INC_PLAYLIST_H

#include <stdbool.h>

#include "datafile.h"
#include "song.h"
#include "songlist.h"
#include "sequence.h"
#include "songsel.h"

typedef enum {
  PLAYLIST_ALLOWED_KEYWORDS,
  PLAYLIST_ANNOUNCE,
  PLAYLIST_DANCEDF,             // used internally
  PLAYLIST_DANCES,              // used internally
  PLAYLIST_GAP,
  PLAYLIST_LEVEL_HIGH,
  PLAYLIST_LEVEL_LOW,
  PLAYLIST_MANUAL_LIST_NAME,
  PLAYLIST_MAX_PLAY_TIME,
  PLAYLIST_MQ_MESSAGE,
  PLAYLIST_PAUSE_EACH_SONG,
  PLAYLIST_RATING,
  PLAYLIST_REQ_KEYWORDS,
  PLAYLIST_RESUME,
  PLAYLIST_SEQ_NAME,
  PLAYLIST_STOP_AFTER,
  PLAYLIST_STOP_AFTER_WAIT,
  PLAYLIST_STOP_TIME,
  PLAYLIST_STOP_TYPE,
  PLAYLIST_STOP_WAIT,
  PLAYLIST_TYPE,
  PLAYLIST_USE_STATUS,
  PLAYLIST_USE_UNRATED,
  PLAYLIST_KEY_MAX,
} playlistkey_t;

typedef enum {
  PLDANCE_DANCE,
  PLDANCE_SELECTED,
  PLDANCE_COUNT,
  PLDANCE_MAXPLAYTIME,
  PLDANCE_LOWBPM,
  PLDANCE_HIGHBPM,
  PLDANCE_KEY_MAX,
} pldancekey_t;

typedef enum {
  PLTYPE_AUTO,
  PLTYPE_MANUAL,
  PLTYPE_SEQ,
} pltype_t;

typedef enum {
  RESUME_FROM_START,
  RESUME_FROM_LAST,
} plresume_t;

typedef enum {
  WAIT_CONTINUE,
  WAIT_PAUSE,
} plwait_t;

typedef enum {
  STOP_IN,
  STOP_AT,
} plstoptype_t;

typedef struct {
  datafile_t    *plinfodf;
  datafile_t    *pldancesdf;
  songlist_t    *songlist;
  sequence_t    *sequence;
  songsel_t     *songsel;
  list_t        *plinfo;
  list_t        *pldances;
  int           manualIdx;
} playlist_t;

playlist_t    *playlistAlloc (char *);
void          playlistFree (playlist_t *);
song_t        *playlistGetNextSong (playlist_t *);
bool          playlistFilterSong (song_t *song, void *tplaylist);

#endif /* INC_PLAYLIST_H */
