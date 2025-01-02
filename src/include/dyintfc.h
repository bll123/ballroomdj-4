/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_DYINTFC_H
#define INC_DYINTFC_H

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

#endif /* INC_DYINTFC_H */
