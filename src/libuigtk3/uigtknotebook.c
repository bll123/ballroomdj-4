/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>

#include "callback.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uinotebook.h"

static void
uiNotebookSwitchPageHandler (GtkNotebook *nb, GtkWidget *page,
    guint pagenum, gpointer udata);

uiwcont_t *
uiCreateNotebook (void)
{
  uiwcont_t   *uiwidget;
  GtkWidget   *widget;

  widget = gtk_notebook_new ();
  gtk_notebook_set_show_border (GTK_NOTEBOOK (widget), TRUE);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz * 2);
  gtk_widget_set_hexpand (widget, TRUE);
  gtk_widget_set_vexpand (widget, FALSE);
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (widget), GTK_POS_TOP);
  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_NOTEBOOK;
  uiwidget->wtype = WCONT_T_NOTEBOOK;
  uiwidget->widget = widget;
  return uiwidget;
}

void
uiNotebookTabPositionLeft (uiwcont_t *uiwidget)
{
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (uiwidget->widget), GTK_POS_LEFT);
}

void
uiNotebookAppendPage (uiwcont_t *uinotebook, uiwcont_t *uiwidget,
    uiwcont_t *uilabel)
{
  gtk_notebook_append_page (GTK_NOTEBOOK (uinotebook->widget),
      uiwidget->widget, uilabel->widget);
}

void
uiNotebookSetActionWidget (uiwcont_t *uinotebook, uiwcont_t *uiwidget)
{
  gtk_notebook_set_action_widget (GTK_NOTEBOOK (uinotebook->widget),
      uiwidget->widget, GTK_PACK_END);
}

void
uiNotebookSetPage (uiwcont_t *uinotebook, int pagenum)
{
  gtk_notebook_set_current_page (GTK_NOTEBOOK (uinotebook->widget), pagenum);
}

void
uiNotebookHideShowPage (uiwcont_t *uinotebook, int pagenum, bool show)
{
  GtkWidget       *page;

  page = gtk_notebook_get_nth_page (
      GTK_NOTEBOOK (uinotebook->widget), pagenum);
  gtk_widget_set_visible (page, show);
}

void
uiNotebookSetCallback (uiwcont_t *uinotebook, callback_t *uicb)
{
  g_signal_connect (uinotebook->widget, "switch-page",
      G_CALLBACK (uiNotebookSwitchPageHandler), uicb);
}

/* internal routines */

static void
uiNotebookSwitchPageHandler (GtkNotebook *nb, GtkWidget *page,
    guint pagenum, gpointer udata)
{
  callback_t  *uicb = udata;
  callbackHandlerLong (uicb, pagenum);
}

