/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_RAFILE_H
#define INC_RAFILE_H

#include <stdio.h>
#include <stdint.h>

typedef int32_t rafileidx_t;

typedef struct rafile rafile_t;

enum {
  RAFILE_NEW  = 0L,
  RAFILE_REC_SIZE = 2048,
  RAFILE_HDR_SIZE = 128,
};
#define RAFILE_LOCK_FN      "rafile"
#define RRN_TO_OFFSET(rrn)  (((rrn) - 1L) * RAFILE_REC_SIZE + RAFILE_HDR_SIZE)

rafile_t *    raOpen (char *fname, int version);
void          raClose (rafile_t *rafile);
int           raWrite (rafile_t *rafile, rafileidx_t rrn, char *data);
int           raClear (rafile_t *rafile, rafileidx_t rrn);
rafileidx_t   raRead (rafile_t *rafile, rafileidx_t rrn, char *data);
rafileidx_t   raGetCount (rafile_t *rafile);
rafileidx_t   raGetNextRRN (rafile_t *rafile);
void          raStartBatch (rafile_t *rafile);
void          raEndBatch (rafile_t *rafile);

/* for debugging only */

rafileidx_t   raGetSize (rafile_t *rafile);
rafileidx_t   raGetVersion (rafile_t *rafile);

#endif /* INC_RAFILE_H */
