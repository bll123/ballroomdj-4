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
/* eventually songutilfullfilename will be removed and moved here */
#include "songutil.h"

const char *
audiosrciDesc (void)
{
  /* CONTEXT: audio source: audio files on disk */
  return _("Audio Files");
}

bool
audiosrciExists (const char *nm)
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
audiosrciRemove (const char *nm)
{
  int   rc;
  char  newnm [MAXPATHLEN];
  char  *ffn;

  ffn = songutilFullFileName (nm);
  if (! fileopFileExists (ffn)) {
    mdfree (ffn);
    return false;
  }

  snprintf (newnm, sizeof (newnm), bdjvarsGetStr (BDJV_DELETE_PFX), ffn);

  /* never overwrite an existing file */
  if (fileopFileExists (newnm)) {
    return false;
  }

  rc = filemanipMove (ffn, newnm);
  mdfree (ffn);
  return rc == 0 ? true : false;
}

