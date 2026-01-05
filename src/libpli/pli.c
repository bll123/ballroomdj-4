/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "bdj4.h"
#include "bdjopt.h"
#include "pathbld.h"
#include "dyintfc.h"
#include "dylib.h"
#include "ilist.h"
#include "mdebug.h"
#include "pli.h"
#include "sysvars.h"
#include "volsink.h"

static const char *plistateTxt [PLI_STATE_MAX] = {
  [PLI_STATE_NONE] = "none",
  [PLI_STATE_IDLE] = "idle",
  [PLI_STATE_OPENING] = "opening",
  [PLI_STATE_BUFFERING] = "buffering",
  [PLI_STATE_PLAYING] = "playing",
  [PLI_STATE_PAUSED] = "paused",
  [PLI_STATE_STOPPED] = "stopped",
  [PLI_STATE_STOPPING] = "stopping",
  [PLI_STATE_ERROR] = "error",
};

static_assert (sizeof (plistateTxt) / sizeof (const char *) == PLI_STATE_MAX,
    "missing pli state");

typedef struct pli {
  dlhandle_t        *dlHandle;
  plidata_t         *(*pliiInit) (const char *plinm, const char *playerargs);
  void              (*pliiFree) (plidata_t *plidata);
  void              (*pliiMediaSetup) (plidata_t *plidata, const char *mediaPath, const char *fullMediaPath, int sourceType);
  void              (*pliiCrossFade) (plidata_t *plidata, const char *mediaPath, const char *fullMediaPath, int sourceType);
  void              (*pliiStartPlayback) (plidata_t *plidata, ssize_t pos, ssize_t speed);
  void              (*pliiCrossFadeVolume) (plidata_t *plidata, int vol);
  void              (*pliiClose) (plidata_t *plidata);
  void              (*pliiPause) (plidata_t *plidata);
  void              (*pliiPlay) (plidata_t *plidata);
  void              (*pliiStop) (plidata_t *plidata);
  ssize_t           (*pliiSeek) (plidata_t *plidata, ssize_t pos);
  ssize_t           (*pliiRate) (plidata_t *plidata, ssize_t rate);
  ssize_t           (*pliiGetDuration) (plidata_t *plidata);
  ssize_t           (*pliiGetTime) (plidata_t *plidata);
  plistate_t        (*pliiState) (plidata_t *plidata);
  int               (*pliiSetAudioDevice) (plidata_t *plidata, const char *dev, plidev_t plidevtype);
  int               (*pliiAudioDeviceList) (plidata_t *plidata, volsinklist_t *sinklist);
  int               (*pliiSupported) (plidata_t *plidata);
  int               (*pliiGetVolume) (plidata_t *plidata);
  plidata_t         *plidata;
} pli_t;

pli_t *
pliInit (const char *plipkg, const char *plinm)
{
  pli_t     *pli;
  char      dlpath [BDJ4_PATH_MAX];

  pli = mdmalloc (sizeof (pli_t));
  pli->plidata = NULL;
  pli->pliiInit = NULL;
  pli->pliiFree = NULL;
  pli->pliiMediaSetup = NULL;
  pli->pliiCrossFade = NULL;
  pli->pliiStartPlayback = NULL;
  pli->pliiCrossFadeVolume = NULL;
  pli->pliiClose = NULL;
  pli->pliiPause = NULL;
  pli->pliiPlay = NULL;
  pli->pliiStop = NULL;
  pli->pliiSeek = NULL;
  pli->pliiRate = NULL;
  pli->pliiGetDuration = NULL;
  pli->pliiGetTime = NULL;
  pli->pliiState = NULL;
  pli->pliiSetAudioDevice = NULL;
  pli->pliiAudioDeviceList = NULL;
  pli->pliiSupported = NULL;
  pli->pliiGetVolume = NULL;

  pathbldMakePath (dlpath, sizeof (dlpath),
      plipkg, sysvarsGetStr (SV_SHLIB_EXT), PATHBLD_MP_DIR_EXEC);
  pli->dlHandle = dylibLoad (dlpath, DYLIB_OPT_NONE);
  if (pli->dlHandle == NULL) {
    fprintf (stderr, "Unable to open library %s\n", dlpath);
    mdfree (pli);
    return NULL;
  }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
  pli->pliiInit = dylibLookup (pli->dlHandle, "pliiInit");
  pli->pliiFree = dylibLookup (pli->dlHandle, "pliiFree");
  pli->pliiMediaSetup = dylibLookup (pli->dlHandle, "pliiMediaSetup");
  pli->pliiCrossFade = dylibLookup (pli->dlHandle, "pliiCrossFade");
  pli->pliiStartPlayback = dylibLookup (pli->dlHandle, "pliiStartPlayback");
  pli->pliiCrossFadeVolume = dylibLookup (pli->dlHandle, "pliiCrossFadeVolume");
  pli->pliiClose = dylibLookup (pli->dlHandle, "pliiClose");
  pli->pliiPause = dylibLookup (pli->dlHandle, "pliiPause");
  pli->pliiPlay = dylibLookup (pli->dlHandle, "pliiPlay");
  pli->pliiStop = dylibLookup (pli->dlHandle, "pliiStop");
  pli->pliiSeek = dylibLookup (pli->dlHandle, "pliiSeek");
  pli->pliiRate = dylibLookup (pli->dlHandle, "pliiRate");
  pli->pliiGetDuration = dylibLookup (pli->dlHandle, "pliiGetDuration");
  pli->pliiGetTime = dylibLookup (pli->dlHandle, "pliiGetTime");
  pli->pliiState = dylibLookup (pli->dlHandle, "pliiState");
  pli->pliiSetAudioDevice = dylibLookup (pli->dlHandle, "pliiSetAudioDevice");
  pli->pliiAudioDeviceList = dylibLookup (pli->dlHandle, "pliiAudioDeviceList");
  pli->pliiSupported = dylibLookup (pli->dlHandle, "pliiSupported");
  pli->pliiGetVolume = dylibLookup (pli->dlHandle, "pliiGetVolume");
#pragma clang diagnostic pop

  if (pli->pliiInit != NULL) {
    pli->plidata = pli->pliiInit (plinm, bdjoptGetStr (OPT_M_PLAYER_ARGS));
  }
  return pli;
}

