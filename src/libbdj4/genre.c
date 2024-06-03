/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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

genre_t *
genreAlloc (void)
{
  genre_t       *genre = NULL;
  ilistidx_t    gkey;
  ilistidx_t    iteridx;
  char          fname [MAXPATHLEN];

  pathbldMakePath (fname, sizeof (fname), "genres",
      BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  if (! fileopFileExists (fname)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: genre: missing %s", fname);
    return NULL;
  }

  genre = mdmalloc (sizeof (genre_t));

  genre->path = mdstrdup (fname);
  genre->df = NULL;
  genre->genreList = NULL;

  genre->df = datafileAllocParse ("genre", DFTYPE_INDIRECT, fname,
      genredfkeys, GENRE_KEY_MAX, DF_NO_OFFSET, NULL);
  genre->genre = datafileGetList (genre->df);
  ilistDumpInfo (genre->genre);

  genre->genreList = slistAlloc ("genre-disp", LIST_UNORDERED, NULL);
  slistSetSize (genre->genreList, ilistGetCount (genre->genre));

  ilistStartIterator (genre->genre, &iteridx);
  while ((gkey = ilistIterateKey (genre->genre, &iteridx)) >= 0) {
    slistSetNum (genre->genreList,
        ilistGetStr (genre->genre, gkey, GENRE_GENRE), gkey);
  }
  slistSort (genre->genreList);
  slistCalcMaxKeyWidth (genre->genreList);

  ilistStartIterator (genre->genre, &iteridx);

  return genre;
}

void
genreFree (genre_t *genre)
{
  if (genre != NULL) {
    dataFree (genre->path);
    datafileFree (genre->df);
    slistFree (genre->genreList);
    mdfree (genre);
  }
}

int
genreGetCount (genre_t *genre)
{
  return ilistGetCount (genre->genre);
}

const char *
genreGetGenre (genre_t *genre, ilistidx_t ikey)
{
  return ilistGetStr (genre->genre, ikey, GENRE_GENRE);
}

ssize_t
genreGetClassicalFlag (genre_t *genre, ilistidx_t ikey)
{
  return ilistGetNum (genre->genre, ikey, GENRE_CLASSICAL_FLAG);
}

void
genreStartIterator (genre_t *genre, ilistidx_t *iteridx)
{
  ilistStartIterator (genre->genre, iteridx);
}

ilistidx_t
genreIterate (genre_t *genre, ilistidx_t *iteridx)
{
  return ilistIterateKey (genre->genre, iteridx);
}

void
genreConv (datafileconv_t *conv)
{
  genre_t     *genre;
  ssize_t     num;

  genre = bdjvarsdfGet (BDJVDF_GENRES);

  if (conv->invt == VALUE_STR) {
    num = slistGetNum (genre->genreList, conv->str);
    conv->outvt = VALUE_NUM;
    conv->num = num;
  } else if (conv->invt == VALUE_NUM) {
    if (conv->num == LIST_VALUE_INVALID) {
      conv->str = "";
    } else {
      num = conv->num;
      conv->outvt = VALUE_STR;
      conv->str = ilistGetStr (genre->genre, num, GENRE_GENRE);
    }
  }
}

slist_t *
genreGetList (genre_t *genre)
{
  if (genre == NULL) {
    return NULL;
  }

  return genre->genreList;
}

void
genreSave (genre_t *genre, ilist_t *list)
{
  datafileSave (genre->df, NULL, list, DF_NO_OFFSET,
      datafileDistVersion (genre->df));
}
