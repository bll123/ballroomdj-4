#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "songlist.h"
#include "datafile.h"
#include "fileop.h"

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
    return NULL;
  }
  df = datafileAlloc (songlistdfkeys, SONGLIST_DFKEY_COUNT, fname, DFTYPE_KEY_LONG);
  return df;
}

void
songlistFree (datafile_t *df)
{
  datafileFree (df);
}
