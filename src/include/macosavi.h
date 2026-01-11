/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "config.h"

#include "pli.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct macosav macosav_t;

macosav_t *       macosavInit (void);
void              macosavClose (macosav_t *macosav);
int               macosavMedia (macosav_t *macosav, const char *fn, int sourceType);
int               macosavCrossFade (macosav_t *macosav, const char *fn, int sourceType);
void              macosavCrossFadeVolume (macosav_t *macosav, int vol);
int               macosavPlay (macosav_t *macosav);
int               macosavPause (macosav_t *macosav);
int               macosavStop (macosav_t *macosav);
ssize_t           macosavGetDuration (macosav_t *macosav);
ssize_t           macosavGetTime (macosav_t *macosav);
plistate_t        macosavState (macosav_t *macosav);
ssize_t           macosavSeek (macosav_t *macosav, ssize_t pos);
double            macosavRate (macosav_t *macosav, double drate);
int               macosavGetVolume (macosav_t *macosav);
int               macosavSetAudioDevice (macosav_t *macosav, const char *dev, plidev_t plidevtype);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

