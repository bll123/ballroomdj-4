/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "callback.h"
#include "mdebug.h"

#include "ui/uiinternal.h"
#include "ui/uigeneral.h"
#include "ui/uiwidget.h"
#include "ui/uimenu.h"

typedef struct uimenu {
  int             menucount;
  uiwidget_t      menuitem [UIUTILS_MENU_MAX];
  bool            initialized : 1;
} uimenu_t;

static void uiMenuActivateHandler (GtkMenuItem *mi, gpointer udata);
static void uiMenuToggleHandler (GtkWidget *mi, gpointer udata);

void
uiCreateMenubar (uiwidget_t *uiwidget)
{
  GtkWidget *menubar;

  menubar = gtk_menu_bar_new ();
  uiwidget->widget = menubar;
}

void
uiCreateSubMenu (uiwidget_t *uimenuitem, uiwidget_t *uimenu)
{
  GtkWidget *menu;

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (uimenuitem->widget), menu);
  uimenu->widget = menu;
}

void
uiMenuCreateItem (uiwidget_t *uimenu, uiwidget_t *uimenuitem,
    const char *txt, callback_t *uicb)
{
  GtkWidget *menuitem;

  menuitem = gtk_menu_item_new_with_label (txt);
  gtk_menu_shell_append (GTK_MENU_SHELL (uimenu->widget), menuitem);
  if (uicb != NULL) {
    g_signal_connect (menuitem, "activate",
        G_CALLBACK (uiMenuActivateHandler), uicb);
  }
  uimenuitem->widget = menuitem;
}

void
uiMenuAddSeparator (uiwidget_t *uimenu, uiwidget_t *uimenuitem)
{
  GtkWidget *menuitem;

  menuitem = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (uimenu->widget), menuitem);
  uimenuitem->widget = menuitem;
}

void
uiMenuCreateCheckbox (uiwidget_t *uimenu, uiwidget_t *uimenuitem,
    const char *txt, int active, callback_t *uicb)
{
  GtkWidget *menuitem;

  menuitem = gtk_check_menu_item_new_with_label (txt);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), active);
  gtk_menu_shell_append (GTK_MENU_SHELL (uimenu->widget), menuitem);
  if (uicb != NULL) {
    g_signal_connect (menuitem, "toggled",
        G_CALLBACK (uiMenuToggleHandler), uicb);
  }
  uimenuitem->widget = menuitem;
}

uimenu_t *
uiMenuAlloc (void)
{
  uimenu_t    *menu;

  menu = mdmalloc (sizeof (uimenu_t));
  menu->initialized = false;
  menu->menucount = 0;
  for (int i = 0; i < UIUTILS_MENU_MAX; ++i) {
    uiutilsUIWidgetInit (&menu->menuitem [i]);
  }
  return menu;
}

void
uiMenuFree (uimenu_t *menu)
{
  if (menu != NULL) {
    mdfree (menu);
  }
}

bool
uiMenuInitialized (uimenu_t *menu)
{
  if (menu == NULL) {
    return false;
  }

  return menu->initialized;
}

void
uiMenuSetInitialized (uimenu_t *menu)
{
  if (menu == NULL) {
    return;
  }

  menu->initialized = true;
}

void
uiMenuAddMainItem (uiwidget_t *uimenubar, uiwidget_t *uimenuitem,
    uimenu_t *menu, const char *txt)
{
  int   i;

  if (menu->menucount >= UIUTILS_MENU_MAX) {
    return;
  }

  i = menu->menucount;
  ++menu->menucount;
  uimenuitem->widget = gtk_menu_item_new_with_label (txt);
  gtk_menu_shell_append (GTK_MENU_SHELL (uimenubar->widget),
      uimenuitem->widget);
  uiWidgetHide (uimenuitem);
  memcpy (&menu->menuitem [i], uimenuitem, sizeof (UIWidget));
}

void
uiMenuDisplay (uimenu_t *menu)
{
  for (int i = 0; i < menu->menucount; ++i) {
    uiWidgetShowAll (&menu->menuitem [i]);
  }
}

void
uiMenuClear (uimenu_t *menu)
{
  for (int i = 0; i < menu->menucount; ++i) {
    uiWidgetHide (&menu->menuitem [i]);
  }
}

static void
uiMenuActivateHandler (GtkMenuItem *mi, gpointer udata)
{
  callback_t  *uicb = udata;

  callbackHandler (uicb);
}

static void
uiMenuToggleHandler (GtkWidget *mi, gpointer udata)
{
  callback_t  *uicb = udata;
  callbackHandler (uicb);
}
