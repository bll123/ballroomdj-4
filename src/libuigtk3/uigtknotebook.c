/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
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
uiNotebookTabPositionLeft (UIWidget *uiwidget)
{
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (uiwidget->widget), GTK_POS_LEFT);
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
  gtk_notebook_set_current_page (GTK_NOTEBOOK (uinotebook->widget), pagenum);
}

void
uiNotebookHideShowPage (UIWidget *uinotebook, int pagenum, bool show)
{
  GtkWidget       *page;

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
  uiutilsCallbackLongHandler (uicb, pagenum);
}

