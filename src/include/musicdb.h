#ifndef INC_MUSICDB_H
#define INC_MUSICDB_H

#include <stdint.h>

#include "rafile.h"
#include "song.h"
#include "slist.h"

typedef int32_t dbidx_t;

typedef struct {
  dbidx_t       count;
  nlist_t       *songs;
  nlist_t       *danceCounts;  // used by main for automatic playlists
  dbidx_t       danceCount;
  rafile_t      *radb;
  char          *fn;
} musicdb_t;

#define MUSICDB_VERSION   10
#define MUSICDB_FNAME     "musicdb"
#define MUSICDB_TMP_FNAME "musicdb-tmp"
#define MUSICDB_EXT       ".dat"

musicdb_t *dbOpen (char *);
void      dbClose (musicdb_t *db);
dbidx_t   dbCount (musicdb_t *db);
int       dbLoad (musicdb_t *);
void      dbStartBatch (musicdb_t *db);
void      dbEndBatch (musicdb_t *db);
void      dbWrite (musicdb_t *db, char *fn, slist_t *tagList);
song_t    *dbGetByName (musicdb_t *db, char *);
song_t    *dbGetByIdx (musicdb_t *db, dbidx_t idx);
void      dbStartIterator (musicdb_t *db, slistidx_t *iteridx);
song_t    *dbIterate (musicdb_t *db, dbidx_t *dbidx, slistidx_t *iteridx);
nlist_t   *dbGetDanceCounts (musicdb_t *db);

#endif /* INC_MUSICDB_H */
