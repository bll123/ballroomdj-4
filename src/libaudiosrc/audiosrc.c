/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include "audiosrc.h"
#include "bdjopt.h"
#include "bdjregex.h"
#include "bdjvars.h"
#include "dyintfc.h"
#include "dylib.h"
#include "bdj4.h"
#include "bdjstring.h"
#include "ilist.h"
#include "mdebug.h"
#include "pathbld.h"
#include "sysvars.h"

typedef struct {
  int               type;
  asdata_t          *asdata;
  dlhandle_t        *dlHandle;
  bool              *(*asiEnabled) (void);
  asdata_t          *(*asiInit) (const char *, const char *);
  void              (*asiFree) (asdata_t *);
  void              (*asiPostInit) (asdata_t *, const char *);
  int               (*asiTypeIdent) (void);
  bool              (*asiIsTypeMatch) (asdata_t *, const char *nm);
  bool              (*asiExists) (asdata_t *, const char *nm);
  bool              (*asiOriginalExists) (asdata_t *, const char *nm);
  bool              (*asiRemove) (asdata_t *, const char *nm);
  bool              (*asiPrep) (asdata_t *, const char *sfname, char *tempnm, size_t sz);
  void              (*asiPrepClean) (asdata_t *, const char *tempnm);
  const char        *(*asiPrefix) (asdata_t *);
  void              (*asiURI) (asdata_t *, const char *sfname, char *uri, size_t sz, const char *prefix, int pfxlen);
  void              (*asiFullPath) (asdata_t *, const char *sfname, char *fullpath, size_t sz, const char *prefix, int pfxlen);
  const char        *(*asiRelativePath) (asdata_t *, const char *nm, int pfxlen);
  size_t            (*asiDir) (asdata_t *, const char *sfname, char *dir, size_t sz, int pfxlen);
  asiterdata_t      *(*asiStartIterator) (asdata_t *, const char *dir);
  void              (*asiCleanIterator) (asdata_t *, asiterdata_t *asiterdata);
  int32_t           (*asiIterCount) (asdata_t *, asiterdata_t *asiterdata);
  const char        *(*asiIterate) (asdata_t *, asiterdata_t *asiterdata);
  bool              (*asiGetPlaylistNames) (asdata_t *);
} asdylib_t;

typedef struct audiosrc {
  int               ascount;
  ilist_t           *aslist;
  asdylib_t         *asdylib;
  bdjregex_t        *protorx;
  int               typeidx [AUDIOSRC_TYPE_MAX];
} audiosrc_t;

typedef struct asiter {
  int           type;
  asiterdata_t  *asiterdata;
  asdylib_t     *asdylib;
  asdata_t      *asdata;
} asiter_t;

enum {
  AS_DYLIB_UNDEF = -1,
};

static audiosrc_t *audiosrc = NULL;

static asdylib_t * audiosrcGetDylibByType (int type);

