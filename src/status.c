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
#include "log.h"
#include "ilist.h"
#include "slist.h"
#include "status.h"

  /* must be sorted in ascii order */
static datafilekey_t statusdfkeys [STATUS_KEY_MAX] = {
  { "PLAYFLAG",   STATUS_PLAY_FLAG,   VALUE_NUM, parseConvBoolean, -1 },
  { "STATUS",     STATUS_STATUS,      VALUE_DATA, NULL, -1 },
};

status_t *
statusAlloc (char *fname)
{
  status_t    *status;
  ilistidx_t  key;
  ilistidx_t  iteridx;


  if (! fileopFileExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: status: missing %s", fname);
    return NULL;
  }

  status = malloc (sizeof (status_t));
  assert (status != NULL);

  status->df = datafileAllocParse ("status", DFTYPE_INDIRECT, fname,
      statusdfkeys, STATUS_KEY_MAX, STATUS_STATUS);
  status->status = datafileGetList (status->df);
  ilistDumpInfo (status->status);

  status->maxWidth = 0;
  ilistStartIterator (status->status, &iteridx);
  while ((key = ilistIterateKey (status->status, &iteridx)) >= 0) {
    char    *val;
    int     len;

    val = ilistGetData (status->status, key, STATUS_STATUS);
    len = istrlen (val);
    if (len > status->maxWidth) {
      status->maxWidth = len;
    }
  }

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

ssize_t
statusGetCount (status_t *status)
{
  return ilistGetCount (status->status);
}

int
statusGetMaxWidth (status_t *status)
{
  return status->maxWidth;
}

char *
statusGetStatus (status_t *status, ilistidx_t idx)
{
  return ilistGetData (status->status, idx, STATUS_STATUS);
}

bool
statusPlayCheck (status_t *status, ilistidx_t ikey)
{
  return (ilistGetNum (status->status, ikey, STATUS_PLAY_FLAG) == 1);
}

void
statusConv (char *keydata, datafileret_t *ret)
{
  status_t     *status;
  slist_t      *lookup;

  ret->valuetype = VALUE_NUM;

  status = bdjvarsdfGet (BDJVDF_STATUS);
  if (status == NULL) {
    ret->u.num = 0;
    return;
  }

  lookup = datafileGetLookup (status->df);
  ret->u.num = slistGetNum (lookup, keydata);
}
