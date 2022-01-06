#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "sortopt.h"
#include "datafile.h"
#include "fileop.h"
#include "log.h"

datafile_t *
sortoptAlloc (char *fname)
{
  datafile_t    *df;

  if (! fileopExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: sortopt: missing %s\n", fname);
    return NULL;
  }
  df = datafileAllocParse ("sortopt", DFTYPE_LIST, fname, NULL, 0,
      DATAFILE_NO_LOOKUP);
  return df;
}

void
sortoptFree (datafile_t *df)
{
  datafileFree (df);
}
