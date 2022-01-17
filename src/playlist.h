#ifndef INC_PLAYLIST_H
#define INC_PLAYLIST_H

#include <stdbool.h>

#include "datafile.h"
#include "musicdb.h"
#include "song.h"
#include "songlist.h"
#include "sequence.h"
#include "songsel.h"

typedef enum {
  PLAYLIST_ALLOWED_KEYWORDS,      //
  PLAYLIST_ANNOUNCE,              //
  PLAYLIST_GAP,                   //
  PLAYLIST_LEVEL_HIGH,            //
  PLAYLIST_LEVEL_LOW,             //
  PLAYLIST_MANUAL_LIST_NAME,      //
  PLAYLIST_MAX_PLAY_TIME,         //
  PLAYLIST_MQ_MESSAGE,
  PLAYLIST_PAUSE_EACH_SONG,
  PLAYLIST_RATING,                //
  PLAYLIST_SEQ_NAME,              //
  PLAYLIST_TYPE,                  //
  PLAYLIST_USE_STATUS,            //
  PLAYLIST_USE_UNRATED,
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
  char          *name;
  datafile_t    *plinfodf;
  datafile_t    *pldancesdf;
  songlist_t    *songlist;
  sequence_t    *sequence;
  songsel_t     *songsel;
  list_t        *plinfo;
  list_t        *pldances;
  int           manualIdx;
} playlist_t;

#define VALID_SONG_ATTEMPTS   40

typedef bool (*playlistCheck_t)(song_t *, void *);

playlist_t    *playlistAlloc (char *);
void          playlistFree (void *);
char          *playlistGetName (playlist_t *pl);
ssize_t       playlistGetConfigNum (playlist_t *pl, playlistkey_t key);
ssize_t       playlistGetDanceNum (playlist_t *pl, dancekey_t dancekey,
                  pldancekey_t key);
song_t        *playlistGetNextSong (playlist_t *, playlistCheck_t checkProc,
                  void *userdata);
list_t        *playlistGetPlaylistList (void);
bool          playlistFilterSong (dbidx_t dbidx, song_t *song,
                  void *tplaylist);

#endif /* INC_PLAYLIST_H */
