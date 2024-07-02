/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
/* the conversion routines use the internal genre-list, but it */
/* is not necessary to update this for editing or for the save */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>
#include <stdarg.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjvarsdf.h"
#include "configui.h"
#include "genre.h"
#include "log.h"
#include "mdebug.h"
#include "ui.h"

static void confuiGenreSave (confuigui_t *gui);
static void confuiGenreFillRow (void *udata, uivirtlist_t *vl, int32_t rownum);
static bool confuiGenreChangeCB (void *udata);
static int confuiGenreEntryChangeCB (uiwcont_t *entry, const char *label, void *udata);
static void confuiGenreAdd (confuigui_t *gui);
static void confuiGenreRemove (confuigui_t *gui, ilistidx_t idx);
static void confuiGenreMove (confuigui_t *gui, ilistidx_t idx, int dir);

void
confuiBuildUIEditGenres (confuigui_t *gui)
{
  uiwcont_t    *vbox;
  uiwcont_t    *hbox;

  logProcBegin ();
  vbox = uiCreateVertBox ();

  /* edit genre */
  confuiMakeNotebookTab (vbox, gui,
      /* CONTEXT: configuration: edit genres table */
      _("Edit Genres"), CONFUI_ID_GENRES);

  hbox = uiCreateHorizBox ();
  uiWidgetAlignHorizStart (hbox);
  uiBoxPackStartExpand (vbox, hbox);

  confuiMakeItemTable (gui, hbox, CONFUI_ID_GENRES, CONFUI_TABLE_NONE);
  gui->tables [CONFUI_ID_GENRES].savefunc = confuiGenreSave;
  gui->tables [CONFUI_ID_GENRES].addfunc = confuiGenreAdd;
  gui->tables [CONFUI_ID_GENRES].removefunc = confuiGenreRemove;
  gui->tables [CONFUI_ID_GENRES].movefunc = confuiGenreMove;
  confuiCreateGenreTable (gui);

  uiwcontFree (vbox);
  uiwcontFree (hbox);

  logProcEnd ("");
}

void
confuiCreateGenreTable (confuigui_t *gui)
{
  genre_t           *genres;
  uivirtlist_t      *uivl;

  logProcBegin ();

  genres = bdjvarsdfGet (BDJVDF_GENRES);

  uivl = gui->tables [CONFUI_ID_GENRES].uivl;
  uivlSetNumColumns (uivl, CONFUI_GENRE_COL_MAX);

  uivlMakeColumnEntry (uivl, "genre", CONFUI_GENRE_COL_GENRE, 15, 30);
  uivlSetColumnHeading (uivl, CONFUI_GENRE_COL_GENRE,
      tagdefs [TAG_GENRE].displayname);

  uivlMakeColumn (uivl, "clflag", CONFUI_GENRE_COL_CLASSICAL, VL_TYPE_CHECKBOX);
  /* CONTEXT: configuration: genre: title of the classical setting column */
  uivlSetColumnHeading (uivl, CONFUI_GENRE_COL_CLASSICAL, _("Classical?"));
  uivlSetColumnAlignCenter (uivl, CONFUI_GENRE_COL_CLASSICAL);

  uivlSetNumRows (uivl, genreGetCount (genres));
  gui->tables [CONFUI_ID_GENRES].currcount = genreGetCount (genres);

  gui->tables [CONFUI_ID_GENRES].callbacks [CONFUI_GENRE_CB] =
      callbackInit (confuiGenreChangeCB, gui, NULL);
  uivlSetEntryValidation (uivl, CONFUI_GENRE_COL_GENRE,
      confuiGenreEntryChangeCB, gui);
  uivlSetCheckboxChangeCallback (uivl, CONFUI_GENRE_COL_CLASSICAL,
      gui->tables [CONFUI_ID_GENRES].callbacks [CONFUI_GENRE_CB]);

  uivlSetRowFillCallback (uivl, confuiGenreFillRow, gui);
  uivlDisplay (uivl);

  logProcEnd ("");
}

/* internal routines */

static void
confuiGenreSave (confuigui_t *gui)
{
  genre_t       *genres;
  ilist_t       *genrelist;
  ilistidx_t    count;

  logProcBegin ();

  if (gui->tables [CONFUI_ID_GENRES].changed == false) {
    return;
  }

  genres = bdjvarsdfGet (BDJVDF_GENRES);

  genrelist = ilistAlloc ("genre-save", LIST_ORDERED);
  count = genreGetCount (genres);
  for (int rowidx = 0; rowidx < count; ++rowidx) {
    const char  *genredisp;
    int         clflag;

    genredisp = genreGetGenre (genres, rowidx);
    clflag = genreGetClassicalFlag (genres, rowidx);
    ilistSetStr (genrelist, rowidx, GENRE_GENRE, genredisp);
    ilistSetNum (genrelist, rowidx, GENRE_CLASSICAL_FLAG, clflag);
  }

  genreSave (genres, genrelist);
  ilistFree (genrelist);
  logProcEnd ("");
}

