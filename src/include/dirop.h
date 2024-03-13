/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_DIROP_H
#define INC_DIROP_H

enum {
  DIROP_ALL             = 0,
  DIROP_ONLY_IF_EMPTY   = (1 << 0),
};

int   diropMakeDir (const char *dirname);
bool  diropDeleteDir (const char *dir, int flags);

#endif /* INC_DIROP_H */