void
pliFree (pli_t *pli)
{
  if (pli != NULL && pli->pliiFree != NULL) {
    pliClose (pli);
    if (pli->plidata != NULL) {
      pli->pliiFree (pli->plidata);
    }
    if (pli->dlHandle != NULL) {
      dylibClose (pli->dlHandle);
    }
    mdfree (pli);
  }
}


void
pliMediaSetup (pli_t *pli, const char *mediaPath,
    const char *fullMediaPath, int sourceType)
{
  if (pli != NULL && pli->pliiMediaSetup != NULL && mediaPath != NULL) {
    pli->pliiMediaSetup (pli->plidata, mediaPath, fullMediaPath, sourceType);
  }
}

void
pliStartPlayback (pli_t *pli, ssize_t pos, ssize_t speed)
{
  if (pli != NULL && pli->pliiStartPlayback != NULL) {
    pli->pliiStartPlayback (pli->plidata, pos, speed);
  }
}

void
pliCrossFade (pli_t *pli, const char *mediaPath, const char *fullMediaPath, int sourceType)
{
  if (pli != NULL && pli->pliiCrossFade != NULL) {
    pli->pliiCrossFade (pli->plidata, mediaPath, fullMediaPath, sourceType);
  }
}

void
pliCrossFadeVolume (pli_t *pli, int vol)
{
  if (pli != NULL && pli->pliiCrossFadeVolume != NULL) {
    pli->pliiCrossFadeVolume (pli->plidata, vol);
  }
}

void
pliPause (pli_t *pli)
{
  if (pli != NULL && pli->pliiPause != NULL) {
    pli->pliiPause (pli->plidata);
  }
}

void
pliPlay (pli_t *pli)
{
  if (pli != NULL && pli->pliiPlay != NULL) {
    pli->pliiPlay (pli->plidata);
  }
}

void
pliStop (pli_t *pli)
{
  if (pli != NULL && pli->pliiStop != NULL) {
    pli->pliiStop (pli->plidata);
  }
}

ssize_t
pliSeek (pli_t *pli, ssize_t pos)
{
  ssize_t     ret = -1;

  if (pli != NULL && pli->pliiSeek != NULL) {
    ret = pli->pliiSeek (pli->plidata, pos);
  }
  return ret;
}

ssize_t
pliRate (pli_t *pli, ssize_t rate)
{
  ssize_t   ret = 100;

  if (pli != NULL && pli->pliiRate != NULL) {
    ret = pli->pliiRate (pli->plidata, rate);
  }
  return ret;
}

void
pliClose (pli_t *pli)
{
  if (pli != NULL && pli->pliiClose != NULL) {
    pli->pliiClose (pli->plidata);
  }
}

ssize_t
pliGetDuration (pli_t *pli)
{
  ssize_t     duration = 0;

  if (pli != NULL && pli->pliiGetDuration != NULL) {
    duration = pli->pliiGetDuration (pli->plidata);
  }
  return duration;
}

ssize_t
pliGetTime (pli_t *pli)
{
  ssize_t     playTime = 0;

  if (pli != NULL && pli->pliiGetTime != NULL) {
    playTime = pli->pliiGetTime (pli->plidata);
  }
  return playTime;
}

plistate_t
pliState (pli_t *pli)
{
  plistate_t          plistate = PLI_STATE_NONE; /* unknown */

  if (pli != NULL && pli->pliiState != NULL) {
    plistate = pli->pliiState (pli->plidata);
  }
  return plistate;
}

int
pliSetAudioDevice (pli_t *pli, const char *dev, plidev_t plidevtype)
{
  if (pli != NULL && pli->pliiSetAudioDevice != NULL) {
    return pli->pliiSetAudioDevice (pli->plidata, dev, plidevtype);
  }
  return -1;
}

int
pliAudioDeviceList (pli_t *pli, volsinklist_t *sinklist)
{
  if (pli != NULL && pli->pliiAudioDeviceList != NULL) {
    return pli->pliiAudioDeviceList (pli->plidata, sinklist);
  }
  return -1;
}

int
pliSupported (pli_t *pli)
{
  int   rc = PLI_SUPPORT_NONE;

  if (pli != NULL && pli->pliiSupported != NULL) {
    rc = pli->pliiSupported (pli->plidata);
  }
  return rc;
}

const char *
pliStateText (pli_t *pli)
{
  if (pli == NULL) {
    return "unknown";
  }
  if (pli->pliiState == NULL) {
    return "unknown";
  }

  return plistateTxt [pli->pliiState (pli->plidata)];
}

ilist_t *
pliInterfaceList (void)
{
  ilist_t   *interfaces;

  interfaces = dyInterfaceList ("libpli", "pliiDesc");
  return interfaces;
}

/* for debugging */
int
pliGetVolume (pli_t *pli)
{
  int   val = 0;

  if (pli != NULL && pli->pliiGetVolume != NULL) {
    val = pli->pliiGetVolume (pli->plidata);
  }

  return val;
}
