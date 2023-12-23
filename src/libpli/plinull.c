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

#include "mdebug.h"
#include "pli.h"
#include "tmutil.h"
#include "volsink.h"

typedef struct plidata {
  char              *name;
  void              *plData;
  ssize_t           duration;
  ssize_t           playTime;
  plistate_t        state;
  int               supported;
  mstime_t          playStart;    // for the null player
} plidata_t;

void
pliiDesc (char **ret, int max)
{
  int         c = 0;

  if (max < 2) {
    return;
  }

  ret [c++] = "Null Player";
  ret [c++] = NULL;
}

plidata_t *
pliiInit (const char *plinm)
{
  plidata_t *pliData;

  pliData = mdmalloc (sizeof (plidata_t));
  pliData->plData = NULL;
  pliData->duration = 20000;
  pliData->playTime = 0;
  pliData->state = PLI_STATE_STOPPED;
  pliData->name = "null";
  pliData->supported = PLI_SUPPORT_NONE;
  return pliData;
}

void
pliiFree (plidata_t *pliData)
{
  if (pliData != NULL) {
    pliiClose (pliData);
    mdfree (pliData);
  }
}

void
pliiMediaSetup (plidata_t *pliData, const char *mediaPath)
{
  if (pliData != NULL && mediaPath != NULL) {
    pliData->state = PLI_STATE_STOPPED;
    pliData->duration = 20000;
    pliData->playTime = 0;
  }
}

void
pliiStartPlayback (plidata_t *pliData, ssize_t dpos, ssize_t speed)
{
  if (pliData != NULL) {
    pliData->duration = 20000;
    pliData->playTime = 0;
    mstimestart (&pliData->playStart);
    pliData->state = PLI_STATE_PLAYING;
    pliiSeek (pliData, dpos);
  }
}

void
pliiPause (plidata_t *pliData)
{
  if (pliData != NULL) {
    pliData->state = PLI_STATE_PAUSED;
  }
}

void
pliiPlay (plidata_t *pliData)
{
  if (pliData != NULL) {
    pliData->state = PLI_STATE_PLAYING;
  }
}

void
pliiStop (plidata_t *pliData)
{
  if (pliData != NULL) {
    pliData->state = PLI_STATE_STOPPED;
    pliData->duration = 20000;
    pliData->playTime = 0;
  }
}

ssize_t
pliiSeek (plidata_t *pliData, ssize_t pos)
{
  ssize_t     ret = pos;

  if (pliData != NULL) {
    pliData->duration = 20000 - pos;
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
    duration = pliData->duration;
  }
  return duration;
}

ssize_t
pliiGetTime (plidata_t *pliData)
{
  ssize_t     playTime = 0;

  if (pliData != NULL) {
    pliData->playTime = mstimeend (&pliData->playStart);
    playTime = pliData->playTime;
  }
  return playTime;
}

plistate_t
pliiState (plidata_t *pliData)
{
  plistate_t          plistate = PLI_STATE_NONE; /* unknown */

  if (pliData != NULL) {
    pliData->playTime = mstimeend (&pliData->playStart);
    if (pliData->playTime >= pliData->duration) {
      pliData->state = PLI_STATE_STOPPED;
    }
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
