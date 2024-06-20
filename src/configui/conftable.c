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
confuiMakeItemTable (confuigui_t *gui, uiwcont_t *boxp, confuiident_t id,
    int flags)
{
  uiwcont_t     *mhbox = NULL;
  uiwcont_t     *bvbox = NULL;
  uiwcont_t     *scwindow = NULL;
  uiwcont_t     *uiwidgetp = NULL;
  uivirtlist_t  *uivl = NULL;

  logProcBegin ();

  if (id == CONFUI_ID_DANCE) {
    uivl = uiCreateVirtList (boxp, 5, VL_NO_HEADING, 100);
    gui->tables [id].uivl = uivl;
  } else {
    mhbox = uiCreateHorizBox ();
    uiWidgetSetMarginTop (mhbox, 2);
    uiBoxPackStart (boxp, mhbox);

    scwindow = uiCreateScrolledWindow (300);
    uiWidgetExpandVert (scwindow);
    uiBoxPackStartExpand (mhbox, scwindow);

    gui->tables [id].uitree = uiCreateTreeView ();
    gui->tables [id].flags = flags;

    uiWidgetSetMarginStart (gui->tables [id].uitree, 8);
    uiTreeViewEnableHeaders (gui->tables [id].uitree);
    uiWindowPackInWindow (scwindow, gui->tables [id].uitree);

    uiwcontFree (scwindow);

    bvbox = uiCreateVertBox ();
    uiWidgetSetAllMargins (bvbox, 4);
    uiWidgetSetMarginTop (bvbox, 32);
    uiWidgetAlignVertStart (bvbox);
    uiBoxPackStart (mhbox, bvbox);
  }

  if ((flags & CONFUI_TABLE_NO_UP_DOWN) != CONFUI_TABLE_NO_UP_DOWN) {
    gui->tables [id].callbacks [CONFUI_TABLE_CB_UP] = callbackInit (
        confuiTableMoveUp, gui, NULL);
    uiwidgetp = uiCreateButton (
        gui->tables [id].callbacks [CONFUI_TABLE_CB_UP],
        /* CONTEXT: configuration: table edit: button: move selection up */
        _("Move Up"), "button_up");
    uiBoxPackStart (bvbox, uiwidgetp);
    gui->tables [id].buttons [CONFUI_BUTTON_TABLE_UP] = uiwidgetp;

    gui->tables [id].callbacks [CONFUI_TABLE_CB_DOWN] = callbackInit (
        confuiTableMoveDown, gui, NULL);
    uiwidgetp = uiCreateButton (
        gui->tables [id].callbacks [CONFUI_TABLE_CB_DOWN],
        /* CONTEXT: configuration: table edit: button: move selection down */
        _("Move Down"), "button_down");
    uiBoxPackStart (bvbox, uiwidgetp);
    gui->tables [id].buttons [CONFUI_BUTTON_TABLE_DOWN] = uiwidgetp;
  }

  gui->tables [id].callbacks [CONFUI_TABLE_CB_REMOVE] = callbackInit (
      confuiTableRemove, gui, NULL);
  uiwidgetp = uiCreateButton (
      gui->tables [id].callbacks [CONFUI_TABLE_CB_REMOVE],
      /* CONTEXT: configuration: table edit: button: delete selection */
      _("Delete"), "button_remove");
  uiBoxPackStart (bvbox, uiwidgetp);
  gui->tables [id].buttons [CONFUI_BUTTON_TABLE_DELETE] = uiwidgetp;

  gui->tables [id].callbacks [CONFUI_TABLE_CB_ADD] = callbackInit (
      confuiTableAdd, gui, NULL);
  uiwidgetp = uiCreateButton (
      gui->tables [id].callbacks [CONFUI_TABLE_CB_ADD],
      /* CONTEXT: configuration: table edit: button: add new selection */
      _("Add New"), "button_add");
  uiBoxPackStart (bvbox, uiwidgetp);
  gui->tables [id].buttons [CONFUI_BUTTON_TABLE_ADD] = uiwidgetp;

  uiwcontFree (mhbox);
  uiwcontFree (bvbox);

  logProcEnd ("");
}

void
confuiTableFree (confuigui_t *gui, confuiident_t id)
{
  for (int i = 0; i < CONFUI_TABLE_CB_MAX; ++i) {
    callbackFree (gui->tables [id].callbacks [i]);
    gui->tables [id].callbacks [i] = NULL;
  }
  for (int i = 0; i < CONFUI_BUTTON_TABLE_MAX; ++i) {
    uiwcontFree (gui->tables [id].buttons [i]);
    gui->tables [id].buttons [i] = NULL;
  }
  uiwcontFree (gui->tables [id].uitree);
  gui->tables [id].uitree = NULL;
}

