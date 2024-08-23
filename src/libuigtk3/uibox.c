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

#include "callback.h"
#include "oslocale.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uibox.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"

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
  if (uiwidget == NULL || uiwidget->uidata.packwidget == NULL) {
    return;
  }

  gtk_box_pack_start (GTK_BOX (uibox->uidata.widget), uiwidget->uidata.packwidget, FALSE, FALSE, 0);
  uiwidget->packed = true;
}

void
uiBoxPackStartExpand (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start-exp")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.packwidget == NULL) {
    return;
  }

  gtk_box_pack_start (GTK_BOX (uibox->uidata.widget), uiwidget->uidata.packwidget, TRUE, TRUE, 0);
  uiwidget->packed = true;
}

void
uiBoxPackEnd (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-end")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.packwidget == NULL) {
    return;
  }

  gtk_box_pack_end (GTK_BOX (uibox->uidata.widget), uiwidget->uidata.packwidget, FALSE, FALSE, 0);
  uiwidget->packed = true;
}

void
uiBoxPackEndExpand (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-end-exp")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.packwidget == NULL) {
    return;
  }
  gtk_box_pack_end (GTK_BOX (uibox->uidata.widget), uiwidget->uidata.packwidget, TRUE, TRUE, 0);
  uiwidget->packed = true;
}

void
uiBoxSetSizeChgCallback (uiwcont_t *uiwindow, callback_t *uicb)
{
  if (! uiwcontValid (uiwindow, WCONT_T_BOX, "box-set-size-chg-cb")) {
    return;
  }

  uiWidgetSetSizeChgCallback (uiwindow, uicb);
}

/* internal routines */

static uiwcont_t *
uiCreateBox (int orientation)
{
  uiwcont_t *uiwidget = NULL;
  GtkWidget *box;

  box = gtk_box_new (orientation, 0);
  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    uiwidget = uiwcontAlloc (WCONT_T_BOX, WCONT_T_HBOX);
  }
  if (orientation == GTK_ORIENTATION_VERTICAL) {
    uiwidget = uiwcontAlloc (WCONT_T_BOX, WCONT_T_VBOX);
  }
  uiwcontSetWidget (uiwidget, box, NULL);
//  uiwidget->uidata.widget = box;
//  uiwidget->uidata.packwidget = box;
  return uiwidget;
}

