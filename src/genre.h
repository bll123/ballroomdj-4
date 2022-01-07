#ifndef INC_GENRE_H
#define INC_GENRE_H

#include "datafile.h"

typedef enum {
  GENRE_GENRE,
  GENRE_CLASSICAL_FLAG,
  GENRE_KEY_MAX
} genrekey_t;

typedef struct {
  datafile_t        *df;
} genre_t;

genre_t       *genreAlloc (char *);
void          genreFree (genre_t *);
void          genreConv (char *keydata, datafileret_t *ret);

#endif /* INC_GENRE_H */
