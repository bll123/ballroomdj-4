/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "log.h"
#include "macosav.h"
#include "mdebug.h"
#include "pli.h"

typedef struct plidata {
  macosav_t         *macosav;
  int               supported;
} plidata_t;

static void plimacosavWaitUntilPlaying (plidata_t *pliData);

void
pliiDesc (const char **ret, int max)
{
  int         c = 0;

  if (max < 2) {
    return;
  }

  ret [c++] = "MacOS AVPlayer";
  ret [c++] = NULL;
}

plidata_t *
pliiInit (const char *plinm, const char *playerargs)
{
  plidata_t *pliData;

  pliData = mdmalloc (sizeof (plidata_t));
  pliData->macosav = macosavInit ();
  pliData->supported = PLI_SUPPORT_SEEK | PLI_SUPPORT_SPEED;
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
pliiCleanup (void)
{
  return;
}

void
pliiMediaSetup (plidata_t *pliData, const char *mediaPath,
    const char *fullMediaPath, int sourceType)
{
  if (pliData == NULL || pliData->macosav == NULL || mediaPath == NULL) {
    return;
  }

  macosavMedia (pliData->macosav, fullMediaPath, sourceType);
}

void
pliiStartPlayback (plidata_t *pliData, ssize_t dpos, ssize_t speed)
{
  double      drate;

  if (pliData == NULL || pliData->macosav == NULL) {
    return;
  }

  drate = (double) speed / 100.0;
  macosavRate (pliData->macosav, drate);
  macosavPlay (pliData->macosav);
  plimacosavWaitUntilPlaying (pliData);
}

void
pliiPause (plidata_t *pliData)
{
  if (pliData == NULL || pliData->macosav == NULL) {
    return;
  }

fprintf (stderr, "pli-pause\n");
  macosavPause (pliData->macosav);
}

void
pliiPlay (plidata_t *pliData)
{
  if (pliData == NULL || pliData->macosav == NULL) {
    return;
  }

fprintf (stderr, "pli-play\n");
  macosavPlay (pliData->macosav);
}

void
pliiStop (plidata_t *pliData)
{
  if (pliData == NULL || pliData->macosav == NULL) {
    return;
  }

fprintf (stderr, "pli-stop\n");
  macosavStop (pliData->macosav);
}

ssize_t
pliiSeek (plidata_t *pliData, ssize_t pos)
{
  if (pliData == NULL || pliData->macosav == NULL) {
    return 0;
  }

fprintf (stderr, "pli-seek\n");
  pos = macosavSeek (pliData->macosav, pos);
  return pos;
}

ssize_t
pliiRate (plidata_t *pliData, ssize_t rate)
{
  double    drate;

  if (pliData == NULL || pliData->macosav == NULL) {
    return 100;
  }

fprintf (stderr, "pli-rate\n");
  drate = (double) rate / 100.0;
  drate = macosavRate (pliData->macosav, drate);
  rate = (ssize_t) round (drate * 100.0);
  return rate;
}

void
pliiClose (plidata_t *pliData)
{
  if (pliData == NULL || pliData->macosav == NULL) {
    return;
  }

  macosavClose (pliData->macosav);
  pliData->macosav = NULL;
}

ssize_t
pliiGetDuration (plidata_t *pliData)
{
  ssize_t     duration = 0;

  if (pliData == NULL || pliData->macosav == NULL) {
    return 0;
  }

  duration = macosavGetDuration (pliData->macosav);
  return duration;
}

ssize_t
pliiGetTime (plidata_t *pliData)
{
  ssize_t     playTime = 0;

  if (pliData == NULL || pliData->macosav == NULL) {
    return 0;
  }

  playTime = macosavGetTime (pliData->macosav);
  return playTime;
}

plistate_t
pliiState (plidata_t *pliData)
{
  plistate_t          plistate = PLI_STATE_NONE; /* unknown */

  if (pliData == NULL || pliData->macosav == NULL) {
    return PLI_STATE_NONE;
  }

  plistate = macosavState (pliData->macosav);
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

/* internal routines */

static void
plimacosavWaitUntilPlaying (plidata_t *pliData)
{
  plistate_t  state;
  long        count;

  state = macosavState (pliData->macosav);
  count = 0;
  while (state == PLI_STATE_IDLE ||
      state == PLI_STATE_OPENING ||
      state == PLI_STATE_STOPPED) {
    mssleep (1);
    state = macosavState (pliData->macosav);
    ++count;
    if (count > 10000) {
      break;
    }
  }
}

