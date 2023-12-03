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

#include "bdj4.h"
#include "audiosrc.h"
#include "dyintfc.h"
#include "dylib.h"
#include "mdebug.h"
#include "pathbld.h"
#include "slist.h"
#include "sysvars.h"

typedef struct audiosrc {
  dlhandle_t        *dlHandle;
  bool              *(*audiosrciExists) (const char *nm);
  bool              *(*audiosrciRemove) (const char *nm);
} audiosrc_t;

audiosrc_t *
audiosrcInit (const char *pkg)
{
  audiosrc_t    *audiosrc;
  char          dlpath [MAXPATHLEN];

  audiosrc = mdmalloc (sizeof (audiosrc_t));
  audiosrc->audiosrciExists = NULL;
  audiosrc->audiosrciRemove = NULL;

  pathbldMakePath (dlpath, sizeof (dlpath),
      pkg, sysvarsGetStr (SV_SHLIB_EXT), PATHBLD_MP_DIR_EXEC);
  audiosrc->dlHandle = dylibLoad (dlpath);
  if (audiosrc->dlHandle == NULL) {
    fprintf (stderr, "Unable to open library %s\n", dlpath);
    mdfree (audiosrc);
    return NULL;
  }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
  audiosrc->audiosrciExists = dylibLookup (audiosrc->dlHandle, "audiosrciExists");
  audiosrc->audiosrciRemove = dylibLookup (audiosrc->dlHandle, "audiosrciRemove");
#pragma clang diagnostic pop

  return audiosrc;
}

void
audiosrcFree (audiosrc_t *audiosrc)
{
  if (audiosrc == NULL) {
    return;
  }

  if (audiosrc->dlHandle != NULL) {
    dylibClose (audiosrc->dlHandle);
  }
  mdfree (audiosrc);
}

bool
audiosrcExists (audiosrc_t *audiosrc, const char *nm)
{
  bool rc = false;

  if (audiosrc != NULL && audiosrc->audiosrciExists != NULL && nm != NULL) {
    rc = audiosrc->audiosrciExists (nm);
  }

  return rc;
}

bool
audiosrcRemove (audiosrc_t *audiosrc, const char *nm)
{
  bool rc = false;

  if (audiosrc != NULL && audiosrc->audiosrciRemove != NULL && nm != NULL) {
    rc = audiosrc->audiosrciRemove (nm);
  }

  return rc;
}

slist_t *
audiosrcInterfaceList (void)
{
  slist_t     *interfaces;

  interfaces = dyInterfaceList (LIBAUDIOSRC_PFX, "audiosrciDesc");
  return interfaces;
}

