/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
  uiwcont_t     *vbox = NULL;
  uiwcont_t     *bvbox = NULL;
  uiwcont_t     *uiwidgetp = NULL;
  uivirtlist_t  *uivl = NULL;
  const char    *tag = "conf";
  int           vlflags = VL_FLAGS_NONE;


  switch (id) {
    case CONFUI_ID_AUDIOSRC: { tag = "conf-as"; break; }
    case CONFUI_ID_DANCE: { tag = "conf-dance"; break; }
    case CONFUI_ID_GENRES: { tag = "conf-genre"; break; }
    case CONFUI_ID_LEVELS: { tag = "conf-level"; break; }
    case CONFUI_ID_RATINGS: { tag = "conf-rating"; break; }
    case CONFUI_ID_STATUS: { tag = "conf-status"; break; }
    default: { tag = "conf"; break; }
  }

  gui->tables [id].flags = flags;

  if (id == CONFUI_ID_DANCE || id == CONFUI_ID_AUDIOSRC) {
    vlflags = VL_NO_HEADING;
  }
  vbox = uiCreateVertBox ();
  uiBoxPackStart (boxp, vbox, WCONT_FREE);
  uiWidgetAlignHorizStart (vbox);
  uiWidgetSetAllMargins (vbox, 1);
  uivl = uivlCreate (tag, gui->window, vbox, 10, 100, vlflags);
  gui->tables [id].uivl = uivl;
  uiBoxPostProcess (vbox);

  bvbox = uiCreateVertBox ();
  uiBoxPackStart (boxp, bvbox, WCONT_FREE);
  uiWidgetAlignVertStart (bvbox);
  uiWidgetSetAllMargins (bvbox, 4);
  uiWidgetSetMarginTop (bvbox, 32);

  if ((flags & CONFUI_TABLE_NO_UP_DOWN) != CONFUI_TABLE_NO_UP_DOWN) {
    gui->tables [id].callbacks [CONFUI_TABLE_CB_UP] = callbackInit (
        confuiTableMoveUp, gui, NULL);
    uiwidgetp = uiCreateButton (
        gui->tables [id].callbacks [CONFUI_TABLE_CB_UP],
        /* CONTEXT: configuration: table edit: button: move selection up */
        NULL, "button_up", _("Move Up"));
    uiBoxPackStart (bvbox, uiwidgetp, WCONT_KEEP);
    gui->tables [id].buttons [CONFUI_BUTTON_TABLE_UP] = uiwidgetp;

    gui->tables [id].callbacks [CONFUI_TABLE_CB_DOWN] = callbackInit (
        confuiTableMoveDown, gui, NULL);
    uiwidgetp = uiCreateButton (
        gui->tables [id].callbacks [CONFUI_TABLE_CB_DOWN],
        /* CONTEXT: configuration: table edit: button: move selection down */
        NULL, "button_down", _("Move Down"));
    uiBoxPackStart (bvbox, uiwidgetp, WCONT_KEEP);
    gui->tables [id].buttons [CONFUI_BUTTON_TABLE_DOWN] = uiwidgetp;
  }

  gui->tables [id].callbacks [CONFUI_TABLE_CB_REMOVE] = callbackInit (
      confuiTableRemove, gui, NULL);
  uiwidgetp = uiCreateButton (
      gui->tables [id].callbacks [CONFUI_TABLE_CB_REMOVE],
      /* CONTEXT: configuration: table edit: button: delete selection */
      NULL, "button_remove", _("Delete"));
  uiBoxPackStart (bvbox, uiwidgetp, WCONT_KEEP);
  gui->tables [id].buttons [CONFUI_BUTTON_TABLE_DELETE] = uiwidgetp;

  gui->tables [id].callbacks [CONFUI_TABLE_CB_ADD] = callbackInit (
      confuiTableAdd, gui, NULL);
  uiwidgetp = uiCreateButton (
      gui->tables [id].callbacks [CONFUI_TABLE_CB_ADD],
      /* CONTEXT: configuration: table edit: button: add new entry */
      NULL, "button_add", _("Add New"));
  uiBoxPackStart (bvbox, uiwidgetp, WCONT_KEEP);
  gui->tables [id].buttons [CONFUI_BUTTON_TABLE_ADD] = uiwidgetp;

  uiBoxPostProcess (bvbox);

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
  uivlFree (gui->tables [id].uivl);
  gui->tables [id].uivl = NULL;
}

void
confuiTableSave (confuigui_t *gui, confuiident_t id)
{
  savefunc_t    savefunc;

  if (gui->tables [id].changed == false) {
    return;
  }
  if (gui->tables [id].savefunc == NULL) {
    return;
  }

  savefunc = gui->tables [id].savefunc;
  if (savefunc != NULL) {
    savefunc (gui);
  }
}

