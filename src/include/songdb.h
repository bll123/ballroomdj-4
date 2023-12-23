/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_SONGDB_H
#define INC_SONGDB_H

#include "musicdb.h"
#include "song.h"

/* the return flags are used by the database update process */
enum {
  SONGDB_NONE             = 0x0000,
  SONGDB_FORCE_RENAME     = 0x0001,
  /* output flags */
  SONGDB_RET_REN_FILE_EXISTS  = 0x0002,
  SONGDB_RET_RENAME_SUCCESS   = 0x0004,
  SONGDB_RET_RENAME_FAIL      = 0x0008,
  SONGDB_RET_ORIG_RENAME_FAIL = 0x0010,
  SONGDB_RET_NULL             = 0x0020,
  SONGDB_RET_NO_CHANGE        = 0x0040,
  SONGDB_RET_LOC_LOCK         = 0x0080,
  SONGDB_RET_SUCCESS          = 0x0100,
};

typedef struct songdb songdb_t;

void  songdbWriteDB (songdb_t *songdb, dbidx_t dbidx);
size_t songdbWriteDBSong (songdb_t *songdb, song_t *song, int *flags, dbidx_t rrn);
songdb_t *songdbAlloc (musicdb_t *musicdb);
void  songdbFree (songdb_t *songdb);
void  songdbSetMusicDB (songdb_t *songdb, musicdb_t *musicdb);
bool  songdbNewName (songdb_t *songdb, song_t *song, char *newuri, size_t sz);

#endif /* INC_SONGDB_H */
