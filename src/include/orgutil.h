#ifndef INC_ORGUTIL_H
#define INC_ORGUTIL_H

#include "slist.h"
#include "song.h"
#include "tagdef.h"

typedef struct org org_t;

org_t   * orgAlloc (char *orgpath);
void    orgFree (org_t *org);
slist_t *orgGetList (org_t *org);
char    * orgGetFromPath (org_t *org, const char *path, tagdefkey_t tagkey);
char    * orgMakeSongPath (org_t *org, song_t *song);


#endif /* INC_ORGUTIL_H */
