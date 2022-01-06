#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "genre.h"
#include "datafile.h"
#include "fileop.h"
#include "bdjvarsdf.h"
#include "log.h"

  /* must be sorted in ascii order */
static datafilekey_t genredfkeys[] = {
  { "classical",  GENRE_CLASSICAL_FLAG, VALUE_LONG, parseConvBoolean },
  { "genre",      GENRE_GENRE,          VALUE_DATA, NULL },
};
#define GENRE_DFKEY_COUNT (sizeof (genredfkeys) / sizeof (datafilekey_t))

datafile_t *
genreAlloc (char *fname)
{
  datafile_t    *df;

  if (! fileopExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: genre: missing %s\n", fname);
    return NULL;
  }
  df = datafileAllocParse ("genre", DFTYPE_KEY_LONG, fname,
      genredfkeys, GENRE_DFKEY_COUNT, GENRE_GENRE);
  return df;
}

void
genreFree (datafile_t *df)
{
  datafileFree (df);
}

void
genreConv (char *keydata, datafileret_t *ret)
{
  list_t      *lookup;

  ret->valuetype = VALUE_LONG;
  lookup = datafileGetLookup (bdjvarsdf [BDJVDF_GENRES]);
  ret->u.l = listGetLong (lookup, keydata);
}

