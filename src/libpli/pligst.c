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

#if _hdr_gst_gst

#include "bdj4.h"
#include "dyintfc.h"
#include "gsti.h"
#include "mdebug.h"
#include "pli.h"
#include "tmutil.h"
#include "volsink.h"

typedef struct plidata {
  gsti_t            *gsti;
  int               state;
  int               supported;
} plidata_t;

void
pliiDesc (char **ret, int max)
{
  int         c = 0;

  ret [c++] = "GStreamer";
  ret [c++] = NULL;
}

plidata_t *
pliiInit (const char *plinm)
{
  plidata_t *pliData;

  pliData = mdmalloc (sizeof (plidata_t));
  pliData->gsti = gstiInit (plinm);
  pliData->state = PLI_STATE_STOPPED;
  pliData->supported = PLI_SUPPORT_NONE;
  pliData->supported |= PLI_SUPPORT_SEEK;
  if (gstiHasSpeed (pliData->gsti)) {
    pliData->supported |= PLI_SUPPORT_SPEED;
  }

  return pliData;
}

void
pliiFree (plidata_t *pliData)
{
  if (pliData == NULL) {
    return;
  }

  gstiFree (pliData->gsti);
  pliiClose (pliData);
  mdfree (pliData);
  gstiCleanup ();
}

void
pliiMediaSetup (plidata_t *pliData, const char *mediaPath,
    const char *fullMediaPath)
{
  if (pliData == NULL || mediaPath == NULL) {
    return;
  }

  gstiMedia (pliData->gsti, fullMediaPath);
  pliData->state = PLI_STATE_STOPPED;
}

void
pliiCleanup (void)
{
  gstiCleanup ();
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

  gstiPause (pliData->gsti);
  pliData->state = PLI_STATE_PAUSED;
}

void
pliiPlay (plidata_t *pliData)
{
  if (pliData == NULL) {
    return;
  }

  gstiPlay (pliData->gsti);
  pliData->state = PLI_STATE_PLAYING;
}

void
pliiStop (plidata_t *pliData)
{
  if (pliData == NULL) {
    return;
  }

  gstiStop (pliData->gsti);
  pliData->state = PLI_STATE_STOPPED;
}

ssize_t
pliiSeek (plidata_t *pliData, ssize_t pos)
{
  ssize_t   ret = pos;

  if (pliData == NULL) {
    return pos;
  }

  gstiSetPosition (pliData->gsti, pos);
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
  gstiSetRate (pliData->gsti, drate);
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

  /* gsti:length from the metadata */
  duration = gstiGetDuration (pliData->gsti);
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

  dpos = gstiGetPosition (pliData->gsti);
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

  plistate = gstiState (pliData->gsti);
  pliData->state = plistate;
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

#endif /* _hdr_gst */
