#include "config.h"
#include "configvlc.h"

#if _lib_libvlc_new

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "pli.h"
#include "vlci.h"
#include "tmutil.h"

#if 0
      "-vv",
      "--file-logging",
      "--verbose=3",
#endif

static char *   vlcDefaultOptions [] = {
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

plidata_t *
pliiInit (void)
{
  plidata_t *pliData;

  pliData = malloc (sizeof (plidata_t));
  assert (pliData != NULL);
  if (pliData != NULL) {
    pliData->plData = vlcInit (VLC_DFLT_OPT_SZ, vlcDefaultOptions);
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
pliiStartPlayback (plidata_t *pliData)
{
  if (pliData != NULL && pliData->plData != NULL) {
    vlcPlay (pliData->plData);
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

#endif /* have libvlc_new */
