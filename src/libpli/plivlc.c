#include "config.h"

#if _lib_libvlc_new

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <math.h>

#include "pli.h"
#include "vlci.h"
#include "tmutil.h"

#if 0
      "-vv",
      "--file-logging",
      "--verbose=3",
#endif

static char *vlcDefaultOptions [] = {
      "--quiet",
      "--audio-filter", "scaletempo",
      "--intf=dummy",
      "--ignore-config",
      "--no-media-library",
      "--no-playlist-autostart",
      "--no-random",
      "--no-loop",
      "--no-repeat",
      "--play-and-stop",
      "--novideo",
};
#define VLC_DFLT_OPT_SZ (sizeof (vlcDefaultOptions) / sizeof (char *))

static void     pliiWaitUntilPlaying (plidata_t *pliData);

plidata_t *
pliiInit (const char *volpkg, const char *sinkname)
{
  plidata_t *pliData;
  char      * vlcOptions [5];

  pliData = malloc (sizeof (plidata_t));
  assert (pliData != NULL);
  if (pliData != NULL) {
    char  tbuff [200];

    vlcOptions [0] = NULL;
    if (strcmp (volpkg, "libvolwin") == 0) {
      vlcOptions [0] = "--aout=direct";
      snprintf (tbuff, sizeof (tbuff), "--directx-audio-device=%s\n", sinkname);
      vlcOptions [1] = tbuff;
      vlcOptions [2] = NULL;
    }
    if (strcmp (volpkg, "libvolalsa") == 0) {
      snprintf (tbuff, sizeof (tbuff), "--alsa-audio-device=%s\n", sinkname);
      vlcOptions [0] = tbuff;
      vlcOptions [1] = NULL;
    }

    pliData->plData = vlcInit (VLC_DFLT_OPT_SZ, vlcDefaultOptions, vlcOptions);
  }
  pliData->name = "VLC Integrated";
  return pliData;
}

void
pliiFree (plidata_t *pliData)
{
  if (pliData != NULL) {
    pliiClose (pliData);
    free (pliData);
  }
}

void
pliiMediaSetup (plidata_t *pliData, char *mediaPath)
{
  if (pliData != NULL && pliData->plData != NULL && mediaPath != NULL) {
    vlcMedia (pliData->plData, mediaPath);
  }
}

void
pliiStartPlayback (plidata_t *pliData, ssize_t dpos, ssize_t speed)
{
  /* Do the busy loop so that the seek can be done immediately as */
  /* vlc starts playing.  This should help avoid startup glitches */
  if (pliData != NULL && pliData->plData != NULL) {
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
}

void
pliiPause (plidata_t *pliData)
{
  if (pliData != NULL && pliData->plData != NULL) {
    vlcPause (pliData->plData);
  }
}

void
pliiPlay (plidata_t *pliData)
{
  if (pliData != NULL && pliData->plData != NULL) {
    vlcPlay (pliData->plData);
  }
}

void
pliiStop (plidata_t *pliData)
{
  if (pliData != NULL && pliData->plData != NULL) {
    vlcStop (pliData->plData);
  }
}

ssize_t
pliiSeek (plidata_t *pliData, ssize_t dpos)
{
  ssize_t     dret = -1;

  if (pliData != NULL && pliData->plData != NULL) {
    dret = vlcSeek (pliData->plData, dpos);
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
    dret = vlcRate (pliData->plData, drate);
    ret = (ssize_t) round (dret * 100.0);
  }
  return ret;
}

void
pliiClose (plidata_t *pliData)
{
  if (pliData != NULL) {
    if (pliData->plData != NULL) {
      vlcClose (pliData->plData);
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
      duration = vlcGetDuration (pliData->plData);
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
      playTime = vlcGetTime (pliData->plData);
    }
  }
  return playTime;
}

plistate_t
pliiState (plidata_t *pliData)
{
  libvlc_state_t      vlcstate;
  plistate_t          plistate = PLI_STATE_NONE; /* unknown */

  if (pliData != NULL) {
    if (pliData->plData != NULL) {
      vlcstate = vlcState (pliData->plData);

      switch (vlcstate) {
        case libvlc_NothingSpecial: { plistate = PLI_STATE_NONE; break; }
        case libvlc_Opening: { plistate = PLI_STATE_OPENING; break; }
        case libvlc_Buffering: { plistate = PLI_STATE_BUFFERING; break; }
        case libvlc_Playing: { plistate = PLI_STATE_PLAYING; break; }
        case libvlc_Paused: { plistate = PLI_STATE_PAUSED; break; }
        case libvlc_Stopped: { plistate = PLI_STATE_STOPPED; break; }
        case libvlc_Ended: { plistate = PLI_STATE_ENDED; break; }
        case libvlc_Error: { plistate = PLI_STATE_ERROR; break; }
      }
    }
  }
  return plistate;
}

static void
pliiWaitUntilPlaying (plidata_t *pliData)
{
  libvlc_state_t      vlcstate;
  long                count;

  vlcstate = vlcState (pliData->plData);
  count = 0;
  while (vlcstate == libvlc_NothingSpecial ||
         vlcstate == libvlc_Opening ||
         vlcstate == libvlc_Buffering ||
         vlcstate == libvlc_Stopped) {
    mssleep (1);
    vlcstate = vlcState (pliData->plData);
    ++count;
    if (count > 10000) {
      break;
    }
  }
}


#endif /* have libvlc_new */

