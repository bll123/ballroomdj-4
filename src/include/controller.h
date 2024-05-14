/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */

#ifndef INC_CONTROLLER_H
#define INC_CONTROLLER_H

#include "ilist.h"

typedef struct controller controller_t;
typedef struct contdata contdata_t;

const char *controllerDesc (void);
controller_t *controllerInit (const char *contpkg);
void controllerFree (controller_t *cont);
void controllerSetup (controller_t *cont);
bool controllerCheckReady (controller_t *cont);
ilist_t *controllerInterfaceList (void);

void contiDesc (char **ret, int max);
contdata_t *contiInit (const char *instname);
void contiFree (contdata_t *contdata);
void contiSetup (contdata_t *contdata);
bool contiCheckReady (contdata_t *contdata);

#endif /* INC_CONTROLLER_H */
