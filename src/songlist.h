#ifndef INC_SONGLIST_H
#define INC_SONGLIST_H

#include "list.h"

typedef enum {
  SL_FILE,
  SL_TITLE,
  SL_DANCE,
  SL_KEY_MAX
} songlistkey_t;

typedef struct {
  char        *fname;
  list_t      *songs;
} songlist_t;

songlist_t *songlistAlloc (char *);
void        songlistFree (songlist_t *);
void        songlistLoad (songlist_t *, char *);
void        songlistCleanup (void);

#endif /* INC_SONGLIST_H */
