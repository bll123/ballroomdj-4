#ifndef INC_MUSICDB_H
#define INC_MUSICDB_H

#include <stdint.h>

#include "rafile.h"
#include "song.h"
#include "slist.h"

typedef int32_t dbidx_t;

typedef struct musicdb musicdb_t;

#define MUSICDB_VERSION   10
#define MUSICDB_FNAME     "musicdb"
#define MUSICDB_TMP_FNAME "musicdb-tmp"
#define MUSICDB_EXT       ".dat"
#define MUSICDB_ENTRY_NEW RAFILE_NEW

musicdb_t *dbOpen (char *);
void      dbClose (musicdb_t *db);
dbidx_t   dbCount (musicdb_t *db);
int       dbLoad (musicdb_t *);
void      dbLoadEntry (musicdb_t *musicdb, dbidx_t dbidx);
void      dbStartBatch (musicdb_t *db);
void      dbEndBatch (musicdb_t *db);
size_t    dbWriteSong (musicdb_t *musicdb, song_t *song);
size_t    dbWrite (musicdb_t *db, const char *fn, slist_t *tagList, dbidx_t rrn);
song_t    *dbGetByName (musicdb_t *db, const char *);
song_t    *dbGetByIdx (musicdb_t *db, dbidx_t idx);
void      dbStartIterator (musicdb_t *db, slistidx_t *iteridx);
song_t    *dbIterate (musicdb_t *db, dbidx_t *dbidx, slistidx_t *iteridx);
nlist_t   *dbGetDanceCounts (musicdb_t *db);
void      dbBackup (void);

#endif /* INC_MUSICDB_H */
