/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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

#include "oslocale.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uibox.h"
#include "ui/uiui.h"

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
uiBoxPackStart (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_box_pack_start (GTK_BOX (uibox->widget), uiwidget->widget, FALSE, FALSE, 0);
}

void
uiBoxPackStartExpand (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start-exp")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_box_pack_start (GTK_BOX (uibox->widget), uiwidget->widget, TRUE, TRUE, 0);
}

void
uiBoxPackEnd (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-end")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_box_pack_end (GTK_BOX (uibox->widget), uiwidget->widget, FALSE, FALSE, 0);
}

void
uiBoxPackEndExpand (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-end-exp")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }
  gtk_box_pack_end (GTK_BOX (uibox->widget), uiwidget->widget, TRUE, TRUE, 0);
}

/* internal routines */

static uiwcont_t *
uiCreateBox (int orientation)
{
  uiwcont_t *uiwidget;
  GtkWidget *box;

  box = gtk_box_new (orientation, 0);
  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_BOX;
  uiwidget->wtype = WCONT_T_BOX;
  uiwidget->widget = box;
  return uiwidget;
}

