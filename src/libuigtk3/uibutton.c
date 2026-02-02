/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "callback.h"
#include "log.h"
#include "mdebug.h"
#include "pathbld.h"
#include "tmutil.h"
#include "uiclass.h"
#include "uigeneral.h"
#include "uiwcont.h"

#include "ui-gtk3.h"

#include "ui/uiwcont-int.h"

#include "ui/uibutton.h"
#include "ui/uiui.h"
#include "ui/uispecific.h"
#include "ui/uiwidget.h"

static void uiButtonSignalHandler (GtkButton *b, gpointer udata);
static void uiButtonRepeatSignalHandler (GtkButton *b, gpointer udata);

typedef struct uibutton {
  GtkWidget       *currimage;
  GtkWidget       *image;
  GdkPixbuf       *imageraw;
  GtkWidget       *altimage;
  GdkPixbuf       *altimageraw;
  uibuttonstate_t state;
} uibutton_t;

uiwcont_t *
uiCreateButton (callback_t *uicb, const char *txt,
    const char *imagenm, const char *tooltiptxt)
{
  uiwcont_t       *uiwidget;
  uibutton_t      *uibutton;
  uibuttonbase_t  *bbase;
  GtkWidget       *widget;

  uibutton = mdmalloc (sizeof (uibutton_t));
  uibutton->image = NULL;
  uibutton->altimage = NULL;
  uibutton->state = BUTTON_OFF;

  widget = gtk_button_new ();
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);

  gtk_button_set_label (GTK_BUTTON (widget), "");
  if (txt != NULL) {
    gtk_button_set_label (GTK_BUTTON (widget), txt);
  }
  if (tooltiptxt != NULL) {
    gtk_widget_set_tooltip_text (widget, tooltiptxt);
  }

  if (imagenm != NULL) {
    GtkWidget   *image;

    image = uiImageWidget (imagenm);
    uibutton->image = image;
    if (image != NULL) {
      uibutton->imageraw = gtk_image_get_pixbuf (GTK_IMAGE (image));
    }

    image = gtk_image_new ();
    gtk_button_set_image (GTK_BUTTON (widget), image);
    gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE); // macos
    gtk_widget_set_halign (image, GTK_ALIGN_CENTER);
    gtk_widget_set_valign (image, GTK_ALIGN_CENTER);
    uibutton->currimage = image;
  }

  uiwidget = uiwcontAlloc (WCONT_T_BUTTON, WCONT_T_BUTTON);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwidget->uiint.uibutton = uibutton;

  bbase = &uiwidget->uiint.uibuttonbase;
  bbase->cb = uicb;
  bbase->presscb = callbackInit (uiButtonPressCallback,
      uiwidget, "button-repeat-press");
  bbase->releasecb = callbackInit (uiButtonReleaseCallback,
      uiwidget, "button-repeat-release");
  bbase->repeating = false;
  bbase->repeatOn = false;
  bbase->repeatMS = 250;

  if (uicb != NULL) {
    g_signal_connect (widget, "clicked",
        G_CALLBACK (uiButtonSignalHandler), uiwidget);
  }

  if (imagenm != NULL) {
    uibutton->state = BUTTON_ON;    // force set of image
    uiButtonSetState (uiwidget, BUTTON_OFF);
  }

  if (txt != NULL && imagenm != NULL) {
    gtk_button_set_image_position (GTK_BUTTON (widget), GTK_POS_RIGHT);
    gtk_widget_set_margin_start (uibutton->currimage, 2);
  }

  return uiwidget;
}

void
uiButtonFree (uiwcont_t *uiwidget)
{
  uibutton_t      *uibutton;
  uibuttonbase_t  *bbase;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-free")) {
    return;
  }

  uibutton = uiwidget->uiint.uibutton;
  bbase = &uiwidget->uiint.uibuttonbase;

  callbackFree (bbase->presscb);
  callbackFree (bbase->releasecb);

  mdfree (uibutton);
}

void
uiButtonSetImageMarginTop (uiwcont_t *uiwidget, int margin)
{
  uibutton_t    *uibutton;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-image-margin-top")) {
    return;
  }

  uibutton = uiwidget->uiint.uibutton;
  if (uibutton->currimage != NULL) {
    gtk_widget_set_margin_top (uibutton->currimage, uiBaseMarginSz * margin);
  }
}

void
uiButtonSetImageIcon (uiwcont_t *uiwidget, const char *nm)
{
  GtkWidget *image;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-image-icon")) {
    return;
  }

  image = gtk_image_new_from_icon_name (nm, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (uiwidget->uidata.widget), image);
  gtk_button_set_always_show_image (
      GTK_BUTTON (uiwidget->uidata.widget), TRUE);
}

