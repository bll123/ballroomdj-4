#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "dnctypes.h"
#include "datafile.h"
#include "fileop.h"

datafile_t *
dnctypesAlloc (char *fname)
{
  datafile_t    *df;

  if (! fileopExists (fname)) {
    return NULL;
  }
  df = datafileAlloc (NULL, 0, fname, DFTYPE_LIST);
  return df;
}

void
dnctypesFree (datafile_t *df)
{
  datafileFree (df);
}
