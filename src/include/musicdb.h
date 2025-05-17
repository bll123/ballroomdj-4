/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_MUSICDB_H
#define INC_MUSICDB_H

#include <stdint.h>

#include "rafile.h"
#include "song.h"
#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef int32_t dbidx_t;

typedef struct musicdb musicdb_t;

/* music db flags */
enum {
  MUSICDB_STD,
  /* temporarily added to the database, used for external requests */
  MUSICDB_TEMP,
  /* permanently removed */
  MUSICDB_REMOVED,
  /* marked for removal, can be reverted */
  MUSICDB_REMOVE_MARK,
};

enum {
  MUSICDB_VERSION = 10,
  MUSICDB_MAX_SAVE = RAFILE_REC_SIZE,
  MUSICDB_ENTRY_NEW = RAFILE_NEW,
  MUSICDB_ENTRY_UNK = RAFILE_UNKNOWN,
};

#define MUSICDB_FNAME     "musicdb"
#define MUSICDB_TMP_FNAME "musicdb-tmp"
#define MUSICDB_EXT       ".dat"

musicdb_t *dbOpen (const char *);
void      dbClose (musicdb_t *db);
dbidx_t   dbCount (musicdb_t *db);
int       dbLoad (musicdb_t *);
void      dbLoadEntry (musicdb_t *musicdb, dbidx_t dbidx);
void      dbMarkEntryRenamed (musicdb_t *musicdb, const char *olduri, const char *newuri, dbidx_t dbidx);
void      dbMarkEntryRemoved (musicdb_t *musicdb, dbidx_t dbidx);
void      dbClearEntryRemoved (musicdb_t *musicdb, dbidx_t dbidx);
void      dbStartBatch (musicdb_t *db);
void      dbEndBatch (musicdb_t *db);
void      dbDisableLastUpdateTime (musicdb_t *db);
void      dbEnableLastUpdateTime (musicdb_t *db);
bool      dbRemoveSong (musicdb_t *musicdb, dbidx_t dbidx);
rafileidx_t dbWriteSong (musicdb_t *musicdb, song_t *song);
size_t    dbCreateSongEntryFromSong (char *tbuff, size_t sz, song_t *song, const char *fn);
song_t    *dbGetByName (musicdb_t *db, const char *);
song_t    *dbGetByIdx (musicdb_t *db, dbidx_t idx);
void      dbStartIterator (musicdb_t *db, slistidx_t *iteridx);
song_t    *dbIterate (musicdb_t *db, dbidx_t *dbidx, slistidx_t *iteridx);
void      dbBackup (void);
dbidx_t   dbAddTemporarySong (musicdb_t *db, song_t *song);
/* void      dbDumpSongList (musicdb_t *db); */ // for debugging

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_MUSICDB_H */
