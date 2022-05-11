#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <gtk/gtk.h>

#include "log.h"
#include "ui.h"
#include "uiutils.h"

GtkWidget *
uiCreateLabel (const char *label)
{
  GtkWidget *widget;

  logProcBegin (LOG_PROC, "uiCreateLabel");

  widget = gtk_label_new (label);
  assert (widget != NULL);
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  logProcEnd (LOG_PROC, "uiCreateLabel", "");
  return widget;
}

GtkWidget *
uiCreateColonLabel (const char *label)
{
  GtkWidget *widget;
  char      tbuff [100];

  logProcBegin (LOG_PROC, "uiCreateColonLabel");

  snprintf (tbuff, sizeof (tbuff), "%s:", label);
  widget = gtk_label_new (tbuff);
  assert (widget != NULL);
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  logProcEnd (LOG_PROC, "uiCreateColonLabel", "");
  return widget;
}

inline void
uiLabelSetText (GtkWidget *widget, const char *text)
{
  gtk_label_set_text (GTK_LABEL (widget), text);
}

inline void
uiLabelEllipsizeOn (GtkWidget *widget)
{
  gtk_label_set_ellipsize (GTK_LABEL (widget), PANGO_ELLIPSIZE_END);
}

inline void
uiLabelSetMaxWidth (GtkWidget *widget, int width)
{
  gtk_label_set_max_width_chars (GTK_LABEL (widget), width);
}
