#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdjvarsdf.h"
#include "datafile.h"
#include "fileop.h"
#include "log.h"
#include "status.h"

  /* must be sorted in ascii order */
static datafilekey_t statusdfkeys[] = {
  { "PLAYFLAG",   STATUS_PLAY_FLAG,  VALUE_NUM, parseConvBoolean },
  { "STATUS",     STATUS_STATUS,      VALUE_DATA, NULL },
};
#define STATUS_DFKEY_COUNT (sizeof (statusdfkeys) / sizeof (datafilekey_t))

status_t *
statusAlloc (char *fname)
{
  status_t    *status;

  if (! fileopExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: status: missing %s", fname);
    return NULL;
  }

  status = malloc (sizeof (status_t));
  assert (status != NULL);

  status->df = datafileAllocParse ("status", DFTYPE_INDIRECT, fname,
      statusdfkeys, STATUS_DFKEY_COUNT, STATUS_STATUS);
  status->status = datafileGetList (status->df);
  llistDumpInfo (status->status);
  return status;
}

void
statusFree (status_t *status)
{
  if (status != NULL) {
    if (status->df != NULL) {
      datafileFree (status->df);
    }
    free (status);
  }
}

bool
statusPlayCheck (status_t *status, listidx_t ikey)
{
  return ilistGetNum (status->status, ikey, STATUS_PLAY_FLAG);
}

void
statusConv (char *keydata, datafileret_t *ret)
{
  status_t     *status;
  list_t      *lookup;

  ret->valuetype = VALUE_NUM;

  status = bdjvarsdf [BDJVDF_STATUS];
  lookup = datafileGetLookup (status->df);
  ret->u.num = listGetNum (lookup, keydata);
}
