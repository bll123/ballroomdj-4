#ifndef INC_DIRLIST_H
#define INC_DIRLIST_H

#include "slist.h"

enum {
  DIRLIST_FILES = 0x01,
  DIRLIST_DIRS = 0x02,
};

/* dirlist.c */
slist_t * dirlistBasicDirList (const char *dir, const char *extension);
slist_t * dirlistRecursiveDirList (const char *dir, int flags);

#endif /* INC_DIRLIST_H */
