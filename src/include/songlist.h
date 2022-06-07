#ifndef INC_SONGLIST_H
#define INC_SONGLIST_H

#include "datafile.h"
#include "ilist.h"

typedef enum {
  SONGLIST_FILE,
  SONGLIST_TITLE,
  SONGLIST_DANCE,
  SONGLIST_DANCESTR,
  SONGLIST_KEY_MAX
} songlistkey_t;

typedef struct songlist songlist_t;

songlist_t * songlistAlloc (const char *);
songlist_t * songlistCreate (const char *fname);
void songlistFree (songlist_t *);
void songlistStartIterator (songlist_t *sl, ilistidx_t *iteridx);
ilistidx_t songlistIterate (songlist_t *sl, ilistidx_t *iteridx);
char * songlistGetStr (songlist_t *sl, ilistidx_t ikey, ilistidx_t lidx);
void songlistSetNum (songlist_t *sl, ilistidx_t ikey, ilistidx_t lidx, ilistidx_t val);
void songlistSetStr (songlist_t *sl, ilistidx_t ikey, ilistidx_t lidx, const char *sval);
void songlistSave (songlist_t *sl);

#endif /* INC_SONGLIST_H */
