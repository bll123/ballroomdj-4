/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_MUSICDB_H
#define INC_MUSICDB_H

#include <stdint.h>

#include "rafile.h"
#include "song.h"
#include "slist.h"

typedef int32_t dbidx_t;

typedef struct musicdb musicdb_t;

enum {
  MUSICDB_VERSION = 10,
};

#define MUSICDB_FNAME     "musicdb"
#define MUSICDB_TMP_FNAME "musicdb-tmp"
#define MUSICDB_EXT       ".dat"
#define MUSICDB_ENTRY_NEW RAFILE_NEW

musicdb_t *dbOpen (const char *);
void      dbClose (musicdb_t *db);
dbidx_t   dbCount (musicdb_t *db);
int       dbLoad (musicdb_t *);
void      dbLoadEntry (musicdb_t *musicdb, dbidx_t dbidx);
void      dbStartBatch (musicdb_t *db);
void      dbEndBatch (musicdb_t *db);
void      dbDisableLastUpdateTime (musicdb_t *db);
void      dbEnableLastUpdateTime (musicdb_t *db);
size_t    dbWriteSong (musicdb_t *musicdb, song_t *song);
size_t    dbWrite (musicdb_t *db, const char *fn, slist_t *tagList, dbidx_t rrn);
size_t    dbCreateSongEntryFromTags (char *tbuff, size_t sz, slist_t *tagList, const char *fn, dbidx_t rrn);
song_t    *dbGetByName (musicdb_t *db, const char *);
song_t    *dbGetByIdx (musicdb_t *db, dbidx_t idx);
void      dbStartIterator (musicdb_t *db, slistidx_t *iteridx);
song_t    *dbIterate (musicdb_t *db, dbidx_t *dbidx, slistidx_t *iteridx);
nlist_t   *dbGetDanceCounts (musicdb_t *db);
void      dbBackup (void);
dbidx_t   dbAddTemporarySong (musicdb_t *db, song_t *song);
/* void      dbDumpSongList (musicdb_t *db); */ // for debugging

#endif /* INC_MUSICDB_H */
