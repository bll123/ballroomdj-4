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

#include "callback.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uitoggle.h"

static void uiToggleButtonToggleHandler (GtkButton *b, gpointer udata);
static void uiToggleButtonSetImageAlignment (GtkWidget *widget);
static void uiToggleButtonSIACallback (GtkWidget *widget, void *udata);

uiwcont_t *
uiCreateCheckButton (const char *txt, int value)
{
  uiwcont_t   *uiwidget;
  GtkWidget   *widget;

  widget = gtk_check_button_new_with_label (txt);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);
  uiwidget = uiwcontAlloc ();
  uiwidget->wtype = WCONT_T_CHECK_BOX;
  uiwidget->widget = widget;
  return uiwidget;
}

uiwcont_t *
uiCreateRadioButton (uiwcont_t *widgetgrp, const char *txt, int value)
{
  uiwcont_t   *uiwidget = NULL;
  GtkWidget   *widget = NULL;
  GtkWidget   *gwidget = NULL;

  if (widgetgrp != NULL) {
    gwidget = widgetgrp->widget;
  }
  widget = gtk_radio_button_new_with_label_from_widget (
      GTK_RADIO_BUTTON (gwidget), txt);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);
  uiwidget = uiwcontAlloc ();
  uiwidget->wtype = WCONT_T_RADIO_BUTTON;
  uiwidget->widget = widget;
  return uiwidget;
}

uiwcont_t *
uiCreateToggleButton (const char *txt,
    const char *imgname, const char *tooltiptxt, uiwcont_t *image, int value)
{
  uiwcont_t   *uiwidget;
  GtkWidget   *widget;
  GtkWidget   *imagewidget = NULL;

  widget = gtk_toggle_button_new_with_label (txt);
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
  if (imagewidget != NULL) {
    uiToggleButtonSetImageAlignment (widget);
  }

  uiwidget = uiwcontAlloc ();
  uiwidget->wtype = WCONT_T_TOGGLE_BUTTON;
  uiwidget->widget = widget;
  return uiwidget;
}

void
uiToggleButtonSetCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }
  if (uiwidget->wtype != WCONT_T_TOGGLE_BUTTON &&
      uiwidget->wtype != WCONT_T_RADIO_BUTTON &&
      uiwidget->wtype != WCONT_T_CHECK_BOX) {
    return;
  }

  g_signal_connect (uiwidget->widget, "toggled",
      G_CALLBACK (uiToggleButtonToggleHandler), uicb);
}

void
uiToggleButtonSetImage (uiwcont_t *uiwidget, uiwcont_t *image)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }
  if (uiwidget->wtype != WCONT_T_TOGGLE_BUTTON) {
    return;
  }

  gtk_button_set_image (GTK_BUTTON (uiwidget->widget), image->widget);
}

void
uiToggleButtonSetText (uiwcont_t *uiwidget, const char *txt)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }
  if (uiwidget->wtype != WCONT_T_TOGGLE_BUTTON) {
    return;
  }

  gtk_button_set_label (GTK_BUTTON (uiwidget->widget), txt);
}

bool
uiToggleButtonIsActive (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return 0;
  }
  if (uiwidget->wtype != WCONT_T_TOGGLE_BUTTON &&
      uiwidget->wtype != WCONT_T_RADIO_BUTTON &&
      uiwidget->wtype != WCONT_T_CHECK_BOX) {
    return 0;
  }

  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (uiwidget->widget));
}

void
uiToggleButtonSetState (uiwcont_t *uiwidget, int state)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }
  if (uiwidget->wtype != WCONT_T_TOGGLE_BUTTON &&
      uiwidget->wtype != WCONT_T_RADIO_BUTTON &&
      uiwidget->wtype != WCONT_T_CHECK_BOX) {
    return;
  }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (uiwidget->widget), state);
}

/* gtk appears to re-allocate the radio button label, */
/* so set the ellipsize on after setting the text value */
void
uiToggleButtonEllipsize (uiwcont_t *uiwidget)
{
  GtkWidget *widget;

  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }
  if (uiwidget->wtype != WCONT_T_TOGGLE_BUTTON &&
      uiwidget->wtype != WCONT_T_RADIO_BUTTON &&
      uiwidget->wtype != WCONT_T_CHECK_BOX) {
    return;
  }

  widget = gtk_bin_get_child (GTK_BIN (uiwidget->widget));
  if (GTK_IS_LABEL (widget)) {
    gtk_label_set_ellipsize (GTK_LABEL (widget), PANGO_ELLIPSIZE_END);
  }
}

/* internal routines */

static void
uiToggleButtonToggleHandler (GtkButton *b, gpointer udata)
{
  callback_t *uicb = udata;

  if (uicb != NULL) {
    callbackHandler (uicb);
  }
}

/* to find the image and center it vertically, it is necessary */
/* to traverse the child tree of the toggle button */

static void
uiToggleButtonSetImageAlignment (GtkWidget *widget)
{
  gtk_container_foreach (GTK_CONTAINER (widget), uiToggleButtonSIACallback, NULL);
}

static void
uiToggleButtonSIACallback (GtkWidget *widget, void *udata)
{
  if (GTK_IS_IMAGE (widget)) {
    gtk_widget_set_valign (widget, GTK_ALIGN_CENTER);
    return;
  }
  if (GTK_IS_CONTAINER (widget)) {
    uiToggleButtonSetImageAlignment (widget);
  }
}

