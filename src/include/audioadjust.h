/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_AUDIOADJUST_H
#define INC_AUDIOADJUST_H

#include "nodiscard.h"
#include "musicdb.h"
#include "nlist.h"
#include "song.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  AA_NO_FADEIN = -1,
  AA_NO_FADEOUT = -1,
  AA_NO_GAP = -1,
};

typedef struct aa aa_t;

NODISCARD aa_t * aaAlloc (void);
void aaFree (aa_t *aa);
bool aaApplyAdjustments (musicdb_t *musicdb, dbidx_t dbidx, int aaflags);
void aaAdjust (musicdb_t *musicdb, song_t *song, const char *infn, const char *outfn, long dur, int fadein, int fadeout, int gap);
int aaSilenceDetect (const char *infn, double *sstart, double *send);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_AUDIOADJUST_H */
