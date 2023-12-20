/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "mprisi.h"
#include "mdebug.h"
#include "pli.h"
#include "tmutil.h"
#include "volsink.h"

typedef struct plidata {
  mpris_t           *mpris;
  plistate_t        state;
  int               supported;
  mstime_t          playStart;    // for the null player
} plidata_t;

const char *
pliiDesc (void)
{
  return "MPRIS";
}

plidata_t *
pliiInit (const char *volpkg, const char *sinkname)
{
  plidata_t *pliData;

  pliData = mdmalloc (sizeof (plidata_t));
  pliData->mpris = mprisInit ();
  pliData->state = PLI_STATE_STOPPED;
  pliData->supported = PLI_SUPPORT_NONE;

  return pliData;
}

void
pliiFree (plidata_t *pliData)
{
  if (pliData != NULL) {
    mprisFree (pliData->mpris);
    pliiClose (pliData);
    mdfree (pliData);
  }
}

void
pliiMediaSetup (plidata_t *pliData, const char *mediaPath)
{
  if (pliData != NULL && mediaPath != NULL) {
    mprisMedia (pliData->mpris, mediaPath);
    pliData->state = PLI_STATE_STOPPED;
  }
}

void
pliiStartPlayback (plidata_t *pliData, ssize_t dpos, ssize_t speed)
{
  if (pliData != NULL) {
    mstimestart (&pliData->playStart);
    pliData->state = PLI_STATE_PLAYING;
    pliiSeek (pliData, dpos);
  }
}

void
pliiPause (plidata_t *pliData)
{
  if (pliData != NULL) {
    mprisPause (pliData->mpris);
    pliData->state = PLI_STATE_PAUSED;
  }
}

void
pliiPlay (plidata_t *pliData)
{
  if (pliData != NULL) {
    mprisPlay (pliData->mpris);
    pliData->state = PLI_STATE_PLAYING;
  }
}

void
pliiStop (plidata_t *pliData)
{
  if (pliData != NULL) {
    mprisStop (pliData->mpris);
    pliData->state = PLI_STATE_STOPPED;
  }
}

ssize_t
pliiSeek (plidata_t *pliData, ssize_t pos)
{
  ssize_t     ret = pos;

  if (pliData != NULL) {

  }
  return ret;
}

ssize_t
pliiRate (plidata_t *pliData, ssize_t rate)
{
  ssize_t   ret = 100;

  if (pliData != NULL) {
    ret = 100;
  }
  return ret;
}

void
pliiClose (plidata_t *pliData)
{
  if (pliData != NULL) {
    pliData->state = PLI_STATE_STOPPED;
  }
}

ssize_t
pliiGetDuration (plidata_t *pliData)
{
  ssize_t     duration = 0;

  if (pliData != NULL) {
  }
  return duration;
}

ssize_t
pliiGetTime (plidata_t *pliData)
{
  ssize_t     playTime = 0;

  if (pliData != NULL) {
    playTime = mprisGetPosition (pliData->mpris);
  }

  return playTime;
}

plistate_t
pliiState (plidata_t *pliData)
{
  plistate_t          plistate = PLI_STATE_NONE; /* unknown */

  if (pliData != NULL) {
    plistate = pliData->state;
  }
  return plistate;
}

int
pliiSetAudioDevice (plidata_t *pliData, const char *dev)
{
  return 0;
}

int
pliiAudioDeviceList (plidata_t *pliData, volsinklist_t *sinklist)
{
  return 0;
}

int
pliiSupported (plidata_t *pliData)
{
  return pliData->supported;
}
