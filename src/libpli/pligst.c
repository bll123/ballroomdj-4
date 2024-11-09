/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
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

static void pliiWaitUntilPlaying (plidata_t *plidata);

void
pliiDesc (const char **ret, int max)
{
  int         c = 0;

  ret [c++] = "GStreamer";
  ret [c++] = NULL;
}

plidata_t *
pliiInit (const char *plinm)
{
  plidata_t *plidata;

  plidata = mdmalloc (sizeof (plidata_t));
  plidata->gsti = gstiInit (plinm);
  plidata->state = PLI_STATE_STOPPED;
  plidata->supported = PLI_SUPPORT_NONE;
  plidata->supported |= PLI_SUPPORT_SEEK;
  plidata->supported |= PLI_SUPPORT_SPEED;
  plidata->supported |= PLI_SUPPORT_CROSSFADE;

  return plidata;
}

void
pliiFree (plidata_t *plidata)
{
  if (plidata == NULL) {
    return;
  }

  gstiFree (plidata->gsti);
  pliiClose (plidata);
  mdfree (plidata);
}

void
pliiCleanup (void)
{
  gstiCleanup ();
  return;
}

void
pliiMediaSetup (plidata_t *plidata, const char *mediaPath,
    const char *fullMediaPath, int sourceType)
{
  if (plidata == NULL || mediaPath == NULL) {
    return;
  }

  gstiMedia (plidata->gsti, fullMediaPath, sourceType);
  plidata->state = PLI_STATE_STOPPED;
}

void
pliiCrossFade (plidata_t *plidata, const char *mediaPath,
    const char *fullMediaPath, int sourceType)
{
  if (plidata == NULL || mediaPath == NULL) {
    return;
  }

  gstiCrossFade (plidata->gsti, fullMediaPath, sourceType);
  plidata->state = PLI_STATE_CROSSFADE;
}

void
pliiCrossFadeVolume (plidata_t *plidata, int vol)
{
  if (plidata == NULL) {
    return;
  }

  gstiCrossFadeVolume (plidata->gsti, vol);
}

void
pliiStartPlayback (plidata_t *plidata, ssize_t dpos, ssize_t speed)
{
  if (plidata == NULL) {
    return;
  }

  pliiPlay (plidata);
  if (dpos > 0 || speed != 100) {
    /* GStreamer must be in a paused or playing state to seek/set rate */
    pliiWaitUntilPlaying (plidata);
  }
  /* set the rate first */
  /* GStreamer can change the rate, but not the pitch */
  /* cannot seem to get scaletempo to work */
  pliiRate (plidata, speed);
  pliiSeek (plidata, dpos);
}

void
pliiPause (plidata_t *plidata)
{
  if (plidata == NULL) {
    return;
  }

  gstiPause (plidata->gsti);
  plidata->state = PLI_STATE_PAUSED;
}

void
pliiPlay (plidata_t *plidata)
{
  if (plidata == NULL) {
    return;
  }

  gstiPlay (plidata->gsti);
  plidata->state = PLI_STATE_PLAYING;
}

void
pliiStop (plidata_t *plidata)
{
  if (plidata == NULL) {
    return;
  }

  gstiStop (plidata->gsti);
  plidata->state = PLI_STATE_STOPPED;
}

ssize_t
pliiSeek (plidata_t *plidata, ssize_t pos)
{
  ssize_t   ret = pos;

  if (plidata == NULL) {
    return pos;
  }

  gstiSetPosition (plidata->gsti, pos);
  return ret;
}

ssize_t
pliiRate (plidata_t *plidata, ssize_t rate)
{
  double    drate;

  if (plidata == NULL) {
    return 100;
  }

  drate = (double) rate / 100.0;
  gstiSetRate (plidata->gsti, drate);
  return rate;
}

void
pliiClose (plidata_t *plidata)
{
  if (plidata == NULL) {
    return;
  }

  plidata->state = PLI_STATE_STOPPED;
}

ssize_t
pliiGetDuration (plidata_t *plidata)
{
  ssize_t     duration = 0;

  if (plidata == NULL) {
    return 0;
  }

  duration = gstiGetDuration (plidata->gsti);
  return duration;
}

ssize_t
pliiGetTime (plidata_t *plidata)
{
  ssize_t     playTime = 0;

  if (plidata == NULL) {
    return playTime;
  }

  playTime = gstiGetPosition (plidata->gsti);
  return playTime;
}

plistate_t
pliiState (plidata_t *plidata)
{
  plistate_t  plistate = PLI_STATE_NONE; /* unknown */

  if (plidata == NULL) {
    return plistate;
  }

  plistate = gstiState (plidata->gsti);
  plidata->state = plistate;
  return plistate;
}

int
pliiSetAudioDevice (plidata_t *plidata, const char *dev, int plidevtype)
{
  return 0;
}

int
pliiAudioDeviceList (plidata_t *plidata, volsinklist_t *sinklist)
{
  return 0;
}

int
pliiSupported (plidata_t *plidata)
{
  return plidata->supported;
}

int
pliiGetVolume (plidata_t *plidata)
{
  int   val;

  val = gstiGetVolume (plidata->gsti);
  return val;
}

/* internal routines */

static void
pliiWaitUntilPlaying (plidata_t *plidata)
{
  plistate_t  state;
  long        count;

  state = gstiState (plidata->gsti);
  count = 0;
  while (state == PLI_STATE_IDLE ||
         state == PLI_STATE_OPENING ||
         state == PLI_STATE_BUFFERING ||
         state == PLI_STATE_STOPPED) {
    mssleep (1);
    state = gstiState (plidata->gsti);
    ++count;
    if (count > 10000) {
      break;
    }
  }
}

#endif /* _hdr_gst */
