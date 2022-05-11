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
uiCreateCheckButton (const char *txt, int value)
{
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "uiCreateCheckButton");

  widget = gtk_check_button_new_with_label (txt);
  assert (widget != NULL);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);
  logProcEnd (LOG_PROC, "uiCreateCheckButton", "");
  return widget;
}

GtkWidget *
uiCreateToggleButton (const char *txt, const char *imgname,
    const char *tooltiptxt, GtkWidget *image, int value)
{
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "uiCreateToggleButton");

  widget = gtk_toggle_button_new_with_label (txt);
  assert (widget != NULL);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);
  if (imgname != NULL) {
    image = gtk_image_new_from_file (imgname);
  }
  if (image != NULL) {
    gtk_button_set_image (GTK_BUTTON (widget), image);
  }
  if (txt != NULL && (imgname != NULL || image != NULL)) {
    gtk_button_set_image_position (GTK_BUTTON (widget), GTK_POS_RIGHT);
    gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE);
  }
  if (tooltiptxt != NULL) {
    gtk_widget_set_tooltip_text (widget, tooltiptxt);
  }
  logProcEnd (LOG_PROC, "uiCreateToggleButton", "");
  return widget;
}

void
uiToggleButtonSetImage (GtkWidget *widget, GtkWidget *image)
{
  gtk_button_set_image (GTK_BUTTON (widget), image);
}

bool
uiToggleButtonIsActive (GtkWidget *widget)
{
  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
}

void
uiToggleButtonSetState (GtkWidget *widget, int state)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), state);
}
