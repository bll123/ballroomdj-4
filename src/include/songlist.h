/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_SONGLIST_H
#define INC_SONGLIST_H

#include "datafile.h"
#include "ilist.h"

typedef enum {
  SONGLIST_URI,
  SONGLIST_TITLE,
  SONGLIST_DANCE,
  SONGLIST_KEY_MAX
} songlistkey_t;

enum {
  SONGLIST_UPDATE_TIMESTAMP,
  SONGLIST_PRESERVE_TIMESTAMP,
};

enum {
  SONGLIST_USE_DIST_VERSION = -3,
};

typedef struct songlist songlist_t;

songlist_t * songlistCreate (const char *fname);
songlist_t * songlistLoad (const char *fname);
void songlistFree (songlist_t *);
bool songlistExists (const char *name);
int  songlistGetCount (songlist_t *);
void songlistStartIterator (songlist_t *sl, ilistidx_t *iteridx);
ilistidx_t songlistIterate (songlist_t *sl, ilistidx_t *iteridx);
ilistidx_t songlistGetNum (songlist_t *sl, ilistidx_t ikey, ilistidx_t lidx);
const char * songlistGetStr (songlist_t *sl, ilistidx_t ikey, ilistidx_t lidx);
void songlistSetNum (songlist_t *sl, ilistidx_t ikey, ilistidx_t lidx, ilistidx_t val);
void songlistSetStr (songlist_t *sl, ilistidx_t ikey, ilistidx_t lidx, const char *sval);
void songlistClear (songlist_t *sl);
void songlistSave (songlist_t *sl, int tmflag, int distvers);
int songlistDistVersion (songlist_t *sl);

#endif /* INC_SONGLIST_H */
