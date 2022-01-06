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

datafile_t *
songlistAlloc (char *fname)
{
  datafile_t    *df;

  if (! fileopExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: songlist: missing %s\n", fname);
    return NULL;
  }
  df = datafileAllocParse ("songlist", DFTYPE_KEY_LONG, fname,
      songlistdfkeys, SONGLIST_DFKEY_COUNT, DATAFILE_NO_LOOKUP);
  return df;
}

void
songlistFree (datafile_t *df)
{
  datafileFree (df);
}
