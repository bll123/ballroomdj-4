/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "nodiscard.h"
#include "musicdb.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct mp3exp mp3exp_t;

BDJ_NODISCARD mp3exp_t *mp3ExportInit (char *msgdata, musicdb_t *musicdb, const char *dirname, int mqidx);
void mp3ExportFree (mp3exp_t *mp3exp);
void mp3ExportGetCount (mp3exp_t *mp3exp, int *count, int *tot);
bool mp3ExportQueue (mp3exp_t *mp3exp);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

