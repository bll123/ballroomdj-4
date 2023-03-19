/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
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
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIEditGenres");
  uiCreateVertBox (&vbox);

  /* edit genres */
  confuiMakeNotebookTab (&vbox, gui,
      /* CONTEXT: configuration: edit genres table */
      _("Edit Genres"), CONFUI_ID_GENRES);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: genres: information on how to edit a genre entry */
  uiCreateLabel (&uiwidget, _("Double click on a field to edit."));
  uiBoxPackStart (&vbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiBoxPackStartExpand (&vbox, &hbox);

  confuiMakeItemTable (gui, &hbox, CONFUI_ID_GENRES, CONFUI_TABLE_NONE);
  gui->tables [CONFUI_ID_GENRES].listcreatefunc = confuiGenreListCreate;
  gui->tables [CONFUI_ID_GENRES].savefunc = confuiGenreSave;
  confuiCreateGenreTable (gui);
  logProcEnd (LOG_PROC, "confuiBuildUIEditGenres", "");
}

void
confuiCreateGenreTable (confuigui_t *gui)
{
  ilistidx_t        iteridx;
  ilistidx_t        key;
  genre_t           *genres;
  uitree_t          *uitree;
  UIWidget          *uitreewidgetp;

  logProcBegin (LOG_PROC, "confuiCreateGenreTable");

  genres = bdjvarsdfGet (BDJVDF_GENRES);

  uitree = gui->tables [CONFUI_ID_GENRES].uitree;
  uitreewidgetp = uiTreeViewGetUIWidget (uitree);

  gui->tables [CONFUI_ID_GENRES].callbacks [CONFUI_TABLE_CB_CHANGED] =
      callbackInitLong (confuiTableChanged, gui);
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
    char    *genredisp;
    long    clflag;

    genredisp = genreGetGenre (genres, key);
    clflag = genreGetClassicalFlag (genres, key);

    uiTreeViewValueAppend (uitree);
    confuiGenreSet (uitree, TRUE, genredisp, clflag);
    gui->tables [CONFUI_ID_GENRES].currcount += 1;
  }

  uiTreeViewAppendColumn (uitree, TREE_WIDGET_TEXT,
      TREE_COL_DISP_GROW, tagdefs [TAG_GENRE].displayname,
      TREE_COL_MODE_TEXT, CONFUI_GENRE_COL_GENRE,
      TREE_COL_MODE_EDITABLE, CONFUI_GENRE_COL_EDITABLE,
      TREE_COL_MODE_END);

  uiTreeViewAppendColumn (uitree, TREE_WIDGET_CHECKBOX,
      /* CONTEXT: configuration: genre: title of the classical setting column */
      TREE_COL_DISP_NORM, _("Classical?"),
      TREE_COL_MODE_ACTIVE, CONFUI_GENRE_COL_CLASSICAL,
      TREE_COL_MODE_END);

  uiTreeViewAppendColumn (uitree, TREE_WIDGET_TEXT,
      TREE_COL_DISP_NORM, NULL,
      TREE_COL_MODE_TEXT, CONFUI_GENRE_COL_SB_PAD,
      TREE_COL_MODE_END);

  logProcEnd (LOG_PROC, "confuiCreateGenreTable", "");
}


static bool
confuiGenreListCreate (void *udata)
{
  confuigui_t *gui = udata;
  char        *genredisp;
  int         clflag;

  logProcBegin (LOG_PROC, "confuiGenreListCreate");
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
  logProcEnd (LOG_PROC, "confuiGenreListCreate", "");
  return FALSE;
}

static void
confuiGenreSave (confuigui_t *gui)
{
  genre_t    *genres;

  logProcBegin (LOG_PROC, "confuiGenreSave");
  genres = bdjvarsdfGet (BDJVDF_GENRES);
  genreSave (genres, gui->tables [CONFUI_ID_GENRES].savelist);
  ilistFree (gui->tables [CONFUI_ID_GENRES].savelist);
  logProcEnd (LOG_PROC, "confuiGenreSave", "");
}

