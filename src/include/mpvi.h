/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_MPVI_H
#define INC_MPVI_H

#include "config.h"

#if _hdr_mpv_client

#include <mpv/client.h>

#include "pli.h"
#include "volsink.h"

typedef struct mpvData mpvData_t;

ssize_t           mpvGetDuration (mpvData_t *mpvData);
ssize_t           mpvGetTime (mpvData_t *mpvData);
int               mpvIsPlaying (mpvData_t *mpvData);
int               mpvIsPaused (mpvData_t *mpvData);
int               mpvStop (mpvData_t *mpvData);
int               mpvPause (mpvData_t *mpvData);
int               mpvPlay (mpvData_t *mpvData);
ssize_t           mpvSeek (mpvData_t *mpvData, ssize_t dpos);
double            mpvRate (mpvData_t *mpvData, double drate);
bool              mpvHaveAudioDevList (void);
int               mpvAudioDevSet (mpvData_t *mpvData, const char *dev);
int               mpvAudioDevList (mpvData_t *mpvData, volsinklist_t *sinklist);
const char *      mpvVersion (mpvData_t *mpvData);
plistate_t        mpvState (mpvData_t *mpvData);
int               mpvMedia (mpvData_t *mpvdata, const char *fn);
mpvData_t *       mpvInit (void);
void              mpvClose (mpvData_t *mpvData);
void              mpvRelease (mpvData_t *mpvData);

#endif /* _hdr_mpv_client */

#endif /* INC_MPVI_H */
