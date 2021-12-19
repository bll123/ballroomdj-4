#ifndef _INC_MUSICDB_H
#define _INC_MUSICDB_H

#include "song.h"

typedef struct {
  size_t      count;
  list_t      *songs;
} db_t;

void    dbOpen (char *);
void    dbClose (void);
int     dbLoad (db_t *, char *);

#endif /* _INC_MUSICDB_H */
