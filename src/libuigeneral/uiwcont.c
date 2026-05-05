/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <stdatomic.h>

#include "bdjstring.h"
#include "log.h"
#include "mdebug.h"
#include "uiwcont.h"

#include "ui/uiui.h"

static _Atomic(int32_t) gbdj4id_old = 0;
static _Atomic(int32_t) gbdj4id = 0;
static _Atomic(int32_t) gwidgetid = 100;

void
uiwcontInitID (int32_t id)
{
  gbdj4id = id;
}

void
uiwcontPushID (int32_t id)
{
  gbdj4id_old = gbdj4id;
  gbdj4id = id;
}

void
uiwcontPopID (void)
{
  gbdj4id = gbdj4id_old;
}

uiwcont_t *
uiwcontAlloc_r (int basetype, int type, const char *fn, int lineno)
{
  uiwcont_t    *uiwidget;

  uiwidget = mdmalloc (sizeof (uiwcont_t));
  uiwidget->wbasetype = basetype;
  uiwidget->wtype = type;
  memset (&uiwidget->uiint, 0, sizeof (uiwcontint_t));
  uiwidget->uidata = NULL;
  uiwidget->packed = false;
  uiwidget->ui_id = gbdj4id;
  uiwidget->id = 0;
  if (uiwidget->wbasetype == WCONT_T_BOX ||
      uiwidget->wbasetype == WCONT_T_WINDOW ||
      uiwidget->wbasetype == WCONT_T_GRID) {
    uiwidget->id = gwidgetid;
    gwidgetid += 100;
  }

  logMsg (LOG_DBG, LOG_UI | LOG_BACKTRACE, "wcont: %s %d-%d", uiwcontDesc (uiwidget->wtype), uiwidget->ui_id, uiwidget->id);
  uiwcontUIInit (uiwidget);                     // ui interface specific
  return uiwidget;
}

void
uiwcontBaseFree (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL) {
    return;
  }

  uiwcontUIFree (uiwidget);                     // ui interface specific
  uiwidget->wbasetype = WCONT_T_UNKNOWN;
  uiwidget->wtype = WCONT_T_UNKNOWN;
  uiwidget->widget = NULL;
  uiwidget->packwidget = NULL;
  mdfree (uiwidget);
}

void
uiwcontSetWidget (uiwcont_t *uiwidget, void *widget, void *packwidget)
{
  if (uiwidget == NULL || widget == NULL) {
    return;
  }

  uiwidget->widget = widget;
  uiwidget->packwidget = packwidget;
  if (packwidget == NULL) {
    uiwidget->packwidget = widget;
  }
  uiwcontUIWidgetInit (uiwidget);
}

void
uiwcontSetID (uiwcont_t *parent, uiwcont_t *uiwidget, int count)
{
  if (uiwidget->wbasetype != WCONT_T_BOX &&
      uiwidget->wbasetype != WCONT_T_GRID &&
      uiwidget->wbasetype != WCONT_T_WINDOW) {
    uiwidget->id = parent->id + count;
  }
  uiwidget->parentid = parent->id;
}

/* for debugging */
static const char *uiwcontdesc [WCONT_T_MAX] = {
  [WCONT_T_ADJUSTMENT] = "ADJUSTMENT",
  [WCONT_T_BOX] = "BOX",
  [WCONT_T_BUTTON] = "BUTTON",
  [WCONT_T_BUTTON_CHKBOX] = "CHECK_BOX",
  [WCONT_T_BUTTON_COLOR] = "COLOR_BUTTON",
  [WCONT_T_BUTTON_FONT] = "FONT_BUTTON",
  [WCONT_T_BUTTON_RADIO] = "RADIO_BUTTON",
  [WCONT_T_BUTTON_TOGGLE] = "TOGGLE_BUTTON",
  [WCONT_T_CHGIND] = "CHGIND",
  [WCONT_T_DIALOG_WINDOW] = "DIALOG_WINDOW",
  [WCONT_T_ENTRY] = "ENTRY",
  [WCONT_T_EVENT_BOX] = "EVENT_BOX",
  [WCONT_T_GRID] = "GRID",
  [WCONT_T_HBOX] = "H-BOX",
  [WCONT_T_IMAGE] = "IMAGE",
  [WCONT_T_KEY] = "KEY",
  [WCONT_T_LABEL] = "LABEL",
  [WCONT_T_LINK] = "LINK",
  [WCONT_T_MENUBAR] = "MENUBAR",
  [WCONT_T_MENU_CHECK_BOX] = "MENU_CHECK_BOX",
  [WCONT_T_MENU_ITEM] = "MENU_ITEM",
  [WCONT_T_MENU] = "MENU",
  [WCONT_T_MENU_MENUBAR_ITEM] = "MENU_MENUBAR_ITEM",
  [WCONT_T_MENU_SUB] = "MENU_SUB",
  [WCONT_T_NOTEBOOK] = "NOTEBOOK",
  [WCONT_T_PANED_WINDOW] = "PANED_WINDOW",
  [WCONT_T_PROGRESS_BAR] = "PROGRESS_BAR",
  [WCONT_T_SCALE] = "SCALE",
  [WCONT_T_SCROLLBAR] = "SCROLLBAR",
  [WCONT_T_WINDOW_SCROLL] = "SCROLL_WINDOW",
  [WCONT_T_SEPARATOR] = "SEPARATOR",
  [WCONT_T_SIZE_GROUP] = "SIZE_GROUP",
  [WCONT_T_SWITCH] = "SWITCH",
  [WCONT_T_TEXT_BOX] = "TEXT_BOX",
  [WCONT_T_TEXT_BUFFER] = "TEXT_BUFFER",
  [WCONT_T_UNKNOWN] = "unknown",
  [WCONT_T_VBOX] = "V-BOX",
  [WCONT_T_WINDOW] = "WINDOW",
  [WCONT_T_WINDOW_MAIN] = "MAIN-WINDOW",
};

static_assert (sizeof (uiwcontdesc) / sizeof (const char *) == WCONT_T_MAX,
    "missing wcont type");

/* for debugging */
void
uiwcontDebugDisp (uiwcont_t *uiwidget, const char *tag)   /* KEEP */
{
  if (uiwidget == NULL) {
    fprintf (stderr, "-- wcont: %s null\n", tag);
    return;
  }
  if (tag == NULL) {
    tag = "";
  }
  fprintf (stderr, "-- wcont: %s %" PRIu32 "-%" PRIu32 " %d/%s\n",
      tag, uiwidget->ui_id, uiwidget->id, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
}

const char *
uiwcontDesc (int wtype)   /* KEEP */
{
  if (wtype < 0 || wtype >= WCONT_T_MAX) {
    return "invalid";
  }
  return uiwcontdesc [wtype];
}
