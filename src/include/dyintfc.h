#ifndef INC_DYINTFC_H
#define INC_DYINTFC_H

#include "ilist.h"

enum {
  DYI_LIB,
  DYI_DESC,
};

ilist_t * dyInterfaceList (const char *pfx, const char *funcnm);

#endif /* INC_DYINTFC_H */
