/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "genre.h"
#include "mdebug.h"
#include "ui.h"
#include "callback.h"
#include "uigenre.h"

typedef struct uigenre {
  genre_t       *genres;
  uidropdown_t  *dropdown;
  uiwcont_t    *parentwin;
  uiwcont_t    *buttonp;
  callback_t    *cb;
  callback_t    *selectcb;
  long          selectedidx;
  bool          allflag : 1;
} uigenre_t;

static bool uigenreSelectHandler (void *udata, long idx);
static void uigenreCreateGenreList (uigenre_t *uigenre);

uigenre_t *
uigenreDropDownCreate (uiwcont_t *boxp, uiwcont_t *parentwin, bool allflag)
{
  uigenre_t  *uigenre;


  uigenre = mdmalloc (sizeof (uigenre_t));
  uigenre->genres = bdjvarsdfGet (BDJVDF_GENRES);
  uigenre->allflag = allflag;
  uigenre->dropdown = uiDropDownInit ();
  uigenre->selectedidx = 0;
  uigenre->parentwin = parentwin;
  uigenre->cb = NULL;
  uigenre->selectcb = NULL;

  uigenre->cb = callbackInitLong (uigenreSelectHandler, uigenre);
  uigenre->buttonp = uiComboboxCreate (uigenre->dropdown,
      parentwin, "", uigenre->cb, uigenre);
  uigenreCreateGenreList (uigenre);
  uiBoxPackStart (boxp, uigenre->buttonp);

  return uigenre;
}

uiwcont_t *
uigenreGetButton (uigenre_t *uigenre)
{
  if (uigenre == NULL) {
    return NULL;
  }
  return uigenre->buttonp;
}


void
uigenreFree (uigenre_t *uigenre)
{
  if (uigenre != NULL) {
    callbackFree (uigenre->cb);
    uiDropDownFree (uigenre->dropdown);
    mdfree (uigenre);
  }
}

int
uigenreGetValue (uigenre_t *uigenre)
{
  if (uigenre == NULL) {
    return 0;
  }

  return uigenre->selectedidx;
}

void
uigenreSetValue (uigenre_t *uigenre, int value)
{
  if (uigenre == NULL || uigenre->dropdown == NULL) {
    return;
  }

  uigenre->selectedidx = value;
  uiDropDownSelectionSetNum (uigenre->dropdown, value);
}

void
uigenreSetState (uigenre_t *uigenre, int state)
{
  if (uigenre == NULL || uigenre->dropdown == NULL) {
    return;
  }
  uiDropDownSetState (uigenre->dropdown, state);
}

void
uigenreSizeGroupAdd (uigenre_t *uigenre, uiwcont_t *sg)
{
  uiSizeGroupAdd (sg, uigenre->buttonp);
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
uigenreSelectHandler (void *udata, long idx)
{
  uigenre_t   *uigenre = udata;

  uigenre->selectedidx = idx;

  if (uigenre->selectcb != NULL) {
    callbackHandlerLong (uigenre->selectcb, idx);
  }
  return UICB_CONT;
}

static void
uigenreCreateGenreList (uigenre_t *uigenre)
{
  slist_t   *genrelist;
  char      *dispptr;

  genrelist = genreGetList (uigenre->genres);
  dispptr = NULL;
  if (uigenre->allflag) {
      /* CONTEXT: genre: a filter: all genres are displayed in the song selection */
      dispptr = _("All Genres");
  }
  uiDropDownSetNumList (uigenre->dropdown, genrelist, dispptr);
}

