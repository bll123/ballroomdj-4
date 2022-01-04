#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "level.h"
#include "datafile.h"
#include "fileop.h"
#include "bdjvarsdf.h"

  /* must be sorted in ascii order */
static datafilekey_t leveldfkeys[] = {
  { "default",  LEVEL_DEFAULT_FLAG, VALUE_LONG, parseConvBoolean },
  { "label",    LEVEL_LABEL,        VALUE_DATA, NULL },
  { "weight",   LEVEL_WEIGHT,       VALUE_LONG, NULL },
};
#define LEVEL_DFKEY_COUNT (sizeof (leveldfkeys) / sizeof (datafilekey_t))

datafile_t *
levelAlloc (char *fname)
{
  datafile_t    *df;

  if (! fileopExists (fname)) {
    return NULL;
  }

  df = datafileAllocParse ("level", DFTYPE_KEY_LONG, fname,
      leveldfkeys, LEVEL_DFKEY_COUNT, LEVEL_LABEL);
  return df;
}

void
levelFree (datafile_t *df)
{
  datafileFree (df);
}

void
levelConv (char *keydata, datafileret_t *ret)
{
  list_t      *lookup;

  ret->valuetype = VALUE_LONG;
  lookup = datafileGetLookup (bdjvarsdf [BDJVDF_LEVELS]);
  ret->u.l = listGetLong (lookup, keydata);
}
