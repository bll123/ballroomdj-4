/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_ORGUTIL_H
#define INC_ORGUTIL_H

#include "slist.h"
#include "song.h"
#include "tagdef.h"

enum {
  ORG_TAG_BYPASS = TAG_KEY_MAX,
};

typedef enum {
  ORG_ALBUM,
  ORG_ALBUMARTIST,
  ORG_ARTIST,
  ORG_BPM,
  ORG_BYPASS,
  ORG_COMPOSER,
  ORG_CONDUCTOR,
  ORG_DANCE,
  ORG_DISC,
  ORG_GENRE,
  ORG_RATING,
  ORG_STATUS,
  ORG_TEXT,         // used internally
  ORG_TITLE,
  ORG_TRACKNUM,
  ORG_TRACKNUM0,
  ORG_MAX_KEY,
} orgkey_t;

typedef struct org org_t;

org_t   * orgAlloc (const char *orgpath);
void    orgFree (org_t *org);
slist_t *orgGetList (org_t *org);
const char * orgGetFromPath (org_t *org, const char *path, tagdefkey_t tagkey);
char    * orgMakeSongPath (org_t *org, song_t *song, const char *bypass);
bool    orgHaveDance (org_t *org);
void    orgStartIterator (org_t *org, slistidx_t *iteridx);
int     orgIterateTagKey (org_t *org, slistidx_t *iteridx);
int     orgIterateOrgKey (org_t *org, slistidx_t *iteridx);
int     orgGetTagKey (int orgkey);
const char *orgGetText (org_t *org, slistidx_t idx);

#endif /* INC_ORGUTIL_H */
