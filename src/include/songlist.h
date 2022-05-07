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

songlist_t * songlistAlloc (char *);
void songlistFree (songlist_t *);
songlist_t * songlistCreate (char *fname);
char *songlistGetNext (songlist_t *, ilistidx_t idx, ilistidx_t key);
void songlistSetNum (songlist_t *sl, ilistidx_t ikey, ilistidx_t lidx, ilistidx_t val);
void songlistSetStr (songlist_t *sl, ilistidx_t ikey, ilistidx_t lidx, const char *sval);
void songlistSave (songlist_t *sl);

#endif /* INC_SONGLIST_H */
