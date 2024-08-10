/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
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
#include "bdj4.h"
#include "bdjstring.h"
#include "fileop.h"
#include "mdebug.h"
#include "pathbld.h"
#include "sysvars.h"

typedef struct asiter {
  int           type;
  asiterdata_t  *asidata;
} asiter_t;

int
audiosrcGetType (const char *nm)
{
  int     type = AUDIOSRC_TYPE_FILE;      // default

  /* no point in trying to cache this, as the name comparison */
  /* is longer than the comparison done to determine the file type */

  if (fileopIsAbsolutePath (nm)) {
    type = AUDIOSRC_TYPE_FILE;
  }

  return type;
}

bool
audiosrcExists (const char *nm)
{
  int     type = AUDIOSRC_TYPE_FILE;
  bool    exists = false;


  if (nm == NULL) {
    return false;
  }

  type = audiosrcGetType (nm);

  if (type == AUDIOSRC_TYPE_FILE) {
    exists = audiosrcfileExists (nm);
  }

  return exists;
}

bool
audiosrcOriginalExists (const char *nm)
{
  int     type = AUDIOSRC_TYPE_FILE;
  bool    exists = false;


  if (nm == NULL) {
    return false;
  }

  type = audiosrcGetType (nm);

  if (type == AUDIOSRC_TYPE_FILE) {
    exists = audiosrcfileOriginalExists (nm);
  }

  return exists;
}

bool
audiosrcRemove (const char *nm)
{
  int     type = AUDIOSRC_TYPE_FILE;
  bool    rc = false;

  type = audiosrcGetType (nm);

  if (type == AUDIOSRC_TYPE_FILE) {
    rc = audiosrcfileRemove (nm);
  }

  return rc;
}

bool
audiosrcPrep (const char *sfname, char *tempnm, size_t sz)
{
  int     type = AUDIOSRC_TYPE_FILE;
  bool    rc;

  type = audiosrcGetType (sfname);

  if (type == AUDIOSRC_TYPE_FILE) {
    rc = audiosrcfilePrep (sfname, tempnm, sz);
  } else {
    /* in most cases, just return the same URI */
    strlcpy (tempnm, sfname, sz);
    rc = true;
  }

  return rc;
}

void
audiosrcPrepClean (const char *sfname, const char *tempnm)
{
  int   type = AUDIOSRC_TYPE_FILE;

  type = audiosrcGetType (sfname);

  if (type == AUDIOSRC_TYPE_FILE) {
    audiosrcfilePrepClean (tempnm);
  }
}

const char *
audiosrcPrefix (const char *sfname)
{
  const char  *pfx = "";
  int         type = AUDIOSRC_TYPE_FILE;

  type = audiosrcGetType (sfname);
  if (type == AUDIOSRC_TYPE_FILE) {
    pfx = audiosrcfilePrefix ();
  }

  return pfx;
}

void
audiosrcURI (const char *sfname, char *uri, size_t sz,
    int pfxlen, const char *oldfn)
{
  int     type = AUDIOSRC_TYPE_FILE;

  if (uri == NULL) {
    return;
  }

  type = audiosrcGetType (sfname);

  *uri = '\0';
  if (type == AUDIOSRC_TYPE_FILE) {
    audiosrcfileURI (sfname, uri, sz, pfxlen, oldfn);
  }
}

void
audiosrcFullPath (const char *sfname, char *fullpath, size_t sz,
    int pfxlen, const char *oldfn)
{
  int     type = AUDIOSRC_TYPE_FILE;

  if (fullpath == NULL) {
    return;
  }

  type = audiosrcGetType (sfname);

  *fullpath = '\0';
  if (type == AUDIOSRC_TYPE_FILE) {
    audiosrcfileFullPath (sfname, fullpath, sz, pfxlen, oldfn);
  } else {
    strlcpy (fullpath, sfname, sz);
  }
}

const char *
audiosrcRelativePath (const char *sfname, int pfxlen)
{
  int         type = AUDIOSRC_TYPE_FILE;
  const char  *p = sfname;

  type = audiosrcGetType (sfname);

  if (type == AUDIOSRC_TYPE_FILE) {
    p = audiosrcfileRelativePath (sfname, pfxlen);
  }

  return p;
}

size_t
audiosrcDir (const char *sfname, char *dir, size_t sz, int pfxlen)
{
  int     type = AUDIOSRC_TYPE_FILE;
  size_t  rc = 0;

  type = audiosrcGetType (sfname);

  *dir = '\0';
  if (type == AUDIOSRC_TYPE_FILE) {
    rc = audiosrcfileDir (sfname, dir, sz, pfxlen);
  }

  return rc;
}

asiter_t *
audiosrcStartIterator (const char *uri)
{
  asiter_t  *asiter = NULL;
  int       type;

  if (uri == NULL) {
    return NULL;
  }

  asiter = mdmalloc (sizeof (asiter_t));
  type = audiosrcGetType (uri);
  asiter->type = type;
  asiter->asidata = NULL;

  if (type == AUDIOSRC_TYPE_FILE) {
    asiter->asidata = audiosrcfileStartIterator (uri);
  }

  if (asiter->asidata == NULL) {
    mdfree (asiter);
    asiter = NULL;
  }

  return asiter;
}

void
audiosrcCleanIterator (asiter_t *asiter)
{
  if (asiter == NULL) {
    return;
  }

  if (asiter->type == AUDIOSRC_TYPE_FILE) {
    audiosrcfileCleanIterator (asiter->asidata);
    asiter->asidata = NULL;
  }
  mdfree (asiter);
}

int32_t
audiosrcIterCount (asiter_t *asiter)
{
  int32_t   c = 0;

  if (asiter == NULL) {
    return c;
  }

  if (asiter->type == AUDIOSRC_TYPE_FILE) {
    c = audiosrcfileIterCount (asiter->asidata);
  }
  return c;
}

const char *
audiosrcIterate (asiter_t *asiter)
{
  const char    *rval = rval;

  if (asiter == NULL) {
    return NULL;
  }

  if (asiter->type == AUDIOSRC_TYPE_FILE) {
    rval = audiosrcfileIterate (asiter->asidata);
  }

  return rval;
}
