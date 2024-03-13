/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_SONGDB_H
#define INC_SONGDB_H

#include "musicdb.h"
#include "song.h"

/* the return flags are used by the database update process */
enum {
  SONGDB_NONE                 = 0,
  SONGDB_FORCE_RENAME         = (1 << 0),
  /* output flags */
  SONGDB_RET_REN_FILE_EXISTS  = (1 << 1),
  SONGDB_RET_RENAME_SUCCESS   = (1 << 2),
  SONGDB_RET_RENAME_FAIL      = (1 << 3),
  SONGDB_RET_ORIG_RENAME_FAIL = (1 << 4),
  SONGDB_RET_NULL             = (1 << 5),
  SONGDB_RET_NO_CHANGE        = (1 << 6),
  SONGDB_RET_LOC_LOCK         = (1 << 7),
  SONGDB_RET_SUCCESS          = (1 << 8),
  SONGDB_RET_WRITE_FAIL       = (1 << 9),
  SONGDB_RET_BAD_URI          = (1 << 10),
};

typedef struct songdb songdb_t;

void  songdbWriteDB (songdb_t *songdb, dbidx_t dbidx);
size_t songdbWriteDBSong (songdb_t *songdb, song_t *song, int *flags, dbidx_t rrn);
songdb_t *songdbAlloc (musicdb_t *musicdb);
void  songdbFree (songdb_t *songdb);
void  songdbSetMusicDB (songdb_t *songdb, musicdb_t *musicdb);

#endif /* INC_SONGDB_H */
