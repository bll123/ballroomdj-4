#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "pli.h"
#include "tmutil.h"

plidata_t *
pliiInit (void)
{
  plidata_t *pliData;

  pliData = malloc (sizeof (plidata_t));
  assert (pliData != NULL);
  pliData->plData = NULL;
  pliData->duration = 20000;
  pliData->playTime = 0;
  pliData->state = PLI_STATE_STOPPED;
  pliData->name = "null";
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
  if (pliData != NULL && mediaPath != NULL) {
    pliData->state = PLI_STATE_STOPPED;
    pliData->duration = 20000;
    pliData->playTime = 0;
  }
}

void
pliiStartPlayback (plidata_t *pliData, ssize_t dpos)
{
  if (pliData != NULL) {
    pliData->duration = 20000;
    pliData->playTime = 0;
    mstimestart (&pliData->playStart);
    pliData->state = PLI_STATE_PLAYING;
    pliiSeek (pliData, dpos);
  }
}

void
pliiPause (plidata_t *pliData)
{
  if (pliData != NULL) {
    pliData->state = PLI_STATE_PAUSED;
  }
}

void
pliiPlay (plidata_t *pliData)
{
  if (pliData != NULL) {
    pliData->state = PLI_STATE_PLAYING;
  }
}

void
pliiStop (plidata_t *pliData)
{
  if (pliData != NULL) {
    pliData->state = PLI_STATE_STOPPED;
    pliData->duration = 20000;
    pliData->playTime = 0;
  }
}

ssize_t
pliiSeek (plidata_t *pliData, ssize_t dpos)
{
  ssize_t     dret = dpos;

  if (pliData != NULL) {
    pliData->duration = 20000 - dpos;
  }
  return dret;
}

double
pliiRate (plidata_t *pliData, double drate)
{
  double    dret = -1.0;

  if (pliData != NULL) {
    dret = 1.0;
  }
  return dret;
}

void
pliiClose (plidata_t *pliData)
{
  if (pliData != NULL) {
    pliData->state = PLI_STATE_STOPPED;
  }
}

ssize_t
pliiGetDuration (plidata_t *pliData)
{
  ssize_t     duration = 0;

  if (pliData != NULL) {
    duration = pliData->duration;
  }
  return duration;
}

ssize_t
pliiGetTime (plidata_t *pliData)
{
  ssize_t     playTime = 0;

  if (pliData != NULL) {
    pliData->playTime = mstimeend (&pliData->playStart);
    playTime = pliData->playTime;
  }
  return playTime;
}

plistate_t
pliiState (plidata_t *pliData)
{
  plistate_t          plistate = PLI_STATE_NONE; /* unknown */

  if (pliData != NULL) {
    pliData->playTime = mstimeend (&pliData->playStart);
    if (pliData->playTime >= pliData->duration) {
      pliData->state = PLI_STATE_STOPPED;
    }
    plistate = pliData->state;
  }
  return plistate;
}
