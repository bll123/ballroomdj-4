#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdj4.h"
#include "bdjvarsdf.h"
#include "datafile.h"
#include "fileop.h"
#include "genre.h"
#include "ilist.h"
#include "log.h"
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
  { "CLASSICAL",  GENRE_CLASSICAL_FLAG, VALUE_NUM, convBoolean, -1 },
  { "GENRE",      GENRE_GENRE,          VALUE_STR, NULL, -1 },
};

genre_t *
genreAlloc (void)
{
  genre_t       *genre = NULL;
  slist_t       *dflist = NULL;
  ilistidx_t    gkey;
  ilistidx_t    iteridx;
  char          fname [MAXPATHLEN];

  pathbldMakePath (fname, sizeof (fname), "genres",
      BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
  if (! fileopFileExists (fname)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: genre: missing %s", fname);
    return NULL;
  }

  genre = malloc (sizeof (genre_t));
  assert (genre != NULL);

  genre->path = strdup (fname);
  genre->df = NULL;
  genre->genreList = NULL;

  genre->df = datafileAllocParse ("genre", DFTYPE_INDIRECT, fname,
      genredfkeys, GENRE_KEY_MAX);
  genre->genre = datafileGetList (genre->df);
  ilistDumpInfo (genre->genre);

  dflist = datafileGetList (genre->df);
  genre->genreList = slistAlloc ("genre-disp", LIST_UNORDERED, NULL);
  slistSetSize (genre->genreList, slistGetCount (genre->genre));

  ilistStartIterator (dflist, &iteridx);
  while ((gkey = ilistIterateKey (dflist, &iteridx)) >= 0) {
    slistSetNum (genre->genreList,
        ilistGetStr (dflist, gkey, GENRE_GENRE), gkey);
  }
  slistSort (genre->genreList);

  return genre;
}

void
genreFree (genre_t *genre)
{
  if (genre != NULL) {
    if (genre->path != NULL) {
      free (genre->path);
    }
    if (genre->df != NULL) {
      datafileFree (genre->df);
    }
    if (genre->genreList != NULL) {
      slistFree (genre->genreList);
    }
    free (genre);
  }
}

inline int
genreGetCount (genre_t *genre)
{
  return ilistGetCount (genre->genre);
}

inline char *
genreGetGenre (genre_t *genre, ilistidx_t ikey)
{
  return ilistGetStr (genre->genre, ikey, GENRE_GENRE);
}

inline ssize_t
genreGetClassicalFlag (genre_t *genre, ilistidx_t ikey)
{
  return ilistGetNum (genre->genre, ikey, GENRE_CLASSICAL_FLAG);
}

inline void
genreStartIterator (genre_t *genre, ilistidx_t *iteridx)
{
  ilistStartIterator (genre->genre, iteridx);
}

inline ilistidx_t
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

  conv->allocated = false;
  if (conv->valuetype == VALUE_STR) {
    conv->valuetype = VALUE_NUM;
    num = slistGetNum (genre->genreList, conv->str);
    conv->num = num;
  } else if (conv->valuetype == VALUE_NUM) {
    conv->valuetype = VALUE_STR;
    if (conv->num == LIST_VALUE_INVALID) {
      conv->str = "";
    } else {
      num = conv->num;
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
  datafileSaveIndirect ("genre", genre->path, genredfkeys,
      GENRE_KEY_MAX, list);
}
