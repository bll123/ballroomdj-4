#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "log.h"
#include "uiutils.h"

GtkWidget *
uiutilsCreateMenubar (void)
{
  GtkWidget *menubar;

  menubar = gtk_menu_bar_new ();
  return menubar;
}

GtkWidget *
uiutilsCreateSubMenu (GtkWidget *menuitem)
{
  GtkWidget *menu;

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);
  return menu;
}

GtkWidget *
uiutilsCreateMenuItem (GtkWidget *menu, const char *txt)
{
  GtkWidget *menuitem;

  menuitem = gtk_menu_item_new_with_label (txt);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  return menuitem;
}

void
uiutilsMenuInit (uiutilsmenu_t *menu)
{
  menu->initialized = false;
  menu->menucount = 0;
  for (int i = 0; i < UIUTILS_MENU_MAX; ++i) {
    menu->menuitem [i] = NULL;
  }
}

GtkWidget *
uiutilsMenuAddMainItem (GtkWidget *menubar, uiutilsmenu_t *menu, const char *txt)
{
  int   i;

  if (menu->menucount >= UIUTILS_MENU_MAX) {
    return NULL;
  }

  i = menu->menucount;
  ++menu->menucount;
  menu->menuitem [i] = gtk_menu_item_new_with_label (txt);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), menu->menuitem [i]);
  gtk_widget_hide (menu->menuitem [i]);
  return menu->menuitem [i];
}

void
uiutilsMenuDisplay (uiutilsmenu_t *menu)
{
  for (int i = 0; i < menu->menucount; ++i) {
    gtk_widget_show_all (menu->menuitem [i]);
  }
}

void
uiutilsMenuClear (uiutilsmenu_t *menu)
{
  for (int i = 0; i < menu->menucount; ++i) {
    gtk_widget_hide (menu->menuitem [i]);
  }
}

