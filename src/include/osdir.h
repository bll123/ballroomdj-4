/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_OSDIR_H
#define INC_OSDIR_H

#include "config.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct dirhandle dirhandle_t;

[[nodiscard]] dirhandle_t   * osDirOpen (const char *dir);
char          * osDirIterate (dirhandle_t *dirh);
void          osDirClose (dirhandle_t *dirh);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_OSDIR_H */
