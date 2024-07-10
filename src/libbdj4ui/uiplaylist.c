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
#include "callback.h"
#include "ilist.h"
#include "mdebug.h"
#include "playlist.h"
#include "slist.h"
#include "ui.h"
#include "uidd.h"
#include "uiplaylist.h"

typedef struct uiplaylist {
  uidd_t            *uidd;
  callback_t        *internalselcb;
  callback_t        *selectcb;
  ilist_t           *ddlist;
  slist_t           *ddlookup;
  bool              blankflag : 1;
} uiplaylist_t;

static int32_t uiplaylistSelectHandler (void *udata, const char *key);

uiplaylist_t *
uiplaylistCreate (uiwcont_t *parentwin, uiwcont_t *hbox, int type,
    const char *label, int where, int flag)
{
  uiplaylist_t  *uiplaylist;
  int           titleflag = DD_REPLACE_TITLE;
  int           dwhere = DD_PACK_START;

  uiplaylist = mdmalloc (sizeof (uiplaylist_t));
  uiplaylist->uidd = NULL;
  uiplaylist->internalselcb = NULL;
  uiplaylist->selectcb = NULL;
  uiplaylist->ddlist = NULL;
  uiplaylist->ddlookup = NULL;
  uiplaylist->blankflag = false;
  if (flag == UIPL_USE_BLANK) {
    uiplaylist->blankflag = true;
  }

  uiplaylistSetList (uiplaylist, type, NULL);
  uiplaylist->internalselcb =
      callbackInitS (uiplaylistSelectHandler, uiplaylist);
  if (label != NULL) {
    titleflag = DD_KEEP_TITLE;
  }
  if (label == NULL) {
    label = "";
  }
  if (where == UIPL_PACK_START) {
    dwhere = DD_PACK_START;
  }
  if (where == UIPL_PACK_END) {
    dwhere = DD_PACK_END;
  }
  uiplaylist->uidd = uiddCreate ("uipl",
      parentwin, hbox, dwhere,
      uiplaylist->ddlist, DD_LIST_TYPE_STR,
      label, titleflag, uiplaylist->internalselcb);

  return uiplaylist;
}

void
uiplaylistFree (uiplaylist_t *uiplaylist)
{
  if (uiplaylist == NULL) {
    return;
  }

  callbackFree (uiplaylist->internalselcb);
  uiddFree (uiplaylist->uidd);
  ilistFree (uiplaylist->ddlist);
  slistFree (uiplaylist->ddlookup);
  mdfree (uiplaylist);
}

void
uiplaylistSetList (uiplaylist_t *uiplaylist, int type, const char *dir)
{
  slist_t     *pllist;
  slistidx_t  iteridx;
  ilist_t     *ddlist;
  slist_t     *ddlookup;
  const char  *disp;
  const char  *plkey;
  int         count;
  int         idx;

  ilistFree (uiplaylist->ddlist);
  slistFree (uiplaylist->ddlookup);

  pllist = playlistGetPlaylistList (type, dir);
  count = slistGetCount (pllist);
  ddlist = ilistAlloc ("uipl", LIST_ORDERED);
  ilistSetSize (ddlist, count);
  ddlookup = slistAlloc ("uipl-lookup", LIST_UNORDERED, NULL);
  slistSetSize (ddlookup, count);

  idx = 0;
  if (uiplaylist->blankflag) {
    /* the blank entry is used for the song filter dialog */
    ilistSetStr (ddlist, idx, DD_LIST_KEY_STR, "");
    ilistSetStr (ddlist, idx, DD_LIST_DISP, "");
    slistSetNum (ddlookup, "", idx);
    ++idx;
  }

  slistStartIterator (pllist, &iteridx);
  while ((disp = slistIterateKey (pllist, &iteridx)) != NULL) {
    plkey = slistGetStr (pllist, disp);
    ilistSetStr (ddlist, idx, DD_LIST_KEY_STR, plkey);
    ilistSetStr (ddlist, idx, DD_LIST_DISP, disp);
    slistSetNum (ddlookup, plkey, idx);
    ++idx;
  }

  slistSort (ddlookup);

  uiplaylist->ddlist = ddlist;
  uiplaylist->ddlookup = ddlookup;

  uiddSetList (uiplaylist->uidd, uiplaylist->ddlist);

  slistFree (pllist);
}

static int32_t
uiplaylistSelectHandler (void *udata, const char *str)
{
  uiplaylist_t  *uiplaylist = udata;

  if (uiplaylist->selectcb != NULL) {
    callbackHandlerS (uiplaylist->selectcb, str);
  }

  return 0;
}

const char *
uiplaylistGetKey (uiplaylist_t *uiplaylist)
{
  const char    *fn;

  if (uiplaylist == NULL) {
    return NULL;
  }

  fn = uiddGetSelectionStr (uiplaylist->uidd);
  return fn;
}

void
uiplaylistSetKey (uiplaylist_t *uiplaylist, const char *fn)
{
  slistidx_t      idx;

  if (uiplaylist == NULL) {
    return;
  }

  idx = slistGetNum (uiplaylist->ddlookup, fn);
  if (idx >= 0) {
    uiddSetSelection (uiplaylist->uidd, idx);
  }
}


void
uiplaylistSetSelectCallback (uiplaylist_t *uiplaylist, callback_t *cb)
{
  if (uiplaylist == NULL) {
    return;
  }
  uiplaylist->selectcb = cb;
}
