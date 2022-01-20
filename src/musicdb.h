#ifndef INC_MUSICDB_H
#define INC_MUSICDB_H

#include "song.h"

typedef ssize_t   dbidx_t;

typedef struct {
  ssize_t       count;
  list_t        *songs;
  ssize_t       *danceCounts;  // used by main for automatic playlists
  ssize_t       danceCount;
} db_t;

#define MUSICDB_FNAME   "musicdb"
#define MUSICDB_EXT     ".dat"

void    dbOpen (char *);
void    dbClose (void);
size_t  dbCount (void);
int     dbLoad (db_t *, char *);
song_t  *dbGetByName (char *);
song_t  *dbGetByIdx (dbidx_t idx);
void    dbStartIterator (void);
song_t  *dbIterate (dbidx_t *idx);

#endif /* INC_MUSICDB_H */
