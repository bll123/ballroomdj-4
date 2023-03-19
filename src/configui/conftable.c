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
#include "bdjopt.h"
#include "bdjvarsdf.h"
#include "callback.h"
#include "configui.h"
#include "dance.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "ui.h"

/* table editing */
static bool   confuiTableMoveUp (void *udata);
static bool   confuiTableMoveDown (void *udata);
static void   confuiTableMove (confuigui_t *gui, int dir);
static bool   confuiTableRemove (void *udata);

void
confuiMakeItemTable (confuigui_t *gui, uiwidget_t *boxp, confuiident_t id,
    int flags)
{
  uiwidget_t  mhbox;
  uiwidget_t  bvbox;
  uiwidget_t  scwindow;
  uibutton_t  *uibutton;
  uiwidget_t  *uiwidgetp;

  logProcBegin (LOG_PROC, "confuiMakeItemTable");

  uiCreateHorizBox (&mhbox);
  uiWidgetSetMarginTop (&mhbox, 2);
  uiBoxPackStart (boxp, &mhbox);

  uiCreateScrolledWindow (&scwindow, 300);
  uiWidgetExpandVert (&scwindow);
  uiBoxPackStartExpand (&mhbox, &scwindow);

  gui->tables [id].uitree = uiCreateTreeView ();
  uiwidgetp = uiTreeViewGetUIWidget (gui->tables [id].uitree);
  gui->tables [id].flags = flags;

  uiWidgetSetMarginStart (uiwidgetp, 8);
  uiTreeViewEnableHeaders (gui->tables [id].uitree);
  uiBoxPackInWindow (&scwindow, uiwidgetp);

  uiCreateVertBox (&bvbox);
  uiWidgetSetAllMargins (&bvbox, 4);
  uiWidgetSetMarginTop (&bvbox, 32);
  uiWidgetAlignVertStart (&bvbox);
  uiBoxPackStart (&mhbox, &bvbox);

  if ((flags & CONFUI_TABLE_NO_UP_DOWN) != CONFUI_TABLE_NO_UP_DOWN) {
    gui->tables [id].callbacks [CONFUI_TABLE_CB_UP] = callbackInit (
        confuiTableMoveUp, gui, NULL);
    uibutton = uiCreateButton (
        gui->tables [id].callbacks [CONFUI_TABLE_CB_UP],
        /* CONTEXT: configuration: table edit: button: move selection up */
        _("Move Up"), "button_up");
    gui->tables [id].buttons [CONFUI_BUTTON_TABLE_UP] = uibutton;
    uiwidgetp = uiButtonGetUIWidget (uibutton);
    uiBoxPackStart (&bvbox, uiwidgetp);

    gui->tables [id].callbacks [CONFUI_TABLE_CB_DOWN] = callbackInit (
        confuiTableMoveDown, gui, NULL);
    uibutton = uiCreateButton (
        gui->tables [id].callbacks [CONFUI_TABLE_CB_DOWN],
        /* CONTEXT: configuration: table edit: button: move selection down */
        _("Move Down"), "button_down");
    gui->tables [id].buttons [CONFUI_BUTTON_TABLE_DOWN] = uibutton;
    uiwidgetp = uiButtonGetUIWidget (uibutton);
    uiBoxPackStart (&bvbox, uiwidgetp);
  }

  gui->tables [id].callbacks [CONFUI_TABLE_CB_REMOVE] = callbackInit (
      confuiTableRemove, gui, NULL);
  uibutton = uiCreateButton (
      gui->tables [id].callbacks [CONFUI_TABLE_CB_REMOVE],
      /* CONTEXT: configuration: table edit: button: delete selection */
      _("Delete"), "button_remove");
  gui->tables [id].buttons [CONFUI_BUTTON_TABLE_DELETE] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackStart (&bvbox, uiwidgetp);

  gui->tables [id].callbacks [CONFUI_TABLE_CB_ADD] = callbackInit (
      confuiTableAdd, gui, NULL);
  uibutton = uiCreateButton (
      gui->tables [id].callbacks [CONFUI_TABLE_CB_ADD],
      /* CONTEXT: configuration: table edit: button: add new selection */
      _("Add New"), "button_add");
  gui->tables [id].buttons [CONFUI_BUTTON_TABLE_ADD] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackStart (&bvbox, uiwidgetp);

  logProcEnd (LOG_PROC, "confuiMakeItemTable", "");
}

