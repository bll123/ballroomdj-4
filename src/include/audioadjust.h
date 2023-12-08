/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_AUDIOADJUST_H
#define INC_AUDIOADJUST_H

#include "musicdb.h"
#include "nlist.h"
#include "song.h"

enum {
  AA_NORMVOL_MAX,
  AA_NORMVOL_TARGET,
  AA_TRIMSILENCE_PERIOD,
  AA_TRIMSILENCE_START,
  AA_TRIMSILENCE_THRESHOLD,
  AA_KEY_MAX,
};

enum {
  AA_NO_FADEIN = -1,
  AA_NO_FADEOUT = -1,
  AA_NO_GAP = -1,
};

typedef struct aa aa_t;

aa_t * aaAlloc (void);
void aaFree (aa_t *aa);
bool aaApplyAdjustments (musicdb_t *musicdb, dbidx_t dbidx, int aaflags);
void aaAdjust (musicdb_t *musicdb, song_t *song, const char *infn, const char *outfn, long dur, int fadein, int fadeout, int gap);

#endif /* INC_AUDIOADJUST_H */
