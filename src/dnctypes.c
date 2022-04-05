#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "dnctypes.h"
#include "datafile.h"
#include "fileop.h"
#include "bdjvarsdf.h"
#include "log.h"
#include "slist.h"

dnctype_t *
dnctypesAlloc (char *fname)
{
  dnctype_t       *dnctypes;

  if (! fileopFileExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: dnctypes: missing %s", fname);
    return NULL;
  }

  dnctypes = malloc (sizeof (dnctype_t));
  assert (dnctypes != NULL);

  dnctypes->df = datafileAllocParse ("dance-types", DFTYPE_LIST, fname,
      NULL, 0, DATAFILE_NO_LOOKUP);
  dnctypes->dnctypes = datafileGetList (dnctypes->df);
  listSort (dnctypes->dnctypes);
  listDumpInfo (dnctypes->dnctypes);
  return dnctypes;
}

void
dnctypesFree (dnctype_t *dnctypes)
{
  if (dnctypes != NULL) {
    if (dnctypes->df != NULL) {
      datafileFree (dnctypes->df);
    }
    free (dnctypes);
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

  dnctypes = bdjvarsdfGet (BDJVDF_DANCE_TYPES);

  conv->allocated = false;
  if (conv->valuetype == VALUE_STR) {
    conv->valuetype = VALUE_NUM;
    num = slistGetIdx (dnctypes->dnctypes, conv->u.str);
    conv->u.num = num;
  } else if (conv->valuetype == VALUE_NUM) {
    slistidx_t  iteridx;
    char        *val;
    int         count;

    conv->valuetype = VALUE_STR;
    slistStartIterator (dnctypes->dnctypes, &iteridx);
    count = 0;
    while ((val = slistIterateKey (dnctypes->dnctypes, &iteridx)) != NULL) {
      if (count == conv->u.num) {
        conv->u.str = val;
        break;
      }
      ++count;
    }
  }
}
