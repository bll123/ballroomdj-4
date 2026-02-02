/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "callback.h"
#include "mdebug.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uispecific.h"
#include "ui/uitoggle.h"

typedef struct uitoggle {
  callback_t      *cb;
  GtkWidget       *currimage;
  GtkWidget       *image;
  GdkPixbuf       *imageraw;
  GtkWidget       *altimage;
  GdkPixbuf       *altimageraw;
} uitoggle_t;

static void uiToggleButtonToggleHandler (GtkButton *b, gpointer udata);
static void uiToggleButtonSetImageAlignment (GtkWidget *widget, void *udata);
static void uiToggleButtonSIACallback (GtkWidget *widget, void *udata);
static gboolean uiToggleButtonFocusHandler (GtkWidget* w, GdkEventFocus *event, gpointer udata);
static void uiToggleButtonSetImage (uiwcont_t *uiwidget);
static uitoggle_t * uiToggleButtonInit (void);

uiwcont_t *
uiCreateCheckButton (const char *txt, int value)
{
  uiwcont_t   *uiwidget;
  GtkWidget   *widget;
  uitoggle_t  *uitoggle;

  uitoggle = uiToggleButtonInit ();

  widget = gtk_check_button_new_with_label (txt);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);

  uiwidget = uiwcontAlloc (WCONT_T_BUTTON_TOGGLE, WCONT_T_BUTTON_CHKBOX);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwidget->uiint.uitoggle = uitoggle;
  return uiwidget;
}

uiwcont_t *
uiCreateRadioButton (uiwcont_t *widgetgrp, const char *txt, int value)
{
  uiwcont_t   *uiwidget = NULL;
  GtkWidget   *widget = NULL;
  GtkWidget   *gwidget = NULL;
  uitoggle_t  *uitoggle;

  uitoggle = uiToggleButtonInit ();

  if (widgetgrp != NULL) {
    gwidget = widgetgrp->uidata.widget;
  }
  widget = gtk_radio_button_new_with_label_from_widget (
      GTK_RADIO_BUTTON (gwidget), txt);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);
  uiwidget = uiwcontAlloc (WCONT_T_BUTTON_TOGGLE, WCONT_T_BUTTON_RADIO);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwidget->uiint.uitoggle = uitoggle;
  return uiwidget;
}

uiwcont_t *
uiCreateToggleButton (const char *txt,
    const char *imagenm, const char *tooltiptxt, int value)
{
  uiwcont_t   *uiwidget;
  GtkWidget   *widget;
  uitoggle_t  *uitoggle;

  uitoggle = uiToggleButtonInit ();

  widget = gtk_toggle_button_new_with_label (txt);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);

  if (imagenm != NULL) {
    GtkWidget   *image;

    image = uiImageWidget (imagenm);
    uitoggle->image = image;
    if (image != NULL) {
      uitoggle->imageraw = gtk_image_get_pixbuf (GTK_IMAGE (image));
    }

    image = gtk_image_new ();
    gtk_button_set_image (GTK_BUTTON (widget), image);
    gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE); // macos
    gtk_widget_set_halign (image, GTK_ALIGN_CENTER);
    gtk_widget_set_valign (image, GTK_ALIGN_CENTER);
    uitoggle->currimage = image;
  }

  if (txt != NULL && imagenm != NULL) {
    gtk_button_set_image_position (GTK_BUTTON (widget), GTK_POS_RIGHT);
    gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE);
  }

  if (tooltiptxt != NULL) {
    gtk_widget_set_tooltip_text (widget, tooltiptxt);
  }

  if (imagenm != NULL) {
    int   pad;

    pad = txt != NULL;
    uiToggleButtonSetImageAlignment (widget, &pad);
  }

  uiwidget = uiwcontAlloc (WCONT_T_BUTTON_TOGGLE, WCONT_T_BUTTON_TOGGLE);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwidget->uiint.uitoggle = uitoggle;

  if (imagenm != NULL) {
    uiToggleButtonSetImage (uiwidget);
  }

  return uiwidget;
}

void
uiToggleButtonFree (uiwcont_t *uiwidget)
{
  uitoggle_t      *uitoggle;

  uitoggle = uiwidget->uiint.uitoggle;
  mdfree (uitoggle);
  uiwidget->uiint.uitoggle = NULL;
}

void
uiToggleButtonSetAltImage (uiwcont_t *uiwidget, const char *imagenm)
{
  uitoggle_t      *uitoggle;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON_TOGGLE, "toggle-set-alt-image")) {
    return;
  }

  uitoggle = uiwidget->uiint.uitoggle;

  if (imagenm != NULL) {
    GtkWidget   *image;

    image = uiImageWidget (imagenm);
    uitoggle->altimage = image;
    if (image != NULL) {
      uitoggle->altimageraw = gtk_image_get_pixbuf (GTK_IMAGE (image));
    }
  }

  uiToggleButtonSetImage (uiwidget);
}

