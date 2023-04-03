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
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiwidget.h"
#include "ui/uimenu.h"

typedef struct uimenu {
  int             menucount;
  uiwcont_t       *menuitem [UIUTILS_MENU_MAX];
  bool            initialized : 1;
} uimenu_t;

static void uiMenuActivateHandler (GtkMenuItem *mi, gpointer udata);
static void uiMenuToggleHandler (GtkWidget *mi, gpointer udata);

uiwcont_t *
uiCreateMenubar (void)
{
  uiwcont_t *uiwidget;
  GtkWidget *menubar;

  menubar = gtk_menu_bar_new ();
  uiwidget = uiwcontAlloc ();
  uiwidget->wtype = WCONT_T_MENUBAR;
  uiwidget->widget = menubar;
  return uiwidget;
}

uiwcont_t *
uiCreateSubMenu (uiwcont_t *uimenuitem)
{
  uiwcont_t *uimenu;
  GtkWidget *menu;

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (uimenuitem->widget), menu);
  uimenu = uiwcontAlloc ();
  uimenu->wtype = WCONT_T_MENU_SUB;
  uimenu->widget = menu;
  return uimenu;
}

uiwcont_t *
uiMenuCreateItem (uiwcont_t *uimenu, const char *txt, callback_t *uicb)
{
  uiwcont_t *uimenuitem;
  GtkWidget *menuitem;

  menuitem = gtk_menu_item_new_with_label (txt);
  gtk_menu_shell_append (GTK_MENU_SHELL (uimenu->widget), menuitem);
  if (uicb != NULL) {
    g_signal_connect (menuitem, "activate",
        G_CALLBACK (uiMenuActivateHandler), uicb);
  }
  uimenuitem = uiwcontAlloc ();
  uimenuitem->wtype = WCONT_T_MENU_ITEM;
  uimenuitem->widget = menuitem;
  return uimenuitem;
}

void
uiMenuAddSeparator (uiwcont_t *uimenu)
{
  GtkWidget *menuitem;

  menuitem = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (uimenu->widget), menuitem);
}

uiwcont_t *
uiMenuCreateCheckbox (uiwcont_t *uimenu,
    const char *txt, int active, callback_t *uicb)
{
  uiwcont_t *uimenuitem;
  GtkWidget *menuitem;

  menuitem = gtk_check_menu_item_new_with_label (txt);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), active);
  gtk_menu_shell_append (GTK_MENU_SHELL (uimenu->widget), menuitem);
  if (uicb != NULL) {
    g_signal_connect (menuitem, "toggled",
        G_CALLBACK (uiMenuToggleHandler), uicb);
  }
  uimenuitem = uiwcontAlloc ();
  uimenuitem->wtype = WCONT_T_MENU_CHECK_BOX;
  uimenuitem->widget = menuitem;
  return uimenuitem;
}

uimenu_t *
uiMenuAlloc (void)
{
  uimenu_t    *menu;

  menu = mdmalloc (sizeof (uimenu_t));
  menu->initialized = false;
  menu->menucount = 0;
  for (int i = 0; i < UIUTILS_MENU_MAX; ++i) {
    menu->menuitem [i] = NULL;
  }
  return menu;
}

void
uiMenuFree (uimenu_t *menu)
{
  if (menu != NULL) {
    for (int i = 0; i < UIUTILS_MENU_MAX; ++i) {
      uiwcontFree (menu->menuitem [i]);
    }
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

uiwcont_t *
uiMenuAddMainItem (uiwcont_t *uimenubar, uimenu_t *menu, const char *txt)
{
  uiwcont_t *uimenuitem;
  int       i;

  if (menu->menucount >= UIUTILS_MENU_MAX) {
    return NULL;
  }

  i = menu->menucount;
  ++menu->menucount;
  uimenuitem = uiwcontAlloc ();
  uimenuitem->wtype = WCONT_T_MENU_MAIN_ITEM;
  uimenuitem->widget = gtk_menu_item_new_with_label (txt);
  gtk_menu_shell_append (GTK_MENU_SHELL (uimenubar->widget),
      uimenuitem->widget);
  uiWidgetHide (uimenuitem);
  /* create our own copy here */
  menu->menuitem [i] = uiwcontAlloc ();
  menu->menuitem [i]->wtype = WCONT_T_MENU_MAIN_ITEM;
  menu->menuitem [i]->widget = uimenuitem->widget;
  return uimenuitem;
}

void
uiMenuDisplay (uimenu_t *menu)
{
  for (int i = 0; i < menu->menucount; ++i) {
    uiWidgetShowAll (menu->menuitem [i]);
  }
}

void
uiMenuClear (uimenu_t *menu)
{
  for (int i = 0; i < menu->menucount; ++i) {
    uiWidgetHide (menu->menuitem [i]);
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
