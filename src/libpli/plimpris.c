/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#if __linux__

#include "bdj4.h"
#include "dyintfc.h"
#include "mprisi.h"
#include "mdebug.h"
#include "pli.h"
#include "tmutil.h"
#include "volsink.h"

typedef struct plidata {
  mpris_t           *mpris;
  int               state;
  int               supported;
} plidata_t;

void
pliiDesc (char **ret, int max)
{
  int         c = 0;

  c = mprisGetPlayerList (ret, max);
}

plidata_t *
pliiInit (const char *plinm)
{
  plidata_t *pliData;

  pliData = mdmalloc (sizeof (plidata_t));
  pliData->mpris = mprisInit (plinm);
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
  char        tbuff [MAXPATHLEN];
  const char  *tmp;

  if (pliData != NULL && mediaPath != NULL) {
    tmp = mediaPath;
    if (strncmp (mediaPath, "file://", 7) != 0) {
      snprintf (tbuff, sizeof (tbuff), "file://%s", mediaPath);
      tmp = tbuff;
    }
    mprisMedia (pliData->mpris, tmp);
    pliData->state = PLI_STATE_STOPPED;
  }
}

void
pliiStartPlayback (plidata_t *pliData, ssize_t dpos, ssize_t speed)
{
  if (pliData != NULL) {
    pliData->state = PLI_STATE_PLAYING;
    pliiSeek (pliData, dpos);
    pliiRate (pliData, speed);
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
  ssize_t   ret = pos;

  if (pliData != NULL) {
    mprisSetPosition (pliData->mpris, pos);
  }
  return ret;
}

ssize_t
pliiRate (plidata_t *pliData, ssize_t rate)
{
  double    drate;

  if (pliData != NULL) {
    drate = (double) rate / 100.0;
    mprisSetRate (pliData->mpris, drate);
  }
  return rate;
}

void
pliiClose (plidata_t *pliData)
{
  if (pliData != NULL) {
    mprisStop (pliData->mpris);
    pliData->state = PLI_STATE_STOPPED;
  }
}

ssize_t
pliiGetDuration (plidata_t *pliData)
{
  ssize_t     duration = 0;

  if (pliData != NULL) {
    /* mpris:length from the metadata */
  }
  return duration;
}

ssize_t
pliiGetTime (plidata_t *pliData)
{
  ssize_t     playTime = 0;
  double      dpos;

  if (pliData != NULL) {
    /* mpris:length from the metadata */
    dpos = mprisGetPosition (pliData->mpris);
  }

  return playTime;
}

plistate_t
pliiState (plidata_t *pliData)
{
  plistate_t          plistate = PLI_STATE_NONE; /* unknown */

  if (pliData != NULL) {
    const char  *tstate;

    tstate = mprisPlaybackStatus (pliData->mpris);

    if (strcmp (tstate, "Paused") == 0) {
      plistate = PLI_STATE_PAUSED;
    }
    if (strcmp (tstate, "Stopped") == 0) {
      plistate = PLI_STATE_STOPPED;
    }
    if (strcmp (tstate, "Playing") == 0) {
      plistate = PLI_STATE_STOPPED;
    }
    pliData->state = plistate;
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

#endif /* __linux__ */
