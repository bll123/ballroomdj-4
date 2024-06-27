/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_CONTROLLER_H
#define INC_CONTROLLER_H

typedef struct controller controller_t;

int controllerInit (const char *instname);
int controllerCleanup (controller_t *cont);

#endif /* INC_CONTROLLER_H */
