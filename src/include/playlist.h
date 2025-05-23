/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_PLAYLIST_H
#define INC_PLAYLIST_H

#include <stdbool.h>

#include "dancesel.h"
#include "datafile.h"
#include "grouping.h"
#include "ilist.h"
#include "musicdb.h"
#include "nlist.h"
#include "sequence.h"
#include "slist.h"
#include "song.h"
#include "songfilter.h"
#include "songlist.h"
#include "songsel.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  PLAYLIST_ALLOWED_KEYWORDS,
  PLAYLIST_ANNOUNCE,
  PLAYLIST_GAP,
  PLAYLIST_LEVEL_HIGH,
  PLAYLIST_LEVEL_LOW,
  PLAYLIST_MAX_PLAY_TIME,
  PLAYLIST_RATING,
  PLAYLIST_STOP_AFTER,
  PLAYLIST_STOP_TIME,
  PLAYLIST_TAGS,
  PLAYLIST_TAG_WEIGHT,
  PLAYLIST_TYPE,
  PLAYLIST_KEY_MAX,
} playlistkey_t;

typedef enum {
  PLDANCE_DANCE,
  PLDANCE_MPM_LOW,
  PLDANCE_MPM_HIGH,
  PLDANCE_COUNT,
  PLDANCE_MAXPLAYTIME,
  PLDANCE_SELECTED,
  PLDANCE_KEY_MAX,
} pldancekey_t;

typedef enum {
  PLTYPE_NONE,
  PLTYPE_AUTO,
  PLTYPE_SONGLIST,
  PLTYPE_SEQUENCE,
  PLTYPE_PODCAST,
  PLTYPE_MAX,
} pltype_t;

enum {
  PL_LIST_ALL,          // include QueueDance, exclude History
  PL_LIST_AUTO_SEQ,     // exclude QueueDance, include automatic and sequence
  PL_LIST_NORMAL,       // exclude QueueDance, History
  PL_LIST_SEQUENCE,
  PL_LIST_SONGLIST,     // will include History
  PL_LIST_DIR,          // specified directory
  PL_LIST_PODCAST,
  PL_LIST_MAX,
};

enum {
  PL_VALID_SONG_ATTEMPTS = 40,
  /* negative one-tenth of a second for the spinbox */
  PL_GAP_DEFAULT = -100,
};

typedef struct playlist playlist_t;

playlist_t *playlistLoad (const char *name, musicdb_t *musicdb, grouping_t *grouping);
bool      playlistCheck (playlist_t *pl);
playlist_t *playlistCreate (const char *plname, pltype_t type, musicdb_t *musicdb, grouping_t *grouping);
void      playlistFree (void *tpl);
void      playlistResetAll (playlist_t *pl);
void      playlistSetName (playlist_t *pl, const char *newname);
const char *playlistGetName (playlist_t *pl);
ssize_t   playlistGetConfigNum (playlist_t *pl, playlistkey_t key);
void      playlistGetConfigListStr (playlist_t *pl, playlistkey_t key, char *buff, size_t sz);
void      playlistSetConfigNum (playlist_t *pl, playlistkey_t key, ssize_t value);
void      playlistSetConfigList (playlist_t *pl, playlistkey_t key, const char *value);
ssize_t   playlistGetDanceNum (playlist_t *pl, ilistidx_t dancekey, pldancekey_t key);
void      playlistSetDanceCount (playlist_t *pl, ilistidx_t dancekey, ssize_t value);
void      playlistSetDanceNum (playlist_t *pl, ilistidx_t danceIdx, pldancekey_t key, ssize_t value);
void      playlistSetPodcastNum (playlist_t *pl, ilistidx_t key, ssize_t value);
void      playlistSetPodcastStr (playlist_t *pl, ilistidx_t key, const char *str);
ssize_t   playlistGetPodcastNum (playlist_t *pl, ilistidx_t key);
const char *playlistGetPodcastStr (playlist_t *pl, ilistidx_t key);
song_t    *playlistGetNextSong (playlist_t *pl, ssize_t priorCount, danceselQueueLookup_t queueLookupProc, void *userdata);
slist_t   *playlistGetPlaylistNames (int flag, const char *dir);
bool      playlistFilterSong (dbidx_t dbidx, song_t *song, void *tplaylist);
void      playlistAddCount (playlist_t *, song_t *song);
void      playlistAddPlayed (playlist_t *, song_t *song);
void      playlistSave (playlist_t *, const char *name);
void      playlistSetEditMode (playlist_t *pl, int editmode);
int       playlistGetEditMode (playlist_t *pl);
void      playlistSetSongFilter (playlist_t *pl, songfilter_t *sf);

bool      playlistExists (const char *name);
void      playlistRename (const char *oldname, const char *newname);
void      playlistCheckAndCreate (const char *name, pltype_t pltype);
void      playlistDelete (const char *name);
void      playlistCopy (const char *oldname, const char *newname);
pltype_t  playlistGetType (const char *name);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_PLAYLIST_H */
