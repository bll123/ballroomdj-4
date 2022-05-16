#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <gtk/gtk.h>

#include "ui.h"
#include "uiutils.h"

void
uiCreateLabel (UIWidget *uiwidget, const char *label)
{
  GtkWidget *widget;

  widget = uiCreateLabelW (label);
  uiwidget->widget = widget;
}

void
uiCreateColonLabel (UIWidget *uiwidget, const char *label)
{
  GtkWidget *widget;

  widget = uiCreateColonLabelW (label);
  uiwidget->widget = widget;
}

void
uiLabelSetColor (UIWidget *uiwidget, const char *color)
{
  char  tbuff [200];

  snprintf (tbuff, sizeof (tbuff),
      "label { color: %s; }", color);
  uiSetCss (uiwidget->widget, tbuff);
}

void
uiLabelSetFont (UIWidget *uiwidget, const char *font)
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


inline void
uiLabelSetText (UIWidget *uiwidget, const char *text)
{
  gtk_label_set_text (GTK_LABEL (uiwidget->widget), text);
}

inline void
uiLabelEllipsizeOn (UIWidget *uiwidget)
{
  gtk_label_set_ellipsize (GTK_LABEL (uiwidget->widget), PANGO_ELLIPSIZE_END);
}

inline void
uiLabelSetMaxWidth (UIWidget *uiwidget, int width)
{
  gtk_label_set_max_width_chars (GTK_LABEL (uiwidget->widget), width);
}

inline void
uiLabelAlignEnd (UIWidget *uiwidget)
{
  gtk_label_set_xalign (GTK_LABEL (uiwidget->widget), 1.0);
}

/* these routines will be removed at a later date */

GtkWidget *
uiCreateLabelW (const char *label)
{
  GtkWidget *widget;

  widget = gtk_label_new (label);
  assert (widget != NULL);
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  return widget;
}

GtkWidget *
uiCreateColonLabelW (const char *label)
{
  GtkWidget *widget;
  char      tbuff [100];

  snprintf (tbuff, sizeof (tbuff), "%s:", label);
  widget = gtk_label_new (tbuff);
  assert (widget != NULL);
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  return widget;
}

inline void
uiLabelSetTextW (GtkWidget *widget, const char *text)
{
  gtk_label_set_text (GTK_LABEL (widget), text);
}

inline void
uiLabelEllipsizeOnW (GtkWidget *widget)
{
  gtk_label_set_ellipsize (GTK_LABEL (widget), PANGO_ELLIPSIZE_END);
}

inline void
uiLabelSetMaxWidthW (GtkWidget *widget, int width)
{
  gtk_label_set_max_width_chars (GTK_LABEL (widget), width);
}
