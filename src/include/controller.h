/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */

#ifndef INC_CONTROLLER_H
#define INC_CONTROLLER_H

#include "callback.h"
#include "ilist.h"

enum {
  CONTROLLER_NONE,
  CONTROLLER_PLAY,
  CONTROLLER_PLAYPAUSE,
  CONTROLLER_PAUSE,
  CONTROLLER_STOP,
  CONTROLLER_NEXT,
  CONTROLLER_PREVIOUS,
  CONTROLLER_SEEK,      // long arg
  CONTROLLER_SET_POS,   // long arg
  CONTROLLER_URI,       // string arg
  CONTROLLER_RAISE,
  CONTROLLER_QUIT,
  CONTROLLER_GET_REPEAT_STATUS,
  CONTROLLER_GET_PLAYBACK_STATUS,
  CONTROLLER_GET_RATE,
};

typedef struct controller controller_t;
typedef struct contdata contdata_t;

const char *controllerDesc (void);
controller_t *controllerInit (const char *contpkg);
void controllerFree (controller_t *cont);
void controllerSetup (controller_t *cont);
bool controllerCheckReady (controller_t *cont);
void controllerSetCallback (controller_t *cont, callback_t *cb);
void controllerSetPlayState (controller_t *cont, int state);
void controllerSetRepeatState (controller_t *cont, bool state);
void controllerSetPosition (controller_t *cont, double pos);
ilist_t *controllerInterfaceList (void);

void contiDesc (char **ret, int max);
contdata_t *contiInit (const char *instname);
void contiFree (contdata_t *contdata);
void contiSetup (contdata_t *contdata);
bool contiCheckReady (contdata_t *contdata);
void contiSetCallback (contdata_t *contdata, callback_t *cb);
void contiSetPlayState (contdata_t *contdata, int state);
void contiSetRepeatState (contdata_t *contdata, bool state);
void contiSetPosition (contdata_t *contdata, double pos);

#endif /* INC_CONTROLLER_H */
