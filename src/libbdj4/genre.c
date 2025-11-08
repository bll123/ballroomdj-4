/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "datafile.h"
#include "fileop.h"
#include "genre.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "nodiscard.h"
#include "pathbld.h"
#include "slist.h"

typedef struct genre {
  datafile_t  *df;
  ilist_t     *genre;
  slist_t     *genreList;   // for drop-downs
  char        *path;
} genre_t;

  /* must be sorted in ascii order */
static datafilekey_t genredfkeys [GENRE_KEY_MAX] = {
  { "CLASSICAL",  GENRE_CLASSICAL_FLAG, VALUE_NUM, convBoolean, DF_NORM },
  { "GENRE",      GENRE_GENRE,          VALUE_STR, NULL, DF_NORM },
};

NODISCARD
genre_t *
genreAlloc (void)
{
  genre_t       *genres = NULL;
  ilistidx_t    gkey;
  ilistidx_t    iteridx;
  char          fname [MAXPATHLEN];

  pathbldMakePath (fname, sizeof (fname), "genres",
      BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  if (! fileopFileExists (fname)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: genre: missing %s", fname);
    return NULL;
  }

  genres = mdmalloc (sizeof (genre_t));

  genres->path = mdstrdup (fname);
  genres->df = NULL;
  genres->genreList = NULL;

  genres->df = datafileAllocParse ("genre", DFTYPE_INDIRECT, fname,
      genredfkeys, GENRE_KEY_MAX, DF_NO_OFFSET, NULL);
  genres->genre = datafileGetList (genres->df);
  ilistDumpInfo (genres->genre);

  genres->genreList = slistAlloc ("genre-disp", LIST_UNORDERED, NULL);
  slistSetSize (genres->genreList, ilistGetCount (genres->genre));

  ilistStartIterator (genres->genre, &iteridx);
  while ((gkey = ilistIterateKey (genres->genre, &iteridx)) >= 0) {
    slistSetNum (genres->genreList,
        ilistGetStr (genres->genre, gkey, GENRE_GENRE), gkey);
  }
  slistSort (genres->genreList);

  ilistStartIterator (genres->genre, &iteridx);

  return genres;
}

void
genreFree (genre_t *genres)
{
  if (genres == NULL) {
    return;
  }

  dataFree (genres->path);
  datafileFree (genres->df);
  slistFree (genres->genreList);
  mdfree (genres);
}

ilistidx_t
genreGetCount (genre_t *genres)
{
  if (genres == NULL) {
    return 0;
  }

  return ilistGetCount (genres->genre);
}

const char *
genreGetGenre (genre_t *genres, ilistidx_t ikey)
{
  if (genres == NULL) {
    return NULL;
  }

  return ilistGetStr (genres->genre, ikey, GENRE_GENRE);
}

int
genreGetClassicalFlag (genre_t *genres, ilistidx_t ikey)
{
  int     rc;

  if (genres == NULL) {
    return 0;
  }

  rc = ilistGetNum (genres->genre, ikey, GENRE_CLASSICAL_FLAG);
  if (rc < 0) {
    rc = 0;
  }
  return rc;
}

void
genreSetGenre (genre_t *genres, ilistidx_t ikey, const char *genredisp)
{
  if (genres == NULL) {
    return;
  }

  ilistSetStr (genres->genre, ikey, GENRE_GENRE, genredisp);
}

void
genreSetClassicalFlag (genre_t *genres, ilistidx_t ikey, int cflag)
{
  if (genres == NULL) {
    return;
  }

  ilistSetNum (genres->genre, ikey, GENRE_CLASSICAL_FLAG, cflag);
}

void
genreDeleteLast (genre_t *genres)
{
  ilistidx_t    count;

  if (genres == NULL) {
    return;
  }

  count = ilistGetCount (genres->genre);
  ilistDelete (genres->genre, count - 1);
}

/* for testing */
void
genreStartIterator (genre_t *genres, ilistidx_t *iteridx) /* TESTING */
{
  if (genres == NULL) {
    return;
  }

  ilistStartIterator (genres->genre, iteridx);
}

/* for testing */
ilistidx_t
genreIterate (genre_t *genres, ilistidx_t *iteridx)  /* TESTING */
{
  if (genres == NULL) {
    return LIST_LOC_INVALID;
  }

  return ilistIterateKey (genres->genre, iteridx);
}

void
genreConv (datafileconv_t *conv)
{
  genre_t     *genres;
  slistidx_t  num;

  if (conv == NULL) {
    return;
  }

  genres = bdjvarsdfGet (BDJVDF_GENRES);

  if (conv->invt == VALUE_STR) {
    num = slistGetNum (genres->genreList, conv->str);
    conv->outvt = VALUE_NUM;
    conv->num = num;
  } else if (conv->invt == VALUE_NUM) {
    if (conv->num == LIST_VALUE_INVALID) {
      conv->str = "";
    } else {
      num = conv->num;
      conv->outvt = VALUE_STR;
      conv->str = ilistGetStr (genres->genre, num, GENRE_GENRE);
    }
  }
}

slist_t *
genreGetList (genre_t *genres)
{
  if (genres == NULL) {
    return NULL;
  }

  return genres->genreList;
}

void
genreSave (genre_t *genres, ilist_t *list)
{
  datafileSave (genres->df, NULL, list, DF_NO_OFFSET,
      datafileDistVersion (genres->df));
}
