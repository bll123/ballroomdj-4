/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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

#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uibutton.h"

static void uiButtonSignalHandler (GtkButton *b, gpointer udata);
static void uiButtonRepeatSignalHandler (GtkButton *b, gpointer udata);

typedef struct uibutton {
  GtkWidget       *image;
} uibutton_t;

uiwcont_t *
uiCreateButton (const char *ident, callback_t *uicb, char *title, char *imagenm)
{
  uiwcont_t       *uiwidget;
  uibutton_t      *uibutton;
  uibuttonbase_t  *bbase;
  GtkWidget       *widget;

  uibutton = mdmalloc (sizeof (uibutton_t));
  uibutton->image = NULL;

  widget = gtk_button_new ();
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  if (imagenm != NULL) {
    GtkWidget   *image;
    char        tbuff [MAXPATHLEN];

    gtk_button_set_label (GTK_BUTTON (widget), "");
    /* relative path */
    pathbldMakePath (tbuff, sizeof (tbuff), imagenm, BDJ4_IMG_SVG_EXT,
        PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
    image = gtk_image_new_from_file (tbuff);
    gtk_button_set_image (GTK_BUTTON (widget), image);
    gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE); // macos
    gtk_widget_set_tooltip_text (widget, title);
    gtk_widget_set_valign (image, GTK_ALIGN_CENTER);
    uibutton->image = image;
  } else {
    gtk_button_set_label (GTK_BUTTON (widget), title);
  }

  uiwidget = uiwcontAlloc (WCONT_T_BUTTON, WCONT_T_BUTTON);
  uiwcontSetWidget (uiwidget, widget, NULL);
//  uiwidget->uidata.widget = widget;
//  uiwidget->uidata.packwidget = widget;
  uiwidget->uiint.uibutton = uibutton;

  bbase = &uiwidget->uiint.uibuttonbase;
  bbase->ident = ident;
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
uiButtonSetImagePosRight (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-image-pos-r")) {
    return;
  }

  gtk_button_set_image_position (GTK_BUTTON (uiwidget->uidata.widget),
      GTK_POS_RIGHT);
}

void
uiButtonSetImageMarginTop (uiwcont_t *uiwidget, int margin)
{
  uibutton_t    *uibutton;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-image-margin-top")) {
    return;
  }

  uibutton = uiwidget->uiint.uibutton;
  if (uibutton->image != NULL) {
    gtk_widget_set_margin_top (uibutton->image, uiBaseMarginSz * margin);
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

