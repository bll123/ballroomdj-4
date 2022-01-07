#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "dnctypes.h"
#include "datafile.h"
#include "fileop.h"
#include "bdjvarsdf.h"
#include "log.h"

dnctype_t *
dnctypesAlloc (char *fname)
{
  dnctype_t       *dtype;

  if (! fileopExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: dnctypes: missing %s\n", fname);
    return NULL;
  }

  dtype = malloc (sizeof (dnctype_t));
  assert (dtype != NULL);

  dtype->df = datafileAllocParse ("dance-types", DFTYPE_LIST, fname,
      NULL, 0, DATAFILE_NO_LOOKUP);
  listSort (dtype->df->data);
  return dtype;
}

void
dnctypesFree (dnctype_t *dtype)
{
  if (dtype != NULL) {
    if (dtype->df != NULL) {
      datafileFree (dtype->df);
    }
    free (dtype);
  }
}

void
dnctypesConv (char *keydata, datafileret_t *ret)
{
  dnctype_t       *dtype;

  ret->valuetype = VALUE_LONG;
  dtype = bdjvarsdf [BDJVDF_DANCE_TYPES];
  ret->u.l = listGetStrIdx (datafileGetList (dtype->df), keydata);
}

