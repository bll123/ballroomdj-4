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

static void uiToggleButtonToggleHandler (GtkButton *b, gpointer udata);

void
uiCreateCheckButton (UIWidget *uiwidget, const char *txt, int value)
{
  GtkWidget   *widget;

  widget = gtk_check_button_new_with_label (txt);
  assert (widget != NULL);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);
  uiwidget->widget = widget;
}

void
uiCreateRadioButton (UIWidget *uiwidget, UIWidget *widgetgrp,
    const char *txt, int value)
{
  GtkWidget   *widget;
  GtkWidget   *gwidget = NULL;

  if (widgetgrp != NULL) {
    gwidget = widgetgrp->widget;
  }
  widget = gtk_radio_button_new_with_label_from_widget (
      GTK_RADIO_BUTTON (gwidget), txt);
  assert (widget != NULL);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);
  uiwidget->widget = widget;
}

void
uiCreateToggleButton (UIWidget *uiwidget, const char *txt,
    const char *imgname, const char *tooltiptxt, UIWidget *image, int value)
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
  uiwidget->widget = widget;
}

void
uiToggleButtonSetCallback (UIWidget *uiwidget, UICallback *uicb)
{
  g_signal_connect (uiwidget->widget, "toggled",
      G_CALLBACK (uiToggleButtonToggleHandler), uicb);
}

void
uiToggleButtonSetImage (UIWidget *uiwidget, UIWidget *image)
{
  gtk_button_set_image (GTK_BUTTON (uiwidget->widget), image->widget);
}

bool
uiToggleButtonIsActive (UIWidget *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return 0;
  }
  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (uiwidget->widget));
}

void
uiToggleButtonSetState (UIWidget *uiwidget, int state)
{
  if (uiwidget->widget == NULL) {
    return;
  }
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (uiwidget->widget), state);
}

/* internal routines */

static inline void
uiToggleButtonToggleHandler (GtkButton *b, gpointer udata)
{
  UICallback *uicb = udata;

  uiutilsCallbackHandler (uicb);
}

