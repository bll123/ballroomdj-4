#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "datafile.h"
#include "fileop.h"
#include "level.h"
#include "log.h"
#include "ilist.h"
#include "slist.h"

  /* must be sorted in ascii order */
static datafilekey_t leveldfkeys [LEVEL_KEY_MAX] = {
  { "DEFAULT",  LEVEL_DEFAULT_FLAG, VALUE_NUM, parseConvBoolean, -1 },
  { "LEVEL",    LEVEL_LEVEL,        VALUE_DATA, NULL, -1 },
  { "WEIGHT",   LEVEL_WEIGHT,       VALUE_NUM, NULL, -1 },
};

level_t *
levelAlloc (char *fname)
{
  level_t     *level;
  ilistidx_t  key;
  ilistidx_t  iteridx;


  if (! fileopFileExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: level: missing %s", fname);
    return NULL;
  }

  level = malloc (sizeof (level_t));
  assert (level != NULL);

  level->df = datafileAllocParse ("level", DFTYPE_INDIRECT, fname,
      leveldfkeys, LEVEL_KEY_MAX, LEVEL_LEVEL);
  level->level = datafileGetList (level->df);
  ilistDumpInfo (level->level);

  level->maxWidth = 0;
  ilistStartIterator (level->level, &iteridx);
  while ((key = ilistIterateKey (level->level, &iteridx)) >= 0) {
    char    *val;
    int     len;

    val = ilistGetData (level->level, key, LEVEL_LEVEL);
    len = istrlen (val);
    if (len > level->maxWidth) {
      level->maxWidth = len;
    }
  }

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
levelGetCount (level_t *level)
{
  return ilistGetCount (level->level);
}

int
levelGetMaxWidth (level_t *level)
{
  return level->maxWidth;
}

char *
levelGetLevel (level_t *level, ilistidx_t ikey)
{
  return ilistGetData (level->level, ikey, LEVEL_LEVEL);
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

  level = bdjvarsdfGet (BDJVDF_LEVELS);
  lookup = datafileGetLookup (level->df);
  ret->u.num = slistGetNum (lookup, keydata);
  if (ret->u.num == LIST_VALUE_INVALID) {
    /* unknown levels are dumped into bucket 1 */
    ret->u.num = 1;
  }
}
