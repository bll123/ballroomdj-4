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

#include "ui/uiinternal.h"
#include "ui/uibox.h"

void
uiCreateVertBox (uiwidget_t *uiwidget)
{
  GtkWidget *box;

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  uiwidget->widget = box;
}

void
uiCreateHorizBox (uiwidget_t *uiwidget)
{
  GtkWidget *box;

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  uiwidget->widget = box;
}

void
uiBoxPackInWindow (uiwidget_t *uiwindow, uiwidget_t *uibox)
{
  gtk_container_add (GTK_CONTAINER (uiwindow->widget), uibox->widget);
}

void
uiBoxPackStart (uiwidget_t *uibox, uiwidget_t *uiwidget)
{
  gtk_box_pack_start (GTK_BOX (uibox->widget), uiwidget->widget, FALSE, FALSE, 0);
}

void
uiBoxPackStartExpand (uiwidget_t *uibox, uiwidget_t *uiwidget)
{
  gtk_box_pack_start (GTK_BOX (uibox->widget), uiwidget->widget, TRUE, TRUE, 0);
}

void
uiBoxPackEnd (uiwidget_t *uibox, uiwidget_t *uiwidget)
{
  gtk_box_pack_end (GTK_BOX (uibox->widget), uiwidget->widget, FALSE, FALSE, 0);
}

void
uiBoxPackEndExpand (uiwidget_t *uibox, uiwidget_t *uiwidget)
{
  gtk_box_pack_end (GTK_BOX (uibox->widget), uiwidget->widget, TRUE, TRUE, 0);
}

