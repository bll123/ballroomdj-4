/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_AUDIOADJUST_H
#define INC_AUDIOADJUST_H

#include "musicdb.h"
#include "nlist.h"

enum {
  AA_TRIMSILENCE_PERIOD,
  AA_TRIMSILENCE_START,
  AA_TRIMSILENCE_THRESHOLD,
  AA_LOUDNORM_TARGET_IL,
  AA_LOUDNORM_TARGET_LRA,
  AA_LOUDNORM_TARGET_TP,
  AA_KEY_MAX,
};

typedef struct aa aa_t;

aa_t * aaAlloc (void);
void aaFree (aa_t *aa);
void aaNormalize (const char *ffn);
void aaConvert (const char *ffn, const char *outfn);
void aaExportMP3 (musicdb_t *musicdb, nlist_t *songlist, const char *outdir);

#endif /* INC_AUDIOADJUST_H */
