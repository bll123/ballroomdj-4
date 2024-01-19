/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "datafile.h"
#include "fileop.h"
#include "ilist.h"
#include "istring.h"
#include "log.h"
#include "mdebug.h"
#include "pathbld.h"
#include "slist.h"
#include "status.h"

typedef struct status {
  datafile_t  *df;
  ilist_t     *status;
  slist_t     *statusList;
  int         maxWidth;
  char        *path;
} status_t;

  /* must be sorted in ascii order */
static datafilekey_t statusdfkeys [STATUS_KEY_MAX] = {
  { "PLAYFLAG",   STATUS_PLAY_FLAG,   VALUE_NUM, convBoolean, DF_NORM },
  { "STATUS",     STATUS_STATUS,      VALUE_STR, NULL, DF_NORM },
};

status_t *
statusAlloc (void)
{
  status_t    *status;
  ilistidx_t  key;
  ilistidx_t  iteridx;
  char        fname [MAXPATHLEN];


  pathbldMakePath (fname, sizeof (fname), "status",
      BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  if (! fileopFileExists (fname)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: status: missing %s", fname);
    return NULL;
  }

  status = mdmalloc (sizeof (status_t));

  status->path = mdstrdup (fname);
  status->df = datafileAllocParse ("status", DFTYPE_INDIRECT, fname,
      statusdfkeys, STATUS_KEY_MAX, DF_NO_OFFSET, NULL);
  status->status = datafileGetList (status->df);
  ilistDumpInfo (status->status);

  status->maxWidth = 0;
  status->statusList = slistAlloc ("status-disp", LIST_UNORDERED, NULL);
  slistSetSize (status->statusList, ilistGetCount (status->status));

  ilistStartIterator (status->status, &iteridx);
  while ((key = ilistIterateKey (status->status, &iteridx)) >= 0) {
    const char  *val;
    int         len;

    val = ilistGetStr (status->status, key, STATUS_STATUS);
    slistSetNum (status->statusList, val, key);
    len = istrlen (val);
    if (len > status->maxWidth) {
      status->maxWidth = len;
    }
  }

  slistSort (status->statusList);
  return status;
}

void
statusFree (status_t *status)
{
  if (status != NULL) {
    dataFree (status->path);
    datafileFree (status->df);
    slistFree (status->statusList);
    mdfree (status);
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

const char *
statusGetStatus (status_t *status, ilistidx_t idx)
{
  return ilistGetStr (status->status, idx, STATUS_STATUS);
}

ssize_t
statusGetPlayFlag (status_t *status, ilistidx_t ikey)
{
  return ilistGetNum (status->status, ikey, STATUS_PLAY_FLAG);
}

void
statusStartIterator (status_t *status, ilistidx_t *iteridx)
{
  ilistStartIterator (status->status, iteridx);
}

ilistidx_t
statusIterate (status_t *status, ilistidx_t *iteridx)
{
  return ilistIterateKey (status->status, iteridx);
}

void
statusConv (datafileconv_t *conv)
{
  status_t      *status;
  ssize_t       num;

  status = bdjvarsdfGet (BDJVDF_STATUS);

  if (conv->invt == VALUE_STR) {
    if (status == NULL) {
      conv->num = 0;
      return;
    }

    num = slistGetNum (status->statusList, conv->str);
    conv->outvt = VALUE_NUM;
    conv->num = num;
  } else if (conv->invt == VALUE_NUM) {
    if (status == NULL || conv->num == LIST_VALUE_INVALID) {
      /* CONTEXT: status: default status (new) when status is not set */
      conv->str = _("New");
      return;
    }

    num = conv->num;
    conv->outvt = VALUE_STR;
    conv->str = ilistGetStr (status->status, num, STATUS_STATUS);
  }
}

void
statusSave (status_t *status, ilist_t *list)
{
  datafileSave (status->df, NULL, list, DF_NO_OFFSET,
      datafileDistVersion (status->df));
}
