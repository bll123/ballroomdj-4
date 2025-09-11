/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "config.h"

# include "pli.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct windata windata_t;

ssize_t           winmpGetDuration (windata_t *windata);
ssize_t           winmpGetTime (windata_t *windata);
int               winmpStop (windata_t *windata);
int               winmpPause (windata_t *windata);
int               winmpPlay (windata_t *windata);
ssize_t           winmpSeek (windata_t *windata, ssize_t dpos);
double            winmpRate (windata_t *windata, double drate);
plistate_t        winmpState (windata_t *windata);
int               winmpMedia (windata_t *windata, const char *fn, int sourceType);
windata_t *       winmpInit (void);
void              winmpClose (windata_t *windata);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif
