#ifndef INC_MUSICDB_H
#define INC_MUSICDB_H

#include "song.h"

typedef struct {
  size_t      count;
  list_t      *songs;
} db_t;

#define MUSICDB_FNAME   "musicdb"
#define MUSICDB_EXT     ".dat"

void    dbOpen (char *);
void    dbClose (void);
size_t  dbCount (void);
int     dbLoad (db_t *, char *);
song_t  *dbGetByName (char *);
song_t  *dbGetByIdx (size_t idx);
void    dbStartIterator (void);
song_t  *dbIterate (size_t *idx);

#endif /* INC_MUSICDB_H */
