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

void
uiCreateVertBox (UIWidget *uiwidget)
{
  GtkWidget *box;

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  uiwidget->widget = box;
}

void
uiCreateHorizBox (UIWidget *uiwidget)
{
  GtkWidget *box;

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  uiwidget->widget = box;
}

void
uiBoxPackInWindow (UIWidget *uiwindow, UIWidget *uibox)
{
  gtk_container_add (GTK_CONTAINER (uiwindow->widget), uibox->widget);
}

void
uiBoxPackStart (UIWidget *uibox, UIWidget *uiwidget)
{
  gtk_box_pack_start (GTK_BOX (uibox->widget), uiwidget->widget, FALSE, FALSE, 0);
}

void
uiBoxPackStartExpand (UIWidget *uibox, UIWidget *uiwidget)
{
  gtk_box_pack_start (GTK_BOX (uibox->widget), uiwidget->widget, TRUE, TRUE, 0);
}

void
uiBoxPackEnd (UIWidget *uibox, UIWidget *uiwidget)
{
  gtk_box_pack_end (GTK_BOX (uibox->widget), uiwidget->widget, FALSE, FALSE, 0);
}

void
uiBoxPackEndExpand (UIWidget *uibox, UIWidget *uiwidget)
{
  gtk_box_pack_end (GTK_BOX (uibox->widget), uiwidget->widget, TRUE, TRUE, 0);
}

