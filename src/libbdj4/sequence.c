/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "dance.h"
#include "datafile.h"
#include "pathbld.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "sequence.h"
#include "slist.h"

enum {
  SEQUENCE_VERSION = 1,
};

typedef struct sequence {
  datafile_t  *df;
  nlist_t     *sequence;
  char        *name;
  char        *path;
  int         distvers;
} sequence_t;

sequence_t *
sequenceLoad (const char *fname)
{
  sequence_t    *sequence;
  slist_t       *tlist;
  const char    *seqkey;
  slistidx_t    lkey;
  char          fn [MAXPATHLEN];
  nlistidx_t    iteridx;
  datafileconv_t  conv;


  pathbldMakePath (fn, sizeof (fn), fname, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DREL_DATA);
  if (! fileopFileExists (fn)) {
    // logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: sequence: missing %s", fname);
    return NULL;
  }

  sequence = mdmalloc (sizeof (sequence_t));
  sequence->name = mdstrdup (fname);
  sequence->path = mdstrdup (fn);

  sequence->df = datafileAllocParse ("sequence", DFTYPE_LIST, fn, NULL, 0, DF_NO_OFFSET, NULL);
  tlist = datafileGetList (sequence->df);
  sequence->distvers = datafileDistVersion (sequence->df);

  sequence->sequence = nlistAlloc ("sequence", LIST_UNORDERED, NULL);
  nlistSetSize (sequence->sequence, slistGetCount (tlist));

  slistStartIterator (tlist, &iteridx);
  while ((seqkey = slistIterateKey (tlist, &iteridx)) != NULL) {
    conv.invt = VALUE_STR;
    conv.str = seqkey;
    danceConvDance (&conv);
    lkey = conv.num;
    nlistSetStr (sequence->sequence, lkey, seqkey);
  }
  nlistDumpInfo (sequence->sequence);

  return sequence;
}

sequence_t *
sequenceCreate (const char *fname)
{
  sequence_t    *sequence;
  char          fn [MAXPATHLEN];


  pathbldMakePath (fn, sizeof (fn), fname, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DREL_DATA);

  sequence = mdmalloc (sizeof (sequence_t));
  sequence->name = mdstrdup (fname);
  sequence->path = mdstrdup (fn);
  sequence->distvers = 1;
  sequence->df = datafileAlloc ("sequence", DFTYPE_LIST,
      sequence->path, NULL, 0);

  sequence->sequence = nlistAlloc ("sequence", LIST_UNORDERED, NULL);
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
    datafileFree (sequence->df);
    mdfree (sequence);
  }
}

bool
sequenceExists (const char *name)
{
  char  fn [MAXPATHLEN];

  pathbldMakePath (fn, sizeof (fn), name, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DREL_DATA);
  return fileopFileExists (fn);
}

nlist_t *
sequenceGetDanceList (sequence_t *sequence)
{
  if (sequence == NULL || sequence->sequence == NULL) {
    return NULL;
  }

  return sequence->sequence;
}

int32_t
sequenceGetCount (sequence_t *sequence)
{
  if (sequence == NULL || sequence->sequence == NULL) {
    return 0;
  }

  return nlistGetCount (sequence->sequence);
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
  datafileSave (sequence->df, NULL, slist, DF_NO_OFFSET, sequence->distvers);
}