void
confuiTableFree (confuigui_t *gui, confuiident_t id)
{
  for (int i = 0; i < CONFUI_TABLE_CB_MAX; ++i) {
    callbackFree (gui->tables [id].callbacks [i]);
    gui->tables [id].callbacks [i] = NULL;
  }
  for (int i = 0; i < CONFUI_BUTTON_TABLE_MAX; ++i) {
    uiButtonFree (gui->tables [id].buttons [i]);
    gui->tables [id].buttons [i] = NULL;
  }
  uiTreeViewFree (gui->tables [id].uitree);
  gui->tables [id].uitree = NULL;
}

void
confuiTableSave (confuigui_t *gui, confuiident_t id)
{
  savefunc_t    savefunc;
  char          tbuff [40];

  logProcBegin (LOG_PROC, "confuiTableSave");
  if (gui->tables [id].changed == false) {
    logProcEnd (LOG_PROC, "confuiTableSave", "not-changed");
    return;
  }
  if (gui->tables [id].savefunc == NULL) {
    logProcEnd (LOG_PROC, "confuiTableSave", "no-savefunc");
    return;
  }

  if (gui->tables [id].listcreatefunc != NULL) {
    callback_t    *cb;

    snprintf (tbuff, sizeof (tbuff), "cu-table-save-%d", id);
    gui->tables [id].savelist = ilistAlloc (tbuff, LIST_ORDERED);
    gui->tables [id].saveidx = 0;
    cb = callbackInit (gui->tables [id].listcreatefunc, gui, NULL);
    uiTreeViewForeach (gui->tables [id].uitree, cb);
    callbackFree (cb);
  }

  savefunc = gui->tables [id].savefunc;
  savefunc (gui);
  logProcEnd (LOG_PROC, "confuiTableSave", "");
}

bool
confuiTableChanged (void *udata, long col)
{
  confuigui_t   *gui = udata;

  gui->tables [gui->tablecurr].changed = true;
  return UICB_CONT;
}

bool
confuiSwitchTable (void *udata, long pagenum)
{
  confuigui_t       *gui = udata;
  uitree_t          *uitree;
  confuiident_t     newid;

  logProcBegin (LOG_PROC, "confuiSwitchTable");
  if ((newid = (confuiident_t) uiutilsNotebookIDGet (gui->nbtabid, pagenum)) < 0) {
    logProcEnd (LOG_PROC, "confuiSwitchTable", "bad-pagenum");
    return UICB_STOP;
  }

  if (gui->tablecurr == newid) {
    logProcEnd (LOG_PROC, "confuiSwitchTable", "same-id");
    return UICB_CONT;
  }

  confuiSetStatusMsg (gui, "");

  gui->tablecurr = (confuiident_t) uiutilsNotebookIDGet (
      gui->nbtabid, pagenum);

  if (gui->tablecurr == CONFUI_ID_MOBILE_MQ) {
    confuiUpdateMobmqQrcode (gui);
  }
  if (gui->tablecurr == CONFUI_ID_REM_CONTROL) {
    confuiUpdateRemctrlQrcode (gui);
  }
  if (gui->tablecurr == CONFUI_ID_ORGANIZATION) {
    confuiUpdateOrgExamples (gui, bdjoptGetStr (OPT_G_ORGPATH));
  }
  if (gui->tablecurr == CONFUI_ID_DISP_SEL_LIST) {
    /* be sure to create the listing first */
    confuiCreateTagListingDisp (gui);
    confuiCreateTagSelectedDisp (gui);
  }

  if (gui->tablecurr >= CONFUI_ID_TABLE_MAX) {
    logProcEnd (LOG_PROC, "confuiSwitchTable", "non-table");
    return UICB_CONT;
  }

  uitree = gui->tables [gui->tablecurr].uitree;
  if (uitree == NULL) {
    logProcEnd (LOG_PROC, "confuiSwitchTable", "no-tree-a");
    return UICB_CONT;
  }

  uiTreeViewSelectDefault (uitree);

  logProcEnd (LOG_PROC, "confuiSwitchTable", "");
  return UICB_CONT;
}

/* internal routines */

/* table editing */

static bool
confuiTableMoveUp (void *udata)
{
  logProcBegin (LOG_PROC, "confuiTableMoveUp");
  confuiTableMove (udata, CONFUI_MOVE_PREV);
  logProcEnd (LOG_PROC, "confuiTableMoveUp", "");
  return UICB_CONT;
}

