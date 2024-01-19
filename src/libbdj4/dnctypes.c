/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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

  dnctypes->df = datafileAllocParse ("dance-types", DFTYPE_LIST, fname,
      NULL, 0, DF_NO_OFFSET, NULL);
  dnctypes->dnctypes = datafileGetList (dnctypes->df);
  slistSort (dnctypes->dnctypes);
  slistDumpInfo (dnctypes->dnctypes);
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
  if (dnctypes == NULL) {
    return;
  }
  slistStartIterator (dnctypes->dnctypes, iteridx);
}

const char *
dnctypesIterate (dnctype_t *dnctypes, slistidx_t *iteridx)
{
  if (dnctypes == NULL) {
    return NULL;
  }
  return slistIterateKey (dnctypes->dnctypes, iteridx);
}

void
dnctypesConv (datafileconv_t *conv)
{
  dnctype_t       *dnctypes = NULL;
  ssize_t         num;
  const char      *sval = NULL;

  dnctypes = bdjvarsdfGet (BDJVDF_DANCE_TYPES);
  if (dnctypes == NULL) {
    return;
  }

  if (conv->invt == VALUE_STR) {
    if (conv->str == NULL) {
      num = LIST_VALUE_INVALID;
    } else {
      num = slistGetIdx (dnctypes->dnctypes, conv->str);
    }
    conv->outvt = VALUE_NUM;
    conv->num = num;
  } else if (conv->invt == VALUE_NUM) {
    if (conv->num == LIST_VALUE_INVALID) {
      sval = NULL;
    } else {
      sval = slistGetKeyByIdx (dnctypes->dnctypes, conv->num);
    }
    conv->outvt = VALUE_STR;
    conv->str = sval;
  }
}
