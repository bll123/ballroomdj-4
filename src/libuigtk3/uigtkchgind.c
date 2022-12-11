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

#include <gtk/gtk.h>

#include "ui.h"

typedef struct uichgind {
  UIWidget    label;
} uichgind_t;

uichgind_t *
uiCreateChangeIndicator (UIWidget *boxp)
{
  uichgind_t  *uichgind;
  GtkWidget   *widget;

  uichgind = malloc (sizeof (uichgind_t));

  widget = gtk_label_new ("");
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_widget_set_margin_end (widget, uiBaseMarginSz);

  uichgind->label.widget = widget;
  uiBoxPackStart (boxp, &uichgind->label);
  return uichgind;
}

void
uichgindFree (uichgind_t *uichgind)
{
  if (uichgind != NULL) {
    free (uichgind);
  }
}

void
uichgindMarkNormal (uichgind_t *uichgind)
{
  uiSetCss (uichgind->label.widget,
      "label { border-left-style: solid; border-left-width: 2px; "
      " border-left-color: transparent; }");
}

void
uichgindMarkError (uichgind_t *uichgind)
{
  uiSetCss (uichgind->label.widget,
      "label { border-left-style: solid; border-left-width: 2px; "
      " border-left-color: #ff1111; }");
}

void
uichgindMarkChanged (uichgind_t *uichgind)
{
  uiSetCss (uichgind->label.widget,
      "label { border-left-style: solid; border-left-width: 2px; "
      " border-left-color: #11ff11; }");
}
