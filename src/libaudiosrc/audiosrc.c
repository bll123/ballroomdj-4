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
#include <ctype.h>

#include "asconf.h"
#include "audiosrc.h"
#include "bdj4.h"
#include "bdjopt.h"
#include "bdjregex.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "dyintfc.h"
#include "dylib.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "pathbld.h"
#include "pathinfo.h"
#include "slist.h"
#include "sysvars.h"

typedef struct {
  asdata_t          *asdata;
  dlhandle_t        *dlHandle;
  asdata_t          *(*asiInit) (const char *, const char *);
  void              (*asiFree) (asdata_t *);
  void              (*asiPostInit) (asdata_t *, const char *);
  int               (*asiTypeIdent) (void);
  bool              (*asiIsTypeMatch) (asdata_t *, const char *nm);
  bool              (*asiCheckConnection) (asdata_t *, int askey, const char *uri);
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
  asiterdata_t      *(*asiStartIterator) (asdata_t *, asitertype_t asitertype, const char *uri, const char *nm, int askey);
  void              (*asiCleanIterator) (asdata_t *, asiterdata_t *asiterdata);
  int32_t           (*asiIterCount) (asdata_t *, asiterdata_t *asiterdata);
  const char        *(*asiIterate) (asdata_t *, asiterdata_t *asiterdata);
  const char        *(*asiIterateValue) (asdata_t *, asiterdata_t *asiterdata, const char *key);
  int               type;
  bool              enabled;
} asdylib_t;

typedef struct audiosrc {
  ilist_t           *asdylist;
  asdylib_t         *asdylib;
  bdjregex_t        *protorx;
  asconf_t          *asconf;
  int               ascount;
  int               asactivecount;
  int               typeidx [AUDIOSRC_TYPE_MAX];
  bool              enabled [AUDIOSRC_TYPE_MAX];
} audiosrc_t;

typedef struct asiter {
  asiterdata_t  *asiterdata;
  asdylib_t     *asdylib;
  asdata_t      *asdata;
  int           type;
  asitertype_t  itertype;
  int           iteridx;
} asiter_t;

enum {
  AS_DYLIB_UNDEF = -1,
};

static asdylib_t * audiosrcGetDylibByType (int type);

static audiosrc_t *audiosrc = NULL;

