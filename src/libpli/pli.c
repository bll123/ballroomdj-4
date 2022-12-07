/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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
#include "dylib.h"
#include "pli.h"
#include "sysvars.h"
#include "volsink.h"

char *plistateTxt [PLI_STATE_MAX] = {
  [PLI_STATE_NONE] = "none",
  [PLI_STATE_OPENING] = "opening",
  [PLI_STATE_BUFFERING] = "buffering",
  [PLI_STATE_PLAYING] = "playing",
  [PLI_STATE_PAUSED] = "paused",
  [PLI_STATE_STOPPED] = "stopped",
  [PLI_STATE_ENDED] = "ended",
  [PLI_STATE_ERROR] = "error",
};

pli_t *
pliInit (const char *plipkg, const char *sinkname)
{
  pli_t     *pli;
  char      dlpath [MAXPATHLEN];

  pli = malloc (sizeof (pli_t));
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

  pathbldMakePath (dlpath, sizeof (dlpath),
      plipkg, sysvarsGetStr (SV_SHLIB_EXT), PATHBLD_MP_DIR_EXEC);
  pli->dlHandle = dylibLoad (dlpath);
  if (pli->dlHandle == NULL) {
    fprintf (stderr, "Unable to open library %s\n", dlpath);
    free (pli);
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
#pragma clang diagnostic pop

  if (pli->pliiInit != NULL) {
    pli->pliData = pli->pliiInit (plipkg, sinkname);
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
    free (pli);
  }
}

void
pliMediaSetup (pli_t *pli, const char *mediaPath)
{
  if (pli != NULL && pli->pliiMediaSetup != NULL && mediaPath != NULL) {
    pli->pliiMediaSetup (pli->pliData, mediaPath);
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
pliSetAudioDevice (pli_t *pli, const char *dev)
{
  if (pli != NULL && pli->pliiSetAudioDevice != NULL) {
    return pli->pliiSetAudioDevice (pli->pliData, dev);
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
