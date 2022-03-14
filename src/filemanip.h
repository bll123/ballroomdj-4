#ifndef INC_FILEMANIP_H
#define INC_FILEMANIP_H

#include "slist.h"

int             filemanipCopy (char *from, char *to);
int             filemanipLinkCopy (char *from, char *to);
int             filemanipMove (char *from, char *to);
void            filemanipDeleteDir (const char *dir);
slist_t         * filemanipBasicDirList (char *dir, char *extension);
slist_t         * filemanipRecursiveDirList (char *dir);


#endif /* INC_FILEMANIP_H */