void
confuiTableSave (confuigui_t *gui, confuiident_t id)
{
  savefunc_t    savefunc;
  char          tbuff [40];

  logProcBegin ();
  if (gui->tables [id].changed == false) {
    logProcEnd ("not-changed");
    return;
  }
  if (gui->tables [id].savefunc == NULL) {
    logProcEnd ("no-savefunc");
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
  logProcEnd ("");
}

bool
confuiTableChanged (void *udata, int32_t col)
{
  confuigui_t   *gui = udata;

  gui->tables [gui->tablecurr].changed = true;
  return UICB_CONT;
}

bool
confuiSwitchTable (void *udata, int32_t pagenum)
{
  confuigui_t       *gui = udata;
  uiwcont_t         *uitree;
  confuiident_t     newid;

  logProcBegin ();
  if ((newid = (confuiident_t) uinbutilIDGet (gui->nbtabid, pagenum)) < 0) {
    logProcEnd ("bad-pagenum");
    return UICB_STOP;
  }

  if (gui->tablecurr == newid) {
    logProcEnd ("same-id");
    return UICB_CONT;
  }

  confuiSetStatusMsg (gui, "");

  gui->tablecurr = (confuiident_t) uinbutilIDGet (
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
    /* be sure to create the target first */
    confuiCreateTagSelectedDisp (gui);
    confuiCreateTagListingDisp (gui);
  }

  if (gui->tablecurr >= CONFUI_ID_TABLE_MAX) {
    logProcEnd ("non-table");
    return UICB_CONT;
  }

  uitree = gui->tables [gui->tablecurr].uitree;
  if (uitree == NULL) {
    logProcEnd ("no-tree-a");
    return UICB_CONT;
  }

  uiTreeViewSelectDefault (uitree);

  logProcEnd ("");
  return UICB_CONT;
}

/* internal routines */

/* table editing */

static bool
confuiTableMoveUp (void *udata)
{
  logProcBegin ();
  confuiTableMove (udata, CONFUI_MOVE_PREV);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
confuiTableMoveDown (void *udata)
{
  logProcBegin ();
  confuiTableMove (udata, CONFUI_MOVE_NEXT);
  logProcEnd ("");
  return UICB_CONT;
}

static void
confuiTableMove (confuigui_t *gui, int dir)
{
  uiwcont_t         *uitree;
  int               count;
  int               idx;
  int               flags;

  logProcBegin ();
  flags = gui->tables [gui->tablecurr].flags;

  uitree = gui->tables [gui->tablecurr].uitree;
  count = uiTreeViewSelectGetCount (uitree);
  if (count != 1) {
    logProcEnd ("no-selection");
    return;
  }

  idx = uiTreeViewSelectGetIndex (uitree);

  if (idx == 1 &&
      dir == CONFUI_MOVE_PREV &&
      (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
    logProcEnd ("move-prev-keep-first");
    return;
  }
  if (idx == gui->tables [gui->tablecurr].currcount - 1 &&
      dir == CONFUI_MOVE_PREV &&
      (flags & CONFUI_TABLE_KEEP_LAST) == CONFUI_TABLE_KEEP_LAST) {
    logProcEnd ("move-prev-keep-last");
    return;
  }
  if (idx == gui->tables [gui->tablecurr].currcount - 2 &&
      dir == CONFUI_MOVE_NEXT &&
      (flags & CONFUI_TABLE_KEEP_LAST) == CONFUI_TABLE_KEEP_LAST) {
    logProcEnd ("move-next-keep-last");
    return;
  }
  if (idx == 0 &&
      dir == CONFUI_MOVE_NEXT &&
      (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
    logProcEnd ("move-next-keep-first");
    return;
  }

  if (dir == CONFUI_MOVE_PREV) {
    uiTreeViewMoveBefore (uitree);
  } else {
    uiTreeViewMoveAfter (uitree);
  }
  gui->tables [gui->tablecurr].changed = true;
  logProcEnd ("");
}

static bool
confuiTableRemove (void *udata)
{
  confuigui_t       *gui = udata;
  uiwcont_t         *uitree;
  int               idx;
  int               count;
  int               flags;

  logProcBegin ();

  uitree = gui->tables [gui->tablecurr].uitree;
  if (uitree == NULL) {
    return UICB_STOP;
  }

  flags = gui->tables [gui->tablecurr].flags;
  count = uiTreeViewSelectGetCount (uitree);
  if (count != 1) {
    logProcEnd ("no-selection");
    return UICB_STOP;
  }

  idx = uiTreeViewSelectGetIndex (uitree);
  if (idx == 0 &&
      (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
    logProcEnd ("keep-first");
    return UICB_CONT;
  }
  if (idx == gui->tables [gui->tablecurr].currcount - 1 &&
      (flags & CONFUI_TABLE_KEEP_LAST) == CONFUI_TABLE_KEEP_LAST) {
    logProcEnd ("keep-last");
    return UICB_CONT;
  }

  if (gui->tablecurr == CONFUI_ID_DANCE) {
//    listidx_t     dkey;
//    dance_t       *dances;

//    dkey = uiTreeViewGetValue (uitree, CONFUI_DANCE_COL_DANCE_IDX);
//    dances = bdjvarsdfGet (BDJVDF_DANCES);
//    danceDelete (dances, dkey);
  }

  uiTreeViewValueRemove (uitree);
  gui->tables [gui->tablecurr].changed = true;
  gui->tables [gui->tablecurr].currcount -= 1;

  if (gui->tablecurr == CONFUI_ID_DANCE) {
    confuiDanceSelect (gui, 0);
  }

  logProcEnd ("");
  return UICB_CONT;
}
