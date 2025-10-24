/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
  uiwidget = uiwcontAlloc (WCONT_T_NOTEBOOK, WCONT_T_NOTEBOOK);
  uiwcontSetWidget (uiwidget, widget, NULL);
  return uiwidget;
}

void
uiNotebookTabPositionLeft (uiwcont_t *uinotebook)
{
  if (! uiwcontValid (uinotebook, WCONT_T_NOTEBOOK, "nb-tab-left")) {
    return;
  }

  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (uinotebook->uidata.widget), GTK_POS_LEFT);
}

void
uiNotebookAppendPage (uiwcont_t *uinotebook, uiwcont_t *uibox,
    uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uinotebook, WCONT_T_NOTEBOOK, "nb-append-page")) {
    return;
  }
  /* at this time, only boxes and scrolled-windows are stored in nb pages */
  if (uibox->wbasetype != WCONT_T_BOX && uibox->wtype != WCONT_T_BOX &&
      uibox->wtype != WCONT_T_SCROLL_WINDOW) {
    fprintf (stderr, "ERR: %s incorrect type exp:%d/%s actual:%d/%s %d/%s\n",
        "nb-append-page-box",
        WCONT_T_BOX, uiwcontDesc (WCONT_T_BOX),
        uiwidget->wbasetype, uiwcontDesc (uiwidget->wbasetype),
        uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
    return;
  }
  if (uiwidget == NULL) {
    return;
  }

  gtk_notebook_append_page (GTK_NOTEBOOK (uinotebook->uidata.widget),
      uibox->uidata.widget, uiwidget->uidata.widget);
}

void
uiNotebookSetActionWidget (uiwcont_t *uinotebook, uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uinotebook, WCONT_T_NOTEBOOK, "nb-set-action-widget")) {
    return;
  }
  if (uiwidget == NULL) {
    return;
  }

  gtk_notebook_set_action_widget (GTK_NOTEBOOK (uinotebook->uidata.widget),
      uiwidget->uidata.widget, GTK_PACK_END);
}

void
uiNotebookSetPage (uiwcont_t *uinotebook, int pagenum)
{
  if (! uiwcontValid (uinotebook, WCONT_T_NOTEBOOK, "nb-set-page")) {
    return;
  }

  gtk_notebook_set_current_page (GTK_NOTEBOOK (uinotebook->uidata.widget), pagenum);
}

void
uiNotebookHideShowPage (uiwcont_t *uinotebook, int pagenum, bool show)
{
  GtkWidget       *page;

  if (! uiwcontValid (uinotebook, WCONT_T_NOTEBOOK, "nb-hide-show")) {
    return;
  }

  page = gtk_notebook_get_nth_page (
      GTK_NOTEBOOK (uinotebook->uidata.widget), pagenum);
  gtk_widget_set_visible (page, show);
}

void
uiNotebookSetCallback (uiwcont_t *uinotebook, callback_t *uicb)
{
  if (! uiwcontValid (uinotebook, WCONT_T_NOTEBOOK, "nb-set-cb")) {
    return;
  }
  if (uicb == NULL) {
    return;
  }

  g_signal_connect (uinotebook->uidata.widget, "switch-page",
      G_CALLBACK (uiNotebookSwitchPageHandler), uicb);
}

void
uiNotebookSetScrollable (uiwcont_t *uinotebook)
{
  if (! uiwcontValid (uinotebook, WCONT_T_NOTEBOOK, "nb-scrollable")) {
    return;
  }

  gtk_notebook_set_scrollable (GTK_NOTEBOOK (uinotebook->uidata.widget), true);
}

/* internal routines */

static void
uiNotebookSwitchPageHandler (GtkNotebook *nb, GtkWidget *page,
    guint pagenum, gpointer udata)
{
  callback_t  *uicb = udata;
  callbackHandlerI (uicb, pagenum);
}

