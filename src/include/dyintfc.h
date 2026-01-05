/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "ilist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  DYI_LIB,
  DYI_DESC,
};

ilist_t * dyInterfaceList (const char *pfx, const char *funcnm);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