/* both bdjopt and bdjvars should be initialized */
void
audiosrcInit (void)
{
  if (audiosrc != NULL) {
    return;
  }

  audiosrc = mdmalloc (sizeof (audiosrc_t));
  audiosrc->ascount = 0;
  audiosrc->aslist = NULL;
  audiosrc->asdylib = NULL;
  audiosrc->protorx = NULL;

  audiosrc->protorx = regexInit ("^[[:alpha:]][[:alnum:]+.-]*://");

  audiosrc->aslist = dyInterfaceList ("libas", "asiDesc");
  audiosrc->ascount = ilistGetCount (audiosrc->aslist);

  if (audiosrc->ascount == 0) {
    return;
  }

  for (int i = 0; i < AUDIOSRC_TYPE_MAX; ++i) {
    audiosrc->typeidx [i] = AS_DYLIB_UNDEF;
  }

  audiosrc->asdylib = mdmalloc (sizeof (asdylib_t) * audiosrc->ascount);

  for (int i = 0; i < audiosrc->ascount; ++i) {
    asdylib_t   *asdylib;

    asdylib = &audiosrc->asdylib [i];

    asdylib->type = AUDIOSRC_TYPE_NONE;
    asdylib->asdata = NULL;
    asdylib->dlHandle = NULL;
    asdylib->asiEnabled = NULL;
    asdylib->asiInit = NULL;
    asdylib->asiFree = NULL;
    asdylib->asiPostInit = NULL;
    asdylib->asiTypeIdent = NULL;
    asdylib->asiIsTypeMatch = NULL;
    asdylib->asiExists = NULL;
    asdylib->asiOriginalExists = NULL;
    asdylib->asiRemove = NULL;
    asdylib->asiPrep = NULL;
    asdylib->asiPrepClean = NULL;
    asdylib->asiPrefix = NULL;
    asdylib->asiURI = NULL;
    asdylib->asiFullPath = NULL;
    asdylib->asiRelativePath = NULL;
    asdylib->asiDir = NULL;
    asdylib->asiStartIterator = NULL;
    asdylib->asiCleanIterator = NULL;
    asdylib->asiIterCount = NULL;
    asdylib->asiIterate = NULL;
    asdylib->asiGetPlaylistNames = NULL;
  }

  for (int i = 0; i < audiosrc->ascount; ++i) {
    const char  *pkgnm;
    char        dlpath [MAXPATHLEN];
    asdylib_t   *asdylib;

    pkgnm = ilistGetStr (audiosrc->aslist, i, DYI_LIB);
    asdylib = &audiosrc->asdylib [i];

    pathbldMakePath (dlpath, sizeof (dlpath),
        pkgnm, sysvarsGetStr (SV_SHLIB_EXT), PATHBLD_MP_DIR_EXEC);
    asdylib->dlHandle = dylibLoad (dlpath);
    if (asdylib->dlHandle == NULL) {
      fprintf (stderr, "Unable to open library %s\n", dlpath);
      continue;
    }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
    asdylib->asiEnabled = dylibLookup (asdylib->dlHandle, "asiEnabled");
    if (asdylib->asiEnabled != NULL) {
      if (! asdylib->asiEnabled()) {
        continue;
      }
    }

    asdylib->asiInit = dylibLookup (asdylib->dlHandle, "asiInit");
    asdylib->asiFree = dylibLookup (asdylib->dlHandle, "asiFree");
    asdylib->asiPostInit = dylibLookup (asdylib->dlHandle, "asiPostInit");
    asdylib->asiTypeIdent = dylibLookup (asdylib->dlHandle, "asiTypeIdent");
    asdylib->asiIsTypeMatch = dylibLookup (asdylib->dlHandle, "asiIsTypeMatch");
    asdylib->asiExists = dylibLookup (asdylib->dlHandle, "asiExists");
    asdylib->asiOriginalExists = dylibLookup (asdylib->dlHandle, "asiOriginalExists");
    asdylib->asiRemove = dylibLookup (asdylib->dlHandle, "asiRemove");
    asdylib->asiPrep = dylibLookup (asdylib->dlHandle, "asiPrep");
    asdylib->asiPrepClean = dylibLookup (asdylib->dlHandle, "asiPrepClean");
    asdylib->asiPrefix = dylibLookup (asdylib->dlHandle, "asiPrefix");
    asdylib->asiURI = dylibLookup (asdylib->dlHandle, "asiURI");
    asdylib->asiFullPath = dylibLookup (asdylib->dlHandle, "asiFullPath");
    asdylib->asiRelativePath = dylibLookup (asdylib->dlHandle, "asiRelativePath");
    asdylib->asiDir = dylibLookup (asdylib->dlHandle, "asiDir");
    asdylib->asiStartIterator = dylibLookup (asdylib->dlHandle, "asiStartIterator");
    asdylib->asiCleanIterator = dylibLookup (asdylib->dlHandle, "asiCleanIterator");
    asdylib->asiIterCount = dylibLookup (asdylib->dlHandle, "asiIterCount");
    asdylib->asiIterate = dylibLookup (asdylib->dlHandle, "asiIterate");
    asdylib->asiGetPlaylistNames = dylibLookup (asdylib->dlHandle, "asiGetPlaylistNames");
#pragma clang diagnostic pop

    if (asdylib->asiInit != NULL) {
      asdylib->asdata = asdylib->asiInit (bdjvarsGetStr (BDJV_DELETE_PFX),
          bdjvarsGetStr (BDJV_ORIGINAL_EXT));
    }
    if (asdylib->asiPostInit != NULL) {
      asdylib->asiPostInit (asdylib->asdata, bdjoptGetStr (OPT_M_DIR_MUSIC));
    }
    if (asdylib->asiTypeIdent != NULL) {
      asdylib->type = asdylib->asiTypeIdent ();
    }
    audiosrc->typeidx [asdylib->type] = i;
fprintf (stderr, "type: %d %s\n", asdylib->type, pkgnm);
  }
}

