/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

/*
 * MPV is not currently shipped.
 *
 * There are bugs that need to be tracked down.
 * The testsuite does not complete correctly.
 *
 */

#if _lib_mpv_create

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "pli.h"
#include "mdebug.h"
#include "tmutil.h"
#include "mpvi.h"
#include "volsink.h"

typedef struct plidata {
  char              *name;
  void              *plData;
  ssize_t           duration;
  ssize_t           playTime;
  plistate_t        state;
  int               supported;
} plidata_t;

static void     pliiWaitUntilPlaying (plidata_t *pliData);

void
pliiDesc (char **ret, int max)
{
  int         c = 0;

  if (max < 2) {
    return;
  }

  ret [c++] = "Integrated MPV";
  ret [c++] = NULL;
}

plidata_t *
pliiInit (const char *plinm)
{
  plidata_t *pliData;

  pliData = mdmalloc (sizeof (plidata_t));

  pliData->plData = mpvInit ();
  pliData->name = "Integrated MPV";
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
pliiMediaSetup (plidata_t *pliData, const char *mediaPath)
{
  if (pliData != NULL && pliData->plData != NULL && mediaPath != NULL) {
    mpvMedia (pliData->plData, mediaPath);
  }
}

void
pliiStartPlayback (plidata_t *pliData, ssize_t dpos, ssize_t speed)
{
  /* Do the busy loop so that the seek can be done immediately as */
  /* mpv starts playing.  This should help avoid startup glitches */
  if (pliData != NULL && pliData->plData != NULL) {
    mpvPlay (pliData->plData);
    if (dpos > 0) {
      pliiWaitUntilPlaying (pliData);
      mpvSeek (pliData->plData, dpos);
    }
    if (speed != 100) {
      double    drate;

      pliiWaitUntilPlaying (pliData);
      drate = (double) speed / 100.0;
      mpvRate (pliData->plData, drate);
    }
  }
}

void
pliiPause (plidata_t *pliData)
{
  if (pliData != NULL && pliData->plData != NULL) {
    mpvPause (pliData->plData);
  }
}

void
pliiPlay (plidata_t *pliData)
{
  if (pliData != NULL && pliData->plData != NULL) {
    mpvPlay (pliData->plData);
  }
}

void
pliiStop (plidata_t *pliData)
{
  if (pliData != NULL && pliData->plData != NULL) {
    mpvStop (pliData->plData);
  }
}

ssize_t
pliiSeek (plidata_t *pliData, ssize_t dpos)
{
  ssize_t     dret = -1;

  if (pliData != NULL && pliData->plData != NULL) {
    dret = mpvSeek (pliData->plData, dpos);
  }
  return dret;
}

ssize_t
pliiRate (plidata_t *pliData, ssize_t rate)
{
  ssize_t     ret = 100;
  double      dret = -1.0;
  double      drate;

  if (pliData != NULL && pliData->plData != NULL) {
    drate = (double) rate / 100.0;
    dret = mpvRate (pliData->plData, drate);
    ret = (ssize_t) round (dret * 100.0);
  }
  return ret;
}

void
pliiClose (plidata_t *pliData)
{
  if (pliData != NULL) {
    if (pliData->plData != NULL) {
      mpvClose (pliData->plData);
    }
    pliData->plData = NULL;
  }
}

ssize_t
pliiGetDuration (plidata_t *pliData)
{
  ssize_t     duration = 0;

  if (pliData != NULL) {
    if (pliData->plData != NULL) {
      duration = mpvGetDuration (pliData->plData);
    }
  }
  return duration;
}

ssize_t
pliiGetTime (plidata_t *pliData)
{
  ssize_t     playTime = 0;

  if (pliData != NULL) {
    if (pliData->plData != NULL) {
      playTime = mpvGetTime (pliData->plData);
    }
  }
  return playTime;
}

plistate_t
pliiState (plidata_t *pliData)
{
  plistate_t          plistate = PLI_STATE_NONE; /* unknown */

  if (pliData != NULL && pliData->plData != NULL) {
    plistate = mpvState (pliData->plData);
  }
  return plistate;
}

int
pliiSetAudioDevice (plidata_t *pliData, const char *dev)
{
  int   rc;

  /* MPV must set the audio sink. */
  /* MPV will not use the default sink set by the application. */
  rc = mpvAudioDevSet (pliData->plData, dev);
  return rc;
}

int
pliiAudioDeviceList (plidata_t *pliData, volsinklist_t *sinklist)
{
  int   rc = 0;

  if (mpvHaveAudioDevList ()) {
    rc = mpvAudioDevList (pliData->plData, sinklist);
  }

  return rc;
}

int
pliiSupported (plidata_t *pliData)
{
  return pliData->supported;
}

/* internal routines */

static void
pliiWaitUntilPlaying (plidata_t *pliData)
{
  plistate_t  state;
  long        count;

  state = mpvState (pliData->plData);
  count = 0;
  while (state == PLI_STATE_IDLE ||
         state == PLI_STATE_OPENING ||
         state == PLI_STATE_BUFFERING ||
         state == PLI_STATE_STOPPED) {
    mssleep (1);
    state = mpvState (pliData->plData);
    ++count;
    if (count > 10000) {
      break;
    }
  }
}


#endif /* _lib_mpv_create */

