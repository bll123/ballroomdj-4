/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_MP3EXP_H
#define INC_MP3EXP_H

#include "musicdb.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct mp3exp mp3exp_t;

mp3exp_t *mp3ExportInit (char *msgdata, musicdb_t *musicdb, const char *dirname, int mqidx);
void mp3ExportFree (mp3exp_t *mp3exp);
void mp3ExportGetCount (mp3exp_t *mp3exp, int *count, int *tot);
bool mp3ExportQueue (mp3exp_t *mp3exp);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_MP3EXP_H */
