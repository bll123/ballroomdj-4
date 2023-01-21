/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_AUDIOADJUST_H
#define INC_AUDIOADJUST_H

#include "musicdb.h"
#include "nlist.h"
#include "song.h"

enum {
  AA_TRIMSILENCE_PERIOD,
  AA_TRIMSILENCE_START,
  AA_TRIMSILENCE_THRESHOLD,
  AA_LOUDNORM_TARGET_IL,
  AA_LOUDNORM_TARGET_LRA,
  AA_LOUDNORM_TARGET_TP,
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
void aaNormalize (const char *infn);
void aaApplyAdjustments (song_t *song, const char *infn, const char *outfn, int fadein, int fadeout, long dur, int gap);

#endif /* INC_AUDIOADJUST_H */
