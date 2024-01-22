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
#include "mdebug.h"
#include "playlist.h"
#include "slist.h"
#include "ui.h"
#include "uiplaylist.h"

enum {
  UIPLAYLIST_CB_SEL,
  UIPLAYLIST_CB_MAX,
};

typedef struct uiplaylist {
  uiwcont_t         *dropdown;
  uiwcont_t         *uiwidgetp;
  callback_t        *callbacks [UIPLAYLIST_CB_MAX];
  callback_t        *selectcb;
} uiplaylist_t;

static bool     uiplaylistSelectHandler (void *udata, long idx);

uiplaylist_t *
uiplaylistCreate (uiwcont_t *parentwin, uiwcont_t *hbox, int type)
{
  uiplaylist_t    *uiplaylist;
  uiwcont_t       *uiwidgetp;

  uiplaylist = mdmalloc (sizeof (uiplaylist_t));
  uiplaylist->dropdown = NULL;
  for (int i = 0; i < UIPLAYLIST_CB_MAX; ++i) {
    uiplaylist->callbacks [i] = NULL;
  }
  uiplaylist->selectcb = NULL;

  uiplaylist->dropdown = uiDropDownInit ();
  uiplaylist->callbacks [UIPLAYLIST_CB_SEL] =
      callbackInitLong (uiplaylistSelectHandler, uiplaylist);
  uiwidgetp = uiComboboxCreate (uiplaylist->dropdown,
      parentwin, "",
      uiplaylist->callbacks [UIPLAYLIST_CB_SEL], uiplaylist);
  uiplaylistSetList (uiplaylist, type, NULL);
  uiBoxPackStart (hbox, uiwidgetp);

  uiplaylist->uiwidgetp = uiwidgetp;
  return uiplaylist;
}

void
uiplaylistFree (uiplaylist_t *uiplaylist)
{
  if (uiplaylist == NULL) {
    return;
  }

  for (int i = 0; i < UIPLAYLIST_CB_MAX; ++i) {
    callbackFree (uiplaylist->callbacks [i]);
  }
  uiwcontFree (uiplaylist->dropdown);
  mdfree (uiplaylist);
}

void
uiplaylistSetList (uiplaylist_t *uiplaylist, int type, const char *dir)
{
  slist_t           *pllist;

  pllist = playlistGetPlaylistList (type, dir);
  /* what text is best to use for 'no selection'? */
  uiDropDownSetList (uiplaylist->dropdown, pllist, "");
  slistFree (pllist);
}

static bool
uiplaylistSelectHandler (void *udata, long idx)
{
  uiplaylist_t  *uiplaylist = udata;

  if (uiplaylist->selectcb != NULL) {
    callbackHandlerLong (uiplaylist->selectcb, idx);
  }

  return UICB_CONT;
}

const char *
uiplaylistGetValue (uiplaylist_t *uiplaylist)
{
  const char    *fn;

  if (uiplaylist == NULL) {
    return NULL;
  }

  fn = uiDropDownGetString (uiplaylist->dropdown);
  return fn;
}

void
uiplaylistSetValue (uiplaylist_t *uiplaylist, const char *fn)
{
  if (uiplaylist == NULL) {
    return;
  }

  uiDropDownSelectionSetStr (uiplaylist->dropdown, fn);
}

void
uiplaylistSizeGroupAdd (uiplaylist_t *uiplaylist, uiwcont_t *sg)
{
  if (uiplaylist == NULL) {
    return;
  }
  uiSizeGroupAdd (sg, uiplaylist->uiwidgetp);
}

void
uiplaylistSetSelectCallback (uiplaylist_t *uiplaylist, callback_t *cb)
{
  if (uiplaylist == NULL) {
    return;
  }
  uiplaylist->selectcb = cb;
}