static void
confuiGenreFillRow (void *udata, uivirtlist_t *vl, int32_t rownum)
{
  confuigui_t *gui = udata;
  genre_t     *genres;
  const char  *genredisp;
  int         clflag;

  gui->inchange = true;

  genres = bdjvarsdfGet (BDJVDF_GENRES);
  if (rownum >= genreGetCount (genres)) {
    return;
  }

  genredisp = genreGetGenre (genres, rownum);
  clflag = genreGetClassicalFlag (genres, rownum);
  uivlSetRowColumnStr (gui->tables [CONFUI_ID_GENRES].uivl, rownum,
      CONFUI_GENRE_COL_GENRE, genredisp);
  uivlSetRowColumnNum (gui->tables [CONFUI_ID_GENRES].uivl, rownum,
      CONFUI_GENRE_COL_CLASSICAL, clflag);

  gui->inchange = false;
}

static bool
confuiGenreChangeCB (void *udata)
{
  confuigui_t   *gui = udata;
  uivirtlist_t  *uivl;
  int32_t       rownum;
  int           clflag;
  genre_t       *genres;

  if (gui->inchange) {
    return UICB_CONT;
  }

  genres = bdjvarsdfGet (BDJVDF_GENRES);
  uivl = gui->tables [CONFUI_ID_GENRES].uivl;
  rownum = uivlGetCurrSelection (uivl);
  clflag = uivlGetRowColumnNum (uivl, rownum, CONFUI_GENRE_COL_CLASSICAL);
  genreSetClassicalFlag (genres, rownum, clflag);
  gui->tables [CONFUI_ID_GENRES].changed = true;
  return UICB_CONT;
}

static int
confuiGenreEntryChangeCB (uiwcont_t *entry, const char *label, void *udata)
{
  confuigui_t   *gui = udata;
  uivirtlist_t  *uivl;
  int32_t       rownum;
  const char    *genredisp;
  genre_t       *genres;

  if (gui->inchange) {
    return UIENTRY_OK;
  }

  genres = bdjvarsdfGet (BDJVDF_GENRES);
  uivl = gui->tables [CONFUI_ID_GENRES].uivl;
  rownum = uivlGetCurrSelection (uivl);
  genredisp = uivlGetRowColumnEntry (uivl, rownum, CONFUI_GENRE_COL_GENRE);
  genreSetGenre (genres, rownum, genredisp);
  gui->tables [CONFUI_ID_GENRES].changed = true;
  return UIENTRY_OK;
}

static void
confuiGenreAdd (confuigui_t *gui)
{
  genre_t       *genres;
  int           count;
  uivirtlist_t  *uivl;

  genres = bdjvarsdfGet (BDJVDF_GENRES);
  uivl = gui->tables [CONFUI_ID_GENRES].uivl;

  count = genreGetCount (genres);
  /* CONTEXT: configuration: genre name that is set when adding a new genre */
  genreSetGenre (genres, count, _("New Genre"));
  genreSetClassicalFlag (genres, count, 0);
  count += 1;
  uivlSetNumRows (uivl, count);
  gui->tables [CONFUI_ID_GENRES].currcount = count;
  uivlPopulate (uivl);
  uivlSetSelection (uivl, count - 1);
}

static void
confuiGenreRemove (confuigui_t *gui, ilistidx_t delidx)
{
  genre_t       *genres;
  uivirtlist_t  *uivl;
  int           count;
  bool          docopy = false;

  genres = bdjvarsdfGet (BDJVDF_GENRES);
  uivl = gui->tables [CONFUI_ID_GENRES].uivl;
  count = genreGetCount (genres);

  for (int idx = 0; idx < count - 1; ++idx) {
    if (idx == delidx) {
      docopy = true;
    }
    if (docopy) {
      const char  *genredisp;
      int         clflag;

      genredisp = genreGetGenre (genres, idx + 1);
      clflag = genreGetClassicalFlag (genres, idx + 1);
      genreSetGenre (genres, idx, genredisp);
      genreSetClassicalFlag (genres, idx, clflag);
    }
  }

  genreDeleteLast (genres);
  count = genreGetCount (genres);
  uivlSetNumRows (uivl, count);
  gui->tables [CONFUI_ID_GENRES].currcount = count;
  uivlPopulate (uivl);
}

static void
confuiGenreMove (confuigui_t *gui, ilistidx_t idx, int dir)
{
  genre_t       *genres;
  uivirtlist_t  *uivl;
  ilistidx_t    toidx;
  char          *genredisp;
  int           clflag;

  genres = bdjvarsdfGet (BDJVDF_GENRES);
  uivl = gui->tables [CONFUI_ID_GENRES].uivl;

  toidx = idx;
  if (dir == CONFUI_MOVE_PREV) {
    toidx -= 1;
    uivlMoveSelection (uivl, VL_DIR_PREV);
  }
  if (dir == CONFUI_MOVE_NEXT) {
    toidx += 1;
    uivlMoveSelection (uivl, VL_DIR_NEXT);
  }

  genredisp = mdstrdup (genreGetGenre (genres, toidx));
  clflag = genreGetClassicalFlag (genres, toidx);
  genreSetGenre (genres, toidx, genreGetGenre (genres, idx));
  genreSetClassicalFlag (genres, toidx, genreGetClassicalFlag (genres, idx));
  genreSetGenre (genres, idx, genredisp);
  genreSetClassicalFlag (genres, idx, clflag);
  dataFree (genredisp);

  uivlPopulate (uivl);

  return;
}