void
uiButtonSetAltImage (uiwcont_t *uiwidget, const char *imagenm)
{
  uibutton_t      *uibutton;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-image-pos-r")) {
    return;
  }

  uibutton = uiwidget->uiint.uibutton;

  if (imagenm != NULL) {
    GtkWidget   *image;

    image = uiImageWidget (imagenm);
    uibutton->altimage = image;
    if (image != NULL) {
      uibutton->altimageraw = gtk_image_get_pixbuf (GTK_IMAGE (image));
    }
  }
}

void
uiButtonSetState (uiwcont_t *uiwidget, uibuttonstate_t newstate)
{
  uibutton_t      *uibutton;
  GtkWidget       *widget;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-alt-image")) {
    return;
  }

  uibutton = uiwidget->uiint.uibutton;
  if (uibutton->currimage == NULL) {
    return;
  }
  if (uibutton->image == NULL) {
    return;
  }
  if (uibutton->state == newstate) {
    return;
  }

  uibutton->state = newstate;
  widget = uiwidget->uidata.widget;

  gtk_image_clear (GTK_IMAGE (uibutton->currimage));
  if (uibutton->state == BUTTON_OFF && uibutton->imageraw != NULL) {
    gtk_image_set_from_pixbuf (GTK_IMAGE (uibutton->currimage), uibutton->imageraw);
  }
  if (uibutton->state == BUTTON_ON && uibutton->altimageraw != NULL) {
    gtk_image_set_from_pixbuf (GTK_IMAGE (uibutton->currimage), uibutton->altimageraw);
  }
  gtk_widget_show_all (widget);
}

void
uiButtonAlignLeft (uiwcont_t *uiwidget)
{
  GtkWidget *widget;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-align-left")) {
    return;
  }

  /* a regular button is: */
  /*    button / label  */
  /* a button w/image is: */
  /*    button / alignment / box / label, image (or image, label) */
  /* widget is either a label or an alignment */
  widget = gtk_bin_get_child (GTK_BIN (uiwidget->uidata.widget));
  if (GTK_IS_LABEL (widget)) {
    gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  } else {
    /* this does not seem to work... <sad face> */
    /* there's no way to get the hbox in the alignment to fill the space either */
    gtk_widget_set_halign (widget, GTK_ALIGN_START);
    gtk_widget_set_hexpand (widget, TRUE);
  }
}

void
uiButtonSetReliefNone (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-relief-none")) {
    return;
  }

  gtk_button_set_relief (GTK_BUTTON (uiwidget->uidata.widget), GTK_RELIEF_NONE);
}

void
uiButtonSetFlat (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-flat")) {
    return;
  }

  uiWidgetAddClass (uiwidget, FLATBUTTON_CLASS);
}

void
uiButtonSetText (uiwcont_t *uiwidget, const char *txt)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-text")) {
    return;
  }

  gtk_button_set_label (GTK_BUTTON (uiwidget->uidata.widget), txt);
}

void
uiButtonSetRepeat (uiwcont_t *uiwidget, int repeatms)
{
  uibuttonbase_t    *bbase;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-repeat")) {
    return;
  }

  bbase = &uiwidget->uiint.uibuttonbase;

  bbase->repeatMS = repeatms;
  bbase->repeatOn = true;
  g_signal_connect (uiwidget->uidata.widget, "pressed",
      G_CALLBACK (uiButtonRepeatSignalHandler), bbase->presscb);
  g_signal_connect (uiwidget->uidata.widget, "released",
      G_CALLBACK (uiButtonRepeatSignalHandler), bbase->releasecb);
}

uibuttonstate_t
uiButtonGetState (uiwcont_t *uiwidget)
{
  uibutton_t    *uibutton;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-repeat")) {
    return BUTTON_OFF;
  }

  uibutton = uiwidget->uiint.uibutton;

  return uibutton->state;
}

/* internal routines */

static void
uiButtonSignalHandler (GtkButton *b, gpointer udata)
{
  uiwcont_t       *uiwidget = udata;
  uibuttonbase_t  *bbase;

  if (uiwidget == NULL) {
    return;
  }

  bbase = &uiwidget->uiint.uibuttonbase;

  if (bbase->repeatOn) {
    return;
  }
  if (bbase->cb == NULL) {
    return;
  }

  callbackHandler (bbase->cb);
}

static void
uiButtonRepeatSignalHandler (GtkButton *b, gpointer udata)
{
  callback_t *uicb = udata;

  callbackHandler (uicb);
}

