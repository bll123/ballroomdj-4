/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include "ati.h"
#include "audiofile.h"
#include "log.h"
#include "mdebug.h"

typedef struct atidata {
  int               writetags;
  taglookup_t       tagLookup;
  tagcheck_t        tagCheck;
  tagname_t         tagName;
  audiotaglookup_t  audioTagLookup;
} atidata_t;

typedef struct atisaved {
  bool        hasdata;
  int         filetype;
  int         tagtype;
  char        *data;
  size_t      dlen;
} atisaved_t;

const char *
atiiDesc (void)
{
  return "Null";
}

atidata_t *
atiiInit (const char *atipkg, int writetags,
    taglookup_t tagLookup, tagcheck_t tagCheck,
    tagname_t tagName, audiotaglookup_t audioTagLookup)
{
  atidata_t *atidata;

  atidata = mdmalloc (sizeof (atidata_t));
  atidata->writetags = writetags;
  atidata->tagLookup = tagLookup;
  atidata->tagCheck = tagCheck;
  atidata->tagName = tagName;
  atidata->audioTagLookup = audioTagLookup;

  return atidata;
}

void
atiiFree (atidata_t *atidata)
{
  if (atidata != NULL) {
    mdfree (atidata);
  }
}

void
atiiSupportedTypes (int supported [])
{
  for (int i = 0; i < AFILE_TYPE_MAX; ++i) {
    if (i == AFILE_TYPE_UNKNOWN) {
      continue;
    }
    supported [i] = ATI_NONE;
  }
}

bool
atiiUseReader (void)
{
  return false;
}

char *
atiiReadTags (atidata_t *atidata, const char *ffn)
{
  return NULL;
}

void
atiiParseTags (atidata_t *atidata, slist_t *tagdata, const char *ffn,
    char *data, int filetype, int tagtype, int *rewrite)
{
}

int
atiiWriteTags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  return -1;
}

atisaved_t *
atiiSaveTags (atidata_t *atidata,
    const char *ffn, int tagtype, int filetype)
{
  return NULL;
}

void
atiiFreeSavedTags (atisaved_t *atisaved, int tagtype, int filetype)
{
  return;
}

int
atiiRestoreTags (atidata_t *atidata, atisaved_t *atisaved,
    const char *ffn, int tagtype, int filetype)
{
  return -1;
}

void
atiiCleanTags (atidata_t *atidata,
    const char *ffn, int tagtype, int filetype)
{
  return;
}

