#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdj4.h"
#include "dance.h"
#include "datafile.h"
#include "pathbld.h"
#include "fileop.h"
#include "log.h"
#include "nlist.h"
#include "sequence.h"
#include "slist.h"

sequence_t *
sequenceAlloc (char *fname)
{
  sequence_t    *sequence;
  slist_t       *tlist;
  datafile_t    *df;
  char          *seqkey;
  slistidx_t    lkey;
  char          fn [MAXPATHLEN];
  nlistidx_t    iteridx;
  datafileconv_t  conv;


  pathbldMakePath (fn, sizeof (fn), "", fname, ".sequence", PATHBLD_MP_NONE);
  if (! fileopFileExists (fn)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: sequence: missing %s", fname);
    return NULL;
  }

  sequence = malloc (sizeof (sequence_t));
  assert (sequence != NULL);

  df = datafileAllocParse ("sequence", DFTYPE_LIST, fn,
      NULL, 0, DATAFILE_NO_LOOKUP);
  tlist = datafileGetList (df);

  sequence->sequence = nlistAlloc ("sequence", LIST_UNORDERED, free);
  nlistSetSize (sequence->sequence, slistGetCount (tlist));

  slistStartIterator (tlist, &iteridx);
  while ((seqkey = slistIterateKey (tlist, &iteridx)) != NULL) {
    conv.valuetype = VALUE_STR;
    conv.u.str = seqkey;
    danceConvDance (&conv);
    lkey = conv.u.num;
    nlistSetStr (sequence->sequence, lkey, seqkey);
  }
  datafileFree (df);
  nlistDumpInfo (sequence->sequence);
  return sequence;
}

void
sequenceFree (sequence_t *sequence)
{
  if (sequence != NULL) {
    if (sequence->sequence != NULL) {
      nlistFree (sequence->sequence);
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
sequenceStartIterator (sequence_t *sequence, nlistidx_t *iteridx)
{
  if (sequence == NULL || sequence->sequence == NULL) {
    return;
  }

  nlistStartIterator (sequence->sequence, iteridx);
}

listidx_t
sequenceIterate (sequence_t *sequence, nlistidx_t *iteridx)
{
  listidx_t     lkey;

  if (sequence == NULL || sequence->sequence == NULL) {
    return -1L;
  }

  lkey = nlistIterateKey (sequence->sequence, iteridx);
  if (lkey < 0) {
    /* a sequence just restarts from the beginning */
    lkey = nlistIterateKey (sequence->sequence, iteridx);
  }
  return lkey;
}

