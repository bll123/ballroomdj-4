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
static void confuiGenreUpdateData (confuigui_t *gui);
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
  uivlMakeColumnEntry (uivl, CONFUI_GENRE_COL_GENRE, 15, 30);
  uivlMakeColumn (uivl, CONFUI_GENRE_COL_CLASSICAL, VL_TYPE_CHECK_BUTTON);
  uivlSetColumnHeading (uivl, CONFUI_GENRE_COL_GENRE,
      tagdefs [TAG_GENRE].displayname);
  /* CONTEXT: configuration: genre: title of the classical setting column */
  uivlSetColumnHeading (uivl, CONFUI_GENRE_COL_CLASSICAL, _("Classical?"));
  uivlSetNumRows (uivl, genreGetCount (genres));
  gui->tables [CONFUI_ID_GENRES].currcount = genreGetCount (genres);

  gui->tables [CONFUI_ID_GENRES].callbacks [CONFUI_GENRE_CB] =
      callbackInit (confuiGenreChangeCB, gui, NULL);
  uivlSetEntryValidation (uivl, CONFUI_GENRE_COL_GENRE,
      confuiGenreEntryChangeCB, gui);
  uivlSetCheckBoxChangeCallback (uivl, CONFUI_GENRE_COL_CLASSICAL,
      gui->tables [CONFUI_ID_GENRES].callbacks [CONFUI_GENRE_CB]);

  uivlSetRowFillCallback (uivl, confuiGenreFillRow, gui);
  uivlDisplay (uivl);
  /* the first entry field is read-only */
  uivlSetRowColumnReadonly (uivl, 0, CONFUI_GENRE_COL_GENRE);

  logProcEnd ("");
}

/* internal routines */

