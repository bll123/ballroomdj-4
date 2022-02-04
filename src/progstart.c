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
#include "ilist.h"
#include "tmutil.h"

enum {
  PS_CB,
  PS_SUCCESS,
  PS_USERDATA,
};

static programstate_t progstartProcessLoop (progstart_t *progstart,
    programstate_t finalState);

progstart_t *
progstartInit (char *progtag)
{
  progstart_t     *progstart;
  char            tbuff [40];

  progstart = malloc (sizeof (progstart_t));
  assert (progstart != NULL);
  progstart->programState = STATE_NOT_RUNNING;
  for (programstate_t i = STATE_NOT_RUNNING; i < STATE_MAX; ++i) {
    snprintf (tbuff, sizeof (tbuff), "progstart-cb-%d", i);
    progstart->callbacks [i] = ilistAlloc (tbuff, LIST_ORDERED, NULL);
  }

  progstart->progtag = progtag;
  mstimestart (&progstart->tm);
  return progstart;
}

void
progstartFree (progstart_t *progstart)
{
  if (progstart != NULL) {
    for (programstate_t i = STATE_NOT_RUNNING; i < STATE_MAX; ++i) {
      ilistFree (progstart->callbacks [i]);
    }
    free (progstart);
  }
}

void
progstartSetCallback (progstart_t *progstart,
    programstate_t cbtype, progstartCallback_t callback, void *udata)
{
  ilistidx_t        count;

  if (cbtype >= STATE_MAX) {
    return;
  }

  count = ilistGetCount (progstart->callbacks [cbtype]);
  ilistSetData (progstart->callbacks [cbtype], count, PS_CB, callback);
  ilistSetNum (progstart->callbacks [cbtype], count, PS_SUCCESS, 0);
  ilistSetData (progstart->callbacks [cbtype], count, PS_USERDATA, udata);
}

programstate_t
progstartProcess (progstart_t *progstart)
{
  programstate_t      state;


  if (progstart->programState > STATE_RUNNING) {
    return progstart->programState;
  }

  state = progstartProcessLoop (progstart, STATE_RUNNING);
  if (state == STATE_RUNNING) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "%s running: time-to-start: %ld ms",
        progstart->progtag, mstimeend (&progstart->tm));
  }
  return state;
}

programstate_t
progstartShutdownProcess (progstart_t *progstart)
{
  programstate_t      state;

  if (progstart->programState < STATE_STOPPING) {
    progstart->programState = STATE_STOPPING;
    mstimestart (&progstart->tm);
  }

  state = progstartProcessLoop (progstart, STATE_CLOSED);
  if (state == STATE_CLOSED) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "%s closed: time-to-end: %ld ms",
        progstart->progtag, mstimeend (&progstart->tm));
  }
  return state;
}

inline bool
progstartIsRunning (progstart_t *progstart)
{
  if (progstart == NULL) {
    return false;
  }
  return (progstart->programState == STATE_RUNNING);
}

inline programstate_t
progstartCurrState (progstart_t *progstart)
{
  if (progstart == NULL) {
    return STATE_NOT_RUNNING;
  }
  return progstart->programState;
}

void
progstartLogTime (progstart_t *progstart, char *label)
{
  logMsg (LOG_SESS, LOG_IMPORTANT, "%s %s %ld ms",
      progstart->progtag, label, mstimeend (&progstart->tm));
}

/* internal routines */

static programstate_t
progstartProcessLoop (progstart_t *progstart, programstate_t finalState)
{
  bool                  success = true;
  bool                  tsuccess;
  progstartCallback_t   cb = NULL;
  void                  *userdata = NULL;
  ssize_t               count;

  /* skip over empty states */
  while (ilistGetCount (progstart->callbacks [progstart->programState]) == 0 &&
      progstart->programState != finalState) {
    ++progstart->programState;
  }
  count = ilistGetCount (progstart->callbacks [progstart->programState]);
  if (count > 0) {
    for (ilistidx_t ikey = 0; ikey < count; ++ikey) {
      tsuccess = ilistGetNum (progstart->callbacks [progstart->programState],
          ikey, PS_SUCCESS);
      if (! tsuccess) {
        cb = ilistGetData (progstart->callbacks [progstart->programState],
            ikey, PS_CB);
        userdata = ilistGetData (progstart->callbacks [progstart->programState],
            ikey, PS_USERDATA);
        tsuccess = cb (userdata, progstart->programState);
        ilistSetNum (progstart->callbacks [progstart->programState], ikey,
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
    if (progstart->programState != finalState) {
      ++progstart->programState;
    }
    logMsg (LOG_DBG, LOG_MAIN, "program state: %d", progstart->programState);
  }
  return progstart->programState;
}

