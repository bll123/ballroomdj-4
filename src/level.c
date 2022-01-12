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
  { "DEFAULT",  LEVEL_DEFAULT_FLAG, VALUE_NUM, parseConvBoolean },
  { "LABEL",    LEVEL_LABEL,        VALUE_DATA, NULL },
  { "WEIGHT",   LEVEL_WEIGHT,       VALUE_NUM, NULL },
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

  level->df = datafileAllocParse ("level", DFTYPE_INDIRECT, fname,
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

ssize_t
levelGetWeight (level_t *level, listidx_t ikey)
{
  return ilistGetNum (level->level, ikey, LEVEL_WEIGHT);
}

void
levelConv (char *keydata, datafileret_t *ret)
{
  level_t     *level;
  list_t      *lookup;

  ret->valuetype = VALUE_NUM;

  level = bdjvarsdf [BDJVDF_LEVELS];
  lookup = datafileGetLookup (level->df);
  ret->u.num = listGetNum (lookup, keydata);
}
