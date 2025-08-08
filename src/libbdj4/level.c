/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "datafile.h"
#include "fileop.h"
#include "ilist.h"
#include "istring.h"
#include "level.h"
#include "log.h"
#include "mdebug.h"
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
  { "DEFAULT",  LEVEL_DEFAULT_FLAG, VALUE_NUM, convBoolean, DF_NORM },
  { "LEVEL",    LEVEL_LEVEL,        VALUE_STR, NULL, DF_NORM },
  { "WEIGHT",   LEVEL_WEIGHT,       VALUE_NUM, NULL, DF_NORM },
};

NODISCARD
level_t *
levelAlloc ()
{
  level_t     *levels;
  ilistidx_t  key;
  ilistidx_t  iteridx;
  char        fname [MAXPATHLEN];

  pathbldMakePath (fname, sizeof (fname), "levels",
      BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  if (! fileopFileExists (fname)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: level: missing %s", fname);
    return NULL;
  }

  levels = mdmalloc (sizeof (level_t));

  levels->path = mdstrdup (fname);
  levels->df = datafileAllocParse ("level", DFTYPE_INDIRECT, fname,
      leveldfkeys, LEVEL_KEY_MAX, DF_NO_OFFSET, NULL);
  levels->level = datafileGetList (levels->df);
  ilistDumpInfo (levels->level);

  levels->levelList = slistAlloc ("level-disp", LIST_UNORDERED, NULL);
  slistSetSize (levels->levelList, ilistGetCount (levels->level));

  levels->maxWidth = ilistGetMaxValueWidth (levels->level, LEVEL_LEVEL);

  ilistStartIterator (levels->level, &iteridx);
  while ((key = ilistIterateKey (levels->level, &iteridx)) >= 0) {
    const char  *val;
    ilistidx_t  nval;

    val = ilistGetStr (levels->level, key, LEVEL_LEVEL);
    slistSetNum (levels->levelList, val, key);
    nval = ilistGetNum (levels->level, key, LEVEL_DEFAULT_FLAG);
    if (nval && val != NULL) {
      levels->defaultName = mdstrdup (val);
      levels->defaultKey = nval;
    }
  }

  slistSort (levels->levelList);

  return levels;
}

void
levelFree (level_t *levels)
{
  if (levels == NULL) {
    return;
  }

  dataFree (levels->path);
  datafileFree (levels->df);
  dataFree (levels->defaultName);
  slistFree (levels->levelList);
  mdfree (levels);
}

ilistidx_t
levelGetCount (level_t *levels)
{
  if (levels == NULL) {
    return 0;
  }

  return ilistGetCount (levels->level);
}

int
levelGetMaxWidth (level_t *levels)
{
  if (levels == NULL) {
    return 0;
  }

  return levels->maxWidth;
}

const char *
levelGetLevel (level_t *levels, ilistidx_t ikey)
{
  if (levels == NULL) {
    return NULL;
  }

  return ilistGetStr (levels->level, ikey, LEVEL_LEVEL);
}

int
levelGetWeight (level_t *levels, ilistidx_t ikey)
{
  if (levels == NULL) {
    return 0;
  }

  return ilistGetNum (levels->level, ikey, LEVEL_WEIGHT);
}

int
levelGetDefault (level_t *levels, ilistidx_t ikey)
{
  if (levels == NULL) {
    return 0;
  }

  return ilistGetNum (levels->level, ikey, LEVEL_DEFAULT_FLAG);
}

char *
levelGetDefaultName (level_t *levels)
{
  if (levels == NULL) {
    return NULL;
  }

  return levels->defaultName;
}

ilistidx_t
levelGetDefaultKey (level_t *levels)
{
  if (levels == NULL) {
    return 0;
  }

  return levels->defaultKey;
}

void
levelSetLevel (level_t *levels, ilistidx_t ikey, const char *leveldisp)
{
  if (levels == NULL) {
    return;
  }

  ilistSetStr (levels->level, ikey, LEVEL_LEVEL, leveldisp);
}

void
levelSetWeight (level_t *levels, ilistidx_t ikey, int weight)
{
  if (levels == NULL) {
    return;
  }

  ilistSetNum (levels->level, ikey, LEVEL_WEIGHT, weight);
}

void
levelSetDefault (level_t *levels, ilistidx_t ikey)
{
  ilistidx_t  count;

  if (levels == NULL) {
    return;
  }

  count = ilistGetCount (levels->level);
  for (ilistidx_t tidx = 0; tidx < count; ++tidx) {
    ilistSetNum (levels->level, tidx, LEVEL_DEFAULT_FLAG, 0);
  }
  ilistSetNum (levels->level, ikey, LEVEL_DEFAULT_FLAG, 1);
  dataFree (levels->defaultName);
  levels->defaultName = mdstrdup (
      ilistGetStr (levels->level, ikey, LEVEL_LEVEL));
  levels->defaultKey = ikey;
}

void
levelDeleteLast (level_t *levels)
{
  ilistidx_t    count;

  if (levels == NULL) {
    return;
  }

  count = ilistGetCount (levels->level);
  ilistDelete (levels->level, count - 1);
}

void
levelStartIterator (level_t *levels, ilistidx_t *iteridx) /* TESTING */
{
  if (levels == NULL) {
    return;
  }

  ilistStartIterator (levels->level, iteridx);
}

ilistidx_t
levelIterate (level_t *levels, ilistidx_t *iteridx)  /* TESTING */
{
  if (levels == NULL) {
    return LIST_LOC_INVALID;
  }

  return ilistIterateKey (levels->level, iteridx);
}

void
levelConv (datafileconv_t *conv)
{
  level_t     *levels;
  slistidx_t  num;

  if (conv == NULL) {
    return;
  }

  levels = bdjvarsdfGet (BDJVDF_LEVELS);

  if (conv->invt == VALUE_STR) {
    num = slistGetNum (levels->levelList, conv->str);
    conv->outvt = VALUE_NUM;
    conv->num = num;
    if (conv->num == LIST_VALUE_INVALID) {
      /* unknown levels are dumped into bucket 1 */
      conv->num = 1;
    }
  } else if (conv->invt == VALUE_NUM) {
    num = conv->num;
    conv->outvt = VALUE_STR;
    conv->str = ilistGetStr (levels->level, num, LEVEL_LEVEL);
  }
}

void
levelSave (level_t *levels, ilist_t *list)
{
  datafileSave (levels->df, NULL, list, DF_NO_OFFSET,
      datafileDistVersion (levels->df));
}
