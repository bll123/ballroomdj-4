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

GtkWidget *
uiCreateCheckButton (const char *txt, int value)
{
  GtkWidget   *widget;

  widget = gtk_check_button_new_with_label (txt);
  assert (widget != NULL);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);
  return widget;
}

GtkWidget *
uiCreateToggleButton (const char *txt, const char *imgname,
    const char *tooltiptxt, UIWidget *image, int value)
{
  GtkWidget   *widget;
  GtkWidget   *imagewidget = NULL;

  widget = gtk_toggle_button_new_with_label (txt);
  assert (widget != NULL);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);
  if (imgname != NULL) {
    imagewidget = gtk_image_new_from_file (imgname);
  }
  if (image != NULL) {
    imagewidget = image->widget;
  }
  if (imagewidget != NULL) {
    gtk_button_set_image (GTK_BUTTON (widget), imagewidget);
  }
  if (txt != NULL && (imgname != NULL || image != NULL)) {
    gtk_button_set_image_position (GTK_BUTTON (widget), GTK_POS_RIGHT);
    gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE);
  }
  if (tooltiptxt != NULL) {
    gtk_widget_set_tooltip_text (widget, tooltiptxt);
  }
  return widget;
}

void
uiToggleButtonSetImage (GtkWidget *widget, UIWidget *image)
{
  gtk_button_set_image (GTK_BUTTON (widget), image->widget);
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
