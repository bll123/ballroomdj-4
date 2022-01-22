#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdjvarsdf.h"
#include "datafile.h"
#include "fileop.h"
#include "level.h"
#include "log.h"
#include "ilist.h"
#include "slist.h"

  /* must be sorted in ascii order */
static datafilekey_t leveldfkeys[] = {
  { "DEFAULT",  LEVEL_DEFAULT_FLAG, VALUE_NUM, parseConvBoolean, -1 },
  { "LEVEL",    LEVEL_LEVEL,        VALUE_DATA, NULL, -1 },
  { "WEIGHT",   LEVEL_WEIGHT,       VALUE_NUM, NULL, -1 },
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
      leveldfkeys, LEVEL_DFKEY_COUNT, LEVEL_LEVEL);
  level->level = datafileGetList (level->df);
  ilistDumpInfo (level->level);
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
levelGetWeight (level_t *level, ilistidx_t ikey)
{
  return ilistGetNum (level->level, ikey, LEVEL_WEIGHT);
}

ssize_t
levelGetMax (level_t *level)
{
  return ilistGetCount (level->level) - 1;
}

void
levelConv (char *keydata, datafileret_t *ret)
{
  level_t     *level;
  slist_t      *lookup;

  ret->valuetype = VALUE_NUM;

  level = bdjvarsdf [BDJVDF_LEVELS];
  lookup = datafileGetLookup (level->df);
  ret->u.num = slistGetNum (lookup, keydata);
}
