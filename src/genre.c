#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "genre.h"
#include "datafile.h"
#include "fileop.h"

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
    return NULL;
  }
  df = datafileAlloc ("genre", genredfkeys, GENRE_DFKEY_COUNT, fname, DFTYPE_KEY_LONG);
  return df;
}

void
genreFree (datafile_t *df)
{
  datafileFree (df);
}
