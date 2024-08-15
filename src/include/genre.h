/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_GENRE_H
#define INC_GENRE_H

#include "datafile.h"
#include "ilist.h"
#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  GENRE_GENRE,
  GENRE_CLASSICAL_FLAG,
  GENRE_KEY_MAX
} genrekey_t;

typedef struct genre genre_t;

genre_t   *genreAlloc (void);
void      genreFree (genre_t *);
ilistidx_t genreGetCount (genre_t *genres);

const char *genreGetGenre (genre_t *genres, ilistidx_t ikey);
int genreGetClassicalFlag (genre_t *genres, ilistidx_t ikey);

void genreSetGenre (genre_t *genres, ilistidx_t ikey, const char *genredisp);
void genreSetClassicalFlag (genre_t *genres, ilistidx_t ikey, int cflag);

void  genreDeleteLast (genre_t *genres);

void      genreStartIterator (genre_t *genres, ilistidx_t *iteridx);
ilistidx_t genreIterate (genre_t *genres, ilistidx_t *iteridx);
void      genreConv (datafileconv_t *conv);
slist_t   *genreGetList (genre_t *genres);
void      genreSave (genre_t *genres, ilist_t *list);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_GENRE_H */
