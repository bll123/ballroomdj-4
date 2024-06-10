/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "mdebug.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

uiwcont_t *
uiwcontAlloc (void)
{
  uiwcont_t    *uiwidget;

  uiwidget = mdmalloc (sizeof (uiwcont_t));
  uiwidget->wbasetype = WCONT_T_UNKNOWN;
  uiwidget->wtype = WCONT_T_UNKNOWN;
  uiwidget->uidata.widget = NULL;
  uiwidget->uidata.packwidget = NULL;          // often the same as widget
  uiwidget->uiint.voidwidget = NULL;    // union
  return uiwidget;
}

void
uiwcontBaseFree (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL) {
    return;
  }

  uiwidget->wbasetype = WCONT_T_UNKNOWN;
  uiwidget->wtype = WCONT_T_UNKNOWN;
  mdfree (uiwidget);
}

/* for debugging */
static const char *uiwcontdesc [WCONT_T_MAX] = {
  [WCONT_T_ADJUSTMENT] = "ADJUSTMENT",
  [WCONT_T_BOX] = "BOX",
  [WCONT_T_BUTTON] = "BUTTON",
  [WCONT_T_CHECK_BOX] = "CHECK_BOX",
  [WCONT_T_CHGIND] = "CHGIND",
  [WCONT_T_COLOR_BUTTON] = "COLOR_BUTTON",
  [WCONT_T_DIALOG_WINDOW] = "DIALOG_WINDOW",
  [WCONT_T_DROPDOWN] = "DROPDOWN",
  [WCONT_T_ENTRY] = "ENTRY",
  [WCONT_T_FONT_BUTTON] = "FONT_BUTTON",
  [WCONT_T_IMAGE] = "IMAGE",
  [WCONT_T_KEY] = "KEY",
  [WCONT_T_LABEL] = "LABEL",
  [WCONT_T_LINK] = "LINK",
  [WCONT_T_MENUBAR] = "MENUBAR",
  [WCONT_T_MENU] = "MENU",
  [WCONT_T_MENU_CHECK_BOX] = "MENU_CHECK_BOX",
  [WCONT_T_MENU_ITEM] = "MENU_ITEM",
  [WCONT_T_MENU_MENUBAR_ITEM] = "MENU_MENUBAR_ITEM",
  [WCONT_T_MENU_SUB] = "MENU_SUB",
  [WCONT_T_NOTEBOOK] = "NOTEBOOK",
  [WCONT_T_PANED_WINDOW] = "PANED_WINDOW",
  [WCONT_T_PIXBUF] = "PIXBUF",
  [WCONT_T_PROGRESS_BAR] = "PROGRESS_BAR",
  [WCONT_T_RADIO_BUTTON] = "RADIO_BUTTON",
  [WCONT_T_SCALE] = "SCALE",
  [WCONT_T_SCROLLBAR] = "SCROLLBAR",
  [WCONT_T_SCROLL_WINDOW] = "SCROLL_WINDOW",
  [WCONT_T_SEPARATOR] = "SEPARATOR",
  [WCONT_T_SIZE_GROUP] = "SIZE_GROUP",
  [WCONT_T_SPINBOX] = "SPINBOX",
  [WCONT_T_SPINBOX_DOUBLE] = "SPINBOX_DOUBLE",
  [WCONT_T_SPINBOX_DOUBLE_DFLT] = "SPINBOX_DOUBLE_DFLT",
  [WCONT_T_SPINBOX_NUM] = "SPINBOX_NUM",
  [WCONT_T_SPINBOX_TEXT] = "SPINBOX_TEXT",
  [WCONT_T_SPINBOX_TIME] = "SPINBOX_TIME",
  [WCONT_T_SWITCH] = "SWITCH",
  [WCONT_T_TEXT_BOX] = "TEXT_BOX",
  [WCONT_T_TEXT_BUFFER] = "TEXT_BUFFER",
  [WCONT_T_TOGGLE_BUTTON] = "TOGGLE_BUTTON",
  [WCONT_T_TREE] = "TREE",
  [WCONT_T_UNKNOWN] = "unknown",
  [WCONT_T_WINDOW] = "WINDOW",
};

/* for debugging */
const char *
uiwcontDesc (int wtype)
{
  if (wtype < 0 || wtype >= WCONT_T_MAX) {
    return "invalid";
  }
  return uiwcontdesc [wtype];
}
