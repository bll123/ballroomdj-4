#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "dnctypes.h"
#include "datafile.h"
#include "fileop.h"
#include "bdjvarsdf.h"

datafile_t *
dnctypesAlloc (char *fname)
{
  datafile_t    *df;

  if (! fileopExists (fname)) {
    return NULL;
  }
  df = datafileAllocParse ("dance-types", DFTYPE_LIST, fname,
      NULL, 0, DATAFILE_NO_LOOKUP);
  listSort (df->data);
  return df;
}

void
dnctypesFree (datafile_t *df)
{
  datafileFree (df);
}

list_t *
dnctypesGetList (datafile_t *df)
{
  return df->data;
}

void
dnctypesConv (char *keydata, datafileret_t *ret)
{
  ret->valuetype = VALUE_LONG;
  ret->u.l = listGetStrIdx (dnctypesGetList (bdjvarsdf [BDJVDF_DANCE_TYPES]), keydata);
  ret->u.l = 0;
}

