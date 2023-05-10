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
#include "ui/uilabel.h"

uiwcont_t *
uiCreateLabel (const char *label)
{
  uiwcont_t *uiwidget;
  GtkWidget *widget;

  widget = gtk_label_new (label);
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);

  uiwidget = uiwcontAlloc ();
  uiwidget->wtype = WCONT_T_LABEL;
  uiwidget->widget = widget;
  return uiwidget;
}

uiwcont_t *
uiCreateColonLabel (const char *label)
{
  char      tbuff [300];

  snprintf (tbuff, sizeof (tbuff), "%s:", label);
  return uiCreateLabel (tbuff);
}

void
uiLabelAddClass (const char *classnm, const char *color)
{
  char    tbuff [100];

  snprintf (tbuff, sizeof (tbuff), "label.%s", classnm);
  uiAddColorClass (tbuff, color);
}

void
uiLabelSetFont (uiwcont_t *uiwidget, const char *font)
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
uiLabelSetText (uiwcont_t *uiwidget, const char *text)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_label_set_text (GTK_LABEL (uiwidget->widget), text);
}

const char *
uiLabelGetText (uiwcont_t *uiwidget)
{
  const char *txt;

  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return NULL;
  }

  txt = gtk_label_get_text (GTK_LABEL (uiwidget->widget));
  return txt;
}

void
uiLabelEllipsizeOn (uiwcont_t *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_label_set_ellipsize (GTK_LABEL (uiwidget->widget), PANGO_ELLIPSIZE_END);
}

void
uiLabelWrapOn (uiwcont_t *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_label_set_line_wrap (GTK_LABEL (uiwidget->widget), TRUE);
}

void
uiLabelSetSelectable (uiwcont_t *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_label_set_selectable (GTK_LABEL (uiwidget->widget), TRUE);
}

void
uiLabelSetMaxWidth (uiwcont_t *uiwidget, int width)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_label_set_max_width_chars (GTK_LABEL (uiwidget->widget), width);
}

void
uiLabelAlignEnd (uiwcont_t *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_label_set_xalign (GTK_LABEL (uiwidget->widget), 1.0);
}

