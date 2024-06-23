/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
/* the conversion routines use the internal level-list, but it */
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
#include "level.h"
#include "log.h"
#include "mdebug.h"
#include "ui.h"

static void confuiLevelSave (confuigui_t *gui);
static void confuiLevelFillRow (void *udata, uivirtlist_t *vl, int32_t rownum);
static bool confuiLevelChangeCB (void *udata);
static int confuiLevelEntryChangeCB (uiwcont_t *entry, const char *label, void *udata);
static void confuiLevelAdd (confuigui_t *gui);
static void confuiLevelRemove (confuigui_t *gui, ilistidx_t idx);
static void confuiLevelUpdateData (confuigui_t *gui);
static void confuiLevelMove (confuigui_t *gui, ilistidx_t idx, int dir);

void
confuiBuildUIEditLevels (confuigui_t *gui)
{
  uiwcont_t    *vbox;
  uiwcont_t    *hbox;
  uiwcont_t    *uiwidgetp;

  logProcBegin ();
  vbox = uiCreateVertBox ();

  /* edit levels */
  confuiMakeNotebookTab (vbox, gui,
      /* CONTEXT: configuration: edit the dance levels table */
      _("Edit Levels"), CONFUI_ID_LEVELS);

  /* CONTEXT: configuration: dance levels: instructions */
  uiwidgetp = uiCreateLabel (_("Order from easiest to most advanced."));
  uiBoxPackStart (vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  hbox = uiCreateHorizBox ();
  uiWidgetAlignHorizStart (hbox);
  uiBoxPackStartExpand (vbox, hbox);

  confuiMakeItemTable (gui, hbox, CONFUI_ID_LEVELS, CONFUI_TABLE_NONE);
  gui->tables [CONFUI_ID_LEVELS].savefunc = confuiLevelSave;
  gui->tables [CONFUI_ID_LEVELS].addfunc = confuiLevelAdd;
  gui->tables [CONFUI_ID_LEVELS].removefunc = confuiLevelRemove;
  gui->tables [CONFUI_ID_LEVELS].movefunc = confuiLevelMove;
  confuiCreateLevelTable (gui);

  uiwcontFree (vbox);
  uiwcontFree (hbox);

  logProcEnd ("");
}

void
confuiCreateLevelTable (confuigui_t *gui)
{
  level_t           *levels;
  uivirtlist_t      *uivl;

  logProcBegin ();

  levels = bdjvarsdfGet (BDJVDF_LEVELS);

  uivl = gui->tables [CONFUI_ID_LEVELS].uivl;
  uivlSetNumColumns (uivl, CONFUI_LEVEL_COL_MAX);
  uivlMakeColumnEntry (uivl, CONFUI_LEVEL_COL_LEVEL, 15, 30);
  uivlMakeColumnSpinboxNum (uivl, CONFUI_LEVEL_COL_WEIGHT, 0.0, 100.0, 1.0, 5.0);
  uivlMakeColumn (uivl, CONFUI_LEVEL_COL_DEFAULT, VL_TYPE_RADIO_BUTTON);
  uivlSetColumnHeading (uivl, CONFUI_LEVEL_COL_LEVEL,
      tagdefs [TAG_DANCELEVEL].shortdisplayname);
  /* CONTEXT: configuration: level: title of the weight column */
  uivlSetColumnHeading (uivl, CONFUI_LEVEL_COL_WEIGHT, _("Weight"));
  /* CONTEXT: configuration: level: title of the default selection column */
  uivlSetColumnHeading (uivl, CONFUI_LEVEL_COL_DEFAULT, _("Default"));
  uivlSetNumRows (uivl, levelGetCount (levels));
  gui->tables [CONFUI_ID_LEVELS].currcount = levelGetCount (levels);

  gui->tables [CONFUI_ID_LEVELS].callbacks [CONFUI_LEVEL_CB] =
      callbackInit (confuiLevelChangeCB, gui, NULL);
  uivlSetEntryValidation (uivl, CONFUI_LEVEL_COL_LEVEL,
      confuiLevelEntryChangeCB, gui);
  uivlSetSpinboxChangeCallback (uivl, CONFUI_LEVEL_COL_WEIGHT,
      gui->tables [CONFUI_ID_LEVELS].callbacks [CONFUI_LEVEL_CB]);
  uivlSetRadioChangeCallback (uivl, CONFUI_LEVEL_COL_DEFAULT,
      gui->tables [CONFUI_ID_LEVELS].callbacks [CONFUI_LEVEL_CB]);

  uivlSetRowFillCallback (uivl, confuiLevelFillRow, gui);
  uivlDisplay (uivl);

  logProcEnd ("");
}

/* internal routines */

static void
confuiLevelSave (confuigui_t *gui)
{
  level_t       *levels;
  ilist_t       *levellist;
  uivirtlist_t  *uivl;
  ilistidx_t    count;

  logProcBegin ();

  if (gui->tables [CONFUI_ID_LEVELS].changed == false) {
    return;
  }

  levels = bdjvarsdfGet (BDJVDF_LEVELS);

  uivl = gui->tables [CONFUI_ID_LEVELS].uivl;
  levellist = ilistAlloc ("level-save", LIST_ORDERED);
  count = levelGetCount (levels);
  confuiLevelUpdateData (gui);
  for (int rowidx = 0; rowidx < count; ++rowidx) {
    const char  *leveldisp;
    int         weight;
    int         defflag;

    leveldisp = levelGetLevel (levels, rowidx);
    weight = levelGetWeight (levels, rowidx);
    defflag = levelGetDefault (levels, rowidx);
    ilistSetStr (levellist, rowidx, LEVEL_LEVEL, leveldisp);
    ilistSetNum (levellist, rowidx, LEVEL_WEIGHT, weight);
    ilistSetNum (levellist, rowidx, LEVEL_DEFAULT_FLAG, defflag);
  }

  levelSave (levels, levellist);
  ilistFree (levellist);
  logProcEnd ("");
}

static void
confuiLevelFillRow (void *udata, uivirtlist_t *vl, int32_t rownum)
{
  confuigui_t *gui = udata;
  level_t     *levels;
  const char  *leveldisp;
  int         weight;
  int         defflag;

  gui->inchange = true;

  levels = bdjvarsdfGet (BDJVDF_LEVELS);
  if (rownum >= levelGetCount (levels)) {
    return;
  }

  leveldisp = levelGetLevel (levels, rownum);
  weight = levelGetWeight (levels, rownum);
  defflag = levelGetDefault (levels, rownum);
  uivlSetRowColumnValue (gui->tables [CONFUI_ID_LEVELS].uivl, rownum,
      CONFUI_LEVEL_COL_LEVEL, leveldisp);
  uivlSetRowColumnNum (gui->tables [CONFUI_ID_LEVELS].uivl, rownum,
      CONFUI_LEVEL_COL_WEIGHT, weight);
  uivlSetRowColumnNum (gui->tables [CONFUI_ID_LEVELS].uivl, rownum,
      CONFUI_LEVEL_COL_DEFAULT, defflag);

  gui->inchange = false;
}

static bool
confuiLevelChangeCB (void *udata)
{
  confuigui_t   *gui = udata;

  if (gui->inchange) {
    return UICB_CONT;
  }

  /* must update the data here in case of a scroll */
  confuiLevelUpdateData (gui);

  gui->tables [CONFUI_ID_LEVELS].changed = true;
  return UICB_CONT;
}

static int
confuiLevelEntryChangeCB (uiwcont_t *entry, const char *label, void *udata)
{
  confuigui_t *gui = udata;

  if (gui->inchange) {
    return UIENTRY_OK;
  }

  /* must update the data here in case of a scroll */
  confuiLevelUpdateData (gui);

  gui->tables [CONFUI_ID_LEVELS].changed = true;
  return UIENTRY_OK;
}

static void
confuiLevelAdd (confuigui_t *gui)
{
  level_t       *levels;
  int           count;
  uivirtlist_t  *uivl;

  levels = bdjvarsdfGet (BDJVDF_LEVELS);
  uivl = gui->tables [CONFUI_ID_LEVELS].uivl;

  confuiLevelUpdateData (gui);
  count = levelGetCount (levels);
  /* CONTEXT: configuration: level name that is set when adding a new level */
  levelSetLevel (levels, count, _("New Level"));
  levelSetWeight (levels, count, 0);
  /* this will reset the default-flag value for all items */
  levelSetDefault (levels, levelGetDefaultKey (levels));
  count += 1;
  uivlSetNumRows (uivl, count);
  gui->tables [CONFUI_ID_LEVELS].currcount = count;
  uivlPopulate (uivl);
  uivlSetSelection (uivl, count - 1);
}

static void
confuiLevelRemove (confuigui_t *gui, ilistidx_t delidx)
{
  level_t       *levels;
  uivirtlist_t  *uivl;
  int           count;
  bool          docopy = false;
  ilistidx_t    defaultkey;


  levels = bdjvarsdfGet (BDJVDF_LEVELS);
  uivl = gui->tables [CONFUI_ID_LEVELS].uivl;
  /* update with any changes */
  confuiLevelUpdateData (gui);
  count = levelGetCount (levels);
  defaultkey = levelGetDefaultKey (levels);

  for (int idx = 0; idx < count - 1; ++idx) {
    if (idx == delidx) {
      docopy = true;
    }
    if (docopy) {
      const char  *leveldisp;
      int         weight;
      int         defflag;

      leveldisp = levelGetLevel (levels, idx + 1);
      weight = levelGetWeight (levels, idx + 1);
      defflag = levelGetDefault (levels, idx + 1);
      levelSetLevel (levels, idx, leveldisp);
      levelSetWeight (levels, idx, weight);
      if (defflag) {
        levelSetDefault (levels, idx);
      }
    }
  }
  if (defaultkey == delidx) {
    levelSetDefault (levels, 0);
  }

  levelDeleteLast (levels);
  count = levelGetCount (levels);
  uivlSetNumRows (uivl, count);
  gui->tables [CONFUI_ID_LEVELS].currcount = count;
  uivlPopulate (uivl);
}

static void
confuiLevelUpdateData (confuigui_t *gui)
{
  level_t       *levels;
  uivirtlist_t  *uivl;
  ilistidx_t    count;
  int32_t       rowiter;
  int32_t       rownum;

  levels = bdjvarsdfGet (BDJVDF_LEVELS);

  uivl = gui->tables [CONFUI_ID_LEVELS].uivl;
  count = levelGetCount (levels);

  uivlStartRowDispIterator (uivl, &rowiter);
  while ((rownum = uivlIterateRowDisp (uivl, &rowiter)) >= 0) {
    const char  *leveldisp;
    int         weight;
    int         defflag;

    leveldisp = uivlGetRowColumnEntry (uivl, rownum, CONFUI_LEVEL_COL_LEVEL);
    weight = uivlGetRowColumnNum (uivl, rownum, CONFUI_LEVEL_COL_WEIGHT);
    defflag = uivlGetRowColumnNum (uivl, rownum, CONFUI_LEVEL_COL_DEFAULT);
    levelSetLevel (levels, rownum, leveldisp);
    levelSetWeight (levels, rownum, weight);
    if (defflag) {
      levelSetDefault (levels, rownum);
    }
  }
}

static void
confuiLevelMove (confuigui_t *gui, ilistidx_t idx, int dir)
{
  level_t       *levels;
  uivirtlist_t  *uivl;
  ilistidx_t    toidx;
  char          *leveldisp;
  int           weight;
  int           defflag = false;

  levels = bdjvarsdfGet (BDJVDF_LEVELS);
  uivl = gui->tables [CONFUI_ID_LEVELS].uivl;
  confuiLevelUpdateData (gui);

  toidx = idx;
  if (dir == CONFUI_MOVE_PREV) {
    toidx -= 1;
    uivlMoveSelection (uivl, VL_DIR_PREV);
  }
  if (dir == CONFUI_MOVE_NEXT) {
    toidx += 1;
    uivlMoveSelection (uivl, VL_DIR_NEXT);
  }

  leveldisp = mdstrdup (levelGetLevel (levels, toidx));
  weight = levelGetWeight (levels, toidx);
  defflag = levelGetDefault (levels, toidx);
  levelSetLevel (levels, toidx, levelGetLevel (levels, idx));
  levelSetWeight (levels, toidx, levelGetWeight (levels, idx));
  if (levelGetDefault (levels, idx)) {
    levelSetDefault (levels, toidx);
  }
  levelSetLevel (levels, idx, leveldisp);
  levelSetWeight (levels, idx, weight);
  if (defflag) {
    levelSetDefault (levels, idx);
  }
  dataFree (leveldisp);

  uivlPopulate (uivl);

  return;
}
