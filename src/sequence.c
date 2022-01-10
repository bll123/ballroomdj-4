#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "sequence.h"
#include "datafile.h"
#include "fileop.h"
#include "log.h"

sequence_t *
sequenceAlloc (char *fname)
{
  sequence_t    *sequence;

  if (! fileopExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: sequence: missing %s", fname);
    return NULL;
  }

  sequence = malloc (sizeof (sequence_t));

  sequence->df = datafileAllocParse ("sequence", DFTYPE_LIST, fname, NULL, 0,
      DATAFILE_NO_LOOKUP);
  llistDumpInfo (datafileGetList (sequence->df));
  return sequence;
}

void
sequenceFree (sequence_t *sequence)
{
  if (sequence != NULL) {
    if (sequence->df != NULL) {
      datafileFree (sequence->df);
    }
    free (sequence);
  }
}
