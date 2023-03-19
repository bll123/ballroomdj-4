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
void uiCreateMenubar (uiwidget_t *uiwidget);
void uiCreateSubMenu (uiwidget_t *uimenuitem, uiwidget_t *uimenu);
void uiMenuCreateItem (uiwidget_t *uimenu, uiwidget_t *uimenuitem, const char *txt, callback_t *uicb);
void uiMenuCreateCheckbox (uiwidget_t *uimenu, uiwidget_t *uimenuitem,
    const char *txt, int active, callback_t *uicb);
void uiMenuAddMainItem (uiwidget_t *uimenubar, uiwidget_t *uimenuitem,
    uimenu_t *menu, const char *txt);
void uiMenuAddSeparator (uiwidget_t *uimenu, uiwidget_t *uimenuitem);
void uiMenuSetMainCallback (uiwidget_t *uimenuitem, callback_t *uicb);
void uiMenuDisplay (uimenu_t *menu);
void uiMenuClear (uimenu_t *menu);

#endif /* INC_UIMENU_H */
