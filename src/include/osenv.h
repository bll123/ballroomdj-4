/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_OSENV_H
#define INC_OSENV_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "config.h"

void    osGetEnv (const char *name, char *buff, size_t sz);
int     osSetEnv (const char *name, const char *value);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* INC_OSENV_H */
