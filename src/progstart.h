#ifndef INC_PROGSTART_H
#define INC_PROGSTART_H

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

typedef bool (*progstartCallback_t)(void *userdata, programstate_t programState);

typedef struct {
  programstate_t      programState;
  ilist_t             *callbacks [STATE_MAX];
  mstime_t            tm;
  char                *progtag;
} progstart_t;

progstart_t     * progstartInit (char *progtag);
void            progstartFree (progstart_t *progstart);
void            progstartSetCallback (progstart_t *progstart,
                    programstate_t cbtype, progstartCallback_t callback,
                    void *udata);
programstate_t  progstartProcess (progstart_t *progstart);
bool            progstartIsRunning (progstart_t *progstart);
programstate_t  progstartShutdownProcess (progstart_t *progstart);
programstate_t  progstartCurrState (progstart_t *progstart);
void            progstartLogTime (progstart_t *progstart, char *label);

#endif /* INC_PROGSTART_H */
