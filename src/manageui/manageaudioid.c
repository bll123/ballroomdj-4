/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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
#include "ui.h"
#include "uiaudioid.h"
#include "uisongsel.h"
#include "uiwcont.h"

typedef struct manageaudioid {
  manageinfo_t      *minfo;
  uisongsel_t       *uisongsel;
  audioid_t         *audioid;
  uiaudioid_t       *uiaudioid;
  song_t            *song;
  uiwcont_t         *audioidmenu;
  int               state;
} manageaudioid_t;

manageaudioid_t *
manageAudioIdAlloc (manageinfo_t *minfo)
{
  manageaudioid_t *maudioid;

  maudioid = mdmalloc (sizeof (manageaudioid_t));
  maudioid->minfo = minfo;
  maudioid->uisongsel = NULL;
  maudioid->audioid = audioidInit ();
  maudioid->uiaudioid = uiaudioidInit (maudioid->minfo->options,
      maudioid->minfo->dispsel);
  maudioid->song = NULL;
  maudioid->audioidmenu = uiMenuAlloc ();
  maudioid->state = BDJ4_STATE_OFF;
  return maudioid;
}

void
manageAudioIdFree (manageaudioid_t *maudioid)
{
  if (maudioid == NULL) {
    return;
  }

  uiaudioidFree (maudioid->uiaudioid);
  audioidFree (maudioid->audioid);
  uiwcontFree (maudioid->audioidmenu);
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
  uip = uiaudioidBuildUI (maudioid->uiaudioid, maudioid->uisongsel,
      maudioid->minfo->window, maudioid->minfo->statusMsg);

  return uip;
}

/* audio identification */

uiwcont_t *
manageAudioIDMenu (manageaudioid_t *maudioid, uiwcont_t *menubar)
{
  uiwcont_t   *menu = NULL;
  uiwcont_t   *menuitem = NULL;

  logProcBegin ();
  if (! uiMenuIsInitialized (maudioid->audioidmenu)) {
    /* empty menu for now */
    menuitem = uiMenuAddMainItem (menubar, maudioid->audioidmenu, "");
    menu = uiCreateSubMenu (menuitem);
    uiwcontFree (menuitem);

    uiMenuSetInitialized (maudioid->audioidmenu);
    uiwcontFree (menu);
  }

  uiMenuDisplay (maudioid->audioidmenu);

  logProcEnd ("");
  return maudioid->audioidmenu;
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
      uiLabelSetText (maudioid->minfo->statusMsg, maudioid->minfo->pleasewaitmsg);
      /* if the ui is repeating, don't try to set the display list, */
      /* stay in the start state and wait for repeat to go off */
      if (! uiaudioidIsRepeating (maudioid->uiaudioid)) {
        maudioid->state = BDJ4_STATE_WAIT;
        uiaudioidResetDisplayList (maudioid->uiaudioid);
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
      uiLabelSetText (maudioid->minfo->statusMsg, "");
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
  if (! uiaudioidInSave (maudioid->uiaudioid)) {
    maudioid->state = BDJ4_STATE_START;
  }
}

void
manageAudioIdSetSaveCallback (manageaudioid_t *maudioid, callback_t *cb)
{
  if (maudioid == NULL) {
    return;
  }

  uiaudioidSetSaveCallback (maudioid->uiaudioid, cb);
}

void
manageAudioIdSavePosition (manageaudioid_t *maudioid)
{
  uiaudioidSavePanePosition (maudioid->uiaudioid);
}

