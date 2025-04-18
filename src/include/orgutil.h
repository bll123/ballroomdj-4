/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_ORGUTIL_H
#define INC_ORGUTIL_H

#include "slist.h"
#include "song.h"
#include "tagdef.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  ORG_TAG_BYPASS = TAG_KEY_MAX,
  ORG_WIN_CHARS   = (1 << 0),
  ORG_UNIX_CHARS  = (1 << 1),
  ORG_ALL_CHARS   = ORG_WIN_CHARS | ORG_UNIX_CHARS,
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
  ORG_MOVEMENTNAME,
  ORG_MOVEMENTNUM,
  ORG_WORK,
  ORG_MAX_KEY,
} orgkey_t;

typedef struct org org_t;

org_t   * orgAlloc (const char *orgpath);
void    orgFree (org_t *org);
void    orgSetCleanType (org_t *org, int type);
const char * orgGetFromPath (org_t *org, const char *path, tagdefkey_t tagkey);
char    * orgMakeSongPath (org_t *org, song_t *song, const char *bypass);
void    orgStartIterator (org_t *org, slistidx_t *iteridx);
int     orgIterateTagKey (org_t *org, slistidx_t *iteridx);
int     orgIterateOrgKey (org_t *org, slistidx_t *iteridx);
int     orgGetTagKey (int orgkey);
const char *orgGetText (org_t *org, slistidx_t idx);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_ORGUTIL_H */
