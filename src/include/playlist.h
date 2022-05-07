#ifndef INC_PLAYLIST_H
#define INC_PLAYLIST_H

#include <stdbool.h>

#include "dancesel.h"
#include "datafile.h"
#include "ilist.h"
#include "musicdb.h"
#include "nlist.h"
#include "sequence.h"
#include "slist.h"
#include "song.h"
#include "songfilter.h"
#include "songlist.h"
#include "songsel.h"

typedef enum {
  PLAYLIST_ALLOWED_KEYWORDS,      //
  PLAYLIST_ANNOUNCE,              //
  PLAYLIST_GAP,                   //
  PLAYLIST_LEVEL_HIGH,            //
  PLAYLIST_LEVEL_LOW,             //
  PLAYLIST_MAX_PLAY_TIME,         //
  PLAYLIST_MQ_MESSAGE,
  PLAYLIST_PAUSE_EACH_SONG,
  PLAYLIST_RATING,                //
  PLAYLIST_STOP_AFTER,            //
  PLAYLIST_STOP_TIME,             //
  PLAYLIST_TYPE,                  //
  PLAYLIST_USE_STATUS,            //
  PLAYLIST_KEY_MAX,
} playlistkey_t;

typedef enum {
  PLDANCE_DANCE,
  PLDANCE_BPM_LOW,                //
  PLDANCE_BPM_HIGH,               //
  PLDANCE_COUNT,
  PLDANCE_MAXPLAYTIME,            //
  PLDANCE_SELECTED,
  PLDANCE_KEY_MAX,
} pldancekey_t;

typedef enum {
  PLTYPE_AUTO,
  PLTYPE_MANUAL,
  PLTYPE_SEQ,
} pltype_t;

typedef struct playlist playlist_t;

#define VALID_SONG_ATTEMPTS   40

typedef bool (*playlistCheck_t)(song_t *, void *);

playlist_t    *playlistAlloc (musicdb_t *musicdb);
int           playlistLoad (playlist_t *pl, char *);
void          playlistCreate (playlist_t *pl, char *plfname, pltype_t type, char *ofname);
void          playlistFree (void *);
char          *playlistGetName (playlist_t *pl);
ssize_t       playlistGetConfigNum (playlist_t *pl, playlistkey_t key);
void          playlistSetConfigNum (playlist_t *pl, playlistkey_t key, ssize_t value);
ssize_t       playlistGetDanceNum (playlist_t *pl, ilistidx_t dancekey,
                  pldancekey_t key);
void          playlistSetDanceCount (playlist_t *pl, ilistidx_t dancekey,
                  ssize_t value);
song_t        *playlistGetNextSong (playlist_t *pl,
                  nlist_t *danceCounts,
                  ssize_t priorCount, playlistCheck_t checkProc,
                  danceselHistory_t historyProc, void *userdata);
slist_t       *playlistGetPlaylistList (void);
bool          playlistFilterSong (dbidx_t dbidx, song_t *song,
                  void *tplaylist);
void          playlistAddCount (playlist_t *, song_t *song);
void          playlistAddPlayed (playlist_t *, song_t *song);

#endif /* INC_PLAYLIST_H */
