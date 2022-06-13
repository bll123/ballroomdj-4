#ifndef INC_DIROP_H
#define INC_DIROP_H

#include "slist.h"

enum {
  DIROP_FILES = 0x01,
  DIROP_DIRS = 0x02,
};

slist_t * diropBasicDirList (const char *dir, const char *extension);
slist_t * diropRecursiveDirList (const char *dir, int flags);
void    diropDeleteDir (const char *dir);

#endif /* INC_DIROP_H */
