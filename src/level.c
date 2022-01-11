#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "level.h"
#include "datafile.h"
#include "fileop.h"
#include "bdjvarsdf.h"
#include "log.h"

  /* must be sorted in ascii order */
static datafilekey_t leveldfkeys[] = {
  { "default",  LEVEL_DEFAULT_FLAG, VALUE_LONG, parseConvBoolean },
  { "label",    LEVEL_LABEL,        VALUE_DATA, NULL },
  { "weight",   LEVEL_WEIGHT,       VALUE_LONG, NULL },
};
#define LEVEL_DFKEY_COUNT (sizeof (leveldfkeys) / sizeof (datafilekey_t))

level_t *
levelAlloc (char *fname)
{
  level_t    *level;

  if (! fileopExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: level: missing %s", fname);
    return NULL;
  }

  level = malloc (sizeof (level_t));
  assert (level != NULL);

  level->df = datafileAllocParse ("level", DFTYPE_KEY_LONG, fname,
      leveldfkeys, LEVEL_DFKEY_COUNT, LEVEL_LABEL);
  level->level = datafileGetList (level->df);
  llistDumpInfo (level->level);
  return level;
}

void
levelFree (level_t *level)
{
  if (level != NULL) {
    if (level->df != NULL) {
      datafileFree (level->df);
    }
    free (level);
  }
}

long
levelGetWeight (level_t *level, long idx)
{
  list_t  *list = llistGetList (level->level, idx);
  return llistGetLong (list, LEVEL_WEIGHT);
}

void
levelConv (char *keydata, datafileret_t *ret)
{
  level_t     *level;
  list_t      *lookup;

  ret->valuetype = VALUE_LONG;

  level = bdjvarsdf [BDJVDF_LEVELS];
  lookup = datafileGetLookup (level->df);
  ret->u.l = listGetLong (lookup, keydata);
}
