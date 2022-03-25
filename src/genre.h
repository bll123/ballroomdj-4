#ifndef INC_GENRE_H
#define INC_GENRE_H

#include "datafile.h"
#include "ilist.h"
#include "slist.h"

typedef enum {
  GENRE_GENRE,
  GENRE_CLASSICAL_FLAG,
  GENRE_KEY_MAX
} genrekey_t;

typedef struct {
  datafile_t        *df;
  ilist_t           *genre;
  slist_t           *genreList;   // for drop-downs
} genre_t;

genre_t   *genreAlloc (char *);
void      genreFree (genre_t *);
void      genreConv (datafileconv_t *conv);
slist_t   *genreGetList (genre_t *);

#endif /* INC_GENRE_H */
