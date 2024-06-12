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
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "tagdef.h"
#include "ui.h"

static bool confuiGenreListCreate (void *udata);
static void confuiGenreSave (confuigui_t *gui);

void
confuiBuildUIEditGenres (confuigui_t *gui)
{
  uiwcont_t    *vbox;
  uiwcont_t    *hbox;
  uiwcont_t    *uiwidgetp;

  logProcBegin ();
  vbox = uiCreateVertBox ();

  /* edit genres */
  confuiMakeNotebookTab (vbox, gui,
      /* CONTEXT: configuration: edit genres table */
      _("Edit Genres"), CONFUI_ID_GENRES);

  /* CONTEXT: configuration: genres: information on how to edit a genre entry */
  uiwidgetp = uiCreateLabel (_("Double click on a field to edit."));
  uiBoxPackStart (vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  hbox = uiCreateHorizBox ();
  uiBoxPackStartExpand (vbox, hbox);

  confuiMakeItemTable (gui, hbox, CONFUI_ID_GENRES, CONFUI_TABLE_NONE);
  gui->tables [CONFUI_ID_GENRES].listcreatefunc = confuiGenreListCreate;
  gui->tables [CONFUI_ID_GENRES].savefunc = confuiGenreSave;
  confuiCreateGenreTable (gui);
  uiwcontFree (hbox);
  uiwcontFree (vbox);
  logProcEnd ("");
}

void
confuiCreateGenreTable (confuigui_t *gui)
{
  ilistidx_t        iteridx;
  ilistidx_t        key;
  genre_t           *genres;
  uiwcont_t         *uitree;

  logProcBegin ();

  genres = bdjvarsdfGet (BDJVDF_GENRES);

  uitree = gui->tables [CONFUI_ID_GENRES].uitree;

  gui->tables [CONFUI_ID_GENRES].callbacks [CONFUI_TABLE_CB_CHANGED] =
      callbackInitI (confuiTableChanged, gui);
  uiTreeViewSetEditedCallback (uitree,
      gui->tables [CONFUI_ID_GENRES].callbacks [CONFUI_TABLE_CB_CHANGED]);

  uiTreeViewCreateValueStore (uitree, CONFUI_GENRE_COL_MAX,
      TREE_TYPE_INT,        // editable
      TREE_TYPE_STRING,     // display
      TREE_TYPE_BOOLEAN,    // classical flag
      TREE_TYPE_STRING,     // scrollbar pad
      TREE_TYPE_END);

  genreStartIterator (genres, &iteridx);

  while ((key = genreIterate (genres, &iteridx)) >= 0) {
    const char  *genredisp;
    long        clflag;

    genredisp = genreGetGenre (genres, key);
    clflag = genreGetClassicalFlag (genres, key);

    uiTreeViewValueAppend (uitree);
    confuiGenreSet (uitree, true, genredisp, clflag);
    gui->tables [CONFUI_ID_GENRES].currcount += 1;
  }

  uiTreeViewAppendColumn (uitree, TREE_NO_COLUMN,
      TREE_WIDGET_TEXT, TREE_ALIGN_NORM,
      TREE_COL_DISP_GROW, tagdefs [TAG_GENRE].displayname,
      TREE_COL_TYPE_TEXT, CONFUI_GENRE_COL_GENRE,
      TREE_COL_TYPE_EDITABLE, CONFUI_GENRE_COL_EDITABLE,
      TREE_COL_TYPE_END);

  uiTreeViewAppendColumn (uitree, TREE_NO_COLUMN,
      TREE_WIDGET_CHECKBOX, TREE_ALIGN_CENTER,
      /* CONTEXT: configuration: genre: title of the classical setting column */
      TREE_COL_DISP_GROW, _("Classical?"),
      TREE_COL_TYPE_ACTIVE, CONFUI_GENRE_COL_CLASSICAL,
      TREE_COL_TYPE_END);

  uiTreeViewAppendColumn (uitree, TREE_NO_COLUMN,
      TREE_WIDGET_TEXT, TREE_ALIGN_NORM,
      TREE_COL_DISP_GROW, NULL,
      TREE_COL_TYPE_TEXT, CONFUI_GENRE_COL_SB_PAD,
      TREE_COL_TYPE_END);

  logProcEnd ("");
}


static bool
confuiGenreListCreate (void *udata)
{
  confuigui_t *gui = udata;
  char        *genredisp;
  int         clflag;

  logProcBegin ();
  genredisp = uiTreeViewGetValueStr (gui->tables [CONFUI_ID_GENRES].uitree,
      CONFUI_GENRE_COL_GENRE);
  clflag = uiTreeViewGetValue (gui->tables [CONFUI_ID_GENRES].uitree,
      CONFUI_GENRE_COL_CLASSICAL);
  ilistSetStr (gui->tables [CONFUI_ID_GENRES].savelist,
      gui->tables [CONFUI_ID_GENRES].saveidx, GENRE_GENRE, genredisp);
  ilistSetNum (gui->tables [CONFUI_ID_GENRES].savelist,
      gui->tables [CONFUI_ID_GENRES].saveidx, GENRE_CLASSICAL_FLAG, clflag);
  gui->tables [CONFUI_ID_GENRES].saveidx += 1;
  dataFree (genredisp);
  logProcEnd ("");
  return UI_FOREACH_CONT;
}

static void
confuiGenreSave (confuigui_t *gui)
{
  genre_t    *genres;

  logProcBegin ();
  genres = bdjvarsdfGet (BDJVDF_GENRES);
  genreSave (genres, gui->tables [CONFUI_ID_GENRES].savelist);
  ilistFree (gui->tables [CONFUI_ID_GENRES].savelist);
  logProcEnd ("");
}
