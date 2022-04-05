#ifndef INC_ORGUTIL_H
#define INC_ORGUTIL_H

#include "slist.h"
#include "song.h"

typedef enum {
  ORG_TEXT,
  ORG_ALBUM,
  ORG_ALBUMARTIST,
  ORG_ARTIST,
  ORG_COMPOSER,
  ORG_CONDUCTOR,
  ORG_DANCE,
  ORG_DISC,
  ORG_GENRE,
  ORG_TITLE,
  ORG_TRACKNUM,
  ORG_TRACKNUM0,
  ORG_MAX_KEY,
} orgkey_t;

typedef struct {
  char          *orgpath;
  slist_t       *orgparsed;
} org_t;

org_t   * orgAlloc (char *orgpath);
void    orgFree (org_t *org);
slist_t *orgGetList (org_t *org);
char    * orgMakeSongPath (org_t *org, song_t *song);

#endif /* INC_ORGUTIL_H */