/* both bdjopt and bdjvars should be initialized */
void
audiosrcInit (void)
{
  ilistidx_t      askey;
  ilistidx_t      iteridx;

  if (audiosrc != NULL) {
    return;
  }

  audiosrc = mdmalloc (sizeof (audiosrc_t));
  audiosrc->ascount = 0;
  audiosrc->asactivecount = 0;
  audiosrc->asdylist = NULL;
  audiosrc->asdylib = NULL;
  audiosrc->protorx = NULL;

  audiosrc->asconf = asconfAlloc ();
  audiosrc->protorx = regexInit ("^[[:alpha:]][[:alnum:]+.-]*://");

  audiosrc->asdylist = dyInterfaceList ("libas", "asiDesc");
  audiosrc->ascount = ilistGetCount (audiosrc->asdylist);

  if (audiosrc->ascount == 0) {
    return;
  }

  for (int i = 0; i < AUDIOSRC_TYPE_MAX; ++i) {
    audiosrc->enabled [i] = false;
    audiosrc->typeidx [i] = AS_DYLIB_UNDEF;
  }
  /* 'file' audio source is always enabled */
  audiosrc->enabled [AUDIOSRC_TYPE_FILE] = true;

  asconfStartIterator (audiosrc->asconf, &iteridx);
  while ((askey = asconfIterate (audiosrc->asconf, &iteridx)) >= 0) {
    int   mode;
    int   type;

    mode = asconfGetNum (audiosrc->asconf, askey, ASCONF_MODE);
    if (mode != ASCONF_MODE_CLIENT) {
      continue;
    }

    type = asconfGetNum (audiosrc->asconf, askey, ASCONF_TYPE);
    if (type < 0) {
      continue;
    }
    audiosrc->enabled [type] = true;
  }

  audiosrc->asdylib = mdmalloc (sizeof (asdylib_t) * audiosrc->ascount);

  for (int i = 0; i < audiosrc->ascount; ++i) {
    asdylib_t   *asdylib;

    asdylib = &audiosrc->asdylib [i];

    asdylib->type = AUDIOSRC_TYPE_NONE;
    asdylib->enabled = false;
    asdylib->asdata = NULL;
    asdylib->dlHandle = NULL;
    asdylib->asiInit = NULL;
    asdylib->asiFree = NULL;
    asdylib->asiPostInit = NULL;
    asdylib->asiTypeIdent = NULL;
    asdylib->asiIsTypeMatch = NULL;
    asdylib->asiCheckConnection = NULL;
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
    asdylib->asiIterateValue = NULL;
  }

  for (int i = 0; i < audiosrc->ascount; ++i) {
    const char  *pkgnm;
    char        dlpath [MAXPATHLEN];
    asdylib_t   *asdylib;

    pkgnm = ilistGetStr (audiosrc->asdylist, i, DYI_LIB);
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
    asdylib->asiInit = dylibLookup (asdylib->dlHandle, "asiInit");
    asdylib->asiFree = dylibLookup (asdylib->dlHandle, "asiFree");
    asdylib->asiPostInit = dylibLookup (asdylib->dlHandle, "asiPostInit");
    asdylib->asiTypeIdent = dylibLookup (asdylib->dlHandle, "asiTypeIdent");
    asdylib->asiIsTypeMatch = dylibLookup (asdylib->dlHandle, "asiIsTypeMatch");
    asdylib->asiCheckConnection = dylibLookup (asdylib->dlHandle, "asiCheckConnection");
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
    asdylib->asiIterateValue = dylibLookup (asdylib->dlHandle, "asiIterateValue");
#pragma clang diagnostic pop

    if (asdylib->asiTypeIdent != NULL) {
      asdylib->type = asdylib->asiTypeIdent ();
    }

    if (audiosrc->enabled [asdylib->type] == false) {
      continue;
    }

    asdylib->enabled = true;

    if (asdylib->asiInit != NULL) {
      asdylib->asdata = asdylib->asiInit (bdjvarsGetStr (BDJV_DELETE_PFX),
          bdjvarsGetStr (BDJV_ORIGINAL_EXT));
    }

    ++audiosrc->asactivecount;

    if (asdylib->asiPostInit != NULL) {
      asdylib->asiPostInit (asdylib->asdata, bdjoptGetStr (OPT_M_DIR_MUSIC));
    }
    audiosrc->typeidx [asdylib->type] = i;
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
    if (asdylib == NULL) {
      continue;
    }

    if (asdylib->asiFree != NULL) {
      asdylib->asiFree (asdylib->asdata);
    }
    if (asdylib->dlHandle != NULL) {
      dylibClose (asdylib->dlHandle);
    }
  }

  asconfFree (audiosrc->asconf);
  ilistFree (audiosrc->asdylist);
  dataFree (audiosrc->asdylib);
  regexFree (audiosrc->protorx);
  audiosrc->ascount = 0;
  audiosrc->asactivecount = 0;

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
    if (! asdylib->enabled) {
      continue;
    }

    if (asdylib->asiPostInit != NULL) {
      asdylib->asiPostInit (asdylib->asdata, bdjoptGetStr (OPT_M_DIR_MUSIC));
    }
  }
}

