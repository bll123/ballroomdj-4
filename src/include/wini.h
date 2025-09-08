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

ssize_t           winGetDuration (windata_t *windata);
ssize_t           winGetTime (windata_t *windata);
int               winStop (windata_t *windata);
int               winPause (windata_t *windata);
int               winPlay (windata_t *windata);
ssize_t           winSeek (windata_t *windata, ssize_t dpos);
double            winRate (windata_t *windata, double drate);
//const char *      winVersion (windata_t *windata);
plistate_t        winState (windata_t *windata);
int               winMedia (windata_t *windata, const char *fn, int sourceType);
windata_t *       winInit (void);
void              winClose (windata_t *windata);
//void              winRelease (windata_t *windata);
//int               winSetAudioDev (windata_t *windata, const char *dev, int plidevtype);
//int               winGetVolume (windata_t *windata);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif
