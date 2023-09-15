/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>

#include "audioid.h"
#include "log.h"
#include "manageui.h"
#include "mdebug.h"
#include "nlist.h"
#include "uiaudioid.h"
#include "uisongsel.h"
#include "uiwcont.h"

typedef struct manageaudioid {
  dispsel_t         *dispsel;
  nlist_t           *options;
  uiwcont_t         *windowp;
  uiwcont_t         *errorMsg;
  uiwcont_t         *statusMsg;
  uisongsel_t       *uisongsel;
  audioid_t         *audioid;
  uiaudioid_t       *uiaudioid;
} manageaudioid_t;

manageaudioid_t *
manageAudioIdAlloc (dispsel_t *dispsel, nlist_t *options, uiwcont_t *window,
    uiwcont_t *errorMsg, uiwcont_t *statusMsg)
{
  manageaudioid_t *maudioid;

  maudioid = mdmalloc (sizeof (manageaudioid_t));
  maudioid->dispsel = dispsel;
  maudioid->options = options;
  maudioid->windowp = window;
  maudioid->errorMsg = errorMsg;
  maudioid->statusMsg = statusMsg;
  maudioid->uisongsel = NULL;
  maudioid->audioid = audioidInit ();
  maudioid->uiaudioid = uiaudioidInit (maudioid->options, maudioid->dispsel);
  return maudioid;
}

void
manageAudioIdFree (manageaudioid_t *maudioid)
{
  if (maudioid == NULL) {
    return;
  }

  audioidFree (maudioid->audioid);
  mdfree (maudioid);
}

uiwcont_t *
manageAudioIdBuildUI (manageaudioid_t *maudioid, uisongsel_t *uisongsel)
{
  uiwcont_t   *uip;

  if (maudioid == NULL) {
    return NULL;
  }

  maudioid->uisongsel = uisongsel;
  uip = uiaudioidBuildUI (maudioid->uisongsel, maudioid->uiaudioid,
      maudioid->windowp, maudioid->statusMsg);

  return uip;
}

void
manageAudioIdMainLoop (manageaudioid_t *maudioid)
{
  if (maudioid == NULL) {
    return;
  }

  uiaudioidMainLoop (maudioid->uiaudioid);
}

void
manageAudioIdLoad (manageaudioid_t *maudioid, song_t *song, dbidx_t dbidx)
{
  nlist_t   *resp;

  if (maudioid == NULL) {
    return;
  }

  uiaudioidLoadData (maudioid->uiaudioid, song, dbidx);
  resp = audioidLookup (maudioid->audioid, song);
}

void
manageAudioIdSetSaveCallback (manageaudioid_t *maudioid, callback_t *cb)
{
  if (maudioid == NULL) {
    return;
  }

  uiaudioidSetSaveCallback (maudioid->uiaudioid, cb);
}
