/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "nodiscard.h"
#include "ilist.h"
#include "tmutil.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  PROGSTATE_NOT_RUNNING,
  PROGSTATE_INITIALIZING,
  PROGSTATE_LISTENING,
  PROGSTATE_CONNECTING,
  PROGSTATE_WAIT_HANDSHAKE,
  PROGSTATE_INITIALIZE_DATA,
  PROGSTATE_RUNNING,
  PROGSTATE_STOPPING,
  PROGSTATE_STOP_WAIT,
  PROGSTATE_CLOSING,
  PROGSTATE_CLOSED,
  PROGSTATE_MAX,
} programstate_t;

typedef bool (*progstateCallback_t)(void *userdata, programstate_t programState);

typedef struct progstate progstate_t;

enum {
  STATE_NOT_FINISH = 0,
  STATE_FINISHED = 1,
};

NODISCARD progstate_t     * progstateInit (char *progtag);
void            progstateFree (progstate_t *progstate);
void            progstateSetCallback (progstate_t *progstate,
                    programstate_t cbtype, progstateCallback_t callback,
                    void *udata);
programstate_t  progstateProcess (progstate_t *progstate);
bool            progstateIsRunning (progstate_t *progstate);
programstate_t  progstateShutdownProcess (progstate_t *progstate);
programstate_t  progstateCurrState (progstate_t *progstate);
void            progstateLogTime (progstate_t *progstate, char *label);
const char      *progstateDebug (progstate_t *progstate);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

