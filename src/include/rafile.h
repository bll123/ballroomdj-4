/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_RAFILE_H
#define INC_RAFILE_H

#include <stdio.h>
#include <stdint.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef int32_t rafileidx_t;

typedef struct rafile rafile_t;

enum {
  RAFILE_NEW  = 0L,
  RAFILE_REC_SIZE = 2048,
  RAFILE_HDR_SIZE = 128,
};
#define RAFILE_LOCK_FN      "rafile"

rafile_t *    raOpen (char *fname, int version);
void          raClose (rafile_t *rafile);
size_t        raWrite (rafile_t *rafile, rafileidx_t rrn, char *data, ssize_t len);
int           raClear (rafile_t *rafile, rafileidx_t rrn);
rafileidx_t   raRead (rafile_t *rafile, rafileidx_t rrn, char *data);
rafileidx_t   raGetCount (rafile_t *rafile);
rafileidx_t   raGetNextRRN (rafile_t *rafile);
void          raStartBatch (rafile_t *rafile);
void          raEndBatch (rafile_t *rafile);

/* for debugging only */

rafileidx_t   raGetSize (rafile_t *rafile);
rafileidx_t   raGetVersion (rafile_t *rafile);
#define RRN_TO_OFFSET(rrn)  (((rrn) - 1L) * RAFILE_REC_SIZE + RAFILE_HDR_SIZE)

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_RAFILE_H */
