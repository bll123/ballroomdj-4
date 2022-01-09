#if 0

#include "config.h"
#include "configp.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include "pli.h"
#include "tmutil.h"

/* These routines were intended to support running the player */
/* in a separate thread.  They are not currently in use. */
/* changes are required to the call to pliInit() to use this */

/* this would be moved back to pli.h */
typedef enum {
  PLI_CMD_NONE,
  PLI_CMD_INIT,
  PLI_CMD_FREE,
  PLI_CMD_MEDIA_SETUP,
  PLI_CMD_START,
  PLI_CMD_PAUSE,
  PLI_CMD_PLAY,
  PLI_CMD_STOP,
  PLI_CMD_CLOSE,
  PLI_CMD_GET_DURATION,
  PLI_CMD_GET_TIME,
  PLI_CMD_GET_STATE,
} plicmd_t;
void          pliInterface (plidata_t *pliData);
void          pliCmdSet (plidata_t *pliData, plicmd_t cmd, char *mediaPath);
void          pliCmdWait (plidata_t *pliData);

/* changes to plidata_t */
  plicmd_t          cmd;
  plicmdstatus_t    status;
  char              *mediaPath;
  int               count;

void
pliInterface (plidata_t *pliData)
{
  bool    done = false;

  pliData->plData = NULL;
  pliData->duration = 0;
  pliData->playTime = 0;
  pliData->state = PLI_STATE_NONE;
  pliData->cmd = PLI_CMD_NONE;
  pliData->status = PLI_CMD_STATUS_OK;
  pliData->mediaPath = NULL;

  while (! done) {
    switch (pliData->cmd) {
      case PLI_CMD_NONE: {
        break;
      }
      case PLI_CMD_INIT: {
        pliInit (pliData);
        break;
      }
      case PLI_CMD_FREE: {
        pliFree (pliData);
        break;
      }
      case PLI_CMD_MEDIA_SETUP: {
        pliMediaSetup (pliData);
        break;
      }
      case PLI_CMD_START: {
        pliStartPlayback (pliData);
        break;
      }
      case PLI_CMD_PAUSE: {
        pliPause (pliData);
        break;
      }
      case PLI_CMD_PLAY: {
        pliPlay (pliData);
        break;
      }
      case PLI_CMD_STOP: {
        pliStop (pliData);
        break;
      }
      case PLI_CMD_CLOSE: {
        pliClose (pliData);
        done = true;
        break;
      }
      case PLI_CMD_GET_DURATION: {
        pliGetDuration (pliData);
        break;
      }
      case PLI_CMD_GET_TIME: {
        pliGetTime (pliData);
        break;
      }
      case PLI_CMD_GET_STATE: {
        pliState (pliData);
        break;
      }
    }
    pliData->status = PLI_CMD_STATUS_OK;
    pliData->cmd = PLI_CMD_NONE;
    mssleep (10);
  }
  return;
}

void
pliCmdSet (plidata_t *pliData, plicmd_t cmd, char *mediaPath)
{
  pliData->status = PLI_CMD_STATUS_WAIT;
  if (mediaPath != NULL) {
    pliData->mediaPath = mediaPath;
  }
  pliData->cmd = cmd;
}

void
pliCmdWait (plidata_t *pliData)
{
  int     count = 0;
  while (pliData->status == PLI_CMD_STATUS_WAIT) {
    mssleep (20);
    ++count;
  }
  pliData->count = count;
}

#endif
