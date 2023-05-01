/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_SONGDB_H
#define INC_SONGDB_H

#include "musicdb.h"
#include "song.h"

void  songWriteDB (musicdb_t *musicdb, dbidx_t dbidx);
void  songWriteDBSong (musicdb_t *musicdb, song_t *song);

#endif /* INC_SONGDB_H */
