/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "controller.h"
#include "pathbld.h"
#include "dyintfc.h"
#include "dylib.h"
#include "mdebug.h"
#include "sysvars.h"

typedef struct controller {
  dlhandle_t        *dlHandle;
  contdata_t        *contdata;
  contdata_t        *(*contiInit) (const char *instname);
  void              (*contiSetup) (contdata_t *contdata);
  void              (*contiFree) (contdata_t *contdata);
  bool              (*contiCheckReady) (contdata_t *contdata);
} controller_t;

controller_t *
controllerInit (const char *contpkg)
{
  controller_t  *cont;
  char          dlpath [MAXPATHLEN];
  char          instname [200];
  char          temp [40];
  int           altidx;
  int           profidx;

  cont = mdmalloc (sizeof (controller_t));
  cont->contdata = NULL;
  cont->contiInit = NULL;
  cont->contiSetup = NULL;
  cont->contiFree = NULL;
  cont->contiCheckReady = NULL;

  pathbldMakePath (dlpath, sizeof (dlpath),
      contpkg, sysvarsGetStr (SV_SHLIB_EXT), PATHBLD_MP_DIR_EXEC);
  cont->dlHandle = dylibLoad (dlpath);
  if (cont->dlHandle == NULL) {
    fprintf (stderr, "Unable to open library %s\n", dlpath);
    mdfree (cont);
    return NULL;
  }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
  cont->contiInit = dylibLookup (cont->dlHandle, "contiInit");
  cont->contiSetup = dylibLookup (cont->dlHandle, "contiSetup");
  cont->contiFree = dylibLookup (cont->dlHandle, "contiFree");
  cont->contiCheckReady = dylibLookup (cont->dlHandle, "contiCheckReady");
#pragma clang diagnostic pop

  strlcpy (instname, BDJ4_NAME, sizeof (instname));
  altidx = sysvarsGetNum (SVL_ALTIDX);
  if (altidx > 0) {
    snprintf (temp, sizeof (temp), "-%02d", altidx);
    strlcat (instname, temp, sizeof (instname));
  }
  profidx = sysvarsGetNum (SVL_PROFILE_IDX);
  if (altidx > 0 || profidx > 0) {
    snprintf (temp, sizeof (temp), "-%02d", profidx);
    strlcat (instname, temp, sizeof (instname));
  }

  if (cont->contiInit != NULL) {
    cont->contdata = cont->contiInit (instname);
  }
  return cont;
}

void
controllerFree (controller_t *cont)
{
  if (cont == NULL) {
    return;
  }

  if (cont->contdata != NULL && cont->contiFree != NULL) {
    cont->contiFree (cont->contdata);
  }
  if (cont->dlHandle != NULL) {
    dylibClose (cont->dlHandle);
  }
  mdfree (cont);
}

void
controllerSetup (controller_t *cont)
{
  if (cont == NULL) {
    return;
  }

  if (cont->contdata != NULL && cont->contiSetup != NULL) {
    cont->contiSetup (cont->contdata);
  }
}

bool
controllerCheckReady (controller_t *cont)
{
  bool    rc = false;

  if (cont == NULL) {
    return rc;
  }

  if (cont->contdata != NULL && cont->contiCheckReady != NULL) {
    rc = cont->contiCheckReady (cont->contdata);
  }

  return rc;
}


ilist_t *
controllerInterfaceList (void)
{
  ilist_t   *interfaces;

  interfaces = dyInterfaceList ("libcont", "contiDesc");
  return interfaces;
}