void
audiosrcCleanup (void)
{
  if (audiosrc == NULL) {
    return;
  }

  for (int i = 0; audiosrc->asdylib != NULL && i < audiosrc->ascount; ++i) {
    asdylib_t   *asdylib;

    asdylib = &audiosrc->asdylib [i];

    if (asdylib->asiFree != NULL) {
      asdylib->asiFree (asdylib->asdata);
    }
    if (asdylib->dlHandle != NULL) {
      dylibClose (asdylib->dlHandle);
    }
  }

  ilistFree (audiosrc->aslist);
  dataFree (audiosrc->asdylib);
  regexFree (audiosrc->protorx);
  audiosrc->ascount = 0;

  mdfree (audiosrc);
  audiosrc = NULL;
}

void
audiosrcPostInit (void)
{
  if (audiosrc == NULL) {
    return;
  }

  for (int i = 0; audiosrc->asdylib != NULL && i < audiosrc->ascount; ++i) {
    asdylib_t   *asdylib;

    asdylib = &audiosrc->asdylib [i];

    if (asdylib->asiPostInit != NULL) {
      asdylib->asiPostInit (asdylib->asdata, bdjoptGetStr (OPT_M_DIR_MUSIC));
    }
  }
}

int
audiosrcGetCount (void)
{
  if (audiosrc == NULL) {
    return 0;
  }

  return audiosrc->ascount;
}

int
audiosrcGetType (const char *nm)
{
  int       type = AUDIOSRC_TYPE_NONE;

  if (audiosrc == NULL) {
    return type;
  }
  if (nm == NULL) {
    return type;
  }

  /* it is not particularly useful to try and cache the type, */
  /* as in most cases, the name being processed will be constantly */
  /* changing */

  for (int i = 0; i < audiosrc->ascount; ++i) {
    asdylib_t   *asdylib;

    asdylib = &audiosrc->asdylib [i];

    if (asdylib->type != AUDIOSRC_TYPE_NONE && asdylib->asiIsTypeMatch != NULL) {
      if (asdylib->asiIsTypeMatch (asdylib->asdata, nm)) {
        type = asdylib->type;
        break;
      }
    }
  }

  if (type == AUDIOSRC_TYPE_NONE) {
    if (! regexMatch (audiosrc->protorx, nm)) {
      type = AUDIOSRC_TYPE_FILE;
    }
  }

  return type;
}

bool
audiosrcExists (const char *nm)
{
  int       type = AUDIOSRC_TYPE_NONE;
  bool      exists = false;
  asdylib_t *asdylib;

  if (audiosrc == NULL) {
    return false;
  }
  if (nm == NULL) {
    return false;
  }

  type = audiosrcGetType (nm);
  asdylib = audiosrcGetDylibByType (type);

  if (asdylib != NULL && asdylib->asiExists != NULL) {
    exists = asdylib->asiExists (asdylib->asdata, nm);
  }

  return exists;
}

bool
audiosrcOriginalExists (const char *nm)
{
  int       type = AUDIOSRC_TYPE_NONE;
  bool      exists = false;
  asdylib_t *asdylib;

  if (audiosrc == NULL) {
    return false;
  }
  if (nm == NULL) {
    return false;
  }

  type = audiosrcGetType (nm);
  asdylib = audiosrcGetDylibByType (type);

  if (asdylib != NULL && asdylib->asiOriginalExists != NULL) {
    exists = asdylib->asiOriginalExists (asdylib->asdata, nm);
  }

  return exists;
}

