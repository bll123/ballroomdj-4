/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "ati.h"
#include "audiofile.h"
#include "bdj4.h"
#include "dyintfc.h"
#include "dylib.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "pathbld.h"
#include "slist.h"
#include "sysvars.h"

typedef struct ati {
  dlhandle_t        *dlHandle;
  atidata_t         *(*atiiInit) (const char *, int, taglookup_t, tagcheck_t, tagname_t, audiotaglookup_t);
  void              (*atiiFree) (atidata_t *atidata);
  bool              *(*atiiUseReader) (void);
  char              *(*atiiReadTags) (atidata_t *atidata, const char *ffn);
  void              (*atiiParseTags) (atidata_t *atidata, slist_t *tagdata, const char *ffn, char *data, int filetype, int tagtype, int *rewrite);
  int               (*atiiWriteTags) (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);
  atisaved_t        *(*atiiSaveTags) (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
  int               (*atiiRestoreTags) (atidata_t *atidata, atisaved_t *atisaved, const char *ffn, int tagtype, int filetype);
  void              (*atiiCleanTags) (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
  atidata_t         *atidata;
} ati_t;

ati_t *
atiInit (const char *atipkg, int writetags,
    taglookup_t tagLookup, tagcheck_t tagCheck,
    tagname_t tagName, audiotaglookup_t audioTagLookup)
{
  ati_t     *ati;
  char      dlpath [MAXPATHLEN];

  ati = mdmalloc (sizeof (ati_t));
  ati->atiiInit = NULL;
  ati->atiiFree = NULL;
  ati->atiiUseReader = NULL;
  ati->atiiReadTags = NULL;
  ati->atiiParseTags = NULL;
  ati->atiiWriteTags = NULL;
  ati->atiiSaveTags = NULL;
  ati->atiiRestoreTags = NULL;
  ati->atiiCleanTags = NULL;

  pathbldMakePath (dlpath, sizeof (dlpath),
      atipkg, sysvarsGetStr (SV_SHLIB_EXT), PATHBLD_MP_DIR_EXEC);
  ati->dlHandle = dylibLoad (dlpath);
  if (ati->dlHandle == NULL) {
    fprintf (stderr, "Unable to open library %s\n", dlpath);
    mdfree (ati);
    return NULL;
  }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
  ati->atiiInit = dylibLookup (ati->dlHandle, "atiiInit");
  ati->atiiFree = dylibLookup (ati->dlHandle, "atiiFree");
  ati->atiiUseReader = dylibLookup (ati->dlHandle, "atiiUseReader");
  ati->atiiReadTags = dylibLookup (ati->dlHandle, "atiiReadTags");
  ati->atiiParseTags = dylibLookup (ati->dlHandle, "atiiParseTags");
  ati->atiiWriteTags = dylibLookup (ati->dlHandle, "atiiWriteTags");
  ati->atiiSaveTags = dylibLookup (ati->dlHandle, "atiiSaveTags");
  ati->atiiRestoreTags = dylibLookup (ati->dlHandle, "atiiRestoreTags");
  ati->atiiCleanTags = dylibLookup (ati->dlHandle, "atiiCleanTags");
#pragma clang diagnostic pop

  if (ati->atiiInit != NULL) {
    ati->atidata = ati->atiiInit (atipkg, writetags,
        tagLookup, tagCheck, tagName, audioTagLookup);
  }
  return ati;
}

void
atiFree (ati_t *ati)
{
  if (ati != NULL) {
    if (ati->atiiFree != NULL) {
      ati->atiiFree (ati->atidata);
    }
    if (ati->dlHandle != NULL) {
      dylibClose (ati->dlHandle);
    }
    mdfree (ati);
  }
}

bool
atiUseReader (ati_t *ati)
{
  if (ati != NULL && ati->atiiUseReader != NULL) {
    return ati->atiiUseReader ();
  }
  return false;
}

char *
atiReadTags (ati_t *ati, const char *ffn)
{
  if (ati != NULL && ati->atiiReadTags != NULL) {
    return ati->atiiReadTags (ati->atidata, ffn);
  }
  return NULL;
}

void
atiParseTags (ati_t *ati, slist_t *tagdata, const char *ffn,
    char *data, int filetype, int tagtype, int *rewrite)
{
  if (ati != NULL && ati->atiiParseTags != NULL) {
    ati->atiiParseTags (ati->atidata, tagdata, ffn, data, filetype, tagtype, rewrite);
  }

  return;
}

int
atiWriteTags (ati_t *ati, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
fprintf (stderr, "ati: write-tags: %d %d\n", tagtype, filetype);
  if (ati != NULL && ati->atiiWriteTags != NULL) {
    return ati->atiiWriteTags (ati->atidata, ffn,
        updatelist, dellist, datalist,
        tagtype, filetype);
  }
  return 0;
}

atisaved_t *
atiSaveTags (ati_t *ati, const char *ffn, int tagtype, int filetype)
{
  if (ati != NULL && ati->atiiSaveTags != NULL) {
    return ati->atiiSaveTags (ati->atidata, ffn, tagtype, filetype);
  }
  return NULL;
}

int
atiRestoreTags (ati_t *ati, atisaved_t *atisaved, const char *ffn,
    int tagtype, int filetype)
{
  if (ati != NULL && ati->atiiRestoreTags != NULL) {
    return ati->atiiRestoreTags (ati->atidata, atisaved, ffn, tagtype, filetype);
  }
  return 0;
}

void
atiCleanTags (ati_t *ati, const char *ffn, int tagtype, int filetype)
{
  if (ati != NULL && ati->atiiCleanTags != NULL) {
    ati->atiiCleanTags (ati->atidata, ffn, tagtype, filetype);
  }
}


slist_t *
atiInterfaceList (void)
{
  slist_t     *interfaces;

  interfaces = dyInterfaceList ("libati", "atiiDesc");
  return interfaces;
}

