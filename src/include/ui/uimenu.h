/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIMENU_H
#define INC_UIMENU_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

#include "callback.h"

enum {
  UIUTILS_MENU_MAX = 5,
};

typedef struct uimenu uimenu_t;

uimenu_t *uiMenuAlloc (void);
void uiMenuFree (uimenu_t *);
bool uiMenuInitialized (uimenu_t *);
void uiMenuSetInitialized (uimenu_t *menu);
void uiCreateMenubar (uiwcont_t *uiwidget);
void uiCreateSubMenu (uiwcont_t *uimenuitem, uiwcont_t *uimenu);
void uiMenuCreateItem (uiwcont_t *uimenu, uiwcont_t *uimenuitem, const char *txt, callback_t *uicb);
void uiMenuCreateCheckbox (uiwcont_t *uimenu, uiwcont_t *uimenuitem,
    const char *txt, int active, callback_t *uicb);
void uiMenuAddMainItem (uiwcont_t *uimenubar, uiwcont_t *uimenuitem,
    uimenu_t *menu, const char *txt);
void uiMenuAddSeparator (uiwcont_t *uimenu, uiwcont_t *uimenuitem);
void uiMenuSetMainCallback (uiwcont_t *uimenuitem, callback_t *uicb);
void uiMenuDisplay (uimenu_t *menu);
void uiMenuClear (uimenu_t *menu);

#endif /* INC_UIMENU_H */
