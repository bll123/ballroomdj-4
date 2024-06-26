/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "genre.h"
#include "mdebug.h"
#include "ui.h"
#include "callback.h"
#include "uidd.h"
#include "uigenre.h"

typedef struct uigenre {
  genre_t       *genres;
  uiwcont_t     *parentwin;
  uidd_t        *uidd;
  callback_t    *internalselcb;
  callback_t    *selectcb;
  ilist_t       *ddlist;
  nlist_t       *ddlookup;
  ilistidx_t    selectedidx;
  bool          allflag : 1;
} uigenre_t;

static bool uigenreSelectHandler (void *udata, int32_t idx);
static void uigenreCreateGenreList (uigenre_t *uigenre);

uigenre_t *
uigenreDropDownCreate (uiwcont_t *boxp, uiwcont_t *parentwin, bool allflag)
{
  uigenre_t  *uigenre;


  uigenre = mdmalloc (sizeof (uigenre_t));
  uigenre->genres = bdjvarsdfGet (BDJVDF_GENRES);
  uigenre->allflag = allflag;
  uigenre->uidd = NULL;
  uigenre->selectedidx = 0;
  uigenre->parentwin = parentwin;
  uigenre->internalselcb = NULL;
  uigenre->selectcb = NULL;
  uigenre->ddlist = NULL;
  uigenre->ddlookup = NULL;

  uigenreCreateGenreList (uigenre);
  uigenre->internalselcb = callbackInitI (uigenreSelectHandler, uigenre);
  uigenre->uidd = uiddCreate ("uigenre",
      parentwin, boxp, DD_PACK_START,
      uigenre->ddlist, DD_LIST_TYPE_NUM,
      "", DD_REPLACE_TITLE, uigenre->internalselcb);

  return uigenre;
}

void
uigenreFree (uigenre_t *uigenre)
{
  if (uigenre == NULL) {
    return;
  }

  callbackFree (uigenre->internalselcb);
  uiddFree (uigenre->uidd);
  ilistFree (uigenre->ddlist);
  nlistFree (uigenre->ddlookup);
  mdfree (uigenre);
}

ilistidx_t
uigenreGetKey (uigenre_t *uigenre)
{
  if (uigenre == NULL) {
    return 0;
  }

  return uigenre->selectedidx;
}

void
uigenreSetKey (uigenre_t *uigenre, ilistidx_t gkey)
{
  if (uigenre == NULL || uigenre->uidd == NULL) {
    return;
  }

  uigenre->selectedidx = gkey;
  uiddSetSelection (uigenre->uidd, nlistGetNum (uigenre->ddlookup, gkey));
}

void
uigenreSetState (uigenre_t *uigenre, int state)
{
  if (uigenre == NULL || uigenre->uidd == NULL) {
    return;
  }
  uiddSetState (uigenre->uidd, state);
}

void
uigenreSizeGroupAdd (uigenre_t *uigenre, uiwcont_t *sg)
{
  if (uigenre == NULL || uigenre->uidd == NULL) {
    return;
  }
  uiSizeGroupAdd (sg, uiddGetButton (uigenre->uidd));
}

void
uigenreSetCallback (uigenre_t *uigenre, callback_t *cb)
{
  if (uigenre == NULL) {
    return;
  }
  uigenre->selectcb = cb;
}


/* internal routines */

static bool
uigenreSelectHandler (void *udata, int32_t idx)
{
  uigenre_t   *uigenre = udata;

  uigenre->selectedidx = idx;

  if (uigenre->selectcb != NULL) {
    callbackHandlerI (uigenre->selectcb, idx);
  }
  return UICB_CONT;
}

static void
uigenreCreateGenreList (uigenre_t *uigenre)
{
  slist_t   *genrelist;
  slistidx_t  iteridx;
  ilist_t     *ddlist;
  nlist_t     *ddlookup;
  const char  *disp;
  int         count;
  ilistidx_t  gkey;
  int         idx;

  genrelist = genreGetList (uigenre->genres);
  count = slistGetCount (genrelist);
  ddlist = ilistAlloc ("uigenre", LIST_ORDERED);
  ilistSetSize (ddlist, count);
  ddlookup = nlistAlloc ("uigenre-lookup", LIST_UNORDERED, NULL);
  nlistSetSize (ddlookup, count);

  idx = 0;
  if (uigenre->allflag) {
    /* CONTEXT: genre: a filter: all genres are displayed in the song selection */
    disp = _("All Genres");
    ilistSetNum (ddlist, idx, DD_LIST_KEY_NUM, DD_NO_SELECTION);
    ilistSetStr (ddlist, idx, DD_LIST_DISP, disp);
    nlistSetNum (ddlookup, DD_NO_SELECTION, idx);
    ++idx;
  }

  slistStartIterator (genrelist, &iteridx);
  while ((disp = slistIterateKey (genrelist, &iteridx)) != NULL) {
    gkey = slistGetNum (genrelist, disp);
    ilistSetNum (ddlist, idx, DD_LIST_KEY_NUM, gkey);
    ilistSetStr (ddlist, idx, DD_LIST_DISP, disp);
    nlistSetNum (ddlookup, gkey, idx);
    ++idx;
  }

  nlistSort (ddlookup);

  uigenre->ddlist = ddlist;
  uigenre->ddlookup = ddlookup;
}

