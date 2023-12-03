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
#include "bdjvars.h"
#include "fileop.h"
#include "filemanip.h"
#include "mdebug.h"
#include "pathutil.h"
/* eventually songutilfullfilename will be removed and moved here */
#include "songutil.h"

bool
audiosrcfileExists (const char *nm)
{
  bool    rc;
  char    *ffn;

  ffn = songutilFullFileName (nm);
  rc = fileopFileExists (ffn);
  mdfree (ffn);
  return rc;
}

/* does not actually remove the file, renames it with a 'delete-' prefix */
bool
audiosrcfileRemove (const char *nm)
{
  int           rc = false;
  char          *ffn;
  char          newnm [MAXPATHLEN];
  pathinfo_t    *pi;

  ffn = songutilFullFileName (nm);
  if (! fileopFileExists (ffn)) {
    mdfree (ffn);
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
    mdfree (ffn);
    return false;
  }

  rc = filemanipMove (ffn, newnm);
  mdfree (ffn);
  return rc == 0 ? true : false;
}

char *
audiosrcfilePrep (const char *nm)
{
  return NULL;
}

