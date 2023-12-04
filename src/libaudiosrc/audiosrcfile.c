/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
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
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "fileop.h"
#include "filemanip.h"
#include "mdebug.h"
#include "pathutil.h"

static void audiosrcfileFullPath (const char *sfname, char *buff, size_t sz);

bool
audiosrcfileExists (const char *nm)
{
  bool    rc;
  char    ffn [MAXPATHLEN];

  audiosrcfileFullPath (nm, ffn, sizeof (ffn));
  rc = fileopFileExists (ffn);
  return rc;
}

/* does not actually remove the file, renames it with a 'delete-' prefix */
bool
audiosrcfileRemove (const char *nm)
{
  int           rc = false;
  char          ffn [MAXPATHLEN];
  char          newnm [MAXPATHLEN];
  pathinfo_t    *pi;

  audiosrcfileFullPath (nm, ffn, sizeof (ffn));
  if (! fileopFileExists (ffn)) {
    return false;
  }

  pi = pathInfo (ffn);
  snprintf (newnm, sizeof (newnm), "%.*s/%s%.*s",
      (int) pi->dlen, pi->dirname,
      bdjvarsGetStr (BDJV_DELETE_PFX),
      (int) pi->flen, pi->filename);
  pathInfoFree (pi);

  /* never overwrite an existing file */
  if (fileopFileExists (newnm)) {
    return false;
  }

  rc = filemanipMove (ffn, newnm);
  return rc == 0 ? true : false;
}

char *
audiosrcfilePrep (const char *nm)
{
  return NULL;
}

static void
audiosrcfileFullPath (const char *sfname, char *buff, size_t sz)
{
  *buff = '\0';

  if (sfname == NULL) {
    return;
  }

  if (fileopIsAbsolutePath (sfname)) {
    strlcpy (buff, sfname, sz);
  } else {
    snprintf (buff, sz, "%s/%s", bdjoptGetStr (OPT_M_DIR_MUSIC), sfname);
  }
}