static bool
confuiTableMoveDown (void *udata)
{
  logProcBegin (LOG_PROC, "confuiTableMoveDown");
  confuiTableMove (udata, CONFUI_MOVE_NEXT);
  logProcEnd (LOG_PROC, "confuiTableMoveDown", "");
  return UICB_CONT;
}

static void
confuiTableMove (confuigui_t *gui, int dir)
{
  uitree_t          *uitree;
  int               count;
  int               idx;
  int               flags;

  logProcBegin (LOG_PROC, "confuiTableMove");
  flags = gui->tables [gui->tablecurr].flags;

  uitree = gui->tables [gui->tablecurr].uitree;
  count = uiTreeViewSelectGetCount (uitree);
  if (count != 1) {
    logProcEnd (LOG_PROC, "confuiTableMove", "no-selection");
    return;
  }

  idx = uiTreeViewSelectGetIndex (uitree);

  if (idx == 1 &&
      dir == CONFUI_MOVE_PREV &&
      (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
    logProcEnd (LOG_PROC, "confuiTableMove", "move-prev-keep-first");
    return;
  }
  if (idx == gui->tables [gui->tablecurr].currcount - 1 &&
      dir == CONFUI_MOVE_PREV &&
      (flags & CONFUI_TABLE_KEEP_LAST) == CONFUI_TABLE_KEEP_LAST) {
    logProcEnd (LOG_PROC, "confuiTableMove", "move-prev-keep-last");
    return;
  }
  if (idx == gui->tables [gui->tablecurr].currcount - 2 &&
      dir == CONFUI_MOVE_NEXT &&
      (flags & CONFUI_TABLE_KEEP_LAST) == CONFUI_TABLE_KEEP_LAST) {
    logProcEnd (LOG_PROC, "confuiTableMove", "move-next-keep-last");
    return;
  }
  if (idx == 0 &&
      dir == CONFUI_MOVE_NEXT &&
      (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
    logProcEnd (LOG_PROC, "confuiTableMove", "move-next-keep-first");
    return;
  }

  if (dir == CONFUI_MOVE_PREV) {
    uiTreeViewMoveBefore (uitree);
  } else {
    uiTreeViewMoveAfter (uitree);
  }
  gui->tables [gui->tablecurr].changed = true;
  logProcEnd (LOG_PROC, "confuiTableMove", "");
}

static bool
confuiTableRemove (void *udata)
{
  confuigui_t       *gui = udata;
  uitree_t          *uitree;
  int               idx;
  int               count;
  int               flags;

  logProcBegin (LOG_PROC, "confuiTableRemove");

  uitree = gui->tables [gui->tablecurr].uitree;
  if (uitree == NULL) {
    return UICB_STOP;
  }

  flags = gui->tables [gui->tablecurr].flags;
  count = uiTreeViewSelectGetCount (uitree);
  if (count != 1) {
    logProcEnd (LOG_PROC, "confuiTableRemove", "no-selection");
    return UICB_STOP;
  }

  idx = uiTreeViewSelectGetIndex (uitree);
  if (idx == 0 &&
      (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
    logProcEnd (LOG_PROC, "confuiTableRemove", "keep-first");
    return UICB_CONT;
  }
  if (idx == gui->tables [gui->tablecurr].currcount - 1 &&
      (flags & CONFUI_TABLE_KEEP_LAST) == CONFUI_TABLE_KEEP_LAST) {
    logProcEnd (LOG_PROC, "confuiTableRemove", "keep-last");
    return UICB_CONT;
  }

  if (gui->tablecurr == CONFUI_ID_DANCE) {
    listidx_t     dkey;
    dance_t       *dances;

    dkey = uiTreeViewGetValue (uitree, CONFUI_DANCE_COL_DANCE_IDX);
    dances = bdjvarsdfGet (BDJVDF_DANCES);
    danceDelete (dances, dkey);
  }

  uiTreeViewValueRemove (uitree);
  gui->tables [gui->tablecurr].changed = true;
  gui->tables [gui->tablecurr].currcount -= 1;

  if (gui->tablecurr == CONFUI_ID_DANCE) {
    confuiDanceSelect (gui);
  }

  logProcEnd (LOG_PROC, "confuiTableRemove", "");
  return UICB_CONT;
}