void
uiToggleButtonSetCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  uitoggle_t    *uitoggle;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON_TOGGLE, "tb-set-cb")) {
    return;
  }

  uitoggle = uiwidget->uiint.uitoggle;
  if (uitoggle == NULL) {
    return;
  }

  uitoggle->cb = uicb;

  g_signal_connect (uiwidget->uidata.widget, "toggled",
      G_CALLBACK (uiToggleButtonToggleHandler), uiwidget);
  uiToggleButtonSetImage (uiwidget);
}

void
uiToggleButtonSetFocusCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON_TOGGLE, "tb-set-cb")) {
    return;
  }

  g_signal_connect_after (uiwidget->uidata.widget, "focus-in-event",
      G_CALLBACK (uiToggleButtonFocusHandler), uicb);
}

void
uiToggleButtonSetText (uiwcont_t *uiwidget, const char *txt)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON_TOGGLE, "tb-set-txt")) {
    return;
  }

  gtk_button_set_label (GTK_BUTTON (uiwidget->uidata.widget), txt);
}

bool
uiToggleButtonIsActive (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON_TOGGLE, "tb-is-active")) {
    return 0;
  }

  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (uiwidget->uidata.widget));
}

void
uiToggleButtonSetValue (uiwcont_t *uiwidget, int state)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON_TOGGLE, "tb-set-state")) {
    return;
  }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (uiwidget->uidata.widget), state);
  uiToggleButtonSetImage (uiwidget);
}

/* gtk appears to re-allocate the radio button label, */
/* so set the ellipsize on after setting the text value */
void
uiToggleButtonEllipsize (uiwcont_t *uiwidget)
{
  GtkWidget *widget;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON_TOGGLE, "tb-ellipsize")) {
    return;
  }

  /* this seems to work for radio buttons. */
  /* for other buttons, a recursive child descent like the image alignment */
  /* may be necessary */
  widget = gtk_bin_get_child (GTK_BIN (uiwidget->uidata.widget));
  if (GTK_IS_LABEL (widget)) {
    gtk_label_set_ellipsize (GTK_LABEL (widget), PANGO_ELLIPSIZE_END);
  }
}

/* internal routines */

static void
uiToggleButtonToggleHandler (GtkButton *b, gpointer udata)
{
  uiwcont_t   *uiwidget = udata;
  uitoggle_t  *uitoggle;
  callback_t  *uicb;

  uitoggle = uiwidget->uiint.uitoggle;
  if (uitoggle == NULL) {
    return;
  }

  uicb = uitoggle->cb;
  if (uicb != NULL) {
    callbackHandler (uicb);
  }
  uiToggleButtonSetImage (uiwidget);
}

/* to find the image and center it vertically, it is necessary */
/* to traverse the child tree of the toggle button */

static void
uiToggleButtonSetImageAlignment (GtkWidget *widget, void *udata)
{
  gtk_container_foreach (GTK_CONTAINER (widget), uiToggleButtonSIACallback, udata);
}

static void
uiToggleButtonSIACallback (GtkWidget *widget, void *udata)
{
  const int   *pad = udata;

  if (GTK_IS_IMAGE (widget)) {
    gtk_widget_set_valign (widget, GTK_ALIGN_CENTER);
    if (*pad) {
      gtk_widget_set_margin_start (widget, 2);
    }
    return;
  }
  if (GTK_IS_CONTAINER (widget)) {
    uiToggleButtonSetImageAlignment (widget, udata);
  }
}

static gboolean
uiToggleButtonFocusHandler (GtkWidget* w, GdkEventFocus *event, gpointer udata)
{
  callback_t    *uicb = udata;

  if (uicb != NULL) {
    callbackHandler (uicb);
  }

  return false;
}

static void
uiToggleButtonSetImage (uiwcont_t *uiwidget)
{
  GtkWidget   *widget;
  uitoggle_t  *uitoggle;
  int         active;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON_TOGGLE, "tb-set-img")) {
    return;
  }

  widget = uiwidget->uidata.widget;
  uitoggle = uiwidget->uiint.uitoggle;
  if (uitoggle == NULL) {
    return;
  }


  if (uitoggle->currimage == NULL) {
    return;
  }

  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  gtk_image_clear (GTK_IMAGE (uitoggle->currimage));
  if ((! active || uitoggle->altimageraw == NULL) &&
      uitoggle->imageraw != NULL) {
    gtk_image_set_from_pixbuf (GTK_IMAGE (uitoggle->currimage), uitoggle->imageraw);
  }
  if (active && uitoggle->altimageraw != NULL) {
    gtk_image_set_from_pixbuf (GTK_IMAGE (uitoggle->currimage), uitoggle->altimageraw);
  }
  gtk_widget_show_all (uiwidget->uidata.widget);
}

static uitoggle_t *
uiToggleButtonInit (void)
{
  uitoggle_t    *uitoggle;

  uitoggle = mdmalloc (sizeof (uitoggle_t));
  uitoggle->currimage = NULL;
  uitoggle->image = NULL;
  uitoggle->imageraw = NULL;
  uitoggle->altimage = NULL;
  uitoggle->altimageraw = NULL;

  return uitoggle;
}
