/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_DIROP_H
#define INC_DIROP_H

enum {
  DIROP_ALL             = 0x0000,
  DIROP_ONLY_IF_EMPTY   = 0x0001,
};

int   diropMakeDir (const char *dirname);
bool  diropDeleteDir (const char *dir, int flags);

#endif /* INC_DIROP_H */
