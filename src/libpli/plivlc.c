/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#if _lib_libvlc_new

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
#include "vlci.h"
#include "volsink.h"

typedef struct plidata {
  char              *name;
  void              *plData;
  ssize_t           duration;
  ssize_t           playTime;
  plistate_t        state;
  int               supported;
} plidata_t;

static char *vlcDefaultOptions [] = {
      "--quiet",
      "--audio-filter", "scaletempo",
      "--src-converter-type=0",
      "--intf=dummy",
      "--ignore-config",
      "--no-media-library",
      "--no-playlist-autostart",
      "--no-random",
      "--no-loop",
      "--no-repeat",
      "--play-and-stop",
      "--novideo",
      "--no-metadata-network-access",
#if 0  // VLC logging options
      "-vv",
      "--file-logging",
      "--verbose=3",
#endif
};
enum {
  VLC_DFLT_OPT_SZ = (sizeof (vlcDefaultOptions) / sizeof (char *))
};

static void     pliiWaitUntilPlaying (plidata_t *pliData);

void
pliiDesc (char **ret, int max)
{
  int         c = 0;

  if (max < 2) {
    return;
  }

  ret [c++] = "Integrated VLC";
  ret [c++] = NULL;
}

plidata_t *
pliiInit (const char *plinm)
{
  plidata_t *pliData;
  char      * vlcOptions [5];

  pliData = mdmalloc (sizeof (plidata_t));

  vlcOptions [0] = NULL;
  pliData->plData = vlcInit (VLC_DFLT_OPT_SZ, vlcDefaultOptions, vlcOptions);
  pliData->name = "Integrated VLC";
  pliData->supported = PLI_SUPPORT_SEEK | PLI_SUPPORT_SPEED;
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
    const char *fullMediaPath)
{
  if (pliData == NULL || pliData->plData == NULL || mediaPath == NULL) {
    return;
  }

  vlcMedia (pliData->plData, mediaPath);
}

void
pliiStartPlayback (plidata_t *pliData, ssize_t dpos, ssize_t speed)
{
  if (pliData == NULL || pliData->plData == NULL) {
    return;
  }

  /* Do the busy loop so that the seek can be done immediately as */
  /* vlc starts playing.  This should help avoid startup glitches */
  vlcPlay (pliData->plData);
  if (dpos > 0) {
    pliiWaitUntilPlaying (pliData);
    vlcSeek (pliData->plData, dpos);
  }
  if (speed != 100) {
    double    drate;

    pliiWaitUntilPlaying (pliData);
    drate = (double) speed / 100.0;
    vlcRate (pliData->plData, drate);
  }
}

void
pliiPause (plidata_t *pliData)
{
  if (pliData == NULL || pliData->plData == NULL) {
    return;
  }

  vlcPause (pliData->plData);
}

void
pliiPlay (plidata_t *pliData)
{
  if (pliData == NULL || pliData->plData == NULL) {
    return;
  }

  vlcPlay (pliData->plData);
}

void
pliiStop (plidata_t *pliData)
{
  if (pliData == NULL || pliData->plData == NULL) {
    return;
  }

  vlcStop (pliData->plData);
}

ssize_t
pliiSeek (plidata_t *pliData, ssize_t dpos)
{
  ssize_t     dret = -1;

  if (pliData == NULL || pliData->plData == NULL) {
    return dret;
  }

  dret = vlcSeek (pliData->plData, dpos);
  return dret;
}

ssize_t
pliiRate (plidata_t *pliData, ssize_t rate)
{
  ssize_t     ret = 100;
  double      dret = -1.0;
  double      drate;

  if (pliData == NULL || pliData->plData == NULL) {
    return ret;
  }

  drate = (double) rate / 100.0;
  dret = vlcRate (pliData->plData, drate);
  ret = (ssize_t) round (dret * 100.0);
  return ret;
}

void
pliiClose (plidata_t *pliData)
{
  if (pliData == NULL || pliData->plData == NULL) {
    return;
  }

  vlcClose (pliData->plData);
  pliData->plData = NULL;
}

ssize_t
pliiGetDuration (plidata_t *pliData)
{
  ssize_t     duration = 0;

  if (pliData == NULL || pliData->plData == NULL) {
    return duration;
  }

  duration = vlcGetDuration (pliData->plData);
  return duration;
}

ssize_t
pliiGetTime (plidata_t *pliData)
{
  ssize_t     playTime = 0;

  if (pliData == NULL || pliData->plData == NULL) {
    return playTime;
  }

  playTime = vlcGetTime (pliData->plData);
  return playTime;
}

plistate_t
pliiState (plidata_t *pliData)
{
  plistate_t    plistate = PLI_STATE_NONE; /* unknown */

  if (pliData == NULL || pliData->plData == NULL) {
    return plistate;
  }

  plistate = vlcState (pliData->plData);
  return plistate;
}

int
pliiSetAudioDevice (plidata_t *pliData, const char *dev)
{
  int   rc;

  if (pliData == NULL || pliData->plData == NULL) {
    return -1;
  }

  rc = vlcAudioDevSet (pliData->plData, dev);
  return rc;
}

int
pliiAudioDeviceList (plidata_t *pliData, volsinklist_t *sinklist)
{
  int   rc = 0;

  /* VLC will use the default sink set by the application. */
  /* No need to make things more complicated. */
  /* test 2023-12-4 on Linux, MacOS, Windows */
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

  state = vlcState (pliData->plData);
  count = 0;
  while (state == PLI_STATE_IDLE ||
         state == PLI_STATE_OPENING ||
         state == PLI_STATE_BUFFERING ||
         state == PLI_STATE_STOPPED) {
    mssleep (1);
    state = vlcState (pliData->plData);
    ++count;
    if (count > 10000) {
      break;
    }
  }
}

#endif /* have libvlc_new */

