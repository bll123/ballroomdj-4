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

#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uibox.h"

static uiwcont_t * uiCreateBox (int orientation);

uiwcont_t *
uiCreateVertBox (void)
{
  return uiCreateBox (GTK_ORIENTATION_VERTICAL);
}

uiwcont_t *
uiCreateHorizBox (void)
{
  return uiCreateBox (GTK_ORIENTATION_HORIZONTAL);
}

void
uiBoxPackInWindow (uiwcont_t *uiwindow, uiwcont_t *uibox)
{
  gtk_container_add (GTK_CONTAINER (uiwindow->widget), uibox->widget);
}

void
uiBoxPackStart (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  gtk_box_pack_start (GTK_BOX (uibox->widget), uiwidget->widget, FALSE, FALSE, 0);
}

void
uiBoxPackStartExpand (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  gtk_box_pack_start (GTK_BOX (uibox->widget), uiwidget->widget, TRUE, TRUE, 0);
}

void
uiBoxPackEnd (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  gtk_box_pack_end (GTK_BOX (uibox->widget), uiwidget->widget, FALSE, FALSE, 0);
}

void
uiBoxPackEndExpand (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  gtk_box_pack_end (GTK_BOX (uibox->widget), uiwidget->widget, TRUE, TRUE, 0);
}

static uiwcont_t *
uiCreateBox (int orientation)
{
  uiwcont_t *uiwidget;
  GtkWidget *box;

  box = gtk_box_new (orientation, 0);
  uiwidget = uiwcontAlloc ();
  uiwidget->wtype = WCONT_T_BOX;
  uiwidget->widget = box;
  return uiwidget;
}