bool
confuiSwitchTable (void *udata, int32_t pagenum)
{
  confuigui_t       *gui = udata;
  confuiident_t     newid;

  if ((newid = uivnbGetID (gui->mainvnb)) < 0) {
    return UICB_STOP;
  }

  if (gui->tablecurr == newid) {
    return UICB_CONT;
  }

  gui->reset_validation = true;
  confuiSetStatusMsg (gui, "");
  confuiSetErrorMsg (gui, "");

  gui->tablecurr = uivnbGetID (gui->mainvnb);

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
    return UICB_CONT;
  }

  return UICB_CONT;
}

bool
confuiTableAdd (void *udata)
{
  confuigui_t     *gui = udata;
  uivirtlist_t    *uivl = NULL;
  int             count = 0;
  addfunc_t       addfunc = NULL;


  if (gui->tablecurr >= CONFUI_ID_TABLE_MAX) {
    return UICB_STOP;
  }

  uivl = gui->tables [gui->tablecurr].uivl;
  if (uivl == NULL) {
    return UICB_STOP;
  }

  count = uivlSelectionCount (uivl);
  if (count != 1) {
    return UICB_STOP;
  }

  addfunc = gui->tables [gui->tablecurr].addfunc;
  if (addfunc != NULL) {
    addfunc (gui);
  }

  gui->tables [gui->tablecurr].changed = true;
  return UICB_CONT;
}

/* internal routines */

/* table editing */

static bool
confuiTableMoveUp (void *udata)
{
  confuiTableMove (udata, CONFUI_MOVE_PREV);
  return UICB_CONT;
}

static bool
confuiTableMoveDown (void *udata)
{
  confuiTableMove (udata, CONFUI_MOVE_NEXT);
  return UICB_CONT;
}

static void
confuiTableMove (confuigui_t *gui, int dir)
{
  uivirtlist_t  *uivl = NULL;
  int           count = 0;
  ilistidx_t    idx = -1;
  int           flags;
  movefunc_t    movefunc = NULL;


  flags = gui->tables [gui->tablecurr].flags;

  uivl = gui->tables [gui->tablecurr].uivl;
  if (uivl == NULL) {
    return;
  }

  count = uivlSelectionCount (uivl);
  if (count != 1) {
    return;
  }

  idx = uivlGetCurrSelection (uivl);
  if (idx < 0) {
    return;
  }

  count = gui->tables [gui->tablecurr].currcount;

  if (idx == 0 && dir == CONFUI_MOVE_PREV) {
    return;
  }
  if (idx == count - 1 && dir == CONFUI_MOVE_NEXT) {
    return;
  }
  if (idx == 1 &&
      dir == CONFUI_MOVE_PREV &&
      (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
    return;
  }
  if (idx == count - 1 &&
      dir == CONFUI_MOVE_PREV &&
      (flags & CONFUI_TABLE_KEEP_LAST) == CONFUI_TABLE_KEEP_LAST) {
    return;
  }
  if (idx == count - 2 &&
      dir == CONFUI_MOVE_NEXT &&
      (flags & CONFUI_TABLE_KEEP_LAST) == CONFUI_TABLE_KEEP_LAST) {
    return;
  }
  if (idx == 0 &&
      dir == CONFUI_MOVE_NEXT &&
      (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
    return;
  }

  movefunc = gui->tables [gui->tablecurr].movefunc;
  if (movefunc != NULL) {
    movefunc (gui, idx, dir);
  }
  gui->tables [gui->tablecurr].changed = true;
}

static bool
confuiTableRemove (void *udata)
{
  confuigui_t       *gui = udata;
  uivirtlist_t      *uivl;
  int               idx = -1;
  int               count = 0;
  int               flags;


  flags = gui->tables [gui->tablecurr].flags;

  uivl = gui->tables [gui->tablecurr].uivl;
  if (uivl == NULL) {
    return UICB_STOP;
  }

  count = uivlSelectionCount (uivl);
  if (count != 1) {
    return UICB_STOP;
  }

  idx = uivlGetCurrSelection (uivl);
  if (idx < 0) {
    return UICB_STOP;
  }

  count = gui->tables [gui->tablecurr].currcount;

  if (idx == 0 &&
      (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
    return UICB_CONT;
  }
  if (idx == count - 1 &&
      (flags & CONFUI_TABLE_KEEP_LAST) == CONFUI_TABLE_KEEP_LAST) {
    return UICB_CONT;
  }

  if (uivl != NULL) {
    removefunc_t  removefunc;

    removefunc = gui->tables [gui->tablecurr].removefunc;
    if (removefunc != NULL) {
      removefunc (gui, idx);
    }

    count = gui->tables [gui->tablecurr].currcount;
    if (idx >= count) {
      uivlSetSelection (uivl, count - 1);
    }
  }

  gui->tables [gui->tablecurr].changed = true;

  return UICB_CONT;
}

