#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>

#include "ui.h"
#include "uiutils.h"

static void
uiNotebookSwitchPageHandler (GtkNotebook *nb, GtkWidget *page,
    guint pagenum, gpointer udata);

void
uiCreateNotebook (UIWidget *uiwidget)
{
  GtkWidget   *widget;

  widget = gtk_notebook_new ();
  assert (widget != NULL);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (widget), TRUE);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz * 2);
  gtk_widget_set_hexpand (widget, TRUE);
  gtk_widget_set_vexpand (widget, FALSE);
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (widget), GTK_POS_TOP);
  uiSetCss (widget,
      "notebook tab:checked { background-color: shade(@theme_base_color,0.6); }");
  uiwidget->widget = widget;
}

void
uiNotebookAppendPage (UIWidget *uinotebook, UIWidget *uiwidget,
    UIWidget *uilabel)
{
  gtk_notebook_append_page (GTK_NOTEBOOK (uinotebook->widget),
      uiwidget->widget, uilabel->widget);
}

void
uiNotebookSetActionWidget (UIWidget *uinotebook, UIWidget *uiwidget)
{
  gtk_notebook_set_action_widget (GTK_NOTEBOOK (uinotebook->widget),
      uiwidget->widget, GTK_PACK_END);
}

void
uiNotebookSetPage (UIWidget *uinotebook, int pagenum)
{
fprintf (stderr, "set current: %d\n", pagenum);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (uinotebook->widget), pagenum);
}

void
uiNotebookHideShowPage (UIWidget *uinotebook, int pagenum, bool show)
{
  GtkWidget       *page;

fprintf (stderr, "hide/show: %d (%d)\n", pagenum, show);
  page = gtk_notebook_get_nth_page (
      GTK_NOTEBOOK (uinotebook->widget), pagenum);
  gtk_widget_set_visible (page, show);
}

void
uiNotebookSetCallback (UIWidget *uinotebook, UICallback *uicb)
{
  g_signal_connect (uinotebook->widget, "switch-page",
      G_CALLBACK (uiNotebookSwitchPageHandler), uicb);
}

/* internal routines */

static void
uiNotebookSwitchPageHandler (GtkNotebook *nb, GtkWidget *page,
    guint pagenum, gpointer udata)
{
  UICallback  *uicb = udata;

  if (uicb == NULL) {
    return;
  }
  if (uicb->intcb == NULL) {
    return;
  }

  uicb->intcb (uicb->udata, pagenum);
}

/* these routines will be removed at a later date */

GtkWidget *
uiCreateNotebookW (void)
{
  GtkWidget   *widget;

  widget = gtk_notebook_new ();
  assert (widget != NULL);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (widget), TRUE);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz * 2);
  gtk_widget_set_hexpand (widget, TRUE);
  gtk_widget_set_vexpand (widget, FALSE);
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (widget), GTK_POS_TOP);
  uiSetCss (widget,
      "notebook tab:checked { background-color: shade(@theme_base_color,0.6); }");
  return widget;
}

void
uiNotebookAppendPageW (GtkWidget *notebook, GtkWidget *widget,
    GtkWidget *labelwidget)
{
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), widget, labelwidget);
}

void
uiNotebookSetActionWidgetW (GtkWidget *notebook, GtkWidget *widget, GtkPackType pack)
{
  gtk_notebook_set_action_widget (GTK_NOTEBOOK (notebook), widget, pack);
}

void
uiNotebookSetPageW (GtkWidget *notebook, int pagenum)
{
  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), pagenum);
}

