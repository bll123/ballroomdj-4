/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdj4.h"
#include "dnctypes.h"
#include "datafile.h"
#include "fileop.h"
#include "bdjvarsdf.h"
#include "log.h"
#include "mdebug.h"
#include "pathbld.h"
#include "slist.h"

typedef struct dnctype {
  datafile_t  *df;
  slist_t     *dnctypes;
} dnctype_t;

dnctype_t *
dnctypesAlloc (void)
{
  dnctype_t *dnctypes;
  char      fname [MAXPATHLEN];

  pathbldMakePath (fname, sizeof (fname), "dancetypes",
      BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  if (! fileopFileExists (fname)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: dnctypes: missing %s", fname);
    return NULL;
  }

  dnctypes = mdmalloc (sizeof (dnctype_t));
  assert (dnctypes != NULL);

  dnctypes->df = datafileAllocParse ("dance-types", DFTYPE_LIST, fname,
      NULL, 0);
  dnctypes->dnctypes = datafileGetList (dnctypes->df);
  listSort (dnctypes->dnctypes);
  listDumpInfo (dnctypes->dnctypes);
  return dnctypes;
}

void
dnctypesFree (dnctype_t *dnctypes)
{
  if (dnctypes != NULL) {
    datafileFree (dnctypes->df);
    mdfree (dnctypes);
  }
}

void
dnctypesStartIterator (dnctype_t *dnctypes, slistidx_t *iteridx)
{
  slistStartIterator (dnctypes->dnctypes, iteridx);
}

char *
dnctypesIterate (dnctype_t *dnctypes, slistidx_t *iteridx)
{
  return slistIterateKey (dnctypes->dnctypes, iteridx);
}

void
dnctypesConv (datafileconv_t *conv)
{
  dnctype_t       *dnctypes;
  ssize_t         num;
  char            *sval = NULL;

  dnctypes = bdjvarsdfGet (BDJVDF_DANCE_TYPES);
  assert (dnctypes != NULL);

  conv->allocated = false;
  if (conv->valuetype == VALUE_STR) {
    conv->valuetype = VALUE_NUM;
    if (conv->str == NULL) {
      num = LIST_VALUE_INVALID;
    } else {
      num = slistGetIdx (dnctypes->dnctypes, conv->str);
    }
    conv->num = num;
  } else if (conv->valuetype == VALUE_NUM) {
    conv->valuetype = VALUE_STR;
    if (conv->num == LIST_VALUE_INVALID) {
      sval = NULL;
    } else {
      sval = slistGetKeyByIdx (dnctypes->dnctypes, conv->num);
    }
    conv->str = sval;
  }
}
