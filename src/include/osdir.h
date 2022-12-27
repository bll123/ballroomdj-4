/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_OSDIR_H
#define INC_OSDIR_H

#include "config.h"

typedef struct dirhandle dirhandle_t;

dirhandle_t   * osDirOpen (const char *dir);
char          * osDirIterate (dirhandle_t *dirh);
void          osDirClose (dirhandle_t *dirh);

#endif /* INC_OSDIR_H */
