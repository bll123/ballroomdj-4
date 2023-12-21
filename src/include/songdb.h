/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_SONGDB_H
#define INC_SONGDB_H

#include "musicdb.h"
#include "song.h"

void  songWriteDB (musicdb_t *musicdb, dbidx_t dbidx, const char *olduri);
void  songWriteDBSong (musicdb_t *musicdb, song_t *song, const char *olduri);

#endif /* INC_SONGDB_H */
