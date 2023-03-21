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
#include "ilist.h"
#include "level.h"
#include "log.h"
#include "mdebug.h"
#include "ui.h"

static bool confuiLevelListCreate (void *udata);
static void confuiLevelSave (confuigui_t *gui);

void
confuiBuildUIEditLevels (confuigui_t *gui)
{
  uiwcont_t    vbox;
  uiwcont_t    hbox;
  uiwcont_t    uiwidget;

  logProcBegin (LOG_PROC, "confuiBuildUIEditLevels");
  uiCreateVertBox (&vbox);

  /* edit levels */
  confuiMakeNotebookTab (&vbox, gui,
      /* CONTEXT: configuration: edit dance levels table */
      _("Edit Levels"), CONFUI_ID_LEVELS);

  /* CONTEXT: configuration: dance levels: instructions */
  uiCreateLabel (&uiwidget, _("Order from easiest to most advanced."));
  uiBoxPackStart (&vbox, &uiwidget);

  /* CONTEXT: configuration: dance levels: information on how to edit a level entry */
  uiCreateLabel (&uiwidget, _("Double click on a field to edit."));
  uiBoxPackStart (&vbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiBoxPackStartExpand (&vbox, &hbox);

  confuiMakeItemTable (gui, &hbox, CONFUI_ID_LEVELS, CONFUI_TABLE_NONE);
  gui->tables [CONFUI_ID_LEVELS].listcreatefunc = confuiLevelListCreate;
  gui->tables [CONFUI_ID_LEVELS].savefunc = confuiLevelSave;
  confuiCreateLevelTable (gui);
  logProcEnd (LOG_PROC, "confuiBuildUIEditLevels", "");
}

void
confuiCreateLevelTable (confuigui_t *gui)
{
  ilistidx_t        iteridx;
  ilistidx_t        key;
  level_t           *levels;
  uitree_t          *uitree;

  logProcBegin (LOG_PROC, "confuiCreateLevelTable");

  levels = bdjvarsdfGet (BDJVDF_LEVELS);

  uitree = gui->tables [CONFUI_ID_LEVELS].uitree;

  gui->tables [CONFUI_ID_LEVELS].callbacks [CONFUI_TABLE_CB_CHANGED] =
      callbackInitLong (confuiTableChanged, gui);
  uiTreeViewSetEditedCallback (uitree,
      gui->tables [CONFUI_ID_LEVELS].callbacks [CONFUI_TABLE_CB_CHANGED]);

  uiTreeViewCreateValueStore (uitree, CONFUI_LEVEL_COL_MAX,
      TREE_TYPE_INT,          // editable
      TREE_TYPE_STRING,       // display
      TREE_TYPE_NUM,          // weight
      TREE_TYPE_BOOLEAN,      // default
      TREE_TYPE_WIDGET,
      TREE_TYPE_INT,          // digits
      TREE_TYPE_END);

  levelStartIterator (levels, &iteridx);

  while ((key = levelIterate (levels, &iteridx)) >= 0) {
    char  *leveldisp;
    long  weight;
    long  def;
    bool  deffound = false;

    leveldisp = levelGetLevel (levels, key);
    weight = levelGetWeight (levels, key);
    def = levelGetDefault (levels, key);
    if (def && deffound) {
      def = 0;
    }
    if (def && ! deffound) {
      uiTreeViewRadioSetRow (uitree, key);
      deffound = true;
    }

    uiTreeViewValueAppend (uitree);
    confuiLevelSet (uitree, TRUE, leveldisp, weight, def);
    /* all cells other than the very first (Unrated) are editable */
    gui->tables [CONFUI_ID_LEVELS].currcount += 1;
  }

  uiTreeViewAppendColumn (uitree, TREE_NO_COLUMN,
      TREE_WIDGET_TEXT, TREE_ALIGN_NORM,
      TREE_COL_DISP_GROW, tagdefs [TAG_DANCELEVEL].shortdisplayname,
      TREE_COL_TYPE_TEXT, CONFUI_LEVEL_COL_LEVEL,
      TREE_COL_TYPE_EDITABLE, CONFUI_LEVEL_COL_EDITABLE,
      TREE_COL_TYPE_END);

  uiTreeViewAppendColumn (uitree, TREE_NO_COLUMN,
      TREE_WIDGET_SPINBOX, TREE_ALIGN_RIGHT,
      /* CONTEXT: configuration: level: title of the weight column */
      TREE_COL_DISP_GROW, _("Weight"),
      TREE_COL_TYPE_TEXT, CONFUI_LEVEL_COL_WEIGHT,
      TREE_COL_TYPE_EDITABLE, CONFUI_LEVEL_COL_EDITABLE,
      TREE_COL_TYPE_ADJUSTMENT, CONFUI_LEVEL_COL_ADJUST,
      TREE_COL_TYPE_DIGITS, CONFUI_LEVEL_COL_DIGITS,
      TREE_COL_TYPE_END);

  uiTreeViewAppendColumn (uitree, TREE_NO_COLUMN,
      TREE_WIDGET_RADIO, TREE_ALIGN_CENTER,
      /* CONTEXT: configuration: level: title of the default selection column */
      TREE_COL_DISP_GROW, _("Default"),
      TREE_COL_TYPE_ACTIVE, CONFUI_LEVEL_COL_DEFAULT,
      TREE_COL_TYPE_END);

  logProcEnd (LOG_PROC, "confuiCreateLevelTable", "");
}

static bool
confuiLevelListCreate (void *udata)
{
  confuigui_t *gui = udata;
  char        *leveldisp;
  int         weight;
  int         def;

  logProcBegin (LOG_PROC, "confuiLevelListCreate");
  leveldisp = uiTreeViewGetValueStr (gui->tables [CONFUI_ID_LEVELS].uitree,
      CONFUI_LEVEL_COL_LEVEL);
  weight = uiTreeViewGetValue (gui->tables [CONFUI_ID_LEVELS].uitree,
      CONFUI_LEVEL_COL_WEIGHT);
  def = uiTreeViewGetValue (gui->tables [CONFUI_ID_LEVELS].uitree,
      CONFUI_LEVEL_COL_DEFAULT);
  ilistSetStr (gui->tables [CONFUI_ID_LEVELS].savelist,
      gui->tables [CONFUI_ID_LEVELS].saveidx, LEVEL_LEVEL, leveldisp);
  ilistSetNum (gui->tables [CONFUI_ID_LEVELS].savelist,
      gui->tables [CONFUI_ID_LEVELS].saveidx, LEVEL_WEIGHT, weight);
  ilistSetNum (gui->tables [CONFUI_ID_LEVELS].savelist,
      gui->tables [CONFUI_ID_LEVELS].saveidx, LEVEL_DEFAULT_FLAG, def);
  gui->tables [CONFUI_ID_LEVELS].saveidx += 1;
  dataFree (leveldisp);
  logProcEnd (LOG_PROC, "confuiLevelListCreate", "");
  return FALSE;
}

static void
confuiLevelSave (confuigui_t *gui)
{
  level_t    *levels;

  logProcBegin (LOG_PROC, "confuiLevelSave");
  levels = bdjvarsdfGet (BDJVDF_LEVELS);
  levelSave (levels, gui->tables [CONFUI_ID_LEVELS].savelist);
  ilistFree (gui->tables [CONFUI_ID_LEVELS].savelist);
  logProcEnd (LOG_PROC, "confuiLevelSave", "");
}

