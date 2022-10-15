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
#include "volsink.h"

#if 0  // VLC logging options
      "-vv",
      "--file-logging",
      "--verbose=3",
#endif

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
};
enum {
  VLC_DFLT_OPT_SZ = (sizeof (vlcDefaultOptions) / sizeof (char *))
};

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
    char  tbuffb [200];

    vlcOptions [0] = NULL;
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
pliiMediaSetup (plidata_t *pliData, const char *mediaPath)
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

int
pliiSetAudioDevice (plidata_t *pliData, const char *dev)
{
  int   rc;

  rc = vlcAudioDevSet (pliData->plData, dev);
  return rc;
}

int
pliiAudioDeviceList (plidata_t *pliData, volsinklist_t *sinklist)
{
  int   rc = 0;

  if (vlcHaveAudioDevList ()) {
    rc = vlcAudioDevList (pliData->plData, sinklist);
  }

  return rc;
}

/* internal routines */

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

