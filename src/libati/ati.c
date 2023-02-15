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
#include "bdj4.h"
#include "pathbld.h"
#include "dylib.h"
#include "mdebug.h"
#include "nlist.h"
#include "slist.h"
#include "sysvars.h"
#include "volsink.h"

typedef struct ati {
  dlhandle_t        *dlHandle;
  void              (*atiiInit) (const char *atipkg);
  char              *(*atiiReadTags) (const char *ffn);
  void              (*atiiParseTags) (slist_t *tagdata, char *data, int tagtype, int *rewrite);
  int               (*atiiWriteTags) (const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int filetype, int writetags);
} ati_t;

ati_t *
atiInit (const char *atipkg)
{
  ati_t     *ati;
  char      dlpath [MAXPATHLEN];

  ati = mdmalloc (sizeof (ati_t));
  ati->atiiInit = NULL;
  ati->atiiReadTags = NULL;
  ati->atiiParseTags = NULL;
  ati->atiiWriteTags = NULL;

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
  ati->atiiReadTags = dylibLookup (ati->dlHandle, "atiiReadTags");
  ati->atiiParseTags = dylibLookup (ati->dlHandle, "atiiParseTags");
  ati->atiiWriteTags = dylibLookup (ati->dlHandle, "atiiWriteTags");
#pragma clang diagnostic pop

  if (ati->atiiInit != NULL) {
    ati->atiiInit (atipkg);
  }
  return ati;
}

void
atiFree (ati_t *ati)
{
  if (ati != NULL) {
    if (ati->dlHandle != NULL) {
      dylibClose (ati->dlHandle);
    }
    mdfree (ati);
  }
}

char *
atiReadTags (ati_t *ati, const char *ffn)
{
  if (ati != NULL && ati->atiiReadTags != NULL) {
    return ati->atiiReadTags (ffn);
  }
  return NULL;
}

void
atiParseTags (ati_t *ati, slist_t *tagdata,
    char *data, int tagtype, int *rewrite)
{
  if (ati != NULL && ati->atiiParseTags != NULL) {
    ati->atiiParseTags (tagdata, data, tagtype, rewrite);
  }
  return;
}

int
atiWriteTags (ati_t *ati, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int filetype, int writetags)
{
  if (ati != NULL && ati->atiiWriteTags != NULL) {
    return ati->atiiWriteTags (ffn, updatelist, dellist, datalist,
        filetype, writetags);
  }
  return 0;
}
