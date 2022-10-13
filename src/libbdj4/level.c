#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "datafile.h"
#include "fileop.h"
#include "ilist.h"
#include "istring.h"
#include "level.h"
#include "log.h"
#include "pathbld.h"
#include "slist.h"

typedef struct level {
  datafile_t  *df;
  ilist_t     *level;
  slist_t     *levelList;
  int         maxWidth;
  char        *path;
  char        *defaultName;
  int         defaultKey;
} level_t;

  /* must be sorted in ascii order */
static datafilekey_t leveldfkeys [LEVEL_KEY_MAX] = {
  { "DEFAULT",  LEVEL_DEFAULT_FLAG, VALUE_NUM, convBoolean, -1 },
  { "LEVEL",    LEVEL_LEVEL,        VALUE_STR, NULL, -1 },
  { "WEIGHT",   LEVEL_WEIGHT,       VALUE_NUM, NULL, -1 },
};

level_t *
levelAlloc ()
{
  level_t     *level;
  ilistidx_t  key;
  ilistidx_t  iteridx;
  char        fname [MAXPATHLEN];

  pathbldMakePath (fname, sizeof (fname), "levels",
      BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
  if (! fileopFileExists (fname)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: level: missing %s", fname);
    return NULL;
  }

  level = malloc (sizeof (level_t));
  assert (level != NULL);

  level->path = strdup (fname);
  level->df = datafileAllocParse ("level", DFTYPE_INDIRECT, fname,
      leveldfkeys, LEVEL_KEY_MAX);
  level->level = datafileGetList (level->df);
  ilistDumpInfo (level->level);

  level->maxWidth = 0;
  level->levelList = slistAlloc ("level-disp", LIST_UNORDERED, NULL);
  slistSetSize (level->levelList, slistGetCount (level->level));

  ilistStartIterator (level->level, &iteridx);
  while ((key = ilistIterateKey (level->level, &iteridx)) >= 0) {
    char    *val;
    ssize_t nval;
    int     len;

    val = ilistGetStr (level->level, key, LEVEL_LEVEL);
    slistSetNum (level->levelList, val, key);
    len = istrlen (val);
    if (len > level->maxWidth) {
      level->maxWidth = len;
    }
    nval = ilistGetNum (level->level, key, LEVEL_DEFAULT_FLAG);
    if (nval && val != NULL) {
      level->defaultName = strdup (val);
      level->defaultKey = nval;
    }
  }

  slistSort (level->levelList);

  return level;
}

void
levelFree (level_t *level)
{
  if (level != NULL) {
    if (level->path != NULL) {
      free (level->path);
    }
    if (level->df != NULL) {
      datafileFree (level->df);
    }
    if (level->defaultName != NULL) {
      free (level->defaultName);
    }
    if (level->levelList != NULL) {
      slistFree (level->levelList);
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
  return ilistGetStr (level->level, ikey, LEVEL_LEVEL);
}

ssize_t
levelGetWeight (level_t *level, ilistidx_t ikey)
{
  return ilistGetNum (level->level, ikey, LEVEL_WEIGHT);
}

ssize_t
levelGetDefault (level_t *level, ilistidx_t ikey)
{
  return ilistGetNum (level->level, ikey, LEVEL_DEFAULT_FLAG);
}

char *
levelGetDefaultName (level_t *level)
{
  if (level == NULL) {
    return NULL;
  }
  return level->defaultName;
}

ssize_t
levelGetDefaultKey (level_t *level)
{
  if (level == NULL) {
    return 0;
  }
  return level->defaultKey;
}

ssize_t
levelGetMax (level_t *level)
{
  return ilistGetCount (level->level) - 1;
}

void
levelStartIterator (level_t *level, ilistidx_t *iteridx)
{
  ilistStartIterator (level->level, iteridx);
}

ilistidx_t
levelIterate (level_t *level, ilistidx_t *iteridx)
{
  return ilistIterateKey (level->level, iteridx);
}

void
levelConv (datafileconv_t *conv)
{
  level_t     *level;
  ssize_t     num;

  level = bdjvarsdfGet (BDJVDF_LEVELS);

  conv->allocated = false;
  if (conv->valuetype == VALUE_STR) {
    conv->valuetype = VALUE_NUM;

    num = slistGetNum (level->levelList, conv->str);
    conv->num = num;
    if (conv->num == LIST_VALUE_INVALID) {
      /* unknown levels are dumped into bucket 1 */
      conv->num = 1;
    }
  } else if (conv->valuetype == VALUE_NUM) {
    conv->valuetype = VALUE_STR;
    num = conv->num;
    conv->str = ilistGetStr (level->level, num, LEVEL_LEVEL);
  }
}

void
levelSave (level_t *level, ilist_t *list)
{
  datafileSaveIndirect ("level", level->path, leveldfkeys,
      LEVEL_KEY_MAX, list);
}
