#ifndef INC_MP3EXP_H
#define INC_MP3EXP_H

#include "musicdb.h"
#include "callback.h"

void mp3ExportQueue (char *msg, musicdb_t *musicdb, const char *dirname, int mqidx, callbackFuncIntInt dispcb);

#endif /* INC_MP3EXP_H */
