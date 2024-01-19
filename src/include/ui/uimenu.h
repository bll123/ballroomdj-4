/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIMENU_H
#define INC_UIMENU_H

#include "callback.h"
#include "uiwcont.h"

enum {
  UIUTILS_MENU_MAX = 10,
};

uiwcont_t *uiCreateMenubar (void);
uiwcont_t *uiMenuAddMainItem (uiwcont_t *uimenubar, uiwcont_t *uimenu, const char *txt);

uiwcont_t *uiMenuAlloc (void);
void uiMenuFree (uiwcont_t *);
bool uiMenuIsInitialized (uiwcont_t *);
void uiMenuSetInitialized (uiwcont_t *uiwidget);

uiwcont_t *uiCreateSubMenu (uiwcont_t *uimenuitem);

uiwcont_t *uiMenuCreateItem (uiwcont_t *uimenu, const char *txt, callback_t *uicb);
uiwcont_t *uiMenuCreateCheckbox (uiwcont_t *uimenu, const char *txt, int active, callback_t *uicb);
void uiMenuAddSeparator (uiwcont_t *uimenu);

void uiMenuSetMainCallback (uiwcont_t *uimenuitem, callback_t *uicb);
void uiMenuDisplay (uiwcont_t *uiwidget);
void uiMenuClear (uiwcont_t *uiwidget);

#endif /* INC_UIMENU_H */