static void
confuiGenreSave (confuigui_t *gui)
{
  genre_t       *genres;
  ilist_t       *genrelist;
  uivirtlist_t  *uivl;
  ilistidx_t    count;

  logProcBegin ();

  if (gui->tables [CONFUI_ID_GENRES].changed == false) {
    return;
  }

  genres = bdjvarsdfGet (BDJVDF_GENRES);

  uivl = gui->tables [CONFUI_ID_GENRES].uivl;
  genrelist = ilistAlloc ("genre-save", LIST_ORDERED);
  count = genreGetCount (genres);
  confuiGenreUpdateData (gui);
  for (int rowidx = 0; rowidx < count; ++rowidx) {
    const char  *genredisp;
    int         playflag;

    genredisp = uivlGetRowColumnEntry (uivl, rowidx, CONFUI_GENRE_COL_GENRE);
    playflag = uivlGetRowColumnNum (uivl, rowidx, CONFUI_GENRE_COL_CLASSICAL);
    ilistSetStr (genrelist, rowidx, GENRE_GENRE, genredisp);
    ilistSetNum (genrelist, rowidx, GENRE_CLASSICAL_FLAG, playflag);
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
  int         playflag;

  gui->inchange = true;

  genres = bdjvarsdfGet (BDJVDF_GENRES);
  if (rownum >= genreGetCount (genres)) {
    return;
  }

  genredisp = genreGetGenre (genres, rownum);
  playflag = genreGetClassicalFlag (genres, rownum);
  uivlSetRowColumnValue (gui->tables [CONFUI_ID_GENRES].uivl, rownum,
      CONFUI_GENRE_COL_GENRE, genredisp);
  uivlSetRowColumnNum (gui->tables [CONFUI_ID_GENRES].uivl, rownum,
      CONFUI_GENRE_COL_CLASSICAL, playflag);

  gui->inchange = false;
}

static bool
confuiGenreChangeCB (void *udata)
{
  confuigui_t *gui = udata;

  if (gui->inchange) {
    return UICB_CONT;
  }

  gui->tables [CONFUI_ID_GENRES].changed = true;
  return UICB_CONT;
}

static int
confuiGenreEntryChangeCB (uiwcont_t *entry, const char *label, void *udata)
{
  confuigui_t *gui = udata;

  if (gui->inchange) {
    return UIENTRY_OK;
  }

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

  confuiGenreUpdateData (gui);
  count = genreGetCount (genres);
  /* CONTEXT: configuration: genre name that is set when adding a new genre */
  genreSetGenre (genres, count, _("New Genre"));
  genreSetClassicalFlag (genres, count, 0);
  count += 1;
  uivlSetNumRows (uivl, count);
  gui->tables [CONFUI_ID_GENRES].currcount = count;
  uivlPopulate (uivl);
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
  /* update with any changes */
  confuiGenreUpdateData (gui);
  count = genreGetCount (genres);

  for (int idx = 0; idx < count - 1; ++idx) {
    if (idx == delidx) {
      docopy = true;
    }
    if (docopy) {
      const char  *genredisp;
      int         playflag;

      genredisp = uivlGetRowColumnEntry (uivl, idx + 1, CONFUI_GENRE_COL_GENRE);
      playflag = uivlGetRowColumnNum (uivl, idx + 1, CONFUI_GENRE_COL_CLASSICAL);
      genreSetGenre (genres, idx, genredisp);
      genreSetClassicalFlag (genres, idx, playflag);
    }
  }

  genreDeleteLast (genres);
  count = genreGetCount (genres);
  uivlSetNumRows (uivl, count);
  gui->tables [CONFUI_ID_GENRES].currcount = count;
  uivlPopulate (uivl);
}

static void
confuiGenreUpdateData (confuigui_t *gui)
{
  genre_t       *genres;
  uivirtlist_t  *uivl;
  ilistidx_t    count;

  genres = bdjvarsdfGet (BDJVDF_GENRES);

  uivl = gui->tables [CONFUI_ID_GENRES].uivl;
  count = genreGetCount (genres);

  for (int rowidx = 0; rowidx < count; ++rowidx) {
    const char  *genredisp;
    int         playflag;

    genredisp = uivlGetRowColumnEntry (uivl, rowidx, CONFUI_GENRE_COL_GENRE);
    playflag = uivlGetRowColumnNum (uivl, rowidx, CONFUI_GENRE_COL_CLASSICAL);
    genreSetGenre (genres, rowidx, genredisp);
    genreSetClassicalFlag (genres, rowidx, playflag);
  }
}

static void
confuiGenreMove (confuigui_t *gui, ilistidx_t idx, int dir)
{
  genre_t       *genres;
  uivirtlist_t  *uivl;
  ilistidx_t    toidx;
  const char    *genredisp;
  int           playflag;

  genres = bdjvarsdfGet (BDJVDF_GENRES);
  uivl = gui->tables [CONFUI_ID_GENRES].uivl;
  confuiGenreUpdateData (gui);

  toidx = idx;
  if (dir == CONFUI_MOVE_PREV) {
    toidx -= 1;
    uivlMoveSelection (uivl, VL_DIR_PREV);
  }
  if (dir == CONFUI_MOVE_NEXT) {
    toidx += 1;
    uivlMoveSelection (uivl, VL_DIR_NEXT);
  }

  genredisp = uivlGetRowColumnEntry (uivl, toidx, CONFUI_GENRE_COL_GENRE);
  playflag = uivlGetRowColumnNum (uivl, toidx, CONFUI_GENRE_COL_CLASSICAL);
  genreSetGenre (genres, toidx,
      uivlGetRowColumnEntry (uivl, idx, CONFUI_GENRE_COL_GENRE));
  genreSetClassicalFlag (genres, toidx,
      uivlGetRowColumnNum (uivl, idx, CONFUI_GENRE_COL_CLASSICAL));
  genreSetGenre (genres, idx, genredisp);
  genreSetClassicalFlag (genres, idx, playflag);

  uivlPopulate (uivl);

  return;
}
