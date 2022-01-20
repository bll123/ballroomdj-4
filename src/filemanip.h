#ifndef INC_FILEMANIP_H
#define INC_FILEMANIP_H

#include "slist.h"

int     filemanipCopy (char *from, char *to);
int     filemanipLinkCopy (char *from, char *to);
int     filemanipMove (char *from, char *to);
slist_t  *filemanipBasicDirList (char *dir, char *extension);

#endif /* INC_FILEMANIP_H */
