#ifndef INC_GENRE_H
#define INC_GENRE_H

#include "datafile.h"

typedef enum {
  GENRE_GENRE,
  GENRE_CLASSICAL_FLAG,
  GENRE_KEY_MAX
} genrekey_t;

datafile_t *  genreAlloc (char *);
void          genreFree (datafile_t *);

#endif /* INC_GENRE_H */
