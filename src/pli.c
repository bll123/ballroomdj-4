#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "bdjopt.h"
#include "pathbld.h"
#include "dylib.h"
#include "pli.h"
#include "portability.h"
#include "sysvars.h"

pli_t *
pliInit (void)
{
  pli_t     *pli;
  char      dlpath [MAXPATHLEN];

  pli = malloc (sizeof (pli_t));
  assert (pli != NULL);
  pli->pliData = NULL;

  pathbldMakePath (dlpath, sizeof (dlpath), "",
      bdjoptGetData (OPT_G_PLAYER_INTFC),
      sysvars [SV_SHLIB_EXT], PATHBLD_MP_EXECDIR);
  pli->dlHandle = dylibLoad (dlpath);
  if (pli->dlHandle == NULL) {
    fprintf (stderr, "Unable to open library %s\n", dlpath);
    free (pli);
    return NULL;
  }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
  pli->pliiInit = dylibLookup (pli->dlHandle, "pliiInit");
  assert (pli->pliiInit != NULL);
  pli->pliiFree = dylibLookup (pli->dlHandle, "pliiFree");
  assert (pli->pliiFree != NULL);
  pli->pliiMediaSetup = dylibLookup (pli->dlHandle, "pliiMediaSetup");
  assert (pli->pliiMediaSetup != NULL);
  pli->pliiStartPlayback = dylibLookup (pli->dlHandle, "pliiStartPlayback");
  assert (pli->pliiStartPlayback != NULL);
  pli->pliiClose = dylibLookup (pli->dlHandle, "pliiClose");
  assert (pli->pliiClose != NULL);
  pli->pliiPause = dylibLookup (pli->dlHandle, "pliiPause");
  assert (pli->pliiPause != NULL);
  pli->pliiPlay = dylibLookup (pli->dlHandle, "pliiPlay");
  assert (pli->pliiPlay != NULL);
  pli->pliiStop = dylibLookup (pli->dlHandle, "pliiStop");
  assert (pli->pliiStop != NULL);
  pli->pliiSeek = dylibLookup (pli->dlHandle, "pliiSeek");
  assert (pli->pliiSeek != NULL);
  pli->pliiRate = dylibLookup (pli->dlHandle, "pliiRate");
  assert (pli->pliiRate != NULL);
  pli->pliiGetDuration = dylibLookup (pli->dlHandle, "pliiGetDuration");
  assert (pli->pliiGetDuration != NULL);
  pli->pliiGetTime = dylibLookup (pli->dlHandle, "pliiGetTime");
  assert (pli->pliiGetTime != NULL);
  pli->pliiState = dylibLookup (pli->dlHandle, "pliiState");
  assert (pli->pliiState != NULL);
#pragma clang diagnostic pop

  pli->pliData = pli->pliiInit ();
  return pli;
}

void
pliFree (pli_t *pli)
{
  if (pli != NULL) {
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
pliMediaSetup (pli_t *pli, char *mediaPath)
{
  if (pli != NULL && mediaPath != NULL) {
    pli->pliiMediaSetup (pli->pliData, mediaPath);
  }
}

void
pliStartPlayback (pli_t *pli, ssize_t pos, ssize_t speed)
{
  if (pli != NULL) {
    pli->pliiStartPlayback (pli->pliData, pos, speed);
  }
}

void
pliPause (pli_t *pli)
{
  if (pli != NULL) {
    pli->pliiPause (pli->pliData);
  }
}

void
pliPlay (pli_t *pli)
{
  if (pli != NULL) {
    pli->pliiPlay (pli->pliData);
  }
}

void
pliStop (pli_t *pli)
{
  if (pli != NULL) {
    pli->pliiStop (pli->pliData);
  }
}

ssize_t
pliSeek (pli_t *pli, ssize_t pos)
{
  ssize_t     ret = -1;

  if (pli != NULL) {
    ret = pli->pliiSeek (pli->pliData, pos);
  }
  return ret;
}

ssize_t
pliRate (pli_t *pli, ssize_t rate)
{
  ssize_t   ret = 100;

  if (pli != NULL) {
    ret = pli->pliiRate (pli->pliData, rate);
  }
  return ret;
}

void
pliClose (pli_t *pli)
{
  if (pli != NULL) {
    pli->pliiClose (pli->pliData);
  }
}

ssize_t
pliGetDuration (pli_t *pli)
{
  ssize_t     duration = 0;

  if (pli != NULL) {
    duration = pli->pliiGetDuration (pli->pliData);
  }
  return duration;
}

ssize_t
pliGetTime (pli_t *pli)
{
  ssize_t     playTime = 0;

  if (pli != NULL) {
    playTime = pli->pliiGetTime (pli->pliData);
  }
  return playTime;
}

plistate_t
pliState (pli_t *pli)
{
  plistate_t          plistate = PLI_STATE_NONE; /* unknown */

  if (pli != NULL) {
    plistate = pli->pliiState (pli->pliData);
  }
  return plistate;
}
