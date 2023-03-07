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

typedef struct {
  int             menucount;
  UIWidget        menuitem [UIUTILS_MENU_MAX];
  bool            initialized : 1;
} uimenu_t;

void uiCreateMenubar (UIWidget *uiwidget);
void uiCreateSubMenu (UIWidget *uimenuitem, UIWidget *uimenu);
void uiMenuCreateItem (UIWidget *uimenu, UIWidget *uimenuitem, const char *txt, callback_t *uicb);
void uiMenuCreateCheckbox (UIWidget *uimenu, UIWidget *uimenuitem,
    const char *txt, int active, callback_t *uicb);
void uiMenuInit (uimenu_t *menu);
void uiMenuAddMainItem (UIWidget *uimenubar, UIWidget *uimenuitem,
    uimenu_t *menu, const char *txt);
void uiMenuAddSeparator (UIWidget *uimenu, UIWidget *uimenuitem);
void uiMenuSetMainCallback (UIWidget *uimenuitem, callback_t *uicb);
void uiMenuDisplay (uimenu_t *menu);
void uiMenuClear (uimenu_t *menu);

#endif /* INC_UIMENU_H */
