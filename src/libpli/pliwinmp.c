/*
 * Copyright 2016-2026 Brad Lanam Walnut Creek CA
 * Copyright 2020 Brad Lanam Pleasant Hill CA
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <assert.h>

#include "bdj4intl.h"
#include "log.h"
#include "mdebug.h"
#include "tmutil.h"
#include "pli.h"
#include "winmpi.h"
#include "volsink.h"

typedef struct plidata {
  char              *name;
  windata_t         *windata;
  ssize_t           duration;
  ssize_t           playTime;
  plistate_t        state;
  int               supported;
} plidata_t;

static void pliwinWaitUntilStopped (plidata_t *pliData);

void
pliiDesc (const char **ret, int max)
{
  int   c = 0;

  if (max < 2) {
    return;
  }

  ret [c++] = _("Windows Media Player");
  ret [c++] = NULL;
}

plidata_t *
pliiInit (const char *plinm, const char *playerargs)
{
  plidata_t *pliData;

  pliData = mdmalloc (sizeof (plidata_t));

  pliData->windata = winmpInit ();
  pliData->name = "Windows Native";
  pliData->supported =
      PLI_SUPPORT_SEEK |
      PLI_SUPPORT_SPEED |
      PLI_SUPPORT_CROSSFADE |
      PLI_SUPPORT_STREAM |
      PLI_SUPPORT_STREAM_SPD;

  return pliData;
}

void
pliiFree (plidata_t *pliData)
{
  if (pliData == NULL) {
    return;
  }

  pliiClose (pliData);
  mdfree (pliData);
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
  if (pliData == NULL || pliData->windata == NULL || mediaPath == NULL) {
    return;
  }

  winmpMedia (pliData->windata, fullMediaPath, sourceType);
}

void
pliiCrossFade (plidata_t *pliData, const char *mediaPath,
    const char *fullMediaPath, int sourceType)
{
  if (pliData == NULL || pliData->windata == NULL || mediaPath == NULL) {
    return;
  }

  winmpCrossFade (pliData->windata, fullMediaPath, sourceType);
}

void
pliiStartPlayback (plidata_t *pliData, ssize_t dpos, ssize_t speed)
{
  if (pliData == NULL || pliData->windata == NULL) {
    return;
  }

  /* windows allows seek and rate changes before playing */
  if (dpos > 0) {
    winmpSeek (pliData->windata, dpos);
  }
  if (speed != 100) {
    double    drate;

    drate = (double) speed / 100.0;
    winmpRate (pliData->windata, drate);
  }
  winmpPlay (pliData->windata);
}

void
pliiPause (plidata_t *pliData)
{
  if (pliData == NULL || pliData->windata == NULL) {
    return;
  }

  winmpPause (pliData->windata);
}

void
pliiPlay (plidata_t *pliData)
{
  if (pliData == NULL || pliData->windata == NULL) {
    return;
  }

  winmpPlay (pliData->windata);
}

void
pliiStop (plidata_t *pliData)
{
  if (pliData == NULL || pliData->windata == NULL) {
    return;
  }

  winmpStop (pliData->windata);
  pliwinWaitUntilStopped (pliData);
}

ssize_t
pliiSeek (plidata_t *pliData, ssize_t dpos)
{
  ssize_t     dret = -1;

  if (pliData == NULL || pliData->windata == NULL) {
    return dret;
  }

  dret = winmpSeek (pliData->windata, dpos);
  return dret;
}

ssize_t
pliiRate (plidata_t *pliData, ssize_t rate)
{
  ssize_t     ret = 100;
  double      dret = -1.0;
  double      drate;

  if (pliData == NULL || pliData->windata == NULL) {
    return ret;
  }

  drate = (double) rate / 100.0;
  dret = winmpRate (pliData->windata, drate);
  ret = (ssize_t) round (dret * 100.0);
  return ret;
}

void
pliiClose (plidata_t *pliData)
{
  if (pliData == NULL || pliData->windata == NULL) {
    return;
  }

  winmpClose (pliData->windata);
  pliData->windata = NULL;
}

ssize_t
pliiGetDuration (plidata_t *pliData)
{
  ssize_t     duration = 0;

  if (pliData == NULL || pliData->windata == NULL) {
    return duration;
  }

  duration = winmpGetDuration (pliData->windata);
  return duration;
}

ssize_t
pliiGetTime (plidata_t *pliData)
{
  ssize_t     playTime = 0;

  if (pliData == NULL || pliData->windata == NULL) {
    return playTime;
  }

  playTime = winmpGetTime (pliData->windata);
  return playTime;
}

plistate_t
pliiState (plidata_t *pliData)
{
  plistate_t    plistate = PLI_STATE_NONE; /* unknown */

  if (pliData == NULL || pliData->windata == NULL) {
    return plistate;
  }

  plistate = winmpState (pliData->windata);
  return plistate;
}

int
pliiSetAudioDevice (plidata_t *pliData, const char *dev, plidev_t plidevtype)
{
  int   rc = 0;

  if (pliData == NULL || pliData->windata == NULL) {
    return -1;
  }

  rc = winmpSetAudioDevice (pliData->windata, dev, plidevtype);
  return rc;
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
pliiGetVolume (plidata_t *pliData)
{
  int   val = 100;

  val = winmpGetVolume (pliData->windata);
  return val;
}

void
pliiCrossFadeVolume (plidata_t *pliData, int vol)
{
  winmpCrossFadeVolume (pliData->windata, vol);
}

/* internal routines */

static void
pliwinWaitUntilStopped (plidata_t *pliData)
{
  plistate_t  state;
  long        count;

  /* it appears that the stop action usually happens within < a few ms */
  state = winmpState (pliData->windata);
  count = 0;
  while (state == PLI_STATE_PLAYING ||
      state == PLI_STATE_PAUSED ||
      state == PLI_STATE_STOPPING) {
    mssleep (1);
    state = winmpState (pliData->windata);
    ++count;
    if (count > 10000) {
      break;
    }
  }
}

