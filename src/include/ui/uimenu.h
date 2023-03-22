/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIMENU_H
#define INC_UIMENU_H

#include "callback.h"
#include "uiwcont.h"

enum {
  UIUTILS_MENU_MAX = 5,
};

typedef struct uimenu uimenu_t;

uimenu_t *uiMenuAlloc (void);
void uiMenuFree (uimenu_t *);
bool uiMenuInitialized (uimenu_t *);
void uiMenuSetInitialized (uimenu_t *menu);
uiwcont_t *uiCreateMenubar (void);
uiwcont_t *uiCreateSubMenu (uiwcont_t *uimenuitem);
uiwcont_t *uiMenuCreateItem (uiwcont_t *uimenu, const char *txt, callback_t *uicb);
uiwcont_t *uiMenuCreateCheckbox (uiwcont_t *uimenu,
    const char *txt, int active, callback_t *uicb);
uiwcont_t *uiMenuAddMainItem (uiwcont_t *uimenubar, uimenu_t *menu, const char *txt);
void uiMenuAddSeparator (uiwcont_t *uimenu);
void uiMenuSetMainCallback (uiwcont_t *uimenuitem, callback_t *uicb);
void uiMenuDisplay (uimenu_t *menu);
void uiMenuClear (uimenu_t *menu);

#endif /* INC_UIMENU_H */
