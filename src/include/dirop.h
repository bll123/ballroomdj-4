/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_DIROP_H
#define INC_DIROP_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  DIROP_ALL             = 0,
  DIROP_ONLY_IF_EMPTY   = (1 << 0),
};

int   diropMakeDir (const char *dirname);
bool  diropDeleteDir (const char *dir, int flags);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_DIROP_H */