bool
audiosrcRemove (const char *nm)
{
  int       type = AUDIOSRC_TYPE_NONE;
  asdylib_t *asdylib;
  bool      rc = false;

  if (audiosrc == NULL) {
    return rc;
  }
  if (nm == NULL) {
    return rc;
  }

  type = audiosrcGetType (nm);
  asdylib = audiosrcGetDylibByType (type);

  if (asdylib != NULL && asdylib->asiRemove != NULL) {
    rc = asdylib->asiRemove (asdylib->asdata, nm);
  }

  return rc;
}

bool
audiosrcPrep (const char *sfname, char *tempnm, size_t sz)
{
  int       type = AUDIOSRC_TYPE_NONE;
  asdylib_t *asdylib;
  bool      rc = false;

  if (audiosrc == NULL) {
    return rc;
  }
  if (sfname == NULL || tempnm == NULL) {
    return rc;
  }

  type = audiosrcGetType (sfname);
  asdylib = audiosrcGetDylibByType (type);

  if (asdylib != NULL && asdylib->asiPrep != NULL) {
    rc = asdylib->asiPrep (asdylib->asdata, sfname, tempnm, sz);
  }

  return rc;
}

void
audiosrcPrepClean (const char *sfname, const char *tempnm)
{
  int       type = AUDIOSRC_TYPE_NONE;
  asdylib_t *asdylib;

  if (audiosrc == NULL) {
    return;
  }
  if (sfname == NULL || tempnm == NULL) {
    return;
  }

  type = audiosrcGetType (sfname);
  asdylib = audiosrcGetDylibByType (type);

  if (asdylib != NULL && asdylib->asiPrepClean != NULL) {
    asdylib->asiPrepClean (asdylib->asdata, tempnm);
  }
}

const char *
audiosrcPrefix (const char *sfname)
{
  const char  *pfx = "";
  int         type = AUDIOSRC_TYPE_NONE;
  asdylib_t   *asdylib;

  if (audiosrc == NULL) {
    return NULL;
  }
  if (sfname == NULL) {
    return NULL;
  }

  type = audiosrcGetType (sfname);
  asdylib = audiosrcGetDylibByType (type);

  if (asdylib != NULL && asdylib->asiPrefix != NULL) {
    pfx = asdylib->asiPrefix (asdylib->asdata);
  }

  return pfx;
}

void
audiosrcURI (const char *sfname, char *uri, size_t sz,
    const char *prefix, int pfxlen)
{
  int       type = AUDIOSRC_TYPE_NONE;
  asdylib_t *asdylib;

  if (audiosrc == NULL) {
    return;
  }
  if (sfname == NULL || uri == NULL) {
    return;
  }

  type = audiosrcGetType (sfname);
  asdylib = audiosrcGetDylibByType (type);

  *uri = '\0';
  if (asdylib != NULL && asdylib->asiURI != NULL) {
    asdylib->asiURI (asdylib->asdata, sfname, uri, sz, prefix, pfxlen);
  }
}

void
audiosrcFullPath (const char *sfname,
    char *fullpath, size_t sz, const char *prefix, int pfxlen)
{
  int       type = AUDIOSRC_TYPE_NONE;
  asdylib_t *asdylib;

  if (audiosrc == NULL) {
    return;
  }
  if (sfname == NULL || fullpath == NULL) {
    return;
  }

  type = audiosrcGetType (sfname);
  asdylib = audiosrcGetDylibByType (type);

  *fullpath = '\0';
  if (asdylib != NULL && asdylib->asiFullPath != NULL) {
    if (pfxlen < 0) {
      pfxlen = 0;
    }
    asdylib->asiFullPath (asdylib->asdata, sfname, fullpath, sz, prefix, pfxlen);
  }
}

const char *
audiosrcRelativePath (const char *sfname, int pfxlen)
{
  int         type = AUDIOSRC_TYPE_NONE;
  const char  *p = sfname;
  asdylib_t   *asdylib;

  if (audiosrc == NULL) {
    return sfname;
  }
  if (sfname == NULL) {
    return sfname;
  }

  type = audiosrcGetType (sfname);
  asdylib = audiosrcGetDylibByType (type);

  if (asdylib != NULL && asdylib->asiRelativePath != NULL) {
    p = asdylib->asiRelativePath (asdylib->asdata, sfname, pfxlen);
  }

  return p;
}

