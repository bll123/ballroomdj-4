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
#include "progstate.h"
#include "ilist.h"
#include "tmutil.h"

enum {
  PS_CB,
  PS_SUCCESS,
  PS_USERDATA,
};

typedef struct progstate {
  programstate_t      programState;
  ilist_t             *callbacks [STATE_MAX];
  mstime_t            tm;
  char                *progtag;
} progstate_t;

/* for debugging */
static char *progstatetext [] = {
  [STATE_NOT_RUNNING] = "not_running",
  [STATE_INITIALIZING] = "initializing",
  [STATE_LISTENING] = "listening",
  [STATE_CONNECTING] = "connecting",
  [STATE_WAIT_HANDSHAKE] = "wait_handshake",
  [STATE_INITIALIZE_DATA] = "initialize_data",
  [STATE_RUNNING] = "running",
  [STATE_STOPPING] = "stopping",
  [STATE_STOP_WAIT] = "stop_wait",
  [STATE_CLOSING] = "closing",
  [STATE_CLOSED] = "closed",
};

static programstate_t progstateProcessLoop (progstate_t *progstate,
    programstate_t finalState);

progstate_t *
progstateInit (char *progtag)
{
  progstate_t     *progstate;
  char            tbuff [40];

  progstate = malloc (sizeof (progstate_t));
  assert (progstate != NULL);
  progstate->programState = STATE_NOT_RUNNING;
  for (programstate_t i = STATE_NOT_RUNNING; i < STATE_MAX; ++i) {
    snprintf (tbuff, sizeof (tbuff), "progstate-cb-%d", i);
    progstate->callbacks [i] = ilistAlloc (tbuff, LIST_ORDERED);
  }

  progstate->progtag = progtag;
  mstimestart (&progstate->tm);
  return progstate;
}

void
progstateFree (progstate_t *progstate)
{
  if (progstate != NULL) {
    for (programstate_t i = STATE_NOT_RUNNING; i < STATE_MAX; ++i) {
      ilistFree (progstate->callbacks [i]);
    }
    free (progstate);
  }
}

void
progstateSetCallback (progstate_t *progstate,
    programstate_t cbtype, progstateCallback_t callback, void *udata)
{
  ilistidx_t        count;

  if (cbtype >= STATE_MAX) {
    return;
  }

  count = ilistGetCount (progstate->callbacks [cbtype]);
  ilistSetData (progstate->callbacks [cbtype], count, PS_CB, callback);
  ilistSetNum (progstate->callbacks [cbtype], count, PS_SUCCESS, 0);
  ilistSetData (progstate->callbacks [cbtype], count, PS_USERDATA, udata);
}

programstate_t
progstateProcess (progstate_t *progstate)
{
  programstate_t      state;


  if (progstate->programState > STATE_RUNNING) {
    return progstate->programState;
  }

  state = progstateProcessLoop (progstate, STATE_RUNNING);
  if (state == STATE_RUNNING) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "%s running: time-to-start: %ld ms",
        progstate->progtag, mstimeend (&progstate->tm));
  }
  return state;
}

programstate_t
progstateShutdownProcess (progstate_t *progstate)
{
  programstate_t      state;

  if (progstate->programState < STATE_STOPPING) {
    progstate->programState = STATE_STOPPING;
    mstimestart (&progstate->tm);
  }

  state = progstateProcessLoop (progstate, STATE_CLOSED);
  if (state == STATE_CLOSED) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "%s closed: time-to-end: %ld ms",
        progstate->progtag, mstimeend (&progstate->tm));
  }
  return state;
}

inline bool
progstateIsRunning (progstate_t *progstate)
{
  if (progstate == NULL) {
    return false;
  }
  return (progstate->programState == STATE_RUNNING);
}

inline programstate_t
progstateCurrState (progstate_t *progstate)
{
  if (progstate == NULL) {
    return STATE_NOT_RUNNING;
  }
  return progstate->programState;
}

void
progstateLogTime (progstate_t *progstate, char *label)
{
  logMsg (LOG_SESS, LOG_IMPORTANT, "%s %s %ld ms",
      progstate->progtag, label, mstimeend (&progstate->tm));
}

/* internal routines */

static programstate_t
progstateProcessLoop (progstate_t *progstate, programstate_t finalState)
{
  bool                  success = true;
  bool                  tsuccess;
  progstateCallback_t   cb = NULL;
  void                  *userdata = NULL;
  ssize_t               count;

  /* skip over empty states */
  while (ilistGetCount (progstate->callbacks [progstate->programState]) == 0 &&
      progstate->programState != finalState) {
    ++progstate->programState;
  }
  count = ilistGetCount (progstate->callbacks [progstate->programState]);
  if (count > 0) {
    for (ilistidx_t ikey = 0; ikey < count; ++ikey) {
      tsuccess = ilistGetNum (progstate->callbacks [progstate->programState],
          ikey, PS_SUCCESS);
      if (! tsuccess) {
        cb = ilistGetData (progstate->callbacks [progstate->programState],
            ikey, PS_CB);
        userdata = ilistGetData (progstate->callbacks [progstate->programState],
            ikey, PS_USERDATA);
        tsuccess = cb (userdata, progstate->programState);
        ilistSetNum (progstate->callbacks [progstate->programState], ikey,
            PS_SUCCESS, tsuccess);
        if (! tsuccess) {
          /* if any one callback in the list is not finished, */
          /* stay in this state */
          success = false;
        }
      }
    }
  }
  if (success) {
    if (progstate->programState != finalState) {
      ++progstate->programState;
    }
    logMsg (LOG_DBG, LOG_MAIN, "program state: %d %s",
        progstate->programState, progstatetext [progstate->programState]);
  }
  return progstate->programState;
}

