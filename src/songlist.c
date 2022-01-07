#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "songlist.h"
#include "datafile.h"
#include "fileop.h"
#include "log.h"

  /* must be sorted in ascii order */
static datafilekey_t songlistdfkeys[] = {
  { "DANCE",  SONGLIST_DANCE, VALUE_DATA, NULL },
  { "FILE",   SONGLIST_FILE,  VALUE_DATA, NULL },
  { "TITLE",  SONGLIST_TITLE, VALUE_DATA, NULL },
};
#define SONGLIST_DFKEY_COUNT (sizeof (songlistdfkeys) / sizeof (datafilekey_t))

songlist_t *
songlistAlloc (char *fname)
{
  songlist_t    *sl;

  sl = malloc (sizeof (songlist_t));
  assert (sl != NULL);
  sl->df = NULL;
  sl->songlist = NULL;

  if (! fileopExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: songlist: missing %s\n", fname);
    return NULL;
  }
  sl->df = datafileAllocParse ("songlist", DFTYPE_KEY_LONG, fname,
      songlistdfkeys, SONGLIST_DFKEY_COUNT, DATAFILE_NO_LOOKUP);
  if (sl->df == NULL) {
    songlistFree (sl);
    return NULL;
  }
  sl->fname = strdup (fname);
  assert (sl->fname != NULL);
  sl->songlist = datafileGetList (sl->df);
  return sl;
}

void
songlistFree (songlist_t *sl)
{
  if (sl != NULL) {
    if (sl->df != NULL) {
      datafileFree (sl->df);
    }
    if (sl->fname != NULL) {
      free (sl->fname);
    }
    free (sl);
  }
}

char *
songlistGetData (songlist_t *sl, long idx, long key)
{
  if ((size_t) idx >= llistGetSize (sl->songlist)) {
    logMsg (LOG_DBG, LOG_BASIC, "end of songlist %s reached", sl->fname);
    return NULL;
  }

  list_t *tlist = llistGetList (sl->songlist, idx);
  return llistGetData (tlist, key);
}