size_t
audiosrcDir (const char *sfname, char *dir, size_t sz, int pfxlen)
{
  int       type = AUDIOSRC_TYPE_NONE;
  size_t    rc = 0;
  asdylib_t *asdylib;

  if (audiosrc == NULL) {
    return 0;
  }
  if (sfname == NULL) {
    return 0;
  }

  type = audiosrcGetType (sfname);
  asdylib = audiosrcGetDylibByType (type);

  *dir = '\0';
  if (asdylib != NULL && asdylib->asiDir != NULL) {
    asdylib->asiDir (asdylib->asdata, sfname, dir, sz, pfxlen);
  }

  return rc;
}

asiter_t *
audiosrcStartIterator (const char *uri)
{
  asiter_t  *asiter = NULL;
  int       type;
  asdylib_t *asdylib;

  if (audiosrc == NULL) {
    return NULL;
  }
  if (uri == NULL) {
    return NULL;
  }

  asiter = mdmalloc (sizeof (asiter_t));
  type = audiosrcGetType (uri);
  asdylib = audiosrcGetDylibByType (type);

  asiter->type = type;
  asiter->asiterdata = NULL;
  asiter->asdylib = asdylib;
  asiter->asdata = asdylib->asdata;

  if (asdylib != NULL && asdylib->asiStartIterator != NULL) {
    asiter->asiterdata = asdylib->asiStartIterator (asdylib->asdata, uri);
  }

  if (asiter->asiterdata == NULL) {
    mdfree (asiter);
    asiter = NULL;
  }

  return asiter;
}

void
audiosrcCleanIterator (asiter_t *asiter)
{
  asdylib_t   *asdylib;

  if (audiosrc == NULL) {
    return;
  }
  if (asiter == NULL) {
    return;
  }

  asdylib = asiter->asdylib;
  if (asdylib != NULL && asdylib->asiCleanIterator != NULL) {
    asdylib->asiCleanIterator (asiter->asdata, asiter->asiterdata);
    asiter->asiterdata = NULL;
  }
  mdfree (asiter);
}

int32_t
audiosrcIterCount (asiter_t *asiter)
{
  int32_t     c = 0;
  asdylib_t   *asdylib;

  if (asiter == NULL) {
    return c;
  }

  asdylib = asiter->asdylib;
  if (asdylib != NULL && asdylib->asiIterCount != NULL) {
    c = asdylib->asiIterCount (asiter->asdata, asiter->asiterdata);
  }
  return c;
}

const char *
audiosrcIterate (asiter_t *asiter)
{
  const char  *rval = rval;
  asdylib_t   *asdylib;

  if (asiter == NULL) {
    return NULL;
  }

  asdylib = asiter->asdylib;
  if (asdylib != NULL && asdylib->asiIterCount != NULL) {
    rval = asdylib->asiIterate (asiter->asdata, asiter->asiterdata);
  }

  return rval;
}

bool
audiosrcGetPlaylistNames (int type)
{
  asdylib_t *asdylib;
  bool      rc = false;

  if (audiosrc == NULL) {
    return false;
  }
  if (type == AUDIOSRC_TYPE_NONE || type >= AUDIOSRC_TYPE_MAX) {
    return false;
  }
fprintf (stderr, "as-gpln: type %d\n", type);

  asdylib = audiosrcGetDylibByType (type);

  if (asdylib != NULL && asdylib->asiGetPlaylistNames != NULL) {
fprintf (stderr, "as-gpln: b4 call\n");
    rc = asdylib->asiGetPlaylistNames (asdylib->asdata);
  }

  return rc;
}


/* internal routines */

static asdylib_t *
audiosrcGetDylibByType (int type)
{
  int       idx;
  asdylib_t *asdylib;

  if (type == AUDIOSRC_TYPE_NONE) {
    return NULL;
  }

  idx = audiosrc->typeidx [type];
  if (idx == AS_DYLIB_UNDEF) {
    return NULL;
  }

  asdylib = &audiosrc->asdylib [idx];
  return asdylib;
}
