#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "genre.h"
#include "datafile.h"

static datafilekey_t genredfkeys[] = {
  { GENRE_GENRE, "genre", VALUE_DATA },
  { GENRE_CLASSICAL_FLAG, "classical", VALUE_LONG },
};
#define GENRE_DFKEY_COUNT (sizeof (genredfkeys) / sizeof (datafilekey_t))

datafile_t *
genreAlloc (char *fname)
{
  datafile_t    *df;

  df = datafileAlloc (genredfkeys, GENRE_DFKEY_COUNT, fname);
  return df;
}

void
genreFree (datafile_t *df)
{
  datafileFree (df);
}
