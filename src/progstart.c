#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>

#include "log.h"
#include "progstart.h"

progstart_t *
progstartInit (void)
{
  progstart_t     *progstart;

  progstart = malloc (sizeof (progstart_t));
  assert (progstart != NULL);

  progstart->programState = STATE_INITIALIZING;
  for (programstate_t i = STATE_NOT_RUNNING; i < STATE_MAX; ++i) {
    progstart->callbacks [i] = NULL;
  }

  return progstart;
}

void
progstartFree (progstart_t *progstart)
{
  if (progstart != NULL) {
    free (progstart);
  }
}

void
progstartSetCallback (progstart_t *progstart,
    programstate_t cbtype, progstartCallback_t callback)
{
  if (cbtype >= STATE_MAX) {
    return;
  }

  progstart->callbacks [cbtype] = callback;
}

programstate_t
progstartProcess (progstart_t *progstart, void *userdata)
{
  bool  success;

  if (progstart->programState > STATE_RUNNING) {
    return progstart->programState;
  }

  success = true;
  if (progstart->callbacks [progstart->programState] != NULL) {
    success = progstart->callbacks [progstart->programState] (userdata,
        progstart->programState);
  }
  if (success) {
    if (progstart->programState != STATE_RUNNING) {
      ++progstart->programState;
    }
    logMsg (LOG_DBG, LOG_MAIN, "program state: %d", progstart->programState);
  }
  return progstart->programState;
}

programstate_t
progstartShutdownProcess (progstart_t *progstart, void *userdata)
{
  bool  success;

  if (progstart->programState < STATE_STOPPING) {
    progstart->programState = STATE_STOPPING;
  }

  success = true;
  if (progstart->callbacks [progstart->programState] != NULL) {
    success = progstart->callbacks [progstart->programState] (userdata,
        progstart->programState);
  }
  if (success) {
    if (progstart->programState != STATE_CLOSED) {
      ++progstart->programState;
    }
    logMsg (LOG_DBG, LOG_MAIN, "program state: %d", progstart->programState);
  }
  return progstart->programState;
}

inline bool
progstartIsRunning (progstart_t *progstart)
{
  return (progstart->programState == STATE_RUNNING);
}
