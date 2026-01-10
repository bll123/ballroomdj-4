/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
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
pliiDesc (const char **ret, int max)
{
  mprisGetPlayerList (NULL, (char **) ret, max);
// ### memory leak: the list is not being freed
}

plidata_t *
pliiInit (const char *plinm, const char *playerargs)
{
  plidata_t *pliData;

  pliData = mdmalloc (sizeof (plidata_t));
  pliData->mpris = mprisInit (plinm);
  pliData->state = PLI_STATE_STOPPED;
  pliData->supported = PLI_SUPPORT_NONE;
  if (mprisCanSeek (pliData->mpris)) {
    pliData->supported |= PLI_SUPPORT_SEEK;
  }
  if (mprisHasSpeed (pliData->mpris)) {
    pliData->supported |= PLI_SUPPORT_SPEED;
    pliData->supported |= PLI_SUPPORT_STREAM_SPD;
  }
  pliData->supported |= PLI_SUPPORT_STREAM;

  return pliData;
}

void
pliiFree (plidata_t *pliData)
{
  if (pliData == NULL) {
    return;
  }

  mprisFree (pliData->mpris);
  pliiClose (pliData);
  mdfree (pliData);
  mprisCleanup ();
}

void
pliiMediaSetup (plidata_t *pliData, const char *mediaPath,
    const char *fullMediaPath, int sourceType)
{
  if (pliData == NULL || mediaPath == NULL) {
    return;
  }

  mprisMedia (pliData->mpris, fullMediaPath, sourceType);
  pliData->state = PLI_STATE_STOPPED;
}

void
pliiCleanup (void)
{
  mprisCleanup ();
  return;
}

void
pliiStartPlayback (plidata_t *pliData, ssize_t dpos, ssize_t speed)
{
  if (pliData == NULL) {
    return;
  }

  pliData->state = PLI_STATE_PLAYING;
  pliiSeek (pliData, dpos);
  pliiRate (pliData, speed);
}

void
pliiPause (plidata_t *pliData)
{
  if (pliData == NULL) {
    return;
  }

  mprisPause (pliData->mpris);
  pliData->state = PLI_STATE_PAUSED;
}

void
pliiPlay (plidata_t *pliData)
{
  if (pliData == NULL) {
    return;
  }

  mprisPlay (pliData->mpris);
  pliData->state = PLI_STATE_PLAYING;
}

void
pliiStop (plidata_t *pliData)
{
  if (pliData == NULL) {
    return;
  }

  mprisStop (pliData->mpris);
  pliData->state = PLI_STATE_STOPPED;
}

ssize_t
pliiSeek (plidata_t *pliData, ssize_t pos)
{
  ssize_t   ret = pos;

  if (pliData == NULL) {
    return pos;
  }

  mprisSetPosition (pliData->mpris, pos);
  return ret;
}

ssize_t
pliiRate (plidata_t *pliData, ssize_t rate)
{
  double    drate;

  if (pliData == NULL) {
    return 100;
  }

  drate = (double) rate / 100.0;
  mprisSetRate (pliData->mpris, drate);
  return rate;
}

void
pliiClose (plidata_t *pliData)
{
  if (pliData == NULL) {
    return;
  }

  pliData->state = PLI_STATE_STOPPED;
}

ssize_t
pliiGetDuration (plidata_t *pliData)
{
  ssize_t     duration = 0;

  if (pliData == NULL) {
    return 0;
  }

  /* mpris:length from the metadata */
  duration = mprisGetDuration (pliData->mpris);
  return duration;
}

ssize_t
pliiGetTime (plidata_t *pliData)
{
  ssize_t     playTime = 0;
  double      dpos;

  if (pliData == NULL) {
    return playTime;
  }

  dpos = mprisGetPosition (pliData->mpris);
  playTime = (ssize_t) (dpos * 1000.0);
  return playTime;
}

plistate_t
pliiState (plidata_t *pliData)
{
  plistate_t  plistate = PLI_STATE_NONE; /* unknown */

  if (pliData == NULL) {
    return plistate;
  }

  plistate = mprisState (pliData->mpris);
  pliData->state = plistate;
  return plistate;
}

int
pliiSetAudioDevice (plidata_t *pliData, const char *dev, plidev_t plidevtype)
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

int
pliiUnsupportedFileTypes (plidata_t *plidata, int types [], size_t typmax)
{
  /* linux supports everything */
  return 0;
}

#endif /* __linux__ */
