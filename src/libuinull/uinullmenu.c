/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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
  return NULL;
}

uiwcont_t *
uiCreateSubMenu (uiwcont_t *uimenuitem)
{
  return NULL;
}

uiwcont_t *
uiMenuCreateItem (uiwcont_t *uimenu, const char *txt, callback_t *uicb)
{
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
  return NULL;
}

uimenu_t *
uiMenuAlloc (void)
{
  return NULL;
}

void
uiMenuFree (uimenu_t *menu)
{
  return;
}

bool
uiMenuInitialized (uimenu_t *menu)
{
  return true;
}

void
uiMenuSetInitialized (uimenu_t *menu)
{
  return;
}

uiwcont_t *
uiMenuAddMainItem (uiwcont_t *uimenubar, uimenu_t *menu, const char *txt)
{
  return NULL;
}

void
uiMenuDisplay (uimenu_t *menu)
{
  return;
}

void
uiMenuClear (uimenu_t *menu)
{
  return;
}
