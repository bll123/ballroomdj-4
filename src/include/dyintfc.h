#ifndef INC_DYINTFC_H
#define INC_DYINTFC_H

#include "ilist.h"

typedef struct {
  const char    *desc;
  int           index;
} dylist_t;

enum {
  DYI_LIB,
  DYI_DESC,
  DYI_INDEX,
};

ilist_t * dyInterfaceList (const char *pfx, const char *funcnm);

#endif /* INC_DYINTFC_H */
