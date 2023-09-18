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
  const char        *pleasewaitmsg;
  uisongsel_t       *uisongsel;
  audioid_t         *audioid;
  uiaudioid_t       *uiaudioid;
  song_t            *song;
  int               state;
} manageaudioid_t;

manageaudioid_t *
manageAudioIdAlloc (dispsel_t *dispsel, nlist_t *options, uiwcont_t *window,
    uiwcont_t *errorMsg, uiwcont_t *statusMsg, const char *pleasewaitmsg)
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
  maudioid->pleasewaitmsg = pleasewaitmsg;
  maudioid->song = NULL;
  maudioid->state = BDJ4_STATE_OFF;
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

  switch (maudioid->state) {
    case BDJ4_STATE_OFF: {
      break;
    }
    case BDJ4_STATE_START: {
      uiLabelSetText (maudioid->statusMsg, maudioid->pleasewaitmsg);
      /* if the ui is repeating, don't try to set the display list, */
      /* stay in the start state and wait for repeat to go off */
      if (! uiaudioidIsRepeating (maudioid->uiaudioid)) {
        maudioid->state = BDJ4_STATE_WAIT;
      }
      break;
    }
    case BDJ4_STATE_WAIT: {
      if (audioidLookup (maudioid->audioid, maudioid->song)) {
        audioidStartIterator (maudioid->audioid);
        maudioid->state = BDJ4_STATE_PROCESS;
      }
      break;
    }
    case BDJ4_STATE_PROCESS: {
      int   key;

      while ((key = audioidIterate (maudioid->audioid)) >= 0) {
        nlist_t   *dlist;

        dlist = audioidGetList (maudioid->audioid, key);
        uiaudioidSetDisplayList (maudioid->uiaudioid, dlist);
      }
      uiaudioidFinishDisplayList (maudioid->uiaudioid);
      maudioid->state = BDJ4_STATE_FINISH;
      break;
    }
    case BDJ4_STATE_FINISH: {
      uiLabelSetText (maudioid->statusMsg, "");
      maudioid->state = BDJ4_STATE_OFF;
      break;
    }
  }
}

void
manageAudioIdLoad (manageaudioid_t *maudioid, song_t *song, dbidx_t dbidx)
{
  if (maudioid == NULL) {
    return;
  }

  uiaudioidLoadData (maudioid->uiaudioid, song, dbidx);
  maudioid->song = song;
  maudioid->state = BDJ4_STATE_START;
}

void
manageAudioIdSetSaveCallback (manageaudioid_t *maudioid, callback_t *cb)
{
  if (maudioid == NULL) {
    return;
  }

  uiaudioidSetSaveCallback (maudioid->uiaudioid, cb);
}
