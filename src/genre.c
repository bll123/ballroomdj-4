#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdjvarsdf.h"
#include "datafile.h"
#include "fileop.h"
#include "genre.h"
#include "log.h"
#include "ilist.h"
#include "slist.h"

  /* must be sorted in ascii order */
static datafilekey_t genredfkeys[] = {
  { "CLASSICAL",  GENRE_CLASSICAL_FLAG, VALUE_NUM, parseConvBoolean, -1 },
  { "GENRE",      GENRE_GENRE,          VALUE_DATA, NULL, -1 },
};
#define GENRE_DFKEY_COUNT (sizeof (genredfkeys) / sizeof (datafilekey_t))

genre_t *
genreAlloc (char *fname)
{
  genre_t       *genre;
  slist_t       *dflist;
  ilistidx_t    gkey;
  ilistidx_t    iteridx;

  if (! fileopExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: genre: missing %s", fname);
    return NULL;
  }

  genre = malloc (sizeof (genre));
  assert (genre != NULL);

  genre->df = datafileAllocParse ("genre", DFTYPE_INDIRECT, fname,
      genredfkeys, GENRE_DFKEY_COUNT, GENRE_GENRE);
  ilistDumpInfo (datafileGetList (genre->df));

  dflist = datafileGetList (genre->df);
  genre->genreList = slistAlloc ("sortopt-disp", LIST_UNORDERED, free, NULL);
  ilistStartIterator (dflist, &iteridx);
  while ((gkey = ilistIterateKey (dflist, &iteridx)) >= 0) {
    slistSetNum (genre->genreList,
        ilistGetData (dflist, gkey, GENRE_GENRE), gkey);
  }
  slistSort (genre->genreList);

  return genre;
}

void
genreFree (genre_t *genre)
{
  if (genre != NULL) {
    if (genre->df != NULL) {
      datafileFree (genre->df);
    }
    if (genre->genreList != NULL) {
      slistFree (genre->genreList);
    }
    free (genre);
  }
}

void
genreConv (char *keydata, datafileret_t *ret)
{
  genre_t     *genre;
  slist_t      *lookup;

  ret->valuetype = VALUE_NUM;
  genre = bdjvarsdfGet (BDJVDF_GENRES);
  lookup = datafileGetLookup (genre->df);
  ret->u.num = slistGetNum (lookup, keydata);
}

slist_t *
genreGetList (genre_t *genre)
{
  if (genre == NULL) {
    return NULL;
  }

  return genre->genreList;
}
