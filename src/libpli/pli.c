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

#include "bdj4.h"
#include "pathbld.h"
#include "dyintfc.h"
#include "dylib.h"
#include "ilist.h"
#include "mdebug.h"
#include "pli.h"
#include "sysvars.h"
#include "volsink.h"

static char *plistateTxt [PLI_STATE_MAX] = {
  [PLI_STATE_NONE] = "none",
  [PLI_STATE_OPENING] = "opening",
  [PLI_STATE_BUFFERING] = "buffering",
  [PLI_STATE_PLAYING] = "playing",
  [PLI_STATE_PAUSED] = "paused",
  [PLI_STATE_STOPPED] = "stopped",
  [PLI_STATE_ENDED] = "ended",
  [PLI_STATE_ERROR] = "error",
};

typedef struct pli {
  dlhandle_t        *dlHandle;
  plidata_t         *(*pliiInit) (const char *plinm);
  void              (*pliiFree) (plidata_t *pliData);
  void              (*pliiMediaSetup) (plidata_t *pliData, const char *mediaPath, const char *fullMediaPath, int sourceType);
  void              (*pliiStartPlayback) (plidata_t *pliData, ssize_t pos, ssize_t speed);
  void              (*pliiClose) (plidata_t *pliData);
  void              (*pliiPause) (plidata_t *pliData);
  void              (*pliiPlay) (plidata_t *pliData);
  void              (*pliiStop) (plidata_t *pliData);
  ssize_t           (*pliiSeek) (plidata_t *pliData, ssize_t pos);
  ssize_t           (*pliiRate) (plidata_t *pliData, ssize_t rate);
  ssize_t           (*pliiGetDuration) (plidata_t *pliData);
  ssize_t           (*pliiGetTime) (plidata_t *pliData);
  plistate_t        (*pliiState) (plidata_t *pliData);
  int               (*pliiSetAudioDevice) (plidata_t *pliData, const char *dev, int plidevtype);
  int               (*pliiAudioDeviceList) (plidata_t *pliData, volsinklist_t *sinklist);
  int               (*pliiSupported) (plidata_t *pliData);
  plidata_t         *pliData;
} pli_t;

pli_t *
pliInit (const char *plipkg, const char *plinm)
{
  pli_t     *pli;
  char      dlpath [MAXPATHLEN];

  pli = mdmalloc (sizeof (pli_t));
  pli->pliData = NULL;
  pli->pliiInit = NULL;
  pli->pliiFree = NULL;
  pli->pliiMediaSetup = NULL;
  pli->pliiStartPlayback = NULL;
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

  pathbldMakePath (dlpath, sizeof (dlpath),
      plipkg, sysvarsGetStr (SV_SHLIB_EXT), PATHBLD_MP_DIR_EXEC);
  pli->dlHandle = dylibLoad (dlpath);
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
  pli->pliiStartPlayback = dylibLookup (pli->dlHandle, "pliiStartPlayback");
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
#pragma clang diagnostic pop

  if (pli->pliiInit != NULL) {
    pli->pliData = pli->pliiInit (plinm);
  }
  return pli;
}

void
pliFree (pli_t *pli)
{
  if (pli != NULL && pli->pliiFree != NULL) {
    pliClose (pli);
    if (pli->pliData != NULL) {
      pli->pliiFree (pli->pliData);
    }
    if (pli->dlHandle != NULL) {
      dylibClose (pli->dlHandle);
    }
    mdfree (pli);
  }
}

#if 0 /* UNUSED */
void
pliCleanup (dlhandle_t *dlHandle)  /* UNUSED */
{
  void (*pliiCleanup) (void);

  pliiCleanup = dylibLookup (dlHandle, "pliiCleanup");
  if (pliiCleanup != NULL) {
    pliiCleanup ();
  }
}
#endif

void
pliMediaSetup (pli_t *pli, const char *mediaPath,
    const char *fullMediaPath, int sourceType)
{
  if (pli != NULL && pli->pliiMediaSetup != NULL && mediaPath != NULL) {
    pli->pliiMediaSetup (pli->pliData, mediaPath, fullMediaPath, sourceType);
  }
}

void
pliStartPlayback (pli_t *pli, ssize_t pos, ssize_t speed)
{
  if (pli != NULL && pli->pliiStartPlayback != NULL) {
    pli->pliiStartPlayback (pli->pliData, pos, speed);
  }
}

void
pliPause (pli_t *pli)
{
  if (pli != NULL && pli->pliiPause != NULL) {
    pli->pliiPause (pli->pliData);
  }
}

void
pliPlay (pli_t *pli)
{
  if (pli != NULL && pli->pliiPlay != NULL) {
    pli->pliiPlay (pli->pliData);
  }
}

void
pliStop (pli_t *pli)
{
  if (pli != NULL && pli->pliiStop != NULL) {
    pli->pliiStop (pli->pliData);
  }
}

ssize_t
pliSeek (pli_t *pli, ssize_t pos)
{
  ssize_t     ret = -1;

  if (pli != NULL && pli->pliiSeek != NULL) {
    ret = pli->pliiSeek (pli->pliData, pos);
  }
  return ret;
}

ssize_t
pliRate (pli_t *pli, ssize_t rate)
{
  ssize_t   ret = 100;

  if (pli != NULL && pli->pliiRate != NULL) {
    ret = pli->pliiRate (pli->pliData, rate);
  }
  return ret;
}

void
pliClose (pli_t *pli)
{
  if (pli != NULL && pli->pliiClose != NULL) {
    pli->pliiClose (pli->pliData);
  }
}

ssize_t
pliGetDuration (pli_t *pli)
{
  ssize_t     duration = 0;

  if (pli != NULL && pli->pliiGetDuration != NULL) {
    duration = pli->pliiGetDuration (pli->pliData);
  }
  return duration;
}

ssize_t
pliGetTime (pli_t *pli)
{
  ssize_t     playTime = 0;

  if (pli != NULL && pli->pliiGetTime != NULL) {
    playTime = pli->pliiGetTime (pli->pliData);
  }
  return playTime;
}

plistate_t
pliState (pli_t *pli)
{
  plistate_t          plistate = PLI_STATE_NONE; /* unknown */

  if (pli != NULL && pli->pliiState != NULL) {
    plistate = pli->pliiState (pli->pliData);
  }
  return plistate;
}

int
pliSetAudioDevice (pli_t *pli, const char *dev, int plidevtype)
{
  if (pli != NULL && pli->pliiSetAudioDevice != NULL) {
    return pli->pliiSetAudioDevice (pli->pliData, dev, plidevtype);
  }
  return -1;
}

int
pliAudioDeviceList (pli_t *pli, volsinklist_t *sinklist)
{
  if (pli != NULL && pli->pliiAudioDeviceList != NULL) {
    return pli->pliiAudioDeviceList (pli->pliData, sinklist);
  }
  return -1;
}

int
pliSupported (pli_t *pli)
{
  int   rc = PLI_SUPPORT_NONE;

  if (pli != NULL && pli->pliiSupported != NULL) {
    rc = pli->pliiSupported (pli->pliData);
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

  return plistateTxt [pli->pliiState (pli->pliData)];
}

ilist_t *
pliInterfaceList (void)
{
  ilist_t   *interfaces;

  interfaces = dyInterfaceList ("libpli", "pliiDesc");
  return interfaces;
}
