#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

#include <gtk/gtk.h>

#include "bdj4intl.h"
#include "ui.h"
#include "uiduallist.h"
#include "uiutils.h"

uiduallist_t *
uiCreateDualList (UIWidget *vbox,
    UICallback *uiselectcb, UICallback *uiremovecb,
    UICallback *uimoveupcb, UICallback *uimovedowncb)
{
  uiduallist_t  *duallist;
  UIWidget      hbox;
  UIWidget      dvbox;
  UIWidget      uiwidget;
  GtkWidget     *tree;

  duallist = malloc (sizeof (uiduallist_t));

  uiutilsUIWidgetInit (&hbox);
  uiutilsUIWidgetInit (&dvbox);
  uiutilsUIWidgetInit (&uiwidget);

  uiCreateHorizBox (&hbox);
  uiWidgetAlignHorizStart (&hbox);
  uiBoxPackStartExpand (vbox, &hbox);

  uiCreateScrolledWindow (&uiwidget, 300);
  uiWidgetExpandVert (&uiwidget);
  uiBoxPackStartExpand (&hbox, &uiwidget);

  tree = uiCreateTreeView ();
  uiSetCss (tree,
      "treeview { background-color: shade(@theme_base_color,0.8); } "
      "treeview:selected { background-color: @theme_selected_bg_color; } ");
  uiWidgetSetMarginStartW (tree, uiBaseMarginSz * 8);
  uiWidgetSetMarginTopW (tree, uiBaseMarginSz * 8);
  uiWidgetExpandVertW (tree);
  uiBoxPackInWindowUW (&uiwidget, tree);
  duallist->ltree = tree;
  duallist->lsel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

  uiCreateVertBox (&dvbox);
  uiWidgetSetAllMargins (&dvbox, uiBaseMarginSz * 4);
  uiWidgetSetMarginTop (&dvbox, uiBaseMarginSz * 64);
  uiWidgetAlignVertStart (&dvbox);
  uiBoxPackStart (&hbox, &dvbox);

  /* CONTEXT: configuration: display settings: button: add the selected field */
  uiCreateButton (&uiwidget, uiselectcb, _("Select"),
      "button_right", NULL, NULL);
  uiBoxPackStart (&dvbox, &uiwidget);

  /* CONTEXT: configuration: display settings: button: remove the selected field */
  uiCreateButton (&uiwidget, uiremovecb, _("Remove"),
      "button_left", NULL, NULL);
  uiBoxPackStart (&dvbox, &uiwidget);

  uiCreateScrolledWindow (&uiwidget, 300);
  uiWidgetExpandVert (&uiwidget);
  uiBoxPackStartExpand (&hbox, &uiwidget);

  tree = uiCreateTreeView ();
  uiSetCss (tree,
      "treeview { background-color: shade(@theme_base_color,0.8); } "
      "treeview:selected { background-color: @theme_selected_bg_color; } ");
  uiWidgetSetMarginStartW (tree, uiBaseMarginSz * 8);
  uiWidgetSetMarginTopW (tree, uiBaseMarginSz * 8);
  uiWidgetExpandVertW (tree);
  uiBoxPackInWindowUW (&uiwidget, tree);
  duallist->rtree = tree;
  duallist->rsel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

  uiCreateVertBox (&dvbox);
  uiWidgetSetAllMargins (&dvbox, uiBaseMarginSz * 4);
  uiWidgetSetMarginTop (&dvbox, uiBaseMarginSz * 64);
  uiWidgetAlignVertStart (&dvbox);
  uiBoxPackStart (&hbox, &dvbox);

  /* CONTEXT: configuration: display settings: button: move the selected field up */
  uiCreateButton (&uiwidget, uimoveupcb, _("Move Up"),
      "button_up", NULL, NULL);
  uiBoxPackStart (&dvbox, &uiwidget);

  /* CONTEXT: configuration: display settings: button: move the selected field down */
  uiCreateButton (&uiwidget, uimovedowncb, _("Move Down"),
      "button_down", NULL, NULL);
  uiBoxPackStart (&dvbox, &uiwidget);

  return duallist;
}
