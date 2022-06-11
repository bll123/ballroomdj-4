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
#include "ui.h"
#include "uigenre.h"
#include "uiutils.h"

typedef struct uigenre {
  genre_t       *genres;
  uidropdown_t  *dropdown;
  UIWidget      *parentwin;
  UIWidget      *buttonp;
  UICallback    cb;
  long          selectedidx;
  bool          allflag : 1;
} uigenre_t;

static bool uigenreSelectHandler (void *udata, long idx);
static void uigenreCreateGenreList (uigenre_t *uigenre);

uigenre_t *
uigenreDropDownCreate (UIWidget *boxp, UIWidget *parentwin, bool allflag)
{
  uigenre_t  *uigenre;


  uigenre = malloc (sizeof (uigenre_t));
  uigenre->genres = bdjvarsdfGet (BDJVDF_GENRES);
  uigenre->allflag = allflag;
  uigenre->dropdown = uiDropDownInit ();
  uigenre->selectedidx = 0;
  uiutilsUICallbackLongInit (&uigenre->cb, uigenreSelectHandler, uigenre);
  uigenre->buttonp = uiComboboxCreate (uigenre->parentwin, "",
      &uigenre->cb, uigenre->dropdown, uigenre);
  uigenreCreateGenreList (uigenre);
  uiBoxPackStart (boxp, uigenre->buttonp);

  return uigenre;
}

UIWidget *
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
    uiDropDownFree (uigenre->dropdown);
    free (uigenre);
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
uigenreDisable (uigenre_t *uigenre)
{
  if (uigenre == NULL || uigenre->dropdown == NULL) {
    return;
  }
  uiDropDownDisable (uigenre->dropdown);
}

void
uigenreEnable (uigenre_t *uigenre)
{
  if (uigenre == NULL || uigenre->dropdown == NULL) {
    return;
  }
  uiDropDownEnable (uigenre->dropdown);
}

void
uigenreSizeGroupAdd (uigenre_t *uigenre, UIWidget *sg)
{
  uiSizeGroupAdd (sg, uigenre->buttonp);
}

/* internal routines */

static bool
uigenreSelectHandler (void *udata, long idx)
{
  uigenre_t   *uigenre = udata;

  uigenre->selectedidx = idx;
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
      /* CONTEXT: a filter: all genres are displayed in the song selection */
      dispptr = _("All Genres");
  }
  uiDropDownSetNumList (uigenre->dropdown, genrelist, dispptr);
}

