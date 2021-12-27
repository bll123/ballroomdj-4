#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "level.h"
#include "datafile.h"

static datafilekey_t leveldfkeys[] = {
  { LEVEL_LABEL, "label", VALUE_DATA },
  { LEVEL_DEFAULT_FLAG, "default", VALUE_LONG },
  { LEVEL_WEIGHT, "weight", VALUE_LONG },
};
#define LEVEL_DFKEY_COUNT (sizeof (leveldfkeys) / sizeof (datafilekey_t))

datafile_t *
levelAlloc (char *fname)
{
  datafile_t    *df;

  df = datafileAlloc (leveldfkeys, LEVEL_DFKEY_COUNT, fname, DFTYPE_KEY_LONG);
  return df;
}

void
levelFree (datafile_t *df)
{
  datafileFree (df);
}
