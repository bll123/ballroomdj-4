#ifndef INC_SONGLIST_H
#define INC_SONGLIST_H

#include "datafile.h"

typedef enum {
  SONGLIST_FILE,
  SONGLIST_TITLE,
  SONGLIST_DANCE,
  SONGLIST_KEY_MAX
} songlistkey_t;

datafile_t *  songlistAlloc (char *);
void          songlistFree (datafile_t *);

#endif /* INC_SONGLIST_H */
