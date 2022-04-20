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
} db_t;

#define MUSICDB_VERSION   10
#define MUSICDB_FNAME     "musicdb"
#define MUSICDB_TMP_FNAME "musicdb-tmp"
#define MUSICDB_EXT       ".dat"

void    dbOpen (char *);
void    dbClose (void);
dbidx_t dbCount (void);
int     dbLoad (db_t *);
void    dbStartBatch (void);
void    dbEndBatch (void);
void    dbWrite (char *fn, slist_t *tagList);
song_t  *dbGetByName (char *);
song_t  *dbGetByIdx (dbidx_t idx);
void    dbStartIterator (slistidx_t *iteridx);
song_t  *dbIterate (dbidx_t *dbidx, slistidx_t *iteridx);
nlist_t *dbGetDanceCounts (void);

#endif /* INC_MUSICDB_H */
