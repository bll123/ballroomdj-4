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

#include "ui/uiinternal.h"
#include "ui/uiui.h"
#include "ui/uilabel.h"

void
uiCreateLabel (uiwidget_t *uiwidget, const char *label)
{
  GtkWidget *widget;

  widget = gtk_label_new (label);
  assert (widget != NULL);
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  uiwidget->widget = widget;
}

void
uiCreateColonLabel (uiwidget_t *uiwidget, const char *label)
{
  GtkWidget *widget;
  char      tbuff [200];

  snprintf (tbuff, sizeof (tbuff), "%s:", label);
  widget = gtk_label_new (tbuff);
  assert (widget != NULL);
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_widget_set_margin_end (widget, uiBaseMarginSz * 2);
  uiwidget->widget = widget;
}

void
uiLabelAddClass (const char *classnm, const char *color)
{
  char    tbuff [100];

  snprintf (tbuff, sizeof (tbuff), "label.%s", classnm);
  uiAddColorClass (tbuff, color);
}

void
uiLabelSetFont (uiwidget_t *uiwidget, const char *font)
{
  PangoFontDescription  *font_desc;
  PangoAttribute        *attr;
  PangoAttrList         *attrlist;

  attrlist = pango_attr_list_new ();
  font_desc = pango_font_description_from_string (font);
  attr = pango_attr_font_desc_new (font_desc);
  pango_attr_list_insert (attrlist, attr);
  gtk_label_set_attributes (GTK_LABEL (uiwidget->widget), attrlist);
  pango_attr_list_unref (attrlist);
}

void
uiLabelSetText (uiwidget_t *uiwidget, const char *text)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_label_set_text (GTK_LABEL (uiwidget->widget), text);
}

const char *
uiLabelGetText (uiwidget_t *uiwidget)
{
  const char *txt;

  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return NULL;
  }

  txt = gtk_label_get_text (GTK_LABEL (uiwidget->widget));
  return txt;
}

void
uiLabelEllipsizeOn (uiwidget_t *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_label_set_ellipsize (GTK_LABEL (uiwidget->widget), PANGO_ELLIPSIZE_END);
}

void
uiLabelSetSelectable (uiwidget_t *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_label_set_selectable (GTK_LABEL (uiwidget->widget), TRUE);
}

void
uiLabelSetMaxWidth (uiwidget_t *uiwidget, int width)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_label_set_max_width_chars (GTK_LABEL (uiwidget->widget), width);
}

void
uiLabelAlignEnd (uiwidget_t *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_label_set_xalign (GTK_LABEL (uiwidget->widget), 1.0);
}

