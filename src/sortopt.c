#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "sortopt.h"
#include "datafile.h"
#include "fileop.h"
#include "log.h"

sortopt_t *
sortoptAlloc (char *fname)
{
  sortopt_t     *sortopt;

  if (! fileopExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: sortopt: missing %s", fname);
    return NULL;
  }

  sortopt = malloc (sizeof (sortopt_t));
  assert (sortopt != NULL);

  sortopt->df = datafileAllocParse ("sortopt", DFTYPE_LIST, fname, NULL, 0,
      DATAFILE_NO_LOOKUP);
  return sortopt;
}

void
sortoptFree (sortopt_t *sortopt)
{
  if (sortopt != NULL) {
    if (sortopt->df != NULL) {
      datafileFree (sortopt->df);
    }
    free (sortopt);
  }
}
