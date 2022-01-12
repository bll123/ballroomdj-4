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
#include "dance.h"
#include "datautil.h"
#include "portability.h"

sequence_t *
sequenceAlloc (char *fname)
{
  sequence_t    *sequence;
  list_t        *tlist;
  datafile_t    *df;
  list_t        *danceLookup;
  char          *seqkey;
  listidx_t     lkey;
  char          fn [MAXPATHLEN];


  datautilMakePath (fn, sizeof (fn), "", fname, ".sequence", DATAUTIL_MP_NONE);
  if (! fileopExists (fn)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: sequence: missing %s", fname);
    return NULL;
  }

  sequence = malloc (sizeof (sequence_t));
  assert (sequence != NULL);

  df = datafileAllocParse ("sequence", DFTYPE_LIST, fn,
      NULL, 0, DATAFILE_NO_LOOKUP);
  tlist = datafileGetList (df);

  danceLookup = danceGetLookup ();
  sequence->sequence = llistAlloc ("sequence", LIST_UNORDERED, free);
  llistSetSize (sequence->sequence, listGetCount (tlist));

  listStartIterator (tlist);
  while ((seqkey = listIterateKeyStr (tlist)) != NULL) {
    lkey = listGetNum (danceLookup, seqkey);
    llistSetData (sequence->sequence, lkey, strdup (seqkey));
  }

  datafileFree (df);

  listDumpInfo (sequence->sequence);
  return sequence;
}

void
sequenceFree (sequence_t *sequence)
{
  if (sequence != NULL) {
    if (sequence->sequence != NULL) {
      listFree (sequence->sequence);
    }
    free (sequence);
  }
}

list_t *
sequenceGetDanceList (sequence_t *sequence)
{
  if (sequence == NULL || sequence->sequence == NULL) {
    return NULL;
  }

  return sequence->sequence;
}

void
sequenceStartIterator (sequence_t *sequence)
{
  if (sequence == NULL || sequence->sequence == NULL) {
    return;
  }

  listStartIterator (sequence->sequence);
}

listidx_t
sequenceIterate (sequence_t *sequence)
{
  listidx_t     lkey;

  if (sequence == NULL || sequence->sequence == NULL) {
    return -1L;
  }

  lkey = llistIterateKeyNum (sequence->sequence);
  if (lkey < 0) {
      /* a sequence just restarts from the beginning */
    lkey = llistIterateKeyNum (sequence->sequence);
  }
  return lkey;
}

