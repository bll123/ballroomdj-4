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
#include "bdjopt.h"
#include "pathbld.h"
#include "dylib.h"
#include "mdebug.h"
#include "nlist.h"
#include "slist.h"
#include "sysvars.h"
#include "volsink.h"

typedef struct ati {
  dlhandle_t        *dlHandle;
  atidata_t         *(*atiiInit) (const char *, const char *, const char *, const char *, int, taglookup_t, tagcheck_t);
  void              (*atiiFree) (atidata_t *atidata);
  char              *(*atiiReadTags) (atidata_t *atidata, const char *ffn);
  void              (*atiiParseTags) (atidata_t *atidata, slist_t *tagdata, char *data, int tagtype, int *rewrite);
  int               (*atiiWriteTags) (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int filetype, int writetags);
  atidata_t         *atidata;
  taglookup_t       tagLookup;
} ati_t;

ati_t *
atiInit (const char *atipkg, taglookup_t tagLookup, tagcheck_t tagCheck)
{
  ati_t     *ati;
  char      dlpath [MAXPATHLEN];

  ati = mdmalloc (sizeof (ati_t));
  ati->atiiInit = NULL;
  ati->atiiFree = NULL;
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
  ati->atiiFree = dylibLookup (ati->dlHandle, "atiiFree");
  ati->atiiReadTags = dylibLookup (ati->dlHandle, "atiiReadTags");
  ati->atiiParseTags = dylibLookup (ati->dlHandle, "atiiParseTags");
  ati->atiiWriteTags = dylibLookup (ati->dlHandle, "atiiWriteTags");
#pragma clang diagnostic pop

  if (ati->atiiInit != NULL) {
    ati->atidata = ati->atiiInit (atipkg,
        sysvarsGetStr (SV_PATH_PYTHON),
        sysvarsGetStr (SV_PYTHON_MUTAGEN),
        sysvarsGetStr (SV_LOCALE_RADIX),
        bdjoptGetNum (OPT_G_WRITETAGS),
        tagLookup, tagCheck);
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

char *
atiReadTags (ati_t *ati, const char *ffn)
{
  if (ati != NULL && ati->atiiReadTags != NULL) {
    return ati->atiiReadTags (ati->atidata, ffn);
  }
  return NULL;
}

void
atiParseTags (ati_t *ati, slist_t *tagdata,
    char *data, int tagtype, int *rewrite)
{
  if (ati != NULL && ati->atiiParseTags != NULL) {
    ati->atiiParseTags (ati->atidata, tagdata, data, tagtype, rewrite);
  }
  return;
}

int
atiWriteTags (ati_t *ati, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int filetype, int writetags)
{
  if (ati != NULL && ati->atiiWriteTags != NULL) {
    return ati->atiiWriteTags (ati->atidata, ffn,
        updatelist, dellist, datalist,
        filetype, writetags);
  }
  return 0;
}