int
audiosrcGetActiveCount (void)
{
  if (audiosrc == NULL) {
    return 0;
  }

  return audiosrc->asactivecount;
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
    if (! asdylib->enabled) {
      continue;
    }

    if (asdylib->type != AUDIOSRC_TYPE_NONE && asdylib->asiIsTypeMatch != NULL) {
      if (asdylib->asiIsTypeMatch (asdylib->asdata, nm)) {
        type = asdylib->type;
        break;
      } else {
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
audiosrcCheckConnection (int askey, const char *uri)
{
  int       type = AUDIOSRC_TYPE_NONE;
  bool      rc = false;
  asdylib_t *asdylib;

  if (audiosrc == NULL) {
    return false;
  }
  if (askey < 0 || askey >= audiosrc->ascount) {
    return false;
  }

  type = asconfGetNum (audiosrc->asconf, askey, ASCONF_TYPE);
  asdylib = audiosrcGetDylibByType (type);

  if (asdylib != NULL && asdylib->asiCheckConnection != NULL) {
    rc = asdylib->asiCheckConnection (asdylib->asdata, askey, uri);
  }

  return rc;
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

/* the asitertype determines what will be iterated through */
/* the 'file' audio source only has a directory iterator */
/* other audio sources have playlist-names (used for import-playlist), */
/* songs in a playlist, and song-tags */
asiter_t *
audiosrcStartIterator (int type, asitertype_t asitertype,
    const char *uri, const char *nm, int askey)
{
  asiter_t  *asiter = NULL;
  asdylib_t *asdylib;

  if (audiosrc == NULL) {
    return NULL;
  }
  if (type >= AUDIOSRC_TYPE_MAX) {
    return NULL;
  }
  if ((asitertype == AS_ITER_PL || asitertype == AS_ITER_TAGS) &&
      nm == NULL) {
    return NULL;
  }
  if (type == AUDIOSRC_TYPE_BDJ4 &&
      (askey < 0 || askey >= audiosrc->ascount)) {
    return NULL;
  }

  asiter = mdmalloc (sizeof (asiter_t));
  asdylib = audiosrcGetDylibByType (type);

  asiter->type = type;
  asiter->itertype = asitertype;
  asiter->asiterdata = NULL;
  asiter->asdylib = asdylib;
  asiter->asdata = NULL;
  if (asdylib != NULL) {
    asiter->asdata = asdylib->asdata;
  }
  asiter->iteridx = 0;

  if (asitertype == AS_ITER_AUDIO_SRC) {
    return asiter;
  }

  if (asdylib != NULL && asdylib->asiStartIterator != NULL) {
    asiter->asiterdata = asdylib->asiStartIterator (
        asdylib->asdata, asitertype, uri, nm, askey);
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
  if (asiter->asiterdata == NULL) {
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

  if (asiter->itertype == AS_ITER_AUDIO_SRC) {
    c = audiosrc->asactivecount;
  } else {
    asdylib = asiter->asdylib;
    if (asdylib != NULL && asdylib->asiIterCount != NULL) {
      c = asdylib->asiIterCount (asiter->asdata, asiter->asiterdata);
    }
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

  if (asiter->itertype == AS_ITER_AUDIO_SRC) {
    while (asiter->iteridx < audiosrc->ascount) {
      asdylib = &audiosrc->asdylib [asiter->iteridx];
      if (asdylib != NULL && asdylib->enabled) {
        break;
      }
      ++asiter->iteridx;
    }
    rval = ilistGetStr (audiosrc->asdylist, asiter->iteridx, DYI_DESC);
    ++asiter->iteridx;
  } else {
    asdylib = asiter->asdylib;
    if (asdylib != NULL && asdylib->asiIterate != NULL) {
      rval = asdylib->asiIterate (asiter->asdata, asiter->asiterdata);
    }
  }

  return rval;
}

const char *
audiosrcIterateValue (asiter_t *asiter, const char *key)
{
  const char  *rval = rval;
  asdylib_t   *asdylib;

  if (asiter == NULL) {
    return NULL;
  }
  if (asiter->itertype == AS_ITER_AUDIO_SRC) {
    return NULL;
  }

  asdylib = asiter->asdylib;
  if (asdylib != NULL && asdylib->asiIterateValue != NULL) {
    rval = asdylib->asiIterateValue (asiter->asdata, asiter->asiterdata, key);
  }

  return rval;
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

