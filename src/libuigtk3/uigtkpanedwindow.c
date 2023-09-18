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

#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uipanedwindow.h"

static uiwcont_t * uiPanedWindowCreate (int orientation);

uiwcont_t *
uiPanedWindowCreateHoriz (void)
{
  uiwcont_t *uiwidget;

  uiwidget = uiPanedWindowCreate (GTK_ORIENTATION_HORIZONTAL);
  return uiwidget;
}

uiwcont_t *
uiPanedWindowCreateVert (void)
{
  uiwcont_t *uiwidget;

  uiwidget = uiPanedWindowCreate (GTK_ORIENTATION_VERTICAL);
  return uiwidget;
}

void
uiPanedWindowPackStart (uiwcont_t *panedwin, uiwcont_t *box)
{
  if (panedwin == NULL || box == NULL) {
    return;
  }
  if (panedwin->wtype != WCONT_T_PANED_WINDOW) {
    return;
  }

  gtk_paned_pack1 (GTK_PANED (panedwin->widget), box->widget, true, true);
}

void
uiPanedWindowPackEnd (uiwcont_t *panedwin, uiwcont_t *box)
{
  if (panedwin == NULL || box == NULL) {
    return;
  }

  if (panedwin->wtype != WCONT_T_PANED_WINDOW) {
    return;
  }

  gtk_paned_pack2 (GTK_PANED (panedwin->widget), box->widget, true, true);
}

int
uiPanedWindowGetPosition (uiwcont_t *panedwin)
{
  if (panedwin == NULL) {
    return -1;
  }

  if (panedwin->wtype != WCONT_T_PANED_WINDOW) {
    return -1;
  }

  return gtk_paned_get_position (GTK_PANED (panedwin->widget));
}

void
uiPanedWindowSetPosition (uiwcont_t *panedwin, int position)
{
  if (panedwin == NULL) {
    return;
  }

  if (panedwin->wtype != WCONT_T_PANED_WINDOW) {
    return;
  }

  gtk_paned_set_position (GTK_PANED (panedwin->widget), position);
}

/* internal routines */

static uiwcont_t *
uiPanedWindowCreate (int orientation)
{
  uiwcont_t *uiwidget;
  GtkWidget *widget;

  widget = gtk_paned_new (orientation);
  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_widget_set_margin_top (widget, 0);
  gtk_widget_set_margin_bottom (widget, 0);
  gtk_widget_set_margin_start (widget, 0);
  gtk_widget_set_margin_end (widget, 0);
  gtk_paned_set_wide_handle (GTK_PANED (widget), true);

  uiwidget = uiwcontAlloc ();
  uiwidget->wtype = WCONT_T_PANED_WINDOW;
  uiwidget->widget = widget;
  return uiwidget;
}
