#ifndef INC_PROGSTATE_H
#define INC_PROGSTATE_H

#include "ilist.h"
#include "tmutil.h"

typedef enum {
  STATE_NOT_RUNNING,
  STATE_INITIALIZING,
  STATE_LISTENING,
  STATE_CONNECTING,
  STATE_WAIT_HANDSHAKE,
  STATE_INITIALIZE_DATA,
  STATE_RUNNING,
  STATE_STOPPING,
  STATE_CLOSING,
  STATE_CLOSED,
  STATE_MAX,
} programstate_t;

typedef bool (*progstateCallback_t)(void *userdata, programstate_t programState);

typedef struct {
  programstate_t      programState;
  ilist_t             *callbacks [STATE_MAX];
  mstime_t            tm;
  char                *progtag;
} progstate_t;

progstate_t     * progstateInit (char *progtag);
void            progstateFree (progstate_t *progstate);
void            progstateSetCallback (progstate_t *progstate,
                    programstate_t cbtype, progstateCallback_t callback,
                    void *udata);
programstate_t  progstateProcess (progstate_t *progstate);
bool            progstateIsRunning (progstate_t *progstate);
programstate_t  progstateShutdownProcess (progstate_t *progstate);
programstate_t  progstateCurrState (progstate_t *progstate);
void            progstateLogTime (progstate_t *progstate, char *label);

#endif /* INC_PROGSTATE_H */
