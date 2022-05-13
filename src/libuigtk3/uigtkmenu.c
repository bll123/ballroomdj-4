#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "ui.h"
#include "uiutils.h"

GtkWidget *
uiCreateMenubar (void)
{
  GtkWidget *menubar;

  menubar = gtk_menu_bar_new ();
  return menubar;
}

GtkWidget *
uiCreateSubMenu (GtkWidget *menuitem)
{
  GtkWidget *menu;

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);
  return menu;
}

GtkWidget *
uiMenuCreateItem (GtkWidget *menu, const char *txt,
    void *activateAction, void *udata)
{
  GtkWidget *menuitem;

  menuitem = gtk_menu_item_new_with_label (txt);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  if (activateAction != NULL) {
    g_signal_connect (menuitem, "activate",
        G_CALLBACK (activateAction), udata);
  }

  return menuitem;
}

GtkWidget *
uiMenuCreateCheckbox (GtkWidget *menu, const char *txt, int active,
    void *toggleAction, void *udata)
{
  GtkWidget *menuitem;

  menuitem = gtk_check_menu_item_new_with_label (txt);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), active);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  if (toggleAction != NULL) {
    g_signal_connect (menuitem, "toggled", G_CALLBACK (toggleAction), udata);
  }
  return menuitem;
}

void
uiMenuInit (uimenu_t *menu)
{
  menu->initialized = false;
  menu->menucount = 0;
  for (int i = 0; i < UIUTILS_MENU_MAX; ++i) {
    menu->menuitem [i] = NULL;
  }
}

GtkWidget *
uiMenuAddMainItem (GtkWidget *menubar, uimenu_t *menu, const char *txt)
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
uiMenuDisplay (uimenu_t *menu)
{
  for (int i = 0; i < menu->menucount; ++i) {
    gtk_widget_show_all (menu->menuitem [i]);
  }
}

void
uiMenuClear (uimenu_t *menu)
{
  for (int i = 0; i < menu->menucount; ++i) {
    gtk_widget_hide (menu->menuitem [i]);
  }
}

