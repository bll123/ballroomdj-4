#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "bdjopt.h"
#include "datautil.h"
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

  datautilMakePath (dlpath, sizeof (dlpath), "",
      bdjoptGetData (OPT_G_PLAYER_INTFC),
      sysvars [SV_SHLIB_EXT], DATAUTIL_MP_EXECPREFIX);
  pli->dlHandle = dylibLoad (dlpath);
  if (pli->dlHandle == NULL) {
    free (pli);
    return NULL;
  }

  pli->pliiInit = dylibLookup (pli->dlHandle, "pliiInit");
  pli->pliiFree = dylibLookup (pli->dlHandle, "pliiFree");
  pli->pliiMediaSetup = dylibLookup (pli->dlHandle, "pliiMediaSetup");
  pli->pliiStartPlayback = dylibLookup (pli->dlHandle, "pliiStartPlayback");
  pli->pliiClose = dylibLookup (pli->dlHandle, "pliiClose");
  pli->pliiPause = dylibLookup (pli->dlHandle, "pliiPause");
  pli->pliiPlay = dylibLookup (pli->dlHandle, "pliiPlay");
  pli->pliiStop = dylibLookup (pli->dlHandle, "pliiStop");
  pli->pliiGetDuration = dylibLookup (pli->dlHandle, "pliiGetDuration");
  pli->pliiGetTime = dylibLookup (pli->dlHandle, "pliiGetTime");
  pli->pliiState = dylibLookup (pli->dlHandle, "pliiState");

  pli->pliData = pli->pliiInit ();
  return pli;
}

void
pliFree (pli_t *pli)
{
  if (pli != NULL) {
    pliClose (pli);
    dylibClose (pli->dlHandle);
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
pliStartPlayback (pli_t *pli)
{
  if (pli != NULL) {
    pli->pliiStartPlayback (pli->pliData);
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
