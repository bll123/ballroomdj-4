#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "songlist.h"
#include "datafile.h"

static datafilekey_t songlistdfkeys[] = {
  { SONGLIST_FILE, "FILE", VALUE_DATA },
  { SONGLIST_TITLE, "TITLE", VALUE_DATA },
  { SONGLIST_DANCE, "DANCE", VALUE_DATA },
};
#define SONGLIST_DFKEY_COUNT (sizeof (songlistdfkeys) / sizeof (datafilekey_t))

datafile_t *
songlistAlloc (char *fname)
{
  datafile_t    *df;

  df = datafileAlloc (songlistdfkeys, SONGLIST_DFKEY_COUNT, fname);
  return df;
}

void
songlistFree (datafile_t *df)
{
  datafileFree (df);
}
