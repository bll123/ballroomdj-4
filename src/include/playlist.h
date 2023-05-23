/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
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
  PLAYLIST_RATING,                //
  PLAYLIST_STOP_AFTER,            //
  PLAYLIST_STOP_TIME,             //
  PLAYLIST_TYPE,                  //
  PLAYLIST_KEY_MAX,
} playlistkey_t;

typedef enum {
  PLDANCE_DANCE,
  PLDANCE_MPM_LOW,                //
  PLDANCE_MPM_HIGH,               //
  PLDANCE_COUNT,
  PLDANCE_MAXPLAYTIME,            //
  PLDANCE_SELECTED,
  PLDANCE_KEY_MAX,
} pldancekey_t;

typedef enum {
  PLTYPE_NONE,
  PLTYPE_AUTO,
  PLTYPE_SONGLIST,
  PLTYPE_SEQUENCE,
  PLTYPE_ALL,
} pltype_t;

enum {
  PL_LIST_ALL,          // everything, include QueueDance
  PL_LIST_AUTO_SEQ,
  PL_LIST_NORMAL,       // excludes the special QueueDance playlist
  PL_LIST_SEQUENCE,
  PL_LIST_SONGLIST,
  PL_LIST_DIR,          // specified directory
  PL_LIST_MAX,
};

enum {
  PL_VALID_SONG_ATTEMPTS = 40,
};

typedef struct playlist playlist_t;

playlist_t *playlistLoad (const char *name, musicdb_t *musicdb);
playlist_t *playlistCreate (const char *plname, pltype_t type, musicdb_t *musicdb);
void      playlistFree (void *tpl);
void      playlistResetAll (playlist_t *pl);
const char *playlistGetName (playlist_t *pl);
ssize_t   playlistGetConfigNum (playlist_t *pl, playlistkey_t key);
void      playlistSetConfigNum (playlist_t *pl, playlistkey_t key, ssize_t value);
void      playlistSetConfigList (playlist_t *pl, playlistkey_t key, const char *value);
ssize_t   playlistGetDanceNum (playlist_t *pl, ilistidx_t dancekey, pldancekey_t key);
void      playlistSetDanceCount (playlist_t *pl, ilistidx_t dancekey, ssize_t value);
void      playlistSetDanceNum (playlist_t *pl, ilistidx_t danceIdx, pldancekey_t key, ssize_t value);
song_t    *playlistGetNextSong (playlist_t *pl, ssize_t priorCount, danceselQueueLookup_t queueLookupProc, void *userdata);
slist_t   *playlistGetPlaylistList (int flag, const char *dir);
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

#endif /* INC_PLAYLIST_H */
