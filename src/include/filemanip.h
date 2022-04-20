#ifndef INC_FILEMANIP_H
#define INC_FILEMANIP_H

#include "slist.h"

enum {
  FILEMANIP_FILES = 0x01,
  FILEMANIP_DIRS = 0x02,
};

int     filemanipCopy (const char *from, const char *to);
int     filemanipLinkCopy (char *from, char *to);
int     filemanipMove (char *from, char *to);
void    filemanipDeleteDir (const char *dir);
slist_t * filemanipBasicDirList (char *dir, char *extension);
slist_t * filemanipRecursiveDirList (char *dir, int flags);

#endif /* INC_FILEMANIP_H */
