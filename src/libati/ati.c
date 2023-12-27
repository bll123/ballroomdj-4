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
#include "atioggutil.h"
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
  void              (*atiiParseTags) (atidata_t *atidata, slist_t *tagdata, const char *ffn, int filetype, int tagtype, int *rewrite);
  int               (*atiiWriteTags) (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);
  atisaved_t        *(*atiiSaveTags) (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
  void              (*atiiFreeSavedTags) (atisaved_t *atisaved, int tagtype, int filetype);
  int               (*atiiRestoreTags) (atidata_t *atidata, atisaved_t *atisaved, const char *ffn, int tagtype, int filetype);
  void              (*atiiCleanTags) (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
  atidata_t         *atidata;
} ati_t;

bool
atiCheck (const char *atipkg)
{
  bool        rc = false;
  dlhandle_t  *dlHandle = NULL;
  char        dlpath [MAXPATHLEN];

  pathbldMakePath (dlpath, sizeof (dlpath),
      atipkg, sysvarsGetStr (SV_SHLIB_EXT), PATHBLD_MP_DIR_EXEC);
  dlHandle = dylibLoad (dlpath);
  if (dlHandle != NULL) {
    dylibClose (dlHandle);
    rc = true;
  }

  return rc;
}

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
  ati->atiiParseTags = NULL;
  ati->atiiWriteTags = NULL;
  ati->atiiSaveTags = NULL;
  ati->atiiFreeSavedTags = NULL;
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
  ati->atiiParseTags = dylibLookup (ati->dlHandle, "atiiParseTags");
  ati->atiiWriteTags = dylibLookup (ati->dlHandle, "atiiWriteTags");
  ati->atiiSaveTags = dylibLookup (ati->dlHandle, "atiiSaveTags");
  ati->atiiFreeSavedTags = dylibLookup (ati->dlHandle, "atiiFreeSavedTags");
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

void
atiParseTags (ati_t *ati, slist_t *tagdata, const char *ffn,
    int filetype, int tagtype, int *rewrite)
{
  if (ati != NULL && ati->atiiParseTags != NULL) {
    ati->atiiParseTags (ati->atidata, tagdata, ffn, filetype, tagtype, rewrite);
  }

  return;
}

int
atiWriteTags (ati_t *ati, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
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

void
atiFreeSavedTags (ati_t *ati, atisaved_t *atisaved, int tagtype, int filetype)
{
  if (ati != NULL && ati->atiiFreeSavedTags != NULL) {
    ati->atiiFreeSavedTags (atisaved, tagtype, filetype);
  }
  return;
}

int
atiRestoreTags (ati_t *ati, atisaved_t *atisaved, const char *ffn,
    int tagtype, int filetype)
{
  int   rc = -1;

  if (ati != NULL && ati->atiiRestoreTags != NULL) {
    rc = ati->atiiRestoreTags (ati->atidata, atisaved, ffn, tagtype, filetype);
  }
  return rc;
}

void
atiCleanTags (ati_t *ati, const char *ffn, int tagtype, int filetype)
{
  if (ati != NULL && ati->atiiCleanTags != NULL) {
    ati->atiiCleanTags (ati->atidata, ffn, tagtype, filetype);
  }
}

int
atiCheckCodec (const char *ffn, int filetype)
{
  if (filetype == AFILE_TYPE_OGG ||
      filetype == AFILE_TYPE_VORBIS ||
      filetype == AFILE_TYPE_OPUS) {
    filetype = atioggCheckCodec (ffn, filetype);
  }
  return filetype;
}

ilist_t *
atiInterfaceList (void)
{
  ilist_t     *interfaces;

  interfaces = dyInterfaceList (LIBATI_PFX, "atiiDesc");
  return interfaces;
}

void
atiGetSupportedTypes (const char *atipkg, int supported [])
{
  dlhandle_t  *dlHandle = NULL;
  void        *(*sproc) (int supported []);
  char        dlpath [MAXPATHLEN];

  for (int i = 0; i < AFILE_TYPE_MAX; ++i) {
    supported [i] = ATI_NONE;
  }

  pathbldMakePath (dlpath, sizeof (dlpath),
      atipkg, sysvarsGetStr (SV_SHLIB_EXT), PATHBLD_MP_DIR_EXEC);
  dlHandle = dylibLoad (dlpath);
  if (dlHandle != NULL) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
    sproc = dylibLookup (dlHandle, "atiiSupportedTypes");
#pragma clang diagnostic pop

    if (sproc != NULL) {
      sproc (supported);
    }
    dylibClose (dlHandle);
  }
  return;
}
