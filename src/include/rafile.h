/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "nodiscard.h"
#include <stdio.h>
#include <stdint.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef int32_t rafileidx_t;

typedef struct rafile rafile_t;

enum {
  RAFILE_UNKNOWN  = -1,
  RAFILE_NEW      = 0,
  RAFILE_REC_SIZE = 2048,
  RAFILE_HDR_SIZE = 128,
  RAFILE_RO,
  RAFILE_RW,
};
#define RAFILE_LOCK_FN      "rafile"

NODISCARD rafile_t *    raOpen (const char *fname, int version, int openmode);
void          raClose (rafile_t *rafile);
rafileidx_t   raWrite (rafile_t *rafile, rafileidx_t rrn, char *data, ssize_t len);
bool          raClear (rafile_t *rafile, rafileidx_t rrn);
int           raRead (rafile_t *rafile, rafileidx_t rrn, char *data);
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

