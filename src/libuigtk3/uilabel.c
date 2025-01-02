/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "uiwcont.h"

#include "ui-gtk3.h"

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

  uiwidget = uiwcontAlloc (WCONT_T_LABEL, WCONT_T_LABEL);
  uiwcontSetWidget (uiwidget, widget, NULL);
//  uiwidget->uidata.widget = widget;
//  uiwidget->uidata.packwidget = widget;
  return uiwidget;
}

void
uiLabelAddClass (const char *classnm, const char *color)
{
  char    tbuff [100];

  if (classnm == NULL || color == NULL) {
    return;
  }

  snprintf (tbuff, sizeof (tbuff), "label.%s", classnm);
  uiAddColorClass (tbuff, color);
}

void
uiLabelSetTooltip (uiwcont_t *uiwidget, const char *txt)
{
  if (! uiwcontValid (uiwidget, WCONT_T_LABEL, "label-set-tooltip")) {
    return;
  }

  gtk_widget_set_tooltip_text (uiwidget->uidata.widget, txt);
}

void
uiLabelSetFont (uiwcont_t *uiwidget, const char *font)
{
  PangoFontDescription  *font_desc;
  PangoAttribute        *attr;
  PangoAttrList         *attrlist;

  if (! uiwcontValid (uiwidget, WCONT_T_LABEL, "label-set-font")) {
    return;
  }

  attrlist = pango_attr_list_new ();
  font_desc = pango_font_description_from_string (font);
  attr = pango_attr_font_desc_new (font_desc);
  pango_attr_list_insert (attrlist, attr);
  gtk_label_set_attributes (GTK_LABEL (uiwidget->uidata.widget), attrlist);
  pango_attr_list_unref (attrlist);
}

void
uiLabelSetText (uiwcont_t *uiwidget, const char *text)
{
  if (! uiwcontValid (uiwidget, WCONT_T_LABEL, "label-set-text")) {
    return;
  }

  gtk_label_set_text (GTK_LABEL (uiwidget->uidata.widget), text);
}

const char *
uiLabelGetText (uiwcont_t *uiwidget)
{
  const char *txt;

  if (! uiwcontValid (uiwidget, WCONT_T_LABEL, "label-get-text")) {
    return NULL;
  }

  txt = gtk_label_get_text (GTK_LABEL (uiwidget->uidata.widget));
  return txt;
}

void
uiLabelEllipsizeOn (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_LABEL, "label-ell-on")) {
    return;
  }

  gtk_label_set_ellipsize (GTK_LABEL (uiwidget->uidata.widget), PANGO_ELLIPSIZE_END);
}

void
uiLabelWrapOn (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_LABEL, "label-wrap-on")) {
    return;
  }

  gtk_label_set_line_wrap (GTK_LABEL (uiwidget->uidata.widget), TRUE);
}

void
uiLabelSetSelectable (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_LABEL, "label-set-sel")) {
    return;
  }

  gtk_label_set_selectable (GTK_LABEL (uiwidget->uidata.widget), TRUE);
}

void
uiLabelSetMinWidth (uiwcont_t *uiwidget, int width)
{
  if (! uiwcontValid (uiwidget, WCONT_T_LABEL, "label-set-minw")) {
    return;
  }

  gtk_label_set_width_chars (GTK_LABEL (uiwidget->uidata.widget), width);
}

void
uiLabelSetMaxWidth (uiwcont_t *uiwidget, int width)
{
  if (! uiwcontValid (uiwidget, WCONT_T_LABEL, "label-set-maxw")) {
    return;
  }

  gtk_label_set_max_width_chars (GTK_LABEL (uiwidget->uidata.widget), width);
}

void
uiLabelAlignEnd (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_LABEL, "label-align-end")) {
    return;
  }

  gtk_label_set_xalign (GTK_LABEL (uiwidget->uidata.widget), 1.0);
}

