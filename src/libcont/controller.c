/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
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
#include "bdjvars.h"
#include "callback.h"
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
  void              (*contiSetCallbacks) (contdata_t *contdata, callback_t *cb, callback_t *cburi);
  void              (*contiSetPlayState) (contdata_t *contdata, int state);
  void              (*contiSetRepeatState) (contdata_t *contdata, bool state);
  void              (*contiSetPosition) (contdata_t *contdata, int32_t pos);
  void              (*contiSetRate) (contdata_t *contdata, int rate);
  void              (*contiSetVolume) (contdata_t *contdata, int volume);
  void              (*contiSetCurrent) (contdata_t *contdata, contmetadata_t *cmetadata);
} controller_t;

controller_t *
controllerInit (const char *contpkg)
{
  controller_t  *cont;
  char          dlpath [MAXPATHLEN];

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
  cont->contiSetCallbacks = dylibLookup (cont->dlHandle, "contiSetCallbacks");
  cont->contiSetPlayState = dylibLookup (cont->dlHandle, "contiSetPlayState");
  cont->contiSetRepeatState = dylibLookup (cont->dlHandle, "contiSetRepeatState");
  cont->contiSetPosition = dylibLookup (cont->dlHandle, "contiSetPosition");
  cont->contiSetRate = dylibLookup (cont->dlHandle, "contiSetRate");
  cont->contiSetVolume = dylibLookup (cont->dlHandle, "contiSetVolume");
  cont->contiSetCurrent = dylibLookup (cont->dlHandle, "contiSetCurrent");
#pragma clang diagnostic pop

  if (cont->contiInit != NULL) {
    cont->contdata = cont->contiInit (bdjvarsGetStr (BDJV_INSTANCE_NAME));
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

void
controllerSetCallbacks (controller_t *cont, callback_t *cb, callback_t *cburi)
{
  if (cont == NULL || cb == NULL) {
    return;
  }

  if (cont->contdata != NULL && cont->contiSetCallbacks != NULL) {
    cont->contiSetCallbacks (cont->contdata, cb, cburi);
  }
}

void
controllerSetPlayState (controller_t *cont, int state)
{
  if (cont == NULL) {
    return;
  }

  if (cont->contdata != NULL && cont->contiSetPlayState != NULL) {
    cont->contiSetPlayState (cont->contdata, state);
  }
}

void
controllerSetRepeatState (controller_t *cont, bool state)
{
  if (cont == NULL) {
    return;
  }

  if (cont->contdata != NULL && cont->contiSetRepeatState != NULL) {
    cont->contiSetRepeatState (cont->contdata, state);
  }
}

void
controllerSetPosition (controller_t *cont, int32_t pos)
{
  if (cont == NULL) {
    return;
  }

  if (cont->contdata != NULL && cont->contiSetPosition != NULL) {
    cont->contiSetPosition (cont->contdata, pos);
  }
}

void
controllerSetRate (controller_t *cont, int rate)
{
  if (cont == NULL) {
    return;
  }

  if (cont->contdata != NULL && cont->contiSetRate != NULL) {
    cont->contiSetRate (cont->contdata, rate);
  }
}

void
controllerSetVolume (controller_t *cont, int volume)
{
  if (cont == NULL) {
    return;
  }

  if (cont->contdata != NULL && cont->contiSetVolume != NULL) {
    cont->contiSetVolume (cont->contdata, volume);
  }
}

void
controllerInitMetadata (contmetadata_t *cmetadata)
{
  if (cmetadata == NULL) {
    return;
  }
  cmetadata->album = NULL;
  cmetadata->albumartist = NULL;
  cmetadata->artist = NULL;
  cmetadata->title = NULL;
  cmetadata->genre = NULL;
  cmetadata->uri = NULL;
  cmetadata->arturi = NULL;
  cmetadata->songstart = 0;
  cmetadata->songend = 0;
  cmetadata->trackid = 0;
  cmetadata->duration = 0;
}

void
controllerSetCurrent (controller_t *cont, contmetadata_t *cmetadata)
{
  if (cont == NULL || cmetadata == NULL) {
    return;
  }

  if (cont->contdata != NULL && cont->contiSetCurrent != NULL) {
    cont->contiSetCurrent (cont->contdata, cmetadata);
  }
}

ilist_t *
controllerInterfaceList (void)
{
  ilist_t   *interfaces;

  interfaces = dyInterfaceList ("libcont", "contiDesc");
  return interfaces;
}

