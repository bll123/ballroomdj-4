/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "callback.h"
#include "mdebug.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiwidget.h"
#include "ui/uimenu.h"

typedef struct uimenu {
  int         junk;
} uimenu_t;

uiwcont_t *
uiCreateMenubar (void)
{
fprintf (stderr, "c-menubar\n");
  return NULL;
}

uiwcont_t *
uiCreateSubMenu (uiwcont_t *uimenuitem)
{
fprintf (stderr, "c-sub-menu\n");
  return NULL;
}

uiwcont_t *
uiMenuCreateItem (uiwcont_t *uimenu, const char *txt, callback_t *uicb)
{
fprintf (stderr, "c-menu-item\n");
  return NULL;
}

void
uiMenuAddSeparator (uiwcont_t *uimenu)
{
  return;
}

uiwcont_t *
uiMenuCreateCheckbox (uiwcont_t *uimenu,
    const char *txt, int active, callback_t *uicb)
{
fprintf (stderr, "c-menu-cb\n");
  return NULL;
}

uiwcont_t *
uiMenuAlloc (void)
{
  return NULL;
}

void
uiMenuFree (uiwcont_t *uiwidget)
{
  return;
}

bool
uiMenuIsInitialized (uiwcont_t *uiwidget)
{
  return true;
}

void
uiMenuSetInitialized (uiwcont_t *uiwidget)
{
  return;
}

uiwcont_t *
uiMenuAddMainItem (uiwcont_t *uimenubar, uiwcont_t *uimenu, const char *txt)
{
  return NULL;
}

void
uiMenuDisplay (uiwcont_t *uiwidget)
{
  return;
}

void
uiMenuClear (uiwcont_t *uiwidget)
{
  return;
}

void
uiMenuItemSetText (uiwcont_t *uiwidget, const char *txt)
{
  if (! uiwcontValid (uiwidget, WCONT_T_MENU_ITEM, "menu-item-set-text")) {
    return;
  }
}

