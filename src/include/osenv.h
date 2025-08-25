/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "config.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

void    osGetEnv (const char *name, char *buff, size_t sz);
int     osSetEnv (const char *name, const char *value);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

