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
#include "inline.h"
#include "log.h"
#include "nlist.h"
#include "sequence.h"
#include "slist.h"

enum {
  SEQUENCE_VERSION = 1,
};

sequence_t *
sequenceAlloc (const char *fname)
{
  sequence_t    *sequence;
  slist_t       *tlist;
  datafile_t    *df;
  char          *seqkey;
  slistidx_t    lkey;
  char          fn [MAXPATHLEN];
  nlistidx_t    iteridx;
  datafileconv_t  conv;


  pathbldMakePath (fn, sizeof (fn), fname, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DATA);
  if (! fileopFileExists (fn)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: sequence: missing %s", fname);
    return false;
  }

  sequence = malloc (sizeof (sequence_t));
  assert (sequence != NULL);
  sequence->name = strdup (fname);
  sequence->path = strdup (fn);

  df = datafileAllocParse ("sequence", DFTYPE_LIST, fn, NULL, 0);
  tlist = datafileGetList (df);

  sequence->sequence = nlistAlloc ("sequence", LIST_UNORDERED, free);
  nlistSetSize (sequence->sequence, slistGetCount (tlist));

  slistStartIterator (tlist, &iteridx);
  while ((seqkey = slistIterateKey (tlist, &iteridx)) != NULL) {
    conv.allocated = false;
    conv.valuetype = VALUE_STR;
    conv.str = seqkey;
    danceConvDance (&conv);
    lkey = conv.num;
    nlistSetStr (sequence->sequence, lkey, seqkey);
  }
  datafileFree (df);
  nlistDumpInfo (sequence->sequence);

  return sequence;
}

sequence_t *
sequenceCreate (const char *fname)
{
  sequence_t    *sequence;
  char          fn [MAXPATHLEN];


  pathbldMakePath (fn, sizeof (fn), fname, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DATA);

  sequence = malloc (sizeof (sequence_t));
  assert (sequence != NULL);
  sequence->name = strdup (fname);
  sequence->path = strdup (fn);

  sequence->sequence = nlistAlloc ("sequence", LIST_UNORDERED, free);
  nlistSetVersion (sequence->sequence, SEQUENCE_VERSION);
  return sequence;
}


void
sequenceFree (sequence_t *sequence)
{
  if (sequence != NULL) {
    dataFree (sequence->path);
    dataFree (sequence->name);
    nlistFree (sequence->sequence);
    free (sequence);
  }
}

nlist_t *
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

nlistidx_t
sequenceIterate (sequence_t *sequence, nlistidx_t *iteridx)
{
  nlistidx_t     lkey;

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

void
sequenceSave (sequence_t *sequence, slist_t *slist)
{
  if (slistGetCount (slist) <= 0) {
    return;
  }

  slistSetVersion (slist, SEQUENCE_VERSION);
  datafileSaveList ("sequence", sequence->path, slist);
}
