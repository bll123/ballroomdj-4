#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "dance.h"
#include "datafile.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "songlist.h"

  /* must be sorted in ascii order */
static datafilekey_t songlistdfkeys [SONGLIST_KEY_MAX] = {
  { "DANCE",    SONGLIST_DANCE,     VALUE_NUM, danceConvDance, SONGLIST_DANCESTR },
  { "DANCESTR", SONGLIST_DANCESTR,  VALUE_DATA, NULL, -1 },
  { "FILE",     SONGLIST_FILE,      VALUE_DATA, NULL, -1 },
  { "TITLE",    SONGLIST_TITLE,     VALUE_DATA, NULL, -1 },
};

songlist_t *
songlistAlloc (char *fname)
{
  songlist_t    *sl;

  sl = malloc (sizeof (songlist_t));
  assert (sl != NULL);
  sl->df = NULL;
  sl->songlist = NULL;

  if (! fileopFileExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: songlist: missing %s", fname);
    return NULL;
  }
  sl->df = datafileAllocParse ("songlist", DFTYPE_INDIRECT, fname,
      songlistdfkeys, SONGLIST_KEY_MAX, DATAFILE_NO_LOOKUP);
  if (sl->df == NULL) {
    songlistFree (sl);
    return NULL;
  }
  sl->fname = strdup (fname);
  assert (sl->fname != NULL);
  sl->songlist = datafileGetList (sl->df);
  ilistDumpInfo (sl->songlist);
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
songlistGetData (songlist_t *sl, ilistidx_t ikey, ilistidx_t lidx)
{
  if (ikey >= nlistGetCount (sl->songlist)) {
    logMsg (LOG_DBG, LOG_BASIC, "end of songlist %s reached", sl->fname);
    return NULL;
  }

  return ilistGetData (sl->songlist, ikey, lidx);
}
